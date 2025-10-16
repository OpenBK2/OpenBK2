#include "stdafx.h"

#include "Diplomacy.h"
#include "Formation.h"
#include "NewUpdater.h"
#include "Cheats.h"
#include "Stats_B2_M1/AnimationFromAction.h"
#include "3Dmotor/DBScene.h"
#include "GroupLogic.h"
#include "Artillery.h"
#include "Soldier.h"

#include "AILogic_export.h"

#include <algorithm>

#include <fmt/format.h>

//#define N_GRIDCELL_SIZE 8

CEventUpdater updater;
extern CGroupLogic theGroupLogic;
extern CDiplomacy theDipl;
extern SCheats theCheats;

//CUpdateData

det_map< int, CPtr<CEventUpdater::CUpdateData::IUpdateTransformer> > CEventUpdater::CUpdateData::clientTransformers;
REGISTER_SAVELOAD_CLASS_NM( AILOGIC, 0x110B2C80, CUpdateData , CEventUpdater );
//REGISTER_SAVELOAD_CLASS_NM( AILOGIC, 0x110B94C0, CInterpolatableUpdate, CEventUpdater );

const int DIVIDER_CONST = AI_TILES_IN_VIS_TILE * 2;

void CEventUpdater::CUpdateData::Init()
{
	clientTransformers.clear();
	clientTransformers[ACTION_NOTIFY_CHANGE_DBID] = new CChangeDBIDUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_PARADROP_STARTED] = new CParadropStartedTransformer;

	clientTransformers[ACTION_NOTIFY_ST_OBJ_PLACEMENT] = new CPlacementUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_PLACEMENT] = new CPlacementUpdateTransformer;
//	static_cast<CPlacementUpdateTransformer*>( clientTransformers[ACTION_NOTIFY_PLACEMENT].GetPtr() )->SetTimer();
	clientTransformers[ACTION_NOTIFY_RPG_CHANGED] = new CRPGUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_MECH_SHOOT] = new CStructCopierUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_INFANTRY_SHOOT] = new CStructCopierUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_HIT] = new CHitUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_TURRET_HOR_TURN] = new CHTurretTurnUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_TURRET_VERT_TURN] = new CVTurretTurnUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_ENTRANCE_STATE] = new CEntranceUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_MODIFY_ENTRANCE_STATE] = new CModifyEntranceUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_UPDATE_DIPLOMACY] = new CDiplomacyUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_SHOOT_AREA] = new CShootAreaUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_RANGE_AREA] = new CRangeAreaUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_NEW_PROJECTILE] = new CNewProjectileUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_DEAD_UNIT] = new CDeadUnitUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_DELETED_ST_OBJ] = new CDisappearUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_DISSAPEAR_OBJ] = new CDisappearUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_DEAD_PROJECTILE] = new CDisappearUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_NEW_BRIDGE_SPAN] = new CNewUnitUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_NEW_UNIT] = new CNewUnitUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_NEW_ST_OBJ] = new CNewUnitUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_NEW_ENTRENCHMENT] = new CNewEntrenchmentUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_NEW_FORMATION] = new CNewFormationUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_REVEAL_ARTILLERY] = new CRevealArtilleryUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_REINF_POINT] = new CStructCopierUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_AVAIL_REINF] = new CStructCopierUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_REINF_RECYCLE] = new CStructCopierUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_SPECIAL_ABLITY] = new CStructCopierUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_TREE_BROKEN] = new CStructCopierUpdateTransformer;	
	clientTransformers[ACTION_NOTIFY_FEEDBACK] = new CStructCopierUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_OBJECTS_UNDER_CONSTRUCTION] = new CObjectsUnderConstructionTransformer;
	clientTransformers[ACTION_NOTIFY_NEW_KEY_BUILDING] = new CKeyBuildingUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_KEY_BUILDING_CAPTURED] = new CKeyBuildingUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_KEY_BUILDING_LOST] = new CKeyBuildingUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_KEY_CAPTURE_PROGRESS] = new CKeyBuildingUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_IDLE_TRENCH] = new CIdleTrenchUpdateTransformer;
	clientTransformers[ACTION_NOTIFY_SCAMERA_RUN] = new CScriptCameraRunTransformer;
	clientTransformers[ACTION_NOTIFY_SCAMERA_RESET] = new CScriptCameraResetTransformer;
	clientTransformers[ACTION_NOTIFY_SC_START_MOVIE] = new CScriptCameraStartMovieTransformer;
	clientTransformers[ACTION_NOTIFY_SC_STOP_MOVIE] = new CScriptCameraStopMovieTransformer;
	clientTransformers[ACTION_NOTIFY_WEATHER_CHANGED] = new CWeatherChangedTransformer;
	clientTransformers[ACTION_NOTIFY_CHANGE_VISIBILITY] = new CChangeVisibilityTransformer;
	clientTransformers[ACTION_NOTIFY_PLAY_EFFECT] = new CPlayEffectTransformer;
	clientTransformers[ACTION_NOTIFY_UPDATE_STATUS] = new CUpdateStatusTransformer;
	clientTransformers[ACTION_NOTIFY_SUPERWEAPON_CONTROL] = new CUpdateSuperWeaponControlTransform;
	clientTransformers[ACTION_NOTIFY_SUPERWEAPON_RECYCLE] = new CUpdateSuperWeaponRecycleTransform;
}

