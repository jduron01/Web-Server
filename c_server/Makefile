CC = gcc
CFLAGS = -O3 -march=native -flto

.PHONY: all clean

all: server

server: server.o
	$(CC) $(CFLAGS) -o server server.o
server.o: server.c
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f server *.o
