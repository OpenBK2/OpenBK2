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
	  tysan                     ~/bk2-tysan
	  clang-debug, clang-release  ~/bk2-clang

	BK2_PREFIX overrides the lookup entirely.

	Options:
	  -d, --data NAME|PATH   run against a standalone data tree instead of the
	                         prefix's own Data. A name means ~/bk2-NAME; a path
	                         with a slash is taken as is.
	      --list-data        list the data trees found, and exit
	  -M, --mod NAME         attach a mod, naming a folder under the data tree's
	                         MODs directory. Without this the profile's own
	                         remembered choice is used, and --mod "" overrides
	                         that back to unmodded. Attaching is remembered, so
	                         a mod picked here persists into later runs of the
	                         same tree until another --mod says otherwise.
	      --list-mods        list the mods installed in the tree, and exit
	  -m, --mission MAP      start a mission instead of showing the menus, by
	                         passing --mission= to the game. MAP names a MapInfo
	                         record, e.g. Custom/Missions/Razvedka/MapInfo.xdb
	  -c, --campaign SPEC    start one mission of a campaign, by passing
	                         --campaign= to the game. SPEC is
	                         <campaign>[;<chapter>[;<mission>[;<difficulty>]]],
	                         e.g. Scenario/Campaigns/GER/Campaign.xdb;0;0;normal
	  -r, --replay FILE      play a recorded replay instead of showing the menus,
	                         by passing --replay= to the game
	      --replay-player N  watch the replay as player N rather than player 0,
	                         by passing --replay-player= to the game. The
	                         simulation must play out the same whoever watches,
	                         so --verify as each player in turn is a check of
	                         its own. Needs --replay.
	      --verify           play the replay out and exit, reporting through the
	                         exit status: 0 if it reached the last recorded
	                         segment and every recorded checksum matched, 1 for
	                         anything else. Needs --replay. Sampling comes from
	                         the replay, so record with a smaller
	                         checksumperiod to compare more finely.
	                         See docs/TestHarness.md.
	      --exit-after-load  load the map and leave, printing how long each phase
	                         of loading took. Needs --mission, --campaign or
	                         --replay, since on its own there is no map to load.
	                         This is what makes a load profilable: it is one
	                         blocking call, so a run that stays in the mission
	                         afterwards goes on sampling a frame loop instead.
	                         A profiler that attaches by process id has to be
	                         running before the load starts, which it cannot be
	                         if it is waiting for the id; pass
	                         --setvar Mission.LoadProfileDelay=10 and the game
	                         prints its id and waits that many seconds first.
	                         docs/MapLoadProfiling.md has the recipes.
	      --setvar N=V       set a game variable after the configs load, so it
	                         overrides them. Repeatable.
	      --software-vk      force the lavapipe software Vulkan device, so a run
	                         needs no GPU. Quietens the DXVK log too.
	      --hud [WHICH]      draw a frame rate overlay. WHICH is dxvk, the
	                         default, which uses the HUD built into the DXVK the
	                         game already renders through and needs nothing
	                         installed; or mango, which enables MangoHud's
	                         Vulkan layer. DXVK_HUD and MangoHud's own
	                         configuration say what each one draws.
	      --profile [TOOL]   record a CPU profile of the whole run. TOOL is
	                         perforator, the default, or perf. Both launch the
	                         game themselves and stop when it exits, so there is
	                         no process to attach to and no race to lose, and the
	                         profile covers startup as well as whatever the run
	                         was for. Pair with --exit-after-load to profile a
	                         map load and nothing else.
	                         Both need privileges: perforator is eBPF and wants
	                         root, perf wants kernel.perf_event_paranoid lowered.
	                         sudo is used for perforator, and the game is put
	                         back to this user with setpriv so the run does not
	                         leave root-owned files in the data tree.
	                         BK2_PERFORATOR names the binary when it is not on
	                         PATH.
	      --profile-out F    where the profile goes. A base name: the extension
	                         is supplied, and a known one on F is replaced, so
	                         /tmp/mi, /tmp/mi.pprof and /tmp/mi.html all name
	                         the same pair. Default is timestamped and beside
	                         this script.
	                         perforator records pprof and renders the flame
	                         graph from it, so a run leaves both .pprof, which
	                         go tool pprof and perforator profile text read,
	                         and .html to look at. perf leaves .data for
	                         perf report.
	  -e, --editor           run B2_MapEditor instead of Game
	  -g, --gdb              run under gdb
	      --valgrind         run under valgrind's memcheck, which reports the
	                         use of an uninitialised value of any type, not just
	                         the bool and enum loads UBSan can see. Needs a
	                         build with -DUSE_MIMALLOC=OFF and must not be a
	                         sanitizer one; both are checked. VALGRIND_OPTS
	                         replaces the options, VALGRIND_EXTRA adds to them
	      --env              print the environment that would be used, and exit
	  -h, --help             this text

	Examples:
	  ./launch.sh                          debug build, repository data
	  ./launch.sh release
	  ./launch.sh release --data retail
	  ./launch.sh release --mission Custom/Missions/Razvedka/MapInfo.xdb
	  ./launch.sh release --campaign Scenario/Campaigns/GER/Campaign.xdb;0;0
	  ./launch.sh asan+ubsan --data totalconv
	  ./launch.sh debug --editor
	  ./launch.sh asan+ubsan --gdb
	  ./launch.sh release --hud
	  ./launch.sh release --hud mango --data retail
	  ./launch.sh release --data totalconv --mission Custom/Missions/Razvedka/MapInfo.xdb --exit-after-load
	  ./launch.sh release --valgrind --data retail
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
readonly -a NOT_TREES=(bk2 bk2-asan bk2-tsan bk2-tysan bk2-clang)

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
MOD=
MOD_GIVEN=no
LIST_MODS=no
USE_GDB=no
USE_VALGRIND=no
SHOW_ENV=no
REPLAY=
REPLAY_PLAYER=
MISSION=
CAMPAIGN=
VERIFY=no
EXIT_AFTER_LOAD=no
PROFILER=
PROFILE_OUT=
declare -a SETVARS=()
SOFTWARE_VK=no
HUD=
# The lavapipe ICD, which is what makes a run possible with no GPU. Debian names
# it without an architecture suffix.
readonly LVP_ICD=/usr/share/vulkan/icd.d/lvp_icd.json
# Where a MangoHud install drops its implicit layer manifest. The file name
# carries an architecture suffix on some distributions and not on others, so it
# is globbed rather than named.
readonly -a MANGOHUD_LAYER_DIRS=(
	/usr/share/vulkan/implicit_layer.d
	/usr/local/share/vulkan/implicit_layer.d
	"$HOME/.local/share/vulkan/implicit_layer.d"
)

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
		-M|--mod)     [ $# -ge 2 ] || die "$1 needs a mod folder name, or \"\" for none"
		              MOD="$2"; MOD_GIVEN=yes; shift 2 ;;
		--list-mods)  LIST_MODS=yes; shift ;;
		-r|--replay)  [ $# -ge 2 ] || die "$1 needs a replay file"
		              REPLAY="$2"; shift 2 ;;
		-m|--mission) [ $# -ge 2 ] || die "$1 needs a MapInfo record name"
		              MISSION="$2"; shift 2 ;;
		-c|--campaign) [ $# -ge 2 ] || die "$1 needs <campaign>[;<chapter>[;<mission>[;<difficulty>]]]"
		              CAMPAIGN="$2"; shift 2 ;;
		--replay-player) [ $# -ge 2 ] || die "$1 needs a player number"
		              case "$2" in ''|*[!0-9]*) die "$1 wants a player number, got '$2'" ;; esac
		              REPLAY_PLAYER="$2"; shift 2 ;;
		--verify)     VERIFY=yes; shift ;;
		--exit-after-load) EXIT_AFTER_LOAD=yes; shift ;;
		--profile)    # Optional argument, handled the way --hud handles its own:
		              # a bare --profile has to stay valid, so the next word is
		              # consumed only when it names a tool, and anything else that
		              # is not an option is a typo worth reporting here.
		              PROFILER=perforator
		              if [ $# -ge 2 ] && [ "${2#-}" = "$2" ]; then
		                  case "$2" in
		                      perforator|perf) PROFILER="$2" ;;
		                      *) die "--profile takes perforator or perf, got '$2'" ;;
		                  esac
		                  shift
		              fi
		              shift ;;
		--profile-out) [ $# -ge 2 ] || die "$1 needs a file name"
		              PROFILE_OUT="$2"; shift 2 ;;
		--setvar)     [ $# -ge 2 ] || die "$1 needs NAME=VALUE"
		              case "$2" in *=*) ;; *) die "--setvar wants NAME=VALUE, got '$2'" ;; esac
		              SETVARS+=("--setvar=$2"); shift 2 ;;
		--software-vk) SOFTWARE_VK=yes; shift ;;
		--hud)        # The overlay is optional, so a bare --hud has to stay
		              # valid and the next word is only consumed when it names
		              # one. Anything else that is not an option is a typo
		              # worth reporting here, rather than leaving it to be
		              # rejected as an unknown option a turn later.
		              HUD=dxvk
		              if [ $# -ge 2 ] && [ "${2#-}" = "$2" ]; then
		                  case "$2" in
		                      dxvk|mango) HUD="$2" ;;
		                      *)          die "--hud takes dxvk or mango, got '$2'" ;;
		                  esac
		                  shift
		              fi
		              shift ;;
		-e|--editor)  EXE=B2_MapEditor; shift ;;
		-g|--gdb)     USE_GDB=yes; shift ;;
		--valgrind)   USE_VALGRIND=yes; shift ;;
		--env)        SHOW_ENV=yes; shift ;;
		--)           shift; break ;;
		*)            die "unknown option '$1'. ./launch.sh --help" ;;
	esac
