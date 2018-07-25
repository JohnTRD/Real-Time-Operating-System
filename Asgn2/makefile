CC = gcc
CFLAGS = -Wall -lm -std=c99 -pthread

all: Prg_1.out Prg_2.out

run: clean Prg_1.out 
	$ ./Prg_1.out

run2: clean Prg_2.out 
	$ ./Prg_2.out

Prg_1.out: Prg_1.c
	$(CC) $(CFLAGS) -o Prg_1.out Prg_1.c -lrt

Prg_2.out: Prg_2.c
	$(CC) $(CFLAGS) -o Prg_2.out Prg_2.c -lrt

clean:
	rm -rf *.o Prg_1.out Prg_2.out
