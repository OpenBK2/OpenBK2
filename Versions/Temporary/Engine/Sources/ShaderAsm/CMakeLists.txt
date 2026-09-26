project(ShaderAsm)

# A Direct3D 9 shader assembler for vs.1.1, ps.1.1, ps.1.4 and ps.2.0, which
# produces the same tokens as D3DXAssembleShader for every shader the game has.
# ShaderCompiler assembles with it so that it needs no D3DX and builds on every
# platform. See ShaderAsm.h.
#
# Static rather than the SHARED library with an export header that every engine
# subsystem is: one function, no engine dependencies, and one user.
add_library(ShaderAsm STATIC
    ShaderAsm.cpp
    ShaderAsm.h
)
target_include_directories(ShaderAsm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ShaderAsm PUBLIC cxx_std_17)

# unit tests, built on request: GfxShadersCorpus_test
add_subdirectory(test)
