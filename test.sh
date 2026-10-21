#!/usr/bin/env bash
#
# Build and run the unit tests for one configuration.
#
# Plain ctest finds nothing here, which is the whole reason this script exists.
# Every test target is EXCLUDE_FROM_ALL (cmake/unittest.cmake), ctest cannot
# build EXCLUDE_FROM_ALL targets itself, and so an unprepared tree reports
# "Unable to find executable" for each one. The unittests aggregate target is
# what builds them; this runs that first and then hands over to ctest.
#
# The label matters too. The dependencies register around 150 tests of their
# own and cmake/benchmarks.cmake registers the benchmarks under a label that
# shares the obk2 prefix, so the filter has to be exactly obk2-test. It lives
# in the test presets rather than here.
#
# There is already a target that does the build-then-ctest pair:
# add_suite_runner in cmake/benchmarks.cmake defines run-unittests, which
# depends on unittests and runs ctest -L obk2-test --output-on-failure. This
# script exists for what that target cannot do - choose a configuration, pass
# arguments through to ctest (-R, --rerun-failed, --repeat until-fail), and
# supply the sanitizer runtime options from the test preset. For the plain case
# the two are equivalent:
#
#   cmake --build --preset Linux-x64-Debug --target run-unittests

set -u -o pipefail

readonly SELF="$(basename "$0")"
cd "$(dirname "$0")" || exit 1
. scripts/configs.sh

usage()
{
	cat <<-'USAGE'
	Usage: ./test.sh [config] [options] [-- ctest args]

	Configurations are the same as ./build.sh: debug (default), release,
	asan+ubsan, tsan, tysan, clang-debug, clang-release, or any preset name.

	Options:
	  -j, --jobs N      parallel jobs, for both the build and the test run
	      --no-build    run ctest against the tree as it stands
	      --list        list the tests that would run, do not run them
	  -R, --tests-regex RE   only tests whose name matches RE
	  -h, --help        this text

	Anything after -- goes to ctest, so --repeat until-fail:20,
	--output-junit results.xml and the rest are available unchanged.

	The sanitizer configurations set their own ASAN_OPTIONS, UBSAN_OPTIONS,
	TSAN_OPTIONS and TYSAN_OPTIONS from the test preset; docs/Sanitizers.md
	says what they mean and what to expect. Export the variable yourself to
	override one. Note that tysan does not fail a test on a report: it prints
	and carries on, so read the output rather than the exit status.

	Examples:
	  ./test.sh                            build and run the suite, debug
	  ./test.sh release -R Sample
	  ./test.sh asan+ubsan
	  ./test.sh debug --no-build -- --rerun-failed
	USAGE
}

die()
{
	printf '%s: %s\n' "$SELF" "$*" >&2
	exit 1
}

run()
{
	printf '\n+ %s\n' "$*" >&2
	"$@"
}

CONFIG=debug
JOBS="$(nproc)"
DO_BUILD=yes
declare -a CTEST_ARGS=()

if [ $# -gt 0 ] && [ "${1#-}" = "$1" ]; then
	CONFIG="$1"
	shift
fi
PRESET="$(resolve_preset "$CONFIG")" ||
	die "unknown configuration '$CONFIG'. One of: $CONFIG_NAMES, or a preset name from cmake --list-presets"

while [ $# -gt 0 ]; do
	case "$1" in
		-h|--help)    usage; exit 0 ;;
		-j|--jobs)    [ $# -ge 2 ] || die "$1 needs a job count"
		              JOBS="$2"; shift 2 ;;
		--no-build)   DO_BUILD=no; shift ;;
		--list)       CTEST_ARGS+=(--show-only); shift ;;
		-R|--tests-regex)
		              [ $# -ge 2 ] || die "$1 needs a regex"
		              CTEST_ARGS+=(-R "$2"); shift 2 ;;
		--)           shift; CTEST_ARGS+=("$@"); break ;;
		*)            die "unknown option '$1'. ./test.sh --help" ;;
	esac
done

# ctest cannot do this itself, and a list of test target names in a script
# would go stale the first time someone adds a test.
if [ "$DO_BUILD" = yes ]; then
	run cmake --build --preset "$PRESET" -j "$JOBS" --target unittests ||
		die "building the unittests target failed"
fi

run ctest --preset "$PRESET" -j "$JOBS" "${CTEST_ARGS[@]+"${CTEST_ARGS[@]}"}"
status=$?

# noTestsAction=error in the preset, so an empty run is a failure rather than a
# pass. Say why, since the cause is almost always a tree that was never built.
if [ $status -ne 0 ] && [ "$DO_BUILD" = no ]; then
	printf '\n%s: --no-build was given; if every test was missing, drop it.\n' "$SELF" >&2
fi
exit $status
