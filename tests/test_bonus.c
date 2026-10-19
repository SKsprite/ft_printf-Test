#include "printf_utils.h"

/*
** Bonus part: the flags '-0.' with a minimum field width, and '# +'. Named
** cases first (each one a known trap), then a sweep that tries every
** combination of flags, width and precision on every conversion.
*/

/* ----------------------------------------------------------- width */

static void	width(void)
{
	int	x;

	COMPARE("[%5c]", 'a');
	COMPARE("[%1c]", 'a');
	COMPARE("[%5c]", 0);
	COMPARE("[%10s]", "hello");
	COMPARE("[%3s]", "hello");
	COMPARE("[%10s]", (char *)opaque_ptr(NULL));
	COMPARE("[%20p]", (void *)&x);
	COMPARE("[%20p]", opaque_ptr(NULL));
	COMPARE("[%1p]", (void *)0x12345);
	COMPARE("[%10d]", -42);
	COMPARE("[%2d]", -42);
	COMPARE("[%15d]", INT_MIN);
	COMPARE("[%10i]", 42);
	COMPARE("[%10u]", UINT_MAX);
	COMPARE("[%10x]", 255);
	COMPARE("[%10X]", 255);
	COMPARE("[%1d]", 12345);
	COMPARE("[%1d %2d %3d]", 0, -9, 99);
}

/* ----------------------------------------------------------- '-' */

static void	minus(void)
{
	COMPARE("[%-5c]", 'L');
	COMPARE(" %-1c %-2c %-3c ", '0', 0, '1');
	COMPARE(" %-1c %-2c %-3c ", 0, '1', '2');
	COMPARE("[%-10s]", "left");
	COMPARE("[%-10s]", "");
	COMPARE("[%-10s]", (char *)opaque_ptr(NULL));
	COMPARE("[%-2s]", "longstring");
	COMPARE("[%-20p]", (void *)0x12345);
	COMPARE("[%-20p]", opaque_ptr(NULL));
	COMPARE("[%-1p]", (void *)0x12345);
	COMPARE("[%-10d]", 42);
	COMPARE("[%-10i]", -42);
	COMPARE("[%-12d]", INT_MAX);
	COMPARE("[%-12d]", INT_MIN);
	COMPARE("[%-10u]", 12345);
	COMPARE("[%-12u]", UINT_MAX);
	COMPARE("[%-10x]", 0xabc);
	COMPARE("[%-10X]", 0xABC);
	COMPARE("[%-1x]", 0xFF);
	COMPARE("[%-3u]", 10000);
	COMPARE("A %-5c B", 'X');
	COMPARE("%-10s %-10d", "Mix", 123);
}

/* ----------------------------------------------------------- '0' */

static void	zero(void)
{
	COMPARE("[%03d]", 5);
	COMPARE("[%03d]", -5);
	COMPARE("[%010d]", 123);
	COMPARE("[%010d]", -123);
	COMPARE("[%03d]", 1234);
	COMPARE("[%03d]", 0);
	COMPARE("[%01d]", 0);
	COMPARE("[%02d]", 0);
	COMPARE("[%015d]", INT_MAX);
	COMPARE("[%015d]", INT_MIN);
	COMPARE("[%020d]", -9999);
	COMPARE("[%05i]", 42);
	COMPARE("[%05u]", 42);
	COMPARE("[%05u]", 0);
	COMPARE("[%012u]", 1000000);
	COMPARE("[%05x]", 42);
	COMPARE("[%05x]", 0);
	COMPARE("[%05X]", 0xABC);
	COMPARE("[%09x]", 0xFFFFFF);
	COMPARE("[%010x]", INT_MIN);
}

/* ----------------------------------------------------------- '.' */

