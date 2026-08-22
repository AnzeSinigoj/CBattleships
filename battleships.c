#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

//Global variables
uint8_t field_size;
uint8_t **field_1;
uint8_t **field_2;
uint8_t **current_player_field;
uint8_t **current_enemy_field;
uint8_t player_1_score;
uint8_t player_2_score;
uint8_t current_player = 1;
uint8_t score_for_win;

struct point {
    uint8_t x;
    uint8_t y;
} typedef point;

typedef enum {
    EMPTY,
    FULL,
    HIT,
    MISS
} state;

//Screen functions
void clear_screen();
void print_menu();
void print_field();

//Field operations
void spawn_ships(uint8_t **field);
bool shoot(point p);

//User input gathering and processing
point read_strike_coordinates();

//Gameplay functions
bool check_for_win();
void play_turn();

int main () {
    srand(time(NULL));
    //Turn on green color
    printf("\033[32m");
    print_menu();

    field_1 = malloc(field_size*sizeof(uint8_t*));
    field_2 = malloc(field_size*sizeof(uint8_t*));

    for(size_t i = 0; i < field_size; i++)  {
        field_1[i] = malloc(field_size * sizeof(uint8_t));
        field_2[i] = malloc(field_size * sizeof(uint8_t));
        for (size_t j = 0; j < field_size; j++) {
            field_1[i][j] = EMPTY;
            field_2[i][j] = EMPTY;
        }
    }
    
    spawn_ships(field_1);
    spawn_ships(field_2);

    current_player_field = field_1;
    current_enemy_field = field_2;

    while (1) {
        if(current_player_field == current_enemy_field) {
            clear_screen();
            printf("Error! Field missmatch!\n");
        }

        switch (current_player) {
            case 1:
                play_turn();
                if(check_for_win()) {
                    //Reset back to original color
                    printf("\033[0m");
                    exit(0);
                }

                //Pass turn 
                current_player = 2;
                current_player_field = field_2;
                current_enemy_field = field_1;

                //Hide first player's ships
                clear_screen();
                printf("Player %d: press any key to play your turn...", current_player);
                getchar();
                break;
            case 2:
                play_turn();
                if(check_for_win()) {
                    //Reset back to original color
                    printf("\033[0m");
                    exit(0);
                }

                //Pass turn
                current_player = 1;
                current_player_field = field_1;
                current_enemy_field = field_2;

                //Hide second player's ships
                clear_screen();
                printf("Player %d: press any key to play your turn...", current_player);
                getchar();
                break;
            default:
                clear_screen();
                printf("Error! Failed to determine turns!\n");
                exit(-1);
        }
    }

}

// This function clears the screen when called
void clear_screen() {
    printf("\033[H");
    printf("\033[2J");
}

/* 
* This function prints the main menu and take user input for the size of the playing field.
* The function writes the value into the field_size global varriable 
*/ 
void print_menu() {
    clear_screen();
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("|                 Battleships                 |\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("\nEnter the size of the playing field [6,26]: ");
    
    while(1) {
        char input[32];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nError! Bad read!\n");
            exit(1);
        }

        int value = atoi(input);
        if(value != 0) {
            if(value < 6 || value > 26) {
                printf("Error! The entered value is outside of the range of [6,26]!\n");
            } else {
                field_size = value;
                return;
            }
        } else {
            printf("Error! Wrong value entered!\n");
        }
        printf("Enter a new size [6,26]: ");
    }
}

