#!/usr/bin/env bash
#
# One entry point for every local Linux build.
#
# Nothing about a configuration is defined here. CMakePresets.json is the one
# source of truth - the IDE, ctest, CI and this script all read it - and this
# file only supplies the short names and the configure-build-install sequence.
#
# DXSDK_ROOT is gone: cmake/dxsdk.cmake is only included under if(WIN32), and
# every dxsdk:: link is guarded the same way, so the stub tree it pointed at has
# nothing left to satisfy. PKG_CONFIG_PATH for DXVK is in base-linux's
# environment block; cmake/dxvk.cmake pkg_check_modules it as REQUIRED, so the
# path is not optional.
#
# ccache is not set up here either. cmake/ccache.cmake finds it on PATH at
# configure time and wraps it with the CCACHE_SLOPPINESS the precompiled headers
# require; setting CMAKE_CXX_COMPILER_LAUNCHER from a preset would shadow that
# and silently make every PCH compilation uncacheable. Turn it off with
# --no-ccache.
#
# What each sanitizer build is for, what it will report first and which runtime
# options it needs is in docs/Sanitizers.md.

set -u -o pipefail

readonly SELF="$(basename "$0")"
cd "$(dirname "$0")" || exit 1
. scripts/configs.sh

usage()
{
	cat <<-'USAGE'
	Usage: ./build.sh [config] [options] [-- native tool args]

	Configurations, and the CMakePresets.json preset each is short for:

	  debug           Linux-x64-Debug            -> ~/bk2        (default)
	  release         Linux-x64-Release          -> ~/bk2
	  asan+ubsan      Linux-x64-ASan+UBSan       -> ~/bk2-asan
	  tsan            Linux-x64-TSan             -> ~/bk2-tsan
	  tysan           Linux-x64-TySan            -> ~/bk2-tysan
	  clang-debug     Linux-x64-Clang-Debug      -> ~/bk2-clang
	  clang-release   Linux-x64-Clang-Release    -> ~/bk2-clang

	Shorthand: dbg, rel, asan, ubsan, san, type, clang. A full preset name works too,
	so anything cmake --list-presets prints can be passed straight through.

	Build trees are ${sourceDir}/out/build/<preset name>.

	Options:
	  -t, --target NAME   build only this target; repeatable. Implies
	                      --no-install.
	  -j, --jobs N        parallel jobs (default: all cores)
	      --no-install    configure and build, do not install
	      --no-editor     configure with BUILD_EDITOR=OFF. Roughly halves the
	                      build, and for tsan drops the wxWidgets and GTK
	                      threads out of the reports.
	      --no-ccache     configure with USE_CCACHE=OFF. Needed when comparing
	                      object files before and after a whitespace-only
	                      change: ccache keys on the preprocessed source, so a
	                      cache hit would make that comparison meaningless.
	      --fresh         discard the existing CMake cache before configuring
	      --configure     configure only, do not build
	  -D VAR=VALUE        extra cache variable, passed to configure
	  -h, --help          this text

	Anything after -- goes to ninja. The presets already pass -k 0, so a
	failing translation unit does not stop the rest of the build.

	Examples:
	  ./build.sh                        debug, build everything, install
	  ./build.sh release
	  ./build.sh asan+ubsan --no-editor
	  ./build.sh tsan -t Game
	  ./build.sh clang-release -- -v

	Each tree is 11-14 GB and they are not shared. See docs/Sanitizers.md.
	USAGE
}

die()
{
	printf '%s: %s\n' "$SELF" "$*" >&2
	exit 1
}

# Echo a command and run it, so the log says what produced what. The old
# scripts used set -x, which also traced the shell's own bookkeeping.
run()
{
	printf '\n+ %s\n' "$*" >&2
	"$@"
}

PRESET=Linux-x64-Debug
JOBS="$(nproc)"
DO_INSTALL=yes
DO_BUILD=yes
FRESH=no
declare -a TARGETS=()
declare -a CACHE_ARGS=()
declare -a NATIVE_ARGS=()

CONFIG=debug

# The first argument is the configuration if it is not an option. Checked
# before the option loop so that ./build.sh release reads naturally and a typo
# in a configuration name is reported as such rather than as a bad option.
if [ $# -gt 0 ] && [ "${1#-}" = "$1" ]; then
	CONFIG="$1"
	shift
fi
PRESET="$(resolve_preset "$CONFIG")" ||
	die "unknown configuration '$CONFIG'. One of: $CONFIG_NAMES, or a preset name from cmake --list-presets"

while [ $# -gt 0 ]; do
	case "$1" in
		-h|--help)    usage; exit 0 ;;
		-t|--target)  [ $# -ge 2 ] || die "$1 needs a target name"
		              TARGETS+=("$2"); DO_INSTALL=no; shift 2 ;;
		-j|--jobs)    [ $# -ge 2 ] || die "$1 needs a job count"
		              JOBS="$2"; shift 2 ;;
		--no-install) DO_INSTALL=no; shift ;;
		--no-editor)  CACHE_ARGS+=("-DBUILD_EDITOR=OFF"); shift ;;
		--no-ccache)  CACHE_ARGS+=("-DUSE_CCACHE=OFF"); shift ;;
		--fresh)      FRESH=yes; shift ;;
		--configure)  DO_BUILD=no; DO_INSTALL=no; shift ;;
		-D)           [ $# -ge 2 ] || die "-D needs VAR=VALUE"
		              CACHE_ARGS+=("-D$2"); shift 2 ;;
		-D*)          CACHE_ARGS+=("$1"); shift ;;
		--)           shift; NATIVE_ARGS=("$@"); break ;;
		*)            die "unknown option '$1'. ./build.sh --help" ;;
	esac
done

declare -a CONFIGURE_ARGS=(--preset "$PRESET")
[ "$FRESH" = yes ] && CONFIGURE_ARGS+=(--fresh)
[ ${#CACHE_ARGS[@]} -gt 0 ] && CONFIGURE_ARGS+=("${CACHE_ARGS[@]}")

run cmake "${CONFIGURE_ARGS[@]}" || die "configure failed"

if [ "$DO_BUILD" = yes ]; then
	declare -a BUILD_ARGS=(--build --preset "$PRESET" -j "$JOBS")
	for t in "${TARGETS[@]}"; do
		BUILD_ARGS+=(--target "$t")
	done
	[ ${#NATIVE_ARGS[@]} -gt 0 ] && BUILD_ARGS+=(-- "${NATIVE_ARGS[@]}")

	run cmake "${BUILD_ARGS[@]}" || die "build failed"
fi

if [ "$DO_INSTALL" = yes ]; then
	run cmake --build --preset "$PRESET" --target install || die "install failed"
fi

# Where it went and how to run it, because the prefix depends on the
# configuration and the data trees are a separate thing from the install.
printf '\n%s: %s done' "$SELF" "$PRESET"
PREFIX="$(resolve_prefix "$PRESET")" && [ -n "$PREFIX" ] &&
	printf ', installed to %s' "$PREFIX"
printf '\n'
if [ "$DO_INSTALL" = yes ]; then
	printf 'run it with:  ./launch.sh %s\n' "$CONFIG"
fi
