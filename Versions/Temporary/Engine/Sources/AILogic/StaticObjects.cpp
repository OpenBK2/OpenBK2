#include "stdafx.h"

#include "StaticObjects.h"
#include "StaticObjectsIters.h"
#include "Building.h"
#include "Entrenchment.h"
#include "NewUpdater.h"
#include "Bridge.h"
#include "Mine.h"
#include "GlobalWarFog.h"
#include "Diplomacy.h"
#include "AIUnit.h"
#include "Fence.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/PathFinder.h"
#include "SmokeScreen.h"
#include "ObstacleINternal.h"
#include "Graveyard.h"
#include "Cheats.h"

#include "Common_RTS_AI/StaticMapHeights.h"
#include "ExecutorStaticObjectSegment.h"
#include "KeyBuildingBonusSystem.h"

extern CKeyBuildingBonusSystem theBonusSystem;
extern CBridgeHeightRemover theBridgeHeightsRemover;
CStaticObjects theStatObjs;
extern CEventUpdater updater;
extern CGlobalWarFog theWarFog;
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CGraveyard theGraveyard;
extern SCheats theCheats;

//*******************************************************************
//*									CStaticObjects::SSegmentObjectsSort							*
//*******************************************************************

bool CStaticObjects::SSegmentObjectsSort::operator()( const CPtr<CStaticObject> &segmObj1, const CPtr<CStaticObject> &segmObj2 ) const
{
	bool res = 
					segmObj1->GetNextSegmentTime() < segmObj2->GetNextSegmentTime() ||
				 segmObj1->GetNextSegmentTime() == segmObj2->GetNextSegmentTime() &&
				 segmObj1->GetUniqueId() < segmObj2->GetUniqueId();
	return res;
}

//*******************************************************************
//*									CStaticObjects::CStoragesContainer							*
//*******************************************************************

CStaticObjects::CStoragesContainer::CStoragesContainer()
{
}

void CStaticObjects::CStoragesContainer::StorageChangedDiplomacy( class CBuilding *pNewStorage, const int nNewPlayer )
{
	RemoveStorage( pNewStorage );
	AddStorage( pNewStorage, nNewPlayer, pNewStorage->GetLink() );
}

void CStaticObjects::CStoragesContainer::AddStorage( class CBuilding *pNewStorage, const int nPlayer, int nLinkID )
{
	const int nParty = theDipl.GetNParty( nPlayer );
	if ( nParty >= 2 ) 
		return;
	const SBuildingRPGStats *pStats = static_cast<const SBuildingRPGStats *>(pNewStorage->GetStats());
	if ( pStats->etype ==  TYPE_MAIN_RU_STORAGE || pStats->etype ==  TYPE_TEMP_RU_STORAGE ||
		theBonusSystem.IsStorage( nLinkID ) )
	{
		storages[pNewStorage->GetUniqueId()] = pNewStorage;
	}		
}

void CStaticObjects::CStoragesContainer::RemoveStorage( class CBuilding *pNewStorage )
{
	const int nID = pNewStorage->GetUniqueId();
	if ( storages.find( nID ) != storages.end() ) // this static object is storage
	{
		const int nParty = theDipl.GetNParty(pNewStorage->GetPlayer());
		if ( nParty >= 2 ) 
			return;
		storages.erase( nID );
	}
}

void CStaticObjects::CStoragesContainer::Clear()
{
	storages.clear();
}

void CStaticObjects::CStoragesContainer::EnumStoragesForParty( const int nParty, CStaticObjects::IEnumStoragesPredicate * pPred )
{
	for ( CStorages::iterator i = storages.begin(); i != storages.end(); ++i )
	{
		CBuilding *pStor = i->second;
		if ( theDipl.GetNParty(pStor->GetPlayer()) == nParty && pStor->IsAlive() )
			pPred->AddStorage( pStor, 0 );
	}
}

