#pragma once
#include <D3DX9.h>
#include "../Misc/Win32Helper.h"

typedef LPCSTR D3DXHANDLE;
struct IDirect3DVertexShader9;

namespace NGfx
{

// CPixelShader

class CPixelShader: public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CPixelShader)
private:
	bool bBegin;
	D3DXHANDLE hTechnique;

public:
	CPixelShader() {}
	CPixelShader( const string &szName );
	~CPixelShader();

	void Begin();
	void End();
};

// CVertexShader

class CVertexShader: public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CVertexShader)
private:
	NWin32Helper::com_ptr<IDirect3DVertexShader9> pShader;

public:
	CVertexShader() {}
	CVertexShader( const string &szName );

	void Use();
};

bool InitShaderFX();
void DoneShaderFX();
CPixelShader* CreatePixelShader( const string &szName );
CVertexShader* CreateVertexShader( const string &szName );

} // NAMESPACE


