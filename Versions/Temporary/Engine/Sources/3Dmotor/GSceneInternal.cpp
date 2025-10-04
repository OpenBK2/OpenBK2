#include "stdafx.h"
#include "Gfx.h"
#include "GfxEffects.h"
#include "LoadingCounter.h"
#include "GSceneUtils.h"
#include "GSceneInternal.h"
#include "GMaterial.hpp"
#include "GRenderLight.h"
#include "Misc/HPTimer.h"
#include "SuperCollider.h" // for TraceStatic(...)
#include "GDecalGeometry.h"
#include "GShadowVolume.h"
#include "GRenderPathFastest.h"
#include "GRenderPathOverdraw.h"
#include "GRenderPathTnl.h"
#include "GRenderPathLightmap.h"
#include "GRTShare.h"
#include "GLightmapCalc.h"
#include "GRenderUtils.h"
#include "4dCalcs.h"
#include "GPartParticles.h"
#include "GPostProcessors.h"
#include "GRenderPathPolycount.h"
#include "System/Commands.h"
#include <D3D9.h>
#include <D3DX9.h>

#include "GShaderFX.h"
#include "GPostEffects.h"


const int N_SKIP_IGNORED_TEST = -1;
namespace NGScene
{

static SRenderStats lastFrameStats;
static bool bWireframe;
static bool bShow2DTextureCache = false, bShowTranspTextureCache = false, bShowParticleLMCache = false;
static bool bTwilight = false;
//static bool bUseHWHSR = false;
static int nTotalParts, nTotalElements;
static float s_fLODSwitchDistance = 300;
enum EShowLinearCache
{
	SLC_NONE,
	SLC_STATIC,
	SLC_DYNAMIC
};
static EShowLinearCache showLinearCache = SLC_NONE;
extern bool bLowRAM;

class CGetTranspCache : public CPtrFuncBase<NGfx::CTexture>
{
	OBJECT_NOCOPY_METHODS(CGetTranspCache);
	void Recalc() { pValue = NGfx::GetTransparentTextureCache(); }
};

class CAmbientAnimator : public CFuncBase<CVec3>
{
	OBJECT_BASIC_METHODS(CAmbientAnimator)
	ZDATA
	CDGPtr<CFuncBase<CVec3> > pAmbient;
	CDGPtr<CPtrFuncBase<CAnimLight> > pAnim;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAmbient); f.Add(3,&pAnim); return 0; }
	bool NeedUpdate() 
	{ 
		bool b1 = pAmbient.Refresh();
		bool b2 = IsValid( pAnim ) ? pAnim.Refresh() : false;
		return b1 || b2;
	}
	void Recalc()
	{
		if ( IsValid( pAnim ) )
		{
			CAnimLight *pInfo = pAnim->GetValue();
			if ( pInfo && !pInfo->bEnd )
			{
				value = pInfo->color;
				return;
			}
		}
		value = pAmbient->GetValue();
	}
public:
	CAmbientAnimator() {}
	CAmbientAnimator( CPtrFuncBase<CAnimLight> *_pAnim, CFuncBase<CVec3> *pColor ): pAmbient( pColor ), pAnim( _pAnim ) {}

	void SetAmbientAnimation( CPtrFuncBase<CAnimLight> *_pAnim ) { pAnim = _pAnim; }
	CPtrFuncBase<CAnimLight> *GetAnimation() const { return pAnim; }
};

bool GetGeometryObjectInfo( CObjectBase *p, 
	CPtrFuncBase<CObjectInfo> **pGeometry, SFBTransform *pPos, SFullGroupInfo *pGroupInfo )
{
	CDynamicCast<ISomePart> pPart( p );
	if ( !pPart )
		return false;
	switch ( pPart->GetTransformType() )
	{
		case TT_NONE:
			Identity( &pPos->forward );
			Identity( &pPos->backward );
			break;
		case TT_SIMPLE:
			*pPos = pPart->GetSimplePos();
			break;
		default:
			return false;
	}
	*pGeometry = pPart->GetObjectInfoNode();
	*pGroupInfo = pPart->GetFullGroupInfo();
	return true;
}

// CPostProcessBinder

bool CPostProcessBinder::Initialize( CObjectBase *_p, IPostProcess *_pPost )
{
	pPostProcess = _pPost;
	if ( CDynamicCast<ISomePart> pPart = _p )
	{
		pTarget = pPart;
		return true;
	}
	return false;
}

void CPostProcessBinder::Store( vector<IPostProcess::SObject> *pRes, CTransformStack *pTS, const SGroupSelect &mask )
{
	if ( !IsValid( pTarget ) || !IsValid( pTarget->pOwner ) )
		return;
	SRenderGeometryInfo *pGeom = pTarget->pOwner->GetGeometryInfo();
	// skip animating not visible selection
	// requires that selection was rendered after normal scene without marking new DG frame
	if ( !pGeom->pVertices->WasRefreshed() )
		return;
	const vector<CPtr<IPart> > &parts = pTarget->pOwner->GetCombiner()->GetValue();
	int nIdx = -1;
	for ( int k = 0; k < parts.size(); ++k )
		if ( parts[k].GetPtr() == pTarget.GetPtr() )
			nIdx = k;
	if ( nIdx < 0 )
		return;
	if ( !pTarget->pOwner->GetPartsInfo()[nIdx].groupInfo.IsMaskMatch( mask ) )
		return;
	if ( !pTS->IsIn( pGeom->pVertices->GetBounds()[ nIdx ] ) )
		return;
	if ( pGeom->pTriLists[TLT_GEOM] )
		pRes->push_back( IPostProcess::SObject( pGeom, nIdx ) );
}

// CPolyline

CPolyline::CPolyline( CPtrFuncBase<NGfx::CGeometry> *_pGeometry, const vector<unsigned short> &_indices,
	const CVec4 &_color, bool _bCheckDepth )
	: pGeometry(_pGeometry), indices(_indices), color(_color), bCheckDepth(_bCheckDepth)
{
	while ( ( indices.size() % 6 ) != 0 )
		indices.push_back( 0 );
}

void CPolyline::Render( NGfx::CRenderContext *pRC )
{
	pGeometry.Refresh();
	pRC->AddLineStrip( pGeometry->GetValue(), &indices[0], indices.size() / 2 );
}

// CFakeParticleLMTexture

const int N_FAKE_LM_SIZEX = 2;
const int N_FAKE_LM_SIZEY = 1;
void CFakeParticleLMTexture::Recalc()
{
	pValue = NGfx::MakeTexture( N_FAKE_LM_SIZEX, N_FAKE_LM_SIZEY, 1, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::CLAMP );
	NGfx::CTextureLock<NGfx::SPixel8888> lock( pValue, 0, NGfx::INPLACE );
	dwNormalColor = NGfx::GetDWORDColor( CVec4( 0.25f, 0.25f, 0.25f, 1 ) );
	dwParticleColor = NGfx::GetDWORDColor( CVec4( ( pAmbient->GetValue() + pColor->GetValue() ), 1 ) );
	lock[0][0].dwColor = dwNormalColor;
	lock[0][1].dwColor = dwParticleColor;
}


// CGScene

CGScene::CGScene() : holdMask(0,0), nFrameCounter(100), lastMask(0,0), bWaitForLoad( true ), 
	nReuseIgnoreList(0), nGfxDeviceCreationID(-1), nIgnoreListWasCalced(0), bIsTwilight(false)
{
	Identity( &mHoldTransform );
}

CGScene::CGScene( int ) : holdMask(0,0), nFrameCounter(100), lastMask(0,0), nReuseIgnoreList(0),
	nGfxDeviceCreationID(-1), nIgnoreListWasCalced(0), fSunFlareCoeff( 0 ), sSunFlareTime( 0 ), 
	bIsTwilight(false)
{
	Identity( &mHoldTransform );
	pVolume = new CVolumeNode;
	pVolume->SetSize( CVec3( -128, -128, -128 ), 1024 );
	renderMode = SRM_BEST;
	pCamera = new CCVec4;
	nSlowVolumeWalk = 30;
	CVec3 vDefaultAmbient( 0.25f, 0.25f, 0.25f );
	pAmbient = new CCVec3( vDefaultAmbient );
	pAmbientAnimator = new CAmbientAnimator( 0, pAmbient );
	nCurrentIgnoreMark = 1;
	SMaterialCreateInfo mc;
	mc.pTexture = new CGetTranspCache;
	mc.alphaMode = MF_OPAQUE|MF_ALPHA_TEST;
	pTransparentMaterial = CreateMaterial( mc );
	pFakeParticleLM = new CFakeParticleLMTexture;
	pFakeParticleLM->SetAmbient( pAmbient );
	pFakeParticleLM->SetColor( new CCVec3( CVec3(0,0,0) ) );
	pParticlesLightColor = new CCVec3( CVec3(1, 1, 1) );
	pDecalsManager = new CDecalsManager( this );
	bWaitForLoad = true;
	pLightState = new CLightStateNode;
	trackers.pLightState = pLightState;
}

// only positive collisions qualify, closest is searched for
static bool Collide( const SRayInfo &r, const vector<CVec3> &points, const vector<STriangle> &tris, float *pfT, CVec3 *pNormal )
{
	const CVec3 &vRayOrigin = r.vOrigin;
	const CVec3 &vRayDir = r.vDir;
	NCollider::SSegment ray( vRayOrigin, vRayOrigin + vRayDir );
	bool bRes = false;
	for ( int k = 0; k < tris.size(); ++k )
	{
		const STriangle &t = tris[k];
		NCollider::SSegment seg1( points[t.i3], points[t.i1] );//points[nPrev], points[ indices[i] ] );
		if ( NCollider::SegmentDotProduct( ray, seg1 ) < 0 )
			continue;
		NCollider::SSegment seg2( points[t.i1], points[t.i2] );//points[nPrev], points[ indices[i] ] );
		if ( NCollider::SegmentDotProduct( ray, seg2 ) < 0 )
			continue;
		NCollider::SSegment seg3( points[t.i2], points[t.i3] );//points[nPrev], points[ indices[i] ] );
		if ( NCollider::SegmentDotProduct( ray, seg3 ) < 0 )
			continue;
		// collides - calc exact place
		CVec3 vNormal;
		vNormal = ( points[t.i3] - points[t.i2] ) ^ ( points[t.i1] - points[t.i2] );
		if ( fabs2( vNormal ) < FP_EPSILON )
		{
			continue;
		}
		Normalize( &vNormal );

		float fT = ( ( points[ t.i1 ] - vRayOrigin ) * vNormal ) / ( vRayDir * vNormal );
		if ( fT > 0 && fT < *pfT )
		{
			*pfT = fT;
			//trans.backward.RotateVectorTransposed( pNormal, vNormal );
			*pNormal = vNormal;
			//Normalize( pNormal );
			bRes = true;
		}
	}
	return bRes;
}

static bool DoesIntersect( const SSphere &s, const SRayInfo &r )
{
	CVec3 vToCenter( s.ptCenter - r.vOrigin );
	float fDist = r.vDirOrt * vToCenter;
	float fRo = ( vToCenter * vToCenter ) - sqr( fDist );
	if ( fRo > sqr( s.fRadius ) )
		return false;
	float fSide = sqrt( sqr( s.fRadius ) - fRo );
	if ( fDist - fSide > r.fLength )
		return false;
	if ( fDist + fSide < 0 )
		return false;
	return true;
}

