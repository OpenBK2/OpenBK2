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
#
# The same walk guards against a circular dependency. dbcodegen links libdb,
# Parser and the base libraries, and none of them owns a .cll: the generated
# code depends on libdb, never the other way round. A .cll added to any
# library in dbcodegen's link closure would make the generator depend on its
# own output, so configuring stops with an error naming that library.
include(cmake/get_all_targets.cmake)

# Every non-imported target that <target> links, directly or through the
# libraries it links, into <out_var>.
function(_dbcodegen_link_closure target out_var)
    set(pending ${target})
    set(seen)
    while(pending)
        list(POP_FRONT pending current)
        get_target_property(links ${current} LINK_LIBRARIES)
        get_target_property(interface_links ${current} INTERFACE_LINK_LIBRARIES)
        foreach(link ${links} ${interface_links})
            # target_link_libraries(PRIVATE) on a static library records its
            # dependencies as $<LINK_ONLY:x>
            if(link MATCHES "^\\$<LINK_ONLY:(.+)>$")
                set(link ${CMAKE_MATCH_1})
            endif()
            if(NOT TARGET ${link} OR link IN_LIST seen)
                continue()
            endif()
            get_target_property(imported ${link} IMPORTED)
            if(imported)
                continue()
            endif()
            list(APPEND seen ${link})
            list(APPEND pending ${link})
        endforeach()
    endwhile()
    set(${out_var} ${seen} PARENT_SCOPE)
endfunction()

function(write_type_descriptions_list output)
    get_all_targets(targets ${CMAKE_CURRENT_SOURCE_DIR})
    _dbcodegen_link_closure(dbcodegen generator_closure)
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
                if(target IN_LIST generator_closure)
                    message(FATAL_ERROR
                        "${target} lists ${source}, but dbcodegen links ${target}: the "
                        "generator would depend on code it generates. Move the .cll to a "
                        "module dbcodegen does not link.")
                endif()
            endif()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES cll_files)
    list(SORT cll_files CASE INSENSITIVE)
    list(JOIN cll_files "\n" content)
    file(GENERATE OUTPUT ${output} CONTENT "${content}\n")
endfunction()

# regenerate-db: rewrites the committed DB*.h/.cpp sources and
# Versions/Current/Data/types.xml from the .cll files. Run it by hand after
# changing a .cll, then review and commit the diff.
#
# Never part of ALL, and nothing in the build depends on its output: the
# generated files are committed, so a build never runs the generator and has no
# rule deciding when to rerun it. CI runs this target after the build and fails
# when it changes anything, which catches both a hand edit to a generated file
# and a .cll change committed without regenerating.
#
# dbcodegen rewrites only the files whose content differs, so a run that
# changes nothing leaves every timestamp alone and triggers no recompile.
#
# The DLLs are copied beside dbcodegen first. Its own POST_BUILD copy runs only
# when dbcodegen relinks, and a rebuilt System.dll or libdb.dll does not always
# relink it, which would leave the generator running against the old copies.
#
# Without BUILD_EDITOR the .cll list lacks the editor's types, and the
# types.xml written from it would silently drop them, so the target refuses.
function(add_regenerate_db_target type_list)
    set(sources_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    set(types_dir "${CMAKE_SOURCE_DIR}/Versions/Current/Data")
    if(NOT BUILD_EDITOR)
        add_custom_target(regenerate-db
            COMMAND ${CMAKE_COMMAND} -E echo
                    "regenerate-db needs BUILD_EDITOR=ON: types.xml collects the editor's types too"
            COMMAND ${CMAKE_COMMAND} -E false
            VERBATIM)
        return()
    endif()
    set(copy_dlls)
    if(WIN32)
        set(copy_dlls COMMAND ${CMAKE_COMMAND} -E copy_if_different
                              $<TARGET_RUNTIME_DLLS:dbcodegen> $<TARGET_FILE_DIR:dbcodegen>)
    endif()
    add_custom_target(regenerate-db
        ${copy_dlls}
        COMMAND dbcodegen -all --file-list "${type_list}" --types-path "${types_dir}"
        WORKING_DIRECTORY "${sources_dir}"
        COMMENT "Regenerating the DB*.h/.cpp sources and Versions/Current/Data/types.xml from the .cll files"
        COMMAND_EXPAND_LISTS
        VERBATIM)
endfunction()