void CStaticObjects::CStoragesContainer::EnumStoragesInRange( const CVec2 &vCenter, 
																					 const int nParty, 
																					 const float fMaxPathLenght,
																					 const float fMaxOffset,
  																				 CCommonUnit *pUnitToFindPath, 
																					 CStaticObjects::IEnumStoragesPredicate *pPred )
{
	if ( nParty >= 2 ) 
		return;
	
	for ( CStorages::iterator it = storages.begin(); it != storages.end(); ++it )
	{
		CBuilding *pStor = it->second;
		if ( theDipl.GetNParty(pStor->GetPlayer()) == nParty && pStor->IsAlive() )
		{
			const CVec2 vStorageCenter( pStor->GetAttackCenter( pUnitToFindPath->GetCenterPlain() ) );
			CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vStorageCenter, VNULL2, pUnitToFindPath, true, GetAIMap() );
			if ( pPath &&
						fabs2( vStorageCenter - pPath->GetFinishPoint() ) <= sqr( fMaxOffset ) &&
						pPred->AddStorage( pStor, pPath->GetLength() ) )
			{
				return;
			}
		}
	}
}

//*******************************************************************
//*												  CStaticObjects													*
//*******************************************************************

void CStaticObjects::StorageChangedDiplomacy( class CBuilding *pNewStorage, const int nNewPlayer )
{
	storagesContainer.StorageChangedDiplomacy( pNewStorage, nNewPlayer );
}

void CStaticObjects::Init( const int nMapTileSizeX, const int nMapTileSizeY )
{
	areaMap.SetSizes( nMapTileSizeX / SConsts::STATIC_OBJ_CELL, nMapTileSizeY / SConsts::STATIC_OBJ_CELL );
	containersAreaMap.SetSizes( nMapTileSizeX / SConsts::STATIC_CONTAINER_OBJ_CELL, nMapTileSizeY / SConsts::STATIC_CONTAINER_OBJ_CELL );
	obstacles.SetSizes( nMapTileSizeX / SConsts::STATIC_OBJ_CELL, nMapTileSizeY / SConsts::STATIC_OBJ_CELL );
	nObjs = 0;
	
	storagesContainer.Init();
}

void CStaticObjects::EnumObstaclesInRange(  const CVec2 &vCenter,
																						const float fR,
																						struct IObstacleEnumerator *f )
{
	const int nMinX = (std::max)( 0, int( ( vCenter.x - fR ) / ( SConsts::TILE_SIZE * SConsts::STATIC_OBJ_CELL ) ) );
	const int nMaxX = (std::min)( obstacles.GetSizeX() - 1, int( ( vCenter.x + fR ) / float( SConsts::TILE_SIZE * SConsts::STATIC_OBJ_CELL ) ) );
	const int nMinY = (std::max)( 0, int( ( vCenter.y - fR ) / ( SConsts::TILE_SIZE * SConsts::STATIC_OBJ_CELL ) ) );

	const int nMaxY = (std::min)( obstacles.GetSizeY() - 1, int( ( vCenter.y + fR ) / float( SConsts::TILE_SIZE * SConsts::STATIC_OBJ_CELL ) ) );
	const float fR2 = sqr( fR );

	for ( int x = nMinX; x <= nMaxX; ++x )
	{
		for ( int y = nMinY; y <= nMaxY; ++y )
		{
			for ( ObstacleAreaMap::CDataList::iterator it = obstacles[x][y].begin(); it != obstacles[x][y].end(); ++it )
			{

				if ( (*it)->IsAlive() && fabs2( vCenter - CVec2((*it)->GetCenter().x,(*it)->GetCenter().y) ) < fR2 )
					f->AddObstacle( *it );
			}
		}
	}
}

void CStaticObjects::EnumStoragesForParty( const int nParty, CStaticObjects::IEnumStoragesPredicate * pPred )
{
	storagesContainer.EnumStoragesForParty( nParty, pPred );
}

