#include "stdafx.h"
#include "grendermodes.h"
#include "GTransparent.h"
#include "3DLib/Transform.h"
#include "GfxUtils.h"
#include "GSceneParticles.h"
#include "GfxEffects.h"
#include "RadixSort.h"
#include "GCombiner.h"
#include "3DLib/GGeometry.h"
#include "GRenderExecute.h"

#include <algorithm>
#include <cstdint>

namespace NGScene
{
const int N_RESERVE_DEPTH_BUFFER = 4096;
const int N_PARTICLES_PER_EFFECT_LG2 = 17;
const unsigned int N_PARTICLE_OVER_FLAG = 0x80000000;
const unsigned int N_OVER_SRC_INFO = N_PARTICLE_OVER_FLAG >> N_PARTICLES_PER_EFFECT_LG2;

// CTransparentRenderer

CTransparentRenderer::CTransparentRenderer( const CTransformStack &ts, const CTPoint<int> &vLightBuffersize, 
	bool _bUseFakeLM, uint32_t _dwLitColor, uint32_t _dwNormalColor, const SPerVertexLightState *_pLightState )
	: nTotalParticles(0), nLitParticles(0), dwLitColor(_dwLitColor), dwNormalColor(_dwNormalColor),
	nElementPtr(0), nInfoIdx(-(1<<N_PARTICLES_PER_EFFECT_LG2)), pLightState(_pLightState)
{
	infos.reserve( 100 );
	depths.resize( N_RESERVE_DEPTH_BUFFER * 2 );
	sourcePtrs.resize( N_RESERVE_DEPTH_BUFFER * 2 );
	bTnLMode = NGfx::IsTnLDevice();
	SHMatrix invRoot;
	invRoot = ts.Get().backward * ts.GetProjection().forward;
	orientation.vBasic[0] = CVec3( invRoot._11, invRoot._21, invRoot._31 );
	orientation.vBasic[1] = CVec3( invRoot._12, invRoot._22, invRoot._32 );
	orientation.vBasic[2] = CVec3( invRoot._13, invRoot._23, invRoot._33 );
	orientation.vBasic[3] = invRoot.GetTranslation();
	orientation.vDepth = CVec3( -ts.Get().forward.wx, -ts.Get().forward.wy, -ts.Get().forward.wz );;

	bUseFakeLM = _bUseFakeLM;
	if ( bUseFakeLM )
	{
		ASSERT( vLightBuffersize.x == 2 );
		ASSERT( vLightBuffersize.y == 1 );
		litParticlesAlloc.Init( 1, 1, 2, 1, false );
		kernel = litParticlesAlloc;
		litParticlesAlloc.ForcedInc();
	}
	else
	{
		litParticlesAlloc.Init( 4, 4, Float2Int( vLightBuffersize.x ), Float2Int( vLightBuffersize.y ), true );
		kernel = litParticlesAlloc;
		litParticlesAlloc.Inc();
		kernel.StopIncrementing();
	}
	NGfx::CalcCompactVector( &vNormal, -orientation.vBasic[2] );
}

STransparentInfo* CTransparentRenderer::AddFragment() 
{ 
	nInfoIdx += 1 << N_PARTICLES_PER_EFFECT_LG2;
	// make sure enough place is allocated
	if ( nElementPtr + N_RESERVE_DEPTH_BUFFER > depths.size() )
	{
		depths.resize( depths.size() + N_RESERVE_DEPTH_BUFFER * 2 );
		sourcePtrs.resize( sourcePtrs.size() + N_RESERVE_DEPTH_BUFFER * 2 );
	}
	infoStartIdx.push_back( nElementPtr );
	return &infos.emplace_back();
}

void CTransparentRenderer::AllocParticlesWriteBuffer()
{
	if ( bTnLMode )
		pParticlesGeometry = pTnLParticlesGeometry = new CTnLParticlesGeometry( this, orientation );
	else
		pParticlesGeometry = pShaderParticlesGeometry = new CShaderParticlesGeometry( this, orientation, vNormal );
	pParticlesTrilist = new CParticlesTriList;
	nTargetParticle = 0;
}

void CTransparentRenderer::FinishParticles() 
{
	if ( IsValid(pParticlesGeometry) )
		pParticlesGeometry->FreeWriteBuffer();
	pParticlesGeometry = 0;
	pTnLParticlesGeometry = 0;
	pShaderParticlesGeometry = 0;
	pParticlesTrilist = 0;
}

void CTransparentRenderer::FinishParticlesPiece()
{
	int nParticles = nTargetParticle - nPieceStart;

	pParticlesTrilist->AddPart( nParticles );
	SBound bv;
	pParticlesGeometry->FinishPart( &bv, pLMAlloc );
	if ( pReportParticles )
	{
		pReportParticles->AddParticles( pParticlesGeometry, pParticlesTrilist, pParticlesGeometry->GetPartsNum() - 1, 
			nParticles, bv );
	}
}

void CTransparentRenderer::StartParticlesPiece()
{
	STransparentInfo *pWriteParticles = AddFragment();
	pWriteParticles->pGeom = pParticlesGeometry;
	pWriteParticles->pObjectInfo = 0;
	pWriteParticles->fDepth = currentEffectBV.s.ptCenter * GetDepth();
	pWriteParticles->nOffset = nTargetParticle * 4;
	nPieceStart = nTargetParticle;
	int nEffectParticle = nTargetParticle - nTargetStart;
	pParticlesGeometry->Start( pCurrentEffect, nEffectParticle, currentEffectBV, *pLMAlloc, dwCurrentParticleColor );
}

void CTransparentRenderer::AddParticles( IParticles *pParticles, bool bIsLit, const SBound &bv, 
	IReportParticlesGeometry *pStore ) 
{
	pCurrentEffect = pParticles->GetEffect();
	if ( pCurrentEffect->IsEmpty() )
	{
		if ( pCurrentEffect->bEnd )
			pParticles->Unlink();
		return;
	}
	pReportParticles = pStore;
	currentEffectBV = bv;
	if ( bIsLit )
	{
		dwCurrentParticleColor = dwLitColor;
		pLMAlloc = &litParticlesAlloc;
	}
	else
	{
		dwCurrentParticleColor = dwNormalColor;
		pLMAlloc = &kernel;
	}
	if ( !IsValid(pParticlesGeometry) )
		AllocParticlesWriteBuffer();
	else if ( pParticlesGeometry->GetPartsNum() == PF_MAX_PARTS_PER_COMBINER )
	{
		pParticlesGeometry->FreeWriteBuffer();
		AllocParticlesWriteBuffer();
	}
	nTargetStart = nTargetParticle;
	StartParticlesPiece();
	
	pCurrentEffect->AddParticles( this );
	
	FinishParticlesPiece();
	nTotalParticles += nTargetParticle - nTargetStart;
	if ( bIsLit )
		nLitParticles += nTargetParticle - nTargetStart;
	bc.Add( bv );
}

void CTransparentRenderer::AddParticleOverflow( const CVec3 vPos[4], uint32_t dwColor, const STransparentTexturePlace &tPlace,
	float fDepth )
{
	// buffer is full, start a new one
	if ( bTnLMode )
		pTnLParticlesGeometry->RealAddParticle( vPos, dwColor, tPlace, fDepth );
	else
		pShaderParticlesGeometry->RealAddParticle( vPos, dwColor, tPlace, fDepth );
	FinishParticlesPiece();
	pParticlesGeometry->FreeWriteBuffer();

	int nEffectParticle = nTargetParticle - nTargetStart;
	AllocParticlesWriteBuffer();
	nTargetStart = nTargetParticle - nEffectParticle;

	StartParticlesPiece();
}

void CTransparentRenderer::AddParticle( const CVec3 vPos[4], uint32_t dwColor, const STransparentTexturePlace &tPlace,
	float fDepth )
{
	// store particle depth info
	depths[ nElementPtr ] = fDepth;
	sourcePtrs[ nElementPtr ] = nInfoIdx | ( nTargetParticle - nPieceStart + 1 );
	++nElementPtr;
	//
	++nTargetParticle;
	if ( nTargetParticle < N_PARTICLES_BUFFER_SIZE / 4 )
	{
		if ( bTnLMode )
			pTnLParticlesGeometry->RealAddParticle( vPos, dwColor, tPlace, fDepth ); // to enable tail call optimization
		else
			pShaderParticlesGeometry->RealAddParticle( vPos, dwColor, tPlace, fDepth ); // to enable tail call optimization
	}
	else
		AddParticleOverflow( vPos, dwColor, tPlace, fDepth );
}

void CTransparentRenderer::SampleWarFog( const std::vector<CVec3> &vPos, std::vector<float> *pRes )
{
	if ( !pLightState )
	{
		pRes->resize(0);
		return;
	}
	return NGScene::SampleWarFog( vPos, *pLightState, pRes );
}

void CTransparentRenderer::AddElement( SRenderGeometryInfo *pGeometry, IMaterial *pMaterial, const SPerPartVariables &vars, int nIndex, float fDepth )
{
	STransparentObjectInfo *pObjInfo = objInfoPool.Alloc();
	pObjInfo->pGeometry = pGeometry;
	pObjInfo->material.pMaterial = pMaterial;
	pObjInfo->material.vars = vars;
	STransparentInfo *pFrag = AddFragment();
	pFrag->pObjectInfo = pObjInfo;
	pFrag->fDepth = fDepth;
	pFrag->nOffset = nIndex;
	// store depth info
	depths[ nElementPtr ] = fDepth;
	sourcePtrs[ nElementPtr ] = nInfoIdx;
	++nElementPtr;
}

static void DoRadixSort( const std::vector<float> &src, std::vector<int> *pRes )
{
	if ( !src.empty() )
    ::DoRadixSort( &src[0], src.size(), pRes );
}

static void SetObjectEffect( const STransparentRenderContext &trc, const SMaterialParams &material )
{
	if ( trc.renderPath == RP_SHOWOVERDRAW )
		return;
	IMaterial *pMaterial = material.pMaterial;
	if ( !pMaterial->IsTransparent() )
	{
		ASSERT(0);
		return;
	}
	pMaterial->SetTransparentRenderMode( trc.pRC, material.vars, trc.lightInfo, trc.pRPC );
}

static void SetParticlesEffect( const STransparentRenderContext &trc )
{
	if ( trc.renderPath == RP_SHOWOVERDRAW )
		return;
	trc.pRC->SetAlphaCombine( NGfx::COMBINE_SMART_ALPHA );
	if ( trc.bTnLMode )
	{
		NGfx::SEffTnLParticles eff;
		trc.pRC->SetEffect( &eff );
	}
	else
	{
		trc.pRC->SetFog( NGfx::FOG_NONE );
		NGfx::SEffTransparentParticles eff;
		eff.pLight = trc.pParticleLight;
		trc.pRC->SetEffect( &eff );
	}
}

static void ReorderTransparent( std::vector<STriangle> *pRet, const SFBTransform &sTransform, IVBCombiner *_pCombiner, int nPart, const NGfx::STriangleList &sList )
{
	CDGPtr<IVBCombiner> pCombiner = _pCombiner;
	pCombiner.Refresh();
	CDGPtr<CFuncBase<std::vector<CPtr<IPart> > > > pParts = _pCombiner->GetCombiner();
	pParts.Refresh();
	const std::vector<CPtr<IPart> > &parts = pParts->GetValue();
	IPart *pPart = parts[nPart];
	pPart->RefreshObjectInfo();
	CObjectInfo *pInfo = pPart->GetObjectInfo();
	////
	std::vector<CVec3> holdPos;
	const std::vector<CVec3> *pTransofmedPos = &pPart->xformedPositions;
	if ( pTransofmedPos->empty() )
	{
		TransformPart( pPart, &holdPos, 0 );
		pTransofmedPos = &holdPos;
	}
	const std::vector<CVec3> &xfpos = *pTransofmedPos;
	std::vector<float> pointsDepth( xfpos.size() );
	CVec4 vW = sTransform.forward.w();
	for ( int nTemp = 0; nTemp < xfpos.size(); ++nTemp )
	{
		const CVec3 &v = xfpos[nTemp];
		pointsDepth[nTemp] = vW.x * v.x + vW.y * v.y + vW.z * v.z + vW.w;
	}
	////
	std::vector<float> depths( sList.nTris );
	const std::vector<uint16_t> &posIndices = pInfo->GetPositionIndices();
	for ( int nTemp = 0; nTemp < sList.nTris; ++nTemp )
	{
		const STriangle &sTri = sList.pTri[nTemp];
		////
		int i1 = posIndices[sTri.i1 - sList.nOffset];
		int i2 = posIndices[sTri.i2 - sList.nOffset];
		int i3 = posIndices[sTri.i3 - sList.nOffset];
		const float fZ1 = pointsDepth[i1];
		const float fZ2 = pointsDepth[i2];
		const float fZ3 = pointsDepth[i3];
		////
		depths[nTemp] = fZ1 + fZ2 + fZ3;
	}
	////
	std::vector<int> sorted;
	DoRadixSort( depths, &sorted );
	pRet->resize( sorted.size() );
	for ( int nTemp = 0; nTemp < sorted.size(); ++nTemp )
	{
		const int &nIndex = sorted[sorted.size() - nTemp - 1];
		(*pRet)[nTemp] = sList.pTri[nIndex];
	}
}

const int N_UNUSED_SRC_PTR = 0xffffffff & ~N_OVER_SRC_INFO;
class CPreciseTranspRender
{
	std::vector<STransparentInfo> &infos;
	std::vector<float> &depths;
	std::vector<int> &sourcePtrs;
	std::vector<STriangle> tris;
	unsigned int nCurrentSrc, nCurrentBase;
	NGfx::CRenderContext *pRC;
	SMaterialParams currentMaterial;