static bool TestParts( const list<CPtr<CCombinedPart> > &elements, const SGroupSelect &mask, const SRayInfo &r, 
	float *pfT, CVec3 *pNormal, SFullGroupInfo *pGroupInfo, CObjectBase **ppPart )
{
	bool bRes = false;
	for ( list<CPtr<CCombinedPart> >::const_iterator i = elements.begin(); i != elements.end(); ++i )
	{
		CCombinedPart *pCPart = *i;
		IVBCombiner *pVB = pCPart->GetVBCombiner();
		if ( !DoesIntersect( pVB->GetBound().s, r ) )
			continue;
		pCPart->UpdatePartInfo();
		CDGPtr<CFuncBase<vector< CPtr<IPart> > > > pC = pCPart->GetCombiner();
		pC.Refresh();
		const vector< CPtr<IPart> > &parts = pC->GetValue();
		const vector<SSphere> &partBVs = pVB->GetBounds();
		const vector<CCombinedPart::SPartInfo> &partInfos = pCPart->GetPartsInfo();
		for ( int k = 0; k < parts.size(); ++k )
		{
			const CCombinedPart::SPartInfo &pi = partInfos[k];
			if ( !pi.groupInfo.IsMaskMatch( mask ) )
				continue;
			if ( !DoesIntersect( partBVs[k], r ) )
				continue;
			IPart *pPart = parts[k];
			vector<CVec3> points;
			vector<STriangle> tris;
			TransformPart( pPart, &points, &tris );
			if ( Collide( r, points, tris, pfT, pNormal ) )
			{
				if ( pGroupInfo )
				{
					if ( CDynamicCast<ISomePart> pS = pPart )
            *pGroupInfo = pS->GetFullGroupInfo();
				}
				if ( ppPart )
					*ppPart = pPart;
				bRes = true;
			}
		}
	}
	return bRes;
}

bool CGScene::TraceParts( ERLRequest req, const SGroupSelect &mask, CVolumeNode *pNode, const SRayInfo &r, 
	float *pfT, CVec3 *pNormal, SFullGroupInfo *pGroupInfo, CObjectBase **ppPart )
{
	if ( !IsValid( pNode ) )
		return false;
	// could be optimized by rejecting volume node by distance
	SSphere nodeBound;
	pNode->GetBound( &nodeBound );
	if ( !DoesIntersect( nodeBound, r ) )
		return false;
	// CRAP - account lit particles in trace
	bool bRes = false;
	if ( req & RN_STATIC )
		bRes |= TestParts( pNode->staticParts.elements, mask, r, pfT, pNormal, pGroupInfo, ppPart );
	if ( req & RN_DYNAMIC )
		bRes |= TestParts( pNode->dynamicParts.elements, mask, r, pfT, pNormal, pGroupInfo, ppPart );
	for ( int k = 0; k < 8; ++k )
		bRes |= TraceParts( req, mask, pNode->GetNode( k ), r, pfT, pNormal, pGroupInfo, ppPart );
	return bRes;
}

bool CGScene::TraceScene( const SGroupSelect &mask, const CRay &r, float *pfT, CVec3 *pNormal, EScenePartsSet ps, SFullGroupInfo *pGroupInfo, CObjectBase **ppPart )
{
	*pfT = 1;
	SRayInfo rayInfo( r );
	int req = 0;
	if ( ps & SPS_STATIC )
		req |= RN_STATIC;
	if ( ps & SPS_DYNAMIC )
		req |= RN_DYNAMIC;
	return TraceParts( (ERLRequest)req, mask, pVolume, rayInfo, pfT, pNormal, pGroupInfo, ppPart );
}

static bool CheckMaterial( IMaterial *pMat )
{
	//ASSERT( IsValid( pMat ) );
	if ( !IsValid( pMat ) )
		return false;
	return true;
}

static bool CheckOI( CPtrFuncBase<CObjectInfo> *pInfo )//, SBound *pBV )
{
	CDGPtr<CPtrFuncBase<CObjectInfo> > pOI( pInfo );
	pOI.Refresh();
	if ( IsValid( pOI->GetValue() ) && pOI->GetValue()->IsEmpty() )
	{
		pOI.Extract();
		return false;
	}
	//pOI->GetValue()->CalcBound( pBV );
	pOI.Extract();
	return true;
}

void CGScene::WalkNotLoadedObjects()
{
	for ( list< CPtr<ISomePart> >::iterator i = toBeLoaded.begin(); i != toBeLoaded.end(); )
	{
		ISomePart *pPart = *i;
		if ( IsValid(pPart) )
		{
			if ( pPart->HasLoadedObjectInfo() )
			{
				SBound bv;
				pPart->GetObjectInfo()->CalcBound( &bv );
				CVec3 vCenter;
				float fR;
				switch ( pPart->GetTransformType() )
				{
					case TT_SIMPLE:
						{
							const SFBTransform &trans = pPart->GetSimplePos();
							trans.forward.RotateHVector( &vCenter, bv.s.ptCenter );
							fR = sqrt( CalcRadius2( bv, trans.forward ) );
						}
						break;
					case TT_NONE:
						vCenter = bv.s.ptCenter;
						fR = bv.s.fRadius;
						break;
					default:
						fR = 0;
						ASSERT(0);
						break;
				}
				PlaceToOctree( pPart, pVolume, vCenter, fR, &trackers, false );
				pDecalsManager->OnCreate( pPart );
				i = toBeLoaded.erase( i );
			}
			else
				++i;
		}
		else
			i = toBeLoaded.erase( i );
	}
	for ( list< CPtr<CStaticAnimatedPart> >::iterator i = toBeLoadedAnimated.begin(); i != toBeLoadedAnimated.end(); )
	{
		CStaticAnimatedPart *pPart = *i;
		if ( IsValid(pPart) )
		{
			if ( pPart->HasLoadedObjectInfo() )
			{
				const SBound &bv = pPart->GetBound();
				PlaceToOctree( pPart, pVolume, bv.s.ptCenter, bv.s.fRadius, &trackers, true );
				//pDecalsManager->OnCreate( pPart );
				i = toBeLoadedAnimated.erase( i );
			}
			else
				++i;
		}
		else
			i = toBeLoadedAnimated.erase( i );
	}
}

CObjectBase* CGScene::CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
	const SFullGroupInfo &_ginfo )
{
	CPtr< CPtrFuncBase<CObjectInfo> > pInfoHolder = pInfo;
	CPtr<IMaterial> pMaterialHolder = pMat;
	//
	if ( !CheckMaterial( pMat ) )
		return 0;
	if ( !CheckOI( pInfo ) )
		return 0;
	ISomePart *pRes = new CNonePart( pInfo, pMat, _ginfo );
	toBeLoaded.push_back( pRes );
	return pRes;
}

CObjectBase* CGScene::CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
	const SFBTransform &trans, const SFullGroupInfo &_ginfo )
{
	CPtr< CPtrFuncBase<CObjectInfo> > pInfoHolder = pInfo;
	CPtr<IMaterial> pMaterialHolder = pMat;
	//
	if ( !CheckMaterial( pMat ) )
		return 0;
	if ( !CheckOI( pInfo ) )
		return 0;
	CSimplePart *pRes = new CSimplePart( pInfo, pMat, _ginfo, trans );
	toBeLoaded.push_back( pRes );
	return pRes;
}

CObjectBase* CGScene::CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
	CFuncBase<SFBTransform> *pPlacement, const SBound &hintBV, const SFullGroupInfo &_ginfo )
{
	CPtr< CPtrFuncBase<CObjectInfo> > pInfoHolder = pInfo;
	CPtr<IMaterial> pMaterialHolder = pMat;
	//
	if ( !CheckMaterial( pMat ) )
		return 0;
	CDynamicPart *pRes = new CDynamicPart( pInfo, pPlacement, pMat, _ginfo );
	GetUpdatable( pVolume, hintBV )->updatable.movingParts.push_back( pRes );
	pDecalsManager->OnCreate( pRes );
	return pRes;
}

CObjectBase* CGScene::CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
	CFuncBase<SFBTransform> *pPlacement, CFuncBase<SBound> *_pBound, const SBound &hintBV, const SFullGroupInfo &_ginfo )
{
	CPtr< CPtrFuncBase<CObjectInfo> > pInfoHolder = pInfo;
	CPtr<IMaterial> pMaterialHolder = pMat;
	//
	if ( !CheckMaterial( pMat ) )
		return 0;
	CDynamicPartWithAnimatedBound *pRes = new CDynamicPartWithAnimatedBound( pInfo, pPlacement, _pBound, pMat, _ginfo );
	GetUpdatable( pVolume, hintBV )->updatable.animatedParts.push_back( pRes );
	pDecalsManager->OnCreate( pRes );
	return pRes;
}

CObjectBase* CGScene::CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
	CFuncBase<vector<SHMatrix> > *pPlacement, CFuncBase<vector<NGfx::SCompactTransformer> > *_pMMXAnim, 
	CFuncBase<SBound> *_pBound, const SBound &hintBV, const SFullGroupInfo &_ginfo )
{
	ASSERT( IsValid(pInfo) );
	if ( !IsValid(pInfo) )
		return 0;
	CPtr< CPtrFuncBase<CObjectInfo> > pInfoHolder = pInfo;
	CPtr<IMaterial> pMaterialHolder = pMat;
	//
	if ( !CheckMaterial( pMat ) )
		return 0;
	CAnimatedPart *pRes = new CAnimatedPart( pInfo, pPlacement, _pMMXAnim, _pBound, pMat, _ginfo );
	GetUpdatable( pVolume, hintBV )->updatable.animatedParts.push_back( pRes );
	pDecalsManager->OnCreate( pRes );
	return pRes;
}

CObjectBase* CGScene::CreateGeometry( CPtrFuncBase<CObjectInfo> *pInfo, IMaterial *pMat, 
	CFuncBase<vector<SHMatrix> > *pPlacement, CFuncBase<vector<NGfx::SCompactTransformer> > *_pMMXAnim, 
	const SBound &_bv, const SFullGroupInfo &_ginfo )
{
	ASSERT( IsValid(pInfo) );
	if ( !IsValid(pInfo) )
		return 0;
	CPtr< CPtrFuncBase<CObjectInfo> > pInfoHolder = pInfo;
	CPtr<IMaterial> pMaterialHolder = pMat;
	//
	if ( !CheckMaterial( pMat ) )
		return 0;
	CStaticAnimatedPart *pRes = new CStaticAnimatedPart( pInfo, pPlacement, _pMMXAnim, _bv, pMat, _ginfo );
	toBeLoadedAnimated.push_back( pRes );
	return pRes;
}

CObjectBase* CGScene::CreateDynamicGeometry( CPtrFuncBase<CObjectInfo> *pInfo, CFuncBase<SFBTransform> *pTransform, IMaterial *pMat, 
	CFuncBase<SBound> *pBound, const SBound &hintBV, const SFullGroupInfo &_ginfo )
{
	ASSERT( IsValid(pInfo) );
	if ( !IsValid(pInfo) )
		return 0;
	CPtr< CPtrFuncBase<CObjectInfo> > pInfoHolder = pInfo;
	CPtr<IMaterial> pMaterialHolder = pMat;
	//
	if ( !CheckMaterial( pMat ) )
		return 0;
	CDynamicGeometryPart *pRes = new CDynamicGeometryPart( pInfo, pTransform, pBound, pMat, _ginfo );
	GetUpdatable( pVolume, hintBV )->updatable.dynamicFrags.push_back( pRes );
	pDecalsManager->OnCreate( pRes );
	return pRes;
}

CObjectBase* CGScene::CreateParticles( CPtrFuncBase<CParticleEffect> *pInfo, 
	CFuncBase<SFBTransform> *pPlacement, const SBound &bound, const SBound &hintBV, const SGroupInfo &_ginfo, int nPFlags )
{
	CParticles* pRes = new CParticles( pInfo, pPlacement, bound, _ginfo, nPFlags );
	//GetUpdatable( pVolume, hintBV )->updatable.particles.push_back( pRes );
	pRes->Update( pVolume );
	return pRes;
}

