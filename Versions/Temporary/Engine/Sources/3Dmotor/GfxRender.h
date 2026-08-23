#pragma once
#include "3Dmotor_export.h"

//
#include "GPixelFormat.h"

#include "pciids/gpus.h"

#include <cstdint>

struct SPShader;
struct SVShader;
struct SHLSLShader;
//
namespace NGfx
{
enum EHardwareLevel
{
	HL_TNL_DEVICE,
	HL_GFORCE3,
	HL_RADEON2,
	HL_R300
};

EVideoCard GetVideoCard();
EHardwareLevel GetHardwareLevel();
bool IsTnLDevice();

enum EAlphaCombineMode
{
	COMBINE_NONE,
	COMBINE_ADD,
	COMBINE_MUL,
	COMBINE_MUL2,
	COMBINE_ALPHA,
	COMBINE_ALPHA_ADD,
	COMBINE_ZERO_ONE,
	COMBINE_SRC_ALPHA_MUL,
	COMBINE_SMART_ALPHA,
	COMBINE_ADD_SRC_ALPHA_MUL,
};
enum EStencilMode
{
	STENCIL_NONE,
	STENCIL_INCR,
	STENCIL_DECR,
	STENCIL_TESTINCR,
	STENCIL_TESTDECR,
	//STENCIL_TESTWRITE,
	STENCIL_TEST,
	STENCIL_TESTNE_WRITE,
	STENCIL_TEST_CLEAR, // will write anyway
	STENCIL_TESTNE_CLEAR, // will write anyway
	STENCIL_WRITE,
	//STENCIL_TEST,
	//STENCIL_INVERT,
	//STENCIL_NOTEQUAL,
	STENCIL_TEST_REPLACE,
	STENCIL_GREATER
};
enum EDepthMode
{
	DEPTH_NONE,
	DEPTH_NORMAL,
	DEPTH_EQUAL,
	DEPTH_OVERWRITE,
	DEPTH_TESTONLY,
	DEPTH_INVERSETEST,
	DEPTH_GREATEEQTEST,
	DEPTH_NORMAL_NOTEQ
};
enum ECullMode
{
	CULL_NONE,
	CULL_CW,
	CULL_CCW
};
enum EColorWriteMask
{
	COLORWRITE_NONE  = 0,
	COLORWRITE_RED   = 1,
	COLORWRITE_GREEN = 2,
	COLORWRITE_BLUE  = 4,
	COLORWRITE_ALPHA = 8,
	COLORWRITE_COLOR = 7,
	COLORWRITE_ALL   = 15
};
enum EWireframe
{
	WIREFRAME_OFF,
	WIREFRAME_ON
};
enum EDithering
{
	DITHER_OFF,
	DITHER_ON
};
enum EFogMode
{
	FOG_NONE,
	FOG_BLACK,
	FOG_NORMAL
};
enum EFilterMode
{
	FILTER_POINT,
	FILTER_LINEAR,
	FILTER_BUMP,
	FILTER_BEST
};
struct SStencilMode
{
	EStencilMode mode;
	int nVal, nMask;

	SStencilMode() {}
	SStencilMode( EStencilMode _mode, int _nVal = 0, int _nMask = 0xffffffff ): mode(_mode), nVal(_nVal), nMask(_nMask) {}
	bool operator==( const SStencilMode &m ) const { return mode == m.mode && nVal == m.nVal && nMask == m.nMask; }
};
struct SFogParams
{
	CVec3 vColor;
	float fMinDist, fMaxDist;
	float fMinZDis, fMaxZDis;

	SFogParams() : vColor(0,0,0), fMinDist(0), fMaxDist(1e20f), fMinZDis( -10000 ), fMaxZDis( -100001 ) {}
	SFogParams( const CVec3 &_vColor, float _fMinDist, float _fMaxDist, float _fMinZDis, float _fMaxZDis ) : vColor(_vColor), fMinDist(_fMinDist), fMaxDist(_fMaxDist), fMinZDis(_fMinZDis),  fMaxZDis(_fMaxZDis) {}
	bool operator==( const SFogParams &m ) const { return vColor == m.vColor && fMinDist == m.fMinDist && fMaxDist == m.fMaxDist; }
};

enum ETnLVS
{
	TNLVS_NONE = 1,
	TNLVS_VERTEX_COLOR,
	TNLVS_VERTEX_COLOR_AND_ALPHA,
	TNLVS_TEXTRANS
};

class CTexture;
class CCubeTexture;
class CGeometry;
class CTriList;
struct S3DTriangle;
class CPixelShader;
class CVertexShader;

enum EFace : int;

class IQuery : public CObjectBase
{
public:
	virtual void Start() = 0;
	virtual void Finish() = 0;
	virtual int GetData() = 0;
	virtual void Flush() = 0;
};

class _3DMOTOR_EXPORT CRenderContext
{
	enum ERenderTargetMode
	{
		RTM_SCREEN,
		RTM_TEXTURE,
		RTM_REGISTERS,
		RTM_CUBETEXTURE
	};
	SFBTransform transform;
	EAlphaCombineMode alpha;
	SStencilMode stencil;
	EColorWriteMask colorWrite;
	EDepthMode depth;
	ECullMode cull;
	SFogParams fogParams;
	EFogMode fog;
	ERenderTargetMode targetMode;
	CObj<CTexture> pTarget;
	CObj<CCubeTexture> pCubeTarget;
	int nMipLevel, nRegister;
	//CTRect<int> rViewport;
	const SPShader *pPixelShader;
	int nVertexShader;
	void *pOutstandingStream; // 0 if no geometry is in fly

