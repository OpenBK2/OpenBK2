#!/usr/bin/env bash
#
# Run an installed build, optionally against a different game's data.
#
# Two independent choices:
#
#   which code    the configuration argument, the same short names ./build.sh
#                 takes. Resolved to that preset's CMAKE_INSTALL_PREFIX.
#   which data    --data, naming one of the standalone trees in $HOME. Without
#                 it the prefix's own Data is used.
#
# So ./launch.sh asan+ubsan --data retail runs the sanitizer build against the
# retail edition. --editor selects the map editor and --gdb runs under gdb,
# so neither needs a script of its own.
#
# The runtime options each sanitizer wants are set here and are overridable:
# export ASAN_OPTIONS yourself and that is what the process gets.
# docs/Sanitizers.md says what they mean.
#
# How a data tree is laid out and why
#
# Each tree holds data only - Data/, Profiles/, splash.bmp - and its bin/ is a
# farm of symlinks into the install prefix, rebuilt on every run. The game
# resolves ../Data and ../Profiles from the working directory, so it is the cd
# into the tree's own bin that selects which game's data gets loaded; the
# symlinks only supply code. That way one build serves every tree, a rebuild is
# picked up by all of them at once, and no tree carries its own ~700 MB copy of
# the modules.
#
# Known limitation: the editor asks GetExecutablePath where it lives, and
# through a symlink that answers the install prefix, not the tree. Game does
# not - it goes by the working directory - so --editor with --data is not
# trustworthy yet.

set -u -o pipefail

readonly SELF="$(basename "$0")"
readonly SRCDIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SRCDIR" || exit 1
. scripts/configs.sh

usage()
{
	cat <<-'USAGE'
	Usage: ./launch.sh [config] [options] [-- game args]

	Which code to run - the same configuration names ./build.sh takes, each
	resolved to its preset's install prefix:

	  debug, release            ~/bk2          (debug is the default)
	  asan+ubsan                ~/bk2-asan
	  tsan                      ~/bk2-tsan
	  clang-debug, clang-release  ~/bk2-clang

	BK2_PREFIX overrides the lookup entirely.

	Options:
	  -d, --data NAME|PATH   run against a standalone data tree instead of the
	                         prefix's own Data. A name means ~/bk2-NAME; a path
	                         with a slash is taken as is.
	      --list-data        list the data trees found, and exit
	  -r, --replay FILE      play a recorded replay instead of showing the menus,
	                         by passing --replay= to the game
	      --software-vk      force the lavapipe software Vulkan device, so a run
	                         needs no GPU. Quietens the DXVK log too.
	  -e, --editor           run B2_MapEditor instead of Game
	  -g, --gdb              run under gdb
	      --env              print the environment that would be used, and exit
	  -h, --help             this text

	Examples:
	  ./launch.sh                          debug build, repository data
	  ./launch.sh release
	  ./launch.sh release --data retail
	  ./launch.sh asan+ubsan --data totalconv
	  ./launch.sh debug --editor
	  ./launch.sh asan+ubsan --gdb
	  ASAN_OPTIONS=detect_leaks=1 ./launch.sh asan+ubsan
	USAGE
}

die()
{
	printf '%s: %s\n' "$SELF" "$*" >&2
	exit 1
}

# Whether a directory holds data the game can actually load. index.bin is the
# database index and every edition has one. GameRoot.xdb is not a usable test:
# it is a loose file only in the development tree, and the shipped editions keep
# it inside a .pak.
#
# Testing for the directory alone is not enough, because cmake/fontpak.cmake
# installs the font pack with DESTINATION Data, so cmake --install leaves a Data
# directory holding that one file and nothing else.
has_game_data()
{
	[ -e "$1/index.bin" ]
}

# A data tree is a directory holding game data. The install prefixes match the
# same $HOME/bk2-* glob and are not trees, so they are named here; the list
# mirrors the CMAKE_INSTALL_PREFIX values in CMakePresets.json.
readonly -a NOT_TREES=(bk2 bk2-asan bk2-tsan bk2-clang)

is_install_prefix()
{
	local name="${1##*/}" p
	for p in "${NOT_TREES[@]}"; do
		[ "$name" = "$p" ] && return 0
	done
	return 1
}

list_trees()
{
	local dir
	for dir in "$HOME"/bk2-*; do
		[ -e "$dir/Data" ] || continue
		is_install_prefix "$dir" && continue
		printf '  %s\n' "${dir##*/bk2-}"
	done
}

CONFIG=debug
EXE=Game
TREE=
USE_GDB=no
SHOW_ENV=no
REPLAY=
SOFTWARE_VK=no
# The lavapipe ICD, which is what makes a run possible with no GPU. Debian names
# it without an architecture suffix.
readonly LVP_ICD=/usr/share/vulkan/icd.d/lvp_icd.json

