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
