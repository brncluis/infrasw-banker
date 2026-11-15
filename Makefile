banker: banker.c
	gcc -Wall -Wextra -std=c99 -pedantic -g -o banker banker.c

clean:
	rm -f banker
