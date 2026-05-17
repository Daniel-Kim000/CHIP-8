CC = gcc
CFLAGS = -Wall -Wextra -std=c11

chip-8: main.o chip-8.o
	$(CC) $(CFLAGS) -o chip-8 main.o chip-8.o

main.o: main.c chip-8.h
	$(CC) $(CFLAGS) -c main.c

chip-8.o: chip-8.c chip-8.h
	$(CC) $(CFLAGS) -c chip-8.c
clean: 
	rm -f *.o chip-8