static void	precision(void)
{
	COMPARE("[%.5d]", 42);
	COMPARE("[%.5d]", -42);
	COMPARE("[%.3d]", 12345);
	COMPARE("[%.3d]", -123);
	COMPARE("[%.0d]", 0);
	COMPARE("[%.d]", 0);
	COMPARE("[%.0d]", 42);
	COMPARE("[%.d]", 10);
	COMPARE("[%.5d]", 0);
	COMPARE("[%.10d]", INT_MAX);
	COMPARE("[%.10d]", INT_MIN);
	COMPARE("[%.20d]", -1);
	COMPARE("[%.0i]", 0);
	COMPARE("[%.10u]", 42);
	COMPARE("[%.20u]", -1);
	COMPARE("[%.0u]", 0);
	COMPARE("[%.5x]", 0xff);
	COMPARE("[%.5X]", 0xff);
	COMPARE("[%.20x]", -1);
	COMPARE("[%.0x]", 0);
	COMPARE("[%.1x]", 0);
	COMPARE("[%.1X]", 0);
	COMPARE("[%.3s]", "mamma");
	COMPARE("[%.10s]", "ciao");
	COMPARE("[%.s]", "test");
	COMPARE("[%.0s]", "test");
	COMPARE("[%.2s]", "");
	COMPARE(" %.1s", "");
	COMPARE(" %.1s %.2s %.3s %.4s ", " - ", "", "4", "");
	/* NULL with a precision: glibc prints "(null)" only if it fits */
	COMPARE("[%.3s]", (char *)opaque_ptr(NULL));
	COMPARE("[%.6s]", (char *)opaque_ptr(NULL));
	COMPARE("[%.10s]", (char *)opaque_ptr(NULL));
	COMPARE("[%10.5d]", 42);
	COMPARE("[%10.5d]", -42);
	COMPARE("[%10.0d]", 0);
	COMPARE("[%10.3s]", "banana");
	COMPARE("[%-10.5d]", 42);
	COMPARE("[%-10.5d]", -42);
	COMPARE("[%-10.5s]", "testing");
}

/* ----------------------------------------------------------- '#' */

static void	hash(void)
{
	COMPARE("[%#x]", 0);
	COMPARE("[%#X]", 0);
	COMPARE("[%#x]", 52);
	COMPARE("[%#X]", 52);
	COMPARE("[%#x]", -1);
	COMPARE("[%#X]", INT_MAX);
	COMPARE("[%#x]", UINT_MAX);
	COMPARE("[%#5x]", 10);
	COMPARE("[%#2x]", 10);
	COMPARE("[%#3x]", 1);
	COMPARE("[%#5x]", 0);
	COMPARE("[%#5X]", 0);
	COMPARE("[%#10x]", 42);
	COMPARE("[%#-10x]", 42);
	COMPARE("[%#-8x]", 255);
	COMPARE("[%#010x]", 42);
	COMPARE("[%#010X]", 42);
	COMPARE("[%#10x]", 0);
	COMPARE("[%#.5x]", 255);
	COMPARE("[%#.0x]", 0);
	COMPARE("[%#.3x]", 0);
	COMPARE("[%#11x]", INT_MIN);
	COMPARE("[%#15x]", INT_MAX);
}

/* ------------------------------------------------------- '+' and ' ' */

static void	plus_space(void)
{
	COMPARE("[%+d]", 42);
	COMPARE("[%+d]", -42);
	COMPARE("[%+d]", 0);
	COMPARE("[%+i]", 0);
	COMPARE("[%+d]", INT_MAX);
	COMPARE("[%+d]", INT_MIN);
	COMPARE("[%+10d]", 42);
	COMPARE("[%+2d]", -42);
	COMPARE("[%+5d]", 0);
	COMPARE("[%+.0d]", 0);
	COMPARE("[%+.5d]", 42);
	COMPARE("[%+15d]", INT_MIN);
	COMPARE("[% d]", 42);
	COMPARE("[% d]", -42);
	COMPARE("[% d]", 0);
	COMPARE("[% i]", INT_MAX);
	COMPARE("[% d]", INT_MIN);
	COMPARE("[% 5d]", 42);
	COMPARE("[% 1d]", 0);
	COMPARE("[% .0d]", 0);
	COMPARE("[% .5d]", 42);
	COMPARE("[% 10d]", -12345);
}

