#include "StdAfx.h"

#include "AttachedObj.h"
#include "SceneInternal.h"
#include "Main/GameTimer.h"

IAttachedObject *CScene::GetAttachedObject( const int nTargetID, const string &szBoneName )
{
	// try to find the bone in attached objects
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nTargetID );
	if ( pos == data[eScene]->visObjects.end() )
		return 0;

	CDynamicCast<SAnimatedVisObjDesc> pVO = pos->second;
	if ( !pVO.GetPtr() )
		return 0;

	for ( SModelVisObjDesc::CAttaches::iterator itAtt = pVO->attachedObjects.begin(); itAtt != pVO->attachedObjects.end(); ++itAtt )
	{
		SModelVisObjDesc::TAttachOfOneType &attType = itAtt->second;
		for ( SAnimatedVisObjDescBase::TAttachOfOneType::iterator iter = attType.begin(); iter != attType.end(); ++iter )
		{
			NI_VERIFY( iter->second != 0, "Null attached obj", continue );
			if ( CDynamicCast<NAnimation::IGetBone> pGetBone = iter->second->GetAnimator() )
			{
				if ( pGetBone->GetBoneIndex( szBoneName.c_str() ) != -1 )
					return iter->second;
			}
		}
	}
	return 0;
}

NAnimation::ISkeletonAnimator* CScene::GetAnimator( const int nTargetID, const string &szBoneName )
{
	NAnimation::ISkeletonAnimator *pAnimator = GetAnimator( nTargetID );
	CDynamicCast<NAnimation::IGetBone> pGetBone = pAnimator;

	if ( pGetBone && pGetBone->GetBoneIndex( szBoneName.c_str() ) != -1 )
		return pAnimator;
	else
	{
		if ( IAttachedObject *pObj = GetAttachedObject( nTargetID, szBoneName ) )
			return pObj->GetAnimator();
	}

	return 0;
}

CFuncBase<SBound> *CScene::GetObjectBounder( const int nID )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	return pos == data[eScene]->visObjects.end() ? 0 : pos->second->GetBounder();
}

CFuncBase<SFBTransform> *CScene::GetParentTransform( const int nTargetID, const string &szBoneName )
{
	NAnimation::ISkeletonAnimator *pAnimator = GetAnimator( nTargetID );
	CDynamicCast<NAnimation::IGetBone> pGetBone = pAnimator;

	if ( pGetBone && pGetBone->GetBoneIndex( szBoneName.c_str() ) != -1 )
		return 0;
	else
	{
		if ( IAttachedObject *pObj = GetAttachedObject( nTargetID, szBoneName ) )
			return pObj->GetTransform();
	}

	return 0;
}

bool CScene::PrepareToAttach( const int nTargetID, ESceneSubObjType eType, const string &szBoneName, const ESceneAttachMode eMode, const NTimer::STime time,
															CFuncBase<SFBTransform> **pTransform, int *pnBoneIndex, const bool bConstantOffset )
{
	NAnimation::ISkeletonAnimator *pAnimator = GetAnimator( nTargetID, szBoneName );
	if ( !pAnimator )
		return false;

	if ( bConstantOffset )
	{
		CFuncBase<SFBTransform> *pParentTransform = GetParentTransform( nTargetID, szBoneName );
		if ( pParentTransform )
			*pTransform = new CConstantOffsetTransform( nTargetID, szBoneName, pAnimator->CreateTransform( szBoneName ), pParentTransform );
		else
			*pTransform = new CConstantOffsetTransform( nTargetID, szBoneName, pAnimator->CreateTransform( szBoneName ) );
	}
	else
		*pTransform = pAnimator->CreateTransform( szBoneName );

	if ( !(*pTransform) )
		return false;

	CDynamicCast<NAnimation::IGetBone> pGetBone = pAnimator;
	*pnBoneIndex = pGetBone->GetBoneIndex( szBoneName.c_str() );
	if ( *pnBoneIndex == -1 )
		return false;
	
	return DeleteAttachesByType( nTargetID, eType, eMode, time, *pnBoneIndex );
}

