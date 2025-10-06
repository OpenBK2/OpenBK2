#include "stdafx.h"
#include "GfxUtils.h"
#include "GfxShaders.h"
#include "3DLib/Transform.h"
namespace NGfx
{

// universal rects buffer

static std::vector<STriangle> universalRectsBuffer;

static void RefreshUniversalRectTrisBuffer()
{
	if ( !universalRectsBuffer.empty() )
		return;
	universalRectsBuffer.resize( N_MAX_RECTANGLES * 2 );
	for ( int i = 0; i < N_MAX_RECTANGLES; ++i )
	{
		WORD wStart = i * 4;
		universalRectsBuffer[i*2  ] = STriangle( wStart + 0, wStart + 1, wStart + 2 );
		universalRectsBuffer[i*2+1] = STriangle( wStart + 0, wStart + 2, wStart + 3 );
	}
}

static CObj<CTexture> pWhiteTexture;
static CTexture* GetWhiteTexture()
{
	if ( IsValid( pWhiteTexture ) )
		return pWhiteTexture;
	if ( Is16BitTextures() )
	{
		pWhiteTexture = MakeTexture( 3, 3, 1, SPixel4444::ID, TEXTURE_2D, CLAMP );
		CTextureLock<SPixel4444> lock( pWhiteTexture, 0, NGfx::INPLACE );
		for ( int y = 0; y < lock.GetSizeY(); ++y )
		{
			for ( int x = 0; x < lock.GetSizeX(); ++x )
				lock[y][x] = SPixel4444( 15, 15, 15, 15 );
		}
	}
	else
	{
		pWhiteTexture = MakeTexture( 3, 3, 1, SPixel8888::ID, TEXTURE_2D, CLAMP );
		CTextureLock<SPixel8888> lock( pWhiteTexture, 0, NGfx::INPLACE );
		for ( int y = 0; y < lock.GetSizeY(); ++y )
		{
			for ( int x = 0; x < lock.GetSizeX(); ++x )
				lock[y][x] = SPixel8888( 255, 255, 255, 255 );
		}
	}
	return pWhiteTexture;
}

void MakeQuadTriList( int nRects, STriangleList *pRes )
{
	ASSERT( nRects <= N_MAX_RECTANGLES );
	RefreshUniversalRectTrisBuffer();
	*pRes = STriangleList( &universalRectsBuffer[0], (std::min)( nRects, N_MAX_RECTANGLES ) * 2, 0 );
}

// C2DQuadsRenderer

typedef SGeomVecFull SRectVertex;
struct SFakeCPPInitOrder
{
	CObj<CGeometry> pGeom;
};
const int N_RECTS_PER_BUF = 128;
struct S2DRectInfoLock : public SFakeCPPInitOrder
{
	int nRects;

	virtual ~S2DRectInfoLock() {}
	bool IsFull() const { return nRects == N_RECTS_PER_BUF; }
	int GetRectsNum() const { return nRects; }
};
template<class T>
struct SRealRectInfoLock : public S2DRectInfoLock
{
	CBufferLock<T> data;