CPolyline* CGScene::CreatePolyline( CPtrFuncBase<NGfx::CGeometry> *pGeometry, const vector<unsigned short> &indices, 
	const CVec4 &color, bool bDepthTest )
{
	CPolyline *pR = new CPolyline( pGeometry, indices, color, bDepthTest );
	lines.push_back( pR );
	return pR;
}

CObjectBase* CGScene::CreatePostProcessor( CObjectBase *pRenderNode, IPostProcess *pProcessor )
{
	CPtr<CPostProcessBinder> p = new CPostProcessBinder;
	if ( p->Initialize( pRenderNode, pProcessor ) )
	{
		postprocessors.push_back( p );
		return p;
	}
	return 0;
}

void CGScene::SetAmbientAnimation( CPtrFuncBase<CAnimLight> *pLight )
{
	pAmbientAnimator->SetAmbientAnimation( pLight );
}

CObjectBase* CGScene::AddPointLight( const CVec3 &_vColor, const CVec3 &ptOrigin, float fR )
{
	if ( fR <= 0 )
		return 0;
	CPointLight *pRes = new CPointLight( _vColor, ptOrigin, fR );
	pLightState->AddPointLight( pRes );
	return pRes;
}

CObjectBase* CGScene::AddPointLight( CPtrFuncBase<CAnimLight> *pLight )
{
	CDynamicPointLight *pRes = new CDynamicPointLight( pLight );
	pLightState->AddPointLight( pRes );
	return pRes;
}

CObjectBase* CGScene::AddSpotLight( CFuncBase<CVec3> *pColor, const CVec3 &ptOrigin, const CVec3 &ptDir, 
	float fFOV, float fRadius, CPtrFuncBase<NGfx::CTexture> *pMask, bool bLightmapOnly )
{
	CPtr<CFuncBase<CVec3> > pHold(pColor);
	CPtr<CPtrFuncBase<NGfx::CTexture> > pHold1(pMask);
	if ( fRadius <= 0 )
		return 0;
	return 0;
/*
	CSpotLight *pLight = new CSpotLight( pColor, ptOrigin, ptDir, fFOV, fRadius, pMask, nTargetGroupID );
	pRes->AddLight( pLight );
	AddLight( pRes );
	return pRes;*/
}

void CGScene::SetDirectionalLight( CFuncBase<CVec3> *pColor, CFuncBase<CVec3> *pGlossColor, 
	const CVec3 &_vLightDir, const CVec3 &_vShadowsLightDir,
	float fMaxHeight, float fShadowsMaxDetailLength, float fBlurShift,
	CFuncBase<CVec3> *_pAmbient, CFuncBase<CVec3> *_pShadeColor, CFuncBase<CVec3> *_pIncidentShadeColor, const CVec3 &vParticlesColor,
	CPtrFuncBase<NGfx::CTexture> *_pClouds, CFuncBase<SHMatrix> *_pCloudsProjection, const CVec3 &vDymanicLightsModifications ) 
{
	pFakeParticleLM->SetColor( pColor );
	pParticlesLightColor->Set( vParticlesColor );
	CDirectionalLight *pDirectionalLight = new CDirectionalLight( pColor, pGlossColor,
		_vLightDir, _vShadowsLightDir, fMaxHeight, _pAmbient,
		_pShadeColor, _pIncidentShadeColor, 
		_pClouds, _pCloudsProjection, fShadowsMaxDetailLength, vDymanicLightsModifications );
	pAmbient = _pAmbient;
	pAmbientAnimator = new CAmbientAnimator( pAmbientAnimator->GetAnimation(), pAmbient );
	pLightState->SetDirectional( pDirectionalLight );
}

void CGScene::WalkOctree()
{
	if ( --nSlowVolumeWalk < 0 )
	{
		nSlowVolumeWalk = 30;
		pVolume->Walk();
		if ( IsValid(pDecalsManager) )
			pDecalsManager->Walk();
	}
}

static void SelectParts( CPartFlags *pRes, CTransformStack *pTS, IVBCombiner *pVB, 
	CCombinedPart *p, const SGroupSelect &mask )
{
	const vector<CCombinedPart::SPartInfo> &partInfos = p->GetPartsInfo();
	int nParts = partInfos.size();
	pRes->Clear();
	if ( pTS->IsFullGet() )
	{
		for ( int i = 0; i < nParts; ++i )
		{
			if ( !partInfos[ i ].groupInfo.IsMaskMatch( mask ) )
				continue;
			pRes->Set( i );
		}
	}
	else
	{
		const vector<SSphere> &partBVs = pVB->GetBounds();
		for ( int i = 0; i < nParts; ++i )
		{
			if ( !partInfos[ i ].groupInfo.IsMaskMatch( mask ) )
				continue;
			if ( pTS->IsIn( partBVs[i] ) )
				pRes->Set( i );
		}
	}
}

static void CalcOpaque( CPartFlags *pRes, CCombinedPart *p )
{
	pRes->Clear();
	const vector<CCombinedPart::SPartInfo> &partsInfo = p->GetPartsInfo();
	for ( int k = 0; k < partsInfo.size(); ++k )
	{
		if ( partsInfo[k].groupInfo.nObjectGroup & N_MASK_OPAQUE )
			pRes->Set( k );
	}
}

static void AddParts( CTransformStack *pTS, list<SRenderPartSet> *pRes, 
	const vector<CObj<CCombinedPart> > &elems, const SGroupSelect &mask )
{
	for ( vector<CObj<CCombinedPart> >::const_iterator i = elems.begin(); i != elems.end(); ++i )
	{
		CCombinedPart *pElement = *i;
		if ( !IsValid( pElement ) )
			continue;
		pElement->UpdatePartInfo();
		IVBCombiner *pVB = pElement->GetVBCombiner();

		if ( !pTS->PushClipHint( pVB->GetBound() ) )
			continue;

		CDGPtr<CPerMaterialCombiner> pCombiner = pElement->GetCombiner();
		pCombiner.Refresh();
		const vector< CPtr<IPart> > &listParts = pCombiner->GetValue();
		SRenderPartSet &res = *pRes->insert( pRes->end(), SRenderPartSet( pElement, &listParts, pElement->GetGeometryInfo(), pElement->GetFloorMask() ) );
		SelectParts( &res.parts, pTS, pVB, pElement, mask );
		CalcOpaque( &res.opaque, pElement );

		pTS->PopClipHint();
	}
}

void CGScene::SelectNodes( CTransformStack *pTS, CVolumeNode *pNode, vector<CVolumeNode*> *pRes )
{
	if ( pNode == 0 )
		return;

	SSphere sClipTest;
	pNode->GetBound( &sClipTest );
	if ( !pTS->PushClipHint( sClipTest ) )
		return;

	pNode->updatable.Update( nFrameCounter, pVolume, &trackers );
	pRes->push_back( pNode );

	for ( int i = 0; i < 8; ++i )
		SelectNodes( pTS, pNode->GetNode(i), pRes );

	pTS->PopClipHint();
}

void CGScene::MakePartList( CTransformStack *pTS, list<SRenderPartSet> *pRes, ERLRequest eReq, const SGroupSelect &mask )
{
	vector<CVolumeNode*> volumeNodes;
	SelectNodes( pTS, pVolume, &volumeNodes );
	
	for ( int i = 0; i < volumeNodes.size(); ++i )
	{
		CVolumeNode *pNode = volumeNodes[i];
		if ( eReq & RN_DYNAMIC )
			AddParts( pTS, pRes, pNode->dynamicParts.normal, mask );
		if ( eReq & RN_STATIC )
			AddParts( pTS, pRes, pNode->staticParts.normal, mask );
	}
}

void CGScene::SSceneFragmentGroupInfo::FilterParts( vector<CPartFlags> *pRes, CTransformStack *pTS, 
	CCombinedPart *p, ERLRequest req, int _nIgnoreMark )
{
	pRes->resize( 0 );//clear();

	const SBound &bv = p->GetVBCombiner()->GetBound();

	if ( !pTS->PushClipHint( bv ) )//IsIn( bv ) )//
	{
		if ( bLowRAM )
			p->GetVBCombiner()->FreeMemory();
		return;
	}

	bool bIsMatchingIgnore = p->GetIgnoreMark() == _nIgnoreMark;
	if ( !bIsMatchingIgnore && pHZBuffer && !pHZBuffer->IsVisible( bv.s, pTS ) )
	{
		pTS->PopClipHint();
		return;
	}

	p->UpdatePartInfo();
	int nOriginalPartsNum = p->GetCombiner()->GetValue().size();

	nTotalParts += nOriginalPartsNum;
	++nTotalElements;

	//	bool bSkipPerPartTests = nOriginalPartsNum == 1;
	vector<CPartFlags> &flags = *pRes;
	flags.resize( p->GetMaterialsNumber() );
	for ( int k = 0; k < flags.size(); ++k )
		flags[k].Clear();

	const SHMatrix &matTransform = pTS->Get().backward;
	CVec3 vCameraPosition( matTransform._14, matTransform._24, matTransform._34 );
	if (  fabs( matTransform._44 ) > FP_EPSILON )
		vCameraPosition /= matTransform._44;

	const vector<CCombinedPart::SPartInfo> &partsInfo = p->GetPartsInfo();
	if ( bIsMatchingIgnore )
	{
		const CPartFlags &ignored = p->GetIgnoredParts();
		for ( int k = 0; k < nOriginalPartsNum; ++k )
		{
			const CCombinedPart::SPartInfo &r = partsInfo[ k ];
			if ( ignored.IsSet( k ) )
				continue;
			flags[ r.nMaterial ].Set( k );
		}
	}
	else
	{
		const vector<SSphere> &bounds = p->GetVBCombiner()->GetBounds();
		for ( int k = 0; k < nOriginalPartsNum; ++k )
		{
			const CCombinedPart::SPartInfo &r = partsInfo[ k ];
			if ( !r.groupInfo.IsMaskMatch( mask ) )
				continue;

			if ( pTS->IsIn( bounds[k] ) )
			{
				if ( !(p->GetFloorMask() & N_MASK_LOD) )
					flags[ r.nMaterial ].Set( k );
				else
				{
					if ( p->GetFloorMask() & N_MASK_LOD_HIGH )
					{ 
						if ( fabs2(bounds[k].ptCenter - vCameraPosition ) < s_fLODSwitchDistance*s_fLODSwitchDistance )
							flags[ r.nMaterial ].Set( k );
					}
					else if ( fabs2(bounds[k].ptCenter - vCameraPosition ) > s_fLODSwitchDistance*s_fLODSwitchDistance )
						flags[ r.nMaterial ].Set( k );
				}
			}
		}
	}
	pTS->PopClipHint();
}

void CGScene::SSceneFragmentGroupInfo::AddElement( CSceneFragments *pRes, CTransformStack *pTS, CCombinedPart *p, ERLRequest req, 
	int _nIgnoreMark )
{
	vector<CPartFlags> take;
	FilterParts( &take, pTS, p, req, _nIgnoreMark );
	if ( take.empty() )
		return;

	bool bIgnoreZ = ( p->GetFloorMask() & N_MASK_IGNOREZ ) != 0;
	int nGeometry = pRes->AddGeometry( p, p->GetGeometryInfo(), p->GetVBCombiner()->GetBound(), bIgnoreZ );

	for ( int k = 0; k < take.size(); ++k )
	{
		const CCombinedPart::SMaterialInfo &m = p->GetMaterial( k );
		pRes->AddElement( nGeometry, take[ k ], m.pMaterial, m.vars );
	}
}

