set(ARCHITECTURE "NONE" CACHE STRING "CPU architecture")
set_property(CACHE ARCHITECTURE PROPERTY STRINGS NONE SSE2 AVX AVX2 AVX512)

# Never fuse a multiply and an add into one FMA. Lockstep multiplayer and
# replays need every client to compute bit-identical floats, and a fused
# multiply-add rounds once where the source rounds twice, so whether it is
# used must not depend on the compiler or the target. gcc defaults to
# -ffp-contract=fast, which is harmless only while the x86-64 baseline has no
# FMA instruction; any -march that has one would start fusing the simulation.
# Whole tree rather than the simulation modules alone, so that nothing the
# simulation calls is ever left out. MSVC needs nothing: it fuses only under
# /fp:contract or /fp:fast, and neither is set (AILogic builds /fp:strict).
if(NOT MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:-ffp-contract=off>")
endif()

if(MSVC)
    if(ARCHITECTURE STREQUAL "SSE2")
        add_compile_options("/arch:SSE2")
    elseif(ARCHITECTURE STREQUAL "AVX")
        add_compile_options("/arch:AVX")
    elseif(ARCHITECTURE STREQUAL "AVX2")
        add_compile_options("/arch:AVX2")
    elseif(ARCHITECTURE STREQUAL "AVX512")
        add_compile_options("/arch:AVX512")
    endif()
endif()
