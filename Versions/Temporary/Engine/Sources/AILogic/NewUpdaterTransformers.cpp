#include "stdafx.h"

#include "NewUpdater.h"
#include "Diplomacy.h"
#include "Entrenchment.h"
#include "Formation.h"
#include "Graveyard.h"
#include "Soldier.h"

#include "Stats_B2_M1/StatusUpdates.h"
#include "Stats_B2_M1/SuperWeaponUpdates.h"

extern CEventUpdater updater;
extern CDiplomacy theDipl;

//Update Transformers

REGISTER_SAVELOAD_CLASS_NM( 0x111B4B40, CPlayEffectTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CPlayEffectTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	return static_cast_ptr<SPlayEffectUpdate*>( pUpdate->pData );
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3340, CChangeDBIDUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CChangeDBIDUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIChangeDBIDUpdate *pData;
	if ( pUpdate->pData )
		pData = static_cast_ptr<SAIChangeDBIDUpdate*>( pUpdate->pData );
	else
	{
		pData = new SAIChangeDBIDUpdate;
		pData->info.pStats = pUpdate->pObj->GetStats();
		pData->nObjUniqueID = pUpdate->pObj->GetUniqueId();
	}
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x111A9B01, CChangeVisibilityTransformer , CEventUpdater::CUpdateData )

SAIBasicUpdate* CEventUpdater::CUpdateData::CChangeVisibilityTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	if ( nReturnTime - CAITimer::GetSegmentTime() <= SConsts::AI_SEGMENT_DURATION )
	{
		SAIChangeVisibilityUpdate *pData = new SAIChangeVisibilityUpdate;
		pUpdate->pObj->GetPlacement( &(pData->info), SConsts::AI_SEGMENT_DURATION - (nReturnTime - CAITimer::GetSegmentTime() ) );
		pData->eUpdateType = pUpdate->eUpdateType;
		pData->nUpdateTime = nReturnTime;
		pData->bVisible = pUpdate->nParam;
		return pData;
	}
	return 0;
}

REGISTER_SAVELOAD_CLASS_NM( 0x11164380, CIdleTrenchUpdateTransformer , CEventUpdater::CUpdateData )

