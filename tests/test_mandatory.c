#include "printf_utils.h"
#include <fcntl.h>

/*
** Mandatory part: %c %s %p %d %i %u %x %X %%. Every COMPARE calls the real
** printf and ft_printf with the same arguments and checks the bytes written
** and the return value (see printf_utils.h).
*/

/* Sweeps take every value in a range; under valgrind they take a sample. */
static int	sweep_step(void)
{
	return (under_valgrind() ? 97 : 1);
}

/* ------------------------------------------------------------------ %c */

static void	c_basic(void)
{
	COMPARE("%c", 'a');
	COMPARE("%c", 'Z');
	COMPARE("%c", '0');
	COMPARE("%c", ' ');
	COMPARE("%c", '%');
	COMPARE("%c%c%c", '1', '2', '3');
	COMPARE("Test: %c done", 'Z');
}

static void	c_nul(void)
{
	/* '\0' is one byte, written and counted like any other */
	COMPARE("%c", 0);
	COMPARE("%c%c%c", 0, 0, 0);
	COMPARE("a%cb", 0);
	COMPARE("Null %c check", 0);
	COMPARE("%c %c %c", 0, 'A', 0);
}

static void	c_text_around(void)
{
	/* Tripouille: text on both sides, '\0' first, middle and last */
	COMPARE(" %c ", '0');
	COMPARE(" %c", '0' - 256);
	COMPARE("%c ", '0' + 256);
	COMPARE(" %c %c %c ", '0', 0, '1');
	COMPARE(" %c %c %c ", ' ', ' ', ' ');
	COMPARE(" %c %c %c ", '2', '1', 0);
	COMPARE(" %c %c %c ", 0, '1', '2');
}

static void	c_every_byte(void)
{
	/* the int is converted to unsigned char: -1 is 0xff, 256+'A' is 'A' */
	for (int c = -1; c <= 256; c++)
		COMPARE_DESC("\"%c\", every value -1..256", "%c", c);
	COMPARE("%c", 256 + 'A');
	COMPARE("%c", INT_MIN);
	COMPARE("%c", INT_MAX);
	COMPARE("%c", '\n');
	COMPARE("%c", '\r');
	COMPARE("%c", '\t');
}

/* ------------------------------------------------------------------ %s */