void CGScene::SSceneFragmentGroupInfo::AddTranspElement( CCombinedPart *p, const vector<CPartFlags> &flags )
{
	if ( flags.empty() )
		return;
	SRenderGeometryInfo *pGeom = p->GetGeometryInfo();
	const vector<SSphere> &partBVs = pGeom->pVertices->GetBounds();
	for ( int k = 0; k < flags.size(); ++k )
	{
		const CCombinedPart::SMaterialInfo &m = p->GetMaterial( k );
		IMaterial *pMat = m.pMaterial;
		const SPerPartVariables &vars = m.vars;
		for ( int i = 0; i < PF_MAX_PARTS_PER_COMBINER; i += 32 )
		{
			if ( flags[k].GetBlock( i / 32 ) )
			{
				for ( int n = i; n < i + 32; ++n )
				{
					if ( flags[k].IsSet( n ) )
						pTransp->AddElement( pGeom, pMat, vars, n, partBVs[n].ptCenter * pTransp->GetDepth() );
				}
			}
		}
	}
}

void CGScene::SSceneFragmentGroupInfo::AddStaticLMElement( CCombinedPart *p, CTransformStack *pTS,
	ERLRequest req, int _nIgnoreMark )
{
	vector<CPartFlags> take;
	FilterParts( &take, pTS, p, req, _nIgnoreMark );
	if ( take.empty() )
		return;
	CPartFlags allTaken( take[0] );
	for ( int k = 1; k < take.size(); ++k )
		allTaken |= take[k];

	bool bIgnoreZ = ( p->GetFloorMask() & N_MASK_IGNOREZ ) != 0;
	const SBound &bv = p->GetGeometryInfo()->pVertices->GetBound();
	int nGeometry = pList->AddGeometry( p, p->GetGeometryInfo(), bv, bIgnoreZ );
	for ( int k = 0; k < take.size(); ++k )
	{
		const CCombinedPart::SMaterialInfo &m = p->GetMaterial( k );
		pList->AddElement( nGeometry, take[k], m.pMaterial, m.vars );
	}
}

CGScene::SSceneFragmentGroupInfo::SSceneFragmentGroupInfo( const SGroupSelect &_mask, CSceneFragments *_pList, CTransparentRenderer *_pTransp ) : mask(_mask), 
	pList(_pList), pTransp(_pTransp), pHZBuffer(0)
{
}

CGScene::SSceneFragmentGroupInfo::SSceneFragmentGroupInfo( const SGroupSelect &_mask, CSceneFragments *_pList, CTransparentRenderer *_pTransp, IHZBuffer *_pHZBuffer ) : mask(_mask), 
	pList(_pList), pTransp(_pTransp), nLMTextureUsed(0), pHZBuffer(_pHZBuffer)
{
}

void CGScene::SSceneFragmentGroupInfo::AddMaterialHolder( CTransformStack *pTS, 
	const CVolumeNode::SPerMaterialHolder &h, ERLRequest req, int _nIgnoreMark )
{
	if ( pTransp )
	{
		for ( int i = 0; i < h.transparent.size(); ++i )
		{
			CCombinedPart *p = h.transparent[i];
			vector<CPartFlags> flags;
			FilterParts( &flags, pTS, p, req, _nIgnoreMark );
			AddTranspElement( p, flags );
		}
	}
	if ( req & RN_DEPTH )
	{
		ASSERT( ( req & RN_LIGHTMAPS ) == 0 );
		for ( int i = 0; i < h.transparent.size(); ++i )
			AddElement( pList, pTS, h.transparent[i], req, _nIgnoreMark );
	}
	for ( int i = 0; i < h.normal.size(); ++i )
	{
		if ( (req & RN_LIGHTMAPS) == 0 )
			AddElement( pList, pTS, h.normal[i], req, _nIgnoreMark );
		else
			AddStaticLMElement( h.normal[i], pTS, req, _nIgnoreMark );
	}
}

struct SPerFloorStuff
{
	int nFloorMask;
	vector<CCombinedPart*> normal;

	SPerFloorStuff( int _n = 0 ) : nFloorMask(_n) {}
	void Add( CCombinedPart *p )
	{
		normal.push_back( p );
	}
	bool IsValidNormal() const
	{
		for ( int k = 0; k < normal.size(); ++k )
		{
			if ( normal[k]->GetGeometryInfo()->pVertices->IsValidValue() )
				return true;
		}
		return false;
	}
};

struct SLitParticlesAdder : public IReportParticlesGeometry
{
	CSceneFragments *pList;
	CTransformStack *pTS;
	SLitParticlesAdder( CSceneFragments *_pList, CTransformStack *_pTS )
		: pList(_pList), pTS(_pTS) {}
	virtual void AddParticles( IVBCombiner *pVertices, CFuncBase<vector<NGfx::STriangleList> > *pTrilists, 
		int nPart, int nParticles, const SBound &bv )
	{
		if ( nParticles )
		{
			if ( pTS->IsIn( bv ) )
				pList->AddLitParticles( pVertices, pTrilists, nPart, bv );
		}
	}
};

void CGScene::MakeRenderList( CTransformStack *pTS, SSceneFragmentGroupInfo *pFragmentsInfo, 
	ERLRequest req, int nIgnoreMark )
{
	vector<CVolumeNode*> volumeNodes;
	SelectNodes( pTS, pVolume, &volumeNodes );

	for ( int i = 0; i < volumeNodes.size(); ++i )
	{
		CVolumeNode *pNode = volumeNodes[i];

		bool bDoLightmpasCalcDynamicAmbient = ( req & RN_LIGHTMAPS ) != 0;
		unsigned short nWasMaskEvery = pFragmentsInfo->mask.nMaskEvery;
		bool bUseHQAdd = false;

		if ( req & RN_DYNAMIC )
			pFragmentsInfo->AddMaterialHolder( pTS, pNode->dynamicParts, req, nIgnoreMark );//N_SKIP_IGNORED_TEST );
		if ( req & RN_STATIC )
			pFragmentsInfo->AddMaterialHolder( pTS, pNode->staticParts, req, nIgnoreMark );

		for ( list<CPtr<CParticles> >::iterator k = pNode->particles.begin(); k != pNode->particles.end(); )
		{
			CParticles *pPart =*k;
			if ( !IsValid( pPart ) )
			{
				k = pNode->particles.erase( k );
				continue;
			}
			else
				++k;
			if ( !pPart->GetGroup().IsMaskMatch( pFragmentsInfo->mask ) )
				continue;
			// check type
			if ( pPart->IsDynamic() )
			{
				if ( ( req & RN_DYNAMIC ) == 0 )
					continue;
			}
			else
			{
				if ( ( req & RN_STATIC ) == 0 )
					continue;
			}
			const SBound &bv = pPart->GetBound();
			if ( pTS->IsIn( bv.s ) && ( nIgnoreMark == N_SKIP_IGNORED_TEST || !pHZBuffer || pHZBuffer->IsVisible( bv.s, pTS ) ) )
			{
				CTransparentRenderer *pTransp = pFragmentsInfo->pTransp;
				ASSERT( pTransp );
				if ( pTransp )
				{
					CSceneFragments *pAddList = pFragmentsInfo->pList;

					if ( req & RN_DEPTH )
					{
						pAddList->SetLitParticlesMaterial( pTransparentMaterial );
						SLitParticlesAdder adder( pAddList, pTS );
						pTransp->AddParticles( pPart, true, bv, &adder );
					}
					else
					{
						if ( pPart->IsLit() )
						{
							pAddList->SetLitParticlesMaterial( pTransparentMaterial );
							SLitParticlesAdder adder( pAddList, pTS );
							pTransp->AddParticles( pPart, true, bv, &adder );
						}
						else
							pTransp->AddParticles( pPart, false, bv, 0 );
					}
				}
			}
		}
	}
	if ( pFragmentsInfo->pTransp )
		pFragmentsInfo->pTransp->FinishParticles();
}

static float fPCLow = 0.15f, fPCHigh = 5;
static void AddPCElement( CSceneFragments *pRes, CCombinedPart *p )
{
	int nGeometry = pRes->AddGeometry( p, p->GetGeometryInfo(), p->GetVBCombiner()->GetBound(), false );

	p->UpdatePartInfo();

	const vector<CCombinedPart::SPartInfo> &partsInfo = p->GetPartsInfo();
	CDGPtr<CFuncBase< vector< CPtr<IPart> > > > pParts = p->GetCombiner();
	pParts.Refresh();
	const vector<CPtr<IPart> > parts = pParts->GetValue();
	for ( int i = 0; i < parts.size(); ++i )
	{
		IPart *pPart = parts[i];
		const CCombinedPart::SPartInfo &r = partsInfo[i];
		const CCombinedPart::SMaterialInfo &m = p->GetMaterial( r.nMaterial );
		SPerPartVariables vars = m.vars;
		float f = ( log( pPart->fAverageTriArea ) - log ( fPCLow ) ) / ( log( fPCHigh ) - log( fPCLow ) );
		vars.fFade = Clamp( f, 0.0f, 1.0f );

		CPartFlags pf;
		pf.Clear();
		pf.Set( i );
		pRes->AddElement( nGeometry, pf, m.pMaterial, vars );
	}
}
static void AddPCMaterialHolder( const CVolumeNode::SPerMaterialHolder &h, CSceneFragments *pList )
{
	for ( int i = 0; i < h.transparent.size(); ++i )
		AddPCElement( pList, h.transparent[i] );
	for ( int i = 0; i < h.normal.size(); ++i )
		AddPCElement( pList, h.normal[i] );
}
void CGScene::MakePolycountRenderList( CTransformStack *pTS, CSceneFragments *pList )
{
	vector<CVolumeNode*> volumeNodes;
	SelectNodes( pTS, pVolume, &volumeNodes );

	for ( int i = 0; i < volumeNodes.size(); ++i )
	{
		CVolumeNode *pNode = volumeNodes[i];
		AddPCMaterialHolder( pNode->dynamicParts, pList );
		AddPCMaterialHolder( pNode->staticParts, pList );
	}
}

template<class T> int PrecacheMaterialsForSet( const T &s, ILoadingCounter *pCounter )
{
	int nRes = 0;
	for ( T::const_iterator i = s.begin(); i != s.end(); ++i )
	{
		if ( IsValid( *i ) )
			(*i)->GetMaterial()->Precache();
		if ( pCounter )
			pCounter->Step();
		++nRes;
	}
	return nRes;
}

int CGScene::PrecacheMaterials( CVolumeNode *pNode, ILoadingCounter *pCounter )
{
	if ( !IsValid(pNode) )
		return 0;
	int nRes = 0;
	for ( list<CPtr<CCombinedPart> >::iterator i = pNode->staticParts.elements.begin(); i != pNode->staticParts.elements.end(); ++i )
	{
		CCombinedPart *p = *i;
		for ( int i = 0; i < p->GetMaterialsNumber(); ++i )
		{
			p->GetMaterial( i ).pMaterial->Precache();
			if ( pCounter )
				pCounter->Step();
		}
		nRes += p->GetMaterialsNumber();
	}
	nRes += PrecacheMaterialsForSet( pNode->updatable.dynamicFrags, pCounter );
	nRes += PrecacheMaterialsForSet( pNode->updatable.animatedParts, pCounter );
	nRes += PrecacheMaterialsForSet( pNode->updatable.movingParts, pCounter );
	for ( int i = 0; i < 8; ++i )
		nRes += PrecacheMaterials( pNode->GetNode(i), pCounter );
	return nRes;
}

int CGScene::PrecacheMaterials( ILoadingCounter *pCounter )
{
	int nRes = PrecacheMaterials( pVolume, pCounter );
	nRes += PrecacheMaterialsForSet( toBeLoaded, pCounter );
	nRes += PrecacheMaterialsForSet( toBeLoadedAnimated, pCounter );
	return nRes;
}

void CGScene::RecalcRenderStats( int nSceneTris, int nParticles, int nLitParticles )
{
	lastFrameStats.nSceneTris += nSceneTris;
	lastFrameStats.nParticles += nParticles;
	lastFrameStats.nLitParticles += nLitParticles;
//	if ( IsValid(pLMTracker) )
//		lastFrameStats.nScenePointLights += pLMTracker->GetCastingShadowsPointLightsNumber();
//	lastFrameStats.bPointLightShadowThrashing = IsCubeTextureShareThrashing();
}