done

[ "$VERIFY" = yes ] && [ -z "$REPLAY" ] && die "--verify needs --replay FILE"
[ -n "$REPLAY_PLAYER" ] && [ -z "$REPLAY" ] && die "--replay-player needs --replay FILE"
[ "$USE_GDB" = yes ] && [ "$USE_VALGRIND" = yes ] &&
	die "--gdb and --valgrind both want to be the outer process; pass one"
# Same reason, and in the profiler's case the point is to sample the game rather
# than a debugger or an emulator standing in front of it.
[ -n "$PROFILER" ] && [ "$USE_GDB" = yes ] &&
	die "--profile and --gdb both want to be the outer process; pass one"
[ -n "$PROFILER" ] && [ "$USE_VALGRIND" = yes ] &&
	die "--profile would sample valgrind's emulator, not the game; pass one"
[ -n "$PROFILE_OUT" ] && [ -z "$PROFILER" ] &&
	die "--profile-out needs --profile"

# The game takes the first of these it is given and ignores the rest, so say so
# here rather than leaving the caller to wonder which one ran.
starters=0
[ -n "$MISSION" ] && starters=$((starters + 1))
[ -n "$CAMPAIGN" ] && starters=$((starters + 1))
[ -n "$REPLAY" ] && starters=$((starters + 1))
[ "$starters" -gt 1 ] && die "--mission, --campaign and --replay are alternatives; pass one"
# Without one of them the game reaches the menus, loads no map and so never
# reaches the point this flag acts at. Refused rather than ignored, because the
# run would otherwise sit at the main menu looking like a profiling run that
# found nothing to profile.
[ "$EXIT_AFTER_LOAD" = yes ] && [ "$starters" -eq 0 ] &&
	die "--exit-after-load needs a map to load; pass --mission, --campaign or --replay"

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

