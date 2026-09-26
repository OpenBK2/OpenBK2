#pragma once

// A Direct3D 9 shader assembler for the shader models the game uses: vs.1.1,
// ps.1.1, ps.1.4 and ps.2.0.
//
// It turns assembly text into the token stream IDirect3DDevice9::
// CreateVertexShader and CreatePixelShader take, the same tokens
// D3DXAssembleShader produces without D3DXSHADER_DEBUG, so no comment blocks.
// It exists so ShaderCompiler does not need D3DX, which is Windows only.
//
// It covers the instructions and operand forms 3Dmotor/GfxShaders.txt uses and
// the plain arithmetic around them, and rejects the rest with an error rather
// than guessing at an encoding: relative addressing, flow control, and shader
// models 1.2, 1.3, 2.x and 3.0 among them. The shaders need no labels, jumps or
// layout, so it is a single pass that turns each line into one instruction.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace NShaderAsm
{

// Assembles one shader: a version line (vs.1.1, ps.1.1, ps.1.4 or ps.2.0, with
// dots or underscores) followed by one instruction per line. ';' and '//' start
// comments. On failure returns false, leaves *pTokens empty and describes the
// first error, prefixed with its line number, in *pszError.
bool Assemble( std::string_view source, std::vector<uint32_t> *pTokens, std::string *pszError );

}
