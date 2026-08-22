#include "stdafx.h"

#include "3Dmotor/DBScene.h"
#include "AttachedObj.h"
#include "VisObjDesc.h"
#include "3Dmotor/aiVisitor.h"
#include "3Dmotor/GScene.h"
#include "3DLib/Transform.h"
#include "Main/GameTimer.h"
#include "System/Commands.h"
#include "VisObjIconsManager.h"

#include <cstdint>

static bool s_bGeomAABB = false;
std::vector<int> SModelVisObjDesc::n2Attach;


class CAnimObjBounder : public CFuncBase<SBound>
{
	OBJECT_NOCOPY_METHODS(CAnimObjBounder);
	ZDATA
	SBound modelBound;
	CDGPtr<CFuncBase<SFBTransform> > pPosition;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&modelBound); f.Add(3,&pPosition); return 0; }

	virtual bool NeedUpdate() { return  pPosition.Refresh(); }
	virtual void Recalc();
	CAnimObjBounder();
public:
	CAnimObjBounder( const NDb::SModel *pModel, CFuncBase<SFBTransform> *_pPosition );
};

CAnimObjBounder::CAnimObjBounder()
{
	Zero( value );
}

CAnimObjBounder::CAnimObjBounder( const NDb::SModel *pModel, CFuncBase<SFBTransform> *_pPosition ) : pPosition( _pPosition )
{
	Zero( value );

	if ( pModel && pModel->pSkeleton && pModel->pGeometry )
	{
		modelBound.BoxExInit( VNULL3, 0.5f * pModel->pGeometry->vSize );
		value.ptHalfBox = modelBound.ptHalfBox;
		value.s = modelBound.s;
	}
}

void CAnimObjBounder::Recalc()
{
	SFBTransform transform = pPosition->GetValue();
	transform.forward.RotateHVector( &value.s.ptCenter, modelBound.s.ptCenter );
}

void SModelVisObjDesc::Visit( IAIVisitor *pVisitor )
{
	if ( pModel && pModel->pGeometry->pAIGeometry ) 
		pVisitor->AddHull( pModel->pGeometry->pAIGeometry, GetPlacement().forward, 0, 0, 1 );
}

void SModelVisObjDesc::FillBBPoints( std::vector<CVec3> &bbPoints, std::vector<uint16_t> &bbIndices )
{
	if ( pModel != 0 && pModel->pGeometry != 0 )
	{
		CVec3 vCenter, vSize;
		if ( s_bGeomAABB )
		{
			vCenter = pModel->pGeometry->vCenter;
			vSize = pModel->pGeometry->vSize * 0.5f;
		}
		else if ( pModel->pGeometry->pAIGeometry != 0 )
		{
			vCenter = pModel->pGeometry->pAIGeometry->vAABBCenter;
			vSize = pModel->pGeometry->pAIGeometry->vAABBHalfSize;
		}
		//
		bbPoints[0] = vCenter + CVec3( -vSize.x, -vSize.y, -vSize.z );
		bbPoints[1] = vCenter + CVec3( -vSize.x, vSize.y, -vSize.z );
		bbPoints[2] = vCenter + CVec3( vSize.x, vSize.y, -vSize.z );
		bbPoints[3] = vCenter + CVec3( vSize.x, -vSize.y, -vSize.z );
		bbPoints[4] = vCenter + CVec3( -vSize.x, -vSize.y, vSize.z );
		bbPoints[5] = vCenter + CVec3( -vSize.x, vSize.y, vSize.z );
		bbPoints[6] = vCenter + CVec3( vSize.x, vSize.y, vSize.z );
		bbPoints[7] = vCenter + CVec3( vSize.x, -vSize.y, vSize.z );
		bbIndices[0] = 0; bbIndices[1] = 1; bbIndices[2] = 1; bbIndices[3] = 2;
		bbIndices[4] = 2; bbIndices[5] = 3; bbIndices[6] = 3; bbIndices[7] = 0;
		bbIndices[8] = 0; bbIndices[9] = 4; bbIndices[10] = 4; bbIndices[11] = 5;
		bbIndices[12] = 5; bbIndices[13] = 1; bbIndices[14] = 5; bbIndices[15] = 6;		
		bbIndices[16] = 6; bbIndices[17] = 2;	bbIndices[18] = 6; bbIndices[19] = 7;		
		bbIndices[20] = 7; bbIndices[21] = 3; bbIndices[22] = 7; bbIndices[23] = 4;		
	}
}

