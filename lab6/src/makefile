CC=gcc
CFLAGS=-I. -Wall -Wextra -pthread
LDLIBS_NETMATH=-L. -lnetmath

.PHONY: all clean

all: server client

libnetmath.a: libnetmath.o
	ar rcs libnetmath.a libnetmath.o

libnetmath.o: libnetmath.h
	$(CC) -o libnetmath.o -c libnetmath.c $(CFLAGS)

server: server.c libnetmath.a
	$(CC) -o server server.c $(CFLAGS) $(LDLIBS_NETMATH)

client: client.c libnetmath.a
	$(CC) -o client client.c $(CFLAGS) $(LDLIBS_NETMATH)

clean:
	rm -f libnetmath.o libnetmath.a server client