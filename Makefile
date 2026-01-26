CC = gcc
CFLAGS = -Wall -Og -g -Icompiler/include

COMPILER_SRC = $(wildcard compiler/*.c)
COMPILER_OBJS = $(COMPILER_SRC:.c=.o)

SRC = $(wildcard src/*.c)
OBJS = $(SRC:.c=.o)

TARGET = Chestnut

all: $(TARGET)

$(TARGET): $(OBJS) $(COMPILER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJS) $(COMPILER_OBJS) $(TARGET)
