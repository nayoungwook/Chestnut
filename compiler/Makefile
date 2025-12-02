
CC = gcc
CFLAGS = -Wall -Og

SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