# Mods live beside Data, in the directory the game builds from its base dir,
# which is the parent of the directory it runs in. Spelled MODs exactly:
# System/FilePath.h says DIR_MODS[] = "MODs", the game enumerates it with a
# plain directory walk rather than through the VFS, and nothing folds case for
# it off Windows.
MODSDIR="$(dirname "$RUNDIR")/MODs"

# A mod is a folder holding name.txt, which is what the game reads to decide
# whether a directory under MODs is a mod at all: GetAllMODs skips any whose
# name comes back empty.
list_mods()
{
	local dir found=no
	for dir in "$MODSDIR"/*; do
		[ -f "$dir/name.txt" ] || continue
		found=yes
		# name.txt is UTF-16, which is what the rest of the text data is, so it
		# needs converting before it can be printed beside the folder name.
		printf '  %-28s %s\n' "${dir##*/}" \
			"$(iconv -f UTF-16 -t UTF-8 "$dir/name.txt" 2>/dev/null | tr -d '\r\n' || true)"
	done
	[ "$found" = no ] && printf '  (none in %s)\n' "$MODSDIR"
	return 0
}

if [ "$LIST_MODS" = yes ]; then
	printf 'mods in %s:\n' "$MODSDIR"
	list_mods
	exit 0
fi

# Caught here rather than left to the game, which attaches a missing mod as if
# it were no mod at all and then starts unmodded without saying so. A run meant
# to measure a mod's map would have quietly measured the wrong thing.
if [ "$MOD_GIVEN" = yes ] && [ -n "$MOD" ] && [ ! -f "$MODSDIR/$MOD/name.txt" ]; then
	printf '%s: no mod %s in %s\nmods installed:\n' "$SELF" "$MOD" "$MODSDIR" >&2
	list_mods >&2
	exit 1
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
# A verifying run compares checksums and nobody listens to it, so it takes SDL's
# dummy audio backend. That is not only tidiness: SDL's PulseAudio backend
# deadlocks on exit when the device is idle, which a finished replay always is.
# ClosePhysicalAudioDevice sets device->shutdown and joins the audio thread,
# while the only thing that signals the Pulse mainloop a parked thread is
# waiting on sits in PULSEAUDIO_CloseDevice, which runs after that join. The
# process then never exits, so the run yields no exit status and no sanitizer
# summary, and reads as a hung instrument. Unfixed in SDL 3.4.18 and on main.
# Set SDL_AUDIO_DRIVER yourself to hear a verifying run anyway.
#
# --exit-after-load is in the same position: nobody is listening, and it exits
# the moment the map is in, which is the idle device that deadlock needs.
if [ "$VERIFY" = yes ] || [ "$EXIT_AFTER_LOAD" = yes ]; then
	ENV_ARGS+=( "SDL_AUDIO_DRIVER=${SDL_AUDIO_DRIVER:-dummy}" )
