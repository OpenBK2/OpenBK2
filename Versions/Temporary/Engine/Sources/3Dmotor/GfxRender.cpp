#include "stdafx.h"
#include <D3D9.h>
#include "GfxRender.h"
#include "GfxInternal.h"
#include "GfxBuffers.h"
#include "Gfx.h"
#include "GfxShaders.h"
#include "GfxShadersDescr.h"
#include "System/Commands.h"
#include "GShaderFX.h"

namespace NGfx
{
extern SRenderTargetsInfo rtInfo;
class CQuery : public IQuery
{
	OBJECT_NOCOPY_METHODS(CQuery);
	NWin32Helper::com_ptr<IDirect3DQuery9> pQuery;
public:
	CQuery() {}
	CQuery( IDirect3DQuery9 *p ) : pQuery(p) {}
	virtual void Start() { pQuery->Issue( D3DISSUE_BEGIN ); }
	virtual void Finish() { pQuery->Issue( D3DISSUE_END ); }
	virtual int GetData() 
	{
		int nRes = 10000;
		for (;;)
		{
			HRESULT hr = pQuery->GetData( &nRes, sizeof(nRes), D3DGETDATA_FLUSH );
			if ( S_FALSE != hr )
				break;
		}
		return nRes;
	}
	virtual void Flush() { int nRes; pQuery->GetData( &nRes, sizeof(nRes), D3DGETDATA_FLUSH ); }
};
static bool operator==( const SFBTransform &a, const SFBTransform &b )
{
	return memcmp( &a, &b, sizeof(SFBTransform) ) == 0;
}
template<class T>
void Apply( const T &a );
template<class T>
struct SRenderParam
{
	T value;
	SRenderParam( const T &_a ): value(_a) {}
	void Set( const T &_a ) { if ( _a == value ) return; value = _a; Apply( _a ); }
	void DoApply() { Apply( value ); }
};
typedef std::unordered_map<int, NWin32Helper::com_ptr<IDirect3DSurface9> > CDepthHash;

const int N_MAX_REGISTERS =	5;
const int N_MAX_ALLOWED_LAG = 10;
const int N_MAX_SHADERS_COUNT = 256;
static int nScreenRegisters;
bool bDoValidateDevice = false;
static SRenderParam<EDithering> ditheringMode( DITHER_ON );
static SRenderParam<EWireframe> wireframeMode( WIREFRAME_OFF );
static SRenderParam<EAlphaCombineMode> alphaMode( COMBINE_NONE );
static SRenderParam<SStencilMode> stencilMode( SStencilMode( STENCIL_NONE, 0, 0xffffffff ) );
static SRenderParam<EColorWriteMask> colorMode( COLORWRITE_ALL );
static SRenderParam<EDepthMode> depthMode( DEPTH_NORMAL );
static SRenderParam<ECullMode> cullMode( CULL_CW );
static SRenderParam<SFogParams> fogParamsMode = SRenderParam<SFogParams>( SFogParams() );
static int nFogModeColor;
static SRenderParam<EFogMode> fogMode( FOG_NONE );
static SRenderParam<SFBTransform> transformMode = SFBTransform();
static NWin32Helper::com_ptr<IDirect3DSurface9> pScreenColor, pScreenDepth, pRegisterDepth;
static CDepthHash sharedZBuffers;
static CMObj<CTexture> pRegisters[N_MAX_REGISTERS];
static CMObj<CTexture> pDepthRegister;
static CTPoint<int> ptRegisterBufferSize, ptScreenSize;
static EFilterMode TSFilterMode[8];
static EWrap lastUsedAddressMode[8];
static NWin32Helper::com_ptr<IDirect3DPixelShader9> pixelShaders[ N_MAX_SHADERS_COUNT ];
static NWin32Helper::com_ptr<IDirect3DVertexShader9> vertexShaders[ N_MAX_SHADERS_COUNT ];
static NWin32Helper::com_ptr<IDirect3DVertexDeclaration9> vertexDeclarations[200];
static int nLastUsedVShader, nLastUsedVDeclaration;
static const SPShader *pCurrentPixelShader;
const int N_RENDER_STATES = 210;
const int N_TSS_STATES = 33;
const int N_SAMPLER_STATES = 14;
static DWORD renderStates[210], tssStates[8][33], samplerStates[8][14];
static std::vector<CMObj<CQuery> > queries;
static bool bDoesSupportOcclusionQueries = false, bDoesSupportEventQueries = false;
static std::list<CObj<IQuery> > lagQueries;
static int nMaxLag = 1;
static float fRegisterResolution = 1;

typedef std::unordered_map<std::string, CObj<CPixelShader> > TPixelShaders;
typedef std::unordered_map<std::string, CObj<CVertexShader> > TVertexShaders;
static TPixelShaders hlslPixelShaders;
static TVertexShaders hlslVertexShaders;

struct SVideoCardType
{
	DWORD dwVendorID;
	DWORD dwDeviceID;
	DWORD dwDeviceIDMask;
	EVideoCard eType;
};
#define VENDOR_NV 0x000010DE
#define VENDOR_ATI 0x00001002
static SVideoCardType videoCardsArray[] =
{
	{ VENDOR_NV, 0x0100, 0x0FF0, VC_GEFORCE1 },
	{ VENDOR_NV, 0x0110, 0x0FF0, VC_GEFORCE2MX },
	{ VENDOR_NV, 0x01A0, 0x0FF0, VC_GEFORCE2MX }, // nForce
	{ VENDOR_NV, 0x0150, 0x0FF0, VC_GEFORCE2 },
	{ VENDOR_NV, 0x0170, 0x0FF0, VC_GEFORCE4MX },
	{ VENDOR_NV, 0x0200, 0x0FF0, VC_GEFORCE3 },
	{ VENDOR_NV, 0x0250, 0x0FF0, VC_GEFORCE4 },
	{ VENDOR_NV, 0x0280, 0x0FF0, VC_GEFORCE4 }, //// AGP 8x
	{ VENDOR_NV, 0x0300, 0x0FF0, VC_GEFORCEFX_FAST },
	{ VENDOR_NV, 0x0310, 0x0FF0, VC_GEFORCEFX_MID },
	{ VENDOR_NV, 0x0320, 0x0FF0, VC_GEFORCEFX_SLOW },
	{ VENDOR_NV, 0x0330, 0x0FF0, VC_GEFORCEFX_FAST },
	{ VENDOR_NV, 0x0342, 0x0FF2, VC_GEFORCEFX_LE },
	{ VENDOR_NV, 0x0340, 0x0FF0, VC_GEFORCEFX_FAST },
	/////
	{ VENDOR_ATI, 0x00005159, 0xFFFF, VC_RADEON7X00 },
	{ VENDOR_ATI, 0x00005144, 0xFFFF, VC_RADEON7X00 },
	{ VENDOR_ATI, 0x00005157, 0xFFFF, VC_RADEON7X00 },
	{ VENDOR_ATI, 0x00004900, 0xFF00, VC_RADEON9000 },
	{ VENDOR_ATI, 0x0000514C, 0xFFFF, VC_RADEON9100 },
	{ VENDOR_ATI, 0x00005940, 0xFFF0, VC_RADEON9200 },
	{ VENDOR_ATI, 0x00005960, 0xFFF0, VC_RADEON9200 },
	{ VENDOR_ATI, 0x00004144, 0xFFFF, VC_RADEON9500 },
	{ VENDOR_ATI, 0x00004164, 0xFFFF, VC_RADEON9500 },
	{ VENDOR_ATI, 0x00004E45, 0xFFFF, VC_RADEON9500 },
	{ VENDOR_ATI, 0x00004E65, 0xFFFF, VC_RADEON9500 },
	{ VENDOR_ATI, 0x00004151, 0xFFF1, VC_RADEON9600SE },
	{ VENDOR_ATI, 0x00004150, 0xFFF0, VC_RADEON9600 },
	{ VENDOR_ATI, 0x00004171, 0xFFF1, VC_RADEON9600SE },
	{ VENDOR_ATI, 0x00004170, 0xFFF0, VC_RADEON9600 }, 
	{ VENDOR_ATI, 0x00004E44, 0xFFFF, VC_RADEON9700 },
	{ VENDOR_ATI, 0x00004E64, 0xFFFF, VC_RADEON9700 },
	{ VENDOR_ATI, 0x00004E4A, 0xFFFF, VC_RADEON9800 },
	{ VENDOR_ATI, 0x00004E6A, 0xFFFF, VC_RADEON9800 }
};

EVideoCard GetVideoCard()
{
	D3DADAPTER_IDENTIFIER9 sID;
	HRESULT hRes = pD3D->GetAdapterIdentifier( GetAdapterToUse(), 0, &sID );
	if ( FAILED( hRes ) )
		return VC_DEFAULT;

	for ( int nTemp = 0; nTemp < ARRAY_SIZE( videoCardsArray ); nTemp++ )
	{
		const SVideoCardType &sType = videoCardsArray[nTemp];
		if ( ( sID.VendorId == sType.dwVendorID ) && ( ( sID.DeviceId & sType.dwDeviceIDMask ) == ( sType.dwDeviceID & sType.dwDeviceIDMask ) ) )
			return sType.eType;
	}

	return VC_DEFAULT;
}

EHardwareLevel GetHardwareLevel()
{
	
	if ( bTnLDevice )
		return HL_TNL_DEVICE;
	if ( bHardwarePixelShaders20 )
		return HL_R300;
	//if ( !bHardwareVP )
	//	return HL_GFORCE;
	if ( bHardwarePixelShaders14 )
		return HL_RADEON2;
	if ( bHardwarePixelShaders )
		return HL_GFORCE3; // not exactly GF3, but GF3 class


	ASSERT(0); // not supported situation, without shaders tnl device should be used and bTnLDevice should be true
	return HL_TNL_DEVICE;
}

bool IsTnLDevice()
{
	return bTnLDevice;
}

bool DoesSupportOcclusionQueries()
{
	return bDoesSupportOcclusionQueries;
}

IQuery* CreateOcclusionQuery()
{
	NWin32Helper::com_ptr<IDirect3DQuery9> pQuery;
	HRESULT hr = NGfx::pDevice->CreateQuery( D3DQUERYTYPE_OCCLUSION, pQuery.GetAddr() );
	if ( FAILED(hr) )
		return 0;
	EraseInvalidRefs( &queries );
	CQuery *pRes = new CQuery( pQuery );
	queries.push_back( pRes );
	return pRes;
}

IQuery* CreateEventQuery()
{
	NWin32Helper::com_ptr<IDirect3DQuery9> pQuery;
	HRESULT hr = NGfx::pDevice->CreateQuery( D3DQUERYTYPE_EVENT, pQuery.GetAddr() );
	if ( FAILED(hr) )
		return 0;
	EraseInvalidRefs( &queries );
	CQuery *pRes = new CQuery( pQuery );
	queries.push_back( pRes );
	return pRes;
}

int GetMaxBufferedFlipsNum()
{
	nMaxLag = Clamp( nMaxLag, 1, 3 );
	if ( bDoesSupportEventQueries )
		return nMaxLag;
	return 3;
}

void ReduceHWLag()
{
	if ( !bDoesSupportEventQueries )
		return;
	int nLag = GetMaxBufferedFlipsNum();
	// mark frame with first event in queue
	if ( !lagQueries.empty() )
	{
		IQuery *p = lagQueries.front();
		if ( IsValid(p) )
			p->Finish();
	}
	CObj<IQuery> pFrameMark;
	while ( lagQueries.size() > nLag )
	{
		pFrameMark = lagQueries.back();
		lagQueries.pop_back();
		if ( !IsValid(pFrameMark) )
			continue;
		pFrameMark->GetData();
	}
	// put query in front of the queue
	if ( !IsValid(pFrameMark) )
		pFrameMark = CreateEventQuery();
	//pFrameMark->Start(); // start is not supported for this type of query
	lagQueries.push_front( pFrameMark );
}

#define ZERO_ARRAY(a) { for ( int k = 0; k < ARRAY_SIZE(a); ++k ) a[k] = 0; }

// Vertex formats description

//{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
//{ 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
//{ 0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
//static D3DVERTEXELEMENT9 dwVecTC[] =
//{
//	{0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
//	{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
//	{0, 16, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
//	D3DDECL_END()
//};
/*static DWORD dwVecTC[] = {
	D3DVSD_STREAM(0),
	D3DVSD_REG(0, D3DVSDT_FLOAT3),
	D3DVSD_REG(2, D3DVSDT_D3DCOLOR),
	D3DVSD_REG(3, D3DVSDT_FLOAT2),
	D3DVSD_END()
};*/
static D3DVERTEXELEMENT9 dwVecT2C1[] =
{
	{0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
	{0, 16, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	{0, 24, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
	D3DDECL_END()
};
/*static DWORD dwVecNT[] = {
	D3DVSD_STREAM(0),
	D3DVSD_REG(0, D3DVSDT_FLOAT3),
	D3DVSD_REG(1, D3DVSDT_FLOAT3),
	D3DVSD_REG(3, D3DVSDT_FLOAT2),
	D3DVSD_END()
};*/
static D3DVERTEXELEMENT9 dwVecFull[] =
{
	{0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
	{0, 16, D3DDECLTYPE_SHORT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	{0, 20, D3DDECLTYPE_SHORT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
	{0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0 },
	{0, 28, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 1 },
	D3DDECL_END()
};
/*static DWORD dwVecFull[] = {
	D3DVSD_STREAM(0),
	D3DVSD_REG(0, D3DVSDT_FLOAT3),
	D3DVSD_REG(1, D3DVSDT_D3DCOLOR),
	D3DVSD_REG(3, D3DVSDT_SHORT2),
	D3DVSD_REG(6, D3DVSDT_SHORT2),
	D3DVSD_REG(4, D3DVSDT_D3DCOLOR),
	D3DVSD_REG(5, D3DVSDT_D3DCOLOR),
	D3DVSD_END()
};*/
SGeomFormatInfo geometryFormatInfo[6] =
{
	{ 0, 0, dwVecFull, 0 },//SGeomVec::ID, sizeof(SGeomVec), dwVec },
	{ 0, 0, dwVecFull, 0 },//{ SGeomVecT1::ID, sizeof(SGeomVecT1), dwVecT },
	{ 0, 0, dwVecFull, 0 },//SGeomVecT1C1::ID, sizeof(SGeomVecT1C1), dwVecTC, D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1 },
	{ 0, 0, dwVecFull, 0 },//{ SGeomVecT2C1::ID, sizeof(SGeomVecT2C1), dwVecT2C },
	{ SGeomVecT2C1::ID, sizeof(SGeomVecT2C1), dwVecT2C1,  D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX2 },
	{ SGeomVecFull::ID, sizeof(SGeomVecFull), dwVecFull, 0 }
};

static void ApplyRenderState( D3DRENDERSTATETYPE state, DWORD dwVal )
{
	ASSERT( state >= 0 && state <= N_RENDER_STATES );
	if ( renderStates[state] != dwVal )
	{
		pDevice->SetRenderState( state, dwVal );
		renderStates[ state ] = dwVal;
	}
}

static void ApplyRenderState( int nStage, D3DTEXTURESTAGESTATETYPE state, DWORD dwVal )
{
	ASSERT( state >= 0 && state <= N_TSS_STATES );
	if ( tssStates[nStage][state] != dwVal )
	{
		pDevice->SetTextureStageState( nStage, state, dwVal );
		tssStates[nStage][state] = dwVal;
	}
}

static void ApplySamplerState( int nStage, D3DSAMPLERSTATETYPE state, DWORD dwVal )
{
	ASSERT( state >= 0 && state <= N_SAMPLER_STATES );
	if ( samplerStates[nStage][state] != dwVal )
	{
		pDevice->SetSamplerState( nStage, state, dwVal );
		samplerStates[nStage][state] = dwVal;
	}
}

static void ApplyRenderStates( const SRenderState *pRS )
{
	for ( const SRenderState *p = pRS; p->state != 0; ++p )
		ApplyRenderState( p->state, p->dwVal );
}

static void ApplyRenderStates( STextureStageState *pRS )
{
	for ( const STextureStageState *p = pRS; p->nStage != -1; ++p )
		ApplyRenderState( p->nStage, p->state, p->dwVal );
}

static void SetVSConst( int nReg, const CVec4 *pData, int nSize )
{
	pDevice->SetVertexShaderConstantF( nReg, (const float*)pData, nSize );
}

static void SetPSConst( int nReg, const CVec4 *pData, int nSize )
{
	if ( bHardwarePixelShaders )
		pDevice->SetPixelShaderConstantF( nReg, (const float*)pData, nSize );
	else
	{
		ASSERT( nReg == 0 && nSize == 1 );
		DWORD dwColor = GetDWORDColor( pData[0] );
		ApplyRenderState( D3DRS_TEXTUREFACTOR, dwColor );
	}
}

static void SetPixelShader( const SPShader &s )
{
	HRESULT hr;
	if ( pCurrentPixelShader == &s )
		return;
	pCurrentPixelShader = &s;
	ApplyRenderStates( pCurrentPixelShader->pStateRS );
	ApplyRenderStates( pCurrentPixelShader->pStateTSS );
	if ( bHardwarePixelShaders )
	{
		hr = pDevice->SetPixelShader( pixelShaders[ s.nID - 1 ] );
		ASSERT( D3D_OK == hr );
	}
	else
	{
		ASSERT( pCurrentPixelShader->pShaTSS->state != 0 );
		ApplyRenderStates( pCurrentPixelShader->pShaRS );
		ApplyRenderStates( pCurrentPixelShader->pShaTSS );
	}
}

static void SetVertexShader( CGeometry *pVB, int nShaderID )
{
	int nFormatID = pVB->GetGeometryFormatID();
	if ( bTnLDevice )
	{
		if ( nFormatID != nLastUsedVDeclaration )
		{
			nLastUsedVDeclaration = nFormatID;
			DWORD dwFVF = geometryFormatInfo[nFormatID].dwFVF;
			pDevice->SetFVF( dwFVF );
		}
		if ( nLastUsedVShader != nShaderID )
		{
			nLastUsedVShader = nShaderID;
			//pDevice->SetRenderState( D3DRS_LIGHTING, (dwFVF & D3DFVF_DIFFUSE) ? FALSE : TRUE );
			switch ( nShaderID )
			{
			case TNLVS_NONE:
				ApplyRenderState( D3DRS_LIGHTING, FALSE );
				ApplyRenderState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
				break;
			case TNLVS_TEXTRANS:
				ApplyRenderState( D3DRS_LIGHTING, FALSE );
				ApplyRenderState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );
				break;
			case TNLVS_VERTEX_COLOR:
				ApplyRenderState( D3DRS_LIGHTING, TRUE );
				ApplyRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL );
				ApplyRenderState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
				break;
			case TNLVS_VERTEX_COLOR_AND_ALPHA:
				ApplyRenderState( D3DRS_LIGHTING, TRUE );
				ApplyRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1 );
				ApplyRenderState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
				break;
			}
		}
		return;
	}
	//const SVShader &shader = vertexShaders
	if ( nFormatID != nLastUsedVDeclaration )
	{
		nLastUsedVDeclaration = nFormatID;
		HRESULT hRes = pDevice->SetVertexDeclaration( vertexDeclarations[ nFormatID ] );
		ASSERT( hRes == D3D_OK );
	}
	if ( nLastUsedVShader != nShaderID )
	{
		nLastUsedVShader = nShaderID;
		HRESULT hRes = pDevice->SetVertexShader( vertexShaders[ nShaderID - 1 ] );
		ASSERT( hRes == D3D_OK );
	}
}

// render modes application

template<>
static void Apply( const SFBTransform &trans )
{
	if ( bTnLDevice )
	{
		SHMatrix m;
		Transpose( &m, trans.forward );
		pDevice->SetTransform( D3DTS_PROJECTION, (const D3DMATRIX*)&m );
		return;
	}
	SetVSConst( 10, (CVec4*)&trans.forward, 4 );
	// calculate world space camera pos and store it to c6
	CVec4 ptRes;
	trans.backward.RotateHVector( &ptRes, CVec4(0,0,1,0) );
	if ( ptRes.w <= 0 )
		ptRes = -ptRes;
	SetVSConst( 9, &ptRes, 1 );
}

template<>
static void Apply( const EWireframe &wireFrame )
{
	if ( wireFrame == WIREFRAME_ON )
		ApplyRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
	else
		ApplyRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
}

template<>
static void Apply( const EDithering &a )
{
	if ( a == DITHER_ON )
		ApplyRenderState( D3DRS_DITHERENABLE, TRUE );
	else
		ApplyRenderState( D3DRS_DITHERENABLE, FALSE );
}

template<>
static void Apply( const EAlphaCombineMode &alphaMode )
{
	switch ( alphaMode )
	{
		case COMBINE_NONE:
			ApplyRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
			break;
		case COMBINE_ADD:
			ApplyRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			ApplyRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
			ApplyRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
			break;
		case COMBINE_MUL:
			ApplyRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			ApplyRenderState( D3DRS_SRCBLEND, D3DBLEND_ZERO );
			ApplyRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR );
			break;
		case COMBINE_MUL2:
			ApplyRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			ApplyRenderState( D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR );
			ApplyRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR );
			break;
		case COMBINE_SMART_ALPHA:
			ApplyRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			ApplyRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
			ApplyRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
			break;
		case COMBINE_ALPHA:
			ApplyRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			ApplyRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			ApplyRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
			break;
		case COMBINE_ALPHA_ADD:
			ApplyRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			ApplyRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			ApplyRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
			break;
		case COMBINE_ZERO_ONE:
			ApplyRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			ApplyRenderState( D3DRS_SRCBLEND, D3DBLEND_ZERO );
			ApplyRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
			break;
		case COMBINE_SRC_ALPHA_MUL:
			ApplyRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			ApplyRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			ApplyRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
			break;
		case COMBINE_ADD_SRC_ALPHA_MUL:
			ApplyRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			ApplyRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			ApplyRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
			break;
	}
}

template<>
static void Apply( const SStencilMode &m )
{
	switch ( m.mode )
	{
		case STENCIL_NONE:
			ApplyRenderState( D3DRS_STENCILENABLE, FALSE );
			break;
		case STENCIL_INCR:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_INCR );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, m.nMask );
			break;
		case STENCIL_DECR:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_DECR );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, m.nMask );
			break;
		case STENCIL_TESTINCR:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_INCR );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, m.nMask );
			break;
		case STENCIL_TESTDECR:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_DECR );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, m.nMask );
			break;
		case STENCIL_TEST:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			break;
		/*case STENCIL_TESTWRITE:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, m.nMask );
			break;*/
		case STENCIL_TESTNE_WRITE:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, m.nMask );
			break;
		case STENCIL_TEST_CLEAR:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_REPLACE );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, m.nMask );
			break;
		case STENCIL_TESTNE_CLEAR:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_REPLACE );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, 0xffffffff );
			break;
		case STENCIL_WRITE:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, m.nMask );
			break;
