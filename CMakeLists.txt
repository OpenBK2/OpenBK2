CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
project(b2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EDITOR "build editor" OFF)

if(MSVC)
  add_compile_options("/wd4251")
  add_compile_options("/wd4275")
  add_compile_options("/FS")
  foreach(flag_var
          CMAKE_CXX_FLAGS_DEBUG
          CMAKE_C_FLAGS_DEBUG)
    string(REPLACE "/Zi" "/Z7" ${flag_var} "${${flag_var}}")
  endforeach()
  add_definitions("-D_CRT_SECURE_NO_WARNINGS=1")
  add_definitions("-D_CRT_NONSTDC_NO_DEPRECATE=1")
  add_definitions("-D_CRT_NON_CONFORMING_SWPRINTFS=1")
endif()

set(ZLIB_COMPAT ON)
add_subdirectory(third_party/zlib-ng)

set(SDLMIXER_VENDORED OFF)
add_subdirectory(third_party/SDL)
add_subdirectory(third_party/SDL_mixer)

include(cmake/dxsdk.cmake)
include(cmake/fmod.cmake)
include(cmake/granny.cmake)

add_subdirectory("Versions/Temporary/Engine/Sources")