elif [ -n "${SDL_AUDIO_DRIVER:-}" ]; then
	ENV_ARGS+=( "SDL_AUDIO_DRIVER=$SDL_AUDIO_DRIVER" )
fi
if [ "$SOFTWARE_VK" = yes ]; then
	[ -e "$LVP_ICD" ] || die "no lavapipe ICD at $LVP_ICD; install mesa-vulkan-drivers"
	# Both names: the loader took VK_ICD_FILENAMES before it took VK_DRIVER_FILES,
	# and which one is honoured depends on the loader version rather than ours.
	ENV_ARGS+=(
		"VK_DRIVER_FILES=$LVP_ICD"
		"VK_ICD_FILENAMES=$LVP_ICD"
	)
fi

# The frame rate overlay. DXVK's own HUD needs nothing installed, because the
# game renders through DXVK whatever this flag says, and DXVK_HUD is a comma
# separated list of which figures to draw: fps alone by default here, where
# full draws every one of them.
#
# MangoHud is an implicit Vulkan layer and MANGOHUD=1 is what enables it, so
# there is nothing to preload and nothing to wrap; the mangohud script on PATH
# is for OpenGL programs, which this is not. Its own configuration is left
# alone, since MANGOHUD_CONFIG reaches the layer through the environment
# already and setting a value here would override the user's MangoHud.conf on
# every run.
case "$HUD" in
	dxvk)
		ENV_ARGS+=( "DXVK_HUD=${DXVK_HUD:-fps}" ) ;;
	mango)
		# MANGOHUD=1 only enables a layer that is installed. With no manifest
		# to enable it is a no-op, and the run would come up with no overlay
		# and nothing said about why, so the manifest is what is tested for.
		mangohud_layer=
		for dir in "${MANGOHUD_LAYER_DIRS[@]}"; do
			for manifest in "$dir"/MangoHud*.json; do
				[ -e "$manifest" ] && { mangohud_layer="$manifest"; break 2; }
			done
		done
		[ -n "$mangohud_layer" ] ||
			die "no MangoHud layer manifest in ${MANGOHUD_LAYER_DIRS[*]}; install mangohud, or use --hud dxvk"
		# The layer runs threads of its own and wraps the swapchain, so it
		# reports as races under TSan that tsan.supp was not built to cover.
		case "$PREFIX" in
			*-tsan)
				printf '%s: warning: the MangoHud layer races with itself as far as TSan is concerned, and tsan.supp does not cover it\n' "$SELF" >&2 ;;
		esac
		ENV_ARGS+=( "MANGOHUD=1" ) ;;
esac