void CStaticObjects::EnumStoragesInRange( const CVec2 &vCenter, 
																					 const int nParty, 
																					 const float fMaxPathLenght,
																					 const float fMaxOffset,
  																				 class CCommonUnit * pUnitToFindPath, 
																					 CStaticObjects::IEnumStoragesPredicate * pPred )
{
	storagesContainer.EnumStoragesInRange( vCenter, nParty, fMaxPathLenght, fMaxOffset, pUnitToFindPath, pPred );
}

void CStaticObjects::DeleteInternalObjectInfo( CExistingObject *pObj )
{
	deletedObjects[pObj->GetUniqueId()] = pObj;
}

void CStaticObjects::DeleteInternalEntrenchmentInfo( CEntrenchment *pEntrench )
{
	CObjectBase *pObj = pEntrench;
	std::list< CObj<CObjectBase> > ::iterator iter = entrenchments.begin();
	while ( iter != entrenchments.end() && (*iter) != pObj )
		++iter;

	NI_ASSERT( iter != entrenchments.end(), "Wrong object to delete" );

	entrenchments.erase( iter );
}

CStaticObject* CStaticObjects::AddNewFenceObject( const SFenceRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD wDir, const int nDiplomacy, const int nFrameIndex )
{
	CFence *pFence = new CFence( pStats, center, pStats->fMaxHP * fHPFactor, wDir, nDiplomacy, /* nFrameIndex */ 0 ); //no nFrameIndex here, because stats don't have apropriate information
	pFence->Init();
	
	AddStaticObject( pFence, false );
	pFence->SetTransparencies();
	
	return pFence;
}

CStaticObject* CStaticObjects::AddNewSmokeScreen( const CVec3 &vCenter, const float fR, const int nTransparency, const int nTime )
{
	CSmokeScreen *pObj = new CSmokeScreen( vCenter, fR, nTransparency, nTime );
	pObj->Mem2UniqueIdObjs();
	pObj->Init();

	pObj->SetTransparencies();
	
	AddToAreaMap( pObj );

	return pObj;
}

CExistingObject* CStaticObjects::AddNewTankPit( const SMechUnitRPGStats *pStats, const CVec3 &center, const WORD dir, const int nFrameIndex, const class CVec2 &vHalfSize, const std::list<SObjTileInfo> &tilesToLock, class CAIUnit *pOwner )
{
	CEntrenchmentTankPit *pObj = new CEntrenchmentTankPit( pStats, center, dir, nFrameIndex, vHalfSize, tilesToLock, pOwner );
	pObj->Mem2UniqueIdObjs();
	pObj->LockTiles();
	AddToAreaMap( pObj );

	return pObj;
}

void CStaticObjects::AddStaticObject( class CExistingObject* pObj, bool bAlreadyLocked )
{
	if ( !IsValid( pObj ) )
		return;

	pObj->Mem2UniqueIdObjs();
	if ( !bAlreadyLocked )
		pObj->LockTiles();
	pObj->SetTransparencies();
	AddToAreaMap( pObj );

	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ST_OBJ, pObj, -1 );
}

CStaticObject* CStaticObjects::AddNewStaticObject( const SObjectBaseRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD wDir, const int nFrameIndex )
{
	CCommonStaticObject *pObj = new CSimpleStaticObject( pStats, center, wDir, pStats->fMaxHP * fHPFactor, nFrameIndex, ESOT_COMMON );
	pObj->Mem2UniqueIdObjs();
	pObj->Init();

	AddStaticObject( pObj, false );
	
	return pObj;
}

CStaticObject* CStaticObjects::AddNewTerraObj( const SObjectBaseRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD wDir, const int nFrameIndex )
{
	CCommonStaticObject *pObj = new CSimpleStaticObject( pStats, center, wDir, pStats->fMaxHP * fHPFactor, nFrameIndex, ESOT_TERRA );
	pObj->Mem2UniqueIdObjs();
	pObj->Init();
	
	pObj->LockTiles();
	pObj->SetTransparencies();
	terraObjs[pObj->GetUniqueId()] = pObj;

	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ST_OBJ, pObj, -1 );
	updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, pObj, -1 );
	return pObj;
}