void SModelVisObjDesc::UpdateBBPolyLine( NGScene::IGameView *pGScene )
{
	std::vector<CVec3> bbPoints( 8 );
	std::vector<uint16_t> bbIndices( 24 );
	std::vector<CVec3> transformedBBPoints( 8 );
	FillBBPoints( bbPoints, bbIndices );
	if ( pModel != 0 )
	{
		for ( int i = 0; i < 8; ++i )
			GetPlacement().forward.RotateHVector( &(transformedBBPoints[i]), bbPoints[i] );
		pBBPolyLine = (CObjectBase*)( pGScene->CreatePolyline( transformedBBPoints, bbIndices, CVec4( 1.0f, 0, 0, 1.0f ), true ) );
	}
	else
		pBBPolyLine = 0;
}

void SModelVisObjDesc::UpdateSrcBind()
{
	if ( pModel && pModel->pGeometry->pAIGeometry ) 
		srcBind.Update();
}

void SModelVisObjDesc::UpdateStuff( CVisObjIconsManager *pVOIM )
{
	const SHMatrix &mPlace = GetPlacement().forward;
	CVec3 vObjPos = mPlace.GetTrans3();

	if ( bSelected )
	{
		const CVec3 vSize( selection.fSelSize, selection.fSelSize, 1.0f );
		CVec3 vVisPos = vObjPos;

		CQuat quat;
		quat.FromEulerMatrix( mPlace );
		CVec3 vRotCent;
		quat.Rotate( &vRotCent, selection.vSelCenter );

		vVisPos.x += vRotCent.x;
		vVisPos.y += vRotCent.y;

		if ( ( selection.eSelType == NDb::SELECTION_TYPE_GROUND ) && ( selection.pBound != 0 ) )
		{
			SBound b;
			SSelectionInfo info( vVisPos, vSize );
			info.MakeBound( &b );
			selection.pBound->Set( b );
			selection.selHolder.pPatch->SetInfo( info );
		}
		else if ( selection.pTransform != 0 && pModel != 0 )
		{
			SFBTransform place;
			vVisPos.z += pModel->pGeometry->vSize.z * 0.5f;
			MakeMatrix( &place, vSize, vVisPos );
			selection.pTransform->Set( place );
		}
	}

	if ( pModel ) 
	{
		//const CVec3 vIconPos = mPlace.GetTrans3();
		if ( pVOIM )
		{
			//pVOIM->UpdateIcon( nID, CVec3(vIconPos.x, vIconPos.y, vIconPos.z + pModel->pGeometry->pAIGeometry->vAABBHalfSize.z) );
			CVec3 vPos;
			mPlace.RotateHVector( &vPos, pModel->pGeometry->vCenter );
			vPos.z += pModel->pGeometry->vSize.z * 0.5f;
			pVOIM->UpdateIcon( nID, vPos );
		}
	}
}

void SModelVisObjDesc::ClearAttached( const ESceneSubObjType eType )
{
	for ( TAttachOfOneType::iterator it = attachedObjects[eType].begin(); it != attachedObjects[eType].end(); ++it )
		it->second->Destroy( GameTimer()->GetGameTime() );

	attachedObjects[eType].clear();
	n2Attach.clear();
	n2Attach.resize( __ESSOT_COUNTER, 0 );
}

// ************************************************************************************************************************ //
// **
// ** static vis obj
// **
// **
// **
// ************************************************************************************************************************ //

void SStaticVisObjDesc::MoveAfterUpdatePlacement( NGScene::IGameView *pGScene )
{
	if ( bHidden || pObj == 0 || pModel == 0 ) 
		return;
	NGScene::IGameView::SMeshInfo meshInfo;
	pGScene->CreateMeshInfo( pModel, &meshInfo, false );
	if ( CObjectBase *_pObj = pGScene->CreateMesh( meshInfo, transform, 0, 0 ) )
	{
		pObj = _pObj;
		if ( fFade != -1.0f )
		{
			NGScene::SetFade( pObj, fFade );
			if ( fFade == 1.0f )
				fFade = -1.0f;
		}
		if ( pModel->pGeometry->pAIGeometry ) 
			srcBind.Update();
	}
	if ( pBBPolyLine != 0 )
		UpdateBBPolyLine( pGScene );
}

