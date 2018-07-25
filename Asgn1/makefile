# 48450 - Real-time Operating Systems Autumn 2018
# Author: John Trinidad
# Student ID: 99141145
CC = gcc
CFLAGS = -Wall -lm -pthread

all: clean Prg_1.o Prg_2.o

run: all 
	$ ./Prg_1.o && ./Prg_2.o

Prg_1.o: Prg_1.c
	$(CC) $(CFLAGS) -o Prg_1.o  Prg_1.c -lrt


Prg_2.o: Prg_2.c
	$(CC) $(CFLAGS) -o Prg_2.o  Prg_2.c -lrt

clean:
	rm -rf *.o Prg_1.o Prg_2.o


