CC = gcc
CFLAGS = -Wall -Wextra -O3

.PHONY: all clean

all: server

server: server.o
	$(CC) $(CFLAGS) -o server server.o -lmagic
server.o: server.c
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f server *.o
