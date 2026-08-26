CC = gcc
CFLAGS = -Wall -Og -g -Icompiler/include -Ivm/include

COMPILER_SRC = $(wildcard compiler/*.c)
COMPILER_OBJS = $(COMPILER_SRC:.c=.o)

VM_SRC = $(wildcard vm/*.c)
VM_OBJS = $(VM_SRC:.c=.o)

SRC = $(wildcard src/*.c)
OBJS = $(SRC:.c=.o)

TARGET = chestnut
CLEAN_FILES = $(OBJS) $(COMPILER_OBJS) $(VM_OBJS) $(TARGET)$(EXE)

ifeq ($(OS),Windows_NT)
EXE = .exe
else
EXE =
endif

all: $(TARGET)$(EXE)

$(TARGET)$(EXE): $(OBJS) $(COMPILER_OBJS) $(VM_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
ifeq ($(OS),Windows_NT)
	cmd /C "for %%f in ($(subst /,\,$(CLEAN_FILES))) do @if exist %%f del /Q %%f"
else
	rm -f $(CLEAN_FILES)
endif