void SStaticVisObjDesc::UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale )
{
	MakeMatrix( &transform, vScale, vPos, qRot );
	MoveAfterUpdatePlacement( pGScene );
}

void SStaticVisObjDesc::UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace )
{
	Invert( &transform.backward, mPlace );
	transform.forward = mPlace;
	MoveAfterUpdatePlacement( pGScene );
}

void SStaticVisObjDesc::ChangeModel( const NDb::SModel *pNewModel, CCSTime *pGameTimer, NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, bool bShowBB )
{
	pModel = pNewModel;
	ClearObject();
	ReCreateObject( pGScene, pSyncSrc, pGameTimer, bShowBB );
}

void SStaticVisObjDesc::ReCreateObject( NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB )
{
	if ( bHidden || pModel == 0 ) 
		return;
	NGScene::IGameView::SMeshInfo meshInfo;
	pGScene->CreateMeshInfo( pModel, &meshInfo, false );
	if ( CObjectBase *_pObj = pGScene->CreateMesh( meshInfo, transform, 0, 0 ) )
	{
		pObj = _pObj;
		if ( pModel->pGeometry->pAIGeometry && pSyncSrc ) 
		{
			srcBind.Link( pSyncSrc, this );
			srcBind.Update();
		}
		if ( fFade != -1.0f )
		{
			NGScene::SetFade( pObj, fFade );
			if ( fFade == 1.0f )
				fFade = -1.0f;
		}
	}

	// create attached objects
	for ( SModelVisObjDesc::CAttaches::iterator it = attachedObjects.begin(); it != attachedObjects.end(); ++it )
	{
		for ( SAnimatedVisObjDescBase::TAttachOfOneType::iterator itAttach = it->second.begin(); itAttach != it->second.end(); ++itAttach )
		{
			IAttachedObject *pObj = itAttach->second;
			pObj->ReCreate( pGScene, pTimer );
		}
	}

	if ( bShowBB )
		UpdateBBPolyLine( pGScene );
}

// ************************************************************************************************************************ //
// **
// ** dynamic vis obj
// **
// **
// **
// ************************************************************************************************************************ //

void SDynamicVisObjDesc::UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale )
{
	SFBTransform place;
	MakeMatrix( &place, vScale, vPos, qRot );
	if ( pTransform == 0 ) 
		pTransform = new CCSFBTransform( place );
	else
		pTransform->Set( place );
	if ( pBBPolyLine != 0 )
		UpdateBBPolyLine( pGScene );
}

void SDynamicVisObjDesc::UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace )
{
	SFBTransform place;
	Invert( &place.backward, mPlace );
	place.forward = mPlace;
	if ( pTransform == 0 ) 
		pTransform = new CCSFBTransform( place );
	else
		pTransform->Set( place );
	if ( pBBPolyLine != 0 )
		UpdateBBPolyLine( pGScene );
}

void SDynamicVisObjDesc::ReCreateObject( NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB )
{
	if ( bHidden || pModel == 0 ) 
		return;
	NGScene::IGameView::SMeshInfo meshInfo;
	pGScene->CreateMeshInfo( pModel, &meshInfo, false );
	if ( CObjectBase *_pObj = pGScene->CreateMesh( meshInfo, pTransform.GetPtr(), 0, 0 ) )
	{
		pObj = _pObj;
		if ( pModel->pGeometry->pAIGeometry && pSyncSrc ) 
		{
			srcBind.Link( pSyncSrc, this );
			srcBind.Update();
		}
		if ( fFade != -1.0f )
		{
			NGScene::SetFade( pObj, fFade );
			if ( fFade == 1.0f )
				fFade = -1.0f;
		}
	}
	if ( bShowBB )
		UpdateBBPolyLine( pGScene );
}

// ************************************************************************************************************************ //
// **
// ** animated vis obj
// **
// **
// **
// ************************************************************************************************************************ //

SAnimatedVisObjDescBase::SAnimatedVisObjDescBase() : SModelVisObjDesc()
{ 
}

