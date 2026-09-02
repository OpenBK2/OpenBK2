# googletest, at the pin the rest of the tree uses. Reused rather than fetched
# again when this is configured from the engine tree, where cmake/googletest.cmake
# has already brought it in; standalone there is nothing to reuse and it is
# fetched here.
if(NOT TARGET GTest::gtest_main)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG de1c60926218478959040ad79be8b2dbed22bd2b
            GIT_PROGRESS TRUE
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    # gmock lands in the default target, so leaving it on means a plain
    # `cmake --build` of this project compiles gtest-all.cc three more times for a
    # library nothing here links.
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Local, because libgr2 is standalone and cannot reach the tree's
# cmake/unittest.cmake without giving that up. The ctest label is the tree's
# though, so that `ctest -L obk2-test` covers these too once libgr2 is wired in.
#
# Tests link gr2_static rather than the DLL. Two reasons: the container, the type
# tree walker and the two Oodle decoders are internal and will need testing
# directly, none of them being among the 54 exports; and a static link needs no
# DLL beside the executable, which is what lets these run in CI on a machine that
# has never seen granny2.dll.
function(gr2_add_test filename)
    set(target_name ${filename}_test)
    add_executable(${target_name} EXCLUDE_FROM_ALL ${filename}.cpp MinimalGr2.cpp)
    target_link_libraries(${target_name} PRIVATE gr2_static GTest::gtest_main)
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(${target_name} PROPERTIES FOLDER "vendor/libgr2/test")

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra)
    endif()

    if(NOT TARGET libgr2-tests)
        add_custom_target(libgr2-tests COMMENT "every libgr2 unit test")
    endif()
    add_dependencies(libgr2-tests ${target_name})

    add_test(NAME ${target_name} COMMAND ${target_name})
    set_tests_properties(${target_name} PROPERTIES LABELS "obk2-test")
endfunction()

gr2_add_test(ReadEntireFileFromMemory)
gr2_add_test(FileContainer)
gr2_add_test(Oodle1)
gr2_add_test(FileInfo)
gr2_add_test(Animation)
gr2_add_test(Curve)
gr2_add_test(Control)
gr2_add_test(ModelInstance)
gr2_add_test(LocalPose)
gr2_add_test(SampleModelAnimations)
gr2_add_test(WorldPose)
gr2_add_test(Oodle0)
gr2_add_test(Mesh)
gr2_add_test(Version)

# Oodle0 has no committed stream fixtures, because granny2.dll 2.11 decodes that
# codec and cannot encode it: GrannyBeginFileCompression refuses it and no
# GrannyOodle0Compress is exported. The test uses GR2 files this repository
# already ships instead, 14,115 of which are pure Oodle0, checked against
# checksums taken from the real DLL. Defined only when that data is present, so an
# extracted copy of this library compiles and skips.
# test -> libgr2 -> vendor -> Sources -> Engine -> Temporary -> Versions -> repo
get_filename_component(LIBGR2_REPO_ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../.. ABSOLUTE)
if(EXISTS ${LIBGR2_REPO_ROOT}/Versions/Current/Data/bin)
    target_compile_definitions(Oodle0_test
        PRIVATE LIBGR2_REPO_DATA_DIR="${LIBGR2_REPO_ROOT}")
    message(STATUS "libgr2: Oodle0 tests will read ${LIBGR2_REPO_ROOT}/Versions/Current/Data")
else()
    message(STATUS "libgr2: no Versions/Current/Data, the Oodle0 data tests will skip")
endif()