/*		case STENCIL_TEST:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, m.nMask );
			break;
		case STENCIL_INVERT:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_INVERT );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, m.nMask );
			break;
		case STENCIL_NOTEQUAL:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, m.nMask );
			break;*/
		case STENCIL_TEST_REPLACE:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE );//INVERT );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			ApplyRenderState( D3DRS_STENCILWRITEMASK, 0xffffffff );//0x80 );
			break;
		case STENCIL_GREATER:
			ApplyRenderState( D3DRS_STENCILENABLE, TRUE );
			ApplyRenderState( D3DRS_STENCILFUNC, D3DCMP_LESS );
			ApplyRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			ApplyRenderState( D3DRS_STENCILREF, m.nVal );
			ApplyRenderState( D3DRS_STENCILMASK, m.nMask );
			break;
	}
}

template<>
static void Apply( const EDepthMode &depth )
{
	switch ( depth )
	{
		case DEPTH_NONE:
			ApplyRenderState( D3DRS_ZENABLE, FALSE );
			ApplyRenderState( D3DRS_ZWRITEENABLE, FALSE );
			ApplyRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
			break;
		case DEPTH_NORMAL:
			ApplyRenderState( D3DRS_ZENABLE, TRUE );
			ApplyRenderState( D3DRS_ZWRITEENABLE, TRUE );
			ApplyRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
			break;
		case DEPTH_INVERSETEST:
			ApplyRenderState( D3DRS_ZENABLE, TRUE );
			ApplyRenderState( D3DRS_ZWRITEENABLE, FALSE );
			ApplyRenderState( D3DRS_ZFUNC, D3DCMP_GREATER );
			break;
		case DEPTH_EQUAL:
			ApplyRenderState( D3DRS_ZENABLE, TRUE );
			ApplyRenderState( D3DRS_ZWRITEENABLE, FALSE );
			ApplyRenderState( D3DRS_ZFUNC, D3DCMP_EQUAL );
			break;
		case DEPTH_OVERWRITE:
			ApplyRenderState( D3DRS_ZENABLE, TRUE );
			ApplyRenderState( D3DRS_ZWRITEENABLE, TRUE );
			ApplyRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
			break;
		case DEPTH_TESTONLY:
			ApplyRenderState( D3DRS_ZENABLE, TRUE );
			ApplyRenderState( D3DRS_ZWRITEENABLE, FALSE );
			ApplyRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
			break;
		case DEPTH_GREATEEQTEST:
			ApplyRenderState( D3DRS_ZENABLE, TRUE );
			ApplyRenderState( D3DRS_ZWRITEENABLE, FALSE );
			ApplyRenderState( D3DRS_ZFUNC, D3DCMP_GREATEREQUAL );
			break;
		case DEPTH_NORMAL_NOTEQ:
			ApplyRenderState( D3DRS_ZENABLE, TRUE );
			ApplyRenderState( D3DRS_ZWRITEENABLE, TRUE );
			ApplyRenderState( D3DRS_ZFUNC, D3DCMP_LESS );
			break;
	}
}

