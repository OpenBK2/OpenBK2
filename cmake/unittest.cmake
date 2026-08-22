# Shared definition of a unit test target, so each test directory names its
# own sources and libraries and nothing else.
#
#   add_unit_test(<filename> [<library> ...] [SOURCES <source> ...])
#
# builds <filename>.cpp into <filename>_test, links googletest's main plus the
# libraries given, and copies whatever the result loads at runtime next to it.
#
# SOURCES adds further translation units to the test executable, for a test that
# has to compile the implementation rather than reach it through a shared library.
#
# Tests are EXCLUDE_FROM_ALL and no add_test() is registered, so ctest finds
# nothing and a test is built by naming its target.
function(add_unit_test filename)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})
    set(target_name ${filename}_test)
    add_executable(${target_name} EXCLUDE_FROM_ALL ${filename}.cpp ${ARG_SOURCES})
    target_link_libraries(${target_name} PRIVATE GTest::gtest_main ${ARG_UNPARSED_ARGUMENTS})
    set_target_properties(${target_name} PROPERTIES FOLDER "unittest")

    # Windows resolves a DLL next to the executable or on PATH, never in the
    # working directory, and every module here builds into its own directory.
    # TARGET_RUNTIME_DLLS reads the set from the link closure, which covers
    # googletest and the engine modules without either being named.
    if(WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:${target_name}> $<TARGET_FILE_DIR:${target_name}>
            COMMAND_EXPAND_LISTS
            VERBATIM)
    endif()
endfunction()