SAIBasicUpdate* CEventUpdater::CUpdateData::GetClientStruct( int nReturnTime )
{
	det_map< int, CPtr<IUpdateTransformer> >::const_iterator it = clientTransformers.find( eUpdateType );
	if ( it != clientTransformers.end() )
	{
		SAIBasicUpdate *pUpdate = it->second->Transform( this, nReturnTime );
		CAITimer::ToClientTime( &pUpdate->nUpdateTime );
		return pUpdate;
	}
	SAIActionUpdate *pUpdate = new SAIActionUpdate;
	pUpdate->nObjUniqueID = pObj && pObj->IsRefValid() ? pObj->GetUniqueId() : 0;
	pUpdate->nParam = nParam;
	pUpdate->eUpdateType = eUpdateType;
	pUpdate->nUpdateTime = nUpdateTime;
	CAITimer::ToClientTime( &pUpdate->nUpdateTime );

	return pUpdate;
}

//CEventUpdater

CEventUpdater::CEventUpdater()
{
	pendingIt = pendingSuspendableUpdates.begin();
}

void CEventUpdater::DestroyContents()
{
	CEventUpdater::~CEventUpdater();
	new(this) CEventUpdater();
}

void CEventUpdater::Init( const int nStaticMapSizeX, const int nStaticMapSizeY )
{
	CUpdateData::Init();
	pendingUpdates.clear();
	pendingSuspendableUpdates.clear();
	pendingIt = pendingSuspendableUpdates.begin();
	updatesHash.clear();
	nCounter = 0;
	visibleTiles.clear();
	nTime = CAITimer::GetSegmentTime();
	nReturnTime = CAITimer::GetGameTime();
	lastTimeUpTo = CAITimer::GetGameTime();
	nMyParty = theDipl.GetMyParty();
	bShowAreas = false;
	suspended.Clear();
	interpolatableUpdates.clear();
	suspended.SetSizes( nStaticMapSizeX / DIVIDER_CONST, nStaticMapSizeY / DIVIDER_CONST );
}

