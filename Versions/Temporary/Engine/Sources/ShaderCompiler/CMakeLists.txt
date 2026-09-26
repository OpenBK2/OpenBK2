project(ShaderCompiler)

# The shader compiler. It reads 3Dmotor/GfxShaders.txt, a vertex shader macro
# language (proc/func/locals expanded into vs.1.1 assembly) and pixel shader
# assembly with the render and texture stage states each shader wants. It
# assembles all of it into D3D9 bytecode and writes GfxShaders.cpp and
# GfxShaders.h, the arrays GfxRender.cpp hands to CreatePixelShader and
# CreateVertexShader.
#
# The original 3Dmotor project ran it as a custom build step, from a network
# share, and had the step excluded from every configuration, so the generated
# files are committed and the game build never needs this. It is here so that
# a change to GfxShaders.txt can be turned into bytecode again.
#
# The assembling is ShaderAsm's, which matches D3DXAssembleShader token for
# token, so the tool builds and runs everywhere. On Windows, where the DirectX
# runtime is installed, it also assembles every shader with D3DX and fails on
# any difference or on anything D3DX refuses, because D3DX validates the
# program and ShaderAsm does not. It also compiled HLSL once, for an [HLSL]
# section with no consumer; that support went with the section.
#
# A plain executable, not the SHARED library with an export header that every
# engine subsystem is: nothing links a tool, so it exports nothing.
set(SOURCES
    Data.cpp
    output.cpp
    parser.cpp
    ShaderCompiler.cpp
    stdafx.cpp
)
add_executable(ShaderCompiler ${SOURCES})
add_stdafx_pch(ShaderCompiler)

# Self-contained: none of the engine's libraries, only ShaderAsm and the
# portability headers under port/, which need Boost.Predef.
target_link_libraries(ShaderCompiler PRIVATE ShaderAsm Boost::predef)

# The D3DX check loads d3dx9_43.dll at run time, so only the SDK's headers are
# needed here and the tool runs without the DirectX runtime, unchecked.
if(WIN32)
    target_include_directories(ShaderCompiler PRIVATE ${DXSDK_INCLUDE})
    target_compile_definitions(ShaderCompiler PRIVATE SHADERCOMPILER_WITH_D3DX=1)
endif()

# The CRT allocator rather than the project-wide mimalloc. No heap object ever
# crosses into an engine module, since it links none, and without mimalloc.dll
# beside it the tool runs straight from the build tree, which is where the
# regenerate-gfxshaders target below runs it.
set_target_properties(ShaderCompiler PROPERTIES OBK2_USE_SYSTEM_ALLOCATOR TRUE)

install(TARGETS ShaderCompiler DESTINATION bin)

# Regenerates the committed 3Dmotor/GfxShaders.cpp and GfxShaders.h in the
# source tree. Never part of ALL: run it by hand after editing GfxShaders.txt,
# and review the diff. Run it on Windows where possible, for the D3DX check.
#
# It runs in 3Dmotor/ because the tool resolves its input and output names
# against the working directory, and the output name is also what the generated
# .cpp #includes.
set(SHADERCOMPILER_3DMOTOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../3Dmotor")
add_custom_target(regenerate-gfxshaders
    COMMAND ShaderCompiler GfxShaders.txt GfxShaders
    WORKING_DIRECTORY "${SHADERCOMPILER_3DMOTOR_DIR}"
    COMMENT "Regenerating 3Dmotor/GfxShaders.cpp and GfxShaders.h from GfxShaders.txt"
    VERBATIM
)
