CC = gcc
CFLAGS = -g -pthread

CLIENT_SRC = client.c
CLIENT_TARGET = client

SERVER_SRC = tictactoe.c
SERVER_TARGET = server

.PHONY: all clean

all: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $^ -o $@

$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET)