bool CScene::PrepareToAttach( const int nTargetID, ESceneSubObjType eType, const ESceneAttachMode eMode, const NTimer::STime time,
															CFuncBase<SFBTransform> **pTransform, const CVec3 &vOffset )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nTargetID );
	if ( pos == data[eScene]->visObjects.end() )
		return false;

	CFuncBase<SBound> *pBound = pos->second->GetBounder();
	SHMatrix mForward;
	Identity( &mForward );
	mForward.xw = vOffset.x;
	mForward.yw = vOffset.y;
	mForward.zw = pBound->GetValue().ptHalfBox.z + vOffset.z;

	*pTransform = new CCenterOffsetTransform( nTargetID, pos->second->GetTransform(), mForward );

	if ( !(*pTransform) )
		return false;

	return DeleteAttachesByType( nTargetID, eType, eMode, time, -1 );
}

bool CScene::DeleteAttachesByType( const int nTargetID, ESceneSubObjType eType, const ESceneAttachMode eMode, const NTimer::STime timeStart, const int nBoneIndex )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nTargetID );
	if ( pos == data[eScene]->visObjects.end() )
		return false;

	CDynamicCast<SModelVisObjDesc> pVO = pos->second;
	if ( !pVO.GetPtr() )
		return false;

	SModelVisObjDesc::TAttachOfOneType &attaches = pVO->attachedObjects[eType];
	switch ( eMode )
	{
	case ESAT_REPLACE_ON_TYPE:
		pVO->ClearAttached( eType );

		break;
	case ESAT_REPLACE_ON_BONE:
		{
			list<int> toErase;
			for ( SModelVisObjDesc::TAttachOfOneType::iterator it = attaches.begin(); it != attaches.end(); ++it )
			{
				IAttachedObject *pAttachedObj = it->second;
				if ( pAttachedObj->GetBoneIndex() == nBoneIndex )
				{
					it->second->Destroy( timeStart );
					toErase.push_back( it->first );
				}
			}

			for ( list<int>::iterator iter = toErase.begin(); iter != toErase.end(); ++iter )
				attaches.erase( *iter );
		}

		break;
	case ESAT_NO_REPLACE:
		break;
	default:
		NI_ASSERT( false, StrFmt( "Wrong attach mode (%d)", (int)eMode ) );
		return false;
	}

	return true;
}

void CScene::AttachSubModel( const int nTargetID, ESceneSubObjType eType, const string &szBoneName, const NDb::SModel *pSubModel, ESceneAttachMode eMode, const int nNumber, bool bForceAnimated, const bool bConstantOffset )
{
	CFuncBase<SFBTransform> *pTransform = 0;
	int nBoneIndex;
	if ( PrepareToAttach( nTargetID, eType, szBoneName, eMode, Singleton<IGameTimer>()->GetGameTime(), &pTransform, &nBoneIndex, bConstantOffset ) )
	{
		CDynamicCast<SModelVisObjDesc> pVO = data[eScene]->visObjects[nTargetID];
		SModelVisObjDesc::TAttachOfOneType &attaches = pVO->attachedObjects[eType];

		IAttachedObject *pObject = 0;
		if ( bForceAnimated || !pSubModel->animations.empty() )
			pObject = IAttachedObject::CreateAnimatedObj( pSubModel, pTransform, data[eScene]->pGameTimer, nBoneIndex, GetObjectBounder( nTargetID ) );
		else
		{
			pObject = IAttachedObject::CreateStaticObj( pSubModel, pTransform, nBoneIndex, GetObjectBounder( nTargetID ), eType == ESSOT_WEAPON );
		}

		if ( pObject )
		{
			SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nTargetID );
			if ( pos == data[eScene]->visObjects.end() )
			{
				if ( !pos->second->bHidden )
					pObject->ReCreate( data[eScene]->GetGScene(), data[eScene]->pGameTimer );
			}

			const int nNumber2Attach = nNumber >= 0 ? nNumber : --(pVO->n2Attach[eType]);
			attaches[nNumber2Attach] = pObject;
			const int nDebugWantToLookAtAttaches = 10;
		}

	}

	CPtr< CFuncBase<SFBTransform> > pDelTransform = pTransform;
}

