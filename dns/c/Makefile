
CC=gcc
LD=ld
FLAGS=-Wall
LDFLAGS=-Wall

all: gethostbyname-nonblock

gethostbyname-nonblock: gethostbyname-nonblock.o gethostbyname.o
	$(CC) $(FLAGS) -o gethostbyname-nonblock gethostbyname-nonblock.o gethostbyname.o

gethostbyname-nonblock.o: gethostbyname-nonblock.c gethostbyname.h
	$(CC) $(FLAGS) -c gethostbyname-nonblock.c

gethostbyname.o: gethostbyname.c gethostbyname.h
	$(CC) $(FLAGS) -c gethostbyname.c

clean:
	rm -rf *.o gethostbyname-nonblock

