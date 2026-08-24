# Routes every compile through ccache, which keeps a cache of object files keyed
# on the preprocessed source and the command line. A rebuild of something already
# compiled once - after switching branches, reconfiguring a second preset, or
# rebasing back over a commit - then costs a file copy instead of a compile.
#
# This is on top of the precompiled headers wired in pch.cmake, not instead of
# them. The two overlap but do not replace each other: the PCH makes a *first*
# compile cheap, the cache makes a *repeat* compile nearly free.
#
# Three things had to be true for this to work here, and all three were measured
# on this tree rather than assumed.
#
# 1. `sloppiness=time_macros` is mandatory. Without it ccache reports every
#    compilation that uses a precompiled header as uncacheable - 0 of 2 cacheable
#    on both toolchains - and silently does nothing but add overhead. `time_macros`
#    tells ccache not to worry about __DATE__ and __TIME__, which it cannot verify
#    through a PCH. That is safe here because nothing in the tree uses either macro;
#    the build timestamp arrives as BUILD_DATE_TIME_STR on the command line, which
#    ccache hashes normally. That is an invariant this setting depends on: a
#    source that started using either macro would silently get a stale cached
#    value. `pch_defines` is the other half of the pair ccache's documentation
#    recommends for precompiled headers; measured on its own it changed nothing,
#    and it is kept because it costs nothing and covers PCH content this
#    measurement did not exercise.
#
# 2. MSVC needs /Z7 rather than /Zi, because a compiler cache cannot reproduce the
#    side effects of writing to a shared .pdb. The top-level CMakeLists already
#    forces that for its own reasons; see the comment there.
#
# 3. It has to be ccache specifically. sccache, which was already installed on the
#    Windows machine this was measured on, refuses outright: it reports /Fp as a
#    non-cacheable option, so with the PCH in place it caches nothing at all. Using
#    it would mean giving up the PCH to get the cache, which measured as a wash.
#    ccache 4.14 caches /Yu compilations fine.
#
# What it bought. One translation unit, warm cache versus a plain compile:
#
#   MSVC   libdb/BindProcessor.cpp   467 ms -> 155 ms
#   GCC    libdb/Variant.cpp        3930 ms ->   8 ms
#
# And a whole x64 Release build, objects deleted but the cache kept, which is what
# a branch switch or a second preset looks like:
#
#   cold cache  681 s     990 of 990 misses
#   warm cache  194 s     990 of 991 hits, 99.9%
#
# Most of that remaining 194 s is linking 365 targets, which a compiler cache does
# not touch, so this is close to the floor for caching alone.
#
# The environment variable is passed through `cmake -E env` rather than a generated
# wrapper script: it is a launcher CMake already ships, it quotes correctly on both
# platforms, and it keeps the setting scoped to this build instead of writing to the
# user's global ccache.conf.
#
# Turn it off with -DUSE_CCACHE=OFF. It also turns itself off, with a message,
# when ccache is not installed, so a machine without it still configures.

option(USE_CCACHE "Route compiles through ccache, if it is installed" ON)

if(USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        # See point 1 above: without this the cache is inert wherever a PCH is used,
        # which here is every target but Scintilla.
        set(CCACHE_LAUNCHER
            "${CMAKE_COMMAND}" -E env "CCACHE_SLOPPINESS=pch_defines,time_macros"
            "${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_LAUNCHER}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_LAUNCHER}")
        message(STATUS "ccache: ${CCACHE_PROGRAM}")
    else()
        message(STATUS "ccache: not found, compiling without a cache")
    endif()
endif()