void CScene::AttachSubModel( const int nTargetID, ESceneSubObjType eType, const NDb::SModel *pSubModel, ESceneAttachMode eMode, 
	const int nNumber, bool bForceAnimated, const CVec3 &vOffset )
{
	CFuncBase<SFBTransform> *pTransform = 0;
	const int nBoneIndex = -1;
	if ( PrepareToAttach( nTargetID, eType, eMode, Singleton<IGameTimer>()->GetGameTime(), &pTransform, vOffset ) )
	{
		CDynamicCast<SModelVisObjDesc> pVO = data[eScene]->visObjects[nTargetID];
		SModelVisObjDesc::TAttachOfOneType &attaches = pVO->attachedObjects[eType];

		IAttachedObject *pObject = 0;
		if ( bForceAnimated || !pSubModel->animations.empty() )
			pObject = IAttachedObject::CreateAnimatedObj( pSubModel, pTransform, data[eScene]->pGameTimer, nBoneIndex, 0/*GetObjectBounder( nTargetID )*/ );
		else
			pObject = IAttachedObject::CreateStaticObj( pSubModel, pTransform, nBoneIndex, 0/*GetObjectBounder( nTargetID )*/ );

		if ( pObject )
		{
			SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nTargetID );
			if ( pos == data[eScene]->visObjects.end() )
			{
				if ( !pos->second->bHidden )
					pObject->ReCreate( data[eScene]->GetGScene(), data[eScene]->pGameTimer );
			}

			const int nNumber2Attach = nNumber >= 0 ? nNumber : --(pVO->n2Attach[eType]);
			attaches[nNumber2Attach] = pObject;
			const int nDebugWantToLookAtAttaches = 10;
		}

	}

	CPtr< CFuncBase<SFBTransform> > pDelTransform = pTransform;
}

void CScene::AttachEffect( const int nTargetID, ESceneSubObjType eType, CFuncBase<SFBTransform> *pTransform, const NDb::SEffect *pEffect, NTimer::STime timeStart, ESceneAttachMode eMode, const int nBoneIndex, bool bVertical )
{
	CDynamicCast<SModelVisObjDesc> pVO = data[eScene]->visObjects[nTargetID];
	SModelVisObjDesc::TAttachOfOneType &attaches = pVO->attachedObjects[eType];

	IAttachedObject *pObject = IAttachedObject::CreateEffect( pEffect, pTransform, timeStart, nBoneIndex, bVertical );
	if ( pObject )
	{
		pObject->ReCreate( data[eScene]->GetGScene(), data[eScene]->pGameTimer );
		attaches[--(pVO->n2Attach[eType])] = pObject;
	}

	CPtr< CFuncBase<SFBTransform> > pDelTransform = pTransform;
}

void CScene::AttachEffect( const int nTargetID, ESceneSubObjType eType, const string &szBoneName, const NDb::SEffect *pEffect, NTimer::STime timeStart, ESceneAttachMode eMode, bool bVertical )
{
	CFuncBase<SFBTransform> *pTransform = 0;
	int nBoneIndex;
	if ( PrepareToAttach( nTargetID, eType, szBoneName, eMode, timeStart, &pTransform, &nBoneIndex, false ) )
		AttachEffect( nTargetID, eType, pTransform, pEffect, timeStart, eMode, nBoneIndex, bVertical );

	CPtr< CFuncBase<SFBTransform> > pDelTransform = pTransform;
}

void CScene::AttachEffect( const int nTargetID, ESceneSubObjType eType, const string &szBoneName, const SHMatrix &mOffset, const NDb::SEffect *pEffect, NTimer::STime timeStart, ESceneAttachMode eMode )
{
	CFuncBase<SFBTransform> *pTransform = 0;
	int nBoneIndex;
	if ( PrepareToAttach( nTargetID, eType, szBoneName, eMode, timeStart, &pTransform, &nBoneIndex, false ) )
		AttachEffect( nTargetID, eType, new CAttachedLightEffectTransform( pTransform, mOffset) , pEffect, timeStart, eMode, nBoneIndex );

	CPtr< CFuncBase<SFBTransform> > pDelTransform = pTransform;
}