if [ $# -gt 0 ] && [ "${1#-}" = "$1" ]; then
	CONFIG="$1"
	shift
fi

while [ $# -gt 0 ]; do
	case "$1" in
		-h|--help)    usage; exit 0 ;;
		-d|--data)    [ $# -ge 2 ] || die "$1 needs a tree name or path"
		              case "$2" in
		                  */*) TREE="$2" ;;
		                  *)   TREE="$HOME/bk2-$2" ;;
		              esac
		              shift 2 ;;
		--list-data)  printf 'data trees found:\n'; list_trees; exit 0 ;;
		-r|--replay)  [ $# -ge 2 ] || die "$1 needs a replay file"
		              REPLAY="$2"; shift 2 ;;
		--software-vk) SOFTWARE_VK=yes; shift ;;
		-e|--editor)  EXE=B2_MapEditor; shift ;;
		-g|--gdb)     USE_GDB=yes; shift ;;
		--env)        SHOW_ENV=yes; shift ;;
		--)           shift; break ;;
		*)            die "unknown option '$1'. ./launch.sh --help" ;;
	esac
done

PRESET="$(resolve_preset "$CONFIG")" ||
	die "unknown configuration '$CONFIG'. One of: $CONFIG_NAMES"

# The prefix comes out of the build tree the preset configured, so this script
# never has to repeat what CMakePresets.json already says.
if [ -n "${BK2_PREFIX:-}" ]; then
	PREFIX="${BK2_PREFIX%/}"
else
	PREFIX="$(resolve_prefix "$PRESET")" ||
		die "out/build/$PRESET has no CMakeCache.txt. Configure it first: ./build.sh $CONFIG"
	[ -n "$PREFIX" ] ||
		die "no CMAKE_INSTALL_PREFIX in out/build/$PRESET/CMakeCache.txt"
	PREFIX="${PREFIX%/}"
fi

[ -x "$PREFIX/bin/$EXE" ] ||
	die "$PREFIX/bin/$EXE is not there. Build and install it first: ./build.sh $CONFIG"

if [ -n "$TREE" ]; then
	# --data: the tree supplies the data, the prefix supplies the code.
	TREE="${TREE%/}"
	[ -d "$TREE" ] || { printf '%s: no such tree: %s\ndata trees found:\n' "$SELF" "$TREE" >&2
	                    list_trees >&2; exit 1; }
	has_game_data "$TREE/Data" ||
		die "$TREE/Data has no index.bin, so it is not a loadable data tree"

	# The game writes into Profiles - global.cfg and the player's profile - so a
	# tree sharing one with another tree would overwrite its settings.
	[ -L "$TREE/Profiles" ] &&
		printf '%s: warning: %s/Profiles is a symlink and the game writes to it\n' "$SELF" "$TREE" >&2

	# Refresh the symlink farm. Previous links go first, dangling ones included,
	# so a module that was renamed or dropped does not linger as a stale link.
	mkdir -p "$TREE/bin"
	find "$TREE/bin" -maxdepth 1 -type l -delete
	for src in "$PREFIX/bin"/lib*.so* "$PREFIX/bin"/*.ico \
		"$PREFIX/bin/Game" "$PREFIX/bin/B2_MapEditor" "$PREFIX/bin/crashpad_handler" \
		"$PREFIX/bin/Server" "$PREFIX/bin/TestClient"; do
		[ -e "$src" ] || continue
		name="${src##*/}"
		# A real file of the same name shadows the link. Those are the per-tree
		# copies of the build that predate this arrangement; drop them.
		rm -f "$TREE/bin/$name"
		ln -s "$src" "$TREE/bin/$name"
	done

	# Install, or refresh, the font pack that cmake/fontpak.cmake builds.
	# Dropping the archive into Data is the whole installation: the VFS globs
	# *.pak out of the directory (WinVFS.cpp) and the newest entry inside any
	# archive wins, so the records with FontFile set take over from the baked
	# fonts in the edition's own texts.pak. No index.bin edit, no data conversion.
	FONTS="${BK2_FONTS:-$PREFIX/Data/obk2_fonts.pak}"
	if [ -f "$FONTS" ]; then
		font_dest="$TREE/Data/${FONTS##*/}"
		# Skipped when the tree's Data is the directory the pack came from.
		if [ "$(realpath "$FONTS")" != "$(realpath -m "$font_dest")" ] &&
		   { [ ! -e "$font_dest" ] || [ "$FONTS" -nt "$font_dest" ]; }; then
			cp -p "$FONTS" "$font_dest"
			printf '%s: installed %s into %s/Data\n' "$SELF" "${FONTS##*/}" "$TREE"
		fi
	else
		printf '%s: warning: no font pack at %s, the baked fonts will be drawn\n' "$SELF" "$FONTS" >&2
	fi

	RUNDIR="$TREE/bin"
