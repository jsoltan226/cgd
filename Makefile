CC?=cc
CFLAGS=-Wall -O2 -fPIC
DEPFLAGS?=-MMD -MP
CCLD?=cc
LDFLAGS?=-pie
LIBS?=-lSDL2 -lSDL2_image
STRIP?=strip
STRIPFLAGS?=-g -s

ECHO=echo
PRINTF=printf
RM=rm -f
TOUCH=touch -c
EXEC=exec
MKDIR=mkdir -p
RMDIR=rmdir

OBJDIR = obj
BINDIR = bin
SRCS=$(wildcard */*.c) $(wildcard ./*.c)
OBJS=$(patsubst %,$(OBJDIR)/%.o,$(shell find . -name "*.c" | xargs basename -as .c))
DEPS=$(patsubst %.o,%.d,$(OBJS))
EXE=$(BINDIR)/main

.PHONY: all release compile link strip clean update run br
.NOTPARALLEL: all release br

all: $(OBJDIR) $(BINDIR) compile link

release: CFLAGS += -O3 -Wall -Werror
release: $(OBJDIR) $(BINDIR) update compile link strip clean

br: all run

link: $(OBJS)
	@$(PRINTF) "CCLD 	%-20s %-20s\n" "$(EXE)" "<= $^"
	@$(CCLD) $(LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)

compile: $(OBJS)

$(OBJDIR):
	@$(ECHO) "MKDIR	$(OBJDIR)"
	@$(MKDIR) $(OBJDIR)

$(BINDIR):
	@$(ECHO) "MKDIR	$(BINDIR)"
	@$(MKDIR) $(BINDIR)

$(EXE): all

$(OBJDIR)/%.o: ./%.c Makefile
	@$(PRINTF) "CC 	%-20s %-20s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: */%.c Makefile
	@$(PRINTF) "CC 	%-20s %-20s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c -o $@ $<

strip:
	@$(ECHO) "STRIP	$(EXE)"
	@$(STRIP) $(STRIPFLAGS) $(EXE)

clean:
	@$(ECHO) "RM	$(OBJS) $(DEPS) $(EXE)"
	@$(RM) $(OBJS) $(DEPS) $(EXE)

update:
	@$(ECHO) "TOUCH	$(SRCS)"
	@$(TOUCH) $(SRCS)

run: $(EXE)
	@$(ECHO) "EXEC	$(EXE)"
	@$(EXEC) $(EXE)

-include $(DEPS)
