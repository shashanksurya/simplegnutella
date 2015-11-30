CC=gcc
CFLAGS= -g -Wall -pthread

all: g04

g04: g04.c
	$(CC) $(CFLAGS) -o g04 g04.c

clean:
	rm g04
