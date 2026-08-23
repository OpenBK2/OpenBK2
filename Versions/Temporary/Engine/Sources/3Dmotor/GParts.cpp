#include "stdafx.h"
#include "GParts.h"
#include "GPartParticles.h"
namespace NGScene
{
extern bool	bFreeze;
extern bool bNewShadows;

static void AttachPart( ISomePart *pRes, CVolumeNode *pNode, 
	SFullStaticTrackers *pTrackers, bool bIsDynamic )
{
	///bool bIsLightmapped = !bIsDynamic && pRes->GetGroupInfo().nLightGroup == 0 && pRes->GetMaterial()->GetType() == IMaterial::MT_NORMAL;//!= IMaterial::MT_OCCLUDER;
	CCombinedPart *pCPart;
	int nFloorMask = pRes->GetGroupInfo().nObjectGroup & N_MASK_FLOORS;
	NGScene::EDynamicType eDT = pRes->GetMaterial()->GetDynamicType();

	if( bNewShadows )
	{
		if( eDT == DT_FORCE_DYNAMIC ) {  bIsDynamic = true; }
		if( eDT == DT_FORCE_STATIC ) { bIsDynamic = false; }
	}
	
	if ( bIsDynamic )
		pCPart = pNode->dynamicParts.GetCombinerPartForAdd( pRes->GetMaterial(), pTrackers, nFloorMask, true );
	else
		pCPart = pNode->staticParts.GetCombinerPartForAdd( pRes->GetMaterial(), pTrackers, nFloorMask, false );
	pRes->pOwner = pCPart;
	pRes->SetCombiner( pCPart->GetCombiner() );
	//if ( bAnimationOnly )
	//	pRes->SetCombiner( pCPart->GetCombiner(), false, true );
	//else
	//	pRes->SetCombiner( pCPart->GetCombiner(), true, false );
}

void PlaceToOctree( ISomePart *pPart, CVolumeNode *pRoot, const CVec3 &vPos, float fR, 
	SFullStaticTrackers *pTrackers, bool bIsDynamic )
{
	CVolumeNode *pNode = pRoot->GetNode( vPos, fR );
	ASSERT( pNode );
	pPart->RefreshObjectInfo();
	AttachPart( pPart, pNode, pTrackers, bIsDynamic );
}

CVolumeNode *GetUpdatable( CVolumeNode *pRoot, const SBound &hintBV )
{
	CVolumeNode *pNode = pRoot->GetNode( hintBV.s.ptCenter, hintBV.s.fRadius );
	return pNode;
}

ISomePart::ISomePart( CPtrFuncBase<CObjectInfo> *pData, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo ) 
: IPart( pData, 0 ), group( _pMaterial, _gInfo )
{
	SetCacheLightFlags();
}

void ISomePart::SetCacheLightFlags()
{
	const SFullGroupInfo &gInfo = group.fullGroupInfo;
	if ( gInfo.groupInfo.nLightFlags & LF_USE_DIRECTIONAL_APPROX )
		cacheLighting.bReplaceWithDirectional = true;
	if ( gInfo.groupInfo.nLightFlags & LF_DO_NOT_CACHE_LIGHT )
		cacheLighting.bDoNotCacheLighting = true;
	if ( gInfo.groupInfo.nLightFlags & LF_SKIP_LIGHTING )
		cacheLighting.bSkipLighting = true;
	if ( gInfo.groupInfo.nLightFlags & LF_DO_NOT_MULTIPLY_ON_TRANSPARENCY )
		cacheLighting.bMultiplyOnTransparency = false;
	if ( group.pMaterial->IsSelfIllum() )
	{
		cacheLighting.bSkipLighting = true;
		cacheLighting.bSelfIllum = true;
	}
	cacheLighting.bSkipStaticPointLights = vars.pLM != 0;
	if ( group.pMaterial )
		cacheLighting.vTranslucentColor = group.pMaterial->GetTranslucentColor();
	else
		cacheLighting.vTranslucentColor = CVec3(0,0,0);
	cacheLighting.bTranslucent = fabs2( cacheLighting.vTranslucentColor ) > 0;
}

void ISomePart::SetVars( const SPerPartVariables &_vars )
{
	vars = _vars;
	if ( pOwner )
		pOwner->PartHasChanged();
}

void ISomePart::SetFade( float _fFade )
{
	SPerPartVariables v = vars;
	if ( _fFade < 1.0f / 256 )
		v.fFade = 0;
	else if ( _fFade > 255.0f / 256 )
		v.fFade = 1;
	else
		v.fFade = _fFade;
	SetVars( v );
}

void ISomePart::SetPriority( int _n )
{
	SPerPartVariables v = vars;
	ASSERT( _n >= 0 && _n < 255 );
	v.nPriority = _n;
	SetVars( v );
}

void ISomePart::SetLM( CPtrFuncBase<NGfx::CTexture> *pLM )
{
	SPerPartVariables v = vars;
	v.pLM = pLM;
	SetVars( v );
	SetCacheLightFlags();
}

// CStaticAnimatedPart

CStaticAnimatedPart::CStaticAnimatedPart( CPtrFuncBase<CObjectInfo> *pData, CFuncBase< std::vector<SHMatrix> > *pAnim,
	CFuncBase<std::vector<NGfx::SCompactTransformer> > *pMMXAnim,
	const SBound &_bv, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo ) 
	: ISomePart( pData, _pMaterial, _gInfo ), pAnimation(pAnim), pMMXAnimation(pMMXAnim), bv(_bv)
{
}

// CGenericDynamicPart

CGenericDynamicPart::CGenericDynamicPart( CPtrFuncBase<CObjectInfo> *pData, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo )
: ISomePart( pData, _pMaterial, _gInfo ), nStillCounter(0), bt(BT_NONE)
{
}

// CDynamicPart

CDynamicPart::CDynamicPart( CPtrFuncBase<CObjectInfo> *pData, CFuncBase<SFBTransform> *pPos, 
													 IMaterial *_pMaterial, const SFullGroupInfo &_gInfo )
													 : CGenericDynamicPart( pData, _pMaterial, _gInfo ), pTransform(pPos), pTrackObjInfo(pData)
{
	bound.SphereInit( CVec3(0,0,0), 1e20f );
	//	RefreshObjectInfo();
	//	GetObjectInfo()->CalcBound( &bound );
}

bool CDynamicPart::Update( CVolumeNode *pVolume, SFullStaticTrackers *pTrackers )
{
	if ( !HasLoadedObjectInfo() )
	{
		SetCombiner( 0 );
		return true;
	}
	if ( bound.s.fRadius == 1e20f )
		GetObjectInfo()->CalcBound( &bound );

	if ( pTransform.Refresh() )
	{
		nStillCounter = 0;
		CVolumeNode *pNode = pVolume->SelectNode( pTransform, bound );
		ASSERT( pNode );
		AttachPart( this, pNode, pTrackers, true );
	}
	else
	{
		if( !bFreeze )
		++nStillCounter;
		if ( nStillCounter == 3 && (GetGroupInfo().nLightFlags & LF_NEVER_STATIC) == 0 )
		{
			CVolumeNode *pNode = pVolume->SelectNode( pTransform, bound );
			ASSERT( pNode );
			AttachPart( this, pNode, pTrackers, false );
		}
	}
	return true;
}

// CDynamicPartWithAnimatedBound

CDynamicPartWithAnimatedBound::CDynamicPartWithAnimatedBound( CPtrFuncBase<CObjectInfo> *pData, CFuncBase<SFBTransform> *pPos, CFuncBase<SBound> *_pBound,
													 IMaterial *_pMaterial, const SFullGroupInfo &_gInfo )
													 : CAnimatedBoundPart( pData, _pMaterial, _gInfo, _pBound, pPos ), pTransform(pPos)
{
}

// CAnimatedPart

CAnimatedPart::CAnimatedPart( CPtrFuncBase<CObjectInfo> *pData, CFuncBase< std::vector<SHMatrix> > *_pAnim,
	CFuncBase<std::vector<NGfx::SCompactTransformer> > *_pMMXAnim,
	CFuncBase<SBound> *_pBound, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo )
	: CAnimatedBoundPart( pData, _pMaterial, _gInfo, _pBound, _pAnim), 
	pAnimation(_pAnim), pMMXAnimation(_pMMXAnim)
{
}

const float F_MAX_DISTANCE_TO_BONE = 0.5f;
bool CAnimatedBoundPart::Update( CVolumeNode *pVolume, SFullStaticTrackers *pTrackers )
{
	if ( !HasLoadedObjectInfo() )
	{
		SetCombiner( 0 );
		return true;
	}

	EDGNodeChange dg = pChanges.GetChanges();

	EBindType needBT = BT_DYNAMIC;
	switch ( dg )
	{
	case DG_CHANGE_UNKNOWN:
		if ( bt == BT_STATIC )
			needBT = bt;
		break;
	case DG_CHANGE_NONE:
		if( !bFreeze )
		++nStillCounter;
		if ( nStillCounter > 3 && (GetGroupInfo().nLightFlags & LF_NEVER_STATIC) == 0 )
			needBT = BT_STATIC;
		break;
	case DG_CHANGE_CHANGED:
		nStillCounter = 0;
		needBT = BT_DYNAMIC;
		break;
	default:
		ASSERT(0);
		break;
	}

	if ( pBound.Refresh() )
		bt = BT_NONE;

	if ( bt == needBT )
		return true;

	ASSERT( needBT != BT_NONE );
	bt = needBT;
	const SBound &bv = pBound->GetValue();
	CVolumeNode *pNode = pVolume->GetNode( bv.s.ptCenter, bv.s.fRadius );
	ASSERT( pNode );
	if ( bt == BT_DYNAMIC )
		AttachPart( this, pNode, pTrackers, true );
	else
	{
		ASSERT( bt == BT_STATIC );
		AttachPart( this, pNode, pTrackers, false );
	}
	return true;
}

// CDynamicGeometryPart

CDynamicGeometryPart::CDynamicGeometryPart( CPtrFuncBase<CObjectInfo> *pData, CFuncBase<SFBTransform> *_pTransform, 
	CFuncBase<SBound> *_pBound, IMaterial *_pMaterial, const SFullGroupInfo &_gInfo )
	: CGenericDynamicPart( pData, _pMaterial, _gInfo ), pBound(_pBound), pTransform(_pTransform), pChanges(pData)
{
}

void CDynamicGeometryPart::AddChangeTrackers( CAnimationWatch *p, bool bVertices ) 
{
	p->AddHandle( GetObjectInfoNode() ); 
	if ( pTransform ) 
		p->AddHandle( pTransform ); 
}

bool CDynamicGeometryPart::Update( CVolumeNode *pVolume, SFullStaticTrackers *pTrackers )
{
	EDGNodeChange dg = pChanges.GetChanges();
	if ( pTransform && pTransform.Refresh() )
		dg = DG_CHANGE_CHANGED;

	EBindType needBT = BT_DYNAMIC;
	switch ( dg )
	{
	case DG_CHANGE_UNKNOWN:
		if ( bt == BT_STATIC )
			needBT = bt;
		break;
	case DG_CHANGE_NONE:
		if( !bFreeze )
		++nStillCounter;
		if ( nStillCounter > 3 && (GetGroupInfo().nLightFlags & LF_NEVER_STATIC) == 0 )
			needBT = BT_STATIC;
		break;
	case DG_CHANGE_CHANGED:
		nStillCounter = 0;
		needBT = BT_DYNAMIC;
		break;
	default:
		ASSERT(0);
		break;
	}

	if ( pBound.Refresh() )
		bt = BT_NONE;

	if ( bt == needBT )
		return true;

	ASSERT( needBT != BT_NONE );
	bt = needBT;
	const SBound &bv = pBound->GetValue();
	CVolumeNode *pNode = pVolume->GetNode( bv.s.ptCenter, bv.s.fRadius );
	ASSERT( pNode );
	if ( bt == BT_DYNAMIC )
		AttachPart( this, pNode, pTrackers, true );
	else
	{
		ASSERT( bt == BT_STATIC );
		AttachPart( this, pNode, pTrackers, false );
	}

	return true;
}

// CCombinedPart

CCombinedPart::CCombinedPart( SFullStaticTrackers *pTrackers, int _nFloorMask, bool _bIsDynamic )
	: nIgnoreMark(0), pCombiner( new CPerMaterialCombiner( 0 ) ), nFloorMask(_nFloorMask),
	pPervertexLightState( pTrackers->pLightState ), bRecalcPartInfo(false), bIsDynamic(_bIsDynamic)
{
}

void CCombinedPart::InitGeometry()
{
	SRenderGeometryInfo &gi = geometryInfo;
	gi.pVertices = new CVBCombiner( 
		pCombiner, 
		bIsDynamic ? CT_DYNAMIC : CT_STATIC,
		pCombiner->GetAnimationTracker(), pPervertexLightState );
	gi.pTriLists[TLT_POSITION] = new CIBCombiner( pCombiner, pCombiner->GetFullChangeTracker(), IBTT_POSITIONS );
	gi.pTriLists[TLT_GEOM] = new CIBCombiner( pCombiner, pCombiner->GetFullChangeTracker(), IBTT_VERTICES );
}

SRenderGeometryInfo* CCombinedPart::GetGeometryInfo() 
{
	if ( !geometryInfo.pVertices )
		InitGeometry();
	return &geometryInfo;
}

void CCombinedPart::UpdatePartInfo()
{
	if ( !pCombiner.Refresh() && !bRecalcPartInfo )
		return;
	bRecalcPartInfo = false;
	CDGPtr<CFuncBase<std::vector< CPtr<IPart> > > > pC( GetCombiner() );
	pC.Refresh();
	const std::vector< CPtr<IPart> > &src = pC->GetValue();
	parts.resize( src.size() );
	materials.resize( 0 );
	for ( int k = 0; k < src.size(); ++k )
	{
		CDynamicCast<ISomePart> p( src[k] );
		SMaterialInfo mi;
		mi.pMaterial = p->GetMaterial();
		mi.vars = p->GetVars();
		int nMat = -1;
		for ( int i = 0; i < materials.size(); ++i )
		{
			if ( materials[i] == mi )
			{
				nMat = i;
				break;
			}
		}
		if ( nMat < 0 )
		{
			nMat = materials.size();
			materials.push_back( mi );
		}
		// find material
		parts[k].groupInfo = p->GetGroupInfo();
		parts[k].nMaterial = nMat;
	}
}

void CCombinedPart::SetIgnored( int _nIgnoreMark, const CPartFlags &_parts )
{ 
	//	pCombiner.Refresh();
	nIgnoreMark = _nIgnoreMark;
	ignoredParts = _parts;
}

// CVolumeNode

bool CVolumeNode::SUpdatableObjects::IsEmpty()
{
	EraseInvalidRefs( &dynamicFrags );
	EraseInvalidRefs( &animatedParts );
	EraseInvalidRefs( &movingParts );
	EraseInvalidRefs( &particles );
	return dynamicFrags.empty() && animatedParts.empty() && movingParts.empty() && particles.empty();
}

template <class TElem, class TParam, class TParam2>
inline void UpdateVector( std::vector<TElem> *a, TParam *p1, TParam2 *p2 )
{
	int nSize = a->size();
	for ( int k = 0; k < nSize; )
	{
		const TElem &ptr = (*a)[k];
		if ( IsValid( ptr ) && ptr->Update( p1, p2 ) )
			++k;
		else
		{
			(*a)[k] = (*a)[ nSize - 1 ];
			--nSize;
		}
	}
	a->resize( nSize );
}
template <class TElem, class TParam>
inline void UpdateVector( std::vector<TElem> *a, TParam *p )
{
	int nSize = a->size();
	for ( int k = 0; k < nSize; )
	{
		const TElem &ptr = (*a)[k];
		if ( IsValid( ptr ) && ptr->Update( p ) )
			++k;
		else
		{
			(*a)[k] = (*a)[ nSize - 1 ];
			--nSize;
		}
	}
	a->resize( nSize );
}

void CVolumeNode::SUpdatableObjects::Update( int nFrame, CVolumeNode *pVolume, SFullStaticTrackers *pTrackers )
{
	if ( nFrame == nLastFrame )
		return;
	nLastFrame = nFrame;
	UpdateVector( &animatedParts, pVolume, pTrackers );
	UpdateVector( &movingParts, pVolume, pTrackers );
	UpdateVector( &dynamicFrags, pVolume, pTrackers );
	UpdateVector( &particles, pVolume );
}

bool CVolumeNode::IsEmpty()
{
	EraseInvalidRefs( &particles );
	return staticParts.IsEmpty() & dynamicParts.IsEmpty() & particles.empty() & updatable.IsEmpty();
}


void SetPartFade( CObjectBase *_p, float fFade )
{
	if ( CDynamicCast<ISomePart> p = _p )
		p->SetFade( fFade );
	else if ( CDynamicCast<IParticles> p = _p )
		p->SetFade( fFade );
}

void SetPartPriority( CObjectBase *_p, int _nPriority )
{
	if ( CDynamicCast<ISomePart> p = _p )
		p->SetPriority( _nPriority );
}

void SetPartLM( CObjectBase *_p, CPtrFuncBase<NGfx::CTexture> *pLM )//CFuncBase<CArray2D<NGfx::SPixel8888> > *pLM )
{
	if ( CDynamicCast<ISomePart> p = _p )
		p->SetLM( pLM );
}

CObjectBase *GetPartGeometry( CObjectBase *_pA )
{
	if ( !_pA )
		return nullptr;
	CDynamicCast<IPart> pA(_pA);
	if ( !pA )
		return nullptr;
	return pA->GetObjectInfoNode();
}
}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02662160, CNonePart )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02662161, CSimplePart )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02662163, CDynamicPart )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02662164, CAnimatedPart )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02662165, CDynamicGeometryPart )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x3422C381, CDynamicPartWithAnimatedBound )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02662166, CStaticAnimatedPart )

