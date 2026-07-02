CC = gcc
CFLAGS = -lpthread -Wall -Wextra -std=c11 -g
DL_CFLAGS = $(shell pkg-config --cflags sdl3 sdl3-image sdl3-ttf sdl3-mixer)
SDL_LIBS = $(shell pkg-config --libs sdl3 sdl3-image sdl3-ttf sdl3-mixer)

chip-8: main.o chip-8.o
        $(CC) -o chip-8 main.o chip-8.o $(SDL_LIBS) -lpthread

main.o: main.c chip-8.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c main.c

chip-8.o: chip-8.c chip-8.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c chip-8.c

clean: 
	rm -f *.o chip-8
