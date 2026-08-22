#include "stdafx.h"
#include "GRenderFactor.h"
#include "GRenderExecute.h"
#include "GfxRender.h"
#include "3DLib/Transform.h"
#include "GfxShaders.h"
#include "System/WorkerPool.h"
#include "System/Commands.h"

#include <algorithm>

namespace NGScene
{
extern bool bNewShadows;
extern bool bLowRAM;
static int nRenderWorkerThreads = 4;
static CWorkerPool *pRenderWorkerPool = 0;

CWorkerPool& GetRenderWorkerPool()
{
	const std::size_t nRequestedThreads = static_cast<std::size_t>(
		(std::max)( 0, nRenderWorkerThreads ) );
	if ( !pRenderWorkerPool )
		pRenderWorkerPool = new CWorkerPool( nRequestedThreads );
	else if ( pRenderWorkerPool->GetThreadCount() != nRequestedThreads )
		pRenderWorkerPool->SetThreadCount( nRequestedThreads );
	return *pRenderWorkerPool;
}

void ShutdownRenderWorkerPool()
{
	// Stop and join the persistent workers before the renderer module is unloaded.
	delete pRenderWorkerPool;
	pRenderWorkerPool = 0;
}

static void PrepareVisibleGeometry( const CSceneFragments &scene )
{
	CWorkerPool &workerPool = GetRenderWorkerPool();
	std::vector<SRenderGeometryInfo*> geometry;
	const int nGeometries = scene.GetGeometriesNum();
	geometry.reserve( nGeometries );
	for ( int k = 0; k < nGeometries; ++k )
	{
		if ( scene.GetGeometryFlags( k ) == FST_REJECT )
			continue;
		SRenderGeometryInfo *pGeometry = scene.GetGeometryInfo( k );
		geometry.push_back( pGeometry );
		pGeometry->pVertices->QueueParallelRecalc( workerPool );
	}

	// All CPU tasks finish before the render thread creates or locks D3D9 buffers.
	workerPool.WaitForAll();
	for ( SRenderGeometryInfo *pGeometry : geometry )
		pGeometry->pVertices.Refresh();
}

struct SCompareOps
{
	bool operator()( const CRenderCmdList::SOperation *pA, const CRenderCmdList::SOperation *pB )
	{
		if ( pA->nPass != pB->nPass )
			return pA->nPass < pB->nPass;
		if ( pA->nDestRegister != pB->nDestRegister )
			return pA->nDestRegister < pB->nDestRegister;
		if ( pA->op != pB->op )
			return pA->op < pB->op;
		if ( pA->nStencilBlendMode != pB->nStencilBlendMode )
			return pA->nStencilBlendMode < pB->nStencilBlendMode;
		if ( pA->p1.pVec3 != pB->p1.pVec3 )
			return pA->p1.pVec3 < pB->p1.pVec3;
		if ( pA->p2.pVec3 != pB->p2.pVec3 )
			return pA->p2.pVec3 < pB->p2.pVec3;
		if ( pA->p3.pVec3 != pB->p3.pVec3 )
			return pA->p3.pVec3 < pB->p3.pVec3;
		if ( pA->nPartPriority != pB->nPartPriority )
			return pA->nPartPriority < pB->nPartPriority;
		if( pA->pFrag != pB->pFrag )
			return pA->pFrag < pB->pFrag;
		//ASSERT(0);
		return false;
	}
};

static std::vector<float> geomDepths;
struct SDepthCompare
{
	const std::vector<SRenderFragmentInfo::SElement> &elems;
	SDepthCompare( const std::vector<SRenderFragmentInfo::SElement> &_elems ) : elems(_elems) {}
	bool operator()( int a, int b ) const { return geomDepths[ elems[ a ].nGeometry ] < geomDepths[ elems[ b ].nGeometry ]; }
};
static void InitGeomDepths( NGfx::CRenderContext *pRC, const CSceneFragments &scene )
{
	int nGeometries = scene.GetGeometriesNum();
	if ( geomDepths.size() < nGeometries )
		geomDepths.resize( nGeometries );
	CVec4 vW = pRC->GetTransform().forward.w;
	CVec3 wDir( vW.x, vW.y, vW.z );
	for ( int k = 0; k < nGeometries; ++k )
		geomDepths[k] = scene.GetStaticInfo( k ).bv.s.ptCenter * wDir;
}

static void AddTriangles( NGfx::CRenderContext *pRC, const SRenderFragmentInfo &fragment, 
	const CSceneFragments &scene, ETrilistType triListType )
{
	static std::vector<int> indices;
	int nElements = fragment.elements.size();
	indices.resize( nElements );
	for ( int k = 0; k < nElements; ++k )
		indices[k] = k;
	std::sort( indices.begin(), indices.end(), SDepthCompare( fragment.elements ) );
	for ( int k = 0; k < nElements; ++k )
	{
		const SRenderFragmentInfo::SElement &element = fragment.elements[ indices[ k ] ];
		EFragmentsSplit filter = scene.GetGeometryFlags( element.nGeometry );
		if ( filter == FST_REJECT )
			continue;
		SRenderGeometryInfo *pGeometryInfo = scene.GetGeometryInfo( element.nGeometry );
		pGeometryInfo->pTriLists[triListType].Refresh();
		pGeometryInfo->pVertices.Refresh();
		const std::vector<NGfx::STriangleList> &tris = pGeometryInfo->pTriLists[triListType]->GetValue();
		ASSERT( element.nBlock * 32 < tris.size() );
		int nBase = element.nBlock * 32;
		int nFlags = element.nFlags;
		if ( filter == FST_SPLIT )
			nFlags &= scene.GetGeometryParts( element.nGeometry ).GetBlock( element.nBlock );
		pRC->AddPrimitive( pGeometryInfo->pVertices->GetValue(), &tris[0] + nBase, (std::min)( 32, int(tris.size() - nBase) ), nFlags );
	}
}

extern bool bNewShadows;

void SetupNLShadowsProjection( NGfx::CRenderContext *pRC, const SPerspDirectionalDepthInfo &depthInfo )
{

	pRC->SetVSConst( 23, depthInfo.vDepth + CVec4( 0, 0, 0, 0.01f ) );
	pRC->SetVSConst( 24, depthInfo.vDepth ) ;
	pRC->SetVSConst( 25, bNewShadows ? depthInfo.nlp.vDirX : depthInfo.nlp.vTexU );
	pRC->SetVSConst( 26, bNewShadows ? depthInfo.nlp.vDirY : depthInfo.nlp.vTexV );

	pRC->SetVSConst( 27, bNewShadows ?  depthInfo.nlp.lvShift : depthInfo.nlp.vShift );
}

void StartRenderExecute( NGfx::CRenderContext *pRC, const SLightInfo &lightInfo )
{
	pRC->Use(); //// CRAP
	if ( !NGfx::IsTnLDevice() )
	{
		pRC->SetVSConst( 14, lightInfo.vLightColor );
		pRC->SetVSConst( 15, lightInfo.vLightPos );
		//pRC->SetVSConst( 20, lightInfo.vBackColor );
		pRC->SetVSConst( 21, lightInfo.vRadius );
	}
}

static ETrilistType ExecuteRenderOp( NGfx::CRenderContext *pRC, const CRenderCmdList::SOperation &op, const SLightInfo &lightInfo )
{
	if ( pRC->HasRegisters() )
		pRC->SetRegister( op.nDestRegister );
	else
		ASSERT( op.nDestRegister == 0 );

	// set fog
	switch ( op.nStencilBlendMode & FOG_MASK )
	{
	case FOG_NORMAL: pRC->SetFog( NGfx::FOG_NORMAL ); break;
	case FOG_NONE: pRC->SetFog( NGfx::FOG_NONE ); break;
	case FOG_BLACK: pRC->SetFog( NGfx::FOG_BLACK ); break;
	default: ASSERT( 0 ); break;
	}

	// set blending
	switch ( op.nStencilBlendMode & ABM_MASK )
	{
	case ABM_NONE: pRC->SetAlphaCombine( NGfx::COMBINE_NONE ); break;
	case ABM_ZERO: pRC->SetAlphaCombine( NGfx::COMBINE_ZERO_ONE ); break;
	case ABM_ALPHA_ADD:  pRC->SetAlphaCombine( NGfx::COMBINE_ALPHA_ADD ); break;
	case ABM_MUL:  pRC->SetAlphaCombine( NGfx::COMBINE_MUL ); break;
	case ABM_SRC_AMUL: pRC->SetAlphaCombine( NGfx::COMBINE_SRC_ALPHA_MUL ); break;
	case ABM_ADD_SRC_AMUL: pRC->SetAlphaCombine( NGfx::COMBINE_ADD_SRC_ALPHA_MUL ); break;
	case ABM_ALPHA_BLEND: pRC->SetAlphaCombine( NGfx::COMBINE_ALPHA ); break;
	case ABM_SMART: pRC->SetAlphaCombine( NGfx::COMBINE_SMART_ALPHA ); break;
	default: ASSERT( 0 ); break;
	}
	// set stencil op
	switch ( op.nStencilBlendMode & STM_MASK )
	{
	case STM_NONE: pRC->SetStencil( NGfx::STENCIL_NONE ); break;
	case STM_LIGHT: pRC->SetStencil( NGfx::STENCIL_TESTNE_WRITE, 0x80, 0x80 ); break;
		///case STM_STENCIL_LIGHT: pRC->SetStencil( NGfx::STENCIL_TEST_REPLACE, 0x80, 0x7f ); break;
		//case STM_TEST_STENCIL_LIGHT: pRC->SetStencil( NGfx::STENCIL_TEST, 0x80, 0x80 ); break;
	case STM_INCREMENT: pRC->SetStencil( NGfx::STENCIL_INCR ); break;
	case STM_MARK: pRC->SetStencil( NGfx::STENCIL_WRITE, 0x80 ); break;
	case STM_TEST_CLEAR_MARK: pRC->SetStencil( NGfx::STENCIL_TESTNE_WRITE, 0, 0x80 ); break;
	case STM_MARK_2: pRC->SetStencil( NGfx::STENCIL_WRITE, 0x40, 0x40 ); break;
	case STM_TEST_MARK_2: pRC->SetStencil( NGfx::STENCIL_TEST, 0x40, 0x40 ); break;
	default: ASSERT( 0 ); break;
	}

	switch ( op.nStencilBlendMode & DPM_MASK )
	{
	case 0: pRC->SetDepth( NGfx::DEPTH_NORMAL ); break;
	case DPM_EQUAL:	pRC->SetDepth( NGfx::DEPTH_EQUAL ); break;
	case DPM_TESTONLY: pRC->SetDepth( NGfx::DEPTH_TESTONLY ); break;
	case DPM_NONE: pRC->SetDepth( NGfx::DEPTH_NONE ); break;
	case DPM_NORMAL_NOTEQ: pRC->SetDepth( NGfx::DEPTH_NORMAL_NOTEQ ); break;
	default: ASSERT(0); break;
	}

	pRC->Use();

	ETrilistType triListType = TLT_GEOM;
	if ( op.op >= RO_USER )
	{
		if ( !op.p1.pMaterial->SetRenderMode( pRC, lightInfo, op.op, op.p2, op.p3 ) )
		{
			pRC->SetPixelShader( psTFactor );
			pRC->SetVertexShader( vsPureGeometry );
			pRC->SetPSConst( 0, CVec4(1,0,0,1) );
		}
	}
	else
	{
		switch ( op.op )
		{
			// TnL path
		case RO_TNL_LIT_TEXTURE:
			if ( op.p1.pFastInfo->pTexture2 )
				pRC->SetPixelShader( psTnLLitTexture2 );
			else
				pRC->SetPixelShader( psTnLLitTexture );
			if ( op.p1.pFastInfo->bPreferPerVertexTransp )
				pRC->SetVertexShader( NGfx::TNLVS_VERTEX_COLOR_AND_ALPHA );
			else
				pRC->SetVertexShader( NGfx::TNLVS_VERTEX_COLOR );
			pRC->SetTnlVertexColor( op.p1.pFastInfo->vColor );
			pRC->SetTexture( 0, op.p1.pFastInfo->pTexture, NGfx::FILTER_BEST );
			if ( op.p1.pFastInfo->pTexture2 )
				pRC->SetTexture( 1, op.p1.pFastInfo->pTexture2, NGfx::FILTER_BEST );
			pRC->SetAlphaRef( op.p1.pFastInfo->nAlphaTest );
			break;
		case RO_TNL_SOLID_COLOR:
			pRC->SetPixelShader( psTFactor );
			pRC->SetVertexShader( NGfx::TNLVS_NONE );
			pRC->SetPSConst( 0, *op.p1.pVec4 );
			triListType = TLT_POSITION;
			break;
		case RO_TNL_TEXTURE:
			pRC->SetPixelShader( psTextureCopyAlpha );
			pRC->SetVertexShader( NGfx::TNLVS_NONE );
			pRC->SetTexture( 0, op.p1.pTex, NGfx::FILTER_BEST );
			break;
		case RO_TNL_SLIDING_TEXTURE:
			{
				pRC->SetPixelShader( psTnLLitTexture  );
				pRC->SetVertexShader( NGfx::TNLVS_TEXTRANS );
				pRC->SetTexture( 0, op.p1.pFastInfo->pTexture, NGfx::FILTER_BEST );
				//pRC->SetPSConst( 0, op.p3.f * CVec4(1,1,1,1) );
				SHMatrix m;
				Identity( &m );
				m.xz = op.p2.pVec3->x;
				m.yz = op.p2.pVec3->y;
				pRC->SetTnlTexTransform( m );
				pRC->SetAlphaRef( op.p1.pFastInfo->nAlphaTest );
			}
			break;

			// vs/ps path
		case RO_SOLID_COLOR:
			pRC->SetPixelShader( psTFactor );
			pRC->SetVertexShader( vsPureGeometry );
			pRC->SetPSConst( 0, *op.p1.pVec4 );
			triListType = TLT_POSITION;
			break;
		case RO_TEXTURE_AT:
			pRC->SetPixelShader( psAlphaTestTexturePerVertexTransp );
			pRC->SetVertexShader( vsTexturePerVertexTransp );
			pRC->SetTexture( 0, op.p1.pTex, NGfx::FILTER_BEST );
			pRC->SetVSConst( 16, CVec4( op.p2.f, 0, 0, 0 ) );
			break;
		case RO_TEXTURE_AT_NLP:
			pRC->SetPixelShader( psAlphaTestTexturePerVertexTransp );
			pRC->SetVertexShader( vsTexturePerVertexTranspNLP );
			SetupNLShadowsProjection( pRC, *op.p3.pPDirDepth );
			pRC->SetVSConst( 17, CVec4(0,0,0,0.5) - op.p3.pPDirDepth->vDepth * 0.10f );
			pRC->SetTexture( 0, op.p1.pTex, NGfx::FILTER_BEST );
			pRC->SetVSConst( 18, CVec4( op.p2.f, 0, 0, 0 ) );
			break;
		case RO_DIR_PARTICLE_LM_SOFT_SHADOW_TEST:
			pRC->SetPixelShader( psSoftShadowTest );
			pRC->SetVertexShader( vsParticleLMDirectionalTest );
			SetupNLShadowsProjection( pRC, *op.p1.pPDirDepth );
			pRC->SetPSConst( 1, CVec4( op.p3.pVec3[0], 0 ) );
			pRC->SetPSConst( 2, CVec4( op.p3.pVec3[1], 0 ) );
			pRC->SetTexture( 0, op.p2.pTex, NGfx::FILTER_LINEAR );
			triListType = TLT_POSITION;
			break;
		case RO_DIR_DEPTH:
			if ( op.nStencilBlendMode & SHADOW_CULL_CCW )
				pRC->SetCulling( NGfx::CULL_CCW );
			else
				pRC->SetCulling( NGfx::CULL_CW );

			pRC->SetPixelShader( psDiffuse );
			pRC->SetVertexShader(  bNewShadows ? vsDepthLP : vsDepth );
			SetupNLShadowsProjection( pRC, *op.p1.pPDirDepth );
			pRC->SetVSConst( 17, CVec4(0,0,0,0.5) - op.p1.pPDirDepth->vDepth * 0.10f );			
			triListType = TLT_POSITION;
			break;
		case RO_DIR_COLOR:
			pRC->SetPixelShader( psConstant );
			pRC->SetVertexShader( bNewShadows ?  vsDepthATTextureLP : vsDepthATTexture );
			SetupNLShadowsProjection( pRC, *op.p1.pPDirDepth );
			pRC->SetVSConst( 17, CVec4(0,0,0,0.5) - op.p1.pPDirDepth->vDepth * 0.10f );
			pRC->SetPSConst( 0, CVec4( 0, 0, 0, 1 ) );
			pRC->SetTexture( 0, op.p2.pTex, NGfx::FILTER_BEST );
			triListType = TLT_POSITION;
			break;
		case RO_DIR_DEPTH_AT_NLP:
			pRC->SetPixelShader( psDepthATTexture );
			pRC->SetVertexShader(  bNewShadows ?  vsDepthATTextureLP : vsDepthATTexture );
			SetupNLShadowsProjection( pRC, *op.p1.pPDirDepth );
			pRC->SetVSConst( 17, CVec4(0,0,0,0.5) - op.p1.pPDirDepth->vDepth * 0.10f );
			pRC->SetPSConst( 0, CVec4( 0, 0, 0, 0 ) );
			pRC->SetTexture( 0, op.p2.pTex, NGfx::FILTER_BEST );
			break;
		case RO_DIR_DEPTH_16:
			pRC->SetPixelShader( psTextureCopyAlpha );
			pRC->SetVertexShader( vsDepth16 );
			SetupNLShadowsProjection( pRC, *op.p1.pPDirDepth );
			pRC->SetVSConst( 17, CVec4(0,0,0,0.5) - op.p1.pPDirDepth->vDepth * 0.10f );
			pRC->SetTexture( 0, Get16bitDepthLookup(), NGfx::FILTER_POINT );
			triListType = TLT_POSITION;
			break;
		case RO_DIR_DEPTH_16_AT_NLP:
			pRC->SetPixelShader( psDepth16ATTexture );
			pRC->SetVertexShader( vsDepth16ATTexture );
			SetupNLShadowsProjection( pRC, *op.p1.pPDirDepth );
			pRC->SetVSConst( 17, CVec4(0,0,0,0.5) - op.p1.pPDirDepth->vDepth * 0.10f );
			pRC->SetPSConst( 0, CVec4( 0, 0, 0, 0 ) );
			pRC->SetTexture( 0, Get16bitDepthLookup(), NGfx::FILTER_POINT );
			pRC->SetTexture( 1, op.p2.pTex, NGfx::FILTER_BEST );
			break;
		case RO_LP_DEPTH:
			pRC->SetPixelShader( psDiffuse );
			pRC->SetVertexShader( vsLPDepth );
			pRC->SetVSConst( 24, op.p1.pDirDepth->vDepth );
			triListType = TLT_POSITION;
			break;

		case RO_G3_SHOW_LM:
			pRC->SetPixelShader( psTextureCopyAlpha );
			pRC->SetVertexShader( vsShowLM );
			pRC->SetVSConst( 16, CVec4( 1.0f/65536, 0.5f, 0, 0 ) );
			pRC->SetTexture( 0, op.p1.pTex, NGfx::FILTER_BEST );
			break;
		case RO_CL_SKY_3LIGHT:
		case RO_CL_SKY_3LIGHT_TRANSLUCENT:
			if ( op.op == RO_CL_SKY_3LIGHT_TRANSLUCENT )
			{
				pRC->SetPixelShader( psCLSkyLight3Translucent );
				pRC->SetPSConst( 10, *op.p2.pVec3 );
			}
			else
				pRC->SetPixelShader( psCLSkyLight3 );
			pRC->SetVertexShader( vsCLSkyLight3 );
			pRC->SetVSConst( 16, op.p1.pSkyDepth3->channels[0]->vDepth + CVec4( 0, 0, 0, 1.0f / 255 ) );
			pRC->SetVSConst( 17, op.p1.pSkyDepth3->channels[0]->vVecU );
			pRC->SetVSConst( 18, op.p1.pSkyDepth3->channels[0]->vVecV );
			pRC->SetVSConst( 25, op.p1.pSkyDepth3->channels[1]->vDepth + CVec4( 0, 0, 0, 1.0f / 255 ) );
			pRC->SetVSConst( 26, op.p1.pSkyDepth3->channels[1]->vVecU );
			pRC->SetVSConst( 27, op.p1.pSkyDepth3->channels[1]->vVecV );
			pRC->SetVSConst( 28, op.p1.pSkyDepth3->channels[2]->vDepth + CVec4( 0, 0, 0, 1.0f / 255 ) );
			pRC->SetVSConst( 29, op.p1.pSkyDepth3->channels[2]->vVecU );
			pRC->SetVSConst( 30, op.p1.pSkyDepth3->channels[2]->vVecV );
			// register mapping
			pRC->SetVSConst( 34, CVec4( 0.5, -0.5, 0.5 + 0.5 / op.p1.pSkyDepth3->nResolution, 0.5 + 0.5 / op.p1.pSkyDepth3->nResolution ) );
			pRC->SetPSConst( 0, CVec4( 1, 0, 0, -1.0f/255.0f ) );
			pRC->SetPSConst( 1, CVec4( 0, 1, 0, 0 ) );
			pRC->SetPSConst( 2, CVec4( 0, 0, 1, 0 ) );
			pRC->SetPSConst( 3, CVec4( 0, 0, 0, 0 ) );
			pRC->SetPSConst( 4, CVec4( 1, 1, 1, 1 ) );
			pRC->SetPSConst( 5, CVec4( lightInfo.vLightColor, 1 ) ); // vAmbientColor
			pRC->SetPSConst( 6, op.p1.pSkyDepth3->vDirs[0] );
			pRC->SetPSConst( 7, op.p1.pSkyDepth3->vDirs[1] );
			pRC->SetPSConst( 8, op.p1.pSkyDepth3->vDirs[2] );
			pRC->SetPSConst( 9, CVec4( 2, -1, 0, 0 ) );
			pRC->SetTexture( 0, op.p1.pSkyDepth3->pDepth, NGfx::FILTER_LINEAR );
			pRC->SetTexture( 1, op.p1.pSkyDepth3->pAdd, NGfx::FILTER_LINEAR );
			pRC->SetTexture( 2, GetNormalizeTexture() );
			break;
		case RO_CL_CUBEMAP_DEPTH:
			pRC->SetPixelShader( psPointCubeMapDepth );//psDiffuse );//
			pRC->SetVertexShader( vsPointCubeMapDepth );//vsConstLight );//
			pRC->SetVSConst( 16, CVec4( 1 / lightInfo.vRadius.x, 0, 0, 0 ) );
			triListType = TLT_POSITION;
			break;
		case RO_CL_PNT_LIGHT_SHADOWED:
			ASSERT( NGfx::GetHardwareLevel() >= NGfx::HL_RADEON2 );
			pRC->SetPixelShader( psCLPointLightShadowed );
			pRC->SetVertexShader( vsCLPointLightShadowed );
			pRC->SetVSConst( 16, CVec4( 1 / lightInfo.vRadius.x, 0, 0, 0 ) );
			// register mapping
			pRC->SetVSConst( 17, CVec4( 0.5, -0.5, 0.5 + 0.5 / op.p1.pPSInfo->fResolution, 0.5 + 0.5 / op.p1.pPSInfo->fResolution ) );
			pRC->SetPSConst( 0, CVec4( 2, -1, 0, 0 ) );
			pRC->SetPSConst( 1, CVec4( 0, 0, 0, 0 ) ); // shadow color
			pRC->SetPSConst( 2, CVec4( 0, 0, 0, 1.5 / 256.0f ) ); // shadow offset
			pRC->SetPSConst( 3, lightInfo.vLightColor );
			pRC->SetPSConst( 4, CVec4( 64, 0.25f, 0, 0 ) ); // attenuation calc params
			pRC->SetPSConst( 5, *op.p2.pVec3 ); // translucent color
			pRC->SetTexture( 0, op.p1.pPSInfo->pDepth );
			pRC->SetTexture( 1, op.p1.pPSInfo->pAdd, NGfx::FILTER_LINEAR );
			pRC->SetTexture( 2, GetNormalizeTexture() );
			break;
		case RO_WRITE_Z:
			pRC->SetPixelShader( psG3WriteZ );
			pRC->SetPSConst( 29, *op.p2.pVec4 );
			pRC->SetPSConst( 30, CVec4(1,1,1,1) );
			pRC->SetVertexShader( vsG3WriteZ );
			pRC->SetTexture( 0, op.p1.pTex, NGfx::FILTER_BEST );
			pRC->SetAlphaRef( 120 );
			break;

		default: ASSERT(0); break;
		}
	}
	return triListType;
}

static void ExecOps( NGfx::CRenderContext *pRC, const std::vector<CRenderCmdList::SOperation> &ops,
	const CSceneFragments &scene, const SLightInfo &lightInfo )
{
	if ( ops.empty() )
		return;

	PrepareVisibleGeometry( scene );
	InitGeomDepths( pRC, scene );

	std::vector<const CRenderCmdList::SOperation*> renderThem;
	for ( std::vector<CRenderCmdList::SOperation>::const_iterator i = ops.begin(); i != ops.end(); ++i )
		renderThem.push_back( &(*i) );

	sort( renderThem.begin(), renderThem.end(), SCompareOps() );
	//renderThem.sort( SCompareOps() );

	StartRenderExecute( pRC, lightInfo );

	const CRenderCmdList::SOperation *pPrevOp = 0;
	ETrilistType triListType = TLT_GEOM;
	for ( std::vector<const CRenderCmdList::SOperation*>::iterator i = renderThem.begin(); i != renderThem.end(); ++i )
	{
		const CRenderCmdList::SOperation &op = *(*i);
		if ( op.op == RO_NOP )
			continue;
		if ( pPrevOp )
		{
			if ( op.IsSame( *pPrevOp ) )
			{
				AddTriangles( pRC, *op.pFrag, scene, triListType );
				continue;
			}
			else
				pRC->Flush();
		}
		pPrevOp = &op;
		
		triListType = ExecuteRenderOp( pRC, op, lightInfo );
		
		// prevent TLT_POSITION from being used to reduce memory usage
		if ( bLowRAM )
			triListType = TLT_GEOM;
		AddTriangles( pRC, *op.pFrag, scene, triListType );
	}
	if ( pPrevOp )
		pRC->Flush();
}

void Execute( IRender *pRender, NGfx::CRenderContext *pRC, const CTransformStack &ts, const CRenderCmdList &cl,
	const CSceneFragments &scene, const SLightInfo &lightInfo )
{
	pRC->SetTransform( ts.Get() );
	ExecOps( pRC, cl.ops, scene, lightInfo );
}

}
START_REGISTER(GRenderExecute)
	REGISTER_VAR_EX( "gfx_worker_threads", NGlobal::VarIntHandler, &NGScene::nRenderWorkerThreads, 4, STORAGE_USER )
FINISH_REGISTER

