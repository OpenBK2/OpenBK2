#include "stdafx.h"
#include "GRenderClouds.h"
#include "GfxRender.h"
#include "GfxBuffers.h"
#include "GfxShaders.h"
#include "GRenderCore.h"
//#include "GRenderFactor.h"

namespace NGScene
{
static CObj<NGfx::CGeometry> pCloudGrid;
static NGfx::STriangleList cloudTris;
static std::vector<STriangle> cloudTrisBuf;

const int N_GRID_SIZE = 64;
static void RefreshGeometry()
{
	if ( IsValid(pCloudGrid) )
		return;
	NGfx::CBufferLock<NGfx::SGeomVecFull> geom( &pCloudGrid, N_GRID_SIZE * N_GRID_SIZE );
	NGfx::SGeomVecFull v;
	Zero( v );
	for ( int y = 0; y < N_GRID_SIZE; ++y )
	{
		for ( int x = 0; x < N_GRID_SIZE; ++x )
		{
			float fX = ((float)x) / ( N_GRID_SIZE - 1 );
			float fY = ((float)y) / ( N_GRID_SIZE - 1 );
			v.pos = CVec3( fX, fY, 0 );
			geom[ y * N_GRID_SIZE + x ] = v;
		}
	}

	cloudTrisBuf.clear();
	for ( int y = 0; y < N_GRID_SIZE - 1; ++y )
	{
		for ( int x = 0; x < N_GRID_SIZE - 1; ++x )
		{
			int nBase = y * N_GRID_SIZE + x;
			cloudTrisBuf.push_back( STriangle( nBase, nBase + 1, nBase + N_GRID_SIZE + 1 ) );
			cloudTrisBuf.push_back( STriangle( nBase, nBase + N_GRID_SIZE + 1, nBase + N_GRID_SIZE ) );
		}
	}
	cloudTris.nBaseIndex = 0;
	cloudTris.nOffset = 0;
	cloudTris.nTris = cloudTrisBuf.size();
	cloudTris.pTri = &cloudTrisBuf[0];
}

void RenderClouds( NGfx::CRenderContext *pRC, const SPerspDirectionalDepthInfo &renderInfo, const SPerspDirectionalDepthInfo &depthInfo, NGfx::CTexture *pCloud, const SHMatrix &proj )
{
	if ( !IsValid(pCloud) )
		return;
	NGfx::CRenderContext rc(*pRC);
	rc.SetDepth( NGfx::DEPTH_NONE );
	rc.SetAlphaCombine( NGfx::COMBINE_NONE );
	rc.SetStencil( NGfx::STENCIL_NONE );
	rc.SetColorWrite( NGfx::COLORWRITE_COLOR );
	rc.SetCulling( NGfx::CULL_NONE );
	rc.SetFog( NGfx::FOG_NONE );
	RefreshGeometry();
	rc.Use();
	rc.SetVSConst( 24, renderInfo.vDepth );
	rc.SetVSConst( 25, renderInfo.nlp.vTexU );
	rc.SetVSConst( 26, renderInfo.nlp.vTexV );
	rc.SetVSConst( 27, renderInfo.nlp.vShift );
	rc.SetVSConst( 28, proj.x() );
	rc.SetVSConst( 29, proj.y() );
	
	const SNLProjectionInfo &nlp = depthInfo.nlp;
	float fDet = 1 / ( nlp.vTexU.x * nlp.vTexV.y - nlp.vTexU.y * nlp.vTexV.x );
	float fCx = -( + nlp.vTexV.y * nlp.vTexU.w - nlp.vTexU.y * nlp.vTexV.w ) * fDet;
	float fCy = -( - nlp.vTexV.x * nlp.vTexU.w + nlp.vTexU.x * nlp.vTexV.w ) * fDet;
	CVec4 vC( fCx, fCy, 0, 1 );
	CVec4 vU( nlp.vTexV.y * fDet, -nlp.vTexV.x * fDet, 0, 0 );
	CVec4 vV(-nlp.vTexU.y * fDet,  nlp.vTexU.x * fDet, 0, 0 );

	CVec4 v1 = vC + vU, v2 = vC + vV, v12 = vC + vU + vV;
	rc.SetVSConst( 30, vC + vU * nlp.vMinMax.x + vV * nlp.vMinMax.z );
	rc.SetVSConst( 31, vU * ( nlp.vMinMax.y - nlp.vMinMax.x ) );
	rc.SetVSConst( 32, vV * ( nlp.vMinMax.w - nlp.vMinMax.z ) );
	rc.SetPixelShader( psTextureCopyAlpha );
	rc.SetVertexShader( vsRenderClouds );

	//pCloud = GetCheckerTexture();
	rc.SetTexture( 0, pCloud, NGfx::FILTER_BEST );

	rc.DrawPrimitive( pCloudGrid, cloudTris );
}

// CCloudMover

void CCloudMover::Recalc()
{
	STime t = pTime->GetValue();
	Identity( &value );
	float fTexTime = 1000 / fSpeed;
	float fScroll = t / fTexTime;
	float fX = cos( ToRadian( fAngle ) ), fY = sin( ToRadian( fAngle ) );
	float fDeltaX = fmod( fX * fScroll, 1 );
	float fDeltaY = fmod( fY * fScroll, 1 );
	value.SetX(CVec4( 1 / vWrapSize.x, 0, 0, fDeltaX ));
	value.SetY(CVec4( 0, 1 / vWrapSize.y, 0, fDeltaY ));
}
}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x20162300, CCloudMover )