	void Flush()
	{
		if ( nCurrentSrc == N_UNUSED_SRC_PTR )
			return;
		NGfx::STriangleList triList;
		triList.pTri = &tris[0];
		triList.nTris = tris.size();
		int nInfo = nCurrentSrc & ~N_OVER_SRC_INFO;
		triList.nBaseIndex = infos[ nInfo ].nOffset;
		CDGPtr<IVBCombiner> pVertices( infos[ nInfo ].pGeom );
		pVertices.Refresh();
		pRC->AddPrimitive( pVertices->GetValue(), triList );
		tris.resize( 0 );
		nCurrentSrc = N_UNUSED_SRC_PTR;
	}
	void MarkOverdrawParticles( const std::vector<int> &sorted )
	{
		for ( int k = sorted.size() - 1; k >= 0; --k )
		{
			if ( depths[ sorted[k] ] != F_PARTICLE_OVER )
				break;
			unsigned int *pnSrcPtr = (unsigned int*)&sourcePtrs[ sorted[k] ];
			*pnSrcPtr |= N_PARTICLE_OVER_FLAG;
		}
	}
public:
	CPreciseTranspRender( NGfx::CRenderContext *_pRC, std::vector<STransparentInfo> *pInfos, std::vector<float> *pDepths,
		std::vector<int> *pSrcPtrs ) : infos(*pInfos), depths(*pDepths), sourcePtrs(*pSrcPtrs), pRC(_pRC)
	{
	}
	void Render( const STransparentRenderContext &trc )
	{
		SetParticlesEffect( trc );
		currentMaterial.Clear();
		std::vector<int> sorted;
		DoRadixSort( depths, &sorted );
		nCurrentSrc = N_UNUSED_SRC_PTR;
		// mark overdraw particles with special source
		MarkOverdrawParticles( sorted );
		// render
		for ( int k = 0; k < sorted.size(); ++k )
		{
			unsigned int nSrcPtr = (unsigned int)sourcePtrs[ sorted[k] ];
			int nParticle = nSrcPtr & ( (1<<N_PARTICLES_PER_EFFECT_LG2) - 1 );
			unsigned int nSrc = nSrcPtr >> N_PARTICLES_PER_EFFECT_LG2;
			if ( nParticle == 0 ) // IsObject
			{
				Flush();
				const STransparentInfo &info = infos[nSrc];
				if ( currentMaterial != info.pObjectInfo->material )
				{
					pRC->Flush();
					pRC->SetDepth( NGfx::DEPTH_TESTONLY );
					currentMaterial = info.pObjectInfo->material;
					SetObjectEffect( trc, currentMaterial );
				}
				SRenderGeometryInfo *pGeometry = info.pObjectInfo->pGeometry;
				pGeometry->pVertices.Refresh();
				pGeometry->pTriLists[TLT_GEOM].Refresh();
				const std::vector<NGfx::STriangleList> &tris = pGeometry->pTriLists[TLT_GEOM]->GetValue();
				if ( !tris.empty() && tris[info.nOffset].nTris != 0 )
				{
					std::vector<STriangle> sorted;
					NGfx::STriangleList sSortedList( tris[ info.nOffset ] );
					ReorderTransparent( &sorted, pRC->GetTransform(), pGeometry->pVertices, info.nOffset, tris[ info.nOffset ] );
					if ( !sorted.empty() )
					{
						sSortedList.pTri = &sorted[0];
						////
						pRC->AddPrimitive( pGeometry->pVertices->GetValue(), sSortedList );
					}
				}
			}
			else
			{
				if ( nSrc != nCurrentSrc )
				{
					Flush();
					if ( !currentMaterial.IsEmpty() || ( ( nSrc ^ nCurrentSrc ) & N_OVER_SRC_INFO ) )
					{
						pRC->Flush();
						if ( nSrc & N_OVER_SRC_INFO )
							pRC->SetDepth( NGfx::DEPTH_NONE );
						else
							pRC->SetDepth( NGfx::DEPTH_TESTONLY );
						SetParticlesEffect( trc );
						currentMaterial.Clear();
					}
				}
				nCurrentSrc = nSrc;
				int nBase = ( nParticle - 1 ) * 4;
				tris.push_back( STriangle( nBase, nBase + 1, nBase + 2 ) );
				tris.push_back( STriangle( nBase, nBase + 2, nBase + 3 ) );
			}
		}
		Flush();
		pRC->Flush();
	}
};

struct SCmpFragments
{
	const std::vector<STransparentInfo> &infos;
	SCmpFragments( const std::vector<STransparentInfo> &_infos ) : infos(_infos) {}
	bool operator()( int n1, int n2 )
	{
		return infos[n1].fDepth < infos[n2].fDepth;
	}
};

void CTransparentRenderer::RealRender( const STransparentRenderContext &trc )
{
	NGfx::CRenderContext &rc = *trc.pRC;
	rc.Use();
	if ( trc.renderPath == RP_SHOWOVERDRAW )
	{
		rc.SetAlphaCombine( NGfx::COMBINE_ZERO_ONE );
		rc.SetDepth( NGfx::DEPTH_NONE );
		rc.SetStencil( NGfx::STENCIL_INCR );
		NGfx::SEffPureGeometry effGeom;
		rc.SetEffect( &effGeom );
	}
	else
	{
		rc.SetAlphaCombine( NGfx::COMBINE_SMART_ALPHA );
		rc.SetDepth( NGfx::DEPTH_TESTONLY );
		rc.SetStencil( NGfx::STENCIL_NONE );
	}
	if ( NGfx::CanStreamGeometry() && nTotalParticles < 14000 )
	{
		CPreciseTranspRender pr( trc.pRC, &infos, &depths, &sourcePtrs );
		pr.Render( trc );
	}
	else
	{
		std::vector<int> fragments;
		fragments.resize( infos.size() );
		for ( int k = 0; k < fragments.size(); ++k )
			fragments[k] = k;
		std::sort( fragments.begin(), fragments.end(), SCmpFragments( infos ) );
		for ( int k = 0; k < fragments.size(); ++k )
		{
			int nFragment = fragments[k];
			const STransparentInfo &info = infos[ nFragment ];
			if ( info.pGeom == 0 )
			{
				rc.SetDepth( NGfx::DEPTH_TESTONLY );
				SetObjectEffect( trc, info.pObjectInfo->material );
				SRenderGeometryInfo *pGeometry = info.pObjectInfo->pGeometry;
				pGeometry->pVertices.Refresh();
				pGeometry->pTriLists[TLT_GEOM].Refresh();
				const std::vector<NGfx::STriangleList> &tris = pGeometry->pTriLists[TLT_GEOM]->GetValue();
				if ( !tris.empty() )
				{
					std::vector<STriangle> sorted;
					NGfx::STriangleList sSortedList( tris[ info.nOffset ] );
					ReorderTransparent( &sorted, trc.pRC->GetTransform(), pGeometry->pVertices, info.nOffset, tris[ info.nOffset ] );
					sSortedList.pTri = &sorted[0];
					////
					trc.pRC->AddPrimitive( pGeometry->pVertices->GetValue(), sSortedList );
					trc.pRC->Flush();
				}
			}
			else
			{
				SetParticlesEffect( trc );

				int nStart = infoStartIdx[ nFragment ], nFinish = infoStartIdx[ nFragment + 1 ];
				int nParticles = nFinish - nStart;
				nParticles = (std::min)( nParticles, NGfx::N_MAX_RECTANGLES ); // CRAP
				if ( nParticles == 0 )
					continue;

				if ( depths[nStart] == F_PARTICLE_OVER )
					rc.SetDepth( NGfx::DEPTH_NONE );
				else
					rc.SetDepth( NGfx::DEPTH_TESTONLY );

				std::vector<int> particles( nParticles );
				for ( int i = 0; i < particles.size(); ++i )
					particles[i] = i;
				::DoRadixSort( &depths[ nStart ], nParticles, &particles );
				CObj<NGfx::CTriList> pTriList;
				{
					NGfx::CBufferLock<NGfx::S3DTriangle> tris( &pTriList, nParticles * 2 );
					//int nOffset = info.nOffset;
					for ( int i = 0; i < particles.size(); ++i )
					{
						int nBase = particles[i] * 4; //+ nOffset;
						tris[ i*2 + 0 ] = NGfx::S3DTriangle( nBase, nBase + 1, nBase + 2 );
						tris[ i*2 + 1 ] = NGfx::S3DTriangle( nBase, nBase + 2, nBase + 3 );
					}
				}
				CDGPtr<IVBCombiner> pGeom( info.pGeom );
				pGeom.Refresh();
				rc.DrawPrimitive( pGeom->GetValue(), pTriList, info.nOffset, nParticles * 4 );
			}
		}
	}
}

void CTransparentRenderer::Render( const STransparentRenderContext &trc )
{
	FinishParticles();
	depths.resize( nElementPtr );
	sourcePtrs.resize( nElementPtr );
	infoStartIdx.push_back( nElementPtr );
	if ( sourcePtrs.empty() )
		return;
	StartRenderExecute( trc.pRC, trc.lightInfo );
	RealRender( trc );
	infoStartIdx.pop_back();
}

}
using namespace NGScene;

