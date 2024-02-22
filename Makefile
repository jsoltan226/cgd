PLATFORM?=gnu_linux

PRINTF?=printf
MKDIR?=mkdir -p
STRIP?=strip
RM?=rm -rf
EXEC=exec

HOSTCC?=cc
CC?=cc
CFLAGS?=-I. -I.. -I/usr/include/freetype2 -pipe -fPIC
CXX?=c++
CXXFLAGS?=$(CFLAGS)
CCLD?=$(CC)
CXXLD?=$(CXX)
LDFLAGS?=
LIBS?=-lSDL2 -lpng -lfreetype

OBJDIR?=obj
BINDIR?=bin

EXEPREFIX?=
EXESUFFIX?=
EXE?=$(BINDIR)/$(EXEPREFIX)main$(EXESUFFIX)

DEP_CFLAGS?=-MP -MMD
COMMON_CFLAGS?=$(CFLAGS) $(DEP_CFLAGS)
DEBUG_CFLAGS?=-O0 -ggdb -Wall -DDEBUG -URELEASE
RELEASE_CFLAGS?=-O3 -Wall -Werror -UDEBUG -DRELEASE

exceptional_srcdirs=./platform ./.git .
exceptional_srcs=test.c tmp.c find-max-strlen.c
srcdirs=$(filter-out $(exceptional_srcdirs), $(shell find . -maxdepth 1 -type d))
_SRCS=$(shell find $(srcdirs) -type f -name "*.c")
_SRCS+=$(wildcard *.c)

ifeq ($(PLATFORM), gnu_linux)
_SRCS+=$(shell find ./platform/linux -type f -name "*.c")
else ifeq ($(PLATFORM), windows)
_SRCS+=$(shell find ./platform/windows -type f -name "*.c")
endif

SRCS=$(filter-out $(exceptional_srcs),$(_SRCS))

OBJS=$(patsubst %.c,$(OBJDIR)/%.c.o,$(shell basename -a $(SRCS)))
DEPS=$(patsubst %.o,%.d,$(OBJS))

FIND_MAX_STRLEN_EXE=$(BINDIR)/$(EXEPREFIX)find-max-strlen$(EXESUFFIX)
# Don't bother reading; this will just get the length of the longest string
# possible in the middle column of most output lines
# CC	obj/menu.c.o		<= gui/menu.c
#            ^
# (this one) ^
#
# If the compiled C program meant for this exists it will use it,
# and if not it will use a slower alternative made out of standard unix commands.
_max_printf_strlen=$(shell if [ -f $(FIND_MAX_STRLEN_EXE) ]; then $(FIND_MAX_STRLEN_EXE) $(OBJS) $(FIND_MAX_STRLEN_EXE); else MAX_STRLEN=0; for i in $(OBJS) $(FIND_MAX_STRLEN_EXE); do strlen=$$(echo $$i | wc -m); [ $$strlen -gt $$MAX_STRLEN ] && MAX_STRLEN=$$strlen; done; echo $$((MAX_STRLEN - 1)); fi)

.PHONY: all

all: $(BINDIR) $(OBJDIR) $(FIND_MAX_STRLEN_EXE) $(EXE)

.NOTPARALLEL: $(FIND_MAX_STRLEN_EXE)
$(FIND_MAX_STRLEN_EXE):
	@$(PRINTF) "HOSTCC 	%-$(_max_printf_strlen)s %-$(_max_printf_strlen)s\n" "$(FIND_MAX_STRLEN_EXE)" "<= find-max-strlen.c"
	@$(HOSTCC) -fPIC -O3 -pipe -o $(FIND_MAX_STRLEN_EXE) find-max-strlen.c

$(EXE): $(OBJS)
	@$(PRINTF) "CCLD 	%-$(_max_printf_strlen)s %-$(_max_printf_strlen)s\n" "$(EXE)" "<= $^"
	@$(CCLD) $(LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)

$(OBJDIR)/%.c.o: %.c
	@$(PRINTF) "CC 	%-$(_max_printf_strlen)s %-$(_max_printf_strlen)s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(DEBUG_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.c.o: */%.c
	@$(PRINTF) "CC 	%-$(_max_printf_strlen)s %-$(_max_printf_strlen)s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(DEBUG_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.c.o: */*/%.c
	@$(PRINTF) "CC 	%-$(_max_printf_strlen)s %-$(_max_printf_strlen)s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(DEBUG_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.c.o: */*/*/%.c
	@$(PRINTF) "CC 	%-$(_max_printf_strlen)s %-$(_max_printf_strlen)s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(DEBUG_CFLAGS) $(CFLAGS) -c -o $@ $<

ifeq ($(PLATFORM), gnu_linux)
$(OBJDIR)/%.c.o: platform/linux/%.c Makefile
	@$(PRINTF) "CC 	%-$(_max_printf_strlen)s %-$(_max_printf_strlen)s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<

else ifeq ($(PLATFORM), windows)
$(OBJDIR)/%.c.o: platform/windows/%.c Makefile
	@$(PRINTF) "CC 	%-$(_max_printf_strlen)s %-$(_max_printf_strlen)s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<

endif

$(OBJDIR):
	@$(PRINTF) "MKDIR 	%-$(_max_printf_strlen)s\n" "$(OBJDIR)"
	@$(MKDIR) $(OBJDIR)

$(BINDIR):
	@$(PRINTF) "MKDIR 	%-$(_max_printf_strlen)s\n" "$(BINDIR)"
	@$(MKDIR) $(BINDIR)

clean:
	@$(PRINTF) "RM 	%-$(_max_printf_strlen)s\n" "$(OBJS) $(DEPS) $(EXE) $(FIND_MAX_STRLEN_EXE)"
	@$(RM) $(OBJS) $(DEPS) $(EXE) $(FIND_MAX_STRLEN_EXE)

mostlyclean:
	@$(PRINTF) "RM 	%-$(_max_printf_strlen)s\n" "$(OBJS) $(DEPS)"
	@$(RM) $(OBJS) $(DEPS)

run:
	@$(PRINTF) "EXEC 	%-$(_max_printf_strlen)s\n" "$(EXE)"
	@$(EXEC) $(EXE)

-include $(DEPS)