	SRealRectInfoLock(): data(&pGeom, N_RECTS_PER_BUF * 4) { nRects = 0; }
	T* AddRect() { return &data[ (nRects++) * 4 ];}
};

C2DQuadsRenderer::C2DQuadsRenderer( const NGfx::CRenderContext &_rc, const CVec2 &vSize, int _dm )
: dm(_dm), rc(_rc), pLock(0)
{
	SetupRC( vSize );
}

void C2DQuadsRenderer::SetTarget( const NGfx::CRenderContext &_rc, const CVec2 &vSize, int _dm )
{
	ASSERT( pLock == 0 );
	Flush();
	dm = _dm;
	rc = _rc;
	SetupRC( vSize );
}

void C2DQuadsRenderer::SetTarget( NGfx::CTexture *pTarget, const CVec2 &vSize, int _dm )
{
	ASSERT( pLock == 0 );
	Flush();
	dm = _dm;
	rc.SetAlphaCombine( NGfx::COMBINE_NONE );
	rc.SetStencil( NGfx::STENCIL_NONE );
	if ( pTarget )
		rc.SetTextureRT( pTarget );
	SetupRC( vSize );
}

void C2DQuadsRenderer::SetTarget( const CVec2 &vSize, int _dm )
{
	ASSERT( pLock == 0 );
	Flush();
	dm = _dm;
	rc.SetAlphaCombine( NGfx::COMBINE_NONE );
	rc.SetStencil( NGfx::STENCIL_NONE );
	SetupRC( vSize );
}

void C2DQuadsRenderer::SetupRC( const CVec2 &vSize )
{
	CTransformStack ts;
	ts.MakeDirect( vSize );
	rc.SetTransform( ts.Get() );

	switch ( dm & QRM_STENCIL_MASK )
	{
		case QRM_KEEP_STENCIL: break;
		case QRM_TEST_STENCIL: rc.SetStencil( NGfx::STENCIL_TEST, 0x80, 0x80 ); break;
		default: ASSERT(0); break;
	}
	switch ( dm & QRM_DEPTH_MASK )
	{
		case QRM_DEPTH_NONE: rc.SetDepth( NGfx::DEPTH_NONE ); break;
		case QRM_OVERWRITE: rc.SetDepth( NGfx::DEPTH_OVERWRITE ); break;
		case QRM_DEPTH_NORMAL: rc.SetDepth( NGfx::DEPTH_NORMAL ); break;
		default: ASSERT(0); break;
	}
}

const int N_UV_PER_TEXEL_STEPS = 8;
S2DRectInfoLock* C2DQuadsRenderer::GetRectInfoLock( CTexture *pContainer, const STexturePlaceInfo &region )
{
	if ( pContainer != pPrevContainer || ( pLock && pLock->IsFull() ) )
		Flush();
	if ( pLock == 0 )
	{
		if ( IsTnLDevice() )
		{
			fUVMult = 1;
			pLock = new SRealRectInfoLock<SGeomVecT2C1>();
		}
		else
		{
			fUVMult = 1.0f / N_UV_PER_TEXEL_STEPS;
			pLock = new SRealRectInfoLock<SRectVertex>();
		}
		pPrevContainer = pContainer;
	}
	if ( NGfx::IsNVidiaNP2Bug() && !region.IsHolderAPow2Texture() )
	{
		fScaleU = fUVMult;
		fScaleV = fUVMult;
	}
	else
	{
		fScaleU = fUVMult / (std::max)( 1, region.size.x );
		fScaleV = fUVMult / (std::max)( 1, region.size.y );
	}
	return pLock;
}

static void FillVertex( SRectVertex *pRes, float x, float y, float u, float v, DWORD dwColor, float fZ )
{
	pRes->pos.x = x;
	pRes->pos.y = y;
	pRes->pos.z = fZ;
	pRes->normal.dw = 0;
	pRes->tex.nU = Float2Int( u * N_UV_PER_TEXEL_STEPS );
	pRes->tex.nV = Float2Int( v * N_UV_PER_TEXEL_STEPS );
	//CalcTexCoords( &pRes->tex, u, v );
	pRes->texLM.dw = 0;
	pRes->texU.dw = dwColor;
	pRes->texV.dw = 0;
}

static void FillVertex( SGeomVecT2C1 *pRes, float x, float y, float u, float v, DWORD dwColor, float fScaleU, float fScaleV, float fZ )
{
	pRes->pos.x = x;
	pRes->pos.y = y;
	pRes->pos.z = fZ;
	pRes->color.dwColor = dwColor;
	pRes->tex1.x = u * fScaleU;
	pRes->tex1.y = v * fScaleV;
	pRes->tex2.x = 0;
	pRes->tex2.y = 0;
}

void C2DQuadsRenderer::FillRect( const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, const CTRect<float> &rSrc, float fXAdd, float fYAdd, float fZ, CTexture *pContainer, const STexturePlaceInfo &region )
{
	fZ = fZ * 998.0f + 1.0f; // fZ in [0...1]
	if ( IsTnLDevice() )
	{
		SGeomVecT2C1 *pGeom = ((SRealRectInfoLock<SGeomVecT2C1>*)GetRectInfoLock( pContainer, region ))->AddRect();
		FillVertex( pGeom + 0, pPos4[0].x - 0.5f, pPos4[0].y - 0.5f, fXAdd + rSrc.x1, fYAdd + rSrc.y1, pColors4[0].dwColor , fScaleU, fScaleV, fZ );
		FillVertex( pGeom + 1, pPos4[1].x - 0.5f, pPos4[1].y - 0.5f, fXAdd + rSrc.x1, fYAdd + rSrc.y2, pColors4[1].dwColor , fScaleU, fScaleV, fZ );
		FillVertex( pGeom + 2, pPos4[2].x - 0.5f, pPos4[2].y - 0.5f, fXAdd + rSrc.x2, fYAdd + rSrc.y2, pColors4[2].dwColor , fScaleU, fScaleV, fZ );
		FillVertex( pGeom + 3, pPos4[3].x - 0.5f, pPos4[3].y - 0.5f, fXAdd + rSrc.x2, fYAdd + rSrc.y1, pColors4[3].dwColor , fScaleU, fScaleV, fZ );
	}
	else
	{
		SRectVertex *pGeom = ((SRealRectInfoLock<SRectVertex>*)GetRectInfoLock( pContainer, region ))->AddRect();
		FillVertex( pGeom + 0, pPos4[0].x - 0.5f, pPos4[0].y - 0.5f, fXAdd + rSrc.x1, fYAdd + rSrc.y1, pColors4[0].dwColor , fZ );
		FillVertex( pGeom + 1, pPos4[1].x - 0.5f, pPos4[1].y - 0.5f, fXAdd + rSrc.x1, fYAdd + rSrc.y2, pColors4[1].dwColor , fZ );
		FillVertex( pGeom + 2, pPos4[2].x - 0.5f, pPos4[2].y - 0.5f, fXAdd + rSrc.x2, fYAdd + rSrc.y2, pColors4[2].dwColor , fZ );
		FillVertex( pGeom + 3, pPos4[3].x - 0.5f, pPos4[3].y - 0.5f, fXAdd + rSrc.x2, fYAdd + rSrc.y1, pColors4[3].dwColor , fZ );
	}
}

void C2DQuadsRenderer::AddRect( const CTRect<float> &_rTarget, NGfx::CTexture *pTex, const CTRect<float> &_rSrc, 
	SPixel8888 color, float fZ )
{
	float fXAdd = 0, fYAdd = 0;
	STexturePlaceInfo region;
	CTexture *pContainer = 0;
	if ( ( dm & QRM_RENDER_MASK ) != QRM_SOLID )
	{
		if ( !pTex )
		{
			AddRect( _rTarget, GetWhiteTexture(), CTRect<float>(1,1,2,2), color, fZ );
			return;
		}
		pContainer = GetTextureContainer( pTex, &region );
		if ( pContainer )
		{
			fXAdd = region.place.x1;
			fYAdd = region.place.y1;
		}
	}
	// determine true target rect
	CTRect<float> rTarget(_rTarget);
	rTarget.x1 = Float2Int( rTarget.x1 );
	rTarget.x2 = Float2Int( rTarget.x2 );
	rTarget.y1 = Float2Int( rTarget.y1 );
	rTarget.y2 = Float2Int( rTarget.y2 );
	// fix scaling rects with target adjust
	CTRect<float> rSrc(_rSrc);
	float fWidth = rTarget.x2 - rTarget.x1, fWidthA = fabsf(fWidth);
	float fTexWidth = rSrc.x2 - rSrc.x1, fTexWidthA = fabsf( fTexWidth );
	if ( fWidthA != fTexWidthA )
	{
		float fU = 0.5f * ( rSrc.x1 + rSrc.x2 );
		if ( fWidthA > fTexWidthA && fTexWidthA > 0 && fWidthA > 1 )
		{
			float fScale = ( fTexWidthA - 1 ) / fTexWidthA * fWidthA / ( fWidthA - 1 );
			rSrc.x1 = fU + (rSrc.x1 - fU ) * fScale;
			rSrc.x2 = fU + (rSrc.x2 - fU ) * fScale;
		}
	}
	float fHeight = rTarget.y2 - rTarget.y1, fHeightA = fabsf(fHeight);
	float fTexHeight = rSrc.y2 - rSrc.y1, fTexHeightA = fabsf( fTexHeight );
	if ( fHeightA != fTexHeightA )
	{
		float fU = 0.5f * ( rSrc.y1 + rSrc.y2 );
		if ( fHeightA > fTexHeightA && fTexHeightA > 0 && fHeightA > 1 )
		{
			float fScale = ( fTexHeightA - 1 ) / fTexHeightA * fHeightA / ( fHeightA - 1 );
			rSrc.y1 = fU + (rSrc.y1 - fU ) * fScale;
			rSrc.y2 = fU + (rSrc.y2 - fU ) * fScale;
		}
	}
	////
	CVec2 sPos[4] =
	{
		CVec2( rTarget.x1, rTarget.y1 ),
		CVec2( rTarget.x1, rTarget.y2 ),
		CVec2( rTarget.x2, rTarget.y2 ),
		CVec2( rTarget.x2, rTarget.y1 )
	};
	NGfx::SPixel8888 sColors[4] = { color, color, color, color };
	FillRect( sPos, sColors, rSrc, fXAdd, fYAdd, fZ, pContainer, region );
}

void C2DQuadsRenderer::AddRect( const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, NGfx::CTexture *pTex, const CTRect<float> &rSrc, float fZ )
{
	float fXAdd = 0, fYAdd = 0;
	STexturePlaceInfo region;
	CTexture *pContainer = 0;
	if ( ( dm & QRM_RENDER_MASK ) != QRM_SOLID )
	{
		if ( !pTex )
		{
			AddRect( pPos4, pColors4, GetWhiteTexture(), CTRect<float>( 1, 1, 2, 2 ), fZ );
			return;
		}
		pContainer = GetTextureContainer( pTex, &region );
		if ( pContainer )
		{
			fXAdd = region.place.x1;
			fYAdd = region.place.y1;
		}
	}
	////
	CVec2 sPos[4];
	for ( int nTemp = 0; nTemp < ARRAY_SIZE( sPos ); ++nTemp )
	{
		sPos[nTemp].x = Float2Int( pPos4[nTemp].x );
		sPos[nTemp].y = Float2Int( pPos4[nTemp].y );
	}
	FillRect( sPos, pColors4, rSrc, fXAdd, fYAdd, fZ, pContainer, region );
}

void C2DQuadsRenderer::Flush()
{
	if ( !pLock )
		return;
	ASSERT( pPrevContainer || (dm & QRM_RENDER_MASK) == QRM_SOLID );
	rc.Use();
	if ( IsTnLDevice() )
	{
		switch ( dm & QRM_RENDER_MASK )
		{
		case QRM_SIMPLE:
			{
				rc.SetPixelShader( psDiffuseTexture );
				rc.SetVertexShader( NGfx::TNLVS_NONE );
				rc.SetTexture( 0, pPrevContainer, FILTER_LINEAR );
			}
			break;
		case QRM_NOCOLOR:
			{
				rc.SetPixelShader( psTextureCopyAlpha );
				rc.SetVertexShader( NGfx::TNLVS_NONE );
				rc.SetTexture( 0, pPrevContainer, FILTER_LINEAR );
			}
			break;
		case QRM_SOLID:
			{
				rc.SetPixelShader( psDiffuse );
				rc.SetVertexShader( NGfx::TNLVS_NONE );
			}
			break;
		default: ASSERT(0); break;
		}
	}
	else
	{
		switch ( dm & QRM_RENDER_MASK )
		{
		case QRM_SIMPLE:
			{
				rc.SetPixelShader( psDiffuseTexture );
				rc.SetVertexShader( vsRender2D );
				rc.SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
				rc.SetTexture( 0, pPrevContainer, FILTER_LINEAR );
			}
			break;
		case QRM_NOCOLOR:
			{
				rc.SetPixelShader( psTextureCopyAlpha );
				//rc.SetVertexShader( vsTextureScale );
				rc.SetVertexShader( vsRender2D );//TextureScale );
				rc.SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
				rc.SetTexture( 0, pPrevContainer, FILTER_LINEAR );
			}
			break;
		case QRM_SOLID:
			{
				rc.SetPixelShader( psDiffuse );
				rc.SetVertexShader( vsDrawTexU );
			}
			break;
		case QRM_USER_EFFECT:
			{
				ASSERT( IsValid(pUserEffect) );
				if ( IsValid(pUserEffect) )
					pUserEffect->SetEffect( &rc, pPrevContainer, fScaleU, fScaleV );
			}
			break;
		default: ASSERT(0); break;
		}
	}
	CObj<CGeometry> pGeom( pLock->pGeom );
	STriangleList triList;
	MakeQuadTriList( pLock->GetRectsNum(), &triList );
	delete pLock;
	pLock = 0;
	rc.DrawPrimitive( pGeom, triList );
}


void CopyTexture( const NGfx::CRenderContext &_rc, const CVec2 &vTargetViewport, const CTRect<float> &rTarget, 
	NGfx::CTexture *pTex, const CTRect<float> &rSrc, const CVec4 &vColor, I2DEffect *pEffect )
{
	NGfx::SPixel8888 color;
	color.dwColor = GetDWORDColor( vColor );
	if ( pEffect )
	{
		C2DQuadsRenderer r( _rc, vTargetViewport, QRM_DEPTH_NONE|QRM_USER_EFFECT );
		r.SetUserEffect( pEffect );
		r.AddRect( rTarget, pTex, rSrc, color );
	}
	else
	{
		int dm = color.dwColor == 0xffffffff ? QRM_NOCOLOR|QRM_DEPTH_NONE : QRM_DEPTH_NONE;
		C2DQuadsRenderer r( _rc, vTargetViewport, dm );
		r.AddRect( rTarget, pTex, rSrc, color );
	}
}

static void AddFullTextureRect( C2DQuadsRenderer *pR, NGfx::CTexture *pTex )
{
	CDynamicCast<I2DBuffer> p2D( pTex );
	int nXSize = p2D->GetSizeX(), nYSize = p2D->GetSizeY();
	CTRect<float> r;
	r.x1 = 0; r.x2 = nXSize;
	r.y1 = 0; r.y2 = nYSize;
	pR->AddRect( r, pTex, r );
}

void ShowTexture( NGfx::CTexture *pTex, float fMag )
{
	C2DQuadsRenderer rend;
	rend.SetTarget( CVec2( 800 / fMag, 600 / fMag ), QRM_NOCOLOR );
	AddFullTextureRect( &rend, pTex );
}

void ShowTexture( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, const CVec2 &vScreenSize )
{
	CObj<NGfx::CTexture> pHold(pTex);
	pRC->SetAlphaCombine( NGfx::COMBINE_ALPHA );
	C2DQuadsRenderer rend;
	rend.SetTarget( *pRC, vScreenSize, QRM_NOCOLOR );
	AddFullTextureRect( &rend, pTex );
}

// CShowAlphaEffect

void CShowAlphaEffect::SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
{
	pRC->SetPixelShader( psTextureAlpha );
	pRC->SetVertexShader( vsRender2D );
	pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
	pRC->SetTexture( 0, pTex, FILTER_LINEAR );
}

// CLinearToGammaEffect

void CLinearToGammaEffect::SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
{
	pRC->SetPixelShader( psLinearToGamma );
	pRC->SetVertexShader( vsRender2D );
	pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
	pRC->SetPSConst( 0, CVec4( 1 / 2.4f, 0, 0, 0 ) );
	pRC->SetTexture( 0, pTex, FILTER_LINEAR );
}

void CShadow16toAlphaEffect::SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
{
	pRC->SetPixelShader( psDp3 );
	pRC->SetVertexShader( vsRender2D );
	pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
	pRC->SetPSConst( 0, CVec4( 0, 252.0/256, 1.0/8, 0 ) );
	pRC->SetTexture( 0, pTex, FILTER_POINT );//FILTER_LINEAR );
}

void CCopyShadowsAndCloudsEffect::SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
{
    pRC->SetPixelShader( psCloudShadows );
	pRC->SetVertexShader( vsRender2DClouds );
	pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
	pRC->SetVSConst( 17, vMAD );

	pRC->SetPSConst( 0, CVec4( 0, 252.0/256, 1.0/8, 0 ) );
	pRC->SetTexture( 0, pTex, FILTER_POINT );//FILTER_LINEAR );
	//ASSERT ( pCloud );
	pRC->SetTexture( 1, pCloud, FILTER_POINT );
}
}

