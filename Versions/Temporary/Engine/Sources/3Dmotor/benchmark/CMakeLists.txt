function(add_benchmark_arch filename flag suffix)
    set(target_name ${filename}_benchmark${suffix})
    add_executable(${target_name} EXCLUDE_FROM_ALL ${filename}.cpp)
    target_link_libraries(${target_name} PRIVATE benchmark::benchmark 3Dmotor Misc)
    # Each variant gets its own directory. All three used to land in this one and copy
    # the same runtime DLLs into it concurrently, which raced and failed the build.
    set_target_properties(${target_name} PROPERTIES
        FOLDER "benchmark"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target_name})
    # The whole point of the _avx/_avx2 variants is to compare ISA levels, so the
    # per-variant flag has to reach the compiler.
    target_compile_options(${target_name} PRIVATE ${flag})
    # Copying only googlebenchmark's DLL left the executable unable to start once it
    # linked engine modules: it died with 0xC0000135, DLL not found. TARGET_RUNTIME_DLLS
    # reads the whole link closure, the same way cmake/unittest.cmake does.
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:${target_name}> $<TARGET_FILE_DIR:${target_name}>
            COMMAND_EXPAND_LISTS
            VERBATIM)
endfunction()

function(add_benchmark filename)
    add_benchmark_arch(${filename} "/arch:SSE2" "")
    add_benchmark_arch(${filename} "/arch:AVX" "_avx")
    add_benchmark_arch(${filename} "/arch:AVX2" "_avx2")
endfunction()

add_benchmark(MMXTransformVector)

# The MMX reference the benchmark measures against is assembled, not inline, so
# every variant has to link it.
target_link_libraries(MMXTransformVector_benchmark PRIVATE OriginalMMX)
target_link_libraries(MMXTransformVector_benchmark_avx PRIVATE OriginalMMX)
target_link_libraries(MMXTransformVector_benchmark_avx2 PRIVATE OriginalMMX)