CStaticObject* CStaticObjects::AddNewTerraMeshObj( const SObjectBaseRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD wDir, const int nFrameIndex )
{
	CCommonStaticObject *pObj = new CTerraMeshStaticObject( pStats, center, wDir, pStats->fMaxHP * fHPFactor, nFrameIndex, ESOT_TERRA );
	pObj->Mem2UniqueIdObjs();
	pObj->Init();
	
	pObj->LockTiles();
	pObj->SetTransparencies();
	terraObjs[pObj->GetUniqueId()] = pObj;

	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ST_OBJ, pObj, -1 );
	updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, pObj, -1 );
	return pObj;
}

void CStaticObjects::AddStorage( CBuilding *pObj )
{
	pObj->LockTiles();
	pObj->SetTransparencies();

	AddToAreaMap( pObj );
	storagesContainer.AddStorage( pObj, pObj->GetPlayer(), pObj->GetLink() );
	//storagesContainer2.AddStorage( pObj );

	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ST_OBJ, pObj, -1 );
	//updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, pObj, -1 );
}

CStaticObject* CStaticObjects::AddNewBuilding( const SBuildingRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD wDir, const int nFrameIndex, int nPlayer, int nLinkID )
{
	CBuildingSimple *pObj = new CBuildingSimple( pStats, center, wDir, pStats->fMaxHP * fHPFactor, nFrameIndex, nPlayer, nLinkID );
	pObj->Mem2UniqueIdObjs();
	pObj->Init();
	
	pObj->LockTiles();
	pObj->SetTransparencies();

	AddToAreaMap( pObj );

	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ST_OBJ, pObj, -1 );
	updater.AddUpdate( 0, ACTION_NOTIFY_UPDATE_DIPLOMACY, pObj, -1 );
	storagesContainer.AddStorage( pObj, nPlayer, nLinkID );

	pObj->CheckHitPoints();
	return pObj;
}

CStaticObject* CStaticObjects::AddNewBridgeSpan( const SBridgeRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD dir, const int nFrameIndex )
{
	const CVec3 vCenter( center.x, center.y, center.z + GetHeights()->GetVisZ( center.x, center.y ) );
	CBridgeSpan *pObj = new CBridgeSpan( pStats, vCenter, pStats->fMaxHP * fHPFactor, dir, nFrameIndex );
	pObj->Mem2UniqueIdObjs();
	pObj->Init();
	
	pObj->LockTiles();
	pObj->SetTransparencies();
	AddToAreaMap( pObj );
	bridges.push_back( pObj );

	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_BRIDGE_SPAN, pObj, -1 );
	//updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, pObj, -1 );
	return pObj;
}

void CStaticObjects::PostAllObjectsInit()
{
	theBridgeHeightsRemover.Clear();
	for ( std::list<CPtr<CBridgeSpan> >::iterator it = bridges.begin(); it != bridges.end(); ++it )
	{
		(*it)->SetHeights();
	}

	/*
	list<CPtr<CExistingObject> > nonZeroHeight;
	// init objects, that are on top of other objects
	for ( CStObjIter<false> iter; iter.IsFinished(); iter.Iterate() )
	{
		if ( (*iter)->GetCenter().z != 0 )
			nonZeroHeight.push_back(*iter);
	}*/
	// iterate 1 more time, look
}

void CStaticObjects::RemoveObstacle( struct IObstacle *pObstacle )
{
	obstacleObjects.erase( pObstacle->GetObject()->GetUniqueId() );
	obstacles.RemoveFromPosition( pObstacle, AICellsTiles::GetTile( CVec2(pObstacle->GetCenter().x,pObstacle->GetCenter().y) ) );
}

void CStaticObjects::AddObstacle( struct IObstacle *pObstacle )
{
	obstacleObjects.insert( std::pair< int, CPtr<IObstacle> >(pObstacle->GetObject()->GetUniqueId(), pObstacle ) );
	obstacles.AddToPosition( pObstacle, AICellsTiles::GetTile( CVec2(pObstacle->GetCenter().x,pObstacle->GetCenter().y) ) );
}

