# The per-vertex lighting kernels are compiled into the tests that need them rather
# than reached through the 3Dmotor DLL. The tests supply their own nCubicRoot table,
# and comparing a kernel against the reference is only meaningful when both index the
# same one. It also lets a test drive refLightingKernels / sse2LightingKernels /
# avx2LightingKernels individually instead of only whatever this CPU dispatches to.
set(LIGHTING_KERNEL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexRef.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexSSE2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexAVX2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexDispatch.cpp)

# Same isolation rule as the engine build: only the AVX2 file gets /arch:AVX2.
if(MSVC)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexAVX2.cpp
        PROPERTIES COMPILE_OPTIONS "/arch:AVX2")
else()
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertexAVX2.cpp
        PROPERTIES COMPILE_OPTIONS "-mavx2")
endif()

function(add_unit_test filename)
    set(target_name ${filename}_test)
    add_executable(${target_name} EXCLUDE_FROM_ALL ${filename}.cpp ${ARGN})
    target_link_libraries(${target_name} PRIVATE GTest::gtest_main 3Dmotor Misc)
    set_target_properties(${target_name} PROPERTIES FOLDER "unittest")
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:GTest::gtest_main>
            $<TARGET_FILE_DIR:${target_name}>)
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:GTest::gtest>
            $<TARGET_FILE_DIR:${target_name}>)
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Misc>
            $<TARGET_FILE_DIR:${target_name}>)
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:MemoryLib>
            $<TARGET_FILE_DIR:${target_name}>)
endfunction()

add_unit_test(MultiplyOnColor ${LIGHTING_KERNEL_SOURCES})
add_unit_test(CalcDirectionalLighting ${LIGHTING_KERNEL_SOURCES})
add_unit_test(CalcPointLightColors ${LIGHTING_KERNEL_SOURCES})
add_unit_test(AddColors ${LIGHTING_KERNEL_SOURCES})
add_unit_test(ScaleColors ${LIGHTING_KERNEL_SOURCES})
add_unit_test(MMXTransformVector)
add_unit_test(SampleWarFogInt ${LIGHTING_KERNEL_SOURCES})
add_unit_test(SampleWarFog ${LIGHTING_KERNEL_SOURCES})
add_unit_test(MMXhelpers)