//void CGScene::UpdateIgnoreMark( IRender *pRender, CTransformStack *pTS, const SGroupSelect &mask, EHSRMode hsrMode )
//{
//	bool bStaticUpdated = true;//pIgnoreStaticTrack.Refresh();
//	bool bStrongMove = fabs2( pTS->Get().forward.GetTranslation() - mHoldTransform.GetTranslation() ) > sqr(1);
//	bool bStrongReason = bStaticUpdated || bStrongMove || holdMask != mask;
//	if ( bStrongReason || pTS->Get().forward != mHoldTransform )
//	{
//		if ( hsrMode == HSR_FAST )
//		{
//			nIgnoreListWasCalced = 0;
//			++nCurrentIgnoreMark;  
//			pHZBuffer = 0;
//		}
//		else
//		{
//			nIgnoreListWasCalced = 0;
//			if ( ++nReuseIgnoreList == 2 )
//				bStrongReason = true;
//			if ( bStrongReason )
//			{
//				nReuseIgnoreList = 0;
//				++nCurrentIgnoreMark;  
//				pHZBuffer = 0;
//				CIgnorePartsHash res;
//				MakeInvisibleElementsListFast( pRender, pTS, mask, GetScreenRect(), &res, &pHZBuffer );
//				for ( CIgnorePartsHash::iterator i = res.begin(); i != res.end(); ++i )
//				{
//					CDynamicCast<CCombinedPart> pC( i->first );
//					pC->SetIgnored( nCurrentIgnoreMark, i->second );
//				}
//			}
//		}
//	}
//	else
//	{
//		if ( nIgnoreListWasCalced == 2 )
//		{
//			++nCurrentIgnoreMark;
//			CIgnorePartsHash res;
//			MakeInvisibleElementsList( pRender, pTS, mask, GetScreenRect(), &res, &pHZBuffer );
//			for ( CIgnorePartsHash::iterator i = res.begin(); i != res.end(); ++i )
//			{
//				CDynamicCast<CCombinedPart> pC( i->first );
//				pC->SetIgnored( nCurrentIgnoreMark, i->second );
//			}
//		}
//		++nIgnoreListWasCalced;
//	}
//	mHoldTransform = pTS->Get().forward;
//	holdMask = mask;
//}

//bool CGScene::NeedUseHWHSR() const
//{
//	if ( !bUseHWHSR )
//		return false;
//	if ( !NGfx::DoesSupportOcclusionQueries() )
//		return false;
//	return nIgnoreListWasCalced < 2;
//}

void CGScene::DrawPostProcess( CTransformStack *pTS, NGfx::CRenderContext *pRC, const SGroupSelect &mask )
{
	if ( postprocessors.empty() )
		return;
	UpdateSet( &postprocessors, this );
	typedef hash_map<CPtr<IPostProcess>, vector<IPostProcess::SObject>, SPtrHash> CPostHash;
	CPostHash postHash;
	for ( list< CPtr<CPostProcessBinder> >::iterator i = postprocessors.begin(); i != postprocessors.end(); ++i )
		(*i)->Store( &postHash[ (*i)->GetPostProcessor() ], pTS, mask );
	SPostProcessData data;
	for ( CPostHash::iterator i = postHash.begin(); i != postHash.end(); ++i )
	{
		if ( i->second.empty() )
			continue;
		i->first->Render( &data, i->second );
	}
	NGScene::RenderPostProcess( pRC, data );
}

static void MakeSunFlareRect( CVec2 *pQuadPos, const CVec2 &sPos, const CVec2 &sDir, const CVec2 &_sSize, float fScale )
{
	CVec2 sDirT( -sDir.y, sDir.x );
	CVec2 sSize( _sSize / 2 * fScale );
	////
	pQuadPos[0] = sPos - sDir * sSize.x - sDirT * sSize.y;
	pQuadPos[1] = sPos - sDir * sSize.x + sDirT * sSize.y;
	pQuadPos[2] = sPos + sDir * sSize.x + sDirT * sSize.y;
	pQuadPos[3] = sPos + sDir * sSize.x - sDirT * sSize.y;
}

static float MakeSunFlareFadeCoeff( const CVec2 &sPos, const CVec2 &sHalfScreen )
{
	return 1.0f - Min( 1.0f, fabs( sPos ) / fabs( sHalfScreen ) );
}

void CGScene::DrawSunFlares( CTransformStack *pTS, NGfx::CRenderContext *pRC )
{
	if ( !IsValid( pSunFlaresTime ) || !IsValid( pCamera ) )
		return;
	////
	CDGPtr<CFuncBase<CVec4> > pCameraPos = pCamera;
	pCameraPos.Refresh();
	pSunFlaresTime.Refresh();
	////
	CVec4 sCamera = pCameraPos->GetValue();
	CVec3 vSunDir = sunFlareDir;
	CVec4 sSun = CVec4( -vSunDir, 0 );
	//sSun = AddHomogen( sSun, sCamera );
	CVec4 sSunProj;
	pTS->Get().forward.RotateHVector( &sSunProj, sSun );
	sSunProj.x = sSunProj.x / sSunProj.w;
	sSunProj.y = sSunProj.y / sSunProj.w;
	if ( ( fabs( sSunProj.x ) > 1 ) || ( fabs( sSunProj.y ) > 1 ) || ( sSunProj.w <= 0 ) )
	{
		fSunFlareCoeff = 0;
		return;
	}
	////
	bool bIsVisible = false;
	{
		CRay sTestRay;
		float fTemp;
		CVec3 sTempNormal;
		SGroupSelect sMask( GetLastMask() );
		////
		sTestRay.ptOrigin = SafeUnhomogen( sCamera ) - vSunDir * 1000;
		sTestRay.ptDir = vSunDir * 1000;//SafeUnhomogen( SubHomogen( sSun, sCamera ) );
		sMask.nMaskEvery |= N_MASK_CAST_SHADOW;
		if ( TraceScene( sMask, sTestRay, &fTemp, &sTempNormal, SPS_ALL, 0, 0 ) )
			bIsVisible = fTemp > 1;
		else
			bIsVisible = true;
	}
	////
	STime sDelta = pSunFlaresTime->GetValue() - sSunFlareTime;
	sSunFlareTime = pSunFlaresTime->GetValue();
	if ( !bIsVisible )
		fSunFlareCoeff -= float( sDelta ) / 500;
	else if ( fSunFlareCoeff < 1.0f )
		fSunFlareCoeff += float( sDelta ) / 500;
	////
	fSunFlareCoeff = Min( 1.0f, Max( 0.0f, fSunFlareCoeff ) );
	if ( fSunFlareCoeff < FP_EPSILON )
		return;
	////
	CVec2 sScreenRect( 1024, 768 );
	CVec2 sHalfScreenRect( sScreenRect / 2 );
	sSunProj.x = +sSunProj.x * sHalfScreenRect.x;
	sSunProj.y = -sSunProj.y * sHalfScreenRect.y;
	CVec2 sDir( sSunProj.x, sSunProj.y );
	Normalize( &sDir );
	////
	pRC->SetStencil( NGfx::STENCIL_NONE );
	pRC->SetAlphaCombine( NGfx::COMBINE_SMART_ALPHA );
	pRC->SetFog( NGfx::FOG_NONE );
	NGfx::C2DQuadsRenderer quadRender( *pRC, CVec2( 1024, 768 ), NGfx::QRM_DEPTH_NONE );
	for ( int nTemp = 0; nTemp < pSunFlares->flares.size(); ++nTemp )
	{
		CSunFlares::SFlare &flare = pSunFlares->flares[nTemp];
		if ( !IsValid( flare.pFlare ) )
			continue;
		////
		flare.pFlare.Refresh();
		NGfx::CTexture *pTexture = flare.pFlare->GetValue();
		////
		CVec2 sFlare( sSunProj.x, sSunProj.y );
		sFlare = sFlare * flare.fDistance + sHalfScreenRect;
		////
		float fXSize = 0, fYSize = 0;
		if ( CDynamicCast<NGfx::I2DBuffer> pObj = pTexture )
		{
			fXSize = pObj->GetSizeX();
			fYSize = pObj->GetSizeY();
		}
		////
		CVec2 sPos[4];
		NGfx::SPixel8888 sColors[4];
		float fScaleCoeff = fSunFlareCoeff * ( 1.0f + flare.fScale * MakeSunFlareFadeCoeff( sFlare - sHalfScreenRect, sHalfScreenRect ) );
		MakeSunFlareRect( sPos, sFlare, sDir, CVec2( fXSize, fYSize ), fScaleCoeff );
		for ( int nTemp = 0; nTemp < 4; ++nTemp )
		{
			DWORD dwVal = 0xFF * fSunFlareCoeff;
			if ( flare.bFade )
				dwVal *= MakeSunFlareFadeCoeff( sPos[nTemp] - sHalfScreenRect, sHalfScreenRect );
			/////
			sColors[nTemp].a = sColors[nTemp].r = sColors[nTemp].g = sColors[nTemp].b = dwVal;
		}
		////
		quadRender.AddRect( sPos, sColors, pTexture, CTRect<float>( 0, 0, fXSize, fYSize ) );
	}
	////
	if ( IsValid( pSunFlares->pOverbright ) )
	{
		pSunFlares->pOverbright.Refresh();
		NGfx::CTexture *pTexture = pSunFlares->pOverbright->GetValue();
		////
		float fXSize = 0, fYSize = 0;
		if ( CDynamicCast<NGfx::I2DBuffer> pObj = pTexture )
		{
			fXSize = pObj->GetSizeX();
			fYSize = pObj->GetSizeY();
		}
		////
		float fCoeff = MakeSunFlareFadeCoeff( CVec2( sSunProj.x, sSunProj.y ), sHalfScreenRect );
		DWORD dwColor = 0xFF * fSunFlareCoeff * fCoeff;
		NGfx::SPixel8888 sColor( dwColor, dwColor, dwColor, dwColor );
		////
		CVec2 sPos[4];
		NGfx::SPixel8888 sColors[4] = { sColor, sColor, sColor, sColor };
		MakeSunFlareRect( sPos, sHalfScreenRect, CVec2( 1, 0 ), sScreenRect, fCoeff );
		quadRender.AddRect( sPos, sColors, pTexture, CTRect<float>( 0, 0, fXSize, fYSize ) );
	}
}

struct SLineDrawIdx
{
	CVec4 vColor;
	bool bCheckDepth;

	bool operator==( const SLineDrawIdx &a ) const { return vColor == a.vColor && bCheckDepth == a.bCheckDepth; }
};
struct SLineDrawHash
{
	int operator()( const SLineDrawIdx &a ) const { const int *p = (const int*)&a.vColor; return ( p[0] + p[1] ) ^ (p[2]>>8); }
};
void CGScene::DrawLines( NGfx::CRenderContext *pRC )
{
	pRC->SetAlphaCombine( NGfx::COMBINE_SMART_ALPHA );
	pRC->SetStencil( NGfx::STENCIL_NONE );
	typedef hash_map<SLineDrawIdx, list<CPolyline*>,SLineDrawHash> CColorHash;
	CColorHash hashSel;
	for ( list< CPtr<CPolyline> >::iterator i = lines.begin(); i != lines.end(); )
	{
		CPolyline *p = *i;
		if ( IsValid(p) )
		{
			SLineDrawIdx idx;
			idx.bCheckDepth = p->GetCheckDepth();
			idx.vColor = p->GetColor();
			hashSel[ idx ].push_back( p );
			++i;
		}
		else
			i = lines.erase( i );
	}

	for ( CColorHash::iterator k = hashSel.begin(); k != hashSel.end(); ++k )
	{
		if ( k->first.bCheckDepth )
			pRC->SetDepth( NGfx::DEPTH_NORMAL );
		else
			pRC->SetDepth( NGfx::DEPTH_NONE );
		// works wrong with fog due to smart_alpha
		NGfx::SEffConstLight d;
		d.color = k->first.vColor;
		pRC->SetEffect( &d );
		const list<CPolyline*> &l = k->second;
		for ( list<CPolyline*>::const_iterator i = l.begin(); i != l.end(); ++i )
			(*i)->Render( pRC );
		pRC->Flush();
	}
}