case "$PREFIX" in
	*-asan)
		ENV_ARGS+=(
			"ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0:abort_on_error=1}"
			"UBSAN_OPTIONS=${UBSAN_OPTIONS:-print_stacktrace=1}"
		) ;;
	*-tsan)
		# DXVK, the graphics driver and SDL3 are prebuilt, so TSan reports their
		# internal synchronisation as races; tsan.supp is what drops those and
		# only those. Named conditionally because TSan errors out if a
		# suppressions file is named and missing, and an explicit TSAN_OPTIONS
		# wins outright, so pass suppressions= yourself if you set it.
		#
		# Mind the exit status of a verifying run here: TSan replaces it with
		# 66 whenever it reported anything, whatever the program returned, so
		# under this prefix the status no longer tells a diverged replay from a
		# race that was found. Read the "checksums matched" line, or add
		# exitcode=0 to let the program's own status through and lose the race
		# signal instead.
		TSAN_SUPP=""
		[ -f "$SRCDIR/tsan.supp" ] && TSAN_SUPP="suppressions=$SRCDIR/tsan.supp:"
		ENV_ARGS+=( "TSAN_OPTIONS=${TSAN_OPTIONS:-${TSAN_SUPP}history_size=4}" ) ;;
	*-tysan)
		# TySan prints a report and carries on, which is what a survey wants:
		# halt_on_error=1 gives one site per run of a game that takes a minute
		# to reach a mission. The exit status stays the program's own, so this
		# prefix is the one sanitizer build whose status still means what it
		# says.
		#
		# The symbolizer has to be named. The runtime looks for a plain
		# "llvm-symbolizer" and Debian ships versioned names only, so an
		# unconfigured run prints bare addresses and the warning "invalid path
		# to external symbolizer". Highest version wins, and sort -V is what
		# orders llvm-symbolizer-9 below llvm-symbolizer-21.
		TYSAN_SYM=""
		if ! command -v llvm-symbolizer >/dev/null; then
			TYSAN_SYM="$(compgen -c llvm-symbolizer- 2>/dev/null | sort -V -t- -k3 | tail -1)"
			[ -n "$TYSAN_SYM" ] &&
				TYSAN_SYM="external_symbolizer_path=$(command -v "$TYSAN_SYM"):"
		fi
		ENV_ARGS+=( "TYSAN_OPTIONS=${TYSAN_OPTIONS:-${TYSAN_SYM}print_stacktrace=1}" ) ;;
esac

# Valgrind's memcheck answers for every type the question UBSan's bool and enum
# checks answer for two: was this value ever written? It tracks definedness per
# bit as the program runs, so it needs no instrumented build and no rebuild of
# the dependencies the way MemorySanitizer would.
#
# --track-origins is the point of using it here. Without it a report says an
# uninitialised value was used; with it, it also says where that value came
# from, which for a member nobody assigned is the allocation. It costs about
# twice as much again on top of memcheck's own 20x or so, and this is already a
# tool for one question rather than for playing.
#
# Leak checking is off by default for the reason the ASan prefix sets
# detect_leaks=0: the object model frees on refcount and a clean exit is not one
# of its guarantees, so the shutdown report buries everything found while
# playing. Pass VALGRIND_EXTRA='--leak-check=full' when leaks are the question.
#
# Suppressions come from valgrind.supp beside this script when it exists, the
# way tsan.supp does. Build one for the prebuilt graphics stack with
# VALGRIND_EXTRA='--gen-suppressions=all'.
declare -a VALGRIND_CMD=()
if [ "$USE_VALGRIND" = yes ]; then
	command -v valgrind >/dev/null || die "no valgrind on PATH"
	case "$PREFIX" in
		*-asan|*-tsan|*-tysan)
			die "valgrind cannot run a sanitizer build: each replaces the allocator and intercepts the same calls. Run release or debug instead" ;;
	esac
	# mimalloc is linked into every executable by default and memcheck cannot
	# see through it: the pools it mmaps arrive zero filled, so an unwritten
	# member reads as a defined zero and no heap error or leak is ever
	# reported. What survives is everything off the heap, which is worth a run
	# on its own, so this says what is lost rather than refusing.
	if command -v readelf >/dev/null &&
	   readelf -d "$PREFIX/bin/$EXE" 2>/dev/null | grep -q mimalloc; then
		printf '%s: warning: %s links mimalloc, so memcheck will report nothing about the heap\n' "$SELF" "$EXE" >&2
		printf '%s:          and no leaks. Configure that prefix with -DUSE_MIMALLOC=OFF for those.\n' "$SELF" >&2
	fi
	declare -a VALGRIND_SUPP=()
	[ -f "$SRCDIR/valgrind.supp" ] && VALGRIND_SUPP=( "--suppressions=$SRCDIR/valgrind.supp" )
	if [ -n "${VALGRIND_OPTS:-}" ]; then
		read -r -a VALGRIND_CMD <<<"valgrind $VALGRIND_OPTS"
	else
		VALGRIND_CMD=(
			valgrind
			--tool=memcheck
			--track-origins=yes
			--leak-check=no
			--num-callers=30
			--error-limit=no
			# The game is threaded and valgrind serialises threads by default,
			# which on a frame loop reads as a hang rather than as slowness.
			--fair-sched=yes
			"--log-file=$RUNDIR/valgrind-%p.log"
			"${VALGRIND_SUPP[@]}"
		)
	fi
	if [ -n "${VALGRIND_EXTRA:-}" ]; then
		declare -a valgrind_extra=()
		read -r -a valgrind_extra <<<"$VALGRIND_EXTRA"
		VALGRIND_CMD+=( "${valgrind_extra[@]}" )
	fi
	printf '%s: memcheck output goes to %s/valgrind-<pid>.log\n' "$SELF" "$RUNDIR" >&2
