add_unit_test(GfxShadersCorpus ShaderAsm)
# every game shader's source and the tokens D3DX assembled it to
target_compile_definitions(GfxShadersCorpus_test PRIVATE
    GFXSHADERS_CORPUS="${CMAKE_CURRENT_SOURCE_DIR}/GfxShaders.corpus")

# Generated shaders assembled by both ShaderAsm and D3DX, which only Windows has.
# D3DX is loaded at run time, so only its headers are needed to build.
if(WIN32)
    add_unit_test(ShaderAsmVsD3DX ShaderAsm)
    target_include_directories(ShaderAsmVsD3DX_test PRIVATE ${DXSDK_INCLUDE})
endif()
