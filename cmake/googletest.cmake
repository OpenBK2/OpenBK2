include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG de1c60926218478959040ad79be8b2dbed22bd2b
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(googletest)

FetchContent_GetProperties(googletest SOURCE_DIR GOOGLETEST_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(googletest_targets ${GOOGLETEST_SOURCE_DIR})
foreach(target IN LISTS googletest_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/GoogleTest")
endforeach()
