# libgr2, a native replacement for RAD Game Tools' granny2.dll.
#
# Standalone on purpose. It is not referenced from
# Versions/Temporary/Engine/Sources/CMakeLists.txt and links nothing from the
# engine, so it configures and builds on its own:
#
#   cmake -S Versions/Temporary/Engine/Sources/vendor/libgr2 -B out/build/libgr2 -G Ninja
#   cmake --build out/build/libgr2
#   cmake --build out/build/libgr2 --target libgr2-verify-exports
#
# project() below runs either way, so add_subdirectory() from the engine tree
# works unchanged once there is something worth linking against.

cmake_minimum_required(VERSION 3.23)
project(libgr2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The shipped game loads granny2.dll by name, and the engine's import library is
# generated from the same name, so the replacement has to answer to it. The x64
# suffix is the vendored DLL's own convention, kept so that the two builds can
# sit in one directory.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(GR2_OUTPUT_NAME granny2_x64)
else()
    set(GR2_OUTPUT_NAME granny2)
endif()

add_library(gr2 SHARED
    src/Allocator.cpp
    src/Animation.cpp
    src/Control.cpp
    src/File.cpp
    src/Mesh.cpp
    src/Model.cpp
    src/Pose.cpp
    src/Skeleton.cpp
    src/Transform.cpp
    src/TypeTree.cpp
    src/Unimplemented.cpp
    src/Unimplemented.h
    include/gr2/granny.h)

add_library(gr2::gr2 ALIAS gr2)

target_include_directories(gr2
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# GR2_BUILD_SHARED turns GR2_API into an export rather than an import. It is
# PRIVATE because a consumer that inherited it would try to export the same
# names again.
target_compile_definitions(gr2 PRIVATE GR2_BUILD_SHARED)

set_target_properties(gr2 PROPERTIES
    OUTPUT_NAME ${GR2_OUTPUT_NAME}
    # Nothing but the 54 entry points is public. On Windows that is already true,
    # since only __declspec(dllexport) exports; elsewhere it takes saying so, and
    # the header's visibility("default") is what re-exposes the 54.
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    FOLDER "vendor/libgr2")

if(MSVC)
    target_compile_options(gr2 PRIVATE /W4)
else()
    target_compile_options(gr2 PRIVATE -Wall -Wextra)
endif()

# The acceptance test for the skeleton: the DLL exports these names, all of them,
# and nothing else. Kept as a target rather than a post-build step so that an
# ordinary build does not depend on dumpbin or nm being on PATH.
add_custom_target(libgr2-verify-exports
    COMMAND ${CMAKE_COMMAND}
            -DGR2_LIBRARY=$<TARGET_FILE:gr2>
            -DGR2_EXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/exports.txt
            -DGR2_MSVC=$<BOOL:${MSVC}>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyExports.cmake
    DEPENDS gr2
    VERBATIM
    COMMENT "checking ${GR2_OUTPUT_NAME} exports against exports.txt")
