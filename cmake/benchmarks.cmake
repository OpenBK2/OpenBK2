# Registration for benchmark executables, so nothing downstream has to name them.
#
#   register_benchmark(<target>)
#
# adds the target to the aggregate "benchmarks" target and registers it with ctest
# under the "obk2-benchmark" label. A new benchmark is then picked up by
#
#   cmake --build <dir> --target benchmarks
#   ctest --test-dir <dir> -L obk2-benchmark
#
# without editing CI, which is the point: a list of executables in a workflow file
# goes stale the first time someone adds one.
#
# The JSON output path is baked in because it has to be predictable for the report to
# find it. Everything else about how the benchmark runs is left to Google Benchmark's
# environment variables - BENCHMARK_MIN_TIME, BENCHMARK_REPETITIONS and the rest - so
# a caller can trade accuracy for wall time without this file knowing about it.

set(BENCHMARK_RESULTS_DIR ${CMAKE_BINARY_DIR}/benchmark-results
    CACHE INTERNAL "where register_benchmark tells benchmarks to write their JSON")
file(MAKE_DIRECTORY ${BENCHMARK_RESULTS_DIR})

function(register_benchmark target_name)
    if(NOT TARGET benchmarks)
        # No ALL: benchmarks are opt in, like the tests.
        add_custom_target(benchmarks
            COMMENT "every executable registered with register_benchmark")
    endif()
    add_dependencies(benchmarks ${target_name})

    add_test(NAME ${target_name} COMMAND ${target_name}
        --benchmark_out=${BENCHMARK_RESULTS_DIR}/${target_name}.json
        --benchmark_out_format=json)
    set_tests_properties(${target_name} PROPERTIES LABELS "obk2-benchmark")
endfunction()
