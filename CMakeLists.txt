# 3.23 is what CMakePresets.json already asks for, and presets are how both
# the documented build and CI configure this, so nothing older has worked
# here in a long time. It also covers TARGET_RUNTIME_DLLS, added in 3.21.
CMAKE_MINIMUM_REQUIRED(VERSION 3.23)

# CMP0168 (CMake 3.30): FetchContent populates a dependency directly instead of
# generating and building a sub-project for it. Under the old behaviour every
# reconfigure ran one nested CMake configure and build per dependency just to
# confirm the pinned download was still in place, ~0.5-0.8 s each, about 10 s
# across this tree's FetchContent dependencies.
#
# Set on its own rather than by raising the minimum to 3.30, which would switch
# on every other policy up to 3.30 as well, CMP0141 among them: that one moves
# MSVC's debug-information flag out of CMAKE_<LANG>_FLAGS_<CONFIG>, which is
# where the /Zi -> /Z7 rewrite below looks for it. Guarded so an older CMake
# keeps configuring, the old way.
if(POLICY CMP0168)
    cmake_policy(SET CMP0168 NEW)
endif()

project(b2 LANGUAGES C CXX)

# ASM_MASM is for the MMX reference implementations under 3Dmotor/test/original/.
# MSVC accepts __asm on x86 only, so keeping those references as inline assembly
# made the bit-exactness tests unbuildable on x64.
#
# ml and ml64 ship with MSVC and nothing else assembles MASM syntax, so naming the
# language in project() failed configure outright on every other toolchain. Enable it
# only where it exists; HAVE_MASM then gates the references themselves and the three
# targets that link them. Section 5 of docs/vertex-skinning-findings.md has the plan
# to convert those references to SSE2 intrinsics, which would retire this entirely.
if(MSVC)
  enable_language(ASM_MASM)
  set(HAVE_MASM ON)
else()
  set(HAVE_MASM OFF)
  message(STATUS "MASM unavailable: the MMX reference tests and benchmark are skipped")
endif()

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The editor is drawn with wxWidgets, which it cannot be built without; see
# cmake/wxwidgets.cmake.
option(BUILD_EDITOR "build editor" ON)
# The dedicated lobby server and the test client that drives it. On, like the
# editor: both build and run on Windows and Linux now, so there is no reason
# for a normal build to skip them and let them rot again, which is how they got
# into the state they were in.
#
# Turning this on means every build fetches the two database backends, MariaDB
# Connector/C and SQLite, since the server links both. Configure with
# -DBUILD_SERVER=OFF to skip them.
option(BUILD_SERVER "build dedicated server" ON)
option(USE_MIMALLOC "use mimalloc for project C++ allocations (process override on non-Windows)" ON)
option(ENABLE_ASAN "build with AddressSanitizer" OFF)
# One option per sanitizer rather than one string naming them, because each has
# its own interaction with the allocator and with the others, and a single
# string invites the combinations that build and then report nothing.
option(ENABLE_TSAN "build with ThreadSanitizer" OFF)
option(ENABLE_UBSAN "build with UndefinedBehaviorSanitizer" OFF)
# TypeSanitizer is the one that answers a question about the language rather
# than about memory or threads: it records the type each byte was last written
# with and reports a read through an incompatible lvalue, which is the strict
# aliasing class that silently miscompiled the DXT decoder here and that no
# warning on either compiler reports at any level. clang only; see
# docs/Sanitizers.md.
option(ENABLE_TYSAN "build with TypeSanitizer (clang only)" OFF)

# ASan and TSan both map shadow memory over the whole address space, and they
# want the same regions, so a binary carrying both aborts as it starts. Said
# here rather than discovered then.
if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ENABLE_ASAN and ENABLE_TSAN cannot both be on: the two runtimes claim the same shadow memory. Configure a separate build directory for each")
endif()

# TySan has shadow memory of its own and clang's driver refuses to pair it with
# either of those two outright ("invalid argument '-fsanitize=type' not allowed
# with '-fsanitize=address'"), so the combination cannot be built at all. Said
# at configure time rather than once per translation unit. UBSan composes with
# it, measured, the same way it composes with ASan.
if(ENABLE_TYSAN AND (ENABLE_ASAN OR ENABLE_TSAN))
    message(FATAL_ERROR "ENABLE_TYSAN cannot be combined with ENABLE_ASAN or ENABLE_TSAN: clang rejects the flag pair. Configure a separate build directory for each")
endif()

# MSVC implements only /fsanitize=address.
if(MSVC AND (ENABLE_TSAN OR ENABLE_UBSAN))
    message(FATAL_ERROR "ENABLE_TSAN and ENABLE_UBSAN need GCC or Clang: MSVC has no thread or undefined-behaviour sanitizer")
endif()

