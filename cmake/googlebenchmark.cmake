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
# Google Benchmark compiles itself with -Werror by default, which makes any
# warning the compiler decides to emit into a failure of this project's build.
# GCC 15 with AddressSanitizer reports maybe-uninitialized inside libstdc++'s
# own std_function.h, from std::regex internals, and that stopped an ASan tree
# from building at all. Nothing here can fix a warning in a system header.
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googlebenchmark)

FetchContent_GetProperties(googlebenchmark SOURCE_DIR GOOGLEBENCHMARK_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(googlebenchmark_targets ${GOOGLEBENCHMARK_SOURCE_DIR})
foreach(target IN LISTS googlebenchmark_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/GoogleBenchmark")

    # BENCHMARK_ENABLE_WERROR above does not cover all of it: Google Benchmark
    # also compiles itself with -pedantic-errors, which turns every pedantic
    # diagnostic into an error on its own, whatever -Werror is set to. Each new
    # compiler release can therefore stop this project building. clang 22 added
    # -Wc2y-extensions, which fires on the __COUNTER__ probe in benchmark.h and
    # breaks the library and every benchmark target with it.
    #
    # -Wno-pedantic is on the target rather than the diagnostic because the
    # pattern repeats, and target options come after CMAKE_CXX_FLAGS on the
    # command line, which is where -pedantic-errors was appended. Both GCC and
    # clang know the flag, so this needs no compiler detection.
    get_target_property(target_type ${target} TYPE)
    if(NOT target_type STREQUAL "INTERFACE_LIBRARY" AND NOT MSVC)
        target_compile_options(${target} PRIVATE -Wno-pedantic)
    endif()
endforeach()