void CScene::AttachLightEffect( const int nTargetID, const NDb::SAttachedLightEffect *pLight, NTimer::STime timeStart, ESceneAttachMode eMode, const bool bInEditor, int nHoldID )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nTargetID );
	if ( pos == data[eScene]->visObjects.end() )
		return;

	CDynamicCast<SModelVisObjDesc> pVO = pos->second;
	if ( pVO == 0 )
		return;

	CDynamicCast<SStaticVisObjDesc> pStaticVO = pos->second;
	const bool bIsAnimated = pStaticVO == 0;

	CFuncBase<SFBTransform> *pTransform = 0;
	if ( bIsAnimated )
	{
		int nBoneIndex;
		if ( PrepareToAttach( nTargetID, ESSOT_LIGHT, pLight->szLocatorName, eMode, timeStart, &pTransform, &nBoneIndex, false ) )
		{
			SModelVisObjDesc::TAttachOfOneType &attaches = pVO->attachedObjects[ESSOT_LIGHT];

			IAttachedObject *pObject = IAttachedObject::CreateAnimatedLightEffect( pLight, pTransform, timeStart, nBoneIndex, bInEditor );
			if ( pObject )
			{
				pObject->ReCreate( data[eScene]->GetGScene(), data[eScene]->pGameTimer );
				if ( nHoldID < 0 )
					nHoldID = --( pVO->n2Attach[ ESSOT_LIGHT ] );
				attaches[ nHoldID ] = pObject;
			}
		}
	}
	else
	{
		CDynamicCast<SStaticVisObjDesc> pStaticVO = data[eScene]->visObjects[nTargetID];
		NI_ASSERT( pStaticVO, StrFmt( "VisObject %d is neither animated nor static", nTargetID ) );

		if ( pStaticVO ) 
		{
			DeleteAttachesByType( nTargetID, ESSOT_LIGHT, eMode, timeStart, 0 );
			SModelVisObjDesc::TAttachOfOneType &attaches = pStaticVO->attachedObjects[ESSOT_LIGHT];

			IAttachedObject *pObject = IAttachedObject::CreateStaticLightEffect( pLight, nTargetID, timeStart, bInEditor );
			if ( pObject )
			{
				pObject->ReCreate( data[eScene]->GetGScene(), data[eScene]->pGameTimer );
				if ( nHoldID < 0 )
					nHoldID = --( pVO->n2Attach[ ESSOT_LIGHT ] );
				attaches[ nHoldID ] = pObject;
			}
		}
	}

	CPtr< CFuncBase<SFBTransform> > pDelTransform = pTransform;
}

void CScene::RemoveAttached( const int nTargetID, ESceneSubObjType eType, const int nNumber )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nTargetID );
	if ( pos != data[eScene]->visObjects.end() )	
	{
		if ( SModelVisObjDesc *pVO = dynamic_cast_ptr<SModelVisObjDesc*>( pos->second ) )
		{
			SModelVisObjDesc::CAttaches::iterator itAtt = pVO->attachedObjects.find( eType );
			if ( itAtt != pVO->attachedObjects.end() )
			{
				SModelVisObjDesc::TAttachOfOneType &attType = itAtt->second;
				SModelVisObjDesc::TAttachOfOneType::iterator posAttaches = attType.find( nNumber );
				if ( posAttaches != attType.end() )
					attType.erase( posAttaches );
			}
		}
	}
}

void CScene::RemoveAllAttached( const int nTargetID, ESceneSubObjType eType )
{
	if ( eScene == ES_UNKNOWN )
		return;

	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nTargetID );
	if ( pos != data[eScene]->visObjects.end() )	
	{
		if ( SModelVisObjDesc *pVO = dynamic_cast_ptr<SModelVisObjDesc*>( pos->second ) )
			pVO->ClearAttached( eType );
	}
}

IAttachedObject* CScene::GetAttached( const int nTargetID, ESceneSubObjType eType, const int nNumber )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nTargetID );
	if ( pos != data[eScene]->visObjects.end() )	
	{
		if ( SModelVisObjDesc *pVO = dynamic_cast_ptr<SModelVisObjDesc*>( pos->second ) )
		{
			SModelVisObjDesc::CAttaches::iterator itAtt = pVO->attachedObjects.find( eType );
			if ( itAtt != pVO->attachedObjects.end() )
			{
				SModelVisObjDesc::TAttachOfOneType &attType = itAtt->second;
				SModelVisObjDesc::TAttachOfOneType::iterator posAttaches = attType.find( nNumber );
				if ( posAttaches != attType.end() )
				{
					posAttaches->second->Destroy( Singleton<IGameTimer>()->GetGameTime() );
					return posAttaches->second;
				}
			}
		}
	}

	return 0;
}

void CScene::AddAttachedMapping( int nAttachObjID, int nMapObjID )
{
	data[eScene]->attachIDToMapObjID[nAttachObjID] = nMapObjID;
}

void CScene::RemoveAttachedMapping( int nAttachObjID )
{
	data[eScene]->attachIDToMapObjID.erase( nAttachObjID );
}


