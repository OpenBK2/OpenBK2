CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
project(b2)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EDITOR "build editor" OFF)

if(MSVC)
  add_compile_options("/nologo")
  add_link_options("/nologo")
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

include(cmake/winapi.cmake)

set(ZLIB_COMPAT ON)
include(cmake/zlib-ng.cmake)

set(SDLMIXER_VENDORED OFF)
include(cmake/sdl.cmake)
include(cmake/sdl_mixer.cmake)

include(cmake/googletest.cmake)
include(cmake/boost.cmake)
include(cmake/glm.cmake)
include(cmake/dxsdk.cmake)
include(cmake/fmod.cmake)
include(cmake/granny.cmake)
include(cmake/fmt.cmake)
include(cmake/versioninfo.cmake)

add_subdirectory("Versions/Temporary/Engine/Sources")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Game)
