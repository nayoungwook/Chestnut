CC = gcc
CFLAGS = -Wall -Og -g -Icompiler/include -Ivm/include

COMPILER_SRC = $(wildcard compiler/*.c)
COMPILER_OBJS = $(COMPILER_SRC:.c=.o)

VM_SRC = $(wildcard vm/*.c)
VM_OBJS = $(VM_SRC:.c=.o)

SRC = $(wildcard src/*.c)
OBJS = $(SRC:.c=.o)

TARGET = chestnut
CLEAN_FILES = $(subst /,\,$(OBJS) $(COMPILER_OBJS) $(VM_OBJS) $(TARGET)$(EXE))

all: $(TARGET)$(EXE)

$(TARGET)$(EXE): $(OBJS) $(COMPILER_OBJS) $(VM_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	@cmd /Q /C "for %%F in ($(CLEAN_FILES)) do if exist %%F del /Q /F %%F"