/*
* This function prints the playing fields with the legend on both axis.
* Note: The friendly and enemy field are printed side by side separated by one \t
*/
void print_field() {
    clear_screen();
    const char *letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    printf("Your field:%*cEnemy's field:\n\n", field_size*3-1, ' ');

    printf("%3c", ' ');
    for(int i = 0; i < field_size; i++) printf("%3c", letters[i]);

    printf("\t%3c", ' ');
    for(int i = 0; i < field_size; i++) printf("%3c", letters[i]);
    printf("\n");
    
    for(size_t i = 1; i <= field_size; i++) {
        if(i < 10) printf("0%-2d", (int)i);
        else printf("%-3d", (int)i);

        for(size_t j = 0; j < field_size; j++) {
            switch (current_player_field[i-1][j]) {
                case FULL:
                    printf("%3c", '#');
                    break;
                case HIT:
                    printf("%3c", 'X');
                    break;
                default:
                    printf("%3c", '~');
                    break;
            }
        }

        if(i < 10) printf("\t0%-2d", (int)i);
        else printf("\t%-3d", (int)i);

        for(size_t j = 0; j < field_size; j++) {

           switch (current_enemy_field[i-1][j]) {
                case HIT:
                    printf("%3c", 'X');
                    break;
                case MISS: 
                    printf("%3c", '0');
                    break;
                default:
                    printf("%3c", '~');
                    break;
            }
        }

        printf("\n");
    }        
}

/*
* This function spawns ships into the passed field.
* The number of spawned ships is determined by the size of the field.
* The function works by bruteforce and if its unable to place a ship in 2^16 iterations it stops the program.
*/
void spawn_ships(uint8_t **field) {
    const float density = 0.2;
    const uint8_t ship_budget = field_size * field_size * density; 
    const uint8_t ship_sizes[] = {2,2,3,3,4,4,5,5,6,6};
    score_for_win = ship_budget;
    uint8_t budget_used = 0;
    uint8_t ship_index = 0;

    while (budget_used < ship_budget) {
        const uint16_t MAX_ATTEMPTS = UINT16_MAX;
        const uint8_t size = ship_sizes[ship_index % 10];
        uint16_t attempts = 0;
        bool placed = false;

        if (size + 2 > field_size) {
            ship_index++;
            continue;
        }

        while (!placed) {
            const uint8_t padding = size*2 + 6; //Around the ship plus 2x3 top and bottom
            const uint8_t size_plus_padding = size + padding;

            int8_t x = rand() % field_size;
            int8_t y = rand() % field_size;
            uint8_t direction = rand() % 4;

            switch (direction) {
                uint8_t free_spaces = 0;
                //North
                case 0:
                    if(y - (size-1) < 0) break;
                    free_spaces = 0;

                    for (int16_t j = y+1; j >= y-(size); j--) {
                        if(j < 0 || j >= field_size) break;
                        
                        for (int16_t k = x-1; k <= x+1; k++) {
                            if(k < 0 || k >= field_size) {
                                free_spaces++;
                                continue;
                            }
                            if (field[j][k] != FULL) free_spaces++; 
                        }
                    }

                    if (free_spaces == size_plus_padding ) {
                        for (uint8_t j = 0; j < size; j++) field[y-j][x] = FULL;
                        placed = true;
                    }

                    break;
                //South
                case 1:
                    if (y + (size-1) >= field_size) break;
                    free_spaces = 0;

                    for (int16_t j = y-1; j <= y+(size); j++) {
                        if(j < 0 || j >= field_size) break;
                        
                        for (int16_t k = x-1; k <= x+1; k++) {
                            if(k < 0 || k >= field_size) {
                                free_spaces++;
                                continue;
                            }
                            if (field[j][k] != FULL) free_spaces++; 
                        }
                    }

                    if (free_spaces == size_plus_padding) {
                        for (uint8_t j = 0; j < size; j++) field[y+j][x] = FULL;
                        placed = true;
                    }

                    break;
                //East
                case 2:
                    if (x + (size-1) >= field_size) break;
                    free_spaces = 0;
                    bool failed = false;

                    for (int16_t j = y-1; j <= y+1 && !failed; j++) {
                        if(j < 0 || j >= field_size) {
                            free_spaces++;
                            continue;
                        }
                        
                        for (int16_t k = x-1; k <= x+size; k++) {
                            if(k < 0 || k >= field_size) {
                                failed = true;
                                break;
                            }
                            if (field[j][k] != FULL) free_spaces++; 
                        }
                    }

                    if (free_spaces == size_plus_padding) {
                        for (uint8_t j = 0; j < size; j++) field[y][x+j] = FULL;
                        placed = true;
                    }

                    break;
                //West
                case 3:
                    if(x - (size-1) < 0) break;
                    free_spaces = 0;
                    failed = false;

                    for (int16_t j = y-1; j <= y+1 && !failed; j++) {
                        if(j < 0 || j >= field_size) {
                            free_spaces++;
                            continue;
                        }
                        
                        for (int16_t k = x+1; k >= x-size; k--) {
                            if(k < 0 || k >= field_size) {
                                failed = true;
                                break;
                            }
                            if (field[j][k] != FULL) free_spaces++; 
                        }
                    }

                    if (free_spaces == size_plus_padding) {
                        for (uint8_t j = 0; j < size; j++) field[y][x-j] = FULL;
                        placed = true;
                    }

                    break;
            }

            attempts++;
            if(attempts == MAX_ATTEMPTS){
                clear_screen();
                printf("Error! Unable to spawn ships! (Max attempts reached)\n");
                exit(-1);
            }
        }
        budget_used += size;
        ship_index++;
    }
}

