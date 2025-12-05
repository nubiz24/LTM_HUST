CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -pthread

# Tên file thực thi
SERVER = server
CLIENT = client

# Source files
SERVER_SRC = server.c
CLIENT_SRC = client.c

.PHONY: all clean

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC) $(LDFLAGS)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC) $(LDFLAGS)

clean:
	rm -f $(SERVER) $(CLIENT) auth.log logged_in.txt failed_attempts.txt

