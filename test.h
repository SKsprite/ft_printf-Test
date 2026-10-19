#ifndef TEST_H
# define TEST_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <strings.h>
# include <ctype.h>
# include <limits.h>
# include <stdint.h>
# include <stdarg.h>
# include <unistd.h>
# include "ft_printf.h"

/*
** Seconds a single test case may run before it is killed (infinite loops).
** Override at runtime with the TEST_TIMEOUT environment variable.
*/
# define DEFAULT_TIMEOUT 10

extern int	g_case_failures;

/* VERBOSE from the environment: 0, 1 (-v) or 2 (-vv). */
int		verbosity(void);

/*
** Printable, escaped view of n bytes (for failure messages): 48 bytes,
** 300 with -v, 4000 with -vv.
*/
const char	*show(const void *buf, size_t n);

void	run_case(const char *name, void (*fn)(void));
int		finish(const char *func_name);
void	report_fail(const char *file, int line, const char *fmt, ...)
		__attribute__((format(printf, 3, 4)));
void	report_note(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Defeats compile-time folding / nonnull warnings when passing odd args. */
void	*opaque_ptr(void *p);

/*
** Runs fn in a separate process and returns 1 if it was killed by a signal
** (segfault, abort, ...), 0 if it returned normally. Used to check that an
** ft_ function crashes exactly when the libc function does.
*/
int		crashes(void (*fn)(void));

/* 1 when the test runs under valgrind (it preloads its vgpreload_*.so). */
int		under_valgrind(void);

/*
** Ends the current case as [SKIP] (not counted as passed or failed) and
** prints why. Only call it from inside a case run by run_case().
*/
void	skip_case(const char *why) __attribute__((noreturn));

# define SIGN(x) (((x) > 0) - ((x) < 0))

# define CHECK(cond, ...) do { \
	if (!(cond)) \
		report_fail(__FILE__, __LINE__, __VA_ARGS__); \
} while (0)

# define CHECK_INT(got, exp) do { \
	long long g__ = (long long)(got); \
	long long e__ = (long long)(exp); \
	CHECK(g__ == e__, "%s: expected %lld, got %lld", #got, e__, g__); \
} while (0)

# define CHECK_SIZE(got, exp) do { \
	size_t g__ = (size_t)(got); \
	size_t e__ = (size_t)(exp); \
	CHECK(g__ == e__, "%s: expected %zu, got %zu", #got, e__, g__); \
} while (0)

/* For is* functions: libc may return any non-zero value for "true". */
# define CHECK_BOOL(got, exp) do { \
	int g__ = !!(got); \
	int e__ = !!(exp); \
	CHECK(g__ == e__, "%s: expected %s, got %s", #got, \
		e__ ? "true" : "false", g__ ? "true" : "false"); \
} while (0)

/* For *cmp functions: only the sign of the result is specified. */
# define CHECK_SIGN(got, exp) do { \
	int g__ = SIGN(got); \
	int e__ = SIGN(exp); \
	CHECK(g__ == e__, "%s: expected sign %d, got sign %d", #got, e__, g__); \
} while (0)

# define CHECK_PTR(got, exp) do { \
	const void *g__ = (const void *)(got); \
	const void *e__ = (const void *)(exp); \
	CHECK(g__ == e__, "%s: expected %p, got %p", #got, e__, g__); \
} while (0)

# define CHECK_MEM(got, exp, n) do { \
	CHECK(memcmp((got), (exp), (n)) == 0, \
		"%s: buffer differs from expected over %zu bytes", \
		#got, (size_t)(n)); \
} while (0)

# define CHECK_STR(got, exp) do { \
	const char *g__ = (got); \
	const char *e__ = (exp); \
	CHECK(g__ && e__ && strcmp(g__, e__) == 0, \
		"%s: expected \"%s\", got \"%s\"", #got, \
		e__ ? e__ : "(null)", g__ ? g__ : "(null)"); \
} while (0)

#endif
