
CC = gcc
CFLAGS = -Wall -Og

SRC = $(wildcard *.c)
OBJS = $(SRC: %.c=%.o)

TARGET = main

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

