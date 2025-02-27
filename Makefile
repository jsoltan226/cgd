# Platform and paths
PLATFORM ?= linux
PREFIX ?= /usr

ifeq ($(PREFIX), /data/data/com.termux/files/usr)
TERMUX=1
endif

# Compiler and flags
CC ?= cc
CCLD ?= $(CC)
CPP ?= cpp

INCLUDES ?=
ifeq ($(PLATFORM), linux)
INCLUDES += -I$(PREFIX)/include/libdrm
endif

COMMON_CFLAGS = -std=c11 -Wall -Wpedantic -Wextra -I. -pipe -fPIC -pthread $(INCLUDES)
DEPFLAGS ?= -MMD -MP

LDFLAGS ?= -pie
ifeq ($(PLATFORM), windows)
LDFLAGS += -municode -mwindows
endif
SO_LDFLAGS = -shared

LIBS ?= -lm
ifeq ($(TERMUX), 1)
LIBS += $(PREFIX)/lib/libandroid-shmem.a -llog
endif
ifeq ($(PLATFORM), windows)
LIBS += -lgdi32
endif

STRIP?=strip
DEBUGSTRIP?=strip -d
STRIPFLAGS?=-g -s

# Shell commands
ECHO = echo
PRINTF = printf
RM = rm -f
TOUCH = touch -c
EXEC = exec
MKDIR = mkdir -p
RMRF = rm -rf
7Z = 7z

# Executable file Prefix/Suffix
EXEPREFIX =
EXESUFFIX =
ifeq ($(PLATFORM),windows)
EXESUFFIX = .exe
endif

SO_PREFIX =
SO_SUFFIX = .so
ifeq ($(PLATFORM),windows)
SO_SUFFIX = .dll
endif

# Directories
OBJDIR = obj
BINDIR = bin
TEST_SRC_DIR = tests
TEST_BINDIR = $(TEST_SRC_DIR)/$(BINDIR)
PLATFORM_SRCDIR = platform

# Test sources and objects
TEST_SRCS = $(wildcard $(TEST_SRC_DIR)/*.c)
TEST_EXES = $(patsubst $(TEST_SRC_DIR)/%.c,$(TEST_BINDIR)/$(EXEPREFIX)%$(EXESUFFIX),$(TEST_SRCS))
TEST_LOGFILE = $(TEST_SRC_DIR)/testlog.txt

STATIC_TESTS=core/static-tests.h

# Sources and objects
PLATFORM_SRCS = $(wildcard $(PLATFORM_SRCDIR)/$(PLATFORM)/*.c)

_all_srcs=$(wildcard */*.c) $(wildcard *.c)
SRCS = $(filter-out $(TEST_SRCS),$(_all_srcs)) $(PLATFORM_SRCS)

OBJS = $(patsubst %.c,$(OBJDIR)/%.c.o,$(shell basename -a $(SRCS)))
DEPS = $(patsubst %.o,%.d,$(OBJS))

_main_obj = $(OBJDIR)/main.c.o
_entry_point_obj = $(OBJDIR)/entry-point.c.o

STRIP_OBJS = $(OBJDIR)/log.c.o

# Executables
EXE = $(BINDIR)/$(EXEPREFIX)main$(EXESUFFIX)
TEST_LIB = $(TEST_BINDIR)/$(SO_PREFIX)libmain_test$(SO_SUFFIX)
TEST_LIB_OBJS = $(filter-out $(_main_obj) $(_entry_point_obj),$(OBJS))
EXEARGS =

.PHONY: all release strip clean mostlyclean update run br tests build-tests run-tests debug-run bdr test-hooks
.NOTPARALLEL: all release br bdr build-tests

# Build targets
all: CFLAGS = -ggdb -O0 -Wall
all: $(STATIC_TESTS) $(OBJDIR) $(BINDIR) $(EXE)

release: LDFLAGS += -flto
release: CFLAGS = -O3 -Wall -Werror -flto -DNDEBUG -DCGD_BUILDTYPE_RELEASE
release: $(STATIC_TESTS) clean $(OBJDIR) $(BINDIR) $(EXE) tests mostlyclean strip

br: all run

# Output executable rules
$(EXE): $(OBJS)
	@$(DEBUGSTRIP) $(STRIP_OBJS) 2>/dev/null
	@$(PRINTF) "CCLD 	%-30s %-30s\n" "$(EXE)" "<= $^"
	@$(CCLD) $(LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)

$(TEST_LIB): $(TEST_LIB_OBJS)
	@$(PRINTF) "CCLD 	%-30s %-30s\n" "$(TEST_LIB)" "<= $(TEST_LIB_OBJS)"
	@$(CCLD) $(SO_LDFLAGS) -o $(TEST_LIB) $(TEST_LIB_OBJS) $(LIBS)

# Output directory rules
$(OBJDIR):
	@$(ECHO) "MKDIR	$(OBJDIR)"
	@$(MKDIR) $(OBJDIR)

