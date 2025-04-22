# Makefile for hash

CC = gcc
CFLAGS = -g -O0

all : libringbuffer.a test.bin

ringbuffer.o: ringbuffer.c
	$(CC) $(CFLAGS) -c ringbuffer.c -o ringbuffer.o

libringbuffer.a: ringbuffer.o
	ar -cr libringbuffer.a ringbuffer.o

test.bin: libringbuffer.a
	$(CC) $(CFLAGS) main.c -L. -lringbuffer -o test.bin

clean:
	rm -rf *.o *.bin *.a
