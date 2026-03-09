CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -pedantic
TARGET  = chess

SRCS    = main.c globals.c utils.c display.c pieces.c moves.c players.c showcase.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c chess.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
