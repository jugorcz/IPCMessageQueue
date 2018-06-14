CC=gcc
CFLAGS=-std=gnu99 -Wall

%.o: %.c common.h
	$(CC) -c -o $@ $< $(CFLAGS)

all: server client clean

server: server.o
	$(CC) -o $@ $< $(CFLAGS)

client: client.o 
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -f *.o
	
cleanAll:
	rm -f *.o
	rm -f client
	rm -f server
