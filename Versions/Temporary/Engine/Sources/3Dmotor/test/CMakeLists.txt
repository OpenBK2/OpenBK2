add_subdirectory(original)

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

# These still compare against the inline __asm in original.h and test/mmx.h, which
# MSVC only accepts on x86. Registering them on x64 would give ctest a list of tests it
# can never run. They come back to both architectures as their references move to MASM
# under original/, the way CalcDirectionalLighting and MMXTransformVector already have.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "3Dmotor tests: skipping the inline-__asm ones, x86 only")
else()
    add_unit_test(MultiplyOnColor 3Dmotor Misc SOURCES ${LIGHTING_KERNEL_SOURCES})
    add_unit_test(CalcPointLightColors 3Dmotor Misc SOURCES ${LIGHTING_KERNEL_SOURCES})
    add_unit_test(AddColors 3Dmotor Misc SOURCES ${LIGHTING_KERNEL_SOURCES})
    add_unit_test(ScaleColors 3Dmotor Misc SOURCES ${LIGHTING_KERNEL_SOURCES})
    add_unit_test(SampleWarFogInt 3Dmotor Misc SOURCES ${LIGHTING_KERNEL_SOURCES})
    add_unit_test(SampleWarFog 3Dmotor Misc SOURCES ${LIGHTING_KERNEL_SOURCES})
    add_unit_test(MMXhelpers 3Dmotor Misc)
endif()

# Both architectures: their MMX references are assembled, not inline.
add_unit_test(CalcDirectionalLighting 3Dmotor Misc OriginalMMX SOURCES ${LIGHTING_KERNEL_SOURCES})
# Drives refLightingKernels / sse2LightingKernels / avx2LightingKernels directly,
# rather than whatever GetLightingKernels() picks, so the sets the running CPU is
# not dispatching to still get checked.
# GLightPerVertex.cpp comes along for SPerVertexLightState's constructor, which the
# kernels take by reference and which the 3Dmotor DLL does not export.
add_unit_test(LightingKernels 3Dmotor Misc SOURCES ${LIGHTING_KERNEL_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/../GLightPerVertex.cpp)
add_unit_test(MMXTransformVector 3Dmotor Misc OriginalMMX)