CEventUpdater::CUpdateData* CEventUpdater::CreateAnimationUpdate( CUpdatableObj *pObj, int nAnimation )
{
	const SUnitBaseRPGStats *pStats = dynamic_cast<const SUnitBaseRPGStats*>( pObj->GetStats() );
	if ( pStats == 0 )
		return 0;
	int nAnimIndex = -1;
	if ( CSoldier *pSoldier = dynamic_cast<CSoldier*>( pObj ) )
	{
		if ( nAnimation == NDb::ANIMATION_MOVE )
		{
			if ( CFormation *pFormation = pSoldier->GetFormation() )
			{
				const int nGeometry = pFormation->GetCurrentGeometry();
				const NDb::SSquadRPGStats *pSquadStats = pFormation->GetStats();
				const NDb::SSquadRPGStats::SFormation::EFormationMoveType eMoveType = pSquadStats->formations[nGeometry].etype;
				switch ( eMoveType )
				{
					case NDb::SSquadRPGStats::SFormation::SNEAK:
					case NDb::SSquadRPGStats::SFormation::DEFENSIVE:
						nAnimation = NDb::ANIMATION_CRAWL;
						break;
					case NDb::SSquadRPGStats::SFormation::OFFENSIVE:
						nAnimation = NDb::ANIMATION_MOVE;
						break;
					case NDb::SSquadRPGStats::SFormation::DEFAULT:
						nAnimation = NDb::ANIMATION_WALK;
						break;
					case NDb::SSquadRPGStats::SFormation::MOVEMENT:
						nAnimation = NDb::ANIMATION_MARCH;
						break;
					default:
						break;
				}
			}
		}
	}
	if ( !pStats->animdescs.empty() ) 
	{
		if ( pStats->animdescs.size() <= nAnimation )
			nAnimation = NDb::ANIMATION_IDLE;
		const std::vector<NDb::SAnimDesc> &anims = pStats->animdescs[nAnimation].anims;
		if ( !anims.empty() )
		{
			if ( pStats->GetTypeID() == NDb::SInfantryRPGStats::typeID )
				nAnimIndex = nAnimation;
			else
				nAnimIndex = anims[0].nFrameIndex;
		}
	}
	return new CUpdateData( nTime, nCounter, ACTION_NOTIFY_ANIMATION_CHANGED, pObj, nAnimIndex );
}

bool CEventUpdater::IsInterpolatableEvent( EActionNotify eUpdateType ) const
{
	return eUpdateType == ACTION_NOTIFY_PLACEMENT || 
		eUpdateType == ACTION_NOTIFY_TURRET_VERT_TURN || 
		eUpdateType == ACTION_NOTIFY_TURRET_HOR_TURN;
}

bool CEventUpdater::IsOneCopyEvent( EActionNotify eUpdateType ) const
{
	return eUpdateType == ACTION_NOTIFY_RPG_CHANGED || IsInterpolatableEvent( eUpdateType );
}

void CEventUpdater::AddUpdate( EFeedBack eFeedBack, int nParam, CObjectBase *pParam )
{
	SAIFeedbackUpdate *pUpdate = new SAIFeedbackUpdate;
	pUpdate->info.feedBackType = eFeedBack;
	pUpdate->info.nParam = nParam;
	pUpdate->info.pParam = pParam;
	AddUpdate( pUpdate, ACTION_NOTIFY_FEEDBACK, 0, nParam );
}