	CObj<CPixelShader> pPShader;
	CObj<CVertexShader> pVShader;



	void ApplyRenderTarget() const;
	void StartStream( CGeometry *pGeom );
	void CheckStream( CGeometry *pGeom );
public:
	CRenderContext();
	~CRenderContext();
	void SetTransform( const SFBTransform &_transform );
	void SetAlphaCombine( EAlphaCombineMode mode );
	void SetStencil( const SStencilMode &m );
	void SetStencil( EStencilMode m, int nVal = 0, int nMask = 0xffffffff ) { SetStencil( SStencilMode( m, nVal, nMask ) ); }
	void SetDepth( EDepthMode mode );
	void SetCulling( ECullMode mode );
	void SetColorWrite( EColorWriteMask mode );
	void SetFogParams( const SFogParams &mode );
	void SetFog( EFogMode mode );
	const SFogParams & GetFogParams( ) const { return fogParams;}
	const EFogMode GetFogMode() const { return fog; }

	//
	void SetScreenRT();
	void SetTextureRT( CTexture *pTexture, int nMipLevel = 0 );
	void SetCubeTextureRT( CCubeTexture *pTexture, EFace nFace, int nMipLevel = 0 );
	//void SetVirtualRT( const CTRect<int> &size ); // registers available only in this mode
	void SetVirtualRT(); // screen size virtual RT
	void SetRegister( int nRegister );
	void ClearBuffers( uint32_t dwColor = 0x808080 );
	void ClearTarget( uint32_t dwColor = 0x808080 );
	void ClearZBuffer();
	//
	bool HasRegisters() const { return targetMode == RTM_REGISTERS; }
	// functions to be used in effect initialisation
	void SetPixelShader( const std::string &szName );
	void SetPixelShader( const SPShader &pShader );
	void SetVertexShader( const std::string &szName );
	void SetVertexShader( const SVShader &pShader );


	void SetVertexShader( ETnLVS shader );
	bool SetShader( const SHLSLShader &pShader );
	void SetVSConst( int nReg, const CVec4 *pData, int nSize ) const;
	void SetVSConst( int nReg, const CVec3 &a ) const;
	void SetVSConst( int nReg, const CVec4 &a ) const { SetVSConst( nReg, &a, 1 ); }
	void SetTnlVertexColor( const CVec4 &a );
	void SetTnlTexTransform( const SHMatrix &m );
	void SetPSConst( int nReg, const CVec4 *pData, int nSize );
	void SetPSConst( int nReg, const CVec3 &a );
	void SetPSConst( int nReg, const CVec4 &a ) { SetPSConst( nReg, &a, 1 ); }
	void SetAlphaRef( int nRef );
	void SetTexture( int nStage, CTexture *pTex, EFilterMode filter );
	void SetTexture( int nStage, CCubeTexture *pTex );
	void Use() const;
	template<class T>
		void SetEffect( T *p ) { Use(); p->Use( this ); }
	//
	const SFBTransform& GetTransform() const { return transform; }
	//
	void DrawPrimitive( CGeometry *pGeom, CTriList *pTris, int nStartVertex, int nVertices );
	void DrawPrimitive( CGeometry *pGeom, CTriList *pTris ) { AddPrimitive( pGeom, pTris ); Flush(); }
	void DrawPrimitive( CGeometry *pGeom, const STriangleList &tris ) { AddPrimitive( pGeom, tris ); Flush(); }
	//void DrawPrimitive( CGeometry *pGeom, const vector<STriangle> &tris ) { AddPrimitive( pGeom, tris ); Flush(); }
	void AddPrimitive( CGeometry *pGeom, CTriList *pTris );
	//void AddPrimitive( CGeometry *pGeom, const vector<STriangle> &tris );
	void AddPrimitive( CGeometry *pGeom, const STriangleList &tris );
	void AddPrimitive( CGeometry *pGeom, const STriangleList *pTris, int nCount, unsigned nMask );
	void Flush();
	void AddLineStrip( CGeometry *pGeom, const unsigned short *pIndices, int nLines );
};

CTexture* GetRegisterTexture( int nRegister );
CTexture* GetDepthRegisterTexture();
bool CopyScreenToRegister( int nRegister );
void CopyScreenToTexture( CTexture *pTex );
IQuery* CreateOcclusionQuery();
void SetWireframe( EWireframe wire );
void GetRegisterSize( CTRect<float> *pRes );
bool IsNVidiaNP2Bug();
bool DoesSupportOcclusionQueries();
void SetDithering( EDithering a );

}
