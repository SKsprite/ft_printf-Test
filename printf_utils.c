#include "printf_utils.h"
#include <errno.h>

void	*__real_malloc(size_t size);
void	*__real_calloc(size_t n, size_t size);
void	__real_free(void *p);
void	*__wrap_malloc(size_t size);
void	*__wrap_calloc(size_t n, size_t size);
void	__wrap_free(void *p);

#define MAX_TRACKED 4096

static int		g_armed = 0;
static long		g_fail_at = -1;
static long		g_nalloc = 0;
static void		*g_live[MAX_TRACKED];
static size_t	g_nlive = 0;

/* What each capture saw: [CAP_EXPECTED] = printf, [CAP_GOT] = ft_printf. */
static char		*g_cap[2];
static size_t	g_cap_len[2];
static size_t	g_cap_size[2];
static int		g_cap_fd = -1;
static int		g_saved_fd = -1;
static int		g_undefined = 0;

long			g_compares = 0;
long			g_compare_failures = 0;

/*
** stdout unbuffered before main: printf then writes as it goes, like
** ft_printf must (no buffer management), and a failing write makes printf
** itself return -1 instead of failing later in fflush.
*/
__attribute__((constructor))
static void	unbuffer_stdout(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
}

static void	*track(void *p)
{
	if (p && g_nlive < MAX_TRACKED)
		g_live[g_nlive++] = p;
	return (p);
}

void	*__wrap_malloc(size_t size)
{
	if (!g_armed)
		return (__real_malloc(size));
	if (g_nalloc++ == g_fail_at)
		return (NULL);
	return (track(__real_malloc(size)));
}

void	*__wrap_calloc(size_t n, size_t size)
{
	if (!g_armed)
		return (__real_calloc(n, size));
	if (g_nalloc++ == g_fail_at)
		return (NULL);
	return (track(__real_calloc(n, size)));
}

void	__wrap_free(void *p)
{
	for (size_t i = 0; p && i < g_nlive; i++)
	{
		if (g_live[i] == p)
		{
			g_live[i] = g_live[--g_nlive];
			break ;
		}
	}
	__real_free(p);
}

void	hook_start(long fail_at)
{
	g_nlive = 0;
	g_nalloc = 0;
	g_fail_at = fail_at;
	g_armed = 1;
}

void	hook_stop(void)
{
	g_armed = 0;
}

long	hook_nalloc(void)
{
	return (g_nalloc);
}

size_t	hook_nlive(void)
{
	return (g_nlive);
}

/* Frees every block allocated (and not freed) during the last hook window. */
void	hook_release(void)
{
	while (g_nlive)
		free(g_live[g_nlive - 1]);
}

int	opaque_int(int v)
{
	volatile int	x;

	x = v;
	return (x);
}

void	run_tagged(const char *tag, const char *name, void (*fn)(void))
{
	const char	*only = getenv("ONLY");

	if (only && *only && strcmp(only, tag) != 0)
		return ;
	run_case(name, fn);
}

const char	*only_label(void)
{
	static char	label[32];
	const char	*only = getenv("ONLY");

	if (!only || !*only)
		return ("");
	snprintf(label, sizeof(label), " (%%%s only)", only);
	return (label);
}

void	undefined_case(void)
{
	g_undefined = 1;
}

/* The capture file: opened once per process, emptied before each call. */
void	capture_begin(void)
{
	FILE	*f;

	fflush(stdout);
	if (g_cap_fd < 0)
	{
		f = tmpfile();
		if (!f || (g_cap_fd = dup(fileno(f))) < 0)
		{
			perror("tmpfile");
			exit(2);
		}
		fclose(f);
	}
	if (ftruncate(g_cap_fd, 0) < 0 || lseek(g_cap_fd, 0, SEEK_SET) < 0)
	{
		perror("capture");
		exit(2);
	}
	g_saved_fd = dup(STDOUT_FILENO);
	dup2(g_cap_fd, STDOUT_FILENO);
}

void	capture_end(int which)
{
	off_t	size;
	ssize_t	n;
	int		err;

	err = errno;
	fflush(stdout);
	clearerr(stdout);
	dup2(g_saved_fd, STDOUT_FILENO);
	close(g_saved_fd);
	size = lseek(g_cap_fd, 0, SEEK_END);
	if (size < 0)
		size = 0;
	if ((size_t)size + 1 > g_cap_size[which])
	{
		free(g_cap[which]);
		g_cap_size[which] = (size_t)size + 1;
		g_cap[which] = malloc(g_cap_size[which]);
		if (!g_cap[which])
		{
			perror("malloc");
			exit(2);
		}
	}
	n = pread(g_cap_fd, g_cap[which], (size_t)size, 0);
	g_cap_len[which] = n < 0 ? 0 : (size_t)n;
	g_cap[which][g_cap_len[which]] = '\0';
	errno = err;
}

const char	*captured(int which, size_t *len)
{
	*len = g_cap_len[which];
	return (g_cap[which] ? g_cap[which] : "");
}

static size_t	first_difference(void)
{
	size_t	i;

	i = 0;
	while (i < g_cap_len[0] && i < g_cap_len[1]
		&& g_cap[0][i] == g_cap[1][i])
		i++;
	return (i);
}

/* Shows the bytes around offset off of capture which. */
static const char	*around(int which, size_t off)
{
	size_t	start;

	start = off > 16 && verbosity() < 1 ? off - 16 : 0;
	return (show(g_cap[which] + start, g_cap_len[which] - start));
}

void	compare_report(const char *file, int line, const char *desc,
			int exp_ret, int got_ret)
{
	int		same_out;
	size_t	leaks;
	size_t	off;

	g_compares++;
	leaks = hook_nlive();
	hook_release();
	same_out = g_cap_len[0] == g_cap_len[1]
		&& memcmp(g_cap[0], g_cap[1], g_cap_len[0]) == 0;
	if (same_out && exp_ret == got_ret && leaks == 0
		&& (got_ret < 0 || (size_t)got_ret == g_cap_len[1]))
		return ;
	g_compare_failures++;
	report_fail(file, line, "ft_printf(%s)%s", desc,
		g_undefined ? "   (undefined in C: expected is what glibc printf does)"
		: "");
	if (!same_out)
	{
		off = first_difference();
		report_note("output differs at byte %zu%s", off,
			off > 16 && verbosity() < 1 ? " (shown from 16 bytes before; -v "
			"shows all)" : "");
		report_note("  printf    wrote %3zu byte(s): %s", g_cap_len[0],
			around(0, off));
		report_note("  ft_printf wrote %3zu byte(s): %s", g_cap_len[1],
			around(1, off));
	}
	if (exp_ret != got_ret)
		report_note("return value: printf %d, ft_printf %d", exp_ret, got_ret);
	if (got_ret >= 0 && (size_t)got_ret != g_cap_len[1])
		report_note("ft_printf returned %d but wrote %zu byte(s)", got_ret,
			g_cap_len[1]);
	if (leaks)
		report_note("%zu block(s) allocated by ft_printf were never freed "
			"(leak)", leaks);
}