void CStaticObjects::AddEntrencmentPart(  class CEntrenchmentPart *pObj, bool bLockedAlready )
{
	pObj->Mem2UniqueIdObjs();
	if ( !bLockedAlready )
		pObj->LockTiles();
	pObj->SetTransparencies();

	AddToAreaMap( pObj );
}

CStaticObject* CStaticObjects::AddNewEntrencmentPart( const SEntrenchmentRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD dir, const int nFrameIndex, int nPlayer, bool bPlayerCreates )
{
	CEntrenchmentPart *pObj = new CEntrenchmentPart( pStats, center, dir, nFrameIndex, pStats->fMaxHP * fHPFactor, nPlayer, bPlayerCreates );
	pObj->Mem2UniqueIdObjs();
	pObj->Init();

	AddEntrencmentPart( pObj, false );

	return pObj;
}

CStaticObject* CStaticObjects::AddNewEntrencment( CObjectBase** segments, const int nLen, class CFullEntrenchment *pFullEntrenchment, const bool bPiecewise )
{
	CEntrenchment *pEntrench = new CEntrenchment( segments, nLen, pFullEntrenchment, bPiecewise );
	pEntrench->Mem2UniqueIdObjs();

	entrenchments.push_back( pEntrench );

	return pEntrench;
}

CMineStaticObject* CStaticObjects::AddNewMine( const SMineRPGStats *pStats, const float fHPFactor, const CVec3 &center, const int nFrameIndex, const int player )
{
	CMineStaticObject *pObj = new CMineStaticObject( pStats, center, pStats->fMaxHP * fHPFactor, nFrameIndex, player );
	pObj->Mem2UniqueIdObjs();
	pObj->Init();

	pObj->LockTiles();
	pObj->SetTransparencies();

	pObj->ClearVisibleStatus();

	AddToAreaMap( pObj );
	
	// ставим только наши мины
	if ( theDipl.GetDiplStatus( theDipl.GetMyNumber(), player ) == EDI_FRIEND )
		pObj->RegisterInWorld();

	return pObj;
}

void CStaticObjects::AddObjectToAreaMapTile( CExistingObject *pObj, const SVector &tile )
{
	if ( GetAIMap()->IsTileInside( tile ) )
	{
		areaMap.AddToPosition( pObj, tile );
		if ( pObj->IsContainer() )
			containersAreaMap.AddToPosition( pObj, tile );
	}
}

