project(ShaderCompiler)

# The shader compiler. It reads 3Dmotor/GfxShaders.txt, a vertex shader macro
# language (proc/func/locals expanded into vs.1.1 assembly), pixel shader
# assembly with the render and texture stage states each shader wants, and an
# [HLSL] section naming GfxMainVS.fx and GfxMainPS.fx. It assembles and compiles
# all of it into D3D9 bytecode and writes GfxShaders.cpp and GfxShaders.h, the
# arrays GfxRender.cpp hands to CreatePixelShader and CreateVertexShader.
#
# The original 3Dmotor project ran it as a custom build step, from a network
# share, and had the step excluded from every configuration, so the generated
# files are committed and the game build never needs this. It is here so that
# a change to GfxShaders.txt can be turned into bytecode again.
#
# Windows only: the assembling and compiling are D3DXAssembleShader and
# D3DXCompileShaderFromFile, and there is no D3DX anywhere else. The bytecode it
# produces is platform neutral, and on Linux DXVK consumes the committed arrays.
#
# A plain executable, not the SHARED library with an export header that every
# engine subsystem is: nothing links a tool, so it exports nothing.
set(SOURCES
    Data.cpp
    hlsl.cpp
    output.cpp
    parser.cpp
    ShaderCompiler.cpp
    Streams.cpp
    stdafx.cpp
)
add_executable(ShaderCompiler ${SOURCES})
add_stdafx_pch(ShaderCompiler)

# Self-contained: none of the engine's libraries, only D3DX and the portability
# headers under port/, which need Boost.Predef.
target_link_libraries(ShaderCompiler PRIVATE dxsdk::d3dx9 dxsdk::d3d9 Boost::predef)

# The CRT allocator rather than the project-wide mimalloc. No heap object ever
# crosses into an engine module, since it links none, and without mimalloc.dll
# beside it the tool runs straight from the build tree, which is where the
# regenerate-gfxshaders target below runs it.
set_target_properties(ShaderCompiler PROPERTIES OBK2_USE_SYSTEM_ALLOCATOR TRUE)

install(TARGETS ShaderCompiler DESTINATION bin)

# Regenerates the committed 3Dmotor/GfxShaders.cpp and GfxShaders.h in the
# source tree. Never part of ALL: run it by hand after editing GfxShaders.txt,
# GfxMainVS.fx or GfxMainPS.fx, and review the diff.
#
# It runs in 3Dmotor/ because the tool resolves everything against the working
# directory: the .fx files the [HLSL] section names, the temp.fx and tempVS.fx
# it writes and deletes while compiling, and the output name, which is also
# what the generated .cpp #includes.
set(SHADERCOMPILER_3DMOTOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../3Dmotor")
add_custom_target(regenerate-gfxshaders
    COMMAND ShaderCompiler GfxShaders.txt GfxShaders
    WORKING_DIRECTORY "${SHADERCOMPILER_3DMOTOR_DIR}"
    COMMENT "Regenerating 3Dmotor/GfxShaders.cpp and GfxShaders.h from GfxShaders.txt"
    VERBATIM
)
