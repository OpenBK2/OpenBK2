# 3.23 is what CMakePresets.json already asks for, and presets are how both
# the documented build and CI configure this, so nothing older has worked
# here in a long time. It also covers TARGET_RUNTIME_DLLS, added in 3.21.
CMAKE_MINIMUM_REQUIRED(VERSION 3.23)
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

option(BUILD_EDITOR "build editor" OFF)
option(USE_MIMALLOC "replace the process allocator with mimalloc" ON)
option(ENABLE_ASAN "build with AddressSanitizer" OFF)

# AddressSanitizer installs its own allocator to see every allocation and
# free, so it cannot share a process with mimalloc, which replaces the same
# thing. Turning one on turns the other off rather than leaving a combination
# that builds and then reports nothing useful.
if(ENABLE_ASAN AND USE_MIMALLOC)
    message(STATUS "ENABLE_ASAN: mimalloc switched off, AddressSanitizer replaces the allocator")
    set(USE_MIMALLOC OFF CACHE BOOL "replace the process allocator with mimalloc" FORCE)
endif()

if(ENABLE_ASAN)
    # -g because a sanitizer report without symbols names an address and
    # nothing else, and no-omit-frame-pointer because the stack it prints is
    # walked rather than read from debug info.
    if(MSVC)
        add_compile_options(/fsanitize=address /Zi)
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address)
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
endif()

include(cmake/winapi.cmake)

set(ZLIB_COMPAT ON)
include(cmake/zlib-ng.cmake)

set(SDLMIXER_VENDORED OFF)
include(cmake/sdl.cmake)
include(cmake/sdl_mixer.cmake)

include(cmake/googletest.cmake)
include(cmake/unittest.cmake)
include(cmake/benchmarks.cmake)
include(cmake/googlebenchmark.cmake)
include(cmake/boost.cmake)
if(USE_MIMALLOC)
    include(cmake/mimalloc.cmake)
endif()
include(cmake/glm.cmake)
if(WIN32)
include(cmake/dxsdk.cmake)
else()
include(cmake/dxvk.cmake)
endif()
include(cmake/fmod.cmake)
include(cmake/granny.cmake)
include(cmake/fmt.cmake)
include(cmake/spdlog.cmake)
include(cmake/ffmpeg.cmake)
include(cmake/crashpad.cmake)
include(cmake/versioninfo.cmake)
include(cmake/exportheader.cmake)
include(cmake/pch.cmake)
include(cmake/ccache.cmake)

add_subdirectory("Versions/Temporary/Engine/Sources")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Game)
