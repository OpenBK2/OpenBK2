#include "stdafx.h"

#include "VisObjDesc.h"
#include "SceneInternal.h"
#include "SceneHoldQueue.h"

int CScene::AddEffect( const int nID, const NDb::SEffect *pEffect, NTimer::STime timeStart, const CVec3 &_vPos, const CQuat &qRot )
{
	NI_VERIFY( pEffect != 0, StrFmt("Adding effect %d with empty descriptor", nID), return -1 );
	//
	CVec3 vPos;
	AI2Vis( &vPos, _vPos );
	//
	CPtr<SEffectVisObjBase> pVOD;
	if ( nID == OBJECT_ID_FORGET )
		pVOD = new SStaticEffectVisObj();
	else
		pVOD = new SDynamicEffectVisObj();
	pVOD->UpdatePlacement( data[eScene]->GetGScene(), vPos, qRot, CVec3( 1, 1, 1 ) );
	pVOD->timeStart = timeStart;
	pVOD->pEffect = pEffect;
	pVOD->ReCreateObject( data[eScene]->GetGScene(), 0, data[eScene]->pGameTimer, false );
	if ( pVOD->pObj == 0 )
		return -1;
	if ( nID == OBJECT_ID_FORGET )
	{
		pVOD->nID = -1;
		SetToSceneHoldQueue( pVOD, true );
		return -1;
	}
	else
	{
		const int nObjectID = GetID( nID );
		NI_ASSERT( data[eScene]->visObjects.find(nObjectID) == data[eScene]->visObjects.end(), StrFmt("Object 0x%.8x already exist", nObjectID) );
		pVOD->nID = nObjectID;
		data[eScene]->visObjects[nObjectID] = pVOD;
		return nObjectID;
	}
}

int CScene::AddEffect( const int nID, const NDb::SEffect *pEffect, NTimer::STime timeStart, const SHMatrix &mPlace )
{
	NI_VERIFY( pEffect != 0, StrFmt("Adding effect %d with empty descriptor", nID), return -1 );
	//
	CPtr<SEffectVisObjBase> pVOD;
	if ( nID == OBJECT_ID_FORGET )
		pVOD = new SStaticEffectVisObj();
	else
		pVOD = new SDynamicEffectVisObj();
	pVOD->UpdatePlacement( data[eScene]->GetGScene(), mPlace );
	pVOD->timeStart = timeStart;
	pVOD->pEffect = pEffect;
	pVOD->ReCreateObject( data[eScene]->GetGScene(), 0, data[eScene]->pGameTimer, false );
	if ( pVOD->pObj == 0 )
		return -1;	
	if ( nID == OBJECT_ID_FORGET )
	{
		pVOD->nID = -1;
		SetToSceneHoldQueue( pVOD, true );
		return -1;
	}
	else
	{
		const int nObjectID = GetID( nID );
		NI_ASSERT( data[eScene]->visObjects.find(nObjectID) == data[eScene]->visObjects.end(), StrFmt("Object 0x%.8x already exist", nObjectID) );
		pVOD->nID = nObjectID;
		data[eScene]->visObjects[nObjectID] = pVOD;
		return nObjectID;
	}
}

void CScene::AddEffect( const int nID, const string &szBoneName, const NDb::SEffect *pEffect, NTimer::STime timeStart, const SHMatrix &mPlace )
{
	NI_VERIFY( pEffect != 0, StrFmt("Adding effect %d with empty descriptor", nID), return );
	NAnimation::ISkeletonAnimator *pAnimator = GetAnimator( nID );
	if ( pAnimator == 0 )
		return;
	SHMatrix mBone;
	if ( !pAnimator->GetBonePosition( szBoneName.c_str(), &mBone ) )
		return;
	SHMatrix mPlacement;
	Multiply( &mPlacement, mBone, mPlace );
	//
	AddEffect( OBJECT_ID_FORGET, pEffect, timeStart, mPlacement );
}

void CScene::StopEffectGeneration( const int nID, NTimer::STime time )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos == data[eScene]->visObjects.end() ) 
		return;																			// No such effect
	if ( pos->second->pObj == 0 )
	{
		data[eScene]->visObjects.erase( pos );
		return;
	}
	//
	NGScene::StopParticlesGeneration( pos->second->pObj, time );
	NGScene::StopDynamicLighting( pos->second->pObj );
	SetToSceneHoldQueue( pos->second, true );
	data[eScene]->visObjects.erase( pos );
}