CObjectBase* CGScene::CreateStaticDecal( ISomePart *pTarget, CPtrFuncBase<CObjectInfo> *pDecal, IMaterial *pMaterial, const SFullGroupInfo &fg )
{
	switch ( pTarget->GetTransformType() )
	{
		case TT_NONE: 
			return CreateGeometry( pDecal, pMaterial, fg );
		case TT_SIMPLE: 
			return CreateGeometry( pDecal, pMaterial, pTarget->GetSimplePos(), fg );
	}
	return 0;
}

CObjectBase* CGScene::CreateDynamicDecal( ISomePart *pTarget, CPtrFuncBase<CObjectInfo> *pDecal, IMaterial *pMaterial, const SFullGroupInfo &fg )
{
	SBound largeBV = MakeLargeHintBound();
	if ( CDynamicCast<CDynamicPart> pDynamic = pTarget )
		return CreateGeometry( pDecal, pMaterial, pDynamic->GetSimplePosNode(), largeBV, fg );
	if ( CDynamicCast<CAnimatedPart> pAnimated = pTarget )
		return CreateGeometry( pDecal, pMaterial, pAnimated->GetAnimationNode(), pAnimated->GetMMXAnimationNode(), pAnimated->GetBound(), largeBV, fg );
	return 0;
}

CObjectBase* CGScene::CreateDecal( ISomePart *pTarget, const vector<CVec3> &srcPositions, 
	const SDecalMappingInfo &_info, IMaterial *pMaterial )
{
	const SFullGroupInfo &fgInfo = pTarget->GetFullGroupInfo();
	SFullGroupInfo fg( fgInfo.groupInfo, 0, 0 );
	
	if ( fabs2(_info.vNormal ) > 0 )
	{
		float f = _info.fRadius;
		if ( CDynamicCast<CGenericDynamicPart> pGeneralDynamics = pTarget ) // I like name of this company :)
		{
			IMaterial *pExactDecalMat = GetExactDecal( pMaterial );
			if ( !pExactDecalMat )
				return 0;
			CPtr<CPerPolyDecalGeometry> pDecal = new CPerPolyDecalGeometry ( pTarget, srcPositions, _info.vCenter, -_info.vNormal, CVec2(f,f), _info.fRotation, CVec2(0.5f, 0.5f) );
			return CreateDynamicDecal( pTarget, pDecal, pExactDecalMat, fg );
		}
		else
		{
			if ( fgInfo.groupInfo.nObjectGroup & N_MASK_OPAQUE )
			{
				CPtrFuncBase<CObjectInfo> *pGeom;
				SFBTransform transform;
				SFullGroupInfo fgInfo;
				if ( GetGeometryObjectInfo( pTarget, &pGeom, &transform, &fgInfo ) )
				{
					SDiscretePos trans( new CFBTransform( transform ), CVec3(0,0,0), 0 );
					CDecalGeometry *pDecal = new CDecalGeometry( pGeom, trans, _info.vCenter, -_info.vNormal, CVec2(f,f), _info.fRotation, CVec2(0.5f, 0.5f), -0.4f, F_DEPTH_WINDOW );
					return CreateGeometry( pDecal, pMaterial, fg );
				}
			}
			else
			{
				IMaterial *pExactDecalMat = GetExactDecal( pMaterial );
				if ( !pExactDecalMat )
					return 0;
				// alpha tested geometry
				CPtr<CPerPolyDecalGeometry> pDecal = new CPerPolyDecalGeometry ( pTarget, srcPositions, _info.vCenter, _info.vNormal, CVec2(f,f), _info.fRotation, CVec2(0.5f, 0.5f) );
				return CreateStaticDecal( pTarget, pDecal, pExactDecalMat, fg );
			}
		}
	}
	else
	{
		CPtr<CExplosionDecalGeometry> pDecal = new CExplosionDecalGeometry ( pTarget, srcPositions, _info.vCenter, _info.fRadius, _info.fRotation );
		if ( CDynamicCast<CGenericDynamicPart> pGeneralDynamics = pTarget ) // I like name of this company :)
			return CreateDynamicDecal( pTarget, pDecal, pMaterial, fg );
		else
			return CreateStaticDecal( pTarget, pDecal, pMaterial, fg );
	}
	return 0;
}

void CGScene::GetPartsList( const SDecalMappingInfo &_info, const CObjectBaseSet &targets, vector<CPtr<ISomePart> > *pRes )
{
	WalkNotLoadedObjects();

	list<SRenderPartSet> res;
	SGroupSelect gs = MakeSelectAll();
	//gs.nMaskEvery |= N_MASK_OPAQUE;

	WalkOctree();
	if ( fabs2( _info.vNormal ) > 0 )
	{
		CTransformStack ts;
		ts.MakeParallel( 2 * _info.fRadius, 2 * _info.fRadius, -2 * _info.fRadius, 2 * _info.fRadius );
		SHMatrix cam;
		MakeMatrix( &cam, _info.vCenter, _info.vNormal );
		ts.SetCamera( cam );
		MakePartList( &ts, &res, CGScene::RN_ALL, gs );
	}
	else
	{
		CRenderWrapper rw( this );
		GeneratePartList( &rw, _info.vCenter, _info.fRadius, &res, IRender::DT_ALL, gs );
	}

	for ( list<SRenderPartSet>::const_iterator i = res.begin(); i != res.end(); ++i )
	{
		const SRenderPartSet &r = *i;
		for ( int k = 0; k < r.pParts->size(); ++k )
		{
			if ( !r.parts.IsSet( k ) )
				continue;
			CDynamicCast<ISomePart> pPart( (*r.pParts)[k] );
			if ( !pPart )
				continue;
			const SFullGroupInfo &fg = pPart->GetFullGroupInfo();
			CObjectBaseSet::const_iterator i = targets.find( fg.pUser );
			if ( i == targets.end() )
				continue;
			if ( pPart->GetFullGroupInfo().nUserID == -1 )
				continue;
			pRes->push_back( pPart.GetPtr() );
		}
	}
}

CDecalTarget* CGScene::CreateDecalTarget( const vector<CObjectBase*> &targets, const SDecalMappingInfo &_info )
{
	return pDecalsManager->CreateDecalTarget( targets, _info );
}

CObjectBase* CGScene::AddDecal( NGScene::CDecalTarget *pTarget, IMaterial *pMaterial )
{
	return pDecalsManager->CreateDecal( pTarget, pMaterial );
}

static CRTPtr pParticleLM( "ParticleLight" );
void CGScene::RefreshParticleLMTarget()
{
	if ( IsValid( particleLM.pParticleLMs ) )
		return;
	particleLM.pParticleLMs = pParticleLM.GetTexture();
	SHMatrix &m = particleLM.rootTransform;
	NGfx::MakeLMToScreenMatrix( &m, N_DEFAULT_RT_RESOLUTION, N_DEFAULT_RT_RESOLUTION );
	particleLM.vParticleLMSize = CVec2( N_DEFAULT_RT_RESOLUTION, N_DEFAULT_RT_RESOLUTION );
}

template<class T>
inline int GetNotLoadedCount( const T &stuff )
{
	int nCount = 0;
	for ( T::const_iterator i = stuff.begin(); i != stuff.end(); ++i )
	{
		if ( IsValid(*i) && !(*i)->HasLoadedObjectInfo() )
			nCount++;
	}

	return nCount;
}

void CGScene::SetWarFogBlend( float fBlend )
{
	pLightState->SetWarFogBlend( fBlend );
}

void CGScene::SetWarFog( const CArray2D<unsigned char> &fog, float fScale )
{
	pLightState->SetWarFog( fog, fScale );
}

void CGScene::RenderPostProcess( CTransformStack *pTS, NGfx::CRenderContext *pRC )
{
	// render post process
	DrawPostProcess( pTS, pRC, lastMask );

	if ( pRC->HasRegisters() )
		pRC->SetRegister( 0 );
	// draw lines
	pRC->SetTransform( pTS->Get() );
	DrawLines( pRC );
}


static int CountUpdatableParts( CVolumeNode *p )
{
	if ( p == 0 )
		return 0;

	int nRes = p->updatable.animatedParts.size();
	nRes += p->updatable.dynamicFrags.size();
	nRes += p->updatable.movingParts.size();

	for ( int k = 0; k < 8; ++k )
		nRes += CountUpdatableParts( p->GetNode( k ) );

	return nRes;
}

static int GetNotLoadedCountInTree( CVolumeNode *p )
{
	if ( p == 0 )
		return 0;

	int nRes = GetNotLoadedCount( p->updatable.animatedParts );
	nRes += GetNotLoadedCount( p->updatable.dynamicFrags );
	nRes += GetNotLoadedCount( p->updatable.movingParts );

	for ( int k = 0; k < 8; ++k )
		nRes += GetNotLoadedCountInTree( p->GetNode( k ) );

	return nRes;
}
void CGScene::LoadEverything()
{
	WalkNotLoadedObjects();

	if ( bWaitForLoad )
	{
		bool bInitCounter = true;
		while ( bWaitForLoad )
		{
			WalkNotLoadedObjects();
			int nToLoad = toBeLoaded.size();
			nToLoad += GetNotLoadedCountInTree( pVolume );
			nToLoad += GetNotLoadedCount( toBeLoadedAnimated );
			if ( nToLoad != 0 )
			{
				if ( IsValid( pLoadingCounter ) )
				{
					if ( bInitCounter )
					{
						pLoadingCounter->SetTotalCount( nToLoad );
						bInitCounter = false;
					}
					pLoadingCounter->LeftToLoad( nToLoad );
				}

				Sleep( 0 );
			}
			else
				bWaitForLoad = false;
		}
	}
}

inline void GetCameraPosition( CVec3 *pRes, CTransformStack *pTS )
{
	CVec4 vCamPos4;
	pTS->Get().backward.RotateHVector( &vCamPos4, CVec4(0,0,0,1) );
	pRes->Set( vCamPos4.x / vCamPos4.w, vCamPos4.y / vCamPos4.w, vCamPos4.z / vCamPos4.w );
}

