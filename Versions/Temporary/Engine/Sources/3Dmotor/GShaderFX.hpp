#pragma once
#include <D3DX9.h>

namespace NGfx
{

// CStatesManager

class CStatesManager: public ID3DXEffectStateManager
{
private:
	LONG nRef;

public:
	CStatesManager() {}

	STDMETHOD(QueryInterface)( REFIID iid, LPVOID *ppv );
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	STDMETHOD(SetTransform)( D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX *pMatrix );
	STDMETHOD(SetMaterial)( CONST D3DMATERIAL9 *pMaterial );
	STDMETHOD(SetLight)( DWORD Index, CONST D3DLIGHT9 *pLight );
	STDMETHOD(LightEnable)( DWORD Index, BOOL Enable );
	STDMETHOD(SetRenderState)( D3DRENDERSTATETYPE State, DWORD Value );
	STDMETHOD(SetTexture)( DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture );
	STDMETHOD(SetTextureStageState)( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value );
	STDMETHOD(SetSamplerState)( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value );
	STDMETHOD(SetNPatchMode)( FLOAT NumSegments );
	STDMETHOD(SetFVF)( DWORD FVF );
	STDMETHOD(SetVertexShader)( LPDIRECT3DVERTEXSHADER9 pShader );
	STDMETHOD(SetVertexShaderConstantF)( UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount );
	STDMETHOD(SetVertexShaderConstantI)( UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount );
	STDMETHOD(SetVertexShaderConstantB)( UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount );
	STDMETHOD(SetPixelShader)( LPDIRECT3DPIXELSHADER9 pShader );
	STDMETHOD(SetPixelShaderConstantF)( UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount );
	STDMETHOD(SetPixelShaderConstantI)( UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount );
	STDMETHOD(SetPixelShaderConstantB)( UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount );
};

} // NAMESPACE