static void	s_basic(void)
{
	COMPARE("%s", "Hello");
	COMPARE("%s", "");
	COMPARE("%s", "   ");
	COMPARE("%s %s", "Hello", "World");
	COMPARE("%s%s%s", "A", "B", "C");
	COMPARE("Start %s End", "");
	COMPARE("%s", "123\n\t\r456");
	COMPARE("%s", "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
}

static void	s_null(void)
{
	/* glibc prints (null) */
	COMPARE("%s", (char *)opaque_ptr(NULL));
	COMPARE(" NULL %s NULL ", (char *)opaque_ptr(NULL));
	COMPARE("%s %s", (char *)opaque_ptr(NULL), (char *)opaque_ptr(NULL));
	COMPARE("%s %s", (char *)opaque_ptr(NULL), "Test");
}

static void	s_text_around(void)
{
	/* Tripouille: empty strings between text */
	COMPARE(" %s", "");
	COMPARE("%s ", "");
	COMPARE(" %s ", "");
	COMPARE(" %s ", "-");
	COMPARE(" %s %s ", "", "-");
	COMPARE(" %s %s ", " - ", "");
	COMPARE(" %s %s %s %s %s ", " - ", "", "4", "", "2 ");
	COMPARE("[%s] %d [%s]", "", 0, "");
}

static void	s_embedded_nul(void)
{
	/* %s stops at the first '\0' of the string */
	COMPARE("%s", "a\0b");
	COMPARE("%s", "\0");
	COMPARE("%s", "\0abc");
	COMPARE("[%s]", "abc\0def");
	COMPARE("%s%c%s", "x", 0, "y");
	COMPARE("%s|%s", "\0", "\0\0");
}

static void	s_not_reinterpreted(void)
{
	/* a % inside the string argument is just a character */
	COMPARE("%s", "%d");
	COMPARE("%s", "Test %s %d");
	COMPARE("%s", "100%");
	COMPARE("%s", "%");
	COMPARE("%s%%", "%");
}

static void	s_long_and_high(void)
{
	char	*s;

	s = malloc(10001);
	for (int i = 0; i < 10000; i++)
		s[i] = (char)('a' + i % 26);
	s[10000] = '\0';
	COMPARE_DESC("\"%s\", 10000 chars", "%s", s);
	COMPARE_DESC("\"<%s><%s>\", 10000 chars twice", "<%s><%s>", s, s);
	free(s);
	COMPARE("%s", "\xff\x80\x7f\x01");
	COMPARE("%s", "héllo wörld ✓");
}

/* ---------------------------------------------------------- format \0 */

static void	format_nul(void)
{
	/* ft_printf stops at the first '\0' of the format, like printf */
	COMPARE("ab\0cd");
	COMPARE("ab\0cd%d", 1);
	COMPARE("%d\0%d", 1, 2);
	COMPARE("\0");
	COMPARE("\0%s", "never printed");
}

/* ------------------------------------------------------------------ %p */

static void	p_basic(void)
{
	int		x;
	char	*heap;

	heap = malloc(1);
	COMPARE("%p", (void *)&x);
	COMPARE("%p", (void *)heap);
	COMPARE("%p %p", (void *)&x, (void *)heap);
	COMPARE("%p%p%p", (void *)1, (void *)2, (void *)3);
	COMPARE("Address: %p end", (void *)0xdeadbeef);
	COMPARE("%p", (void *)0x100);
	COMPARE("%p", (void *)0x12345678);
	COMPARE("%p", (void *)0x7fffffffffff);
	COMPARE("%p", (void *)10);
	free(heap);
}

static void	p_null(void)
{
	/* glibc prints (nil) for NULL (macOS prints 0x0) */
	COMPARE("%p", opaque_ptr(NULL));
	COMPARE("Ptr: %p", opaque_ptr(NULL));
	COMPARE("%p %p", opaque_ptr(NULL), (void *)0xabc);
	COMPARE(" %p %p ", opaque_ptr(NULL), opaque_ptr(NULL));
}

static void	p_limits(void)
{
	COMPARE("%p", (void *)-1);
	COMPARE("%p", (void *)-42);
	COMPARE("%p", (void *)ULONG_MAX);
	COMPARE("%p", (void *)LONG_MIN);
	COMPARE("%p", (void *)LONG_MAX);
	COMPARE(" %p %p ", (void *)LONG_MIN, (void *)LONG_MAX);
	COMPARE(" %p %p ", (void *)ULONG_MAX, (void *)-ULONG_MAX);
	COMPARE(" %p ", (void *)15);
	COMPARE(" %p ", (void *)16);
	COMPARE(" %p ", (void *)17);
}

/* ------------------------------------------------------------ %d / %i */

static const int	g_ints[] = {0, 1, -1, 9, -9, 10, -10, 11, -11, 14, -14,
	15, -15, 16, -16, 17, 42, -42, 99, -99, 100, -100, 101, -101, 9999,
	-9999, 12345, -12345, CHAR_MAX, CHAR_MIN, UCHAR_MAX, SHRT_MAX, SHRT_MIN,
	USHRT_MAX, 1000000000, -1000000000, INT_MAX - 1, INT_MIN + 1, INT_MAX,
	INT_MIN};

#define NINTS ((int)(sizeof(g_ints) / sizeof(g_ints[0])))

static void	d_values(void)
{
	for (int i = 0; i < NINTS; i++)
		COMPARE_DESC("\"%d\", list of edge values", "%d", g_ints[i]);
	COMPARE(" %d ", INT_MIN);
	COMPARE("%d %d", INT_MAX, INT_MIN);
	COMPARE("%d%d%d", 0, 42, -42);
	COMPARE("Start %d Middle %d End", -10, 20);
}

static void	i_values(void)
{
	for (int i = 0; i < NINTS; i++)
		COMPARE_DESC("\"%i\", list of edge values", "%i", g_ints[i]);
	COMPARE("%i %i", INT_MAX, INT_MIN);
	COMPARE("%i%d", -1, 1);
}

static void	d_sweep(void)
{
	int	step = sweep_step();

	for (int v = -10000; v <= 10000; v += step)
		COMPARE_DESC("\"%d\", -10000..10000", "%d", v);
	for (int v = INT_MAX - 1000; v < INT_MAX && v > 0; v += step)
		COMPARE_DESC("\"%d\", INT_MAX-1000..INT_MAX", "%d", v);
	for (int v = INT_MIN; v < INT_MIN + 1000; v += step)
		COMPARE_DESC("\"%i\", INT_MIN..INT_MIN+1000", "%i", v);
	for (long p = 1; p <= 1000000000L; p *= 10)
	{
		COMPARE_DESC("\"%d\", powers of ten and their neighbours", "%d", (int)p);
		COMPARE_DESC("\"%d\", powers of ten and their neighbours", "%d", (int)(p - 1));
		COMPARE_DESC("\"%d\", powers of ten and their neighbours", "%d", (int)-p);
		COMPARE_DESC("\"%d\", powers of ten and their neighbours", "%d", (int)(1 - p));
	}
}

/* ------------------------------------------------------------------ %u */

static void	u_values(void)
{
	int	step = sweep_step();

	for (int i = 0; i < NINTS; i++)
		COMPARE_DESC("\"%u\", list of edge values (as unsigned)", "%u",
			(unsigned int)g_ints[i]);
	COMPARE("%u", 0);
	COMPARE("%u", -1);
	COMPARE("%u", UINT_MAX);
	COMPARE("%u", 2147483648u);
	COMPARE("%u", INT_MIN);
	COMPARE("%u %u", 0, -1);
	for (unsigned int v = UINT_MAX - 1000; v != 0 && v >= UINT_MAX - 1000; v += (unsigned int)step)
		COMPARE_DESC("\"%u\", UINT_MAX-1000..UINT_MAX", "%u", v);
	for (int v = -2000; v <= 2000; v += step)
		COMPARE_DESC("\"%u\", -2000..2000", "%u", v);
}

/* ------------------------------------------------------------- %x / %X */

static void	x_values(void)
{
	int	step = sweep_step();

	for (int i = 0; i < NINTS; i++)
	{
		COMPARE_DESC("\"%x\", list of edge values", "%x", (unsigned int)g_ints[i]);
		COMPARE_DESC("\"%X\", list of edge values", "%X", (unsigned int)g_ints[i]);
	}
	COMPARE("%x", 0xdeadbeef);
	COMPARE("%X", 0xDEADBEEF);
	COMPARE("%X", 0x12AF5);
	COMPARE("Lower: %x Upper: %X", 255, 255);
	COMPARE("%x %d %X", -1, 0, -1);
	COMPARE("%x.%d.%x.%d", 192, 168, 1, 1);
	for (int v = -4200; v <= 4200; v += step)
	{
		COMPARE_DESC("\"%x\", -4200..4200", "%x", v);
		COMPARE_DESC("\"%X\", -4200..4200", "%X", v);
	}
	for (unsigned long b = 1; b <= 0x100000000UL; b <<= 4)
	{
		COMPARE_DESC("\"%x\", powers of 16 and their neighbours", "%x", (unsigned int)b);
		COMPARE_DESC("\"%x\", powers of 16 and their neighbours", "%x", (unsigned int)(b - 1));
		COMPARE_DESC("\"%X\", powers of 16 and their neighbours", "%X", (unsigned int)(b - 1));
	}
}

/* ------------------------------------------------------------------ %% */

static void	percent(void)
{
	COMPARE("%%");
	COMPARE("%%%%");
	COMPARE(" %% ");
	COMPARE(" %%%% ");
	COMPARE(" %% %% %% ");
	COMPARE(" %%  %%  %% ");
	COMPARE("100%%");
	COMPARE("%%d");
	COMPARE("%%c");
	COMPARE("%%s");
	COMPARE("%c%%", 'x');
	COMPARE("%%%d%%", 10);
	COMPARE("Start %% End");
}

/* -------------------------------------------------------------- mixed */

static void	mixed(void)
{
	int	x;

	COMPARE("%c%s%p%d%i%u%x%X%%", 'A', "str", (void *)&x, -42, 42, 42u, 255, 255);
	COMPARE("%% %c %s %d %i %u %x %X %p %%", '!', "DONE", 1, 2, 3, 4, 5, opaque_ptr(NULL));
	COMPARE("%%%c%%%s%%%d%%%i%%%u%%%x%%%X%%%% %%%c%%%s%%%d%%%i%%%u%%%x%%%X%%%%",
		'A', "42", 42, 42, 42, 42, 42, 'B', "-42", -42, -42, -42, -42, -42);
	COMPARE("%c %d", 65, 'A');
	COMPARE("%c %d %x", 'A', 65, 65);
	COMPARE("%d %u %x", INT_MIN, UINT_MAX, INT_MAX);
	COMPARE("%u %d", -1, -1);
	COMPARE("%s\t%d\t%p", "Name", 123, (void *)0x1);
	COMPARE("%p + %d = %p", (void *)1000, 500, (void *)1500);
	COMPARE("%p %s", (void *)-1, (char *)opaque_ptr(NULL));
	COMPARE("%s %p", (char *)opaque_ptr(NULL), opaque_ptr(NULL));
	COMPARE("User: %s (ID: %d) Access: %p", "Admin", 1, (void *)0x999);
	COMPARE("Lorem %s dolor %d sit %c amet", "ipsum", 42, 'X');
}

static void	no_conversion(void)
{
	COMPARE("");
	COMPARE("Hello, world!");
	COMPARE("\n");
	COMPARE("tab\there\nnewline");
	COMPARE("Il puntatore è %p", (void *)0x424242);
	COMPARE("héllo wörld ✓ %d", 7);
}

static void	many_args(void)
{
	COMPARE("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
		"%d %d %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
		12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
		29, 30);
	COMPARE("%s%c%d%s%c%d%s%c%d%s%c%d%s%c%d", "a", 'b', 1, "c", 'd', 2, "e",
		'f', 3, "g", 'h', 4, "i", 'j', 5);
}

static void	long_format(void)
{
	char	fmt[6001];
	int		i;

	for (i = 0; i < 6000; i++)
		fmt[i] = (char)('a' + i % 26);
	fmt[i] = '\0';
	COMPARE_DESC("6000 chars of plain text", fmt);
	/* 3000 conversions that take no argument */
	for (i = 0; i + 2 <= 6000; i += 2)
	{
		fmt[i] = '%';
		fmt[i + 1] = '%';
	}
	fmt[i] = '\0';
	COMPARE_DESC("3000 x %%", fmt);
	memcpy(fmt + 5990, "%d %s %c", 9);
	COMPARE_DESC("2995 x %% then \"%d %s %c\"", fmt, -42, "end", '!');
}

static void	same_call_twice(void)
{
	/* nothing may be left over from the previous call */
	COMPARE("%d %s %c", 42, "again", 'x');
	COMPARE("%d %s %c", 42, "again", 'x');
	COMPARE("%x", 255);
	COMPARE("%x", 255);
}

/* ---------------------------------------------- (undefined) conversions */

static void	invalid_conversions(void)
{
	undefined_case();
	COMPARE("%k");
	COMPARE("%y%d", 42);
	COMPARE("%k%d", 42);
	COMPARE("abc%");
	COMPARE("%");
	COMPARE("%%%");
	COMPARE("% ");
	COMPARE("%5");
	COMPARE("%-");
	COMPARE("%#");
	COMPARE("%0");
	COMPARE("%.");
	COMPARE("%z");
	COMPARE("%w %d", 1);
	COMPARE("100%k done");
}

static void	wrong_size_arguments(void)
{
	/* Tripouille: arguments wider than the conversion */
	undefined_case();
	COMPARE(" %d ", LONG_MAX);
	COMPARE(" %d ", LONG_MIN);
	COMPARE(" %d ", UINT_MAX);
	COMPARE(" %d ", ULONG_MAX);
	COMPARE(" %d ", 9223372036854775807LL);
	COMPARE(" %i ", LONG_MAX);
	COMPARE(" %u ", LONG_MIN);
	COMPARE(" %u ", ULONG_MAX);
	COMPARE(" %x ", LONG_MAX);
	COMPARE(" %X ", LONG_MIN);
	COMPARE(" %x ", 9223372036854775807LL);
	COMPARE(" %d %d %d %d %d %d", INT_MAX, INT_MIN, LONG_MAX, LONG_MIN, ULONG_MAX, 0);
	COMPARE(" %p ", -1);
	COMPARE(" %p ", 1);
	COMPARE(" %p ", 15);
	COMPARE(" %p %p ", INT_MIN, INT_MAX);
	COMPARE(" %p %p ", 0, 0);
}

/* ------------------------------------------------------------ behaviour */

static void	no_buffering(void)
{
	size_t		len;
	const char	*out;

	/* ft_printf must write as it goes: no buffer left for later */
	capture_begin();
	hook_start(-1);
	ft_printf("a");
	write(STDOUT_FILENO, "b", 1);
	ft_printf("%c", 'c');
	write(STDOUT_FILENO, "d", 1);
	ft_printf("%s%d", "e", 1);
	hook_stop();
	capture_end(CAP_GOT);
	hook_release();
	out = captured(CAP_GOT, &len);
	CHECK(len == 6 && memcmp(out, "abcde1", 6) == 0,
		"ft_printf(\"a\"); write(1, \"b\", 1); ft_printf(\"%%c\", 'c'); ...: "
		"expected \"abcde1\", got %s (does ft_printf buffer its output?)",
		show(out, len));
}

/* ------------------------------------------------------- (defensive) */

static void	null_format(void)
{
	/* glibc's printf(NULL) returns -1 and writes nothing */
	undefined_case();
	COMPARE((const char *)opaque_ptr(NULL));
}

static void	write_fails(void)
{
	int	fd;
	int	saved;
	int	exp;
	int	got;

	/* fd 1 opened read-only: every write fails; printf returns -1 */
	fd = open("/dev/null", O_RDONLY);
	saved = dup(STDOUT_FILENO);
	fflush(stdout);
	dup2(fd, STDOUT_FILENO);
	exp = printf("abc");
	clearerr(stdout);
	got = ft_printf("abc");
	dup2(saved, STDOUT_FILENO);
	CHECK(got == exp, "write() fails: ft_printf(\"abc\") returned %d, printf "
		"returns %d", got, exp);
	dup2(fd, STDOUT_FILENO);
	exp = printf("%d %s", 42, "x");
	clearerr(stdout);
	got = ft_printf("%d %s", 42, "x");
	dup2(saved, STDOUT_FILENO);
	CHECK(got == exp, "write() fails: ft_printf(\"%%d %%s\", 42, \"x\") "
		"returned %d, printf returns %d", got, exp);
	close(fd);
	close(saved);
}

/* One call per format, each with its own arguments. */
static void	call_format(int f)
{
	if (f == 0)
		ft_printf("%d", -42);
	else if (f == 1)
		ft_printf("%s", "str");
	else if (f == 2)
		ft_printf("%x %X", 255, 255);
	else if (f == 3)
		ft_printf("%p", (void *)0x2a);
	else
		ft_printf("%u %c %s %d %i %p %%", 42u, 'c', "s", INT_MIN, 7, opaque_ptr(NULL));
}

static void	malloc_fails(void)
{
	static const char	*desc[] = {"\"%d\", -42", "\"%s\", \"str\"",
		"\"%x %X\", 255, 255", "\"%p\", 0x2a", "\"%u %c %s %d %i %p %%\", ..."};
	long				n;

	/* only if ft_printf mallocs: every allocation fails in turn */
	for (int f = 0; f < 5; f++)
	{
		capture_begin();
		hook_start(-1);
		call_format(f);
		hook_stop();
		capture_end(CAP_GOT);
		hook_release();
		n = hook_nalloc();
		for (long fail = 0; fail < n; fail++)
		{
			capture_begin();
			hook_start(fail);
			call_format(f);
			hook_stop();
			capture_end(CAP_GOT);
			CHECK(hook_nlive() == 0, "ft_printf(%s) with malloc #%ld failing: "
				"%zu block(s) never freed", desc[f], fail + 1, hook_nlive());
			hook_release();
		}
	}
}

int	main(void)
{
	printf("ft_printf: mandatory\n");
	run_case("%c: letters, '%', several in a row", c_basic);
	run_case("%c: '\\0' is written and counted (1 byte)", c_nul);
	run_case("%c: text around, '\\0' first/middle/last", c_text_around);
	run_case("%c: every value -1..256, INT_MIN/MAX, \\n \\r \\t", c_every_byte);
	run_case("%s: basic strings", s_basic);
	run_case("%s: NULL -> (null)", s_null);
	run_case("%s: empty strings between text", s_text_around);
	run_case("%s: stops at the '\\0' of the string", s_embedded_nul);
	run_case("%s: a % inside the argument is not a conversion", s_not_reinterpreted);
	run_case("%s: 10000 chars, high bytes, UTF-8", s_long_and_high);
	run_case("format: stops at a '\\0' in the format", format_nul);
	run_case("%p: stack, heap, small values", p_basic);
	run_case("%p: NULL -> (nil)", p_null);
	run_case("%p: -1, LONG_MIN/MAX, ULONG_MAX", p_limits);
	run_case("%d: edge values", d_values);
	run_case("%i: edge values", i_values);
	run_case("%d/%i: -10000..10000, near the limits, powers of ten", d_sweep);
	run_case("%u: edge values, -1, near UINT_MAX", u_values);
	run_case("%x/%X: edge values, sweep, powers of 16", x_values);
	run_case("%%: alone, repeated, between text", percent);
	run_case("mixed: every conversion in one call", mixed);
	run_case("no conversion: \"\", text, UTF-8 in the format", no_conversion);
	run_case("30 arguments in one call", many_args);
	run_case("6000-char format, 3000 conversions", long_format);
	run_case("same call twice gives the same result", same_call_twice);
	run_case("(undefined) invalid conversions: %k, trailing %, ...",
		invalid_conversions);
	run_case("(undefined) arguments wider than the conversion", wrong_size_arguments);
	run_case("output is not buffered (ft_printf and write interleave)", no_buffering);
	run_case("(defensive) ft_printf(NULL) -> like printf(NULL)", null_format);
	run_case("(defensive) write() fails -> returns -1 like printf", write_fails);
	run_case("(defensive) every malloc failing in turn -> no leak", malloc_fails);
	return (finish("ft_printf: mandatory"));
}
