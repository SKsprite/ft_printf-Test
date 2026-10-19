#!/bin/sh
# Usage: ./run_tests.sh [-v|-vv] [--checks|--checks-only|--no-checks]
#                       [mandatory|bonus|star]      (default: mandatory)
#   mandatory  checks + the mandatory tests (cspdiuxX%), on `make`
#   bonus      checks + mandatory + bonus tests (flags, width, precision),
#              on the library built by `make bonus`
#   star       only the `*` tests (width/precision as arguments, not in the
#              subject), on `make bonus` if your Makefile has it
# Checks run first for mandatory and bonus (--no-checks skips them):
# your files built exactly the way the grader does (each .c on its own, no
# -I), Makefile rules and relink, forbidden functions, README, norminette,
# and files not committed / pushed.
# Env: PRINTF_DIR=path       where your ft_printf is (default .., or
#                            ../ft_printf when .. has no ft_printf.h)
#      RUNNER="valgrind ..." wraps each test binary
#      TEST_TIMEOUT=N        seconds before a single case is killed (default 10)
#      V=1 / V=2             same as -v / -vv (what `make V=1` sets)
# -v, --verbose  more detail on each failure: whole outputs, more failures
#                per case, whole build logs
# -vv            all of it in full
# [OK]/[KO] lines and the summary go to stdout; every failure detail goes to
# stderr, so `./run_tests.sh 2> errors.log` keeps the console short.

cd "$(dirname "$0")" || exit 1

if [ -z "$PRINTF_DIR" ]; then
	if [ ! -f ../ft_printf.h ] && [ -f ../ft_printf/ft_printf.h ]; then
		PRINTF_DIR=../ft_printf
	else
		PRINTF_DIR=..
	fi
fi
export PRINTF_DIR

usage() {
	echo "usage: $0 [-v|-vv] [--checks|--checks-only|--no-checks] [mandatory|bonus|star]"
}
VERBOSE="${V:-0}"
CHECKS=""
MODE=""
for arg in "$@"; do
	case "$arg" in
		-v|--verbose) VERBOSE=$((VERBOSE + 1)) ;;
		-vv) VERBOSE=2 ;;
		--checks) CHECKS=yes ;;
		--checks-only) CHECKS=only ;;
		--no-checks) CHECKS=no ;;
		-h|--help) usage; exit 0 ;;
		-*) usage >&2; exit 2 ;;
		*)
			[ -z "$MODE" ] || { usage >&2; exit 2; }
			MODE="$arg"
			;;
	esac
done
case "$VERBOSE" in
	0|1|2) ;;
	*[!0-9]*|'') echo "V must be 0, 1 or 2 (got '$VERBOSE')" >&2; exit 2 ;;
	*) VERBOSE=2 ;;
esac
export VERBOSE
MODE="${MODE:-mandatory}"
case "$MODE" in
	mandatory|bonus|star) ;;
	*) usage >&2; exit 2 ;;
esac
if [ -z "$CHECKS" ]; then
	if [ "$MODE" = star ]; then CHECKS=no; else CHECKS=yes; fi
fi
TEST_TIMEOUT="${TEST_TIMEOUT:-10}"
export TEST_TIMEOUT

CC="${CC:-cc}"
CFLAGS="-Wall -Wextra -Werror"
ALLOWED="malloc free write"

RED="$(printf '\033[31m')"
GREEN="$(printf '\033[32m')"
YELLOW="$(printf '\033[33m')"
BOLD="$(printf '\033[1m')"
RESET="$(printf '\033[0m')"

mkdir -p bin