void CEventUpdater::AddUpdate( SAIBasicUpdate *_pUpdate, EActionNotify eUpdateType, CUpdatableObj *pObj, int nParam )
{
	CPtr<SAIBasicUpdate> pUpdate = _pUpdate; // чтоб не потерялся
	if ( eUpdateType == ACTION_NOTIFY_PLACEMENT )
	{
		if ( pObj && pObj->IsRefValid() )
			updatedPlacements.insert( pObj->GetUniqueId() );
	}

	bool bSuspend = ( pObj != 0 && ( IsInterpolatableEvent( eUpdateType ) || pObj->ShouldSuspendAction( eUpdateType ) ) && !pObj->IsVisible( nMyParty ) );
	
	if ( eUpdateType == ACTION_NOTIFY_PLACEMENT && bSuspend )  
		return;
	
	++nCounter;
	CUpdateData *pData = new CUpdateData( nTime, nCounter, eUpdateType, pObj, nParam );
	
	// animation processing
	static std::vector<CPtr<CUpdateData> > updatesBush(3);
	updatesBush.resize(0);

	updatesBush.push_back( pData );
	const int nAnimation = GetAnimationFromAction( eUpdateType );
	if ( nAnimation != -1 )
	{
		pObj->AnimationSet( nAnimation );
		if ( (eUpdateType & 1) == 1 )
		{
			if ( pObj->IsAlive() || IsDeathAnimation( nAnimation ) )
			{
				if ( CUpdateData *pAnimUpdate = CreateAnimationUpdate( pObj, nAnimation ) )
					updatesBush.push_back( pAnimUpdate );
			}
		}
	}
	
	// special cases - legacy (mainly for animation)
	if ( eUpdateType == ACTION_NOTIFY_CHANGE_FRAME_INDEX )
	{
		updater.AddUpdate( 0, ACTION_NOTIFY_DISSAPEAR_OBJ, pObj, 0 );
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ST_OBJ, pObj, -1 );
		updatesBush.resize(0);
		return;
	}
	else if ( eUpdateType == ACTION_NOTIFY_MECH_SHOOT )
	{
		SAIMechShotUpdate *pInternalData = new SAIMechShotUpdate;
		pObj->GetMechShotInfo( &(pInternalData->info), nTime );
		pInternalData->eUpdateType = ACTION_NOTIFY_MECH_SHOOT;
		pInternalData->nUpdateTime = nTime;
		const int nShotAnimation = GetAnimationFromAction( pInternalData->info.typeID );
		if ( nShotAnimation != -1 )
		{
			CUpdatableObj *pObj = GetObjectByUniqueIdSafe<CUpdatableObj>( pInternalData->info.nObjUniqueID );
			if ( pObj && pObj->IsRefValid() )
				pObj->AnimationSet( nShotAnimation );
//			checked_cast<CUpdatableObj*>(pInternalData->info.pObj.GetPtr())->AnimationSet( nShotAnimation );
		}
		pData->pData = pInternalData;
	}
	else if ( eUpdateType == ACTION_NOTIFY_INFANTRY_SHOOT )
	{
		SAIInfantryShotUpdate *pInternalData = new SAIInfantryShotUpdate;
		pObj->GetInfantryShotInfo( &(pInternalData->info), nTime );
		pInternalData->eUpdateType = ACTION_NOTIFY_INFANTRY_SHOOT;
		pInternalData->nUpdateTime = nTime;
		const int nShotAnimation = GetAnimationFromAction( pInternalData->info.typeID );
		if ( nShotAnimation != -1 )
		{
			CSoldier *pSoldier = GetObjectByUniqueIdSafe<CSoldier>(pInternalData->info.nObjUniqueID);
			if ( pSoldier )
			{
				pSoldier->AnimationSet( nShotAnimation );
				if ( !pSoldier->IsInSolidPlace() )
				{
					if ( CUpdateData *pAnimUpdate = CreateAnimationUpdate( pSoldier, GetAnimationFromAction( pInternalData->info.typeID ) ) )
						updatesBush.push_back( pAnimUpdate );
				}
			}
		}
		pData->pData = pInternalData;
	}
	else //for custom updates
	{
		pData->pData = pUpdate;
		if ( pUpdate != 0 )
		{
			pData->pData->eUpdateType = eUpdateType;
			pData->pData->nUpdateTime = nTime;
		}
	}
	
	if ( bSuspend )
	{
		if ( IsOneCopyEvent( eUpdateType ) )
			ClearUpdates( pObj, eUpdateType );

		//determine current update position
		CTilesSet tiles;
		pObj->GetTilesForVisibility( &tiles );
		for ( CTilesSet::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
		{
			for ( std::vector< CPtr<CUpdateData> >::iterator itUpdate = updatesBush.begin(); itUpdate != updatesBush.end(); ++itUpdate )
				InsertSuspendedUpdate( *itUpdate, *it );
		}
	}
	else
	{
		for ( std::vector< CPtr<CUpdateData> >::iterator it = updatesBush.begin(); it != updatesBush.end(); ++it )
			pendingUpdates.push_back( *it );
	}

	for ( std::vector< CPtr<CUpdateData> >::iterator it = updatesBush.begin(); it != updatesBush.end(); ++it )
		updatesHash[pObj].push_back( *it );

	updatesBush.resize(0);
}

void CEventUpdater::InsertSuspendedUpdate( CUpdateData* pUpdate, const SVector &_vPosition )
{
	const SVector vPosition( _vPosition.x / DIVIDER_CONST, _vPosition.y / DIVIDER_CONST );
	suspended[vPosition.y][vPosition.x].push_back( pUpdate );
}