void SAnimatedVisObjDescBase::UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale )
{
	SFBTransform transform;
	MakeMatrix( &transform, vScale, vPos, qRot );
	pTransform->Set( transform );
//	pAnimator->SetGlobalPosition( transform.forward );
	if ( pBBPolyLine != 0 )
		UpdateBBPolyLine( pGScene );
}

void SAnimatedVisObjDescBase::UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace )
{
	SHMatrix mPlaceInv;
	Invert( &mPlaceInv, mPlace );
	SFBTransform transform;
	transform.backward = mPlaceInv;
	transform.forward = mPlace;
	pTransform->Set( transform );
//	pAnimator->SetGlobalPosition( transform.forward );
	if ( pBBPolyLine != 0 )
		UpdateBBPolyLine( pGScene );
}

void SAnimatedVisObjDescBase::ClearObject() 
{ 
	SModelVisObjDesc::ClearObject();

	for ( SModelVisObjDesc::CAttaches::iterator it = attachedObjects.begin(); it != attachedObjects.end(); ++it )
	{
		for ( SAnimatedVisObjDescBase::TAttachOfOneType::iterator itAttach = it->second.begin(); itAttach != it->second.end(); ++itAttach )
		{
			IAttachedObject *pObj = itAttach->second;
			pObj->Clear( Singleton<IGameTimer>()->GetGameTime() );
		}
	}
}

void SAnimatedVisObjDescBase::ReCreateObject( NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB )
{
	if ( bHidden || pModel == 0 ) 
		return;
	// create main object
	NGScene::IGameView::SMeshInfo meshInfo;
	pGScene->CreateMeshInfo( pModel, &meshInfo, true );
	if ( CObjectBase *_pObj = CreateAnimatedMesh( pGScene, pModel, &meshInfo ) )
	{
		pObj = _pObj;
		if ( pModel->pGeometry->pAIGeometry && pSyncSrc ) 
		{
			srcBind.Link( pSyncSrc, this );
			srcBind.Update();
		}
		if ( fFade != -1.0f )
		{
			NGScene::SetFade( pObj, fFade );
			if ( fFade == 1.0f )
				fFade = -1.0f;
		}
	}
	// create attached objects
	for ( SModelVisObjDesc::CAttaches::iterator it = attachedObjects.begin(); it != attachedObjects.end(); ++it )
	{
		for ( SAnimatedVisObjDescBase::TAttachOfOneType::iterator itAttach = it->second.begin(); itAttach != it->second.end(); ++itAttach )
		{
			IAttachedObject *pObj = itAttach->second;
			pObj->ReCreate( pGScene, pTimer );
		}
	}

	if ( bShowBB )
		UpdateBBPolyLine( pGScene );
}

void SAnimatedVisObjDescBase::ChangeModel( const NDb::SModel *pNewModel, CCSTime *pGameTimer, 
																			NGScene::IGameView *pGScene, CSyncSrc<IVisObj> *pSyncSrc, bool bShowBB )
{
	if ( /*(pNewModel->pGeometry != pModel->pGeometry) ||*/ (pNewModel->pSkeleton != pModel->pSkeleton) ) 
	{
		NAnimation::SGrannySkeletonHandle handle;
		handle.pSkeleton = pNewModel->pSkeleton;
		handle.nModelInFile = 0;
		CPtr<NAnimation::ISkeletonAnimator> pTempAnimator = NAnimation::CreateSkeletonAnimator( handle, pGameTimer );
		if ( pTempAnimator == 0 )
			return;
		pAnimator = pTempAnimator;
		pAnimator->SetGlobalTransform( pTransform );
		// reset all attached objects in the case of animator change
		attachedObjects.clear();
		attachedObjects.reserve( __ESSOT_COUNTER );
	}
	pModel = pNewModel;
	ClearObject();
	ReCreateObject( pGScene, pSyncSrc, pGameTimer, bShowBB );
}

