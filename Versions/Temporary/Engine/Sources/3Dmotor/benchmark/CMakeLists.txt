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

# One target rather than three /arch: variants: each kernel set already lives in its
# own translation unit with its own flags, so building the comparison three times over
# would measure the same three implementations three times.
set(LIGHTING_KERNEL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexRef.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexSSE2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexAVX2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexDispatch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertex.cpp)
if(MSVC)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexAVX2.cpp
        PROPERTIES COMPILE_OPTIONS "/arch:AVX2")
else()
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexAVX2.cpp
        PROPERTIES COMPILE_OPTIONS "-mavx2")
endif()

add_executable(LightingKernels_benchmark EXCLUDE_FROM_ALL
    LightingKernels.cpp ${LIGHTING_KERNEL_SOURCES})
target_link_libraries(LightingKernels_benchmark PRIVATE benchmark::benchmark 3Dmotor Misc)
set_target_properties(LightingKernels_benchmark PROPERTIES
    FOLDER "benchmark"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/LightingKernels_benchmark)
add_custom_command(TARGET LightingKernels_benchmark POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:LightingKernels_benchmark>
            $<TARGET_FILE_DIR:LightingKernels_benchmark>
    COMMAND_EXPAND_LISTS
    VERBATIM)

# The MMX reference the benchmark measures against is assembled, not inline, so
# every variant has to link it.
target_link_libraries(MMXTransformVector_benchmark PRIVATE OriginalMMX)
target_link_libraries(MMXTransformVector_benchmark_avx PRIVATE OriginalMMX)
target_link_libraries(MMXTransformVector_benchmark_avx2 PRIVATE OriginalMMX)
