CC = gcc
CFLAGS = -Wall -fPIC
LIB = libgini.so

all: $(LIB)

$(LIB): gini.c
	$(CC) $(CFLAGS) -shared -o $@ $^

clean:
	rm -f $(LIB) *.o