template<>
static void Apply( const ECullMode &cull )
{
  switch ( cull )
  {
    case CULL_CW:
      ApplyRenderState( D3DRS_CULLMODE, D3DCULL_CW );
      break;
    case CULL_CCW:
      ApplyRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
      break;
    case CULL_NONE:
      ApplyRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
      break;
  }
}

template<>
static void Apply( const EColorWriteMask &colorMode )
{
	DWORD dwFlags = 
		(( colorMode & COLORWRITE_RED ) ? D3DCOLORWRITEENABLE_RED : 0) |
		(( colorMode & COLORWRITE_GREEN ) ? D3DCOLORWRITEENABLE_GREEN : 0) |
		(( colorMode & COLORWRITE_BLUE ) ? D3DCOLORWRITEENABLE_BLUE : 0) |
		(( colorMode & COLORWRITE_ALPHA ) ? D3DCOLORWRITEENABLE_ALPHA : 0);
	ApplyRenderState( D3DRS_COLORWRITEENABLE, dwFlags );
}

template<>
static void Apply( const SFogParams &fog )
{
	if ( IsTnLDevice() )
		return;

	CVec4 v19[2]; // v19[1] = vColor

	float fMaxDist =  10.0f;
	float fMinDist = +20.0f;


	float fAlpha = 1.0f / ( fog.fMaxDist - fog.fMinDist );
	float fAlph2 = 1.0f / ( fog.fMaxZDis - fog.fMinZDis );
	v19[0].Set( -fAlpha, fAlpha * fog.fMaxDist,
	            -fAlph2, fAlph2 * fog.fMaxZDis );
	v19[1].Set( fog.vColor, 1 );
	nFogModeColor = GetDWORDColor( CVec4( fog.vColor, 1 ) );

	//ApplyRenderState( D3DRS_FOGTABLEMODE, D3DFOG_LINEAR );
	//ApplyRenderState( D3DRS_FOGSTART, *(DWORD *)(&pFog->fMinDist) );
	//ApplyRenderState( D3DRS_FOGEND,   *(DWORD *)(&pFog->fMaxDist) );

	ApplyRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	ApplyRenderState( D3DRS_FOGVERTEXMODE, D3DFOG_NONE );
	SetVSConst( 19, &v19[0], 2 );

	if ( fogMode.value == FOG_NORMAL )
		ApplyRenderState( D3DRS_FOGCOLOR, nFogModeColor );
}

