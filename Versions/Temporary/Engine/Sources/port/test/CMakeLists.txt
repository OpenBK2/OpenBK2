# Also usable as an isolated project against dependencies already fetched by a
# game build: -DOBK2_DEPENDENCY_BUILD_DIR=/path/to/out/build/<preset>.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.23)
    project(CrashpadIntegration LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    enable_testing()
    set(OBK2_DEPENDENCY_BUILD_DIR "" CACHE PATH "Existing game build containing _deps")
    if(NOT EXISTS "${OBK2_DEPENDENCY_BUILD_DIR}/_deps/crashpad-src/CMakeLists.txt")
        message(FATAL_ERROR "OBK2_DEPENDENCY_BUILD_DIR must contain fetched Crashpad sources")
    endif()
    add_subdirectory("${OBK2_DEPENDENCY_BUILD_DIR}/_deps/crashpad-src" crashpad)
    set(CRASHPAD_TEST_BOOST_INCLUDE
        "${OBK2_DEPENDENCY_BUILD_DIR}/_deps/boost-src/libs/predef/include")
endif()

find_package(Python3 3.7 QUIET COMPONENTS Interpreter)
if(NOT Python3_FOUND)
    # Building the game itself must not acquire a Python prerequisite just for
    # this subprocess/report validator. CI with Python runs the registered tests.
    message(STATUS "Python 3.7+ unavailable: Crashpad integration tests skipped")
    return()
endif()
find_package(Threads REQUIRED)
add_executable(crashpad_integration_probe EXCLUDE_FROM_ALL CrashpadProbe.cpp)
target_include_directories(crashpad_integration_probe PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../.." ${CRASHPAD_TEST_BOOST_INCLUDE})
target_link_libraries(crashpad_integration_probe PRIVATE crashpad_client Threads::Threads)
if(TARGET Boost::predef)
    target_link_libraries(crashpad_integration_probe PRIVATE Boost::predef)
endif()
add_dependencies(crashpad_integration_probe crashpad_handler)
if(COMMAND copy_runtime_dlls)
    # The main build can attach shared mimalloc to every executable. Carry its
    # runtime into each isolated probe too, before any crash-handler code runs.
    copy_runtime_dlls(crashpad_integration_probe)
endif()

set(crashpad_cases access-violation worker-access-violation abort terminate
    caught-exception fallback-directory missing-handler)
if(WIN32)
    list(APPEND crashpad_cases heap-corruption invalid-parameter purecall)
endif()
foreach(case IN LISTS crashpad_cases)
    add_test(NAME "Crashpad.${case}" COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/verify_crashpad.py"
        --probe "$<TARGET_FILE:crashpad_integration_probe>"
        --handler "$<TARGET_FILE:crashpad_handler>"
        --case "${case}" --work-root "${CMAKE_CURRENT_BINARY_DIR}/reports")
    set_tests_properties("Crashpad.${case}" PROPERTIES TIMEOUT 90 LABELS "obk2-test;crashpad")
endforeach()
if(NOT TARGET unittests)
    add_custom_target(unittests)
endif()
add_dependencies(unittests crashpad_integration_probe)