# TySan exists in clang only. GCC answers "unrecognized argument to
# '-fsanitize=' option: 'type'", which would otherwise be the first line of
# 1400 identical failures.
if(ENABLE_TYSAN AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "ENABLE_TYSAN needs clang: GCC and MSVC have no type sanitizer. Configure with the Linux-x64-TySan preset, which selects the compiler as well as the flag")
endif()

# AddressSanitizer installs its own allocator to see every allocation and
# free, so it cannot share a process with mimalloc, which replaces the same
# thing. Turning one on turns the other off rather than leaving a combination
# that builds and then reports nothing useful. TSan intercepts the allocator
# too, to know which thread owns a block; UBSan does not and leaves mimalloc
# alone.
#
# TySan is in the list for a different reason: it has no allocator of its own,
# but it interposes malloc, calloc, realloc and free to *forget* the type of a
# block as it is freed and reused. Let mimalloc take those calls and the
# runtime keeps the dead object's type, so the next allocation to land on the
# same bytes is reported as a violation. That failure mode is false reports
# rather than missing ones, which is the worse of the two for a survey.
if((ENABLE_ASAN OR ENABLE_TSAN OR ENABLE_TYSAN) AND USE_MIMALLOC)
    message(STATUS "sanitizer build: mimalloc switched off, the sanitizer runtime replaces the allocator")
    set(USE_MIMALLOC OFF CACHE BOOL "use mimalloc for project C++ allocations (process override on non-Windows)" FORCE)
endif()

# -g because a sanitizer report without symbols names an address and nothing
# else, and no-omit-frame-pointer because the stack it prints is walked rather
# than read from debug info.
#
# ENABLE_TYSAN is deliberately absent from this block: its flag is added in
# Versions/Temporary/Engine/Sources/CMakeLists.txt instead, scoped to our own
# code, because instrumenting a dependency with it does not link. The reasoning
# is written out there.
if(ENABLE_ASAN OR ENABLE_TSAN OR ENABLE_UBSAN)
    if(MSVC)
        add_compile_options(/fsanitize=address /Zi)
    else()
        set(OBK2_SANITIZERS "")
        if(ENABLE_ASAN)
            list(APPEND OBK2_SANITIZERS address)
        endif()
        if(ENABLE_TSAN)
            list(APPEND OBK2_SANITIZERS thread)
        endif()
        if(ENABLE_UBSAN)
            list(APPEND OBK2_SANITIZERS undefined)
            # Neither GCC nor clang defines a macro for this one, the way they
            # do __SANITIZE_ADDRESS__ and __SANITIZE_THREAD__, so code that
            # reports which configuration it is has no other way to know.
            # System/BuildStamp.cpp is the consumer, for the replay stamp.
            add_compile_definitions(ENABLE_UBSAN)
        endif()
        list(JOIN OBK2_SANITIZERS "," OBK2_SANITIZE_ARG)
        # The flag goes on the link line as well as the compile line: it is
        # what pulls in the runtime, and every engine module is a shared
        # library that has to carry it.
        add_compile_options(-fsanitize=${OBK2_SANITIZE_ARG} -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=${OBK2_SANITIZE_ARG})
    endif()
endif()

# Every engine module is a shared library and they are installed beside the
# executable, which is where Windows looks by default and ELF does not. Recording
# $ORIGIN means the installed game finds its own libraries without the caller
# having to set LD_LIBRARY_PATH first.
#
# Appended rather than assigned, and set before anything else touches it, because
# cmake/dxvk.cmake adds the directory DXVK was found in and both entries have to
# survive.
if(NOT WIN32)
    list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

include(cmake/arch.cmake)

# std::thread and std::condition_variable need an explicit threading
# library everywhere but Windows
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(MSVC)
  add_compile_options("/nologo")
  add_link_options("/nologo")
  # The Release presets use RelWithDebInfo, whose MSVC defaults enable
  # incremental linking. Repeated links accumulate PDB state until large modules
  # hit LNK1140. Use full links for this configuration while keeping /debug;
  # Debug retains incremental linking for its faster edit-build cycle.
  add_link_options("$<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>")
  # Scoped to the compilers that understand them: ml/ml64 rejects each of these
  # with warning A4018 now that ASM_MASM is an enabled language.
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/wd4251>")
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/wd4275>")
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/FS>")
  # /Zi routes debug info through mspdbsrv.exe, which every parallel cl.exe
  # then serializes on (and which is why /FS above is needed). /Z7 puts the
  # debug info in the .obj instead, so Ninja can actually keep all cores busy.
  # It also makes objects cacheable, which /Zi output is not, should a compiler
  # cache ever be wired up. The cost is fatter .obj files and more link work.
  #
  # All configurations, not just Debug: the Release presets map to
  # RelWithDebInfo, whose default flags carry /Zi, so that is the build that
  # was paying for it. Release and MinSizeRel have no /Zi to replace today,
  # and are listed so they stay correct if debug info is ever added to them.
  foreach(flag_var
          CMAKE_CXX_FLAGS_DEBUG
          CMAKE_CXX_FLAGS_RELEASE
          CMAKE_CXX_FLAGS_RELWITHDEBINFO
          CMAKE_CXX_FLAGS_MINSIZEREL
          CMAKE_C_FLAGS_DEBUG
          CMAKE_C_FLAGS_RELEASE
          CMAKE_C_FLAGS_RELWITHDEBINFO
          CMAKE_C_FLAGS_MINSIZEREL)
    string(REPLACE "/Zi" "/Z7" ${flag_var} "${${flag_var}}")
  endforeach()
  add_definitions("-D_CRT_SECURE_NO_WARNINGS=1")
  add_definitions("-D_CRT_NONSTDC_NO_DEPRECATE=1")
  add_definitions("-D_CRT_NON_CONFORMING_SWPRINTFS=1")
