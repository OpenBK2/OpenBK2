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

# On by default standalone, where building this project at all means working on
# it, and off when it is a subdirectory of something else, which does not want
# googletest fetched on its behalf.
option(LIBGR2_BUILD_TESTS "build libgr2's unit tests" ${PROJECT_IS_TOP_LEVEL})

# The shipped game loads granny2.dll by name, and the engine's import library is
# generated from the same name, so the replacement has to answer to it. The x64
# suffix is the vendored DLL's own convention, kept so that the two builds can
# sit in one directory.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(GR2_OUTPUT_NAME granny2_x64)
else()
    set(GR2_OUTPUT_NAME granny2)
endif()

# spdlog carries the call trace. When this is configured from the engine tree the
# root CMakeLists has already brought it in, so take that one and stay on the
# version the rest of the build uses; standalone, fetch it at the same pin as
# cmake/spdlog.cmake.
if(NOT TARGET spdlog::spdlog_header_only)
    include(FetchContent)
    FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG 486b55554f11c9cccc913e11a87085b2a91f706f
            GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(spdlog)
endif()

set(GR2_SOURCES
    src/Allocator.cpp
    src/Animation.cpp
    src/Control.cpp
    src/Convert.cpp
    src/Convert.h
    src/File.cpp
    src/File.h
    src/Mesh.cpp
    src/Model.cpp
    src/Oodle1.cpp
    src/Oodle1.h
    src/Pose.cpp
    src/Skeleton.cpp
    src/Structures.h
    src/Trace.cpp
    src/Trace.h
    src/Transform.cpp
    src/TypeTree.cpp
    src/TypeTree.h
    include/gr2/granny.h)

add_library(gr2 SHARED ${GR2_SOURCES})
add_library(gr2::gr2 ALIAS gr2)

target_include_directories(gr2
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# GR2_BUILD_SHARED turns GR2_API into an export rather than an import. It is
# PRIVATE because a consumer that inherited it would try to export the same
# names again.
target_compile_definitions(gr2 PRIVATE GR2_BUILD_SHARED)

# Header only, so that this library stays a single DLL with no runtime of its
# own to install beside it. PRIVATE because none of it reaches gr2/granny.h.
target_link_libraries(gr2 PRIVATE spdlog::spdlog_header_only)

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

# spdlog's bundled fmt is not ours to warn about. On a 32-bit build its
# size_t narrowing alone produces five C4244s per translation unit, which buries
# anything this library says about its own code. Marking the headers as system
# takes MSVC's /external:I path, and /external:W0 is what actually lowers the
# level once they are on it.
get_target_property(GR2_SPDLOG_INCLUDES spdlog::spdlog_header_only INTERFACE_INCLUDE_DIRECTORIES)
if(GR2_SPDLOG_INCLUDES)
    target_include_directories(gr2 SYSTEM PRIVATE ${GR2_SPDLOG_INCLUDES})
    if(MSVC)
        target_compile_options(gr2 PRIVATE /external:W0)
    endif()
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

if(LIBGR2_BUILD_TESTS)
    enable_testing()

    # The same code again, statically, for the tests to link. A second target
    # rather than an object library shared with gr2, because the two differ in a
    # compile definition: GR2_BUILD_SHARED makes GR2_API an export and GR2_STATIC
    # makes it neither an export nor an import, and an object library fixes its
    # definitions at compile time and so cannot serve both.
    #
    # The cost is compiling twelve small files twice, which is worth paying for
    # tests that reach the internals the 54 exports do not expose and that need no
    # DLL on PATH to run.
    add_library(gr2_static STATIC EXCLUDE_FROM_ALL ${GR2_SOURCES})
    target_include_directories(gr2_static
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
    # PUBLIC, unlike gr2's GR2_BUILD_SHARED: a test including gr2/granny.h has to
    # agree with the library about what GR2_API means, and here it means neither
    # dllexport nor dllimport.
    target_compile_definitions(gr2_static PUBLIC GR2_STATIC)
    target_link_libraries(gr2_static PRIVATE spdlog::spdlog_header_only)
    set_target_properties(gr2_static PROPERTIES FOLDER "vendor/libgr2")
    if(MSVC)
        target_compile_options(gr2_static PRIVATE /W4)
    else()
        target_compile_options(gr2_static PRIVATE -Wall -Wextra)
    endif()
    if(GR2_SPDLOG_INCLUDES)
        target_include_directories(gr2_static SYSTEM PRIVATE ${GR2_SPDLOG_INCLUDES})
        if(MSVC)
            target_compile_options(gr2_static PRIVATE /external:W0)
        endif()
    endif()

    add_subdirectory(test)
endif()
