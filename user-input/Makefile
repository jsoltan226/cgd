CC?=gcc
CCFLAGS?=-c
LIBS?=-lSDL2

user-input: keyboard.o mouse.o

keyboard.o: keyboard.c keyboard.h
	$(CC) $(CCFLAGS) -o keyboard.o keyboard.c $(LIBS)

mouse.o: mouse.c mouse.h
	$(CC) $(CCFLAGS) -o mouse.o mouse.c $(LIBS)
