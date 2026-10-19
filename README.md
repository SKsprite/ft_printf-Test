# ft_printf tests

Tests for the 42 `ft_printf` project. Each call is made twice, once with the
real `printf` and once with your `ft_printf`, with the same arguments. The
test compares the bytes both write, and both return values.

- **Mandatory** (`make`): `%c %s %p %d %i %u %x %X %%`, including `'\0'`
  from `%c`, `\0` inside strings and inside the format, `NULL` arguments,
  invalid conversions like `%k`, arguments of the wrong size, and the
  limits of every type.
- **Bonus** (`make bonus`): the flags `-0.` with a minimum field width, and
  `# +`. Every flag is tested on its own and in combinations, repeated
  (`%--5d`, `%++d`), and in the wrong order (`%5-d`, `%.-5d`). A sweep then
  tries every flag set × width × precision on every conversion.
- **Star** (`make star`): `*` for the width and the precision (`%*d`,
  `%.*s`, negative values). Not in the subject, so it only runs when you
  ask for it.
- **Checks** (first in `make` and `make bonus`, or alone with `make check`):
  your files are built exactly the way the grader builds them. The checks
  also cover the Makefile rules, relinking, forbidden functions, the README
  format the subject requires, norminette, and anything not committed or
  not pushed.

The tests link against the `libftprintf.a` your Makefile builds. They never
compile your `.c` files themselves, except in the checks.

## Requirements

- `cc` (clang or gcc) and `make`, on **Linux**. The expected output comes
  from glibc's `printf`. On macOS, `printf("%p", NULL)` prints `0x0` instead
  of `(nil)`, and other details differ too.
- `valgrind` (optional), for `make valgrind`
- `norminette` (optional), for the Norm check

## Layout

Clone the tester **inside** your ft_printf repo, or next to it:

```
ft_printf/                  <- your project (builds libftprintf.a with make)
├── Makefile
├── ft_printf.h
├── ft_printf.c, ...
├── libft/                  <- if you use your libft
└── ft_printf-Test/         <- the tester, cloned here...
    ├── Makefile
    ├── run_tests.sh        <- checks, then the suites
    ├── test.h              <- CHECK macros, run_case, skip_case
    ├── test_utils.c        <- runner: fork per case, timeout, crash
    │                          detection, valgrind logs, [SKIP]
    ├── printf_utils.c/.h   <- COMPARE: capture of fd 1, printf vs
    │                          ft_printf, malloc/free tracking
    └── tests/
        ├── test_mandatory.c
        ├── test_bonus.c
        └── test_star.c
ft_printf-Test/             <- ...or here, next to the project
```

The tests use `..` as your project. If `..` has no `ft_printf.h` but
`../ft_printf/` has one, they use that. If your project is somewhere else,
pass `PRINTF_DIR`:

```sh
make PRINTF_DIR=/path/to/ft_printf
```

**Keep the tester out of your repo.** The grader runs norminette on
everything you push. When the tester is inside your project, add it to your
repo's local exclude file. That file works like `.gitignore`, but it is
never pushed:

```sh
echo "ft_printf-Test/" >> .git/info/exclude
```

## Usage

Run these from inside the tester folder:

