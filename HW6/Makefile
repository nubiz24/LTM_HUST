CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = 

# Windows-specific flags
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
    SERVER_EXE = server.exe
    CLIENT_EXE = client.exe
else
    SERVER_EXE = server
    CLIENT_EXE = client
endif

# Source files
SERVER_SRC = server.c
CLIENT_SRC = client.c
PROTOCOL_H = protocol.h

# Object files
SERVER_OBJ = server.o
CLIENT_OBJ = client.o

# Default target
all: $(SERVER_EXE) $(CLIENT_EXE)

# Build server
$(SERVER_EXE): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $(SERVER_EXE) $(SERVER_OBJ) $(LDFLAGS)

# Build client
$(CLIENT_EXE): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $(CLIENT_EXE) $(CLIENT_OBJ) $(LDFLAGS)

# Compile server
$(SERVER_OBJ): $(SERVER_SRC) $(PROTOCOL_H)
	$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $(SERVER_OBJ)

# Compile client
$(CLIENT_OBJ): $(CLIENT_SRC) $(PROTOCOL_H)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $(CLIENT_OBJ)

# Clean build artifacts
clean:
	rm -f $(SERVER_EXE) $(CLIENT_EXE) $(SERVER_OBJ) $(CLIENT_OBJ) *.log

# Run server (for testing)
run-server: $(SERVER_EXE)
	./$(SERVER_EXE)

# Run client (for testing)
run-client: $(CLIENT_EXE)
	./$(CLIENT_EXE)

.PHONY: all clean run-server run-client

