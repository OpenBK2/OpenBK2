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

# Recording every call was the whole deliverable while this was a set of stubs.
# Now that the entry points do work it is a per-call format and a flushed write
# on a function the game calls 17,880 times in a few seconds, which turns the
# game into a slide show and makes animation impossible to judge. So it is a
# Debug-build thing, plus this switch for turning it on in a Release build when
# something needs diagnosing. The warn-once "not implemented" line is not
# affected and stays in every build.
option(LIBGR2_TRACE "record every entry point call, in any build type" OFF)

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
    src/Control.h
    src/Convert.cpp
    src/Convert.h
    src/Curve.cpp
    src/Curve.h
    src/File.cpp
    src/File.h
    src/Identify.cpp
    src/Identify.h
    src/LocalPose.h
    src/Mesh.cpp
    src/Model.cpp
    src/ModelInstance.h
    src/Oodle0.cpp
    src/Oodle0.h
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
    src/Version.cpp
    src/WorldPose.h
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

# The per-call trace, on for Debug and for -DLIBGR2_TRACE=ON. A generator
# expression rather than an if(), so that it is right under a multi-config
# generator as well as under Ninja.
set(GR2_TRACE_CONDITION $<OR:$<CONFIG:Debug>,$<BOOL:${LIBGR2_TRACE}>>)
target_compile_definitions(gr2 PRIVATE $<${GR2_TRACE_CONDITION}:LIBGR2_TRACE_CALLS>)

# Header only, so that this library stays a single DLL with no runtime of its
# own to install beside it. PRIVATE because none of it reaches gr2/granny.h.
target_link_libraries(gr2 PRIVATE spdlog::spdlog_header_only)

set_target_properties(gr2 PROPERTIES
    OUTPUT_NAME ${GR2_OUTPUT_NAME}
    # Nothing but the 63 entry points is public. On Windows that is already true,
    # since only __declspec(dllexport) exports; elsewhere it takes saying so, and
    # the header's visibility("default") is what re-exposes the 63.
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

# The recording shim: the same 63 names, the same log, forwarded to the real
# granny2. It is what makes a libgr2 log worth diffing, since on its own a log
# says what this library did and not what it should have done. Windows only,
# because the DLL it forwards to is.
#
# Not EXCLUDE_FROM_ALL. It is four small files and it shares three of them with
# the library, so building it costs almost nothing, and a diagnostic that has to
# be asked for is a diagnostic that quietly stops compiling.
option(LIBGR2_BUILD_SHIM "build the recording shim over the real granny2" ${WIN32})

if(LIBGR2_BUILD_SHIM AND WIN32)
    add_library(gr2-shim SHARED
        src/Shim.cpp
        src/ShimLoader.cpp
        src/ShimLoader.h
        src/Identify.cpp
        src/Identify.h
        src/Trace.cpp
        src/Trace.h
        src/Structures.h
        include/gr2/granny.h)
    target_include_directories(gr2-shim
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    # Always traced. A shim with the per-call trace compiled out forwards
    # everything and records nothing, which is the one thing it must not do.
    target_compile_definitions(gr2-shim PRIVATE GR2_BUILD_SHARED LIBGR2_TRACE_CALLS)
    target_link_libraries(gr2-shim PRIVATE spdlog::spdlog_header_only)
    set_target_properties(gr2-shim PROPERTIES
        OUTPUT_NAME ${GR2_OUTPUT_NAME}
        # The same file name as the library, since both stand in for the same
        # DLL, so they cannot share a directory.
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shim
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shim
        FOLDER "vendor/libgr2")
    if(MSVC)
        # Not /W4. Shim.cpp is generated, and holding generated code to the same
        # standard as written code means editing the generator to satisfy a
        # warning about code nobody reads.
        target_compile_options(gr2-shim PRIVATE /W3)
    endif()
    if(GR2_SPDLOG_INCLUDES)
        target_include_directories(gr2-shim SYSTEM PRIVATE ${GR2_SPDLOG_INCLUDES})
        if(MSVC)
            target_compile_options(gr2-shim PRIVATE /external:W0)
        endif()
    endif()

    # The same acceptance test as the library's, which is what catches a
    # generator that quietly dropped an entry point.
    add_custom_target(libgr2-verify-shim-exports
        COMMAND ${CMAKE_COMMAND}
                -DGR2_LIBRARY=$<TARGET_FILE:gr2-shim>
                -DGR2_EXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/exports.txt
                -DGR2_MSVC=$<BOOL:${MSVC}>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyExports.cmake
        DEPENDS gr2-shim
        VERBATIM
        COMMENT "checking the shim's exports against exports.txt")
endif()

if(LIBGR2_BUILD_TESTS)
    enable_testing()

    # The same code again, statically, for the tests to link. A second target
    # rather than an object library shared with gr2, because the two differ in a
    # compile definition: GR2_BUILD_SHARED makes GR2_API an export and GR2_STATIC
    # makes it neither an export nor an import, and an object library fixes its
    # definitions at compile time and so cannot serve both.
    #
    # The cost is compiling twelve small files twice, which is worth paying for
    # tests that reach the internals the 63 exports do not expose and that need no
    # DLL on PATH to run.
    add_library(gr2_static STATIC EXCLUDE_FROM_ALL ${GR2_SOURCES})
    target_include_directories(gr2_static
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
    # PUBLIC, unlike gr2's GR2_BUILD_SHARED: a test including gr2/granny.h has to
    # agree with the library about what GR2_API means, and here it means neither
    # dllexport nor dllimport.
    target_compile_definitions(gr2_static PUBLIC GR2_STATIC)
    # The tests get the same trace decision as the DLL, so that a Debug test run
    # says what it called and a Release one runs at speed.
    target_compile_definitions(gr2_static
        PRIVATE $<${GR2_TRACE_CONDITION}:LIBGR2_TRACE_CALLS>)
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