| Command | What it does |
|---|---|
| `make` | Checks, then the mandatory tests (on `make` in your project) |
| `make bonus` | Checks, then the mandatory and bonus tests (on `make bonus`) |
| `make star` | Only the `*` tests (on `make bonus` if you have that rule) |
| `make check` | Only the [checks](#checks-what-the-grader-sees) |
| `make valgrind` / `valgrind_bonus` / `valgrind_star` | The same tests under valgrind (no checks) |
| `make V=1` / `./run_tests.sh -v` | More detail on each failure (`V=2` / `-vv`: everything) |
| `make 2> errors.log` | `[OK]`/`[KO]` lines on the console, every failure detail in `errors.log` |
| `make build` | Only compiles the test binaries (into `bin/`) |
| `make clean` | Deletes `bin/` |
| `make -C ft_printf-Test remove` | Deletes the whole tester folder. Run it from your project folder (see below) |

`run_tests.sh` takes the same suites, plus `--checks`, `--checks-only` and
`--no-checks`: `./run_tests.sh -v bonus`, `./run_tests.sh --no-checks`.

Before testing, `run_tests.sh` always runs `make` (or `make bonus`) in your
project, so the tests use your current code. If you point `PRINTF_DIR` at a
different project, the test binaries are relinked.

**`make remove`** deletes the tester folder. It refuses unless the folder
contains `run_tests.sh` and `printf_utils.h`. `make` can't move your shell
out of a folder it deletes: a program can't change its parent shell's
folder. So run it from your project folder, `make -C ft_printf-Test remove`.
If you run `make remove` from inside the tester, it ends with
`Removed. Your shell is still in the deleted folder: run  cd ..`.

## Checks (what the grader sees)

The tests use the `libftprintf.a` your Makefile builds in your folder. The
grader clones your repo, and one file that doesn't compile gives 0. The
checks build what the grader gets:

- `Makefile` and `ft_printf.h` exist, and `ft_printf.h` declares
  `int ft_printf(const char *, ...)`
- your own headers are included with quotes, not angle brackets (see
  [`""` or `<>`?](#include--or-))
- **each `.c` compiled on its own**: your project (without `.git`, the
  tester, `.o` and `.a` files) is copied to `bin/check/src`. Each `.c` (`*.c`
  and `*/*.c`) is compiled inside its own folder with
  `cc -Wall -Wextra -Werror -c file.c`, with no `-I`. A file that only
  compiles with an `-I` from your Makefile (e.g. `#include "libft.h"` in a
  top-level file) is a `[!!]` warning. A relative include,
  `#include "libft/libft.h"`, needs no `-I`
- **git**: `[!!]` when a graded file (`Makefile, *.h, */*.h, *.c, */*.c`) is
  not committed or was never added, when commits are not pushed, or when the
  remote has commits you don't have (as of your last `git fetch`). When the
  committed (`HEAD`) or pushed version differs from your files, it is
  compiled too
- **Makefile**, in a clean copy:
  - `make` builds `libftprintf.a` **at the root**
  - the compile commands use `-Wall -Wextra -Werror` (read from `make -n`,
    so `@` doesn't hide them)
  - no `libtool`, only `ar`
  - running `make`, `make all` or `make bonus` again rebuilds nothing (no
    relink)
  - `make clean` removes every `.o` and keeps `libftprintf.a`
  - `make fclean` removes `libftprintf.a`; `make re` rebuilds it
  - the rules `all`, `clean`, `fclean`, `re` and `libftprintf.a` exist
- **external functions**: `nm` must find only `malloc`, `free` and `write`
  undefined in `libftprintf.a`. The `va_*` macros are compiler built-ins. An
  undefined `ft_` function means libft's objects aren't inside
  `libftprintf.a`
- **README.md**: the first line is in italics,
  `*This project has been created as part of the 42 curriculum by <login>*`,
  and there are `Description`, `Instructions` and `Resources` sections
- **norminette** on the graded files only. The tester's own files are never
  checked

`[!!]` is a warning: it doesn't fail the run, but read it before you push.

## How results are compared

`COMPARE(fmt, args...)` (in `printf_utils.h`) does this for each call:

1. It points fd 1 at a temporary file, calls the real `printf(fmt, args...)`
   and reads back what it wrote. `stdout` is unbuffered for the whole test,
   so `printf` writes as it goes, like `ft_printf` must.
2. It does the same with `ft_printf(fmt, args...)`.
3. The bytes must be identical, the return values must be identical, and
   `ft_printf`'s return value must be the number of bytes it really wrote.
4. Every `malloc` made by `ft_printf` during the call is tracked. A block
   still allocated after it returns is reported as a leak, without
   valgrind.

The expected result is always what glibc's `printf` does at run time.
Nothing is written out by hand. `%s` with `NULL` gives `(null)`, `%p` with
`NULL` gives `(nil)`, and `printf("abc%")` writes `abc` and returns -1.

## Reading the output

```
ft_printf: bonus
  [OK] width on every conversion
  [KO] '-': left-justified
      -> tests/test_bonus.c:39: ft_printf("[%-5c]", 'L')
         output differs at byte 1
           printf    wrote   7 byte(s): "[L    ]"
           ft_printf wrote   7 byte(s): "[    L]"
  ...
ft_printf: bonus: 20/21 passed
```

- **[OK]**: every comparison in the case matched.
- **[KO]**: the `->` lines give the test file and line, and the call as it
  is written in the test. Then come what `printf` wrote, what `ft_printf`
  wrote, and the first byte where they differ. Non-printable bytes are shown
  as `\xNN`, so a `'\0'` from `%c` appears as `\x00`. For long outputs, the
  bytes from 16 before the first difference are shown (`-v` shows all).
  These lines go to **stderr**. In a file (`2> errors.log`), each block
  starts with `==== suite: case ====`.
- **return value: printf N, ft_printf M**: the output is right, the count
  isn't. A `'\0'` written by `%c` counts as one byte.
- **ft_printf returned N but wrote M byte(s)**: your count doesn't match
  what you wrote.
- **N block(s) allocated by ft_printf were never freed**: a leak in that
  call.
- **[KO] ... -> SEGFAULT / ABORT / TIMEOUT**: the case crashed or hung, e.g.
  `%s` with `NULL` dereferenced. Each case runs in its own process, so the
  other cases still run.
- **[!!]**: a warning from the checks.

A sweep case (`sweep %d: ...`) runs thousands of comparisons, and it shows
the first 10 failures (50 with `-v`, all with `-vv`).

## What is tested

### Mandatory (`tests/test_mandatory.c`)

| Conversion | Cases |
|---|---|
| `%c` | letters, `'%'`, `'\0'` (written and counted), text around (`" %c %c %c "` with `0` first, middle, last), every value -1..256, `256+'A'`, `INT_MIN`/`INT_MAX`, `\n \r \t` |
| `%s` | basic, `""`, `NULL` → `(null)`, empty strings between text, **`"a\0b"` prints `a`**, `"\0"`, a `%` inside the argument is not a conversion, 10 000 chars, high bytes, UTF-8 |
| format | **a `\0` in the format stops it** (`"ab\0cd%d"`), no conversion at all, `""` returns 0, UTF-8 text in the format, 30 arguments, a 6000-char format with 3000 `%%` |
| `%p` | stack, heap, small values, `NULL` → `(nil)`, `-1`, `LONG_MIN`, `LONG_MAX`, `ULONG_MAX`, `-ULONG_MAX` |
| `%d` `%i` | edge values (±1, ±9, ±10, ±99, ±100, the `CHAR`/`SHRT`/`INT` limits), every value -10000..10000, near `INT_MAX`/`INT_MIN`, powers of ten ± 1 |
| `%u` | edge values, `-1`, `INT_MIN`, near `UINT_MAX`, -2000..2000 |
| `%x` `%X` | edge values, -4200..4200, powers of 16 and their neighbours, `0xdeadbeef` |
| `%%` | alone, repeated, between text, `%%d`, `%c%%` |
| mixed | every conversion in one call, cross types (`"%c %d", 65, 'A'`), the same call twice |
| (undefined) | `%k`, `%y%d`, `%k%d`, a trailing `%`, `%%%`, `% `, `%5`, `%-`, `%#`, `%0`, `%.`; arguments wider than the conversion (`%d` with `LONG_MAX`, `%p` with an `int`) |
| behaviour | no buffering: `ft_printf` and `write(1, ...)` interleave in order |
| (defensive) | `ft_printf(NULL)` like `printf(NULL)` (returns -1); a failing `write` returns -1 like `printf`; every `malloc` failing in turn leaks nothing |

### Bonus (`tests/test_bonus.c`)

| Case | Examples |
|---|---|
| width | `%5c` (also with `'\0'`), `%10s`, `%3s` (shorter than the string), `%20p`, `%15d` |
| `-` | `%-5c`, `%-10s` (with `NULL`), `%-20p` (with `NULL`), `%-12d INT_MIN`, `%-1x` |
| `0` | `%03d -5`, `%015d INT_MIN`, `%05u 0`, `%05x 0`, `%010x INT_MIN` |
| `.` | `%.0d 0` → nothing, `%.d`, `%.5d -42`, `%.20d -1`, `%.0x 0`, `%.3s`, `%.0s`, `%.s`, `%.3s NULL` (glibc: `""`), `%.6s NULL`, `%10.3s`, `%-10.5d` |
| `#` | `%#x 0` → `0`, `%#.0x 0`, `%#5x 0`, `%#-10x`, `%#010x`, `%#.5x` |
| `+` and space | `%+d 0`, `%+.0d 0`, `% d`, `% .0d 0`, `% 5d`, `%+15d INT_MIN` |
| precedence | `-` wins over `0` (`%-05d`), a precision cancels `0` on numbers (`%05.3d`), `+` wins over space (`%+ d`, `% +d`) |
| repeated flags | `%--5d`, `%00005d`, `%++d`, `%  d`, `%-0-0-5d`, `%##x`, `%-+-+ 0 10.5d` |
| (undefined) flags | `%#d`, `%#s`, `%#c`, `%+u`, `% x`, `%05s`, `%.3c`, `%.5p`, `%5%`, `%-10%` |
| (undefined) order | `%5-d`, `%.5-d`, `%.-5d`, `%5.3.2d`, `%5 d`, `%-5.k`, `%10.5k%d` |
| (undefined) sizes | wrong-size arguments with every flag and widths 10..15 |
| sweep | every conversion × 16 flag sets × widths {none, 1, 5, 20} × precisions {none, `.`, `.0`, `.1`, `.5`, `.20`} × 8 values. The combinations the standard defines are in one case per conversion; the others are in an (undefined) sweep |

### Star (`tests/test_star.c`, extra)

`%*d` (width 0, 1, 1000), `%*s` with `NULL`, `%*c` with `'\0'`, a negative
width (left-justified), `*` with every flag, `%.*d`, `%.*s`, `%.*x` with
precision 0, a negative precision (as if there were none), `%*.*d`, several
`*` in one call, and (undefined) `%*%`, `%.*c`, `%**d`.

## What the labels mean

- no label: the C standard defines the output, and it must match `printf`.
- **(undefined)**: the standard leaves it undefined (`%k`, `#` on `%d`,
  `0` on `%s`, a precision on `%c`, flags in the wrong order, an argument of
  the wrong size). It is still compared **strictly** with glibc, and a
  difference is a normal `[KO]`. The message adds
  `(undefined in C: expected is what glibc printf does)`, so you know the
  subject doesn't require it. Strict testers check these.
- **(defensive)**: things the subject doesn't ask for: `ft_printf(NULL)`, a
  failing `write`, `malloc` failing.
- **(extra)**: the `*` suite, which the subject doesn't ask for.

## `#include ""` or `<>`?

- `#include "ft_printf.h"` searches **the folder of the file that has the
  `#include`** first, then the same places as `<>`. Use it for your own
  headers.
- `#include <stdarg.h>` searches only the folders given with `-I`, then the
  system folders. It never looks in the current folder. Use it for system
  headers.

So `#include <ft_printf.h>` works only when something adds `-I.`. The grader
compiles your files with no `-I`, and clang stops with
`'ft_printf.h' file not found with <angled> include; use "quotes" instead`.
The checks catch it before you push.

## Verbose output (`-v`, `-vv`)

`-v` (or `V=1` with make) shows whole outputs instead of the part around the
first difference, longer buffers (300 bytes instead of 48), up to 50
failures per case instead of 10, and full build logs. `-vv` (`V=2`) shows
everything.

## Tips

- Your header needs `#include <stdarg.h>` before it uses `va_list`.
- `%c` with `'\0'` must write one byte and count it. Use `write(1, &c, 1)`,
  not a string function.
- Pass `va_list` by pointer (`va_list *ap`) to helper functions, or use
  `va_copy`. A `va_list` passed by value and used in two functions is
  undefined.
- `%p` reads a `void *`. Print it as `unsigned long` in hex after `0x`, and
  print `(nil)` for `NULL` (Linux).
- `%d` with `INT_MIN`: `-n` overflows an `int`. Convert to `long` before
  negating. `%u`, `%x` and `%X` read an `unsigned int`
  (`va_arg(ap, unsigned int)`), so `-1` prints as `4294967295` / `ffffffff`.
- `%s` with `NULL` and a precision: glibc prints `(null)` only when the
  precision is at least 6. `%.3s` prints nothing.
- The grader tests what you **pushed**. Before you push, run `make` and fix
  every `[!!]` about git.
- To add a case, write another `static void` function in `tests/test_*.c`
  with `COMPARE(...)` lines, and register it in `main()` with
  `run_case("description", fn)`.

## Credits

The test suite was generated with AI (Claude, through Claude Code). It is
used to check the ft_printf project, and it is not part of the submitted
project.

Some cases come from comparing with
[Tripouille/printfTester](https://github.com/Tripouille/printfTester):
arguments wider than the conversion, text around every conversion, and
`'\0'` in a run of `%c`. Others come from
[Sfabi28/printf_tester](https://github.com/Sfabi28/printf_tester): UTF-8 in
the format, `printf(NULL)`, `%-10%`, `%#s`, `%#c`, and the `NULL` cases with
`-` and `.`.
