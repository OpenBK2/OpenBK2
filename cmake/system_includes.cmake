# Treat third-party headers as system headers, so their warnings are not ours.
#
# This is the prerequisite for any warning work, not a tidiness pass. Measured
# before it existed: AILogic compiled with 43 `-I` paths into _deps and zero
# `-isystem`, so every fetched dependency was diagnosed as if we had written
# it. That is what made -Wundef look like 69 sites when 4 are ours, and
# -Wsuggest-override look like 295 when **none** are ours; 271 of those were in
# the DXVK d3d9.h shim, which off Windows stands in for the platform SDK.
#
# CMake already does this for IMPORTED targets, which is why /usr/include is
# quiet. It does not do it for the real targets FetchContent adds to our build,
# because as far as CMake is concerned those are part of this project.
#
# FetchContent_Declare gained a SYSTEM argument in CMake 3.25 and the floor here
# is 3.23 (CMakeLists.txt explains why), so rather than version-gate seventeen
# FetchContent calls this moves the include directories afterwards, which works
# on every version and keeps the decision in one file.

include(${CMAKE_CURRENT_LIST_DIR}/get_all_targets.cmake)

# INTERFACE_SYSTEM_INCLUDE_DIRECTORIES is what the generator consults when it
# decides between -I and -isystem for a consumer. Copying rather than moving:
# INTERFACE_INCLUDE_DIRECTORIES still has to list the directories, or the
# consumer would not get them at all.
function(bk2_mark_target_includes_system target)
    if(NOT TARGET ${target})
        return()
    endif()
    get_target_property(imported ${target} IMPORTED)
    if(imported)
        # Already system by default, and most of its properties are read-only.
        return()
    endif()
    get_target_property(kind ${target} TYPE)
    if(kind STREQUAL "UTILITY" OR kind STREQUAL "EXECUTABLE")
        return()
    endif()
    get_target_property(includes ${target} INTERFACE_INCLUDE_DIRECTORIES)
    if(includes)
        set_target_properties(${target} PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${includes}")
    endif()
endfunction()

# Everything FetchContent populated, found by where its sources live, plus the
# hand-written wrappers around vendored third-party sources, which live in
# cmake/ and so have this project's source directory and cannot be found that
# way. Call after the dependencies are declared and before or after our own
# subdirectories: target properties are read at generate time.
function(bk2_mark_third_party_includes_system)
    get_all_targets(all_targets ${CMAKE_SOURCE_DIR})
    foreach(target IN LISTS all_targets)
        get_target_property(source_dir ${target} SOURCE_DIR)
        if(source_dir MATCHES "/_deps/")
            bk2_mark_target_includes_system(${target})
        endif()
    endforeach()

    # Built from third-party sources by our own cmake/*.cmake files: squish and
    # sqlite3 carry PUBLIC include directories, fmod points at the flessd
    # submodule, mariadbclient at its own INTERFACE tree.
    foreach(target squish sqlite3 fmod mariadbclient scintilla)
        bk2_mark_target_includes_system(${target})
    endforeach()
endfunction()