void CEventUpdater::ClearUpdates( CUpdatableObj *pObj, EActionNotify eUpdateType )
{
	//delete all updates by mask (actually, mark as invalid)
	CUpdateData *pData = 0;
	if ( pObj != 0 )
	{
		CUpdateMap::iterator it = updatesHash.find( pObj );
		if ( it != updatesHash.end() )
		{
			std::list< CPtr< CUpdateData > > &lst = it->second;
			for ( std::list< CPtr< CUpdateData > >::iterator iit = lst.begin(); iit != lst.end(); )
			{
				pData = *iit;
				if ( eUpdateType == ACTION_NOTIFY_NONE || pData->eUpdateType == eUpdateType )
				{
					pData->bValid = false;			// We need this, since the update is also referenced elsewhere
					iit = lst.erase( iit );
				}
				else
					++iit;
			}
			if ( lst.empty() )
				updatesHash.erase( it );
		}
	}
	else
	{
		for ( CUpdateMap::iterator it = updatesHash.begin(); it != updatesHash.end(); )
		{
			std::list< CPtr< CUpdateData > > &lst = it->second;
			for ( std::list< CPtr< CUpdateData > >::iterator iit = lst.begin(); iit != lst.end(); )
			{
				pData = *iit;
				if ( eUpdateType == ACTION_NOTIFY_NONE || pData->eUpdateType == eUpdateType )
				{
					pData->bValid = false;			// We need this, since the update is also referenced elsewhere
					iit = lst.erase( iit );
				}
				else
					++iit;
			}
			if ( lst.empty() )
				updatesHash.erase( it++ );
			else
				++it;
		}
	}
}

CEventUpdater::CUpdateData* CEventUpdater::PopUpdate()
{
	CPtr<CUpdateData> pData = 0;
  do
	{
		CUpdateList::iterator it = pendingUpdates.begin();
		// try to get update from common queue
		if ( it != pendingUpdates.end() && ( nReturnTime == 0 || (*it)->nUpdateTime <= nReturnTime ) )
		{
			pData = *it;
			pendingUpdates.erase( it );
			if ( !pData->bValid )
				continue;
			if ( IsInterpolatableEvent( pData->eUpdateType ) )
				interpolatableUpdates.push_back( pData );
			else
				pData->bValid = false;
		}	
		// otherwise, try suspendable queue
		if ( pData == 0 && pendingIt != pendingSuspendableUpdates.end() )
		{
			pData = *pendingIt;
			if ( !pData->bValid )
			{
				pendingIt = pendingSuspendableUpdates.erase( pendingIt );
				continue;
			}
			if ( pData->nUpdateTime < nReturnTime )
				pData->bValid = false;
			++pendingIt;
		}
		
		return pData.Extract();
  } while( true );
}

void CEventUpdater::UpdateTime( const NTimer::STime &nNewTime )
{
	if ( nTime == nNewTime )
		return;
	nTime = nNewTime;
	//reset counter
	nCounter = 0;
	if ( bShowAreas )
	{
		for ( std::list< CPtr<CUpdatableObj> >::iterator it = shootGroupUnits.begin(); it != shootGroupUnits.end(); ++it )
			updater.AddUpdate( 0, eAreaType, it->GetPtr(), -1 );
	}
	shootGroupUnits.clear();
}

void CEventUpdater::TileBecameVisibleFromWarFog( const SVector &_vPos, const int nParty )
{
	const int nDivider = DIVIDER_CONST / AI_TILES_IN_VIS_TILE;
	const SVector vPos( _vPos.x / nDivider, _vPos.y / nDivider );
	if ( nParty != nMyParty )
		return;
	if ( !suspended[vPos.y][vPos.x].empty() )
		visibleTiles.insert( vPos );
}

void CEventUpdater::PumpUpdates()
{
	bool bUpdatesChanged = false;
	bool bSuspendableUpdatesChanged = false;
	CUpdateData *pUpdate;
	// find updates, that became visible
	for ( det_set<SVector, STilesHash>::const_iterator it = visibleTiles.begin(); it != visibleTiles.end(); ++it )
	{
		TUpdatesList &lst = suspended[(*it).y][(*it).x];
		for ( TUpdatesList::const_iterator lit = lst.begin(); lit != lst.end(); ++lit )
		{
			pUpdate = (*lit);

			EActionNotify eType = pUpdate->eUpdateType;
			if ( !pUpdate->bValid )
				continue;

			CUpdateList *pPendingList = &pendingUpdates;
			if ( IsInterpolatableEvent( eType ) )
			{
				pPendingList = &pendingSuspendableUpdates;
				bSuspendableUpdatesChanged = true;
			}
			else
				bUpdatesChanged = true;

			if ( pUpdate->bValid )
				pPendingList->push_back( pUpdate );
		}
		lst.clear();
	}
	
	// clear visible tiles
	visibleTiles.clear();
	
	// Sort the lists
	if ( bUpdatesChanged )
		pendingUpdates.sort( SUpdateDataLessCompare() );

	if ( bSuspendableUpdatesChanged )
		pendingSuspendableUpdates.sort( SUpdateDataLessCompare() );
}

