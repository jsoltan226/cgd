CC?=gcc
CCFLAGS?=-c -Wall
LIBS?=-lSDL2 -lSDL2

main-menu: $(wildcard *.o)

%.o: %.c %.h
	$(CC) $(CCFLAGS) -o $@ $*.c $(LIBS)