SAIBasicUpdate* CEventUpdater::CUpdateData::CIdleTrenchUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIEntranceUpdate *pData = new SAIEntranceUpdate;//checked_cast_ptr<SAIEntranceUpdate*>( pUpdate->pData );
	CSoldier *pSoldier = checked_cast_ptr<CSoldier*>( pUpdate->pObj );
	if ( pSoldier )
	{
		pData->eUpdateType = pUpdate->eUpdateType;
		pData->info.bEnter = pSoldier->IsInEntrenchment();
		pData->info.nInfantryUniqueID = pSoldier->GetUniqueId();
		pData->info.nTargetUniqueID = pSoldier->GetEntrenchment()->GetUniqueId();
	}
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x1115D400, CParadropStartedTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CParadropStartedTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIBasicUpdate* pData = pUpdate->pData;
	pData->eUpdateType = pUpdate->eUpdateType;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3341, CPlacementUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CPlacementUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	if ( nReturnTime - CAITimer::GetSegmentTime() <= SConsts::AI_SEGMENT_DURATION )
	{
		SAIPlacementUpdate* pData = new SAIPlacementUpdate;
		pUpdate->pObj->GetPlacement( &(pData->info), SConsts::AI_SEGMENT_DURATION - (nReturnTime - CAITimer::GetSegmentTime() ) );
		NI_ASSERT( pUpdate->eUpdateType == ACTION_NOTIFY_PLACEMENT ||
			pUpdate->eUpdateType == ACTION_NOTIFY_ST_OBJ_PLACEMENT, "Wrong update type" );
		pData->eUpdateType = pUpdate->eUpdateType;
		pData->nUpdateTime = nReturnTime;
		return pData;
	}
	return 0;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3342, CRPGUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CRPGUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIRPGUpdate *pData = new SAIRPGUpdate;
	pUpdate->pObj->GetRPGStats( &(pData->info) );
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	pData->info.time = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3343, CHitUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CHitUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIHitUpdate *pData = new SAIHitUpdate;
	pUpdate->pObj->GetHitInfo( &(pData->info) );
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3344, CHTurretTurnUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CHTurretTurnUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAITurretUpdate *pData = new SAITurretUpdate;
	pUpdate->pObj->GetHorTurretTurnInfo( &(pData->info) );
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3345, CVTurretTurnUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CVTurretTurnUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAITurretUpdate *pData = new SAITurretUpdate;
	pUpdate->pObj->GetVerTurretTurnInfo( &(pData->info) );
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3346, CEntranceUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CEntranceUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIEntranceUpdate *pData = new SAIEntranceUpdate;
	pUpdate->pObj->GetEntranceStateInfo( &(pData->info) );
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x11163B00, CModifyEntranceUpdateTransformer, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CModifyEntranceUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIModifyEntranceStateUpdate *pData = new SAIModifyEntranceStateUpdate;

	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	pData->nObjUniqueID = pUpdate->pObj->GetUniqueId();
	pData->bOpen = pUpdate->nParam;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3347, CDiplomacyUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CDiplomacyUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIDiplomacyUpdate *pData = new SAIDiplomacyUpdate;
	const int nPlayer = pUpdate->pObj->GetPlayer();
	const bool bNeutral = nPlayer == theDipl.GetNeutralPlayer() || nPlayer >= theDipl.GetNPlayers();
	pData->info.eDiplomacy = bNeutral ? EDI_NEUTRAL : theDipl.GetDiplStatus( theDipl.GetMyNumber(), nPlayer );
	pData->info.nPlayer = nPlayer;
	pData->info.nObjUniqueID = pUpdate->pObj->GetUniqueId();
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3348, CShootAreaUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CShootAreaUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIShootAreaUpdate *pData = new SAIShootAreaUpdate;
	pData->info.resize( 1 );
	int nTemp;
	pUpdate->pObj->GetShootAreas( &(pData->info[0]), &nTemp );
	//pData->info.resize( nTemp );
	pData->nObjID = pUpdate->pObj->GetUniqueId();
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	if ( pUpdate->pObj->IsAlive() )
		updater.RegisterShootAreaUnit( pUpdate->pObj );
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3349, CRangeAreaUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CRangeAreaUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIShootAreaUpdate *pData = new SAIShootAreaUpdate;
	pData->info.resize( 1 );
	construct( static_cast<SShootAreas*>( &(pData->info[0]) ) );
	pUpdate->pObj->GetRangeArea( &(pData->info[0]) );
	pData->nObjID = pUpdate->pObj->GetUniqueId();
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	if ( pUpdate->pObj->IsAlive() )
		updater.RegisterShootAreaUnit( pUpdate->pObj );
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B334A, CNewProjectileUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CNewProjectileUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAINewProjectileUpdate *pData = new SAINewProjectileUpdate;
	pUpdate->pObj->GetProjectileInfo( &(pData->info) );
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B334B, CDeadUnitUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CDeadUnitUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIDeadUnitUpdate *pData = new SAIDeadUnitUpdate;
	CDeadUnit *pDead = checked_cast_ptr<CDeadUnit*>( pUpdate->pObj );
	pDead->GetDyingInfo( &(pData->dieAnimation), &(pData->bVisibleWhenDie) );
	pData->nDeadObj = pDead->GetDieObject()->GetUniqueId();
	pDead->GetDieObject()->GetPlacement( &(pData->placement), 0.0f );
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B334C, CDisappearUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CDisappearUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIDissapearObjUpdate *pData = new SAIDissapearObjUpdate;
	pData->nDissapearObjID = pUpdate->pObj->GetUniqueId();
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	pData->bShowEffects = pUpdate->nParam;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B334D, CNewUnitUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CNewUnitUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAINewUnitUpdate *pData = new SAINewUnitUpdate;
	pUpdate->pObj->GetNewUnitInfo( &(pData->info) );
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B334E, CNewEntrenchmentUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CNewEntrenchmentUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAITrenchUpdate *pData = new SAITrenchUpdate;
	NI_ASSERT( dynamic_cast<CEntrenchmentPart*>( pUpdate->pObj.GetPtr() ) != 0, "Wrong object's type, entrenchment expected" );
	CEntrenchmentPart *pPart = checked_cast<CEntrenchmentPart*>( pUpdate->pObj.GetPtr() );
	//	NI_VERIFY( pPart != 0 && pPart->GetOwner() != 0, "Invalid entrenchment part in updater", return 0 );
	pData->info.nSegmentID = pPart->GetUniqueId();
	pData->info.nEntrenchID = pPart->GetOwner() ? pPart->GetOwner()->GetUniqueId() : 0;
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	pData->bLast = ( pUpdate->nParam > 0 );
	pData->bDigBySegment = ( pUpdate->nParam * pUpdate->nParam == 4 );				// 2 or -2
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B334F, CNewFormationUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CNewFormationUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIFormationUpdate *pData = new SAIFormationUpdate;
	NI_ASSERT( dynamic_cast<CSoldier*>( pUpdate->pObj.GetPtr() ) != 0, "Wrong unit's type, soldier expected" );
	CSoldier *pSoldier = checked_cast<CSoldier*>( pUpdate->pObj.GetPtr() );
	pData->info.nSoldierID = pSoldier->GetUniqueId();
	pData->info.nFormationID = pSoldier->GetFormation()->GetUniqueId();
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3350, CRevealArtilleryUpdateTransformer , CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CRevealArtilleryUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAICircleUpdate *pData = new SAICircleUpdate;
	pUpdate->pObj->GetRevealCircle( &(pData->info) );
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	pData->nParam = pUpdate->nParam;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x120B3351, CStructCopierUpdateTransformer, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CStructCopierUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIBasicUpdate *pData = pUpdate->pData;
	pData->eUpdateType = pUpdate->eUpdateType;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x11131BC0, CObjectsUnderConstructionTransformer, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CObjectsUnderConstructionTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIBasicUpdate *pData = pUpdate->pData;
	pData->eUpdateType = pUpdate->eUpdateType;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x1913EC40, CKeyBuildingUpdateTransformer, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CKeyBuildingUpdateTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIBasicUpdate *pData = pUpdate->pData;
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x1B18F300, CScriptCameraRunTransformer, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CScriptCameraRunTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIBasicUpdate *pData = pUpdate->pData;
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x1B18F500, CScriptCameraResetTransformer, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CScriptCameraResetTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIBasicUpdate *pData = pUpdate->pData;
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x1B225440, CScriptCameraStartMovieTransformer, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CScriptCameraStartMovieTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIBasicUpdate *pData = pUpdate->pData;
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x1B225441, CScriptCameraStopMovieTransformer, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CScriptCameraStopMovieTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIBasicUpdate *pData = pUpdate->pData;
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x1B1A7381, CWeatherChangedTransformer, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CWeatherChangedTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SAIBasicUpdate *pData = pUpdate->pData;
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x31247341, CUpdateStatusTransformer, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CUpdateStatusTransformer::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SUnitStatusUpdate *pData = static_cast_ptr<SUnitStatusUpdate*>( pUpdate->pData );
	pData->nUnitID = pUpdate->pObj->GetUniqueId();
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x31249C00, CUpdateSuperWeaponControlTransform, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CUpdateSuperWeaponControlTransform::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SSuperWeaponControl *pData = static_cast_ptr<SSuperWeaponControl*>( pUpdate->pData );
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}

REGISTER_SAVELOAD_CLASS_NM( 0x31249C01, CUpdateSuperWeaponRecycleTransform, CEventUpdater::CUpdateData );

SAIBasicUpdate* CEventUpdater::CUpdateData::CUpdateSuperWeaponRecycleTransform::Transform( CUpdateData *pUpdate, int nReturnTime )
{
	SSuperWeaponRecycle *pData = static_cast_ptr<SSuperWeaponRecycle*>( pUpdate->pData );
	pData->eUpdateType = pUpdate->eUpdateType;
	pData->nUpdateTime = pUpdate->nUpdateTime;
	return pData;
}