/* ------------------------------------- combinations and precedence */

static void	combinations(void)
{
	/* '-' wins over '0'; a precision cancels '0' on numbers; '+' wins over ' ' */
	COMPARE("[%-05d]", 42);
	COMPARE("[%-010d]", -42);
	COMPARE("[%0-5d]", 42);
	COMPARE("[%05.3d]", 42);
	COMPARE("[%05.3d]", -42);
	COMPARE("[%08.3x]", 255);
	COMPARE("[%010.0d]", 0);
	COMPARE("[%+ d]", 42);
	COMPARE("[% +d]", 42);
	COMPARE("[%+ d]", -42);
	COMPARE("[%-+10d]", 42);
	COMPARE("[%-+10d]", -42);
	COMPARE("[%-+5i]", 0);
	COMPARE("[%- 10d]", 42);
	COMPARE("[%- 10d]", -42);
	COMPARE("[%+010d]", 42);
	COMPARE("[%+010d]", -42);
	COMPARE("[% 010d]", 42);
	COMPARE("[% 010d]", -42);
	COMPARE("[%+05d]", 42);
	COMPARE("[% 05d]", 42);
	COMPARE("[%-+ 0#10.5d]", 42);
	COMPARE("[%#-+ 010.5x]", 42);
}

/* ------------------------------------------------- repeated flags */

static void	repeated_flags(void)
{
	COMPARE("[%--5d]", 42);
	COMPARE("[%00005d]", 42);
	COMPARE("[%++d]", 42);
	COMPARE("[%  d]", 42);
	COMPARE("[%-0-0-5d]", 42);
	COMPARE("[%##x]", 42);
	COMPARE("[%-+-+ 0 10.5d]", -42);
	COMPARE("[%---10s]", "abc");
	COMPARE("[%++++d]", 0);
	COMPARE("[%    d]", -7);
	COMPARE("[%####X]", 255);
	COMPARE("[%-----c]", 'z');
	COMPARE("[%0000.0000d]", 0);
}

/* --------------------------- (undefined) flags where they don't apply */

static void	undefined_flags(void)
{
	int	x;

	undefined_case();
	COMPARE("[%#d]", INT_MAX);
	COMPARE("[%#s]", "hello");
	COMPARE("[%#c]", 'd');
	COMPARE("[%#p]", (void *)&x);
	COMPARE("[%#u]", 42);
	COMPARE("[%-#5d]", 42);
	COMPARE("[%+u]", 42);
	COMPARE("[% x]", 42);
	COMPARE("[%+x]", 42);
	COMPARE("[%+s]", "s");
	COMPARE("[% s]", "s");
	COMPARE(" % 1s", "");
	COMPARE("% 1s ", "");
	COMPARE("[%+c]", 'c');
	COMPARE("[%+p]", (void *)&x);
	COMPARE("[% p]", (void *)&x);
	COMPARE("[%05s]", "abc");
	COMPARE(" %-0s ", "");
	COMPARE("[%05c]", 'c');
	COMPARE("[%05p]", (void *)0x2a);
	COMPARE("[%.3c]", 'c');
	COMPARE("[%.5p]", (void *)0x2a);
	COMPARE("[%.0p]", opaque_ptr(NULL));
	COMPARE("[%+%]");
	COMPARE("[%5%]");
	COMPARE("[%-5%]");
	COMPARE("[%-10%]", 0);
	COMPARE("[%05%]");
	COMPARE("[%.3%]");
}

/* ------------------------- (undefined) invalid flags in between */

static void	invalid_flag_order(void)
{
	undefined_case();
	COMPARE("[%5-d]", 42);
	COMPARE("[%.5-d]", 42);
	COMPARE("[%-5.k]", 42);
	COMPARE("[%.-5d]", 42);
	COMPARE("[%5.3.2d]", 42);
	COMPARE("[%5 d]", 42);
	COMPARE("[%5+d]", 42);
	COMPARE("[%5#x]", 42);
	COMPARE("[%.5 d]", 42);
	COMPARE("[%-k]");
	COMPARE("[%05k]");
	COMPARE("[%-5]");
	COMPARE("[%.5]");
	COMPARE("[%10.5k%d]", 42);
}