/*
* This function takes a shoot at the specified coordinates and sinks a ship if its located on the specified coordinates.
* Returns true if a ship was hit and false if its a miss.
*/
bool shoot(point p) {
    if(current_enemy_field[p.y][p.x] == FULL) {
        current_enemy_field[p.y][p.x] = HIT;
        return true;
    }
    current_enemy_field[p.y][p.x] = MISS;
    return false;
}

/*
* This function reads user input and parses it to determine user-specified coordinates
* Returns a point struct, in case it fails to parse input it returns a struct of {UINT8_MAX, UINT8_MAX}.
* Accepts input in this format: "LetterNumber" ex. A12
*/
point read_strike_coordinates() {
    char buffer[8];
    fgets(buffer, sizeof(buffer), stdin);

    const char *letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char letter = buffer[0];
    char numbers_unparsed[] = {buffer[1], buffer[2], '\0'};
    uint8_t numbers = atoi(numbers_unparsed);
    uint8_t index_stopped = UINT8_MAX;

    //Convert to upper case
    if(letter > 'Z') letter -= 32;

    for(uint8_t i = 0; i < 26; i++) {
        if (letter == letters[i]) {
            index_stopped = i;
            break;
        }
    }

    if(index_stopped == UINT8_MAX) {
        printf("Error! The entered symbol is not a letter in the range of A-Z!\n");
        goto fail;
    }

    if(index_stopped >= field_size) {
        printf("Error! The entered longitude is outside the war zone!\n");
        goto fail;
    }

    if(numbers <= 0 || numbers > 26) {
        printf("Error! The entered number does not fit the range of [1,26]!\n");
        goto fail;
    }

    return (point){index_stopped, numbers-1}; //Adjust numbers for zero indexing

fail:
    return (point){UINT8_MAX, UINT8_MAX};
}

/*
* This function checks if any player has achvived a win and prints the victory screen.
* Returns true if a player won and false if no player has won yet.
*/
bool check_for_win(){
       if(player_1_score >= score_for_win) {
        clear_screen();
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        printf("|                 Player 1 wins!              |\n");
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        printf("Congratulations capitan your orders helped to defeat the enemy!\n");
        return true;
    } else if (player_2_score >= score_for_win) {
        clear_screen();
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        printf("|                 Player 2 wins!              |\n");    
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        printf("Congratulations capitan your orders helped to defeat the enemy!\n");
        return true;
    } else {
        return false;
    }
}

/*
* This function plays a turn for the player who has the turn right now.
*/
void play_turn() {
    print_field();
    point coordinates;
    while (true) {
        printf("\nEnter strike coordinates: ");
        coordinates = read_strike_coordinates();
            
        if((coordinates.x == UINT8_MAX) || (coordinates.y == UINT8_MAX)) {
            printf("Error! Failed to parse coordinates!\n");
            continue;
        } else break;
    }   

    bool status = shoot(coordinates);
    print_field(); //Refresh the screen to display the new shoot status
    if (status) {
        if(current_player == 1) player_1_score++;
        else player_2_score++;
        printf("Hit! An enemy ship was hit in the strike.\n");
    }
    else printf("Miss! Shot landed in the water.\n");

    printf("\nPress any key to continue...");
    getchar();
}