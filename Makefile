PLATFORM?=gnu_linux
PREFIX?=/usr

CC?=cc
AR?=ar
CCLD?=cc
COMMON_CFLAGS=-Wall -I. -I.. -I$(PREFIX)/include/freetype2 -pipe -fPIC
DEPFLAGS?=-MMD -MP
LDFLAGS?=-pie
ARFLAGS=rcs
LIBS?=-lpng -lSDL2 -lfreetype
STRIP?=strip
DEBUGSTRIP?=strip -d
STRIPFLAGS?=-g -s

ECHO=echo
PRINTF=printf
RM=rm -f
TOUCH=touch -c
EXEC=exec
MKDIR=mkdir -p
RMRF=rm -rf

EXEPREFIX =
EXESUFFIX =
OBJDIR = obj
BINDIR = bin

TEST_SRC_DIR = ./tests
TEST_SRCS=$(wildcard $(TEST_SRC_DIR)/*.c)
TEST_EXE_DIR=$(TEST_SRC_DIR)/$(BINDIR)
TEST_EXES=$(patsubst $(TEST_SRC_DIR)/%.c,$(TEST_EXE_DIR)/$(EXEPREFIX)%$(EXESUFFIX),$(TEST_SRCS))
TEST_LOGFILE=$(TEST_SRC_DIR)/testlog.txt

PLATFORM_SRCDIR=./platform
_platform_all_srcs=$(shell find $(PLATFORM_SRCDIR))

SRCS=$(filter-out $(TEST_SRCS) $(_platform_all_srcs),$(shell find . -type f -name "*.c" ))

ifeq ($(PLATFORM), gnu_linux)
SRCS+=$(shell find $(PLATFORM_SRCDIR)/linux -type f -name "*.c")
endif
ifeq ($(PLATFORM), windows)
SRCS+=$(shell find $(PLATFORM_SRCDIR)/windows -type f -name "*.c")
endif

OBJS=$(patsubst %.c,$(OBJDIR)/%.c.o,$(shell basename -a $(SRCS)))
DEPS=$(patsubst %.o,%.d,$(OBJS))
STRIP_OBJS=$(OBJDIR)/log.o

EXE=$(BINDIR)/$(EXEPREFIX)main$(EXESUFFIX)
#TEST_LIB=$(BINDIR)/libmain_test.a
EXEARGS=


#RED=[31;1m
#GREEN=[32;1m
#COL_RESET=[0m
RED=
GREEN=
COL_RESET=

.PHONY: all release strip clean mostlyclean update run br tests build-tests run-tests
.NOTPARALLEL: all release br

all: CFLAGS = -ggdb -O0 -Wall
all: $(OBJDIR) $(BINDIR) $(EXE)

release: CFLAGS = -O3 -Wall -Werror
release: clean $(OBJDIR) $(BINDIR) $(EXE) mostlyclean strip

br: all run

$(EXE): $(OBJS)
#	@$(DEBUGSTRIP) $(STRIP_OBJS) 2>/dev/null
	@$(PRINTF) "CCLD 	%-25s %-20s\n" "$(EXE)" "<= $^"
	@$(CCLD) $(LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)

$(TEST_LIB): $(OBJS) $(BINDIR)
	@$(PRINTF) "AR 	%-20s %-20s\n" "$(TEST_LIB)" "<= $(filter-out $(OBJDIR)/main.o,$(OBJS))"
	@$(AR) $(ARFLAGS) -o $(TEST_LIB) $(filter-out $(OBJDIR)/main.o,$(OBJS)) >/dev/null

$(OBJDIR):
	@$(ECHO) "MKDIR	$(OBJDIR)"
	@$(MKDIR) $(OBJDIR)

$(BINDIR):
	@$(ECHO) "MKDIR	$(BINDIR)"
	@$(MKDIR) $(BINDIR)

$(TEST_EXE_DIR):
	@$(ECHO) "MKDIR	$(TEST_EXE_DIR)"
	@$(MKDIR) $(TEST_EXE_DIR)

$(OBJDIR)/%.c.o: ./%.c Makefile
	@$(PRINTF) "CC 	%-25s %-20s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.c.o: */%.c Makefile
	@$(PRINTF) "CC 	%-25s %-20s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.c.o: */*/%.c Makefile
	@$(PRINTF) "CC 	%-25s %-20s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.c.o: */*/*/%.c Makefile
	@$(PRINTF) "CC 	%-25s %-20s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<

tests: CFLAGS = -ggdb -O0 -Wall
tests: $(OBJDIR) $(BINDIR) $(TEST_EXE_DIR) build-tests
	@n_passed=0; \
	$(ECHO) -n > $(TEST_LOGFILE); \
	for i in $(TEST_EXES); do \
		$(PRINTF) "EXEC	%-30s " "$$i"; \
		$(ECHO) "TEST $$i" >> $(TEST_LOGFILE); \
		if $$i >> $(TEST_LOGFILE) 2>&1; then \
			$(PRINTF) "$(GREEN)OK$(COL_RESET)\n"; \
			n_passed="$$((n_passed + 1))"; \
		else \
			$(PRINTF) "$(RED)FAIL$(COL_RESET)\n"; \
		fi; \
		$(ECHO) "END TEST $$i" >> "$(TEST_LOGFILE)"; \
	done; \
	n_total=$$(echo $(TEST_EXES) | wc -w); \
	if test "$$n_passed" -lt "$$n_total"; then \
		$(PRINTF) "$(RED)"; \
	else \
		$(PRINTF) "$(GREEN)"; \
	fi; \
	$(PRINTF) "%s/%s$(COL_RESET) tests passed.\n" "$$n_passed" "$$n_total";

build-tests: $(TEST_EXE_DIR) compile-tests

compile-tests: $(TEST_EXES)

run-tests: tests

$(TEST_EXE_DIR)/%: $(TEST_SRC_DIR)/%.c $(TEST_LIB) Makefile
	@$(PRINTF) "CCLD	%-30s %-30s\n" "$@" "<= $< $(TEST_LIB)"
	@$(CC) $(COMMON_CFLAGS) $(CFLAGS) -o $@ $< $(TEST_LIB)

strip:
	@$(ECHO) "STRIP	$(EXE)"
	@$(STRIP) $(STRIPFLAGS) $(EXE)

mostlyclean:
#	@$(ECHO) "RM	$(OBJS) $(DEPS) $(TEST_LOGFILE)"
#	@$(RM) $(OBJS) $(DEPS) $(TEST_LOGFILE)
	@$(ECHO) "RM	$(OBJS) $(DEPS)"
	@$(RM) $(OBJS) $(DEPS)

clean:
#	@$(ECHO) "RM	$(OBJS) $(DEPS) $(EXE) $(TEST_LIB) $(BINDIR) $(OBJDIR) $(TEST_EXES) $(TEST_EXE_DIR) $(TEST_LOGFILE) out.png"
#	@$(RM) $(OBJS) $(DEPS) $(EXE) $(TEST_LIB) $(TEST_EXES) $(TEST_LOGFILE)
#	@$(RMRF) $(OBJDIR) $(BINDIR) $(TEST_EXE_DIR)
	@$(ECHO) "RM	$(OBJS) $(DEPS) $(EXE) $(BINDIR) $(OBJDIR)"
	@$(RM) $(OBJS) $(DEPS) $(EXE)
	@$(RMRF) $(OBJDIR) $(BINDIR)

update:
	@$(ECHO) "TOUCH	$(SRCS)"
	@$(TOUCH) $(SRCS)

run:
	@$(ECHO) "EXEC	$(EXE) $(EXEARGS)"
	@$(EXEC) $(EXE) $(EXEARGS)

-include $(DEPS)
