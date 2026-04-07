CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -pedantic

# Executables
CLIENT  = client
SERVER  = server

# Sources
CLIENT_SRCS = globals.c utils.c display.c pieces.c moves.c players.c salon.c application.c clientVrai.c
SERVER_SRCS = globals.c utils.c display.c pieces.c moves.c players.c salon.c application.c serverVrai.c

# Objets
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

# Default
all: $(CLIENT) $(SERVER)

# Client
$(CLIENT): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Server
$(SERVER): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation générique
%.o: %.c chess.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f *.o $(CLIENT) $(SERVER)

# Run
run-client: $(CLIENT)
	./$(CLIENT)

run-server: $(SERVER)
	./$(SERVER)

.PHONY: all clean run-client run-server