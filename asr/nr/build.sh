#!/bin/bash

gcc -c -g -O0 fftwrap.c
gcc -c -g -O0 smallft.c
gcc -c -g -O0 denoise.c

gcc -c -g -O0 test.c

gcc test.o denoise.o smallft.o fftwrap.o -lm -o test.bin
