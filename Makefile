LIBS=-lSDL2 -lSDL2_image
CC=x86_64-pc-linux-gnu-gcc
CCFLAGS=-c -ggdb -O2 -Wall -I. -I..
LINKER_FLAGS=-ggdb -O2 -Wall -I. -I..
MAKEFLAGS=-j2
export

FINAL_INGREDIENTS=util.o main.o main-menu/parallax-bg.o user-input/keyboard.o user-input/mouse.o main-menu/buttons.o

all:
	$(MAKE) -C main-menu
	$(MAKE) -C user-input
	$(MAKE) main

main: $(FINAL_INGREDIENTS)
	$(CC) $(LINKER_FLAGS) -o main $(FINAL_INGREDIENTS) $(LIBS)

main.o: main.c config.h
	$(CC) $(CCFLAGS) -o main.o main.c $(LIBS)

util.o: util.c util.h
	$(CC) $(CCFLAGS) -o util.o util.c $(LIBS)

clean: all
	rm -f *.o */*.o
