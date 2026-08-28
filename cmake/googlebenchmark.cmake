include(FetchContent)

FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG eddb0241389718a23a42db6af5f0164b6e0139af
        GIT_PROGRESS TRUE
)

# Same for googlebenchmark: we want the library, not its own test suite.
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googlebenchmark)

FetchContent_GetProperties(googlebenchmark SOURCE_DIR GOOGLEBENCHMARK_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(googlebenchmark_targets ${GOOGLEBENCHMARK_SOURCE_DIR})
foreach(target IN LISTS googlebenchmark_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/GoogleBenchmark")
endforeach()