$(BINDIR):
	@$(ECHO) "MKDIR	$(BINDIR)"
	@$(MKDIR) $(BINDIR)

$(TEST_BINDIR):
	@$(ECHO) "MKDIR	$(TEST_BINDIR)"
	@$(MKDIR) $(TEST_BINDIR)

# Generic compilation targets
$(OBJDIR)/%.c.o: %.c Makefile
	@$(PRINTF) "CC 	%-30s %-30s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.c.o: */%.c Makefile
	@$(PRINTF) "CC 	%-30s %-30s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.c.o: $(PLATFORM_SRCDIR)/$(PLATFORM)/%.c Makefile
	@$(PRINTF) "CC 	%-30s %-30s\n" "$@" "<= $<"
	@$(CC) $(DEPFLAGS) $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<


# Test preparation targets
test-hooks: asset-load-test-hook

.PHONY: asset-load-test-hook
asset-load-test-hook:
	@test -f assets/tests/asset_load_test/rand_9_3_1_0_0_0_0.png || $(ECHO) "7Z	assets/tests/asset_load_test/random_pngs.7z" && $(PRINTF) 'Y\nA\n' | $(7Z) e -oassets/tests/asset_load_test assets/tests/asset_load_test/random_pngs.7z >/dev/null


# Test execution targets
run-tests: tests

tests: CFLAGS = -ggdb -O0 -Wall
tests: build-tests test-hooks
	@n_passed=0; \
	$(ECHO) -n > $(TEST_LOGFILE); \
	for i in $(TEST_EXES); do \
		$(PRINTF) "EXEC	%-30s " "$$i"; \
		if CGD_TEST_LOG_FILE="$(TEST_LOGFILE)" $$i >/dev/null 2>&1; then \
			$(PRINTF) "$(GREEN)OK$(COL_RESET)\n"; \
			n_passed="$$((n_passed + 1))"; \
		else \
			$(PRINTF) "$(RED)FAIL$(COL_RESET)\n"; \
		fi; \
	done; \
	n_total=$$(echo $(TEST_EXES) | wc -w); \
	if test "$$n_passed" -lt "$$n_total"; then \
		$(PRINTF) "$(RED)"; \
	else \
		$(PRINTF) "$(GREEN)"; \
	fi; \
	$(PRINTF) "%s/%s$(COL_RESET) tests passed.\n" "$$n_passed" "$$n_total";


# Test compilation targets
build-tests: CFLAGS = -ggdb -O0 -Wall
build-tests: $(STATIC_TESTS) $(OBJDIR) $(BINDIR) $(TEST_BINDIR) $(TEST_LIB) $(_entry_point_obj) compile-tests

compile-tests: CFLAGS = -ggdb -O0 -Wall
compile-tests: $(TEST_EXES)

$(TEST_BINDIR)/$(EXEPREFIX)%$(EXESUFFIX): CFLAGS = -ggdb -O0 -Wall
$(TEST_BINDIR)/$(EXEPREFIX)%$(EXESUFFIX): $(TEST_SRC_DIR)/%.c Makefile
	@$(PRINTF) "CCLD	%-30s %-30s\n" "$@" "<= $< $(TEST_LIB) $(_entry_point_obj)"
	@$(CC) $(COMMON_CFLAGS) $(CFLAGS) -o $@ $< $(LDFLAGS) $(TEST_LIB) $(LIBS) $(_entry_point_obj)

$(STATIC_TESTS):
	@$(CPP) $(STATIC_TESTS) >/dev/null

# Cleanup targets
mostlyclean:
	@$(ECHO) "RM	$(OBJS) $(DEPS) $(TEST_LOGFILE)"
	@$(RM) $(OBJS) $(DEPS) $(TEST_LOGFILE)

clean:
	@$(ECHO) "RM	$(OBJS) $(DEPS) $(EXE) $(TEST_LIB) $(BINDIR) $(OBJDIR) $(TEST_EXES) $(TEST_BINDIR) $(TEST_LOGFILE)"
	@$(RM) $(OBJS) $(DEPS) $(EXE) $(TEST_LIB) $(TEST_EXES) $(TEST_LOGFILE) assets/tests/asset_load_test/*.png
	@$(RMRF) $(OBJDIR) $(BINDIR) $(TEST_BINDIR)

# Output execution targets
run:
	@$(ECHO) "EXEC	$(EXE) $(EXEARGS)"
	@$(EXEC) $(EXE) $(EXEARGS)

debug-run:
	@$(ECHO) "EXEC	$(EXE) $(EXEARGS)"
	@bash -c '$(EXEC) -a debug $(EXE) $(EXEARGS)'

bdr: all debug-run

# Miscellaneous targets
strip:
	@$(ECHO) "STRIP	$(EXE)"
	@$(STRIP) $(STRIPFLAGS) $(EXE)

update:
	@$(ECHO) "TOUCH	$(SRCS)"
	@$(TOUCH) $(SRCS)

-include $(DEPS)
