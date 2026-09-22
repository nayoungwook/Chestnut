CC = gcc
SDL3_ROOT ?= engine

# for debug
CFLAGS = -Wall -Og -g -msse4.1 -Icompiler/include -Ivm/include -Iengine/include -Isrc/include -Isrc
# CFLAGS = -Wall -O2 -msse4.1 -Icompiler/include -Ivm/include -Iengine/include -Isrc/include -Isrc

COMPILER_SRC = $(wildcard compiler/*.c)
COMPILER_OBJS = $(COMPILER_SRC:.c=.o)

VM_SRC = $(wildcard vm/*.c)
VM_OBJS = $(VM_SRC:.c=.o)

ENGINE_SRC = $(wildcard engine/*.c)
ENGINE_OBJS = $(ENGINE_SRC:.c=.o)

SRC = $(wildcard src/*.c)
OBJS = $(SRC:.c=.o)

TARGET = chestnut
CLEAN_FILES = $(OBJS) $(COMPILER_OBJS) $(VM_OBJS) $(ENGINE_OBJS) $(TARGET)$(EXE)

LDLIBS += -lm -pthread

ifeq ($(OS),Windows_NT)
EXE = .exe
CFLAGS += -I$(SDL3_ROOT)/include
LDFLAGS += -L$(SDL3_ROOT)/lib
LDLIBS += -lSDL3
else
EXE =
PKG_CONFIG ?= pkg-config
CFLAGS += $(shell $(PKG_CONFIG) --cflags sdl3 2>/dev/null)
LDLIBS += $(shell $(PKG_CONFIG) --libs sdl3 2>/dev/null)
endif

all: $(TARGET)$(EXE)

$(TARGET)$(EXE): $(OBJS) $(COMPILER_OBJS) $(VM_OBJS) $(ENGINE_OBJS) | check-sdl3
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean check-sdl3

check-sdl3:
ifneq ($(OS),Windows_NT)
	@$(PKG_CONFIG) --exists sdl3 || { echo "SDL3 development files not found; install SDL3 for this Linux environment and verify: pkg-config --modversion sdl3"; exit 1; }
endif

clean:
ifeq ($(OS),Windows_NT)
	cmd /C "for %%f in ($(subst /,\,$(CLEAN_FILES))) do @if exist %%f del /Q %%f"
else
	rm -f $(CLEAN_FILES)
endif