void CEventUpdater::ClearInterpolatable()
{
	for ( CUpdateList::iterator it = interpolatableUpdates.begin(); it != interpolatableUpdates.end(); ++it )
		(*it)->bValid = false;
	interpolatableUpdates.clear();
}

void CEventUpdater::CollectUpdates( NTimer::STime nUpTo )
{
	pendingUpdates.splice( pendingUpdates.end(), interpolatableUpdates );
	
	nReturnTime = nUpTo;
	
	for ( CUpdateList::iterator it = pendingSuspendableUpdates.begin(); it != pendingSuspendableUpdates.end(); )
	{
		if ( (*it)->bValid )
			++it;
		else
			it = pendingSuspendableUpdates.erase( it );
	}
	
	PumpUpdates();
	
	pendingIt = pendingSuspendableUpdates.begin();

	if ( nUpTo - lastTimeUpTo > 1000 )
	{
		lastTimeUpTo =  nUpTo;
		for ( CUpdateMap::iterator it = updatesHash.begin(); it != updatesHash.end(); )
		{
			CUpdateList &lst = it->second;
			
			for ( CUpdateList::iterator itUpdate = lst.begin(); itUpdate != lst.end(); )
			{
				if ( (*itUpdate)->bValid )
					++itUpdate;
				else
					itUpdate = lst.erase( itUpdate );
			}
			
			if ( lst.empty() )
				updatesHash.erase( it++ );
			else
				++it;
		}
	}
}

void CEventUpdater::UpdateAreasGroup( const bool bShow, EActionNotify eType )
{
	bShowAreas = bShow;
 	eAreaType = eType;
	shootGroupUnits.clear();
	ClearUpdates( (CUpdatableObj*)0, ACTION_NOTIFY_SHOOT_AREA );
	ClearUpdates( (CUpdatableObj*)0, ACTION_NOTIFY_RANGE_AREA );
}

bool CEventUpdater::IsPlacementUpdated( CUpdatableObj *pObj ) const
{
	if ( !pObj || !pObj->IsRefValid() )
		return false;

	return updatedPlacements.find( pObj->GetUniqueId() ) != updatedPlacements.end();
}

void CEventUpdater::ClearPlacementUpdates()
{
	updatedPlacements.clear();
}

void CEventUpdater::PrepareUpdates()
{
	CollectUpdates( CAITimer::GetGameTime() );
}

SAIBasicUpdate* CEventUpdater::GetUpdate()
{
	if ( CAITimer::GetGameTime() > nReturnTime )
		CollectUpdates( CAITimer::GetGameTime() );
	
	CPtr<CUpdateData> pData = PopUpdate();
	if ( pData == 0 ) 
		return 0;
	
	return pData->GetClientStruct( nReturnTime ) ;
}

struct SUpdateInfo
{
	int nValidCount;
	int nInvalidCount;

	SUpdateInfo() : nValidCount( 0 ), nInvalidCount( 0 ) {}
};

typedef det_map<EActionNotify, SUpdateInfo, SEnumHash> TDumpUpdatesHash; // update id -> update info

typedef det_map<int, TDumpUpdatesHash> TDumpObjectsHash; // object id (-1 for invalid) -> object's updates

