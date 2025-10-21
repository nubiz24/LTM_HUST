CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
TARGET = resolver
SOURCES = resolver.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean zip

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)



