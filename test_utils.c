#include "test.h"
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>

#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

/* Exit status a case uses to report itself as skipped. */
#define SKIP_STATUS 77

/* Longest escaped buffer show() prints (bytes shown at -vv). */
#define SHOW_MAX 4000

int			g_case_failures = 0;
static int	g_passed = 0;
static int	g_failed = 0;
static int	g_skipped = 0;
/* Set by run_case() before each fork: the child's stderr is a tmpfile. */
static int	g_err_color = 0;

/* VERBOSE=0|1|2 (run_tests.sh -v / -vv), read once. */
int	verbosity(void)
{
	static int	v = -1;
	const char	*env;

	if (v < 0)
	{
		env = getenv("VERBOSE");
		v = env ? atoi(env) : 0;
		if (v < 0)
			v = 0;
		if (v > 2)
			v = 2;
	}
	return (v);
}

/* Failures printed per case before the rest are summarised. */
static long	max_reported(void)
{
	static const long	caps[] = {10, 50, 1000000};

	return (caps[verbosity()]);
}

static const char	*ecol(const char *code)
{
	return (g_err_color ? code : "");
}

void	*opaque_ptr(void *p)
{
	void *volatile	v;

	v = p;
	return (v);
}

int	crashes(void (*fn)(void))
{
	pid_t	pid;
	int		status;

	fflush(stdout);
	pid = fork();
	if (pid < 0)
	{
		perror("fork");
		exit(2);
	}
	if (pid == 0)
	{
		signal(SIGSEGV, SIG_DFL);
		signal(SIGBUS, SIG_DFL);
		fn();
		_exit(0);
	}
	waitpid(pid, &status, 0);
	return (WIFSIGNALED(status));
}

int	under_valgrind(void)
{
	const char	*preload;

	preload = getenv("LD_PRELOAD");
	return (preload && strstr(preload, "vgpreload") != NULL);
}

void	skip_case(const char *why)
{
	printf("      " YELLOW "-> " RESET "%s\n", why);
	fflush(stdout);
	_exit(SKIP_STATUS);
}

const char	*show(const void *buf, size_t n)
{
	static const size_t	caps[] = {48, 300, SHOW_MAX};
	static char		out[8][SHOW_MAX * 4 + 64];
	static int		slot = 0;
	const unsigned char	*p;
	char			*o;
	size_t			i;
	size_t			len;

	if (!buf)
		return ("(null)");
	p = buf;
	o = out[slot++ % 8];
	len = 0;
	o[len++] = '"';
	for (i = 0; i < n && i < caps[verbosity()]; i++)
	{
		if (p[i] == '\\' || p[i] == '"')
			len += sprintf(o + len, "\\%c", p[i]);
		else if (isprint(p[i]))
			o[len++] = (char)p[i];
		else
			len += sprintf(o + len, "\\x%02x", p[i]);
	}
	o[len++] = '"';
	if (i < n)
		len += sprintf(o + len, "... (%zu bytes)", n);
	o[len] = '\0';
	return (o);
}

static int	case_timeout(void)
{
	const char	*env;
	int			t;

	env = getenv("TEST_TIMEOUT");
	if (env && (t = atoi(env)) > 0)
		return (t);
	return (DEFAULT_TIMEOUT);
}