else
	# No --data: run out of the prefix, which needs the data beside bin/.
	# cmake --install installs code, and the font pack, and nothing else.
	if ! has_game_data "$PREFIX/Data" || [ ! -e "$PREFIX/Profiles" ]; then
		printf '%s: %s/Data has no index.bin, so the game cannot load its database.\n\n' \
			"$SELF" "$PREFIX" >&2
		if [ -d "$PREFIX/Data" ] && [ ! -L "$PREFIX/Data" ]; then
			printf 'That directory exists because cmake --install put the font pack in it.\n' >&2
			printf 'It has to be removed before the link is made: ln places a link inside an\n' >&2
			printf 'existing directory rather than replacing it, and -n only guards against\n' >&2
			printf 'that when the target is already a symlink.\n\n' >&2
		fi
		printf '  rm -rf "%s/Data"\n' "$PREFIX" >&2
		printf '  ln -sfnT "%s/Versions/Current/Data" "%s/Data"\n' "$SRCDIR" "$PREFIX" >&2
		printf '  cp -r "%s/Versions/Current/Profiles" "%s/Profiles"\n' "$SRCDIR" "$PREFIX" >&2
		printf '  cp "%s/Versions/Current/splash.bmp" "%s/splash.bmp"\n\n' "$SRCDIR" "$PREFIX" >&2
		printf 'With Data a link, a later install writes the font pack through it into the\n' >&2
		printf 'source tree, which is where the VFS has to glob it from.\n\n' >&2
		printf 'Or leave the prefix alone and run against a standalone tree:\n' >&2
		printf '  ./launch.sh %s --data retail\n' "$CONFIG" >&2
		exit 1
	fi
	RUNDIR="$PREFIX/bin"
fi

# Sanitizer runtime options, keyed on the prefix rather than the configuration
# name so BK2_PREFIX pointing at a sanitizer tree still gets them. Defaults
# only: an exported value wins.
#
# detect_leaks=0 to start with - the object model frees on refcount and a clean
# exit is not one of its guarantees, so the shutdown leak report buries
# everything found while playing. print_stacktrace is not a UBSan default and a
# report without it names a source line with no path to it.
#
# DXVK_WSI_DRIVER has no default and the build wants SDL3. LD_LIBRARY_PATH is
# the run directory, which for a tree is its farm of symlinks.
#
# A software-device run is usually a batch one, so it defaults the DXVK log
# quiet instead of the debug level an interactive run wants.
declare -a ENV_ARGS=(
	"DXVK_WSI_DRIVER=${DXVK_WSI_DRIVER:-SDL3}"
	"DXVK_LOG_LEVEL=${DXVK_LOG_LEVEL:-$( [ "$SOFTWARE_VK" = yes ] && echo none || echo debug )}"
	"LD_LIBRARY_PATH=$RUNDIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
)
if [ "$SOFTWARE_VK" = yes ]; then
	[ -e "$LVP_ICD" ] || die "no lavapipe ICD at $LVP_ICD; install mesa-vulkan-drivers"
	# Both names: the loader took VK_ICD_FILENAMES before it took VK_DRIVER_FILES,
	# and which one is honoured depends on the loader version rather than ours.
	ENV_ARGS+=(
		"VK_DRIVER_FILES=$LVP_ICD"
		"VK_ICD_FILENAMES=$LVP_ICD"
	)
fi
case "$PREFIX" in
	*-asan)
		ENV_ARGS+=(
			"ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0:abort_on_error=1}"
			"UBSAN_OPTIONS=${UBSAN_OPTIONS:-print_stacktrace=1}"
		) ;;
	*-tsan)
		# Add suppressions=<file> once one exists; TSan errors out if it is
		# named and missing. DXVK, the graphics driver and SDL3 are prebuilt, so
		# TSan reports their internal synchronisation as races.
		ENV_ARGS+=( "TSAN_OPTIONS=${TSAN_OPTIONS:-history_size=4}" ) ;;
esac

if [ "$SHOW_ENV" = yes ]; then
	printf '%s\n' "${ENV_ARGS[@]}"
	printf 'rundir=%s\nexe=%s\n' "$RUNDIR" "$EXE"
	[ -n "$REPLAY" ] && printf 'args=--replay=%s\n' "$REPLAY"
	exit 0
fi

declare -a CMD=(env "${ENV_ARGS[@]}")
[ "$USE_GDB" = yes ] && CMD+=(gdb --args)
CMD+=("./$EXE")
# Before the caller's own arguments, so an explicit --replay= after -- wins.
[ -n "$REPLAY" ] && CMD+=("--replay=$REPLAY")
CMD+=("$@")

# exec from the run directory: everything the game opens is resolved relative to
# it, which is the point of the cd, and exec makes the exit status the game's.
printf '+ cd %s && %s\n' "$RUNDIR" "${CMD[*]}" >&2
cd "$RUNDIR" && exec "${CMD[@]}"