template<>
static void Apply( const EFogMode &fog )
{
	if ( IsTnLDevice() )
		return;

	switch ( fog )
	{
	case FOG_NONE:
		ApplyRenderState( D3DRS_FOGENABLE, FALSE );
		break;
	case FOG_BLACK:
		ApplyRenderState( D3DRS_FOGCOLOR, 0 );
		ApplyRenderState( D3DRS_FOGENABLE, TRUE );
		break;
	case FOG_NORMAL:
		ApplyRenderState( D3DRS_FOGCOLOR, nFogModeColor );
		ApplyRenderState( D3DRS_FOGENABLE, TRUE );
		break;
	}
}

static void SetTextureWrap( int nStage, EWrap wrap )
{
	if ( lastUsedAddressMode[nStage] == wrap )
		return;
	lastUsedAddressMode[nStage] = wrap;
	ApplySamplerState( nStage, D3DSAMP_ADDRESSU, ( wrap & WRAP_X ) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP );
	ApplySamplerState( nStage, D3DSAMP_ADDRESSV, ( wrap & WRAP_Y ) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP );
}

inline void TrySetAnisotropicMagFilter( int nStage )
{
	if ( devCaps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC )
		pDevice->SetSamplerState( nStage, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC );
	else
		pDevice->SetSamplerState( nStage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
}

inline void TrySetAnisotropicMinFilter( int nStage )
{
	if ( devCaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC )
		pDevice->SetSamplerState( nStage, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC );
	else
		pDevice->SetSamplerState( nStage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
}

static void SetTextureFilter( int n, EFilterMode filter )
{
	if ( TSFilterMode[n] == filter )
		return;
	TSFilterMode[n] = filter;
	if ( filter == FILTER_POINT )
	{
		ApplySamplerState( n, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
		ApplySamplerState( n, D3DSAMP_MINFILTER, D3DTEXF_POINT );
		ApplySamplerState( n, D3DSAMP_MIPFILTER, D3DTEXF_POINT );//D3DTEXF_POINT );
	}
	else
	{
		if ( nUseAnisotropy > 1 && ( filter == FILTER_BEST || filter == FILTER_BUMP ) )
		{
			nUseAnisotropy = Min( nUseAnisotropy, (int)devCaps.MaxAnisotropy );
			TrySetAnisotropicMagFilter( n );
			TrySetAnisotropicMinFilter( n );
			ApplySamplerState( n, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );//D3DTEXF_LINEAR );//D3DTEXF_POINT );
			ApplySamplerState( n, D3DSAMP_MAXANISOTROPY, nUseAnisotropy );
		}
		else
		{
			ApplySamplerState( n, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
			ApplySamplerState( n, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
			// если кто-то решил потратить fillrate`а, то нужно использовать как минимум trilinear
			if ( filter == FILTER_LINEAR )
				ApplySamplerState( n, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
			else
				ApplySamplerState( n, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
		}
	}
}

void ForceTextureFilterSetup()
{
	for ( int i = 0; i < ARRAY_SIZE( TSFilterMode ); ++i )
		TSFilterMode[i] = (EFilterMode)-1;
}

static NWin32Helper::com_ptr<IDirect3DSurface9> pCurrentRTTB, pCurrentRTZB;
static void SetRT( IDirect3DSurface9 *pTB, IDirect3DSurface9 *pZB )
{
	ASSERT( pTB && pZB );
	// short cut
	if ( pCurrentRTTB == pTB && pCurrentRTZB == pZB )
		return;
	HRESULT hr;
	// actually this was required only for textures that are rendertargets
	for ( int k = 0; k < 8; ++k )
		pDevice->SetTexture( k, 0 ); 
	//hr = pDevice->EndScene(); ASSERT( hr == D3D_OK );
	if ( pCurrentRTTB != pTB )
	{
		pCurrentRTTB = pTB;
		hr = pDevice->SetRenderTarget( 0, pTB );
		ASSERT( D3D_OK == hr );
	}
	if ( pCurrentRTZB != pZB )
	{
		pCurrentRTZB = pZB;
		hr = pDevice->SetDepthStencilSurface( pZB );
		ASSERT( D3D_OK == hr );
	}
	/*hr = pDevice->SetRenderTarget( 1, 0 );
	ASSERT( D3D_OK == hr );
	hr = pDevice->SetRenderTarget( 2, 0 );
	ASSERT( D3D_OK == hr );
	hr = pDevice->SetRenderTarget( 3, 0 );
	ASSERT( D3D_OK == hr );*/
	//hr = pDevice->BeginScene(); ASSERT( hr == D3D_OK );
}

// CRenderContext

static const CRenderContext *pCurrentRenderContext;
CRenderContext::CRenderContext()
	: alpha(COMBINE_NONE), stencil( SStencilMode(STENCIL_NONE) ), depth(DEPTH_NORMAL), 
	cull(CULL_CW), colorWrite( COLORWRITE_ALL ), targetMode( RTM_SCREEN ), fog(FOG_NONE),
	pTarget(0), nMipLevel(0), pPixelShader(&psDiffuse), nVertexShader(1),
	nRegister(0), pOutstandingStream(0)
{
	Identity( &transform.forward );
	Identity( &transform.backward );
}

CRenderContext::~CRenderContext()
{
	ASSERT( pOutstandingStream == 0 );
	if ( this == pCurrentRenderContext )
		pCurrentRenderContext = 0;
}

void CRenderContext::SetTransform( const SFBTransform &_transform )
{
	ASSERT( pOutstandingStream == 0 );
	transform = _transform;
	if ( pCurrentRenderContext == this )
		transformMode.Set( transform );
}

void CRenderContext::SetAlphaCombine( EAlphaCombineMode mode )
{
	ASSERT( pOutstandingStream == 0 );
	alpha = mode;
	if ( pCurrentRenderContext == this )
		alphaMode.Set( alpha );
}

void CRenderContext::SetStencil( const SStencilMode &m )
{
	ASSERT( pOutstandingStream == 0 );
	stencil = m;
	if ( pCurrentRenderContext == this )
		stencilMode.Set( stencil );
}

void CRenderContext::SetDepth( EDepthMode mode )
{
	ASSERT( pOutstandingStream == 0 );
	depth = mode;
	if ( pCurrentRenderContext == this )
		depthMode.Set( depth );
}

void CRenderContext::SetCulling( ECullMode mode )
{
	ASSERT( pOutstandingStream == 0 );
	cull = mode;
	if ( pCurrentRenderContext == this )
		cullMode.Set( cull );
}

void CRenderContext::SetColorWrite( EColorWriteMask mode )
{
	ASSERT( pOutstandingStream == 0 );
	colorWrite = mode;
	if ( pCurrentRenderContext == this )
		colorMode.Set( mode );
}

void CRenderContext::SetFogParams( const SFogParams &mode )
{
	ASSERT( pOutstandingStream == 0 );
	fogParams = mode;
	if ( pCurrentRenderContext == this )
		fogParamsMode.Set( mode );
}

void CRenderContext::SetFog( EFogMode mode )
{
	ASSERT( pOutstandingStream == 0 );
	fog = mode;
	if ( pCurrentRenderContext == this )
		fogMode.Set( mode );
}

void CRenderContext::SetScreenRT()
{
	ASSERT( pOutstandingStream == 0 );
	targetMode = RTM_SCREEN;
	pTarget = 0;
	pCubeTarget = 0;
	if ( pCurrentRenderContext == this )
		ApplyRenderTarget();
}

void CRenderContext::SetTextureRT( CTexture *pTexture, int _nMipLevel )
{
	ASSERT( pOutstandingStream == 0 );
	targetMode = RTM_TEXTURE;
	pTarget = pTexture;
	pCubeTarget = 0;
	nMipLevel = _nMipLevel;
	if ( pCurrentRenderContext == this )
		ApplyRenderTarget();
}

void CRenderContext::SetCubeTextureRT( CCubeTexture *pTexture, EFace nFace, int _nMipLevel )
{
	ASSERT( pOutstandingStream == 0 );
	targetMode = RTM_CUBETEXTURE;
	pTarget = 0;
	pCubeTarget = pTexture;
	nMipLevel = _nMipLevel;
	nRegister = nFace;
	if ( pCurrentRenderContext == this )
		ApplyRenderTarget();
}

void CRenderContext::SetVirtualRT()
{
	ASSERT( pOutstandingStream == 0 );
	targetMode = RTM_REGISTERS;
	pTarget = 0;
	pCubeTarget = 0;
	nRegister = 0;
	if ( pCurrentRenderContext == this )
		ApplyRenderTarget();
}

const int N_REGISTER_MASK = 7;
const int N_USE_DEPTH_REGISTER = 8;
void CRenderContext::SetRegister( int _nRegister )
{
	ASSERT( pOutstandingStream == 0 );
	ASSERT( targetMode == RTM_REGISTERS );
	if ( _nRegister >= 0 && (_nRegister & N_REGISTER_MASK ) < nScreenRegisters )
		nRegister = _nRegister;
	else
		ASSERT(0);
	if ( pCurrentRenderContext == this )
		ApplyRenderTarget();
}

bool CopyScreenToRegister( int _nRegister )
{
	if ( _nRegister >= 0 && (_nRegister & N_REGISTER_MASK ) < nScreenRegisters )
	{
		ReplaceTextureSurface( pRegisters[_nRegister], 0, pScreenColor );
		return true;
	}
	return false;
}

void CopyScreenToTexture( CTexture *pTex )
{
	ASSERT( pTex );
	if ( pTex )
		ReplaceTextureSurface( pTex, 0, pScreenColor );
}

static void SetRT( IDirect3DSurface9 *pTB, int nSize )
{
	CDepthHash::iterator i = sharedZBuffers.find( nSize );
	if ( i == sharedZBuffers.end() )
	{
		int nMax = 1000000;
		for ( CDepthHash::iterator k = sharedZBuffers.begin(); k != sharedZBuffers.end(); ++k )
		{
			if ( k->first > nSize && k->first < nMax )
			{
				i = k;
				nMax = k->first;
			}
		}
		if ( i == sharedZBuffers.end() )
		{
			ASSERT( 0 && "no suitable zbuffer found!" );
			return;
		}
	}
	SetRT( pTB, i->second );
}

void CRenderContext::ApplyRenderTarget() const
{
	switch ( targetMode )
	{
	case RTM_SCREEN:
		SetRT( pScreenColor, pScreenDepth );
		break;
	case RTM_TEXTURE:
		if ( IsValid( pTarget ) )
		{
			NWin32Helper::com_ptr<IDirect3DSurface9> pTB;
			GetSurface( pTarget, nMipLevel, &pTB );
			//D3DSURFACE_DESC ddsd;
			//pTB->GetDesc( &ddsd );
			CDynamicCast<I2DBuffer> pTargetBuf( pTarget );
			SetRT( pTB, pTargetBuf->GetSizeX() );
		}
		else
			ASSERT(0);
		break;
	case RTM_REGISTERS:
		{
			NWin32Helper::com_ptr<IDirect3DSurface9> pTB;
			GetSurface( pRegisters[nRegister & N_REGISTER_MASK], 0, &pTB );
			SetRT( pTB, pRegisterDepth );
			/*if ( nRegister & N_USE_DEPTH_REGISTER )
			{
				HRESULT hr;
				NWin32Helper::com_ptr<IDirect3DSurface9> pTB;
				GetSurface( pRegisters[ (nRegister + 1 ) & N_REGISTER_MASK ], 0, &pTB );
				hr = pDevice->SetRenderTarget( 1, pTB );
				ASSERT( D3D_OK == hr );
				GetSurface( pRegisters[ (nRegister + 2 ) & N_REGISTER_MASK ], 0, &pTB );
				hr = pDevice->SetRenderTarget( 2, pTB );
				ASSERT( D3D_OK == hr );
				GetSurface( pDepthRegister, 0, &pTB );
				hr = pDevice->SetRenderTarget( 3, pTB );
				ASSERT( D3D_OK == hr );
			}*/
		}
		break;
	case RTM_CUBETEXTURE:
		if ( IsValid( pCubeTarget ) )
		{
			NWin32Helper::com_ptr<IDirect3DSurface9> pTB;
			GetSurface( pCubeTarget, (EFace)nRegister, nMipLevel, &pTB );
			//D3DSURFACE_DESC ddsd;
			//pTB->GetDesc( &ddsd );
			CDynamicCast<ICubeBuffer> pTargetBuf( pCubeTarget );
			SetRT( pTB, pTargetBuf->GetSize() );
		}
		else
			ASSERT(0);
		break;
	}
}

void CRenderContext::ClearTarget( DWORD dwColor )
{
	ASSERT( pOutstandingStream == 0 );
	HRESULT hr;
	Use();
	hr = pDevice->Clear( 0, 0, D3DCLEAR_TARGET, dwColor, 1, 0 );
	ASSERT( D3D_OK == hr );
}

void CRenderContext::ClearBuffers( DWORD dwColor )
{
	ASSERT( pOutstandingStream == 0 );
	HRESULT hr;
	Use();
	DWORD dwFlags = b16BitMode ? D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER : D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL;
	hr = pDevice->Clear( 0, 0, dwFlags, dwColor, 1, 0 );
	ASSERT( D3D_OK == hr );
}

void CRenderContext::ClearZBuffer()
{
	ASSERT( pOutstandingStream == 0 );
	HRESULT hr;
	Use();
	DWORD dwFlags = b16BitMode ? D3DCLEAR_ZBUFFER : D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL;
	hr = pDevice->Clear( 0, 0, dwFlags, 0, 1, 0 );
	ASSERT( D3D_OK == hr );
}

void CRenderContext::SetPixelShader( const SPShader &s )
{
	ASSERT( pOutstandingStream == 0 );

	pPShader = 0;
	pPixelShader = &s;
	pCurrentPixelShader = 0;
}

void CRenderContext::SetPixelShader( const std::string &szName )
{
	ASSERT( pOutstandingStream == 0 );

	pPShader = 0;
	pPixelShader = 0;
	pCurrentPixelShader = 0;

	TPixelShaders::const_iterator iFindRes = hlslPixelShaders.find( szName );
	if ( iFindRes == hlslPixelShaders.end() )
	{
		pPShader = CreatePixelShader( szName );
		hlslPixelShaders[szName] = pPShader;
	}
	else
		pPShader = iFindRes->second;
}

void CRenderContext::SetVertexShader( const SVShader &s )
{
	ASSERT( !bTnLDevice );
	ASSERT( pOutstandingStream == 0 );
	ASSERT( !IsTnLDevice() );

	pVShader = 0;
	nVertexShader = s.nID;
}


void CRenderContext::SetVertexShader( const std::string &szName )
{
	ASSERT( !bTnLDevice );
	ASSERT( pOutstandingStream == 0 );
	ASSERT( !IsTnLDevice() );

	pVShader = 0;
	nVertexShader = 0;

	TVertexShaders::const_iterator iFindRes = hlslVertexShaders.find( szName );
	if ( iFindRes == hlslVertexShaders.end() )
	{
		pVShader = CreateVertexShader( szName );
		hlslVertexShaders[szName] = pVShader;
	}
	else
		pVShader = iFindRes->second;
}

//void CRenderContext::SetVSConst( int nReg, const CVec4 *pData, int nSize )

/*void CRenderContext::SetPixelShader( const SPShader &s )
{
	ASSERT( pOutstandingStream == 0 );
	pPixelShader = &s;
}

void CRenderContext::SetVertexShader( const SVShader &s )
{
	ASSERT( pOutstandingStream == 0 );
	ASSERT( !IsTnLDevice() );
	nVertexShader = s.nID;
}*/

void CRenderContext::SetVertexShader( ETnLVS shader )
{
	ASSERT( pOutstandingStream == 0 );
	ASSERT( IsTnLDevice() );
	nVertexShader = shader;
}

bool CRenderContext::SetShader( const SHLSLShader &pShader )
{
	ASSERT( pOutstandingStream == 0 );
	ASSERT( !IsTnLDevice() );
	ASSERT( pShader.nPSShaderID > 0 && pShader.nPSShaderID <= ARRAY_SIZE(psAllShaders) );
	ASSERT( pShader.nVSShaderID > 0 && pShader.nVSShaderID <= ARRAY_SIZE(vsAllShaders) );
	if ( pixelShaders[ pShader.nPSShaderID - 1 ] == 0 )
		return false;
	if ( vertexShaders[ pShader.nVSShaderID - 1 ] == 0 )
		return false;
	nVertexShader = pShader.nVSShaderID;
	pPixelShader = psAllShaders[ pShader.nPSShaderID - 1 ];
	return true;
}

void CRenderContext::SetVSConst( int nReg, const CVec4 *pData, int nSize ) const
{
	ASSERT( pOutstandingStream == 0 );
	ASSERT( pCurrentRenderContext == this );
	NGfx::SetVSConst( nReg, pData, nSize );
}

void CRenderContext::SetVSConst( int nReg, const CVec3 &_param ) const
{
	ASSERT( pOutstandingStream == 0 );
	CVec4 a( _param, 0 );
	NGfx::SetVSConst( nReg, &a, 1 );
}

void CRenderContext::SetTnlVertexColor( const CVec4 &a )
{
	int nR = Clamp( Float2Int( a.r * 255 ), 0, 255 );
	int nG = Clamp( Float2Int( a.g * 255 ), 0, 255 );
	int nB = Clamp( Float2Int( a.b * 255 ), 0, 255 );
	ApplyRenderState( D3DRS_AMBIENT, D3DCOLOR_RGBA( nR, nG, nB, 0 ) );
	D3DMATERIAL9 mat;
	Zero( mat );
	mat.Diffuse.a = a.a;
	pDevice->SetMaterial( &mat );
}

void CRenderContext::SetTnlTexTransform( const SHMatrix &_m )
{
	SHMatrix m;
	Transpose( &m, _m );
	pDevice->SetTransform( D3DTS_TEXTURE0, (const D3DMATRIX*)&m );
}

void CRenderContext::SetPSConst( int nReg, const CVec4 *pData, int nSize )
{
	ASSERT( pOutstandingStream == 0 );
	ASSERT( pCurrentRenderContext == this );
	NGfx::SetPSConst( nReg, pData, nSize );	
}

void CRenderContext::SetPSConst( int nReg, const CVec3 &_param )
{
	ASSERT( pOutstandingStream == 0 );
	CVec4 a( _param, 0 );
	NGfx::SetPSConst( nReg, &a, 1 );
}

void CRenderContext::SetAlphaRef( int nRef )
{
	ASSERT( pOutstandingStream == 0 );
	ASSERT( pCurrentRenderContext == this );
	ApplyRenderState( D3DRS_ALPHAREF, nRef );
}

void CRenderContext::SetTexture( int nStage, CTexture *pTex, EFilterMode filter )
{
	ASSERT( pOutstandingStream == 0 );
	ASSERT( nStage < 8 );
	ASSERT( pCurrentRenderContext == this );
	NGfx::SetTexture( nStage, pTex );
	if ( !pTex )
		return;
	SetTextureWrap( nStage, GetWrap( pTex ) );
	SetTextureFilter( nStage, filter );
}

void CRenderContext::SetTexture( int nStage, CCubeTexture *pTex )
{
	ASSERT( pOutstandingStream == 0 );
	ASSERT( pCurrentRenderContext == this );
	NGfx::SetTexture( nStage, pTex );
	SetTextureWrap( nStage, WRAP );
	SetTextureFilter( nStage, FILTER_LINEAR );
}

void CRenderContext::Use() const
{
	if ( pCurrentRenderContext == this )
		return;
	pCurrentRenderContext = this;
	transformMode.Set( transform );
	alphaMode.Set( alpha );
	stencilMode.Set( stencil );
	depthMode.Set( depth );
	cullMode.Set( cull );
	colorMode.Set( colorWrite );
	fogParamsMode.Set( fogParams );
	fogMode.Set( fog );
	ApplyRenderTarget();
}

static void DoValidateDevice( int nPID, int nVID )
{
	HRESULT hr;
	DWORD dwPasses;
	hr = pDevice->ValidateDevice( &dwPasses );
	char szBuf[1024];
	sprintf( szBuf, "D3D validate device failed ps = %d,  vs = %d", nPID, nVID );
	if ( D3D_OK != hr )
		MessageBox( 0, szBuf, 0, MB_OK );
	ASSERT( D3D_OK == hr && dwPasses == 1 );
}


static void SetVertexShader( CGeometry *pVB, CVertexShader *pVShader )
{
	int nFormatID = pVB->GetGeometryFormatID();
	if ( bTnLDevice )
	{
		if ( nFormatID == nLastUsedVDeclaration )
			return;

		nLastUsedVDeclaration = nFormatID;
		DWORD dwFVF = geometryFormatInfo[nFormatID].dwFVF;
		pDevice->SetFVF( dwFVF );
		pDevice->SetRenderState( D3DRS_LIGHTING, (dwFVF & D3DFVF_DIFFUSE) ? FALSE : TRUE );
		return;
	}

	if ( nFormatID != nLastUsedVDeclaration )
	{
		nLastUsedVDeclaration = nFormatID;
		HRESULT hRes = pDevice->SetVertexDeclaration( vertexDeclarations[ nFormatID ] );
		ASSERT( hRes == D3D_OK );
	}

	nLastUsedVShader = -1;
	pVShader->Use();
}

void CRenderContext::StartStream( CGeometry *pGeom )
{
	ASSERT( pOutstandingStream == 0 );
	ASSERT( pCurrentRenderContext == this );
	//	ASSERT( pPixelShader );
	//	ASSERT( pVertexShader );

	if ( pPShader )
		pPShader->Begin();
	else
		NGfx::SetPixelShader( *pPixelShader );

	pGeom->SetVertexStream();
	pOutstandingStream = pGeom->GetVertexStream();

	if ( pVShader )
		NGfx::SetVertexShader( pGeom, pVShader );
	else
		NGfx::SetVertexShader( pGeom, nVertexShader);

	if ( bDoValidateDevice )
		DoValidateDevice( pPixelShader->nID, nVertexShader );
}

void CRenderContext::CheckStream( CGeometry *pGeom )
{
	if ( pOutstandingStream )
	{
		if ( pGeom->GetVertexStream() != pOutstandingStream ) 
		{
			Flush();
			StartStream( pGeom );
		}
	}
	else
		StartStream( pGeom );
}

void CRenderContext::DrawPrimitive( CGeometry *pGeom, CTriList *pTris, int nStartVertex, int nVertices )
{
	if ( !pGeom || !pTris )
		return;
	CheckStream( pGeom );
	AddPrimitiveGeometry( pGeom, pTris, nStartVertex, nVertices );
	Flush();
}

void CRenderContext::AddPrimitive( CGeometry *pGeom, CTriList *pTris )
{
	if ( !pGeom || !pTris )
		return;
	CheckStream( pGeom );
	AddPrimitiveGeometry( pGeom, pTris );
}

void CRenderContext::AddPrimitive( CGeometry *pGeom, const STriangleList *pTris, int nCount, unsigned nMask )
{
	if ( !pGeom )
		return;
	CheckStream( pGeom );
	AddPrimitiveGeometry( pGeom, pTris, nCount, nMask );
}

void CRenderContext::AddPrimitive( CGeometry *pGeom, const STriangleList &tris )
{
	AddPrimitive( pGeom, &tris, 1, 1 );
}

/*void CRenderContext::AddPrimitive( CGeometry *pGeom, const vector<STriangle> &tris )
{
	AddPrimitive( pGeom, STriangleList( &tris[0], tris.size() ) );
}*/

void CRenderContext::Flush()
{
	FlushPrimitive();
	pOutstandingStream = 0;

	if ( pPShader )
			pPShader->End();
}

void CRenderContext::AddLineStrip( CGeometry *pGeom, const unsigned short *pIndices, int nLines )
{
	if ( !pGeom )
		return;
	CheckStream( pGeom );
	NGfx::AddLineStrip( pGeom, pIndices, nLines );
}


// GLOBAL FUNCTIONS

void SetWireframe( EWireframe wire )
{
	wireframeMode.Set( wire );
}

void SetDithering( EDithering a )
{
	ditheringMode.Set( a );
}

void GetRegisterSize( CTRect<float> *pRes )
{
	pRes->x1 = 0; pRes->x2 = ptRegisterBufferSize.x;
	pRes->y1 = 0; pRes->y2 = ptRegisterBufferSize.y;
}

CTexture* GetRegisterTexture( int nRegister )
{
	ASSERT( nRegister >= 0 && nRegister < nScreenRegisters );
	return pRegisters[nRegister];
}

CTexture* GetDepthRegisterTexture()
{
	return pDepthRegister;
}

bool IsNVidiaNP2Bug() 
{ 
	return bNVHackNP2; 
}


// INITIALISATION / FINALISATION CODE

static bool AddZBuffer( D3DFORMAT format, int nSize )
{
	CDepthHash::iterator i = sharedZBuffers.find( nSize );
	if ( i == sharedZBuffers.end() )
	{
		HRESULT hr;
		hr = pDevice->CreateDepthStencilSurface( nSize, nSize, format, D3DMULTISAMPLE_NONE, 0,
			TRUE, sharedZBuffers[ nSize ].GetAddr(), 0 );
		if FAILED(hr)
			return false;
	}
	return true;
}

bool InitZBuffer( D3DFORMAT format )
{
	HRESULT hr;
	nScreenRegisters = rtInfo.nRegisters;
	if ( nScreenRegisters > 0 )
	{
		CVec2 ptSize = GetScreenRect();
		int nXRSize = Float2Int( ptSize.x ), nXSize;
		int nYRSize = Float2Int( ptSize.y ), nYSize;
		ptScreenSize = CTPoint<int>( nXRSize, nYRSize );
		nXSize = 1024;
		nYSize = 512;
		if ( !bBanNP2 && (devCaps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) != 0 || (devCaps.TextureCaps & D3DPTEXTURECAPS_POW2) == 0 )
		{
#ifdef _MAPEDIT
			nXSize = Float2Int( nXRSize * sqrt(fRegisterResolution) );
			nYSize = Float2Int( nYRSize * sqrt(fRegisterResolution) );
#else
			nXSize = Float2Int( nXRSize * sqrt(fRegisterResolution) );
			nYSize = Float2Int( nYRSize * (564.0f / 768.0f ) * sqrt(fRegisterResolution) );
#endif
		}
		nXSize = Min( nXSize, (int)devCaps.MaxTextureWidth );
		nYSize = Min( nYSize, (int)devCaps.MaxTextureHeight );
		ptRegisterBufferSize = CTPoint<int>( nXSize, nYSize );
		hr = pDevice->CreateDepthStencilSurface( nXSize, nYSize, format, D3DMULTISAMPLE_NONE, 0,
			FALSE, pRegisterDepth.GetAddr(), 0 );
		if FAILED(hr)
			return false;
		ASSERT( nScreenRegisters <= N_MAX_REGISTERS );
		for ( int k = 0; k < nScreenRegisters; ++k )
			pRegisters[k] = MakeRenderTarget( nXSize, nYSize, SPixel8888::ID );
		if ( rtInfo.nFloatRegisters == 1 )
			pDepthRegister = MakeRenderTarget( nXSize, nYSize, SPixelFloat::ID );
	}
	for ( SRenderTargetsInfo::CRTHash::iterator i = rtInfo.targets.begin(); i != rtInfo.targets.end(); ++i )
	{
		if ( !AddZBuffer( format, i->first.nResolution ) )
			return false;
	}
	for ( SRenderTargetsInfo::CRTHash::iterator i = rtInfo.cubeTargets.begin(); i != rtInfo.cubeTargets.end(); ++i )
	{
		if ( !AddZBuffer( format, i->first.nResolution ) )
			return false;
	}
	
	hr = pDevice->GetRenderTarget( 0, pScreenColor.GetAddr() );
	D3DASSERT( hr, "GetRenderTarget failed" );
	hr = pDevice->GetDepthStencilSurface( pScreenDepth.GetAddr() );
	D3DASSERT( hr, "GetDepthStencilSurface failed" );
	// обрядовый ритуал, без которого nVidia не работает
	for ( int k = 0; k < nScreenRegisters; ++ k )
	{
		NWin32Helper::com_ptr<IDirect3DSurface9> pTB;
		GetSurface( pRegisters[k], 0, &pTB );
		SetRT( pTB, pRegisterDepth );
		hr = pDevice->Clear( 0, 0, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 1, 0 );
		ASSERT( D3D_OK == hr );
	}
	SetRT( pScreenColor, pScreenDepth );
	return true;
}

void DoneZBuffer()
{
	queries.clear();
	pCurrentRTTB = 0;
	pCurrentRTZB = 0;
	pCurrentRenderContext = 0;
	pScreenDepth = 0;
	sharedZBuffers.clear();
	for ( int i = 0; i < nScreenRegisters; ++i )
		pRegisters[i] = 0;
	pDepthRegister = 0;
	pRegisterDepth = 0;
}

static void InitTextureStage( int n )
{
	if ( nUseAnisotropy > 1 )
	{
		nUseAnisotropy = Min( nUseAnisotropy, (int)devCaps.MaxAnisotropy );

		TrySetAnisotropicMagFilter( n );
		TrySetAnisotropicMinFilter( n );

		pDevice->SetSamplerState( n, D3DSAMP_MAXANISOTROPY, nUseAnisotropy );
	}
	else
	{
		pDevice->SetSamplerState( n, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		pDevice->SetSamplerState( n, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	}

	pDevice->SetSamplerState( n, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	float fMipBias = 0;//-1;
	pDevice->SetSamplerState( n, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&fMipBias );
	lastUsedAddressMode[n] = CLAMP;
	TSFilterMode[n] = FILTER_BEST;
	pDevice->SetSamplerState( n, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
	pDevice->SetSamplerState( n, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
	pDevice->SetTextureStageState( n, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
}

static void InitStateBlocks()
{
	ASSERT( ARRAY_SIZE(pixelShaders) >= ARRAY_SIZE(psAllShaders) );
	ASSERT( ARRAY_SIZE(vertexShaders) >= ARRAY_SIZE(vsAllShaders) );
	ASSERT( ARRAY_SIZE(vertexDeclarations) >= ARRAY_SIZE(geometryFormatInfo) );
	ZERO_ARRAY( pixelShaders );
	ZERO_ARRAY( vertexShaders );
	ZERO_ARRAY( vertexDeclarations );
	if ( bHardwarePixelShaders )
	{
		for ( int k = 0; k < ARRAY_SIZE(psAllShaders); ++k )
		{
			const SPShader &sha = *psAllShaders[k];
			DWORD *pShader = sha.pShader11;//bHardwarePixelShaders14 ? psAllShaders[k]->pShader14 : psAllShaders[k]->pShader;
			if ( bHardwarePixelShaders14 && sha.pShader14 != 0 )
				pShader = sha.pShader14;
			if ( bHardwarePixelShaders20a && pShader == 0 )
				pShader = sha.pShader20a;
			if ( bHardwarePixelShaders20 && pShader == 0 )
				pShader = sha.pShader20;

			if ( !pShader )
				continue;
			// guard
			if ( pShader && *pShader == 0xffff0200 && !bHardwarePixelShaders20 )
				continue;
			HRESULT hr;
			hr = pDevice->CreatePixelShader( 
				pShader,
				pixelShaders[k].GetAddr() );
			D3DASSERT( hr, "CreatePixelShader failed" );
		}
	}
	if ( !bTnLDevice )
	{
		for ( int k = 0; k < ARRAY_SIZE(vsAllShaders); ++k )
		{
			const SVShader &sha = *vsAllShaders[k];
			DWORD *pShader = sha.pShader11;
			if ( !pShader )
				continue;
			HRESULT hr;
			hr = pDevice->CreateVertexShader( 
				pShader,
				vertexShaders[k].GetAddr() );
			D3DASSERT( hr, "CreateVertexShader failed" );
		}

		for ( int k = 0; k < ARRAY_SIZE(geometryFormatInfo); ++k )
		{
			const D3DVERTEXELEMENT9 *p = geometryFormatInfo[k].pdwVSD;
			if ( p )
			{
				HRESULT hr;
				hr = pDevice->CreateVertexDeclaration( 
					p,
					vertexDeclarations[k].GetAddr() );
				D3DASSERT( hr, "CreateVertexDeclaration failed" );
			}
		}
	}
}

inline bool IsPow2( int n ) { return GetNextPow2(n) == n; }
inline bool IsPow2( const CTPoint<int> &t ) { return IsPow2(t.x) && IsPow2(t.y); }
static CVec4 GetRegisterMapScale( const CTPoint<int> &vRegSize )
{
	//const CTRect<int> &vp = ptRegisterBufferSize; //rViewport;//ptRegisterSize;
	//float fXHalf = vp.Width() * 0.5f, fYHalf = vp.Height() * 0.5f;
	CTPoint<int> buf;
	if ( !IsPow2(vRegSize) && bNVHackNP2 )
		buf = CTPoint<int>(1,1);
	else
		buf = vRegSize; // true register size
	float fXHalf = vRegSize.x * 0.5f, fYHalf = vRegSize.y * 0.5f;
	return CVec4( fXHalf/buf.x, -fYHalf/buf.y, fXHalf/buf.x + 0.5/buf.x, fYHalf/buf.y + 0.5/buf.y );// + (1-600.0/1024) );
}

static CVec4 GetRegisterMapScale()
{
	//ASSERT( targetMode == RTM_REGISTERS );
	return GetRegisterMapScale( ptRegisterBufferSize );
}

static void InitEffects()
{
	// some universal initialisation
	if ( !bHardwareVP )
		pDevice->SetSoftwareVertexProcessing( TRUE );
	//pDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
	pDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	pDevice->SetRenderState( D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1 );
	pDevice->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL );
	memset( renderStates, 0xcc, sizeof(renderStates) );
	memset( tssStates, 0xcc, sizeof(tssStates) );
	memset( samplerStates, 0xcc, sizeof(samplerStates) );
	for ( int nTemp = 0; nTemp < 8; nTemp++ )
		InitTextureStage( nTemp );
	
	InitStateBlocks();
	nLastUsedVShader = -1;
	nLastUsedVDeclaration = -1;
	pCurrentPixelShader = 0;
	//pCurrentVertexShader = &vsDiffuse;
	if ( IsTnLDevice() )
	{
		D3DMATERIAL9 mat;
		Zero( mat );
		D3DCOLORVALUE white;
		white.r = white.g = white.b = white.a = 1;
		mat.Ambient = white;
		mat.Diffuse = white;
		pDevice->SetMaterial( &mat );
	}
	else
	{
		CVec4 c[9];
		c[0] = CVec4(0,0,0,0);
		c[1] = CVec4(1,1,1,1);
		c[2] = CVec4(0.5f, 1, 2, 4);
		c[3] = CVec4( 2.0f * 255 / 254, -256.0f / 254, 7, 0 );
		c[4] = CVec4(0.25, 1/16.0, 0, 4096 );
		//c[5] = ?
		c[6] = CVec4( 1.0f / N_VEC_FULL_TEX_SIZE, 1.0f/65536, 0.5f, 0 );
		c[7] = GetRegisterMapScale();
		SetVSConst( 0, &c[0], 8 );
	}
	CObj<CObjectBase> pQ = CreateOcclusionQuery();
	bDoesSupportOcclusionQueries = pQ != 0;
	pQ = CreateEventQuery();
	bDoesSupportEventQueries = pQ != 0;
}

void InitRender()
{
	InitEffects();
	ditheringMode.DoApply();
	wireframeMode.DoApply();
	transformMode.DoApply();
	alphaMode.DoApply();
	stencilMode.DoApply();
	depthMode.DoApply();
	cullMode.DoApply();
	colorMode.DoApply();
	fogParamsMode.DoApply();
	fogMode.DoApply();
	//
	HRESULT hr;
	hr = pDevice->BeginScene();
	D3DASSERT( hr, "BeginScene failed" );
}

void DoneEffects()
{
	hlslPixelShaders.clear();
	hlslVertexShaders.clear();
	ZERO_ARRAY( pixelShaders );
	ZERO_ARRAY( vertexShaders );
	ZERO_ARRAY( vertexDeclarations );
}


bool IsUingRegisters()
{
	return rtInfo.nRegisters > 0;
}
void DoneRender()
{
	HRESULT hr;
	hr = pDevice->EndScene();
	D3DASSERT( hr, "EndScene failed" );
	pScreenColor = 0;
	pScreenDepth = 0;
	DoneShaderFX();
	DoneEffects();
	pDevice->SetVertexShader( 0 );
	pDevice->SetPixelShader( 0 );
	pDevice->SetVertexDeclaration( 0 );
}

START_REGISTER(GfxRender)
	REGISTER_VAR_EX( "gfx_lag", NGlobal::VarIntHandler, &nMaxLag, 1, STORAGE_USER )
	REGISTER_VAR_EX( "gfx_register_resolution", NGlobal::VarFloatHandler, &fRegisterResolution, 1.0f, STORAGE_USER )
FINISH_REGISTER
}


