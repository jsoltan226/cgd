LIBS=-lSDL2 -lSDL2_image
CC=x86_64-pc-linux-gnu-gcc
CFLAGS=-c -Wall -O2 -ggdb
LD=x86_64-pc-linux-gnu-gcc
LDFLAGS=-Wall
STRIP=strip
STRIPFLAGS=-g -s

ECHO=echo
RM=rm
TOUCH=touch

SRCS=$(wildcard */*.c) $(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))
EXE=./main

all: compile link

debug:
	@$(ECHO) "Building debug version"
	@CFLAGS="$(CFLAGS) -DASSERTIONS"
	@$(MAKE) all

link: $(OBJS)
	@$(ECHO) "CCLD	$(OBJS)"
	@$(LD) $(LDFLAGS) -o main $(OBJS) $(LIBS)

compile: $(OBJS)
%.o: %.c
	@$(ECHO) "CC	$@"
	@$(CC) $(CFLAGS) -o $@ $^

strip:
	@$(ECHO) "STRIP	$(EXE)"
	@$(STRIP) $(STRIPFLAGS) $(EXE)

clean:
	@$(ECHO) "RM	$(OBJS)"
	@$(RM) -f $(OBJS)

update:
	@$(ECHO) "TOUCH	$(SRCS)"
	@$(TOUCH) $(SRCS)

run: $(EXE)
	@$(ECHO) "RUN	$(EXE)"
	@$(EXE)
