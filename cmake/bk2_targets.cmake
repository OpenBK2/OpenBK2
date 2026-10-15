# bk2_add_library / bk2_add_executable: add_library and add_executable plus the
# treatments every module in this tree gets anyway.
#
# Measured across the 42 project CMakeLists.txt files under
# Versions/Temporary/Engine/Sources: 40 of them call add_stdafx_pch, 40 call
# add_version_info, 31 call add_export_header, and nearly every library
# installs into bin. Those four lines are copied into every new module by hand
# today, and the diagnostics rule set would have been the fifth. One wrapper
# means the rule set has a single application point instead of 42.
#
# What the wrapper deliberately does NOT absorb is linking. The link lines are
# the part that genuinely varies: most modules have a WIN32 branch for the
# DirectX or Win32 libraries and a non-Windows branch for SDL or DXVK, and
# folding that into keyword arguments would make the common case longer rather
# than shorter. So a converted module reads:
#
#     project(Input)
#     set(SOURCES Bind.cpp Input.cpp stdafx.cpp)
#     bk2_add_library(Input SHARED ${SOURCES})
#     target_link_libraries(Input PRIVATE MemoryLib Misc System)
#     if(WIN32)
#         target_link_libraries(Input PRIVATE dxsdk::dinput8 dxsdk::dxguid winapi)
#     endif()
#
#     bk2_add_library(<name> <SHARED|STATIC|OBJECT|MODULE> <sources...>
#                     [NO_PCH] [NO_EXPORT_HEADER] [NO_INSTALL]
#                     [NO_VERSION_INFO] [NO_WARNINGS])
#     bk2_add_executable(<name> [WIN32] <sources...> [same options])
#
# The defaults follow what the target already is rather than what it is called:
# the PCH is attached only if the module has a stdafx.h, and export headers and
# install rules only to the kinds of target that had them before. The NO_*
# options are for the handful of exceptions, and each use of one should say why
# in a comment, the way the hand-written calls do now.

set(BK2_TARGET_OPTIONS NO_PCH NO_EXPORT_HEADER NO_INSTALL NO_VERSION_INFO NO_WARNINGS)

# The shared tail of both wrappers. `kind` is what add_library/add_executable
# was told, so the defaults can depend on it.
function(bk2_apply_target_treatments target kind)
    cmake_parse_arguments(ARG "${BK2_TARGET_OPTIONS}" "" "" ${ARGN})

    # generate_export_header's output is what every <MODULE>_EXPORT macro comes
    # from, so this is only meaningful for something other modules link against.
    if(NOT ARG_NO_EXPORT_HEADER AND kind STREQUAL "SHARED")
        add_export_header(${target})
    endif()

    # CMake force-includes the PCH into every source of the target, so a module
    # without a stdafx.h must not get one; cmake/pch.cmake explains the trap
    # with sources that deliberately avoid the prelude.
    if(NOT ARG_NO_PCH AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h")
        add_stdafx_pch(${target})
    endif()

    if(NOT ARG_NO_INSTALL AND (kind STREQUAL "SHARED" OR kind STREQUAL "EXECUTABLE"))
        # DESTINATION bin for libraries too, deliberately: the game loads its
        # modules from beside the executable.
        install(TARGETS ${target} DESTINATION bin)
    endif()

    if(NOT ARG_NO_VERSION_INFO AND (kind STREQUAL "SHARED" OR kind STREQUAL "EXECUTABLE"))
        add_version_info(${target})
    endif()

    if(NOT ARG_NO_WARNINGS)
        bk2_target_warnings(${target})
    endif()
endfunction()

function(bk2_add_library target)
    cmake_parse_arguments(ARG "${BK2_TARGET_OPTIONS}" "" "" ${ARGN})

    # The first remaining argument is the library kind if it names one, exactly
    # as add_library reads it; everything after is sources.
    set(rest ${ARG_UNPARSED_ARGUMENTS})
    list(GET rest 0 maybe_kind)
    if(maybe_kind MATCHES "^(SHARED|STATIC|OBJECT|MODULE)$")
        set(kind ${maybe_kind})
        list(REMOVE_AT rest 0)
    else()
        # Matching add_library: no keyword means BUILD_SHARED_LIBS decides. This
        # tree builds every module shared, so spell it out at the call site
        # instead of inheriting a global.
        message(FATAL_ERROR
            "bk2_add_library(${target}): say SHARED, STATIC, OBJECT or MODULE explicitly")
    endif()

    add_library(${target} ${kind} ${rest})
    bk2_apply_target_treatments(${target} ${kind} ${ARGN})
endfunction()

function(bk2_add_executable target)
    cmake_parse_arguments(ARG "${BK2_TARGET_OPTIONS};WIN32" "" "" ${ARGN})

    set(rest ${ARG_UNPARSED_ARGUMENTS})
    # WIN32 is add_executable's own keyword and has to reach it, not us.
    set(win32)
    if(ARG_WIN32)
        set(win32 WIN32)
    endif()

    add_executable(${target} ${win32} ${rest})
    bk2_apply_target_treatments(${target} EXECUTABLE ${ARGN})
endfunction()
