#include "stdafx.h"

// Before GShaderFX.h, and not optional. That header spells its handle type
// LPCSTR, which arrives with DXVK's windows.h shim, and CVertexShader holds a
// com_ptr<IDirect3DVertexShader9> whose Assign and Free call AddRef and Release,
// so the interface has to be complete and not merely forward declared.
#include <d3d9.h>

#include "GShaderFX.h"

#include "3Dmotor_export.h"

// The post-processing shaders, off Windows, where there is no D3DX.
//
// GShaderFX.cpp is the whole of this engine's programmable-shader support and it
// is built only on Windows, because every line of it is D3DX: it reads
// FX//GfxPS.fx and FX//GfxVS.fx as HLSL **source** and compiles them at run time
// through D3DXCreateEffect and D3DXCreateEffectCompiler, then drives the result
// through ID3DXEffect's technique and pass machinery.
//
// DXVK cannot supply that. What DXVK translates is compiled D3D9 shader
// bytecode, which it turns into SPIR-V itself - so if this engine loaded
// prebuilt shaders there would be nothing to do here. What is missing is D3DX,
// which is the HLSL compiler in front of that and the .fx effect runtime around
// it. Neither is part of Direct3D and neither is in DXVK.
//
// **The path is dead, three times over, and this stub is what the Windows build
// already does rather than a reduction of it:**
//
//  1. Nothing ever turns it on. InitShaderFX is reached from exactly one place,
//     GSceneInternal.cpp, inside `if ( bIsTwilight )`. bIsTwilight is set false
//     in GView.cpp and the only line that would ever have set it true,
//     `pScene->SetTwilight( rand() & 1 )`, is commented out beside it.
//     IView::SetTwilight has no caller anywhere outside 3Dmotor.
//  2. The shaders do not ship. Neither FX/GfxPS.fx nor FX/GfxVS.fx exists
//     anywhere in this repository, in the data or in a pak. The two .fx files
//     that do exist, 3Dmotor/GfxMainPS.fx and GfxMainVS.fx, are different
//     shaders under different names, are not installed, and contain no
//     `technique` block at all, so they are not even in the format
//     D3DXCreateEffect reads.
//  3. If it did run it would crash. InitShaderFX logs "Couldn't open file" for
//     each missing .fx and then returns **true** with pPSEffect and pVSEffect
//     left null, and CPixelShader's constructor dereferences pPSEffect with no
//     check.
//
// So the three effects this reaches - Monochrome, Twilight and GausianBlur in
// GPostEffects.cpp - have never run in this tree on any platform. If the .fx
// files are ever recovered, the missing pieces are an HLSL compiler targeting
// SM1-3 bytecode and an ID3DXEffect; both exist in the open, in vkd3d-shader and
// in Wine's d3dx9 respectively. That is the point to reconsider this file.

namespace NGfx
{

bool InitShaderFX()
{
	// False rather than the true the Windows path returns when its files are
	// missing, because there is no effect system here to claim otherwise. The
	// one caller ignores the result either way.
	return false;
}

void DoneShaderFX()
{
}

CPixelShader* CreatePixelShader( const std::string & )
{
	return 0;
}

CVertexShader* CreateVertexShader( const std::string & )
{
	return 0;
}

CPixelShader::~CPixelShader()
{
}

void CPixelShader::Begin()
{
}

void CPixelShader::End()
{
}

void CVertexShader::Use()
{
}

} // namespace

using namespace NGfx;
BASIC_REGISTER_CLASS( _3DMOTOR, CPixelShader )
BASIC_REGISTER_CLASS( _3DMOTOR, CVertexShader )