elseif(UNIX)
  # Nothing here runs code off the stack, and one object in a fetched dependency
  # decides otherwise for every binary it reaches: crashpad's
  # capture_context_linux.S carries no .note.GNU-stack marker, so GNU ld assumes
  # the worst for the whole link and marks PT_GNU_STACK executable. Whether that
  # is only a warning or an executable stack depends on the linker's default:
  # Ubuntu's ld reports it on every CI link of Game, B2_MapEditor and the
  # Crashpad probe, Debian's is silent. Asking for it explicitly makes the
  # answer the same everywhere and is what a distribution's hardening flags
  # would do anyway.
  add_link_options("-Wl,-z,noexecstack")
endif()

include(cmake/winapi.cmake)

set(ZLIB_COMPAT ON)
include(cmake/zlib-ng.cmake)

set(SDLMIXER_VENDORED OFF)
include(cmake/sdl.cmake)
include(cmake/sdl_mixer.cmake)

# Before unittest.cmake and the subdirectories, all of which call into it.
include(cmake/runtimedlls.cmake)

include(cmake/googletest.cmake)
include(cmake/unittest.cmake)
include(cmake/benchmarks.cmake)
include(cmake/googlebenchmark.cmake)
include(cmake/boost.cmake)
# After boost.cmake, since it picks between Boost's stacktrace backend targets.
include(cmake/stacktrace.cmake)
if(USE_MIMALLOC)
    include(cmake/mimalloc.cmake)
endif()
include(cmake/glm.cmake)
include(cmake/fastgltf.cmake)
if(WIN32)
include(cmake/dxsdk.cmake)
else()
include(cmake/dxvk.cmake)
endif()
include(cmake/fmod.cmake)
include(cmake/fmt.cmake)
include(cmake/spdlog.cmake)
# After spdlog: libgr2 carries its call trace on it and reuses this tree's copy
# rather than fetching a second one.
# symbols.cmake first: granny.cmake and bk2_targets.cmake install PDBs with it.
include(cmake/symbols.cmake)
include(cmake/granny.cmake)
include(cmake/ffmpeg.cmake)
include(cmake/crashpad.cmake)
include(cmake/versioninfo.cmake)
include(cmake/exportheader.cmake)
# After exportheader.cmake: squish generates its export header into the same
# build-tree directory and relies on that directory being on the include path.
include(cmake/squish.cmake)
include(cmake/freetype.cmake)
# game data rather than code: the open fonts as a patch archive for Data
include(cmake/fontpak.cmake)
# also data: the custom-mission maps that are in Complete and not in the data tree
include(cmake/testmaps.cmake)
include(cmake/pch.cmake)
include(cmake/ccache.cmake)
# the .cll list dbcodegen compiles, gathered from the modules' SOURCES
include(cmake/dbcodegen.cmake)
if(BUILD_EDITOR)
    include(cmake/wxwidgets.cmake)
endif()
if(BUILD_SERVER)
    include(cmake/mariadb.cmake)
    include(cmake/sqlite.cmake)
endif()

# Diagnostics and the target wrappers that apply them. After every dependency
# above, because marking their headers SYSTEM needs their targets to exist, and
# before our own subdirectories so bk2_add_library is defined when they call it.
include(cmake/warnings.cmake)
include(cmake/bk2_targets.cmake)
include(cmake/system_includes.cmake)
bk2_mark_third_party_includes_system()

add_subdirectory("Versions/Temporary/Engine/Sources")

# Every target of ours that was not created through bk2_add_library or
# bk2_add_executable yet. Those two apply the rule set themselves and mark the
# target, so this only covers what is left of the migration and cannot double
# up. See cmake/warnings.cmake.
bk2_enable_warnings_on_project_targets()

if(WIN32 AND USE_MIMALLOC)
    # Apply after target creation, including tools/tests that exchange objects
    # with engine DLLs. External C/OS DLLs keep their own allocation functions.
    enable_project_mimalloc()
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Game)