fi

# The profiler, as the outermost process.
#
# Both tools here launch the game themselves rather than attaching to it, which
# is what makes this worth wiring in at all: a profiler that attaches by process
# id cannot win against work that starts moments after the process does, and a
# map load is exactly that. perforator traces the child it started and stops when
# that child exits, so the recording begins and ends with the run.
#
# Neither records anything useful without privileges, so both are checked for
# here rather than failing forty lines of game output later.
declare -a PROFILER_CMD=()
declare -a PRIV_CMD=()
if [ -n "$PROFILER" ]; then
	# The default output name carries the time, so a second run does not silently
	# overwrite the first one's profile.
	[ -z "$PROFILE_OUT" ] && PROFILE_OUT="$SRCDIR/profile-$(date +%Y%m%d-%H%M%S)"
	# A base name, with the extension supplied per tool. Stripping a known one
	# the caller may have typed means --profile-out /tmp/mi.html does not become
	# /tmp/mi.html.pprof, and naming either member of the pair names both.
	PROFILE_BASE="${PROFILE_OUT%.html}"
	PROFILE_BASE="${PROFILE_BASE%.pprof}"
	PROFILE_BASE="${PROFILE_BASE%.data}"
	case "$PROFILER" in
		perforator) PROFILE_PRIMARY="$PROFILE_BASE.pprof" ;;
		perf)       PROFILE_PRIMARY="$PROFILE_BASE.data" ;;
	esac

	case "$PROFILER" in
		perforator)
			# BK2_PERFORATOR first, then PATH. The checkout builds the binary
			# through ya into ~/.ya/build/symres and leaves a symlink to it, and
			# that target is removed when ya garbage collects, so a dangling link
			# is the normal way for this to be missing. -e follows the link and
			# so reports exactly that.
			PERFORATOR="${BK2_PERFORATOR:-}"
			[ -z "$PERFORATOR" ] && PERFORATOR="$(command -v perforator 2>/dev/null)"
			[ -n "$PERFORATOR" ] ||
				die "no perforator on PATH; set BK2_PERFORATOR to the binary"
			[ -e "$PERFORATOR" ] ||
				die "$PERFORATOR is not there (a dangling symlink means ya collected the build; rebuild it)"
			[ -x "$PERFORATOR" ] || die "$PERFORATOR is not executable"

			# eBPF, so root. SUDO is overridable because a binary given the
			# capabilities outright needs none of this.
			SUDO="${BK2_PROFILE_SUDO-sudo}"
			[ "$(id -u)" = 0 ] && SUDO=
			if [ -n "$SUDO" ]; then
				command -v "$SUDO" >/dev/null || die "no $SUDO on PATH, and perforator needs root"
			fi

			# BK2_PERFORATOR_EXTRA adds flags, the way VALGRIND_EXTRA does:
			# --freq, --format pprof for a pprof file instead of a flame graph,
			# --log-level debug when a recording comes back empty.
			declare -a perforator_extra=()
			[ -n "${BK2_PERFORATOR_EXTRA:-}" ] &&
				read -r -a perforator_extra <<<"$BK2_PERFORATOR_EXTRA"
			# pprof and not html, even though the flame graph is what gets
			# looked at. --format takes one value, and pprof is the one that
			# keeps the data: the flame graph renders from it afterwards with
			# perforator profile flamegraph, while nothing turns an HTML
			# flame graph back into a profile. So a run leaves both, and the
			# pprof is what go tool pprof -top and a diff against a second
			# recording read.
			PROFILER_CMD=( $SUDO "$PERFORATOR" record
				--format pprof
				-o "$PROFILE_PRIMARY"
				"${perforator_extra[@]}" -- )

			# Under sudo the game would inherit root, and it writes into the data
			# tree: Profiles, log.txt, the crashpad database. One run as root and
			# the next as the user then fails on files it cannot open. setpriv
			# execs rather than forking, so the process perforator registered is
			# still the one that becomes the game.
			if [ -n "$SUDO" ]; then
				command -v setpriv >/dev/null ||
					die "no setpriv on PATH; without it the game would run as root and leave root-owned files in $RUNDIR"
				PRIV_CMD=( setpriv "--reuid=$(id -u)" "--regid=$(id -g)" --init-groups )
			fi
			;;
		perf)
			command -v perf >/dev/null || die "no perf on PATH"
			# Reported rather than raised, since raising it needs root and this
			# is the one check whose answer the user may already have arranged.
			paranoid="$(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo unknown)"
			case "$paranoid" in
				-1|0|1) ;;
				*) printf '%s: warning: kernel.perf_event_paranoid is %s, which refuses perf a sampling event.\n' \
						"$SELF" "$paranoid" >&2
				   printf '%s:          sudo sysctl kernel.perf_event_paranoid=1\n' "$SELF" >&2 ;;
			esac
			# dwarf unwinding, not frame pointers: Release maps to RelWithDebInfo
			# and carries -g, but not -fno-omit-frame-pointer, so a frame pointer
			# walk gives stacks that stop inside the engine's shared libraries.
			# 199 Hz because dwarf copies a stack snapshot per sample and a higher
			# rate on a process this size fills the buffer faster than it drains.
			PROFILER_CMD=( perf record -g --call-graph=dwarf -F 199 -o "$PROFILE_PRIMARY" -- )
			;;
	esac

	# sudo resets the environment, and the game needs a display to open a window
	# on. These are not in ENV_ARGS because an ordinary run inherits them.
	for passthrough in DISPLAY XAUTHORITY WAYLAND_DISPLAY XDG_RUNTIME_DIR HOME; do
		[ -n "${!passthrough:-}" ] && ENV_ARGS+=( "$passthrough=${!passthrough}" )
	done

	printf '%s: %s profile goes to %s\n' "$SELF" "$PROFILER" "$PROFILE_PRIMARY" >&2
