battleships: battleships.c
	gcc battleships.c -o battleships

run: battleships
	./battleships

clean:
	rm -fv battleships

