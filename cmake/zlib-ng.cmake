include(FetchContent)

FetchContent_Declare(
        zlibng
        GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
        GIT_TAG 425439062b114a0f6cf625022c41d929c7e879f9 #refs/tags/2.2.5
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(zlibng)

FetchContent_GetProperties(zlibng SOURCE_DIR ZLIB_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(zlibng_targets ${ZLIB_SOURCE_DIR})
foreach(target IN LISTS zlibng_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/zlib-ng")
endforeach()