void CEventUpdater::DumpSizes()
{
	TDumpUpdatesHash allUpdates;
	TDumpObjectsHash objects;
	std::vector<int> objectsID;

	for ( int x = 0; x < suspended.GetSizeX(); ++x )
	{
		for ( int y = 0; y < suspended.GetSizeY(); ++y )
		{
			TUpdatesList &updates = suspended[y][x];
			for ( TUpdatesList::const_iterator it = updates.begin(); it != updates.end(); ++it )
			{
				if ( IsValid( *it ) )
				{
					const int nID = IsValid( (*it)->pObj ) ? (*it)->pObj->GetUniqueId() : -1;
					const EActionNotify eType =  (*it)->eUpdateType;
					const bool bValid = (*it)->bValid;

					TDumpObjectsHash::const_iterator pos = objects.find( nID );
					if ( pos == objects.end() )
						objectsID.push_back( nID );

					if ( bValid )
					{
						++(allUpdates[eType].nValidCount);
						++(objects[nID][eType].nValidCount);
					}
					else
					{
						++(allUpdates[eType].nInvalidCount);
						++(objects[nID][eType].nInvalidCount);
					}
				}
			}
		}
	}
	std::sort( objectsID.begin(), objectsID.end() );

	std::string str1 = "Unit";
	std::string str2 = "Total";
	for ( TDumpUpdatesHash::const_iterator itUpd = allUpdates.begin(); itUpd != allUpdates.end(); ++itUpd )
	{
		str1 = str1 + fmt::format( "\t0x{:04x}", static_cast<int>(itUpd->first) );
		str2 = str2 + fmt::format( "\t{}/{}", itUpd->second.nValidCount, itUpd->second.nInvalidCount );
	}

	DebugTrace( str1.c_str() );
	for ( std::vector<int>::const_iterator it = objectsID.begin(); it != objectsID.end(); ++it )
	{
		std::string str;
		if ( *it == -1 )
			str = "Invalid";
		else
			str = std::to_string(  *it );

		TDumpObjectsHash::const_iterator pos = objects.find( *it );
		const TDumpUpdatesHash &thisUpdates = pos->second;
		for ( TDumpUpdatesHash::const_iterator itUpd = allUpdates.begin(); itUpd != allUpdates.end(); ++itUpd )
		{
			TDumpUpdatesHash::const_iterator posUpd = thisUpdates.find( itUpd->first );
			if ( posUpd != thisUpdates.end() )
				str = str + fmt::format( "\t{}/{}", posUpd->second.nValidCount, posUpd->second.nInvalidCount );
			else
				str = str +  "\t-/-" ;
		}
		DebugTrace( str.c_str() );
	}
	DebugTrace( str2.c_str() );
}

//Serialization

int CEventUpdater::CUpdateData::operator&( IBinSaver &saver )
{
	saver.Add( 1, &nUpdateTime );
	saver.Add( 2, &nOrder );
	saver.Add( 3, &eUpdateType );
	saver.Add( 4, &nParam );
	saver.Add( 5, &bValid );
	saver.Add( 6, &pObj );
	saver.Add( 7, &pData );

	return 0;
}

int CEventUpdater::operator&( IBinSaver &saver )
{
	if ( !saver.IsChecksum() )
	{
		saver.Add( 1, &suspended );
		saver.Add( 2, &pendingUpdates );
		saver.Add( 3, &pendingSuspendableUpdates );
		if ( saver.IsReading() )
		{
			CPtr<CUpdateData> pCurrPendingIt = 0;
			saver.Add( 4, &pCurrPendingIt );
			for ( pendingIt = pendingSuspendableUpdates.begin(); pendingIt != pendingSuspendableUpdates.end(); ++pendingIt )
			{
				if ( pCurrPendingIt == *pendingIt )
					break;
			}
			CUpdateData::Init();
		}
		else
		{
			CPtr<CUpdateData> pCurrPendingIt = pendingIt == pendingSuspendableUpdates.end() ? 0 : *pendingIt;
			saver.Add( 4, &pCurrPendingIt );
		}
		
		saver.Add( 5, &updatesHash );
		saver.Add( 7, &nTime );
		saver.Add( 8, &nReturnTime );
		saver.Add( 9, &nCounter );
		saver.Add( 10, &nMyParty );
		saver.Add( 11, &visibleTiles );

		if ( saver.IsReading() )
		{
			pendingIt = pendingSuspendableUpdates.begin();
		}

		saver.Add( 13, &updatedPlacements );
		saver.Add( 14, &shootGroupUnits );
		saver.Add( 15, &eAreaType );
		saver.Add( 16, &interpolatableUpdates );
		saver.Add( 17, &lastTimeUpTo );
		saver.Add( 18, &bShowAreas );
	}

	/*
	if ( saver.IsReading() )
		DumpSizes();
	*/

	return 0;
}