# The tester's own folder, relative to PRINTF_DIR, when it is cloned inside
# the project: it is never graded, so every check skips it.
TESTER_REL=""
here=$(pwd -P)
proj=$(cd "$PRINTF_DIR" 2>/dev/null && pwd -P)
case "$here" in
	"$proj"/*) TESTER_REL="${here#"$proj"/}"; TESTER_REL="${TESTER_REL%%/*}" ;;
esac

# Under valgrind, each process logs to its own file in bin/valgrind; after
# each case the non-empty logs are printed to stderr under the case's name
# (run_case() in test_utils.c), so `2> file` puts every report in that file.
VG_LOG_DIR=""
case "$RUNNER" in
	*--log-file*|*--log-fd*|*--log-socket*) ;;
	*valgrind*)
		# Relative (the script cd's to its own directory): $RUNNER is
		# word-split, so an absolute path with a space would break it.
		VG_LOG_DIR="bin/valgrind"
		mkdir -p "$VG_LOG_DIR"
		rm -f "$VG_LOG_DIR"/*.log
		RUNNER="$RUNNER --log-file=$VG_LOG_DIR/%p.log"
		;;
esac
export VG_LOG_DIR

passed=""
failed=""
broken=""

ok()   { echo "  ${GREEN}[OK]${RESET} $1"; }
ko()   { echo "  ${RED}[KO]${RESET} $1"; check_failed=1; }
warn() { echo "  ${YELLOW}[!!]${RESET} $1"; }
hint() { echo "      ${YELLOW}hint: $1${RESET}"; }
# indent [N]: indents stdin, keeping N lines (at least 60 with -v, all with
# -vv) and saying how many were cut.
indent() {
	n="${1:-15}"
	[ "$VERBOSE" -ge 1 ] && [ "$n" -lt 60 ] && n=60
	[ "$VERBOSE" -ge 2 ] && n=1000000000
	awk -v n="$n" '{ if (NR <= n) print "      " $0 }
		END { if (NR > n) printf "      ... %d more line(s) (-v / -vv show more)\n", NR - n }'
}
# log_tail <file>: the end of a build log (where the error is); all of it
# with -v
log_tail() {
	if [ "$VERBOSE" -ge 1 ]; then indent < "$1"; else tail -n 12 "$1" | indent 12; fi
}
count() { set -- $1; echo $#; }
# err_header <what>: names the failure on stderr when it is not the terminal
# (2> file), where the [KO] line on stdout is not next to it.
err_header() { [ -t 2 ] || echo "==== $1 ===="; }

# compile_hints <log>: explains the usual causes of a failed build
compile_hints() {
	if grep -q "file not found with <angled> include" "$1"; then
		hint 'write #include "ft_printf.h" (quotes) for your own headers, not <ft_printf.h>: the grader compiles with no -I, and <> only searches -I and system folders'
	fi
	if grep -q "unknown type name 'va_list'\|va_list.*undeclared\|va_list.*not declared" "$1"; then
		hint "va_list is declared in <stdarg.h>: include it in ft_printf.h before using va_list"
	fi
	if grep -q "implicit declaration of function" "$1"; then
		hint "a function is used before it is declared: add its prototype to your header (or include the right one)"
	fi
	if grep -q "unused parameter\|unused variable\|unused function" "$1"; then
		hint "-Wall -Wextra -Werror turns unused parameters/variables/functions into errors"
	fi
	if grep -q "undefined reference to .ft_" "$1"; then
		hint "an ft_ function is missing from libftprintf.a: libft's objects must be inside libftprintf.a (e.g. copy libft.a to libftprintf.a, then ar rcs your objects into it)"
	fi
}

# ------------------------------------------------------------------ checks
# submake: make for your project, without the flags of the make that
# started this script (`make -s` would silence it, `-k` hide failures).
submake() { env -u MAKEFLAGS -u MFLAGS -u MAKELEVEL make "$@"; }

# graded_files: what the subject turns in (Makefile, *.h, */*.h, *.c,
# */*.c), relative to PRINTF_DIR, never the tester's own folder or .git.
graded_files() {
	(
		cd "$PRINTF_DIR" || exit 0
		find . -maxdepth 2 \( -path ./.git -o -path "./${TESTER_REL:-.git}" \) -prune \
			-o -type f \( -name '*.c' -o -name '*.h' \) -print | sed 's|^\./||'
		[ -f Makefile ] && echo Makefile
	) | sort
}

# copy_project <dest>: a clean copy of your project, without the tester,
# .git, objects and archives (like the grader's fresh clone).
copy_project() {
	rm -rf "$1"
	mkdir -p "$1"
	(cd "$PRINTF_DIR" && tar -cf - --exclude=./.git --exclude="./${TESTER_REL:-.git}" \
		--exclude='*.o' --exclude='*.a' .) | tar -xf - -C "$1"
}

# own_headers_regex: the names of your .h files, as (a\.h|b\.h)
own_headers_regex() {
	graded_files | grep '\.h$' | sed 's|.*/||; s|\.|\\.|g' | sort -u \
		| paste -sd '|' -
}

