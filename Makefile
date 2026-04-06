CC = gcc
CFLAGS = -Wall -fPIC
CFLAGS_DEBUG = $(CFLAGS) -g -O0

SRC_C = gini.c
SRC_ASM = gini_asm.s
LIB = libgini.so

.PHONY: all debug clean

all: $(LIB)

$(LIB): $(SRC_C) $(SRC_ASM)
	$(CC) $(CFLAGS) -shared -o $@ $^

debug: $(SRC_C) $(SRC_ASM)
	$(CC) $(CFLAGS_DEBUG) -shared -o $(LIB) $^

clean:
	rm -f $(LIB) *.o