/* ---------------------- (undefined) arguments wider than the conversion */

static void	wrong_size_with_flags(void)
{
	undefined_case();
	COMPARE(" %-11d ", LONG_MAX);
	COMPARE(" %-12i ", LONG_MIN);
	COMPARE(" %-13u ", UINT_MAX);
	COMPARE(" %-14x ", ULONG_MAX);
	COMPARE(" %-15X ", 9223372036854775807LL);
	COMPARE(" %011d ", LONG_MAX);
	COMPARE(" %012u ", LONG_MIN);
	COMPARE(" %015x ", 9223372036854775807LL);
	COMPARE(" %.10d ", LONG_MAX);
	COMPARE(" %.11i ", LONG_MIN);
	COMPARE(" %.13X ", ULONG_MAX);
	COMPARE(" %+d ", LONG_MAX);
	COMPARE(" % d ", ULONG_MAX);
	COMPARE(" %#x ", 9223372036854775807LL);
	COMPARE(" %-11p %-12p ", INT_MIN, INT_MAX);
	COMPARE(" %-13p %-14p ", ULONG_MAX, -ULONG_MAX);
}

/* --------------------------------------------------------------- sweep */

static const char	*g_flag_sets[] = {"", "-", "0", "#", "+", " ", "-0", "-#",
	"-+", "- ", "0#", "0+", "0 ", "#+", "+ ", "-+ 0#"};
static const char	*g_widths[] = {"", "1", "5", "20"};
static const char	*g_precs[] = {"", ".", ".0", ".1", ".5", ".20"};

#define NFLAGS ((int)(sizeof(g_flag_sets) / sizeof(g_flag_sets[0])))
#define NWIDTHS ((int)(sizeof(g_widths) / sizeof(g_widths[0])))
#define NPRECS ((int)(sizeof(g_precs) / sizeof(g_precs[0])))
#define NVALS 8

static int	g_x;
static const int	g_ivals[NVALS] = {0, 1, -1, 42, -42, 12345, INT_MAX, INT_MIN};

/*
** The C standard: '#' is defined for x/X only, '0' for numbers (d i u x X),
** a precision for numbers and s, '+' and ' ' for signed numbers (d i).
** "%%" takes no flags at all.
*/
static int	is_undefined(char conv, const char *flags, const char *prec)
{
	int	number = strchr("diuxX", conv) != NULL;

	if (conv == '%')
		return (*flags || *prec);
	if (strchr(flags, '#') && conv != 'x' && conv != 'X')
		return (1);
	if (strchr(flags, '0') && !number)
		return (1);
	if (*prec && !number && conv != 's')
		return (1);
	if ((strchr(flags, '+') || strchr(flags, ' ')) && conv != 'd' && conv != 'i')
		return (1);
	return (0);
}

static void	sweep_one(const char *fmt, char conv, int v)
{
	static const char	*strs[NVALS] = {"", "a", "hello world", NULL, "\xff",
		"0123456789abcdefghijklmnop", "-", "%d"};
	static const int	chars[NVALS] = {'a', 0, ' ', '~', 255, '0', -1, 'z'};
	void				*ptrs[NVALS] = {NULL, (void *)1, &g_x, (void *)-1,
		(void *)0xdeadbeef, (void *)LONG_MIN, (void *)15, (void *)16};
	char				desc[200];

	if (conv == 'c')
	{
		snprintf(desc, sizeof(desc), "\"%s\", %d", fmt, chars[v]);
		COMPARE_DESC(desc, fmt, chars[v]);
	}
	else if (conv == 's')
	{
		snprintf(desc, sizeof(desc), "\"%s\", %s%s%s", fmt, strs[v] ? "\"" : "",
			strs[v] ? show(strs[v], strlen(strs[v])) : "NULL", strs[v] ? "\"" : "");
		COMPARE_DESC(desc, fmt, (char *)opaque_ptr((void *)strs[v]));
	}
	else if (conv == 'p')
	{
		snprintf(desc, sizeof(desc), "\"%s\", (void *)%#lx", fmt,
			(unsigned long)ptrs[v]);
		COMPARE_DESC(desc, fmt, opaque_ptr(ptrs[v]));
	}
	else
	{
		snprintf(desc, sizeof(desc), "\"%s\", %d", fmt, g_ivals[v]);
		COMPARE_DESC(desc, fmt, opaque_int(g_ivals[v]));
	}
}