# compile_each <dir> <label>: every graded .c compiled on its own inside
# its own folder of <dir>, like the grader: cc -Wall -Wextra -Werror -c f.c
# (no -I). A file that only fails because it needs -I to another folder of
# your project is a warning: your Makefile adds that -I.
compile_each() {
	dir="$1"; label="$2"
	bad=""; needs_i=""
	log="bin/check/compile.log"; ilog="bin/check/compile_i.log"
	: > "$log"; : > "$ilog"
	for f in $(cd "$dir" && find . -maxdepth 2 -name '*.c' | sed 's|^\./||' | sort); do
		d=$(dirname "$f"); b=$(basename "$f")
		if (cd "$dir/$d" && $CC $CFLAGS -c "$b" -o /dev/null) >"$log.one" 2>&1; then
			continue
		fi
		missing=$(sed -n "s/.*'\([^']*\.h\)' file not found.*/\1/p; s/.*fatal error: \([^:]*\.h\): No such file.*/\1/p" "$log.one" | head -n 1)
		if [ -n "$missing" ] && ! grep -q "with <angled> include" "$log.one" \
				&& [ -n "$(find "$dir" -name "$(basename "$missing")" | head -n 1)" ]; then
			needs_i="$needs_i $f"
			{ echo "\$> (cd $d && $CC $CFLAGS -c $b)"; cat "$log.one"; } >> "$ilog"
		else
			bad="$bad $f"
			{ echo "\$> (cd $d && $CC $CFLAGS -c $b)"; cat "$log.one"; } >> "$log"
		fi
	done
	rm -f "$log.one"
	if [ -z "$bad$needs_i" ]; then
		ok "$label: every .c compiles on its own ($CC $CFLAGS -c file.c, no -I)"
	fi
	if [ -n "$bad" ]; then
		ko "$label: does not compile on its own:$bad"
		{
			err_header "checks: $label: does not compile on its own"
			indent 30 < "$log"
			compile_hints "$log"
		} >&2
	fi
	if [ -n "$needs_i" ]; then
		warn "$label: compiles only with -I from your Makefile:$needs_i (fine if the grader uses your Makefile; quotes with a relative path, e.g. \"../libft/libft.h\", need no -I)"
		{ err_header "checks: $label: needs -I"; indent 20 < "$ilog"; } >&2
	fi
}

# git_graded <git args...>: runs git on the graded paths only
git_graded() {
	if [ -n "$TESTER_REL" ]; then
		git -C "$PRINTF_DIR" "$@" -- ':(glob)*.c' ':(glob)*/*.c' ':(glob)*.h' \
			':(glob)*/*.h' Makefile ":(exclude)$TESTER_REL"
	else
		git -C "$PRINTF_DIR" "$@" -- ':(glob)*.c' ':(glob)*/*.c' ':(glob)*.h' \
			':(glob)*/*.h' Makefile
	fi
}

check_git() {
	upstream=""
	git -C "$PRINTF_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1 || {
		warn "$PRINTF_DIR is not a git repository: can't check what you pushed"
		return 1
	}
	changes=$(git_graded status --porcelain --untracked-files=all 2>/dev/null)
	if [ -z "$changes" ]; then
		ok "git: every graded file is committed"
	else
		untracked=$(echo "$changes" | awk '$1 == "??" {print $2}' | tr '\n' ' ')
		modified=$(echo "$changes" | awk '$1 != "??" {print $NF}' | tr '\n' ' ')
		[ -n "$modified" ] && warn "git: changes not committed (the grader gets the committed version): $modified"
		[ -n "$untracked" ] && warn "git: never added (the grader won't get them at all): $untracked"
	fi
	upstream=$(git -C "$PRINTF_DIR" rev-parse --abbrev-ref '@{upstream}' 2>/dev/null)
	if [ -z "$upstream" ]; then
		warn "git: this branch has no upstream: can't tell what is pushed"
	else
		ahead=$(git -C "$PRINTF_DIR" rev-list --count '@{upstream}..HEAD' 2>/dev/null)
		behind=$(git -C "$PRINTF_DIR" rev-list --count 'HEAD..@{upstream}' 2>/dev/null)
		if [ "${ahead:-0}" -eq 0 ]; then
			ok "git: no commits waiting to be pushed to $upstream (as of your last fetch)"
		else
			warn "git: $ahead commit(s) not pushed to $upstream: the grader doesn't have them"
		fi
		[ "${behind:-0}" -gt 0 ] && warn "git: $upstream has $behind commit(s) you don't have locally: the grader tests those, not your files (git pull)"
	fi
	[ -n "$changes" ]
}

