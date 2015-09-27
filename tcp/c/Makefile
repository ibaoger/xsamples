
CC=gcc
LD=ld
FLAGS=-Wall
LDFLAGS=-Wall

all: client client-select

client: utils.o client.o
	$(CC) $(FLAGS) -o client client.o utils.o

utils.o: utils.c
	$(CC) $(FLAGS) -c utils.c

client.o: client.c utils.h
	$(CC) $(FLAGS) -c client.c

client-select: utils.o client-select.o
	$(CC) $(FLAGS) -o client-select client-select.o utils.o

client-select.o: client-select.c utils.c
	$(CC) $(FLAGS) -c client-select.c

clean:
	rm -rf *.o client client-select