/* Every flag set x width x precision x value on conv, of one kind. */
static void	sweep(char conv, int want_undefined)
{
	char	fmt[64];
	int		nvals = under_valgrind() ? 3 : NVALS;

	if (want_undefined)
		undefined_case();
	for (int f = 0; f < NFLAGS; f++)
		for (int w = 0; w < NWIDTHS; w++)
			for (int p = 0; p < NPRECS; p++)
			{
				if (is_undefined(conv, g_flag_sets[f], g_precs[p]) != want_undefined)
					continue ;
				snprintf(fmt, sizeof(fmt), "[%%%s%s%s%c]", g_flag_sets[f],
					g_widths[w], g_precs[p], conv);
				for (int v = 0; v < (conv == '%' ? 1 : nvals); v++)
					sweep_one(fmt, conv, v);
			}
}

static void	sweep_c(void) { sweep('c', 0); }
static void	sweep_s(void) { sweep('s', 0); }
static void	sweep_p(void) { sweep('p', 0); }
static void	sweep_d(void) { sweep('d', 0); }
static void	sweep_i(void) { sweep('i', 0); }
static void	sweep_u(void) { sweep('u', 0); }
static void	sweep_x(void) { sweep('x', 0); }
static void	sweep_upper_x(void) { sweep('X', 0); }
static void	sweep_percent(void) { sweep('%', 0); }

static void	sweep_undefined(void)
{
	const char	*convs = "cspdiuxX%";

	for (int i = 0; convs[i]; i++)
		sweep(convs[i], 1);
}

int	main(void)
{
	printf("ft_printf: bonus\n");
	run_case("width on every conversion", width);
	run_case("'-': left-justified", minus);
	run_case("'0': zero padding", zero);
	run_case("'.': precision on numbers and strings, NULL", precision);
	run_case("'#': 0x / 0X, not for 0", hash);
	run_case("'+' and ' ': sign or space", plus_space);
	run_case("combinations: '-' beats '0', precision cancels '0', '+' beats ' '",
		combinations);
	run_case("repeated flags: %--5d, %00005d, %++d, %  d, ...", repeated_flags);
	run_case("(undefined) flags where they don't apply: %#d, %05s, %.3c, %5%, ...",
		undefined_flags);
	run_case("(undefined) flags in the wrong order: %5-d, %.-5d, %5.3.2d, ...",
		invalid_flag_order);
	run_case("(undefined) arguments wider than the conversion, with flags",
		wrong_size_with_flags);
	run_case("sweep %c: every flag set x width x precision", sweep_c);
	run_case("sweep %s: every flag set x width x precision", sweep_s);
	run_case("sweep %p: every flag set x width x precision", sweep_p);
	run_case("sweep %d: every flag set x width x precision", sweep_d);
	run_case("sweep %i: every flag set x width x precision", sweep_i);
	run_case("sweep %u: every flag set x width x precision", sweep_u);
	run_case("sweep %x: every flag set x width x precision", sweep_x);
	run_case("sweep %X: every flag set x width x precision", sweep_upper_x);
	run_case("sweep %%: no flags", sweep_percent);
	run_case("(undefined) sweep: every combination the standard leaves undefined",
		sweep_undefined);
	return (finish("ft_printf: bonus"));
}
