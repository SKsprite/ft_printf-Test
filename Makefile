# Default: the tester is cloned inside the ft_printf repo (..). Falls back to
# a sibling ../ft_printf when .. has no ft_printf.h.
PRINTF_DIR	?= $(if $(or $(wildcard ../ft_printf.h),$(if $(wildcard ../ft_printf/ft_printf.h),,x)),..,../ft_printf)
LIBFTPRINTF	= $(PRINTF_DIR)/libftprintf.a

CC			= cc
# -Wno-format: the tests pass odd formats (%k, a trailing %, wrong-size
# arguments) to both printf and ft_printf on purpose.
CFLAGS		= -Wall -Wextra -Werror -Wno-format -Wno-format-security -g \
			  -I. -I$(PRINTF_DIR)
LDLIBS		= -L$(PRINTF_DIR) -lftprintf \
			  -Wl,--wrap=malloc,--wrap=calloc,--wrap=free

SUITES		= mandatory bonus star
BIN_DIR		= bin
UTILS_OBJ	= $(BIN_DIR)/test_utils.o $(BIN_DIR)/printf_utils.o
BINS		= $(addprefix $(BIN_DIR)/test_, $(SUITES))

export PRINTF_DIR

# V=1 / V=2: same as run_tests.sh -v / -vv (more detail on each failure).
# Failure details go to stderr: `make 2> errors.log`.
export V

# md=0: skip the README.md check (`make md=0`, `make check md=0`).
export md

VALGRIND	= RUNNER="valgrind -q --leak-check=full --errors-for-leak-kinds=definite \
			  --error-exitcode=42" TEST_TIMEOUT=120

CONVS		= c s p d i u x X

.PHONY: all test mandatory bonus star check build valgrind valgrind_bonus \
		valgrind_star clean fclean re remove $(CONVS) \%

# `make`: checks (built like the grader does) + the mandatory tests only.
all: mandatory

mandatory:
	@./run_tests.sh mandatory

test: mandatory

# `make c`, `make s`, ... `make X`, `make %`: only the mandatory cases that
# use that one conversion, with no other conversion mixed in. No checks.
# (`\%` is a plain target named %, not a pattern.)
$(CONVS):
	@./run_tests.sh $@

\%:
	@./run_tests.sh %

# Checks + mandatory + bonus tests, on the library built by `make bonus`.
bonus:
	@./run_tests.sh bonus

# Width / precision given as `*` arguments. Not in the subject: never part
# of `make` or `make bonus`.
star:
	@./run_tests.sh star

# Only the checks: files, each .c compiled on its own with no -I (like the
# grader), Makefile rules and relink, forbidden functions, README,
# norminette, git status.
check:
	@./run_tests.sh --checks-only

valgrind:
	@$(VALGRIND) ./run_tests.sh --no-checks mandatory

valgrind_bonus:
	@$(VALGRIND) ./run_tests.sh --no-checks bonus

valgrind_star:
	@$(VALGRIND) ./run_tests.sh star

build: $(BINS)

$(BIN_DIR):
	@mkdir -p $@

$(BIN_DIR)/%.o: %.c test.h printf_utils.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/test_%: tests/test_%.c $(UTILS_OBJ) test.h printf_utils.h \
		$(LIBFTPRINTF) | $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(UTILS_OBJ) -o $@ $(LDLIBS)

clean:
	rm -rf $(BIN_DIR)

fclean: clean

re: fclean build

# Deletes this whole tester directory (e.g. before submitting). Refuses
# unless the directory really is the tester, so it can't wipe anything else.
# Run it from your project folder, `make -C ft_printf-Test remove`: make
# can't move your shell out of a folder it deletes.
remove:
	@test -f "$(CURDIR)/run_tests.sh" && test -f "$(CURDIR)/printf_utils.h" \
		|| { echo "refusing: $(CURDIR) does not look like the tester"; exit 1; }
	@# One shell line: after the rm, a new line would start in the deleted
	@# folder. $(PWD) is make's copy of your shell's folder, taken from the
	@# environment before -C (the recipe shell resets its own $$PWD).
	@inside=no; [ "$(PWD)" = "$(CURDIR)" ] && inside=yes; \
		echo "Removing $(CURDIR)"; \
		cd / && rm -rf -- "$(CURDIR)" && \
		if [ $$inside = yes ]; then \
			echo "Removed. Your shell is still in the deleted folder: run  cd .."; \
		fi