# git_snapshot <ref> <dest>: your project folder as in commit <ref>
git_snapshot() {
	rm -rf "$2"
	mkdir -p "$2"
	prefix=$(git -C "$PRINTF_DIR" rev-parse --show-prefix)
	git -C "$PRINTF_DIR" archive --format=tar "$1" -- . 2>/dev/null | tar -x -C "$2" 2>/dev/null
	if [ -n "$prefix" ]; then
		mv "$2/$prefix" "$2.tmp" && rm -rf "$2" && mv "$2.tmp" "$2"
	fi
	[ -n "$TESTER_REL" ] && rm -rf "${2:?}/$TESTER_REL"
	find "$2" \( -name '*.o' -o -name '*.a' \) -delete
}

# relink_check <dir> <goal>: running `make <goal>` again must rebuild nothing
relink_check() {
	touch "$1/.relink_marker"
	submake -C "$1" $2 >"$log.2" 2>&1
	rebuilt=$(find "$1" \( -name '*.o' -o -name '*.a' \) -newer "$1/.relink_marker")
	rm -f "$1/.relink_marker"
	if [ -z "$rebuilt" ]; then
		ok "running make${2:+ $2} again does not relink"
	else
		ko "make${2:+ $2} relinks: run again, it rebuilt$(echo $rebuilt | sed "s|$1/||g; s|^| |") (it must do nothing)"
		{ err_header "checks: relink (make${2:+ $2})"; indent < "$log.2"; } >&2
	fi
}

check_makefile() {
	dir="bin/check/make"
	copy_project "$dir"
	if [ ! -f "$dir/Makefile" ]; then
		ko "no Makefile"
		return
	fi
	log="bin/check/make.log"
	if ! submake -C "$dir" >"$log" 2>&1 || [ ! -f "$dir/libftprintf.a" ]; then
		if [ -n "$(find "$dir" -mindepth 2 -name libftprintf.a | head -n 1)" ]; then
			ko "make builds libftprintf.a, but not at the root of your repository"
		else
			ko "make (in a clean copy) does not build libftprintf.a"
		fi
		{ err_header "checks: make"; log_tail "$log"; compile_hints "$log"; } >&2
		return
	fi
	ok "make builds libftprintf.a at the root, from a clean copy"
	# -n -B prints every command make would run, even the ones hidden by @
	submake -C "$dir" -n -B >"$log.dry" 2>&1
	for flag in -Wall -Wextra -Werror; do
		grep -q -- "$flag" "$log.dry" || ko "the Makefile does not compile with $flag"
	done
	if grep -q "libtool" "$log.dry"; then
		ko "the Makefile uses libtool (forbidden: use ar)"
	elif ! grep -Eq "(^|[ /;&|])ar( |$)" "$log.dry"; then
		warn "no 'ar' command seen in make -n: the subject requires ar to create libftprintf.a"
	fi
	rm -f "$log.dry"

	relink_check "$dir" ""
	relink_check "$dir" all

	defined=$(nm "$dir/libftprintf.a" 2>/dev/null | awk 'NF == 3 && $2 ~ /^[TDBRC]$/ {print $3}')
	bad=""
	for sym in $(nm -u "$dir/libftprintf.a" 2>/dev/null | awk '$1 == "U" {print $2}' | sort -u); do
		case "$sym" in _*) continue ;; esac
		echo "$defined" | grep -qx "$sym" && continue
		echo " $ALLOWED " | grep -q " $sym " && continue
		bad="$bad $sym"
	done
	if [ -z "$bad" ]; then
		ok "external functions: only $ALLOWED (va_* are compiler built-ins)"
	else
		ko "undefined in libftprintf.a:$bad (allowed: $ALLOWED)"
		case "$bad" in
			*" ft_"*) hint "ft_ functions must be inside libftprintf.a: put libft's objects in it too" ;;
		esac
	fi

	if [ "$MODE" = bonus ]; then
		if submake -C "$dir" bonus >"$log" 2>&1 && [ -f "$dir/libftprintf.a" ]; then
			ok "make bonus builds libftprintf.a"
			relink_check "$dir" bonus
		else
			ko "make bonus does not build libftprintf.a"
			{ err_header "checks: make bonus"; log_tail "$log"; compile_hints "$log"; } >&2
		fi
	fi

	submake -C "$dir" clean >/dev/null 2>&1
	if [ -n "$(find "$dir" -name '*.o' | head -n 1)" ]; then
		ko "make clean leaves .o files behind: $(cd "$dir" && find . -name '*.o' | head -n 3 | tr '\n' ' ')"
	elif [ ! -f "$dir/libftprintf.a" ]; then
		ko "make clean removes libftprintf.a (only fclean should)"
	else
		ok "make clean removes the .o files and keeps libftprintf.a"
	fi
	submake -C "$dir" fclean >/dev/null 2>&1
	if [ -f "$dir/libftprintf.a" ]; then
		ko "make fclean does not remove libftprintf.a"
	else
		ok "make fclean removes libftprintf.a"
	fi
	if submake -C "$dir" re >"$log" 2>&1 && [ -f "$dir/libftprintf.a" ]; then
		ok "make re rebuilds libftprintf.a"
	else
		ko "make re does not rebuild libftprintf.a"
	fi
	for rule in all clean fclean re libftprintf.a; do
		submake -C "$dir" -n "$rule" >/dev/null 2>&1 || ko "the Makefile has no '$rule' rule"
	done
}

