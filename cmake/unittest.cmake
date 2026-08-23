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
# Tests are EXCLUDE_FROM_ALL, so a test is built by naming its target. They are
# registered with ctest under the "obk2" label; see add_test below.
function(add_unit_test filename)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})
    set(target_name ${filename}_test)
    add_executable(${target_name} EXCLUDE_FROM_ALL ${filename}.cpp ${ARG_SOURCES})
    target_link_libraries(${target_name} PRIVATE GTest::gtest_main ${ARG_UNPARSED_ARGUMENTS})
    set_target_properties(${target_name} PROPERTIES FOLDER "unittest")

    # Registered with ctest under a label, because the dependencies register around a
    # hundred and fifty tests of their own and there has to be a way to ask for only
    # this project's. Run them with:  ctest -L obk2-test
    #
    # -L takes a regex, so the label has to be more than a prefix of the
    # benchmark one: plain "obk2" would select both. "obk2" on its own is
    # still useful and means everything this project registers.
    #
    # The target stays EXCLUDE_FROM_ALL, so ctest reports "Unable to find executable"
    # unless it was built first. That is the intended workflow: build the tests you
    # want, then ask ctest for the label.
    # Aggregate target, so building the suite does not mean listing it. ctest cannot
    # build EXCLUDE_FROM_ALL targets itself, so something has to, and a list of names
    # in a workflow file goes stale the first time someone adds a test.
    if(NOT TARGET unittests)
        add_custom_target(unittests COMMENT "every target added by add_unit_test")
    endif()
    add_dependencies(unittests ${target_name})

    add_test(NAME ${target_name} COMMAND ${target_name})
    set_tests_properties(${target_name} PROPERTIES LABELS "obk2-test")

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
