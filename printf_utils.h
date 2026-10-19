#ifndef PRINTF_UTILS_H
# define PRINTF_UTILS_H

# include "test.h"

/*
** COMPARE(fmt, args...): calls the real printf and ft_printf with the same
** arguments, each with fd 1 redirected to a file, and checks that both
** return the same value and write the same bytes, and that ft_printf's
** return value is the number of bytes it really wrote. ft_printf's own
** malloc/free are tracked during the call: a block still allocated after
** it returns is reported as a leak.
**
** The real printf writes through an unbuffered stdout (set up before
** main), so a failing write makes it return -1 the way ft_printf should.
*/
# define COMPARE(...) COMPARE_DESC(#__VA_ARGS__, __VA_ARGS__)

/* Same, with the call described by desc (for formats built at run time). */
# define COMPARE_DESC(desc, ...) do { \
	int	e__; \
	int	g__; \
	capture_begin(); \
	e__ = printf(__VA_ARGS__); \
	capture_end(CAP_EXPECTED); \
	capture_begin(); \
	hook_start(-1); \
	g__ = ft_printf(__VA_ARGS__); \
	hook_stop(); \
	capture_end(CAP_GOT); \
	compare_report(__FILE__, __LINE__, (desc), e__, g__); \
} while (0)

# define CAP_EXPECTED 0
# define CAP_GOT 1

void	capture_begin(void);
void	capture_end(int which);
/* The bytes of the last capture of that kind (NUL-terminated, len set). */
const char	*captured(int which, size_t *len);
void	compare_report(const char *file, int line, const char *desc,
			int exp_ret, int got_ret);

/*
** undefined_case(): the comparisons that follow, in this case, are about
** behaviour the C standard leaves undefined. They are still compared
** strictly with glibc; failure messages say so.
*/
void	undefined_case(void);

/* Comparisons made / failed so far in this case (for sweep summaries). */
extern long	g_compares;
extern long	g_compare_failures;

/*
** malloc/calloc/free made by ft_printf go through these hooks (the test
** binaries are linked with -Wl,--wrap=malloc,--wrap=calloc,--wrap=free).
** Between hook_start() and hook_stop():
**   - allocation number fail_at (0-based) returns NULL (-1 = never),
**   - every block allocated is tracked until freed (hook_nlive).
*/
void	hook_start(long fail_at);
void	hook_stop(void);
long	hook_nalloc(void);
size_t	hook_nlive(void);
void	hook_release(void);

/* An int that the compiler can't see the value of (no folding). */
int		opaque_int(int v);

#endif
