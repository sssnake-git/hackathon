# Makefile for hash

CC = gcc
CFLAGS = -g -O0

all : libhash.a test.bin

hash.o: hash.c
	$(CC) $(CFLAGS) -c -o hash.o hash.c

libhash.a: hash.o
	ar -cr libhash.a hash.o

test.bin: libhash.a
	$(CC) $(CFLAGS) test.c -L. -lhash -o test.bin

clean:
	rm -rf *.o *.bin *.a