CObjectBase *SAnimatedVisObjDesc::CreateAnimatedMesh( NGScene::IGameView *pGScene, const NDb::SModel *pModel, 
																											NGScene::IGameView::SMeshInfo *pMeshInfoPassed )
{
	pBounder = new CAnimObjBounder( pModel, pTransform.GetPtr() );

	if ( pLowLevelModel )
	{
		NGScene::IGameView::SMeshInfo meshInfo;
		pGScene->CreateMeshInfo( pLowLevelModel, &meshInfo, true );
		pObjLowLevel = pGScene->CreateMesh( meshInfo, pTransform.GetPtr(), pBounder.GetPtr(), NGScene::CMeshAnimStuff( pLowLevelModel, pAnimator ), NGScene::SFullRoomInfo(0, NGScene::N_MASK_LOD_LOW) );

		return pGScene->CreateMesh( *pMeshInfoPassed, pTransform.GetPtr(), pBounder.GetPtr(), NGScene::CMeshAnimStuff( pModel, pAnimator ), NGScene::SFullRoomInfo(0, NGScene::N_MASK_LOD_HIGH) );
	}
	else
		return pGScene->CreateMesh( *pMeshInfoPassed, pTransform.GetPtr(), pBounder.GetPtr(), NGScene::CMeshAnimStuff( pModel, pAnimator ) );
}

CObjectBase *SAnimatedStaticVisObjDesc::CreateAnimatedMesh( NGScene::IGameView *pGScene, const NDb::SModel *pModel, 
																														NGScene::IGameView::SMeshInfo *pMeshInfoPassed )
{
	const SHMatrix &matrix = pTransform->GetValue().forward;
	CVec3 vCenter;
	matrix.RotateHVector( &vCenter, pModel->pGeometry->vCenter );
	SBound bound;
	bound.BoxExInit( vCenter, pModel->pGeometry->vSize * 0.5f * FP_SQRT_2 );
	pBounder = new CCSBound( bound );
	return pGScene->CreateMesh( *pMeshInfoPassed, pTransform.GetPtr(), bound, NGScene::CMeshAnimStuff( pModel, pAnimator ) );
}

// ************************************************************************************************************************ //
// **
// ** effect vis obj
// **
// **
// **
// ************************************************************************************************************************ //

void SStaticEffectVisObj::UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale )
{
	MakeMatrix( &transform, vScale, vPos, qRot );
}
void SStaticEffectVisObj::UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace )
{
	transform.forward = mPlace;
	Invert( &transform.backward, mPlace );
}
void SStaticEffectVisObj::ReCreateObject( NGScene::IGameView *pGView, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB )
{
	if ( bHidden || pEffect == 0 )
		return;
	pObj = pGView->CreateParticles( pEffect, timeStart, pTimer, transform );
}

void SDynamicEffectVisObj::UpdatePlacement( NGScene::IGameView *pGScene, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale )
{
	SFBTransform transform;
	MakeMatrix( &transform, vScale, vPos, qRot );
	if ( pTransform == 0 )
		pTransform = new CCSFBTransform( transform );
	else
		pTransform->Set( transform );
}
void SDynamicEffectVisObj::UpdatePlacement( NGScene::IGameView *pGScene, const SHMatrix &mPlace )
{
	SFBTransform transform;
	transform.forward = mPlace;
	Invert( &transform.backward, mPlace );
	if ( pTransform == 0 )
		pTransform = new CCSFBTransform( transform );
	else
		pTransform->Set( transform );
}
void SDynamicEffectVisObj::ReCreateObject( NGScene::IGameView *pGView, CSyncSrc<IVisObj> *pSyncSrc, CCSTime *pTimer, bool bShowBB )
{
	if ( bHidden || pEffect == 0 )
		return;
	pObj = pGView->CreateParticles( pEffect, timeStart, pTimer, pTransform );
}

START_REGISTER( VisObjCommands )
REGISTER_VAR_EX( "aabb_geom", NGlobal::VarBoolHandler, &s_bGeomAABB, false, STORAGE_NONE );
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x10141BC0, SStaticVisObjDesc )
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x10141BC1, SDynamicVisObjDesc )
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x10141BC2, SAnimatedVisObjDesc )
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x10141BC3, SAnimatedStaticVisObjDesc )
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x10141BC4, SStaticEffectVisObj )
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x10141BC5, SDynamicEffectVisObj )
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x3422C300, CAnimObjBounder )
BASIC_REGISTER_CLASS( SCENEB2, SVisObjDescBase )


