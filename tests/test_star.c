#include "printf_utils.h"

/*
** (extra) '*': the width and/or the precision taken from the arguments.
** Not in the subject. A negative width means '-' plus that width; a
** negative precision means "no precision".
*/

static void	star_width(void)
{
	COMPARE("[%*d]", 5, 42);
	COMPARE("[%*d]", 0, 42);
	COMPARE("[%*d]", 1, 42);
	COMPARE("[%*s]", 10, "abc");
	COMPARE("[%*c]", 4, 'x');
	COMPARE("[%*c]", 4, 0);
	COMPARE("[%*p]", 20, (void *)0x2a);
	COMPARE("[%*u]", 12, UINT_MAX);
	COMPARE("[%*x]", 8, 255);
	COMPARE("[%*X]", 8, 255);
	COMPARE("[%*s]", 10, (char *)opaque_ptr(NULL));
	COMPARE("[%*d]", 1000, 1);
}

static void	star_negative_width(void)
{
	/* a negative width is the '-' flag with that width */
	COMPARE("[%*d]", -5, 42);
	COMPARE("[%*s]", -10, "abc");
	COMPARE("[%*c]", -3, 'x');
	COMPARE("[%*p]", -20, (void *)0x2a);
	COMPARE("[%0*d]", -5, 42);
	COMPARE("[%-*d]", -5, 42);
	COMPARE("[%*d]", INT_MIN + 1, 0);
}

static void	star_flags(void)
{
	COMPARE("[%-*d]", 5, 42);
	COMPARE("[%0*d]", 5, -42);
	COMPARE("[%+*d]", 6, 42);
	COMPARE("[% *d]", 6, 42);
	COMPARE("[%#*x]", 8, 255);
	COMPARE("[%-0*d]", 5, 42);
	COMPARE("[%-+ 0#*d]", 8, 42);
}

static void	star_precision(void)
{
	COMPARE("[%.*d]", 5, 42);
	COMPARE("[%.*d]", 0, 0);
	COMPARE("[%.*x]", 0, 0);
	COMPARE("[%.*s]", 3, "abcdef");
	COMPARE("[%.*s]", 0, "abcdef");
	COMPARE("[%.*s]", 3, (char *)opaque_ptr(NULL));
	COMPARE("[%.*u]", 10, 42);
	COMPARE("[%#.*x]", 5, 255);
}

static void	star_negative_precision(void)
{
	/* a negative precision is as if there were none */
	COMPARE("[%.*d]", -5, 42);
	COMPARE("[%.*d]", -1, 0);
	COMPARE("[%.*s]", -3, "abcdef");
	COMPARE("[%05.*d]", -1, 42);
	COMPARE("[%.*x]", -1, 0);
}

static void	star_both(void)
{
	COMPARE("[%*.*d]", 10, 5, 42);
	COMPARE("[%*.*d]", -10, 5, 42);
	COMPARE("[%*.*s]", 8, 3, "abcdef");
	COMPARE("[%-*.*s]", 8, 3, "abcdef");
	COMPARE("[%*.*d]", 0, 0, 0);
	COMPARE("[%0*.*d]", 8, 3, -7);
	COMPARE("[%*.*x]", -8, -1, 255);
}

static void	star_several(void)
{
	COMPARE("[%*s|%*s]", 5, "a", -5, "b");
	COMPARE("[%*d%*d%*d]", 3, 1, 3, 2, 3, 3);
	COMPARE("[%.*s%.*s]", 1, "xyz", 2, "xyz");
	COMPARE("[%*c%s%*d]", 3, 'a', "mid", -4, 9);
}

static void	star_undefined(void)
{
	undefined_case();
	COMPARE("[%*%]", 5);
	COMPARE("[%.*c]", 3, 'c');
	COMPARE("[%.*p]", 5, (void *)0x2a);
	COMPARE("[%**d]", 3, 4, 5);
	COMPARE("[%*.5*d]", 3, 4, 5);
}

int	main(void)
{
	printf("ft_printf: * (extra, not in the subject)\n");
	run_case("(extra) %*d: width from the arguments", star_width);
	run_case("(extra) negative * width -> left-justified", star_negative_width);
	run_case("(extra) * with every flag", star_flags);
	run_case("(extra) %.*d: precision from the arguments", star_precision);
	run_case("(extra) negative * precision -> no precision",
		star_negative_precision);
	run_case("(extra) %*.*d: both from the arguments", star_both);
	run_case("(extra) several * in one call", star_several);
	run_case("(extra, undefined) %*%, %.*c, %**d, ...", star_undefined);
	return (finish("ft_printf: star"));
}