void CGScene::Draw( CTransformStack *pTS, CTransformStack *pClipTS, NGfx::CRenderContext *pRC, const SGroupSelect &_mask, 
	ERenderPath renderPath, const SRTClearParams &rtClear, EHSRMode hsrMode, 
	ETransparentMode trMode, NGfx::CCubeTexture *pSky, SDepthOfField *pDOF, int nLightOptions )
{
	if ( nGfxDeviceCreationID != NGfx::GetDeviceCreationID() )
	{
		nGfxDeviceCreationID = NGfx::GetDeviceCreationID();
		if ( pLightState )
			pLightState->Updated();
		bWaitForLoad = true;
	}

	LoadEverything();

	bool bMaskChanged = lastMask != _mask;
	lastMask = _mask;
	SGroupSelect mask(_mask);
	bool bUseFakeParticleLM = renderPath != RP_GF3_FAST;
	if ( !bUseFakeParticleLM )
		RefreshParticleLMTarget();

	// recalc camera pos
	{
		CVec4 ptCenter;
		pTS->Get().backward.RotateHVector( &ptCenter, CVec4(0,0,1,0) );
		pCamera->Set( CVec4( ptCenter ) );
	}
	
	// recalc light state
	pLightState->SetClipTS( *pClipTS );
	CDGPtr<CFuncBase<SPerVertexLightState> > pRefreshLight( pLightState );
	pRefreshLight.Refresh();
	WalkOctree();

	CSceneFragments geom;
	CObj<CTransparentRenderer> pTransp;
	pFakeParticleLM.Refresh();
	if ( bUseFakeParticleLM )
	{
		CTPoint<int> ptFakeRegisterSize( N_FAKE_LM_SIZEX, N_FAKE_LM_SIZEY );
		pTransp = new CTransparentRenderer( *pTS, ptFakeRegisterSize, true, 
			pFakeParticleLM->GetParticleColor(), pFakeParticleLM->GetNormalColor(),
			&pRefreshLight->GetValue() );
	}
	else
		pTransp = new CTransparentRenderer( *pTS, CTPoint<int>(N_DEFAULT_RT_RESOLUTION,N_DEFAULT_RT_RESOLUTION), false,
			pFakeParticleLM->GetParticleColor(), pFakeParticleLM->GetNormalColor(),
			&pRefreshLight->GetValue() );
	CRenderWrapper renderWrapper( this );
	int nUseIgnoreMark = N_SKIP_IGNORED_TEST;
	//if ( hsrMode != HSR_NONE )
	//{
	//	UpdateIgnoreMark( &renderWrapper, pClipTS, mask, hsrMode );
	//	nUseIgnoreMark = nCurrentIgnoreMark;
	//}
	//geom.SetNeedHSR( NeedUseHWHSR() );
	ERLRequest rlReq = RN_ALL;
	switch ( renderPath )
	{
		case RP_TNL:
		case RP_SHOWOVERDRAW:
		case RP_SHOWLIGHTMAP:
			break;
		case RP_GF3_FAST:
			rlReq = (ERLRequest)( ((int)rlReq) | RN_LIT_PARTICLES );	
			break;
	}


	if ( bWireframe )
		NGfx::SetWireframe( NGfx::WIREFRAME_ON );
	else
		NGfx::SetWireframe( NGfx::WIREFRAME_OFF );

	// render all lights
	if ( renderPath != RP_GF3_FAST || trMode == TRM_ONLY )
		ClearRT( pRC, rtClear );
	//ASSERT( pLightState->GetDirectional() );
	if ( pLightState->GetDirectional() )
	{
		++nFrameCounter;
		SSceneFragmentGroupInfo renderList( mask, &geom, pTransp, nUseIgnoreMark != N_SKIP_IGNORED_TEST ? pHZBuffer : 0 );
		MakeRenderList( pClipTS, &renderList, rlReq, nUseIgnoreMark );

		/*if ( renderPath == RP_GF3_FAST )
		{
			pRenderListGF3 = new SSceneDepthFragmentGroupInfo( vCamPos, mask, &geom, &geomFar, pTransp,
				++nFrameCounter, nUseIgnoreMark != N_SKIP_IGNORED_TEST ? pHZBuffer : 0 );
			MakeRenderList( pClipTS, pRenderListGF3, rlReq, nUseIgnoreMark );
		}
		else
		{
			pRenderList = new SSceneFragmentGroupInfo( mask, &geom, pTransp,
				++nFrameCounter, nUseIgnoreMark != N_SKIP_IGNORED_TEST ? pHZBuffer : 0 );
			MakeRenderList( pClipTS, pRenderList, rlReq, nUseIgnoreMark );
		}*/

		// form scene
		if ( pTransp )
		{
			SBound transpBound;
			if ( !pTransp->IsEmpty() )
			{
				pTransp->GetBound( &transpBound );
				geom.AddGeomBound( transpBound );
			}
		}

		SParticleLMRenderTargetInfo useParticleTarget;
		if ( !bUseFakeParticleLM )
			useParticleTarget = particleLM;
		useParticleTarget.vKernelSize = pTransp->GetKernelLightInfo().vLightSize;
		NGfx::CTexture *pParticleLight = 0;
		if ( trMode != TRM_NONE )
			pParticleLight = bUseFakeParticleLM ? pFakeParticleLM->GetValue() : particleLM.pParticleLMs;

		//static bool bTwilight = true;

		

		switch ( renderPath )
		{
		case RP_TNL:
			RenderTnL( pTS, pClipTS, pRC, &renderWrapper, geom, pTransp, trMode, pSky );
			break;
		case RP_GF3_FAST:
			
			if ( bIsTwilight )
			{
				pRC->SetVirtualRT();
				pRC->SetRegister( 0 );
			}

			RenderGf3Fast( pTS, pClipTS, pRC, &renderWrapper, geom, useParticleTarget, rtClear,
				pLightState->GetDirectional(), pLightState->GetValue().GetWarFogBlend(), nLightOptions,
				pTransp, trMode, pParticleLight, pSky );
			break;
		case RP_SHOWOVERDRAW:
			RenderOverdraw( pTS, pRC, &renderWrapper, geom );
			RenderPostProcess( pTS, pRC );
			if ( trMode != TRM_NONE )
			{
				STransparentRenderContext trc( NGfx::IsTnLDevice(), pRC, 0, 0, renderPath, 0, SLightInfo() );
				pTransp->Render( trc );
			}
			break;
		case RP_SHOWLIGHTMAP:
			RenderShowLightmap( pTS, pRC, &renderWrapper, geom );
			RenderPostProcess( pTS, pRC );
			break;
		case RP_SHOWPOLYCOUNT:
			{
				CSceneFragments polyGeom;
				MakePolycountRenderList( pTS, &polyGeom );
				RenderPolycount( pTS, pRC, &renderWrapper, polyGeom );
			}
			break;
		default:
			ASSERT(0);
			break;
		}

		// draw twilight
		if ( bIsTwilight )
		{
			NGfx::CRenderContext rc;

			CTRect<float> rectReg;
			NGfx::GetRegisterSize( &rectReg );
			const int nWidth = rectReg.Width();
			const int nHeight = rectReg.Height();
			CTRect<float> rectRegDS( 0, 0, nWidth, nHeight );
			CTRect<float> rectSmall( 0, 0, 255, 255 );

			vector<CPtr<NGfx::I2DEffect> > filters;
			NGfx::InitShaderFX();

			static CObj<NGfx::CTexture> pRandomTexture;
			if ( !IsValid( pRandomTexture ) )
			{
				pRandomTexture = NGfx::MakeTexture( 128, 128, 1, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::WRAP );
				NGfx::CTextureLock<NGfx::SPixel8888> lock( pRandomTexture, 0, NGfx::INPLACE );
				for ( int x = 0; x < 128; ++x )
				{
					for ( int y = 0; y < 128; ++y )
					{
						BYTE v1 = (rand()&127);
						lock[y][x] = NGfx::SPixel8888( v1+10, v1+50, v1+100, 0);
					}
				}
			}
			
			filters.push_back(new  NGfx::CTwilightEffect( 0.8f, 0.02f, pRandomTexture, 0, 0 ));
			CTRect<float> rectSrc( rectReg ), rectDst( rectRegDS );
			NGfx::CopyTexture( rc, CVec2( rectReg.Width(), rectReg.Height() ), rectDst, NGfx::GetRegisterTexture( 0 ), rectSrc, CVec4( 0, 0, 0, 0.0f ), filters[0] );
		}
		//CPtr< SDepthOfField > ef=new SDepthOfField(0.1f,1.0f);
		//ProcessDepthOfField( ef , &geom, pTS, &renderWrapper);
		//ProcessDepthOfField( pDOF, &geom, pTS, &renderWrapper );
	}

	NGfx::SetWireframe( NGfx::WIREFRAME_OFF );

	//// render sun flares
	if ( ( trMode != TRM_ONLY ) && IsValid( pSunFlares ) && IsValid( pLightState->GetDirectional() ) )
		DrawSunFlares( pTS, pRC );
	////
	if ( renderPath == RP_SHOWOVERDRAW )
		ColorOverdraw( pRC );
	//
	RecalcRenderStats( geom.GetSceneTris(), pTransp->GetTotalParticles(), pTransp->GetLitParticles() );
	pRC->SetStencil( NGfx::STENCIL_NONE );
	if ( bShow2DTextureCache )
		NGfx::ShowTexture( pRC, NGfx::GetTextureCache(), CVec2(1600,1200) );
	if ( bShowTranspTextureCache )
		NGfx::ShowTexture( pRC, NGfx::GetTransparentTextureCache(), CVec2(1600,1200) );
	if ( bShowParticleLMCache )
	{
		if ( bUseFakeParticleLM )
			;//NGfx::ShowTexture( pRC, pFakeParticleLM->GetValue(), CVec2(10, 10) );
		else
			NGfx::ShowTexture( pRC, particleLM.pParticleLMs, CVec2(800,600) );
	}
	switch ( showLinearCache )
	{
		case SLC_NONE: break;
		case SLC_DYNAMIC: NGfx::ShowTexture( pRC, NGfx::GetLinearBufferMRU( NGfx::DYNAMIC ), CVec2(1024, 768) ); break;
		case SLC_STATIC:  NGfx::ShowTexture( pRC, NGfx::GetLinearBufferMRU( NGfx::STATIC ), CVec2(1024, 768) ); break;
	}
}

template<class T>
static void AddNotLoaded( vector<IPart*> *pRes, const T &data )
{
	for ( T::const_iterator i = data.begin(); i != data.end(); ++i )
	{
		ISomePart *pPart = *i;
		//ASSERT( IsValid(pPart) );
		if ( !IsValid(pPart) )
			continue;
		if ( !pPart->GetCombiner() )
			pRes->push_back( pPart );
	}
}

static void AddNotLoadedInTree( vector<IPart*> *pRes, CVolumeNode *p )
{
	if ( p == 0 )
		return;

	AddNotLoaded( pRes, p->updatable.animatedParts );
	AddNotLoaded( pRes, p->updatable.dynamicFrags );
	AddNotLoaded( pRes, p->updatable.movingParts );

	for ( int k = 0; k < 8; ++k )
		AddNotLoadedInTree( pRes, p->GetNode( k ) );
}

void CGScene::GetNotLoaded( vector<IPart*> *pRes )
{
	pRes->resize( 0 );
	AddNotLoaded( pRes, toBeLoaded );
	AddNotLoadedInTree( pRes, pVolume );
	AddNotLoaded( pRes, toBeLoadedAnimated );
}

static void CollectAllParts( vector<CObjectBase*> *pRes, CCombinedPart *p )
{
	CDGPtr<CPerMaterialCombiner> pComb = p->GetCombiner();
	pComb.Refresh();
	const vector< CPtr<IPart> > &v = pComb->GetValue();
	for ( int i = 0; i < v.size(); ++i )
		pRes->push_back( v[i] );
}
static void CollectAllParts( vector<CObjectBase*> *pRes, CVolumeNode *p )
{
	if ( p == 0 )
		return;
	
	for ( list<CPtr<CCombinedPart> >::const_iterator i = p->staticParts.elements.begin(); i != p->staticParts.elements.end(); ++i )
		CollectAllParts( pRes, (*i) );
	for ( list<CPtr<CCombinedPart> >::const_iterator i = p->dynamicParts.elements.begin(); i != p->dynamicParts.elements.end(); ++i )
		CollectAllParts( pRes, (*i) );

	for ( int k = 0; k < 8; ++k )
		CollectAllParts( pRes, p->GetNode( k ) );
}
void CGScene::CollectAllParts( vector<CObjectBase*> *pRes )
{
	pRes->resize( 0 );
	NGScene::CollectAllParts( pRes, pVolume );
}

