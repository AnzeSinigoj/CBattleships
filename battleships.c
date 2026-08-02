#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

//Global variables
uint8_t field_size;
uint8_t **field_1;
uint8_t **field_2;
uint8_t **current_player_field;
uint8_t **current_enemy_field;

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
void shoot(uint8_t x, uint8_t y);

int main () {
    srand(time(NULL));
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

    print_field();
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
    printf("\nEnter the size of the playing field [5,26]: ");
    
    while(1) {
        char input[32];
        if (scanf("%31s", input) != 1) { 
            printf("\nError! Bad read!\n");
            exit(1);
        }

        int value = atoi(input);
        if(value != 0) {
            if(value < 5 || value > 26) {
                printf("Error! The entered value is outside of the range of [5,26]!\n");
            } else {
                field_size = value;
                return;
            }
        } else {
            printf("Error! Wrong value entered!\n");
        }
        printf("Enter a new size [5-26]: ");
    }
}

/*
* This function prints the playing fields with the legend on both axis.
* Note: The friendly and enemy field are printed side by side separated by one \t
*/
void print_field() {
    clear_screen();
    const char *letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    print_x_axis:
        printf("   ");
        for(int i = 0; i < field_size; i++) printf("%3c", letters[i]);
        printf("\n");


    printf("Firendly field:");
    for (size_t i = 0; i < field_size; i++) printf("%3c", ' ');
    printf("Enemy field:\n");

    for(size_t i = 1; i <= field_size; i++) {
        if(i < 10) printf("0%d  ", (int)i);
        else printf("%d  ", (int)i);

        for(size_t j = 0; j < field_size; j++) {
            switch (current_player_field[i-1][j]) {
                case FULL:
                    printf(" # ");
                    break;
                case HIT:
                    printf(" X ");
                    break;
                default:
                    printf(" ~ ");
                    break;
            }
        }

        if(i < 10) printf("\t0%d  ", (int)i);
        else printf("\t%d  ", (int)i);

        for(size_t j = 0; j < field_size; j++) {
           switch (current_enemy_field[i-1][j]) {
                case HIT:
                    printf(" X ");
                    break;
                case MISS:
                    printf(" 0 ");
                    break;
                default:
                    printf(" ~ ");
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
    const uint8_t num_of_ships = field_size * field_size * density; 
    const uint8_t ship_sizes[] = {2,2,3,3,4,4,5,5,6,6};

    for (uint8_t i = 0; i < num_of_ships; i++) {
        const uint16_t MAX_ATTEMPTS = UINT16_MAX;
        uint16_t attempts = 0;

        while (1) {
            bool placed = false;
            uint8_t size = ship_sizes[i % 10];
            int8_t x = rand() % field_size;
            int8_t y = rand() % field_size;
            uint8_t direction = rand() % 4;

            switch (direction) {
                uint8_t free_blocks = 0;
                //North
                case 0:
                    if(y - (size-1) < 0) break;
                    free_blocks = 0;

                    for (uint8_t j = 0; j < size; j++) {
                        if (field[y-j][x] != FULL) free_blocks++;
                    }

                    if (free_blocks == size) {
                        for (uint8_t j = 0; j < size; j++) field[y-j][x] = FULL;
                        placed = true;
                    }

                    break;
                //South
                case 1:
                    if (y + (size-1) >= field_size) break;
                    free_blocks = 0;

                    for (uint8_t j = 0; j < size; j++) {
                        if (field[y+j][x] != FULL) free_blocks++;
                    }

                    if (free_blocks == size) {
                        for (uint8_t j = 0; j < size; j++) field[y+j][x] = FULL;
                        placed = true;
                    }

                    break;
                //East
                case 2:
                    if (x + (size-1) >= field_size) break;
                    free_blocks = 0;

                    for (uint8_t j = 0; j < size; j++) {
                        if (field[y][x+j] != FULL) free_blocks++;
                    }

                    if (free_blocks == size) {
                        for (uint8_t j = 0; j < size; j++) field[y][x+j] = FULL;
                        placed = true;
                    }

                    break;
                //West
                case 3:
                    if(x - (size-1) < 0) break;
                    free_blocks = 0;

                    for (uint8_t j = 0; j < size; j++) {
                        if (field[y][x-j] != FULL) free_blocks++;
                    }

                    if (free_blocks == size) {
                        for (uint8_t j = 0; j < size; j++) field[y][x-j] = FULL;
                        placed = true;
                    }

                    break;
            }
            attempts++;
            if(placed) break;
            if(attempts == MAX_ATTEMPTS){
                clear_screen();
                printf("Error! Unable to spawn ships! (Max attempts reached)\n");
                exit(-1);
            }
        }
    }
}

/*
* This function takes a shoot at the specified coordinates and sinks a ship if its located on the specified coordinates.
*/
void shoot(uint8_t x, uint8_t y) {
    if(current_enemy_field[y][x] == FULL) {
        current_enemy_field[y][x] = HIT;
        return;
    }
    current_enemy_field[y][x] = MISS;
}