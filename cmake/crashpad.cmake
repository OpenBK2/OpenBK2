include(FetchContent)

FetchContent_Declare(
        crashpad
        GIT_REPOSITORY https://github.com/getsentry/crashpad.git
        GIT_TAG 60dd8995c6a8539718c878f9b41063604abe737c
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(crashpad)

FetchContent_GetProperties(crashpad SOURCE_DIR CRASHPAD_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(crashpad_targets ${CRASHPAD_SOURCE_DIR})
foreach(target IN LISTS crashpad_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/crashpad")
endforeach()

# Both installed and build-tree launches resolve the handler beside the binary.
function(configure_crashpad target)
    target_link_libraries(${target} PRIVATE crashpad_client)
    add_dependencies(${target} crashpad_handler)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:crashpad_handler> $<TARGET_FILE_DIR:${target}>
        COMMENT "Copying Crashpad handler beside ${target}"
        VERBATIM)
    install(PROGRAMS $<TARGET_FILE:crashpad_handler> DESTINATION bin)
endfunction()