fi

if [ "$SHOW_ENV" = yes ]; then
	printf '%s\n' "${ENV_ARGS[@]}"
	printf 'rundir=%s\nexe=%s\n' "$RUNDIR" "$EXE"
	[ ${#VALGRIND_CMD[@]} -gt 0 ] && printf 'valgrind=%s\n' "${VALGRIND_CMD[*]}"
	[ ${#PROFILER_CMD[@]} -gt 0 ] && printf 'profiler=%s %s\n' "${PROFILER_CMD[*]}" "${PRIV_CMD[*]}"
	[ "$MOD_GIVEN" = yes ] && printf 'args=--mod=%s\n' "$MOD"
	[ -n "$MISSION" ] && printf 'args=--mission=%s\n' "$MISSION"
	[ -n "$CAMPAIGN" ] && printf 'args=--campaign=%s\n' "$CAMPAIGN"
	[ -n "$REPLAY" ] && printf 'args=--replay=%s\n' "$REPLAY"
	[ -n "$REPLAY_PLAYER" ] && printf 'args=--replay-player=%s\n' "$REPLAY_PLAYER"
	[ "$VERIFY" = yes ] && printf 'args=--replay-verify\n'
	[ "$EXIT_AFTER_LOAD" = yes ] && printf 'args=--exit-after-load\n'
	[ ${#SETVARS[@]} -gt 0 ] && printf 'args=%s\n' "${SETVARS[@]}"
	exit 0
fi

# The profiler goes outside everything, including the env that sets the run up,
# because it is the process that launches all of it and the one that has to
# survive the game exiting in order to write its output. PRIV_CMD sits between
# the two: it drops root before env, and execs, so the pid does not change.
declare -a CMD=("${PROFILER_CMD[@]}" "${PRIV_CMD[@]}" env "${ENV_ARGS[@]}")
[ "$USE_GDB" = yes ] && CMD+=(gdb --args)
[ ${#VALGRIND_CMD[@]} -gt 0 ] && CMD+=("${VALGRIND_CMD[@]}")
CMD+=("./$EXE")
# Before the caller's own arguments, so an explicit --replay= after -- wins.
# --mod among them, and an empty one still passed: --mod "" is how a run says
# "ignore what the profile remembers and start unmodded".
[ "$MOD_GIVEN" = yes ] && CMD+=("--mod=$MOD")
[ -n "$MISSION" ] && CMD+=("--mission=$MISSION")
[ -n "$CAMPAIGN" ] && CMD+=("--campaign=$CAMPAIGN")
[ -n "$REPLAY" ] && CMD+=("--replay=$REPLAY")
[ -n "$REPLAY_PLAYER" ] && CMD+=("--replay-player=$REPLAY_PLAYER")
[ "$VERIFY" = yes ] && CMD+=("--replay-verify")
[ "$EXIT_AFTER_LOAD" = yes ] && CMD+=("--exit-after-load")
[ ${#SETVARS[@]} -gt 0 ] && CMD+=("${SETVARS[@]}")
CMD+=("$@")

# exec from the run directory: everything the game opens is resolved relative to
# it, which is the point of the cd, and exec makes the exit status the game's.
printf '+ cd %s && %s\n' "$RUNDIR" "${CMD[*]}" >&2
cd "$RUNDIR" || die "cannot enter $RUNDIR"

# A profiling run cannot exec, because there is work left after the profiler
# returns: it wrote its output as root, and a file the user cannot open is not a
# profile they can read.
if [ -n "$PROFILER" ]; then
	"${CMD[@]}"
	status=$?
	if [ ! -e "$PROFILE_PRIMARY" ]; then
		printf '%s: warning: %s produced no %s\n' "$SELF" "$PROFILER" "$PROFILE_PRIMARY" >&2
		exit "$status"
	fi
	# Recorded as root, so it belongs to root; a profile the user cannot open is
	# not one they can read. Done before the render below, which runs as the user.
	[ -w "$PROFILE_PRIMARY" ] ||
		${BK2_PROFILE_SUDO-sudo} chown "$(id -u):$(id -g)" "$PROFILE_PRIMARY"
	printf '%s: profile written to %s (%s)\n' \
		"$SELF" "$PROFILE_PRIMARY" "$(du -h "$PROFILE_PRIMARY" | cut -f1)" >&2

	# The flame graph, rendered from the pprof rather than recorded separately:
	# --format takes one value, so recording both would mean running the game
	# twice and profiling two different runs. Rendering needs no privileges, and
	# the renderer writes the document to stdout and its diagnostics to stderr,
	# so the redirect picks up the HTML and nothing else.
	if [ "$PROFILER" = perforator ]; then
		if "$PERFORATOR" profile flamegraph pprof -i "$PROFILE_PRIMARY" -f html \
		   > "$PROFILE_BASE.html" 2>/dev/null && [ -s "$PROFILE_BASE.html" ]; then
			printf '%s: flame graph written to %s (%s)\n' \
				"$SELF" "$PROFILE_BASE.html" "$(du -h "$PROFILE_BASE.html" | cut -f1)" >&2
		else
			rm -f "$PROFILE_BASE.html"
			printf '%s: warning: could not render a flame graph; the pprof is still there.\n' "$SELF" >&2
			printf '%s:          %s profile flamegraph pprof -i %s -f html > out.html\n' \
				"$SELF" "$PERFORATOR" "$PROFILE_PRIMARY" >&2
		fi
		printf '%s: go tool pprof -top %s\n' "$SELF" "$PROFILE_PRIMARY" >&2
	else
		printf '%s: perf report -i %s --stdio --sort=dso,symbol\n' "$SELF" "$PROFILE_PRIMARY" >&2
	fi
	exit "$status"
fi

exec "${CMD[@]}"