CTransparentRenderer *CGScene::CreateTransparentRenderer( CTransformStack *pTS, bool bLitParticles )
{
	CTransparentRenderer * pTransp = NULL;
	CDGPtr<CFuncBase<SPerVertexLightState> > pRefreshLight( pLightState );
	pRefreshLight.Refresh();
	if ( !bLitParticles )
	{
		CTPoint<int> ptFakeRegisterSize( N_FAKE_LM_SIZEX, N_FAKE_LM_SIZEY );
		pTransp = new CTransparentRenderer( *pTS, ptFakeRegisterSize, true, 
			pFakeParticleLM->GetParticleColor(), pFakeParticleLM->GetNormalColor(),
			&pRefreshLight->GetValue() );
	}
	else
	{
		pTransp = new CTransparentRenderer( *pTS, CTPoint<int>(N_DEFAULT_RT_RESOLUTION,N_DEFAULT_RT_RESOLUTION), false,
			pFakeParticleLM->GetParticleColor(), pFakeParticleLM->GetNormalColor(),
			&pRefreshLight->GetValue() );
	}

	return pTransp;
}

CVec2 CGScene::GetScreenRect()
{
	return NGfx::GetScreenRect();
}

CFuncBase<CVec4>* CGScene::GetCamera()
{
	return pCamera;
}

CFuncBase<SPerVertexLightState> *CGScene::GetLightState() const
{
	return pLightState;
}

// CRenderWrapper

void CRenderWrapper::FormPartList( CTransformStack *pTS, list<SRenderPartSet> *pRes, EDepthType dt, const SGroupSelect &mask )
{
	switch ( dt )
	{
	case DT_STATIC:
		pScene->MakePartList( pTS, pRes, CGScene::RN_STATIC, mask );
		break;
	case DT_DYNAMIC:
		pScene->MakePartList( pTS, pRes, CGScene::RN_DYNAMIC, mask );
		break;
	case DT_ALL:
		pScene->MakePartList( pTS, pRes, CGScene::RN_ALL, mask );
		break;
	default:
		ASSERT( 0 );
	}
}

static CTransparentRenderer* CreateFakeTranspRender( const CVec3 &vLightDir )
{
	CTPoint<int> ptFakeRegisterSize( N_FAKE_LM_SIZEX, N_FAKE_LM_SIZEY );
	CTransformStack particleTS;
	particleTS.MakeParallel( 1, 1,-1000, 1000 );
	SHMatrix particleCam;
	MakeMatrix( &particleCam, CVec3(0,0,0), vLightDir );
	particleTS.SetCamera( particleCam );
	return new CTransparentRenderer( particleTS, ptFakeRegisterSize, true, 0, 0, 0 );
}

void CRenderWrapper::FormDepthList( CTransformStack *pTS, const CVec3 &vDir, IRender::EDepthType dt, CSceneFragments *pRes )
{
	CObj<CTransparentRenderer> pTransp( CreateFakeTranspRender( vDir ) );
	SGroupSelect mask( N_MASK_CAST_SHADOW, 0 );

	//CPtr<CGScene::SSceneFragmentGroupInfo> pTarget = new CGScene::SSceneFragmentGroupInfo( mask, pRes, pTransp );
	CGScene::SSceneFragmentGroupInfo target( mask, pRes, pTransp );
	switch ( dt )
	{
		case DT_STATIC:
			pScene->MakeRenderList( pTS, &target, (CGScene::ERLRequest)(CGScene::RN_STATIC | CGScene::RN_DEPTH), N_SKIP_IGNORED_TEST );
			break;
		case DT_DYNAMIC:
			pScene->MakeRenderList( pTS, &target, (CGScene::ERLRequest)(CGScene::RN_DYNAMIC | CGScene::RN_DEPTH), N_SKIP_IGNORED_TEST );
			break;
		case DT_ALL:
			pScene->MakeRenderList( pTS, &target, (CGScene::ERLRequest)(CGScene::RN_ALL | CGScene::RN_DEPTH), N_SKIP_IGNORED_TEST );
			break;
		default:
			ASSERT( 0 );
	}
}

void CRenderWrapper::FormRenderList( CTransformStack *pTS, CSceneFragments *pRes, CTransparentRenderer *pTransparentRender )
{
	SGroupSelect mask( 255, 0 );
	CGScene::SSceneFragmentGroupInfo renderList( mask, pRes, pTransparentRender );
	pScene->MakeRenderList( pTS, &renderList, (CGScene::ERLRequest)(CGScene::RN_ALL /*| CGScene::RN_DEPTH*/), N_SKIP_IGNORED_TEST );
}

CTransparentRenderer *CRenderWrapper::CreateTransparentRenderer( CTransformStack *pTS, bool bLitParticles )
{
	return pScene->CreateTransparentRenderer( pTS, bLitParticles );
}

IGScene* CreateScene()
{
	return new CGScene(0);
}

bool Is3DActive()
{
	return NGfx::Is3DActive();
}

void ClearScreen( const CVec3 &vColor )
{
	NGfx::CRenderContext rc;
	NGfx::SPixel8888 clearColor( Float2Int( vColor.x * 255 ), Float2Int( vColor.y * 255 ), Float2Int( vColor.z * 255 ), 0 );
	rc.ClearBuffers( clearColor.dwColor );
}

void CopyRegisterOnScreen( const CTRect<float> &rScreenRect, ERegisterCopyMode mode, int nRegister )
{
	//nRegister = 4;
	NGfx::CRenderContext rcScreen;
	CTRect<float> regSize;
	NGfx::GetRegisterSize( &regSize );

	CObj<NGfx::I2DEffect> pEffect;
	switch ( mode )
	{
		case RCM_COPY:
			rcScreen.SetAlphaCombine( NGfx::COMBINE_NONE );
			break;
		case RCM_TRANSPARENT:
			rcScreen.SetAlphaCombine( NGfx::COMBINE_ALPHA );
			break;
		case RCM_SHOWALPHA:
			pEffect = new NGfx::CShowAlphaEffect;
			rcScreen.SetAlphaCombine( NGfx::COMBINE_NONE );
			break;
	}
	rcScreen.SetStencil( NGfx::STENCIL_NONE );
	NGfx::CopyTexture( rcScreen, NGfx::GetScreenRect(), rScreenRect, NGfx::GetRegisterTexture(nRegister), regSize, CVec4(1,1,1,1), pEffect );
}

void Flip()
{
	NGfx::Flip();

	lastFrameStats.nSceneTris = 0;
	lastFrameStats.nParticles = 0;
	lastFrameStats.nLitParticles = 0;
	lastFrameStats.nScenePointLights = 0;
	static NHPTimer::STime timeFrameStart = 0;
	lastFrameStats.fFrameTime = NHPTimer::GetTimePassed( &timeFrameStart );

	//char szBuf[ 1024 ];
	//sprintf( szBuf, "%g parts per element, %d elements\n", ((float)nTotalParts) / nTotalElements, nTotalElements );
	//OutputDebugString( szBuf );
	nTotalParts = 0;
	nTotalElements = 0;
}

int CalcTouchedTextureSize()
{
	return NGfx::CalcTouchedTextureSize();
}

void SetWireframe( bool bWire )
{
	bWireframe = bWire;
}
bool GetWireframe()
{
	return bWireframe;
}

void GetRenderStats( SRenderStats *pStats )
{
	lastFrameStats.nTris = NGfx::renderStats.nTris;
	lastFrameStats.nVertices = NGfx::renderStats.nVertices;
	lastFrameStats.nDIPs = NGfx::renderStats.nDIPs;
	lastFrameStats.bStaticGeometryThrashing = NGfx::IsStaticGeometryThrashing();
	lastFrameStats.b2DTexturesThrashing = NGfx::Is2DTextureThrashing();
	lastFrameStats.bTransparentThrashing = NGfx::IsTransparentThrashing();
	lastFrameStats.bDynamicGeometryTrashing = NGfx::IsDynamicGeometryThrashing();
	*pStats = lastFrameStats;
}

float GetFrameTime()
{
	return lastFrameStats.fFrameTime;
}

CPtrFuncBase<CParticleEffect> *GetParticleAnimator( CObjectBase *_p )
{
	if ( CDynamicCast<CParticles> p = _p )
		return p->GetAnimator();
	return 0;
}

CLightmapsHolder *CalcLightmaps( IGScene *_pScene, CObjectBase *pUser, int nUserID, const SSphere &highResLM, ELightmapQuality quality, CLightmapsTempHolder *pTmpHolder )
{
	CDynamicCast<CGScene> pScene = _pScene;
	CRenderWrapper rw( pScene );
	return CalcLightmaps( _pScene, &rw, pUser, nUserID, highResLM, quality, pTmpHolder  );

}

void ApplyLightmaps( IGScene *_pScene, CObjectBase *pUser, CLightmapsHolder *pLightmaps,  CLightmapsLoader * pLD  )
{
	CDynamicCast<CGScene> pScene = _pScene;
	CRenderWrapper rw( pScene );
	ApplyLightmaps( _pScene, &rw, pUser, pLightmaps, pLD );
}

// Commands/Vars

static void VarSwitchTexCache( const string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	bShow2DTextureCache = false;
	if ( sValue.GetFloat() != 0 )
		bShow2DTextureCache = true;
}

static void VarSwitchTranspCache( const string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	bShowTranspTextureCache = false;
	if ( sValue.GetFloat() != 0 )
		bShowTranspTextureCache = true;
}

static void VarSwitchTranspLMCache( const string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	bShowParticleLMCache = false;
	if ( sValue.GetFloat() != 0 )
		bShowParticleLMCache = true;
}

static void VarSwitchTwilight( const string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	bTwilight = false;
	if ( sValue.GetFloat() != 0 )
		bTwilight = true;
}

static void VarSwitchLinearCache( const string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	showLinearCache = SLC_NONE;
	if ( sValue.GetFloat() != 0 )
	{
		if ( sValue.GetFloat() != 1 )
			showLinearCache = SLC_STATIC;
		else
			showLinearCache = SLC_DYNAMIC;
	}
}

START_REGISTER(GSceneInternal)
    REGISTER_VAR( "gfx_showcache_2d", VarSwitchTexCache, 0.0f, STORAGE_NONE )
	REGISTER_VAR( "gfx_showcache_transp", VarSwitchTranspCache, 0.0f, STORAGE_NONE )
	REGISTER_VAR( "gfx_showcache_transplm", VarSwitchTranspLMCache, 0.0f, STORAGE_NONE )
	REGISTER_VAR( "gfx_showcache_linear", VarSwitchLinearCache, 0.0f, STORAGE_NONE )
	REGISTER_VAR( "gfx_twilight", VarSwitchTwilight, 0.0f, STORAGE_NONE )
	REGISTER_VAR_EX( "gfx_pc_low", NGlobal::VarFloatHandler, &fPCLow, 0.15f, STORAGE_NONE )
	REGISTER_VAR_EX( "gfx_pc_high", NGlobal::VarFloatHandler, &fPCHigh, 5.0f, STORAGE_NONE )
	REGISTER_VAR_EX( "gfx_lod_switch_distance", NGlobal::VarFloatHandler, &s_fLODSwitchDistance, 300, STORAGE_USER )
FINISH_REGISTER

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x0251100b, CGScene )
REGISTER_SAVELOAD_CLASS( 0x03111000, CPolyline )
REGISTER_SAVELOAD_CLASS( 0x00661130, CVolumeNode )
REGISTER_SAVELOAD_CLASS( 0x11791170, CCombinedPart )
REGISTER_SAVELOAD_CLASS( 0x27041130, CParticles )
REGISTER_SAVELOAD_CLASS( 0x00372110, CGetTranspCache )
REGISTER_SAVELOAD_CLASS( 0x00372140, CFakeParticleLMTexture )
REGISTER_SAVELOAD_CLASS( 0x020a2190, CPostProcessBinder )
REGISTER_SAVELOAD_CLASS( 0x01123131, CAmbientAnimator )
REGISTER_SAVELOAD_CLASS( 0xB4409180, CSunFlares )

