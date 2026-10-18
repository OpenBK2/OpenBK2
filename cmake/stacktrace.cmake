# One definition of "which Boost.Stacktrace backend this platform can use".
#
#   target_boost_stacktrace(<target> <PUBLIC|PRIVATE|INTERFACE>)
#
# links the backend and defines the macro that selects it, which have to name
# the same one.
#
# The backend decides how readable a trace is. libbacktrace resolves function,
# file and line out of the DWARF in the binary and is the best of them;
# addr2line shells out per frame; basic gives symbol names only; windbg reads
# the PDB. None of them invent debug information: a build configured as plain
# Release has no -g and resolves to library names and offsets, which still says
# which library a frame is in but not where. RelWithDebInfo, which is what the
# Release presets map to, carries -g.
#
# Boost builds stacktrace_backtrace only if its own configure check compiled
# <backtrace.h>, and that header ships in GCC's private include directory
# rather than as a distribution package, so the check succeeds under GCC and
# fails under clang on the same machine. Asking for the target unconditionally
# is a generate-time error there, not a degraded trace.
function(target_boost_stacktrace target scope)
    if(WIN32)
        # _cached keeps the symbol handler alive between frames and is much
        # faster; both need dbgeng, which Boost's check reports on.
        if(TARGET Boost::stacktrace_windbg_cached)
            target_link_libraries(${target} ${scope} Boost::stacktrace_windbg_cached)
            target_compile_definitions(${target} ${scope} BOOST_STACKTRACE_USE_WINDBG_CACHED)
        elseif(TARGET Boost::stacktrace_windbg)
            target_link_libraries(${target} ${scope} Boost::stacktrace_windbg)
            target_compile_definitions(${target} ${scope} BOOST_STACKTRACE_USE_WINDBG)
        else()
            target_link_libraries(${target} ${scope} Boost::stacktrace_basic)
        endif()
    elseif(TARGET Boost::stacktrace_backtrace)
        target_link_libraries(${target} ${scope} Boost::stacktrace_backtrace)
        target_compile_definitions(${target} ${scope} BOOST_STACKTRACE_USE_BACKTRACE)
    else()
        target_link_libraries(${target} ${scope} Boost::stacktrace_addr2line)
        target_compile_definitions(${target} ${scope} BOOST_STACKTRACE_USE_ADDR2LINE)
    endif()
endfunction()
