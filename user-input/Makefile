CC?=x86_64-pc-linux-gnu-gcc
CCFLAGS?=-c
LIBS?=-lSDL2

user-input: keyboard.o mouse.o

%.o: %.c %.h
	$(CC) $(CCFLAGS) -o $@ $*.c $(LIBS)

