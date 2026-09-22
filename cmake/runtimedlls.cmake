# Putting the DLLs a target loads at run time where it can find them.
#
# Windows resolves a DLL next to the executable or on PATH, never in the
# working directory, and every engine module here builds into its own
# directory. Without this an executable in the build tree cannot start at all:
# the loader fails with STATUS_DLL_NOT_FOUND (0xC0000135) before main() is
# reached, and the process dies with no message of its own.
#
# TARGET_RUNTIME_DLLS resolves the set from the target's link closure, so
# nothing has to be named by hand. That matters because the dependencies here
# are fetched and built rather than found: Boost is the one that bites, since
# FetchContent builds the whole distribution and a hand-written list goes stale
# the moment the set changes. The engine's own modules are resolved too, which
# is harmless.
#
# Both are no-ops off Windows. ELF has no TARGET_RUNTIME_DLLS counterpart, and
# an executable built here carries a RUNPATH that already finds its libraries
# in the build tree; Game installs its closure through
# file(GET_RUNTIME_DEPENDENCIES) instead, which is a different enough job to
# stay where it is.

# Beside the executable in the build tree only. For tests and benchmarks, which
# are run from the build tree and never installed.
function(copy_runtime_dlls target)
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS
            VERBATIM)
    endif()
endfunction()

# The same, and into bin/ on install. For the shipped executables.
function(install_runtime_dlls target)
    copy_runtime_dlls(${target})
    if(WIN32)
        install(FILES $<TARGET_RUNTIME_DLLS:${target}> DESTINATION bin)
    endif()
endfunction()
