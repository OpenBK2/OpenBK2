# SDL3 is built from a pinned revision on every platform rather than taken from
# the distribution off Windows.
#
# Two reasons, the narrower one first. DXVK Native reaches SDL3 through
# dlopen("libSDL3.so.0") and dlsym rather than by linking it: libdxvk_d3d9.so has
# only libm, libc and the loader in its NEEDED entries. The loader hands it
# whichever library with that soname is already mapped, which is this build's, so
# there are never two instances in one process. What is left is that DXVK
# resolves every entry point by name against a library it was not compiled
# against, and that only cuts one way: SDL 3 keeps ABI compatibility going
# forward, so a revision at least as new as the one DXVK was built against
# resolves everything, while an older one can hand back a null dlsym. Keeping the
# pin ahead of the SDL3 the DXVK in use was compiled against settles it.
#
# The broader reason is that choosing a dependency by what the host happens to
# have installed makes the build's behaviour a property of the machine. ABI
# compatibility is not behavioural compatibility, and this engine is a
# deterministic simulation that has to stay bit-identical across machines, so a
# host-dependent dependency graph is the wrong default even where linking would
# succeed. Distributions also disagree with each other by more than is
# comfortable: Debian trixie carries SDL 3.2.10, Ubuntu 26.04 carries 3.4.2, and
# only one of those is new enough for the SDL_mixer pinned in
# cmake/sdl_mixer.cmake. Same source, two machines, two different libraries, and
# nothing in the build says so.
#
# That pin and this one are coupled. SDL_mixer 3.4.0 asks for SDL 3.4.0 and means
# it: SDL_ALIGNED, SDL_PutAudioStreamPlanarData, SDL_PutAudioStreamDataNoCopy and
# SDL_PROP_AUDIOSTREAM_AUTO_CLEANUP_BOOLEAN are all additions since 3.2. The
# revision here reports itself as 3.3.0 and carries all four, being a snapshot of
# the development branch 3.4.0 was released from, so the version numbers do not
# line up while the capabilities do. Bumping either pin means checking the other.
# With both pinned that check is a compile error on every machine at once, rather
# than one that depends on what is installed; SDL_mixer's own version guard does
# not help, since it only runs when no SDL3 target exists yet and this file has
# always made one by then.
#
# SDL_mixer is FetchContent on both platforms for the simpler reason that no
# libsdl3-mixer package exists yet.
include(FetchContent)

FetchContent_Declare(
        SDL
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG f3815ede24fa6e4b759c8e9beb02334003649642
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(SDL)

FetchContent_GetProperties(SDL SOURCE_DIR SDL_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(sdl_targets ${SDL_SOURCE_DIR})
foreach(target IN LISTS sdl_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/SDL")
endforeach()
