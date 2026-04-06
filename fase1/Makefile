CC = gcc
CFLAGS = -Wall -Wextra -fPIC
LDFLAGS = -shared

TARGET = libgini.so
SRC = gini.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: all
	python3 main.py

.PHONY: all clean run