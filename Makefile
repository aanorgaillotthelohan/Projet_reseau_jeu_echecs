CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -pedantic -lpthread -lm

# Executables
CLIENT  = client
SERVER  = server

# Sources
CLIENT_SRCS = temp_client.c globals.c moves.c pieces.c players.c display.c utils.c application.c 
SERVER_SRCS = temp_serv.c globals.c moves.c pieces.c players.c display.c utils.c application.c salon.c

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