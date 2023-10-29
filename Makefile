CC?=x86_64-pc-linux-gnu-gcc
CFLAGS=-Wall -O2 -fPIC -ggdb
DEPFLAGS?=-MMD -MP
CCLD?=x86_64-pc-linux-gnu-gcc
LDFLAGS?=-pie
LIBS?=-lSDL2 -lSDL2_image
STRIP?=strip
STRIPFLAGS?=-g -s

ECHO=echo
RM=rm -f
TOUCH=touch -c
EXEC=exec
MKDIR=mkdir -p
RMDIR=rmdir

OBJDIR = obj
SRCS=$(wildcard */*.c) $(wildcard ./*.c)
OBJS=$(patsubst %,$(OBJDIR)/%.o,$(shell find . -name "*.c" | xargs basename -as .c))
DEPS=$(patsubst %.o,%.d,$(OBJS))
EXE=./main

.PHONY: all release compile link strip clean update run
.NOTPARALLEL: all release debug br

all: $(OBJDIR) debug compile link

release: $(OBJDIR) update compile link strip clean
	@CFLAGS="$(CFLAGS) -O3 -Wall -Werror"

debug:
	@CFLAGS="-ggdb $(CFLAGS) -DASSERTIONS"

br: all run

link: $(OBJS)
	@$(ECHO) "CCLD	$(OBJS)"
	@$(CCLD) $(LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)

compile: $(OBJS)

$(OBJDIR):
	@$(ECHO) "MKDIR	$(OBJDIR)"
	@$(MKDIR) $(OBJDIR)

$(EXEC): all

$(OBJDIR)/%.o: ./%.c Makefile
	@$(ECHO) "CC	$<"
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: */%.c Makefile
	@$(ECHO) "CC	$<"
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c -o $@ $<

strip:
	@$(ECHO) "STRIP	$(EXE)"
	@$(STRIP) $(STRIPFLAGS) $(EXE)

clean:
	@$(ECHO) "RM	$(OBJS) $(DEPS)"
	@$(RM) $(OBJS) $(DEPS)
	@$(ECHO) "RMDIR	$(OBJDIR)"
	@$(RMDIR) $(OBJDIR)

update:
	@$(ECHO) "TOUCH	$(SRCS)"
	@$(TOUCH) $(SRCS)

run: $(EXE)
	@$(ECHO) "EXEC	$(EXE)"
	@$(EXEC) $(EXE)

-include $(DEPS)
