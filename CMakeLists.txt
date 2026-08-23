CMAKE_MINIMUM_REQUIRED(VERSION 3.18)
# ASM_MASM is for the MMX reference implementations under 3Dmotor/test/original/.
# MSVC accepts __asm on x86 only, so keeping those references as inline assembly
# made the bit-exactness tests unbuildable on x64.
project(b2 LANGUAGES C CXX ASM_MASM)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EDITOR "build editor" OFF)

include(cmake/arch.cmake)

if(MSVC)
  add_compile_options("/nologo")
  add_link_options("/nologo")
  # Scoped to the compilers that understand them: ml/ml64 rejects each of these
  # with warning A4018 now that ASM_MASM is an enabled language.
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/wd4251>")
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/wd4275>")
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/FS>")
  foreach(flag_var
          CMAKE_CXX_FLAGS_DEBUG
          CMAKE_C_FLAGS_DEBUG)
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
include(cmake/googlebenchmark.cmake)
include(cmake/boost.cmake)
include(cmake/mimalloc.cmake)
include(cmake/glm.cmake)
include(cmake/dxsdk.cmake)
include(cmake/fmod.cmake)
include(cmake/granny.cmake)
include(cmake/fmt.cmake)
include(cmake/ffmpeg.cmake)
include(cmake/crashpad.cmake)
include(cmake/versioninfo.cmake)

add_subdirectory("Versions/Temporary/Engine/Sources")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Game)