check_readme() {
	readme="$PRINTF_DIR/README.md"
	if [ ! -f "$readme" ]; then
		ko "no README.md at the root of the repository"
		return
	fi
	first=$(head -n 1 "$readme")
	if echo "$first" | grep -Eq '^[*_]This project has been created as part of the 42 curriculum by [^*_ ].*[^*_ ][*_][[:space:]]*$'; then
		ok "README.md: first line in italics with the 42 curriculum sentence"
	else
		ko "README.md: the first line must be in italics: *This project has been created as part of the 42 curriculum by <login>*"
		{ err_header "checks: README first line"; echo "      first line is: $first"; } >&2
	fi
	for section in Description Instructions Resources; do
		grep -Eiq "^#+[[:space:]]*$section" "$readme" \
			|| ko "README.md: no \"$section\" section (a heading like ## $section)"
	done
}

run_checks() {
	check_failed=0
	echo "${BOLD}checks (what the grader sees)${RESET}"
	mkdir -p bin/check

	missing=""
	for f in Makefile ft_printf.h; do
		[ -f "$PRINTF_DIR/$f" ] || missing="$missing $f"
	done
	if [ -z "$missing" ]; then
		ok "files present: Makefile, ft_printf.h"
	else
		ko "missing file(s) at the root of $PRINTF_DIR:$missing"
	fi
	if [ -f "$PRINTF_DIR/ft_printf.h" ]; then
		if grep -Eq 'int[[:space:]]+ft_printf[[:space:]]*\([[:space:]]*const[[:space:]]+char[[:space:]]*\*[^)]*,[[:space:]]*\.\.\.[[:space:]]*\)' "$PRINTF_DIR/ft_printf.h"; then
			ok "ft_printf.h declares int ft_printf(const char *, ...)"
		else
			ko "ft_printf.h does not declare int ft_printf(const char *, ...)"
		fi
	fi

	own=$(own_headers_regex)
	if [ -n "$own" ]; then
		# graded_files cd's itself: never call it after a cd (relative paths)
		angled=$(graded_files | grep -v '^Makefile$' | while read -r f; do
				grep -qE "^[[:space:]]*#[[:space:]]*include[[:space:]]*<($own)>" \
					-- "$PRINTF_DIR/$f" 2>/dev/null && echo "$f"
			done | tr '\n' ' ')
		if [ -z "$angled" ]; then
			ok "your own headers are included with quotes (\"...\")"
		else
			ko "your own header included with <angle brackets> in: $angled"
			hint 'use quotes, e.g. #include "ft_printf.h": the grader compiles with no -I, and <> never looks in the current folder'
		fi
	fi

	copy_project bin/check/src
	compile_each bin/check/src "your files"

	if check_git; then
		# Some graded files are not committed: build what was committed too.
		git_snapshot HEAD bin/check/head
		compile_each bin/check/head "committed version (HEAD)"
	fi
	# The pushed version is what the grader clones: build it if it differs.
	if [ -n "$upstream" ] && ! git_graded diff --quiet '@{upstream}' 2>/dev/null; then
		git_snapshot '@{upstream}' bin/check/pushed
		compile_each bin/check/pushed "pushed version ($upstream, as of your last fetch)"
	fi

	check_makefile
	check_readme

	# The list is made before any cd, and norminette never runs without a
	# file: with none it would check every folder below, this tester too.
	norm_files=$(graded_files | grep -v '^Makefile$')
	if [ -z "$norm_files" ]; then
		ko "norminette: no .c or .h file found in $PRINTF_DIR"
	elif command -v norminette >/dev/null 2>&1; then
		norm=$(cd "$PRINTF_DIR" && norminette $norm_files 2>&1 \
			| grep -v ': OK!$\|^Setting locale')
		if [ -z "$norm" ]; then
			ok "norminette"
		else
			ko "norminette errors (on stderr)"
			{ err_header "checks: norminette"; echo "$norm" | indent 20; } >&2
		fi
	else
		warn "norminette not installed: Norm not checked"
	fi
	echo
	if [ $check_failed -eq 0 ]; then
		passed="$passed checks"
	else
		failed="$failed checks"
	fi
}

