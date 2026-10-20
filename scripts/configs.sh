# Shared by build.sh, test.sh and launch.sh: the short names those three accept,
# and the CMakePresets.json preset each one stands for. Sourced, not executed.
#
# This is the only place the mapping lives, so the three scripts cannot come to
# disagree about what "asan" means. It maps names and nothing else - build type,
# install prefix, sanitizer flags and build directory are all in the preset,
# which is what the IDE and CI read too.

# Short name, or a full preset name, to a preset name. Prints the preset name on
# success. Fails for anything cmake does not list, so a typo is reported here
# rather than as a cmake error forty lines later.
resolve_preset()
{
	case "$1" in
		debug|dbg)                     printf 'Linux-x64-Debug'; return 0 ;;
		release|rel)                   printf 'Linux-x64-Release'; return 0 ;;
		asan+ubsan|asan|ubsan|san)     printf 'Linux-x64-ASan+UBSan'; return 0 ;;
		tsan)                          printf 'Linux-x64-TSan'; return 0 ;;
		tysan|type)                    printf 'Linux-x64-TySan'; return 0 ;;
		clang-debug|clang-dbg)         printf 'Linux-x64-Clang-Debug'; return 0 ;;
		clang-release|clang|clang-rel) printf 'Linux-x64-Clang-Release'; return 0 ;;
	esac

	# Not shorthand, so it has to be a preset name cmake knows about. Each
	# listing line is two spaces, the name in quotes, then padding and the
	# display name, so the name has to be cut out before comparing.
	if cmake --list-presets 2>/dev/null |
	   sed -n 's/^  "\([^"]*\)".*/\1/p' | grep -qxF "$1"; then
		printf '%s' "$1"
		return 0
	fi
	return 1
}

# Where a preset installs to, read out of the build tree it configured rather
# than duplicated here. CMakePresets.json sets CMAKE_INSTALL_PREFIX and is
# entitled to change it without this file knowing.
resolve_prefix()
{
	local preset="$1" cache="out/build/$1/CMakeCache.txt"
	[ -f "$cache" ] || return 1
	sed -n 's/^CMAKE_INSTALL_PREFIX:PATH=//p' "$cache" | tail -1
}

readonly CONFIG_NAMES="debug release asan+ubsan tsan tysan clang-debug clang-release"
