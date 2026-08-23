function(add_benchmark_arch filename flag suffix)
    set(target_name ${filename}_benchmark${suffix})
    add_executable(${target_name} EXCLUDE_FROM_ALL ${filename}.cpp)
    target_link_libraries(${target_name} PRIVATE benchmark::benchmark 3Dmotor Misc)
    set_target_properties(${target_name} PROPERTIES FOLDER "benchmark")
    # The whole point of the _avx/_avx2 variants is to compare ISA levels, so the
    # per-variant flag has to reach the compiler.
    target_compile_options(${target_name} PRIVATE ${flag})
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:benchmark::benchmark>
            $<TARGET_FILE_DIR:${target_name}>)
endfunction()

function(add_benchmark filename)
    add_benchmark_arch(${filename} "/arch:SSE2" "")
    add_benchmark_arch(${filename} "/arch:AVX" "_avx")
    add_benchmark_arch(${filename} "/arch:AVX2" "_avx2")
endfunction()

add_benchmark(MMXTransformVector)
