PREFIX?=/usr/local
BIN_INSTALLDIR?=$(PREFIX)/bin
LIB_INSTALLDIR?=$(PREFIX)/lib

LIBDIR=lib
BINDIR=bin
OBJDIR=obj

EXEPREFIX?=
EXESUFFIX?=

custom_INCLUDE_DIRS=
custom_STATIC_LIB_LINKARGS?=
custom_SHARED_LIB_LINKARGS?=

# Custom lib dependencies

custom_CFLAGS+=$(custom_INCLUDE_DIRS)
custom_LIBS+=$(custom_STATIC_LIB_LINKARGS) $(custom_SHARED_LIB_LINKARGS)

# System lib dependencies
pkgconf_DEPS?=sdl2 libpng freetype2
pkgconf_LIBS=$(shell pkgconf --libs $(pkgconf_DEPS)) 
pkgconf_CFLAGS=$(shell pkgconf --cflags $(pkgconf_DEPS)) 

CC?=cc
CXX?=c++
COMMON_CFLAGS=-Wall -fPIC $(pkgconf_CFLAGS) $(custom_CFLAGS) -I.
COMMON_CXXFLAGS=$(COMMON_CFLAGS)
DEPFLAGS?=-MMD -MP
LD=$(CXX)
LDFLAGS?=-pie
LIBS?=$(pkgconf_LIBS) $(custom_LIBS)
STRIP?=strip
STRIPFLAGS?=-g -s

# System commands used throughout the Makefile
ECHO?=echo
PRINTF?=printf
RM?=rm -f
TOUCH?=touch -c
EXEC?=exec
MKDIR?=mkdir -p
RMDIR?=rmdir --ignore-fail-on-empty
INSTALL?=install

C_SRCS=$(shell find . -name "*.c")
CXX_SRCS=$(shell find . -name "*.cpp")
C_OBJS=$(patsubst %,$(OBJDIR)/%.o,$(shell echo "$(C_SRCS)" | xargs basename -a))
CXX_OBJS=$(patsubst %,$(OBJDIR)/%.o,$(shell echo "$(CXX_SRCS)" | xargs basename -a "$(CXX_SRCS)"))
DEPFILES=$(patsubst %.o,%.d,$(C_OBJS) $(CXX_OBJS))
EXE=$(BINDIR)/$(EXEPREFIX)main$(EXESUFFIX)
TESTEXE=$(EXE)

_LONGEST_FILE_NAME_LENGTH=$(shell find . | awk '{print length}' | sort -n | tail -n1)

.PHONY: all release compile link strip clean mostlyclean run br test execute_file
.NOTPARALLEL: all release br

all: CFLAGS = -ggdb -O0 -Wall
all: CXXFLAGS = -ggdb -O0 -Wall
all: compile link

release: CFLAGS = -O3 -Wall -Werror
release: CXXFLAGS = -O3 -Wall -Werror
release: clean compile link strip mostlyclean test

br: all run

link: $(C_OBJS) $(CXX_OBJS) $(LIBS)
	@$(PRINTF) "LD 	%-30s %-30s\n" "$(EXE)" "<= $^"
	@$(LD) $(LDFLAGS) -o $(EXE) $^

compile: $(C_OBJS) $(CXX_OBJS)

$(OBJDIR)/%.c.o: ./%.c Makefile
	@$(PRINTF) "CC 	%-30s %-30s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.c.o: */%.c Makefile
	@$(PRINTF) "CC 	%-30s %$30-s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.cpp.o: ./%.cpp Makefile
	@$(PRINTF) "CXX	%-30s %-30s\n" "$@" "<= $<"
	@$(CXX) $(DEPFLAGS) $(COMMON_CFLAGS) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR)/%.cpp.o: */%.cpp Makefile
	@$(PRINTF) "CXX 	%-30s %-30s\n" "$@" "<= $<"
	@$(CXX) $(DEPFLAGS) $(COMMON_CXXFLAGS) $(CXXFLAGS) -c -o $@ $<

strip:
	@$(ECHO) "STRIP	$(EXE)"
	@$(STRIP) $(STRIPFLAGS) $(EXE)

mostlyclean:
	@$(ECHO) "RM	$(C_OBJS) $(CXX_OBJS) $(DEPFILES)"
	@$(RM) $(C_OBJS) $(CXX_OBJS) $(DEPFILES)

clean: 
	@$(ECHO) "RM	$(C_OBJS) $(DEPFILES) $(EXE) $(CGD_USERINPUT_LIBNAME)"
	@$(RM) $(C_OBJS) $(DEPFILES) $(EXE) $(CGD_USERINPUT_LIBNAME)

run: LD_LIBRARY_PATH=$(LIBDIR)
run: EXE_FILE=$(EXE)
run: execute_file

test: EXE_FILE=$(TESTEXE)
test: execute_file

execute_file:
	@$(ECHO) "EXEC	$(EXE_FILE)"
	@$(EXEC) $(EXE_FILE)

-include $(DEPFILES)
