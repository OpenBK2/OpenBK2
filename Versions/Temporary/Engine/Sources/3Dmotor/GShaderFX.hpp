#pragma once

#include <D3DX9.h>

#include <cstdint>

namespace NGfx
{

// CStatesManager

class CStatesManager: public ID3DXEffectStateManager
{
private:
	long nRef;

public:
	CStatesManager() {}

	STDMETHOD(QueryInterface)( REFIID iid, LPVOID *ppv );
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	STDMETHOD(SetTransform)( D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX *pMatrix );
	STDMETHOD(SetMaterial)( CONST D3DMATERIAL9 *pMaterial );
	STDMETHOD(SetLight)( unsigned long Index, CONST D3DLIGHT9 *pLight );
	STDMETHOD(LightEnable)(unsigned long Index, int Enable );
	STDMETHOD(SetRenderState)( D3DRENDERSTATETYPE State, unsigned long Value );
	STDMETHOD(SetTexture)(unsigned long Stage, LPDIRECT3DBASETEXTURE9 pTexture );
	STDMETHOD(SetTextureStageState)(unsigned long Stage, D3DTEXTURESTAGESTATETYPE Type, unsigned long Value );
	STDMETHOD(SetSamplerState)(unsigned long Sampler, D3DSAMPLERSTATETYPE Type, unsigned long Value );
	STDMETHOD(SetNPatchMode)( FLOAT NumSegments );
	STDMETHOD(SetFVF)(unsigned long FVF );
	STDMETHOD(SetVertexShader)( LPDIRECT3DVERTEXSHADER9 pShader );
	STDMETHOD(SetVertexShaderConstantF)( unsigned RegisterIndex, CONST FLOAT *pConstantData, unsigned RegisterCount );
	STDMETHOD(SetVertexShaderConstantI)( unsigned RegisterIndex, CONST INT *pConstantData, unsigned RegisterCount );
	STDMETHOD(SetVertexShaderConstantB)( unsigned RegisterIndex, CONST int *pConstantData, unsigned RegisterCount );
	STDMETHOD(SetPixelShader)( LPDIRECT3DPIXELSHADER9 pShader );
	STDMETHOD(SetPixelShaderConstantF)( unsigned RegisterIndex, CONST FLOAT *pConstantData, unsigned RegisterCount );
	STDMETHOD(SetPixelShaderConstantI)( unsigned RegisterIndex, CONST INT *pConstantData, unsigned RegisterCount );
	STDMETHOD(SetPixelShaderConstantB)( unsigned RegisterIndex, CONST int *pConstantData, unsigned RegisterCount );
};

} // NAMESPACE