void	report_fail(const char *file, int line, const char *fmt, ...)
{
	va_list	ap;

	g_case_failures++;
	if (g_case_failures > max_reported())
	{
		if (g_case_failures == max_reported() + 1)
			fprintf(stderr, "      %s-> %s... more failures in this case "
				"suppressed%s\n", ecol(RED), ecol(RESET),
				verbosity() < 2 ? " (-v / -vv show more)" : "");
		return ;
	}
	fprintf(stderr, "      %s-> %s%s:%d: ", ecol(RED), ecol(RESET), file, line);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

/* A detail line under the last failure (hidden once failures are capped). */
void	report_note(const char *fmt, ...)
{
	va_list	ap;

	if (g_case_failures > max_reported())
		return ;
	fprintf(stderr, "         ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

static const char	*signal_name(int sig)
{
	if (sig == SIGSEGV)
		return ("SEGFAULT");
	if (sig == SIGBUS)
		return ("BUS ERROR");
	if (sig == SIGABRT)
		return ("ABORT (bad free / heap corruption?)");
	if (sig == SIGALRM)
		return ("TIMEOUT (infinite loop?)");
	if (sig == SIGFPE)
		return ("FLOATING POINT EXCEPTION");
	return (strsignal(sig));
}

/* Reads everything the child wrote so it can be printed under the KO line. */
static char	*slurp(int fd)
{
	char	*buf;
	char	*tmp;
	size_t	len;
	size_t	cap;
	ssize_t	n;

	len = 0;
	cap = 4096;
	buf = malloc(cap + 1);
	if (!buf)
		return (NULL);
	while ((n = read(fd, buf + len, cap - len)) > 0)
	{
		len += (size_t)n;
		if (len == cap)
		{
			cap *= 2;
			tmp = realloc(buf, cap + 1);
			if (!tmp)
				break ;
			buf = tmp;
		}
	}
	buf[len] = '\0';
	return (buf);
}

/*
** Under make valgrind*, run_tests.sh has valgrind write one log per process
** into $VG_LOG_DIR (--log-file=<dir>/%p.log). After each case the logs of
** the processes that reported something are printed to stderr under the
** case's name, then deleted: on the console they show up right under the
** KO line, and `2> file` sends them to that file instead.
*/
static void	dump_valgrind_logs(const char *name)
{
	const char		*dir;
	const char		*context;
	char			own[32];
	char			path[4096];
	char			buf[4096];
	DIR				*d;
	struct dirent	*e;
	FILE			*f;
	size_t			n;
	int				tty;

	dir = getenv("VG_LOG_DIR");
	if (!dir || !*dir || !(d = opendir(dir)))
		return ;
	context = getenv("TEST_CONTEXT");
	snprintf(own, sizeof(own), "%d.log", (int)getpid());
	tty = isatty(STDERR_FILENO);
	while ((e = readdir(d)))
	{
		if (!strstr(e->d_name, ".log") || strcmp(e->d_name, own) == 0)
			continue ;
		snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
		if (!(f = fopen(path, "r")))
			continue ;
		if ((n = fread(buf, 1, sizeof(buf), f)) > 0)
		{
			fprintf(stderr, "%s==== valgrind: %s%s%s ====%s\n",
				tty ? RED : "", context ? context : "", context ? ": " : "",
				name, tty ? RESET : "");
			do
				fwrite(buf, 1, n, stderr);
			while ((n = fread(buf, 1, sizeof(buf), f)) > 0);
			fputc('\n', stderr);
		}
		fclose(f);
		unlink(path);
	}
	closedir(d);
	fflush(stderr);
}

/*
** Copies the case's messages to stderr. When stderr is not the terminal
** (2> file), the [KO] line is not next to them, so a header names the case.
*/
static void	copy_to_stderr(FILE *f, const char *name)
{
	const char	*context;
	char		buf[4096];
	size_t		n;
	int			first;

	fflush(f);
	rewind(f);
	context = getenv("TEST_CONTEXT");
	first = 1;
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
	{
		if (first && !g_err_color)
			fprintf(stderr, "==== %s%s%s ====\n", context ? context : "",
				context ? ": " : "", name);
		first = 0;
		fwrite(buf, 1, n, stderr);
	}
	fflush(stderr);
}

/*
** Each case runs in its own child process so a crash or infinite loop
** only fails that case instead of killing the whole run. The child's
** stdout (skip reasons) comes back through a pipe and goes to stdout; its
** stderr (every failure message) goes to a tmpfile that is copied to
** stderr once the status line is printed. So the [OK]/[KO] lines stay on
** stdout and `2> file` collects the explanations.
*/
void	run_case(const char *name, void (*fn)(void))
{
	pid_t	pid;
	int		status;
	int		fds[2];
	char	*details;
	FILE	*errf;

	fflush(stdout);
	fflush(stderr);
	g_err_color = isatty(STDERR_FILENO);
	errf = tmpfile();
	if (pipe(fds) < 0 || (pid = fork()) < 0)
	{
		perror("pipe/fork");
		exit(2);
	}
	if (pid == 0)
	{
		close(fds[0]);
		dup2(fds[1], STDOUT_FILENO);
		close(fds[1]);
		if (errf)
			dup2(fileno(errf), STDERR_FILENO);
		alarm((unsigned int)case_timeout());
		g_case_failures = 0;
		fn();
		fflush(stdout);
		fflush(stderr);
		_exit(g_case_failures ? 1 : 0);
	}
	close(fds[1]);
	details = slurp(fds[0]);
	close(fds[0]);
	waitpid(pid, &status, 0);
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
	{
		g_passed++;
		printf("  " GREEN "[OK]" RESET " %s\n", name);
	}
	else if (WIFEXITED(status) && WEXITSTATUS(status) == SKIP_STATUS)
	{
		g_skipped++;
		printf("  " YELLOW "[SKIP]" RESET " %s\n", name);
	}
	else
	{
		g_failed++;
		if (WIFSIGNALED(status))
			printf("  " RED "[KO]" RESET " %s " YELLOW "-> %s" RESET "\n",
				name, signal_name(WTERMSIG(status)));
		else if (WEXITSTATUS(status) == 42)
			printf("  " RED "[KO]" RESET " %s " YELLOW
				"-> valgrind reported memory errors (log on stderr)" RESET "\n",
				name);
		else
			printf("  " RED "[KO]" RESET " %s\n", name);
	}
	if (details)
		fputs(details, stdout);
	free(details);
	fflush(stdout);
	if (errf)
	{
		copy_to_stderr(errf, name);
		fclose(errf);
	}
	dump_valgrind_logs(name);
}

int	finish(const char *func_name)
{
	printf("%s%s: %d/%d passed" RESET, g_failed ? RED : GREEN, func_name,
		g_passed, g_passed + g_failed);
	if (g_skipped)
		printf(YELLOW " (%d skipped)" RESET, g_skipped);
	printf("\n\n");
	return (g_failed != 0);
}
