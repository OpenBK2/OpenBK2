# The input list for dbcodegen, reconstructed from the build.
#
# Each module lists its .cll type descriptions in its own SOURCES, next to the
# DB*.cpp generated from them; CMake compiles nothing from a .cll, it only
# records it. write_type_descriptions_list walks every target under the
# current directory, picks the .cll files out of their SOURCES and writes them,
# one absolute path per line, sorted, to <output>. dbcodegen reads that with
# --file-list. The two .cll files that belong to no module, base.cll and
# game.cll at the root of the sources, are not listed: dbcodegen adds them
# itself, as it always has.
#
# Call it once, after every subdirectory has been added, so the walk sees all
# of them.
#
# The list is only as complete as the configuration. Without BUILD_EDITOR the
# editor modules and their seven .cll files are not part of the build, and the
# list lacks them. The generated sources of the other modules do not depend on
# them, but types.xml collects every type, so it may only be regenerated from a
# list written with BUILD_EDITOR on.
#
# file(GENERATE) rewrites the file only when its content changes, so anything
# that depends on it reruns only when the set of .cll files actually changes.
include(cmake/get_all_targets.cmake)

function(write_type_descriptions_list output)
    get_all_targets(targets ${CMAKE_CURRENT_SOURCE_DIR})
    set(cll_files)
    foreach(target ${targets})
        get_target_property(target_sources ${target} SOURCES)
        if(NOT target_sources)
            continue()
        endif()
        get_target_property(target_dir ${target} SOURCE_DIR)
        foreach(source ${target_sources})
            if(source MATCHES "\\.cll$")
                cmake_path(ABSOLUTE_PATH source BASE_DIRECTORY ${target_dir} NORMALIZE
                           OUTPUT_VARIABLE cll_path)
                list(APPEND cll_files ${cll_path})
            endif()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES cll_files)
    list(SORT cll_files CASE INSENSITIVE)
    list(JOIN cll_files "\n" content)
    file(GENERATE OUTPUT ${output} CONTENT "${content}\n")
endfunction()