void CStaticObjects::AddToAreaMap( CExistingObject *pObj )
{
	std::list<SVector> tiles;
	pObj->GetCoveredTiles( &tiles );
	
	// чтобы не удалился после update
	if ( tiles.empty() )
		AddObjectToAreaMapTile( pObj, AICellsTiles::GetTile( CVec2(pObj->GetCenter().x,pObj->GetCenter().y) ) );
	else
	{
		for ( std::list<SVector>::iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
			AddObjectToAreaMapTile( pObj, *iter );
	}

	++nObjs;
}

void CStaticObjects::RemoveObjectFromAreaMapTile( CExistingObject *pObj, const SVector &tile )
{
	if ( GetAIMap()->IsTileInside( tile ) )
	{
		areaMap.RemoveFromPosition( pObj, tile );
		if ( pObj->IsContainer() )
			containersAreaMap.RemoveFromPosition( pObj, tile );
	}
}

void CStaticObjects::RemoveFromAreaMap( CExistingObject *pObj )
{
	std::list<SVector> tiles;
	pObj->GetCoveredTiles( &tiles );

	if ( tiles.empty() )
		RemoveObjectFromAreaMapTile( pObj, AICellsTiles::GetTile( CVec2(pObj->GetCenter().x,pObj->GetCenter().y) ) );
	else
	{
		for ( std::list<SVector>::iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
			RemoveObjectFromAreaMapTile( pObj, *iter );
	}

	--nObjs;
	NI_ASSERT( nObjs >= 0, "Wrong number of static objects" );
}

void CStaticObjects::RegisterSegment( class CStaticObject *pObj )
{
	//segmObjects.insert( pObj );
	pObj->SetTerminateExecutorFlag( false );
	pTheExecutorsContainer->Add( new CExecutorStaticObjectSegment( pObj ) );
}

void CStaticObjects::UnregisterSegment( class CStaticObject *pObj )
{
	pObj->SetTerminateExecutorFlag( true );
}

void CStaticObjects::Segment()
{
	for ( CObjectsHashSet::iterator iter = deletedObjects.begin(); iter != deletedObjects.end(); ++iter )
	{
		CExistingObject *pObj = iter->second;
		UnregisterSegment( pObj );

		if ( !pObj->HasFallen() && !pObj->IsTrampled() )
			updater.AddUpdate( 0, ACTION_NOTIFY_DELETED_ST_OBJ, pObj, -1 );

		storagesContainer.RemoveStorage( static_cast<CBuilding*>(pObj) );
		
		if ( obstacleObjects.end() != obstacleObjects.find( pObj->GetUniqueId() ) )
			RemoveObstacle( obstacleObjects[pObj->GetUniqueId()] );

		pObj->UnlockTiles();

		pObj->RemoveTransparencies();
		if ( pObj->GetObjectType() != ESOT_TERRA )
			RemoveFromAreaMap( pObj );
		else
			terraObjs.erase( pObj->GetUniqueId() );
	}

	deletedObjects.clear();

	// горящие объекты
	std::list<int> burningList;
	for ( std::unordered_set<int>::const_iterator iter = burningObjects.begin(); iter != burningObjects.end(); ++iter )
		burningList.push_back( *iter );

	for ( std::list<int>::iterator iter = burningList.begin(); iter != burningList.end(); ++iter )
	{
		CExistingObject *pObj = GetObjectByUniqueIdSafe<CExistingObject>( *iter );
		if ( !pObj || !pObj->IsRefValid() || !pObj->IsAlive() )
			burningObjects.erase( *iter );
		else
			pObj->BurnSegment();
	}
}

void CStaticObjects::StartBurning( CExistingObject *pObj )
{
	burningObjects.insert( pObj->GetUniqueId() );
}

void CStaticObjects::EndBurning( CExistingObject *pObj )
{
	burningObjects.erase( pObj->GetUniqueId() );
}

void CStaticObjects::UpdateAllObjectsPos()
{
	for ( CObjectsHashSet::iterator iter = terraObjs.begin(); iter != terraObjs.end(); ++iter )
	{
		CExistingObject *pObj = iter->second;
		if ( IsValidObj( pObj ) )
		{
			pObj->SetNewPlacement( pObj->GetCenter(), pObj->GetDir() );
			pObj->RestoreTransparenciesImmidiately();
		}
	}

	std::unordered_map<int, CPtr<CFullBridge> > fullBridges;
	for ( CStObjGlobalIter<false> iter; !iter.IsFinished(); iter.Iterate() )
	{
		CExistingObject *pObj = *iter;
		if ( IsValid( pObj ) )
		{
			pObj->SetNewPlacement( pObj->GetCenter(), pObj->GetDir() );
			pObj->RestoreTransparenciesImmidiately();
			CDynamicCast<CBridgeSpan> pSpan = pObj;
			if ( pSpan )
			{
				CFullBridge *pFullBridge = pSpan->GetFullBridge();
				if ( IsValid( pFullBridge ) )
				{
					if ( fullBridges.find( pFullBridge->GetUniqueId() ) == fullBridges.end() )
						fullBridges[pFullBridge->GetUniqueId()] = pFullBridge;
				}
			}
		}
	}

	for ( std::unordered_map<int, CPtr<CFullBridge> >::iterator it = fullBridges.begin(); it != fullBridges.end(); ++it )
		it->second->InitEntireBridge();
	
}