[ "$CHECKS" = yes ] || [ "$CHECKS" = only ] && run_checks

# ------------------------------------------------------------------- tests
# Always rebuild libftprintf.a first (make, or make bonus), so the tests
# never run against an old one.
build_lib() {
	goal="$1"
	log="bin/build_lib.log"
	if [ "$goal" = bonus ] && ! submake -C "$PRINTF_DIR" -n bonus >/dev/null 2>&1; then
		echo "${BOLD}libftprintf.a${RESET}"
		echo "  ${RED}[BUILD FAIL]${RESET} your Makefile has no 'bonus' rule"
		echo
		broken="$broken libftprintf.a(bonus)"
		return 1
	fi
	if submake -C "$PRINTF_DIR" $goal >"$log" 2>&1 && [ -f "$PRINTF_DIR/libftprintf.a" ]; then
		return 0
	fi
	echo "${BOLD}libftprintf.a${RESET}"
	echo "  ${RED}[BUILD FAIL]${RESET} make${goal:+ $goal} in $PRINTF_DIR did not build libftprintf.a: no test can run"
	{ err_header "libftprintf.a: build failed"; log_tail "$log"; compile_hints "$log"; } >&2
	echo
	broken="$broken libftprintf.a"
	return 1
}

# run_suite <name>
run_suite() {
	log="bin/build_test_$1.log"
	if ! make -s --no-print-directory "bin/test_$1" >"$log" 2>&1; then
		echo "${BOLD}$1${RESET}"
		echo "  ${RED}[BUILD FAIL]${RESET} could not compile/link the tests against libftprintf.a"
		{ err_header "$1: build failed"; log_tail "$log"; compile_hints "$log"; } >&2
		echo
		broken="$broken $1"
		return
	fi
	TEST_CONTEXT="$1" $RUNNER "./bin/test_$1"
	status=$?
	if [ $status -eq 0 ]; then
		passed="$passed $1"
	else
		[ $status -eq 42 ] && echo "  ${RED}valgrind reported memory errors (log on stderr)${RESET}" >&2
		failed="$failed $1"
	fi
}

# The test binaries are linked against one project's libftprintf.a; make
# only relinks when that file is newer. Pointing PRINTF_DIR at another
# project (with an older library) would run the old binaries: relink.
stamp="bin/.printf_dir"
if [ "$(cat "$stamp" 2>/dev/null)" != "$proj" ]; then
	rm -f bin/test_mandatory bin/test_bonus bin/test_star
	echo "$proj" > "$stamp"
fi

if [ "$CHECKS" != only ]; then
	case "$MODE" in
		mandatory)
			build_lib "" && run_suite mandatory ;;
		bonus)
			build_lib bonus && { run_suite mandatory; run_suite bonus; } ;;
		star)
			if submake -C "$PRINTF_DIR" -n bonus >/dev/null 2>&1; then goal=bonus; else goal=""; fi
			build_lib "$goal" && run_suite star ;;
	esac
fi

echo "${BOLD}==================== SUMMARY ====================${RESET}"
echo "${GREEN}passed:     $(count "$passed")${RESET}"
[ -n "$failed" ] && echo "${RED}failed:     $(count "$failed") ->$failed${RESET}"
[ -n "$broken" ] && echo "${YELLOW}build fail: $(count "$broken") ->$broken${RESET}"

[ -z "$failed" ] && [ -z "$broken" ]
