CC=gcc
CFLAGS=-O2 -Wall

# Windows needs -lws2_32
WINLIBS=
ifeq ($(OS),Windows_NT)
	WINLIBS=-lws2_32
endif

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(WINLIBS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(WINLIBS)

clean:
	rm -f server client *.o 2>/dev/null || del /Q server.exe client.exe 2>nul || true

.PHONY: all clean
