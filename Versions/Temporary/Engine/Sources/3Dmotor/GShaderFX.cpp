#include "StdAfx.h"
#include <D3D9.h>
#include <D3DX9.h>
#include "GfxInternal.h"
#include "../System/VFSOperations.h"

////
#include "GShaderFX.h"
#include "GShaderFX.hpp"
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dxguid.lib")
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGfx
{
////////////////////////////////////////////////////////////////////////////////////////////////////
static float fPixelShaderMaxVersion = 2.0;
static const int N_MAX_TECHNIQUES = 5;
static NWin32Helper::com_ptr<CStatesManager> pManager;
static NWin32Helper::com_ptr<ID3DXEffect> pPSEffect;
static NWin32Helper::com_ptr<ID3DXEffectCompiler> pVSEffect;
////////////////////////////////////////////////////////////////////////////////////////////////////
bool InitShaderFX()
{
	if(pManager) return true;

	pManager = new CStatesManager;
	HRESULT hRes;

	string szFileName = "FX//GfxPS.fx";
	if ( NVFS::GetMainVFS()->DoesFileExist( szFileName ) )
	{
		CFileStream stream( NVFS::GetMainVFS(), szFileName );
		if ( stream.IsOk() )
		{
			int n = stream.GetSize();

			string sResult;
			  sResult.resize( n );
			stream.Read( &sResult[0], n );
			
			NWin32Helper::com_ptr<ID3DXBuffer> pCompilationErrors;
			hRes = D3DXCreateEffect( pDevice, sResult.c_str(), n, 0, 0, 0, 0, pPSEffect.GetAddr(), pCompilationErrors.GetAddr() );
			if ( FAILED( hRes ) )
			{
				IDirect3DDevice9 *pdev=pDevice;

				if ( pCompilationErrors )
					csSystem << (char *) pCompilationErrors->GetBufferPointer() << endl;

				return false;
			}
			else
				csSystem << "GfxPS.fx was compiled" << endl;

		}
	}
	else
		csSystem << CC_ORANGE << "Couldn't open file " << szFileName << endl;

	szFileName = "FX//GfxVS.fx";
	if ( NVFS::GetMainVFS()->DoesFileExist( szFileName ) )
	{
		CFileStream stream( NVFS::GetMainVFS(), szFileName );
		if ( stream.IsOk() )
		{
			int n = stream.GetSize();

			string sResult;
			sResult.resize( n );
			stream.Read( &sResult[0], n );

			NWin32Helper::com_ptr<ID3DXBuffer> pCompilationErrors;
		
			hRes = D3DXCreateEffectCompiler( sResult.c_str(), n,
				0, 0, 0, pVSEffect.GetAddr(), pCompilationErrors.GetAddr() );
	

			if ( FAILED( hRes ) )
			{
				IDirect3DDevice9 *pdev=pDevice;

				if ( pCompilationErrors )
					csSystem << (char *) pCompilationErrors->GetBufferPointer() << endl;

				return false;
			}
			else
			{
				csSystem << "GfxVS.fx was compiled" << endl;

			}

		}
	}
	else
		csSystem << CC_ORANGE << "Couldn't open file " << szFileName << endl;
	
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoneShaderFX()
{
	pManager = 0;

	pPSEffect = 0;
	pVSEffect = 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CPixelShader* CreatePixelShader( const string &szName )
{
	return new CPixelShader( szName );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CVertexShader* CreateVertexShader( const string &szName )
{
	return new CVertexShader( szName );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CEffect
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// форматирование строки
const char* __cdecl Format( const char *pszFormat, ... )
{
	static char buff[2048];
	va_list va;
	// 
	va_start( va, pszFormat );
	vsprintf( buff, pszFormat, va );
	va_end( va );
	//
	return buff;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

CPixelShader::CPixelShader( const string &szName ):
	bBegin( false ), hTechnique( 0 )
{
	DWORD dwMaxVersion = 0;
	if ( fPixelShaderMaxVersion > 0 )
	{
		int nTemp = fPixelShaderMaxVersion * 10;
		int nMinor = nTemp % 10;
		nTemp = nTemp / 10;
		int nMajor = nTemp % 10;
		dwMaxVersion = D3DPS_VERSION( nMajor, nMinor );
	}

	vector<D3DXHANDLE> handles;
	for ( int nTemp = 0; nTemp < N_MAX_TECHNIQUES; ++nTemp )
	{
		string szID( szName );
		if ( nTemp > 0)
			szID = Format( "%s_%d", szName.c_str(), nTemp );

		D3DXHANDLE hHandle = pPSEffect->GetTechniqueByName( szID.c_str() );
		if ( hHandle == NULL )
			continue;
		HRESULT hRes = pPSEffect->ValidateTechnique( hHandle );
		if ( FAILED( hRes ) )
			continue;

		if ( dwMaxVersion != 0 )
		{
			D3DXPASS_DESC passDesc;
			D3DXHANDLE hPass = pPSEffect->GetPass( hHandle, 0 );
			ASSERT( hPass != NULL );
			hRes = pPSEffect->GetPassDesc( hPass, &passDesc );
			ASSERT( SUCCEEDED( hRes ) );

			DWORD dwVer = D3DXGetShaderVersion( passDesc.pPixelShaderFunction );
			if ( dwVer > dwMaxVersion )
				continue;
		}

		handles.push_back( hHandle );
	}

	if ( handles.empty() )
	{
		ASSERT( 0 );
		return;
	}

	hTechnique = handles.back();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CPixelShader::~CPixelShader()
{
	End();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CPixelShader::Begin()
{
	ASSERT( !bBegin );
	bBegin = true;

	pPSEffect->SetTechnique( hTechnique );

	UINT nPasses;
	HRESULT hRes = pPSEffect->Begin( &nPasses, 0 );
	ASSERT( nPasses == 1 );
	ASSERT( SUCCEEDED( hRes ) );
	hRes = pPSEffect->BeginPass( 0 );
	ASSERT( SUCCEEDED( hRes ) );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CPixelShader::End()
{
	if ( !bBegin )
		return;

	bBegin = false;
	pPSEffect->EndPass();
	pPSEffect->End();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CVertexShader
////////////////////////////////////////////////////////////////////////////////////////////////////
CVertexShader::CVertexShader( const string &szName )
{
	D3DXHANDLE hFnc = pVSEffect->GetFunctionByName( szName.c_str() );

	NWin32Helper::com_ptr<ID3DXBuffer> pBuffer;
	NWin32Helper::com_ptr<ID3DXBuffer> pCompilationErrors;
	HRESULT hRes = pVSEffect->CompileShader( hFnc, "vs_1_1", 0, pBuffer.GetAddr(), pCompilationErrors.GetAddr(), 0 );
	if ( FAILED( hRes ) )
	{
		if ( pCompilationErrors )
			DebugTrace( "Compilation error: %s\n", pCompilationErrors->GetBufferPointer() );
		ASSERT( 0 );
		return;
	}

	pDevice->CreateVertexShader( (DWORD*)pBuffer->GetBufferPointer(), pShader.GetAddr() );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CVertexShader::Use()
{
	pDevice->SetVertexShader( pShader );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CStatesManager
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::QueryInterface( REFIID iid, LPVOID *ppv )
{
	if ( ( iid != IID_IUnknown ) && ( iid != IID_ID3DXEffectStateManager ) )
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
        
	*ppv = static_cast<ID3DXEffectStateManager*>(this);
	reinterpret_cast<IUnknown*>(this)->AddRef();
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG CStatesManager::AddRef()
{
	return (ULONG)InterlockedIncrement( &nRef );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG CStatesManager::Release()
{
	if( 0 == InterlockedDecrement( &nRef ) )
	{
		delete this;
		return 0;
	}

	return nRef;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetTransform( D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX *pMatrix )
{
	ASSERT( 0 );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetMaterial( CONST D3DMATERIAL9 *pMaterial )
{
	ASSERT( 0 );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetLight( DWORD Index, CONST D3DLIGHT9 *pLight )
{
	ASSERT( 0 );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::LightEnable( DWORD Index, BOOL Enable )
{
	ASSERT( 0 );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetRenderState( D3DRENDERSTATETYPE State, DWORD Value )
{
	pDevice->SetRenderState( State, Value );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetTexture( DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture )
{
	pDevice->SetTexture( Stage, pTexture );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetTextureStageState( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value )
{
	pDevice->SetTextureStageState( Stage, Type, Value );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetSamplerState( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value )
{
	pDevice->SetSamplerState( Sampler, Type, Value );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetNPatchMode( FLOAT NumSegments )
{
	ASSERT( 0 );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetFVF( DWORD FVF )
{
	ASSERT( 0 );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetVertexShader( LPDIRECT3DVERTEXSHADER9 pShader )
{
	pDevice->SetVertexShader( pShader );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetVertexShaderConstantF( UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount )
{
	pDevice->SetVertexShaderConstantF( RegisterIndex, pConstantData, RegisterCount );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetVertexShaderConstantI( UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount )
{
	pDevice->SetVertexShaderConstantI( RegisterIndex, pConstantData, RegisterCount );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetVertexShaderConstantB( UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount )
{
	pDevice->SetVertexShaderConstantB( RegisterIndex, pConstantData, RegisterCount );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetPixelShader( LPDIRECT3DPIXELSHADER9 pShader )
{
	pDevice->SetPixelShader( pShader );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetPixelShaderConstantF( UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount )
{
//	pDevice->SetPixelShaderConstantF( RegisterIndex, pConstantData, RegisterCount );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetPixelShaderConstantI( UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount )
{
//	pDevice->SetPixelShaderConstantI( RegisterIndex, pConstantData, RegisterCount );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CStatesManager::SetPixelShaderConstantB( UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount )
{
//	pDevice->SetPixelShaderConstantB( RegisterIndex, pConstantData, RegisterCount );
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace NGfx;
BASIC_REGISTER_CLASS( CPixelShader )
BASIC_REGISTER_CLASS( CVertexShader )
////////////////////////////////////////////////////////////////////////////////////////////////////

