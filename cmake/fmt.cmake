include(FetchContent)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG e424e3f2e607da02742f73db84873b8084fc714c
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(fmt)

FetchContent_GetProperties(fmt SOURCE_DIR FMT_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(fmt_targets ${SDL_MIXER_SOURCE_DIR})
foreach(target IN LISTS fmt_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/fmt")
endforeach()
