
CC = gcc
CFLAGS = -Wall -Og -Iinclude

SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)

TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJS)
