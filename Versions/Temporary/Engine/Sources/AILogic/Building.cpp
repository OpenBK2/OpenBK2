#include "stdafx.h"

#include "Building.h"
#include "Soldier.h"
#include "NewUpdater.h"
#include "StaticObjects.h"
#include "UnitsIterators2.h"
#include "MountedGun.h"
#include "UnitGuns.h"
#include "Turret.h"
#include "Cheats.h"
#include "Statistics.h"
#include "UnitStates.h"
#include "Formation.h"
#include "GroupLogic.h"
#include "Scripts.h"
#include "AIGeometry.h"
#include "ObjectProfile.h"
#include "../Common_RTS_AI/PathFinder.h"
#include "KeyBuildingBonusSystem.h"
#include "../DebugTools/DebugInfoManager.h"
#include "UnitCreation.h"
#include "..\System\Commands.h"
#include "FeedbackSystem.h"
#include "ScenarioTracker.h"
#include "GlobalWarFog.h"
#include "../Stats_B2_M1/StatusUpdates.h"

REGISTER_SAVELOAD_CLASS( 0x1108D450, CBuildingSimple );

extern CFeedBackSystem theFeedBackSystem;
extern CKeyBuildingBonusSystem theBonusSystem;
extern CEventUpdater updater;
extern NTimer::STime curTime;
extern CStaticObjects theStatObjs;
extern CDiplomacy theDipl;
extern CGlobalWarFog theWarFog;
extern SCheats theCheats;
extern CStatistics theStatistics;
extern CScripts *pScripts;
extern CGroupLogic theGroupLogic;
extern CUnitCreation theUnitCreation;

bool g_bNewLock;
float g_fKeyPointDamageThreshold = 0.95f;
START_REGISTER(BuildingConsts)
	REGISTER_VAR_EX( "new_lock", NGlobal::VarBoolHandler, &g_bNewLock, false, STORAGE_NONE );
	REGISTER_VAR_EX( "key_point_damage_threshold", NGlobal::VarFloatHandler, &g_fKeyPointDamageThreshold, 0.95f, STORAGE_NONE );
FINISH_REGISTER

//DEBUG{
//DEBUG}
//extern CBonusSystem theBonusSystem;

//*******************************************************************
//*												  CBuildingSimple 												*
//*******************************************************************

void CBuildingSimple::AddSoldier( CSoldier *pUnit )
{
	CBuilding::AddSoldier( pUnit );

	if ( GetScenarioTracker()->GetGameType() != IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
	 	ChangePlayer( pUnit->GetPlayer() );
}

void CBuildingSimple::ChangePlayer( const int _nPlayer )
{
	if ( nPlayer != _nPlayer ) 
	{
		const int nOldParty = theDipl.GetNParty( nPlayer );
		const int nNewParty = theDipl.GetNParty( _nPlayer );

		if ( theBonusSystem.IsKeyBuilding( nLinkID ) && nOldParty != nNewParty )
		{
			SAIKeyBuildingUpdate *pUpdate = new SAIKeyBuildingUpdate;
			pUpdate->info.nObjUniqueID = GetUniqueId();
			pUpdate->info.nPrevPlayer = nPlayer;
			pUpdate->info.nPlayer = _nPlayer;
			pUpdate->info.bStorage = theBonusSystem.IsStorage( nLinkID );
			pUpdate->info.bFriendLost = ( nOldParty == theDipl.GetMyParty() && nNewParty != theDipl.GetMyParty() );
			if ( nNewParty == theDipl.GetMyParty() )				// We captured it!
			{
				updater.AddUpdate( pUpdate, ACTION_NOTIFY_KEY_BUILDING_CAPTURED, this, 0 );
			}
			else if ( nNewParty != theDipl.GetMyParty() )		// We lost it...
			{
				updater.AddUpdate( pUpdate, ACTION_NOTIFY_KEY_BUILDING_LOST, this, 0 );
			}
			
			// SetPartyFlag
			// upon change player change party flag according to side info
			RaisePlayerFlag( _nPlayer );
		}

		theBonusSystem.ChangeOwnership( nPlayer, _nPlayer, nLinkID, false );

		const bool bOldSelectable = IsSelectable();
		nPlayer = _nPlayer;
		const bool bSelectable = IsSelectable();

		theStatObjs.StorageChangedDiplomacy( this, _nPlayer );

		if ( bOldSelectable != bSelectable )
			updater.AddUpdate( 0, ACTION_NOTIFY_SELECTABLE_CHANGED, this, IsSelectable() );

		nSideToCapture = -1;
		timeToChangeOwner = 0;

		updater.AddUpdate( 0, ACTION_NOTIFY_UPDATE_DIPLOMACY, this, -1 );	
	}
}

void CBuildingSimple::RaisePlayerFlag( int nNewPlayer )
{
	if ( pKeyBuildingFlag )
	{
		theStatObjs.DeleteInternalObjectInfo( pKeyBuildingFlag );
		updater.AddUpdate( 0, ACTION_NOTIFY_DELETED_ST_OBJ, pKeyBuildingFlag, -1 );
		pKeyBuildingFlag = 0;
	}
	pKeyBuildingFlag = theUnitCreation.CreatePlayerFlag( nNewPlayer , pNeutralKeyBuildingFlag );
	if ( !pKeyBuildingFlag && pNeutralKeyBuildingFlag ) // substitute with neutral
	{
		pKeyBuildingFlag = pNeutralKeyBuildingFlag;
		theStatObjs.AddStaticObject( pKeyBuildingFlag, false );
	}
}

void CBuildingSimple::SetPartyFlag( CExistingObject * _pKeyBuildingFlag )
{ 
	pNeutralKeyBuildingFlag = _pKeyBuildingFlag; 
	pKeyBuildingFlag = pNeutralKeyBuildingFlag;
	if ( GetPlayer() != theDipl.GetNeutralPlayer() )
		RaisePlayerFlag( GetPlayer() );
}

CBuildingSimple::CBuildingSimple( const SBuildingRPGStats *pStats, const CVec3 &center, const WORD wDir, const float fHP, const int nFrameIndex, int nPlayerIndex, int _nLinkID )
: CBuilding( pStats, center, wDir, fHP, nFrameIndex, _nLinkID ), nPlayer( nPlayerIndex ), nSideToCapture(-1), timeToChangeOwner(0), 
timeToChangeOwnerTotal(0)
{
	SetAlive( fHP > 0.0f );

//DEBUG{
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "show_building_entrances", 0 ) )
	{
		CSegment segm;
		for ( int i = 0; i < pStats->entrances.size() -1; ++i )
		{
			const CVec2 vPoint( GetEntrancePoint( i ) );
			segm.p1 = CVec2( vPoint.x + 10, vPoint.y + 10 );
			segm.p2 = CVec2( vPoint.x - 10, vPoint.y - 10 );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 1, NDebugInfo::WHITE );
			segm.p1 = CVec2( vPoint.x - 10, vPoint.y + 10 );
			segm.p2 = CVec2( vPoint.x + 10, vPoint.y - 10 );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 1, NDebugInfo::WHITE );
		}
	}
#endif
//DEBUG}

	if ( GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL && theBonusSystem.IsKeyBuilding( _nLinkID ) )
	{
		if ( !theDipl.IsPlayerExist( nPlayer ) )
		{
			theBonusSystem.ChangeOwnership( nPlayer, theDipl.GetNeutralPlayer(), _nLinkID, true );
			nPlayer = theDipl.GetNeutralPlayer();
			theStatObjs.StorageChangedDiplomacy( this, theDipl.GetNeutralPlayer() );
		}
		else
		{
			theBonusSystem.ChangeOwnership( theDipl.GetNeutralPlayer(), nPlayer, _nLinkID, true );
			theStatObjs.StorageChangedDiplomacy( this, nPlayer );
		}

		if ( theDipl.GetNParty( GetPlayer() ) == theDipl.GetNeutralParty() )
		{
			// Raze building
			SetHitPoints( GetMinHP() );
		}
		else
		{
			// Owned building
		}
	}
	else if ( theBonusSystem.IsKeyBuilding( _nLinkID ) && theDipl.GetNParty( GetPlayer() ) != theDipl.GetNeutralParty() )
	{
		theBonusSystem.ChangeOwnership( theDipl.GetNeutralPlayer(), nPlayer, _nLinkID, true );
		theStatObjs.StorageChangedDiplomacy( this, nPlayer );
	}
}

bool CBuildingSimple::ShouldSuspendAction( const EActionNotify &eAction ) const
{
	if ( theBonusSystem.IsKeyBuilding( nLinkID ) && eAction == ACTION_NOTIFY_UPDATE_DIPLOMACY )
		return false;
	else
		return CBuilding::ShouldSuspendAction( eAction );
}

const BYTE CBuildingSimple::GetPlayer() const
{
	if ( theBonusSystem.IsKeyBuilding( nLinkID ) )
		return nPlayer;
	return CBuilding::GetPlayer();
}

void CBuildingSimple::Segment()
{
	CBuilding::Segment();

	if ( theBonusSystem.IsKeyBuilding( nLinkID ) )
	{
		int nNewPlayer = GetPlayer();
		bool bShouldCount = true;

		if ( GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
		{
			if ( GetHitPoints() < 0.1f * GetStats()->fMaxHP && GetPlayer() != theDipl.GetNeutralPlayer() )
			{
				nNewPlayer = theDipl.GetNeutralPlayer();
				ChangePlayer( nNewPlayer );		// Change instantly
				bShouldCount = false;
			}
		}

		// neutral building can be taken by the first unit
		if ( bShouldCount )
		{
			if ( theDipl.GetNParty( GetPlayer() ) == theDipl.GetNeutralParty() )
			{
				if ( GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL &&
					GetHitPoints() < GetStats()->fMaxHP * 0.9f )
				{
					// Do nothing if in this game mode a neutral key building has less than 90% HP
				}
				else
				{
					for ( CUnitsIter<0,3> iter( theDipl.GetNParty(nPlayer), ANY_PARTY, CVec2(GetCenter().x,GetCenter().y), SConsts::RADIUS_TO_TAKE_STORAGE_OWNERSHIP );
						!iter.IsFinished(); iter.Iterate() )
					{
						CPtr<CAIUnit> curUnit = *iter;
						const int nUnitPlayer = curUnit->GetPlayer();
						if ( curUnit->IsAlive() && !curUnit->GetStats()->IsAviation() && nUnitPlayer != nPlayer && theDipl.GetNParty( nUnitPlayer ) != theDipl.GetNeutralParty() )
						{
							nNewPlayer = nUnitPlayer;
							//ChangePlayer( nNewPlayer );		// Change instantly
							break;
						}
					}
				}
			}
			else
			{
				// enemy present
				if ( theDipl.GetDiplStatus( CBuilding::GetPlayer(), nPlayer ) == EDI_ENEMY )
				{
					nNewPlayer = CBuilding::GetPlayer(); 
				}
				else
				{
					for ( CUnitsIter<0,3> iter( theDipl.GetNParty(nPlayer), EDI_ENEMY, CVec2(GetCenter().x, GetCenter().y), SConsts::RADIUS_TO_TAKE_STORAGE_OWNERSHIP );
						!iter.IsFinished(); iter.Iterate() )
					{
						CPtr<CAIUnit> curUnit = *iter;
						if ( curUnit->IsAlive() && curUnit->GetPlayer() != theDipl.GetNeutralPlayer() && !curUnit->GetStats()->IsAviation() )
						{
							nNewPlayer = curUnit->GetPlayer();
							break;
						}
					}
				}
				if ( nNewPlayer != nPlayer && theDipl.GetNParty( nNewPlayer ) != theDipl.GetNParty( nPlayer ) && GetPlayer() == theDipl.GetMyNumber() )
					theFeedBackSystem.AddFeedbackAndForget( nUniqueID, CVec2( GetCenter().x, GetCenter().y ), EFB_KEYBUILDING_ATTACKED, -1 );

				// scan for friendly units
				if ( theDipl.GetDiplStatus( CBuilding::GetPlayer(), nPlayer ) == EDI_FRIEND )
				{
					nNewPlayer = nPlayer; // don't change player, defenders are here
				}
				else
				{
					for ( CUnitsIter<0,3> iter( theDipl.GetNParty(nPlayer), EDI_FRIEND, CVec2(GetCenter().x, GetCenter().y), SConsts::RADIUS_TO_TAKE_STORAGE_OWNERSHIP );
						!iter.IsFinished(); iter.Iterate() )
					{
						CPtr<CAIUnit> curUnit = *iter;
						if ( curUnit->IsAlive() && !curUnit->GetStats()->IsAviation() )
						{
							nNewPlayer = nPlayer; // don't change player, defenders are here
							break;
						}
					}
				}
			}
		}

		// This will be ignore after instant changes
		if ( theDipl.GetNParty( nNewPlayer ) != theDipl.GetNParty( GetPlayer() ) )
		{
			// count some time then change player
			if ( timeToChangeOwner == 0 || nSideToCapture == -1 )
			{
				if ( GetScenarioTracker()->GetGameType() != IAIScenarioTracker::EGT_SINGLE )
					timeToChangeOwnerTotal = NGlobal::GetVar( "AI.MultiplayerCaptureTime", int( SConsts::STORAGE_CAPTURE_TIME ) );
				else
					timeToChangeOwnerTotal = SConsts::STORAGE_CAPTURE_TIME;

				timeToChangeOwner = curTime + timeToChangeOwnerTotal;
				nSideToCapture = theDipl.GetNParty( nNewPlayer );
				nNewPlayer = GetPlayer();
			}
			else
			{
				if ( curTime < timeToChangeOwner )		// still not done, do not change
				{
					float fProgress = 0.5f;
					if ( timeToChangeOwnerTotal > 1 )
						fProgress = 1.0f - float( timeToChangeOwner - curTime ) / timeToChangeOwnerTotal;

					CPtr<SAIKeyBuildingCaptureUpdate> pProgressUpdate = new SAIKeyBuildingCaptureUpdate;
					pProgressUpdate->nObjUniqueID = GetUniqueId();
					pProgressUpdate->fProgress = fProgress;
					pProgressUpdate->nOldSide = theDipl.GetNParty( GetPlayer() );
					pProgressUpdate->nNewSide = theDipl.GetNParty( nNewPlayer );
					updater.AddUpdate( pProgressUpdate, ACTION_NOTIFY_KEY_CAPTURE_PROGRESS, this, 0 );

					nNewPlayer = GetPlayer();
				}
			}
		}
		else
		{
			if ( nSideToCapture != -1 )
				updater.AddUpdate( 0, ACTION_NOTIFY_UPDATE_DIPLOMACY, this, -1 );	

			nSideToCapture = -1;
			timeToChangeOwner = 0;
			nNewPlayer = GetPlayer();
		}

		if ( theDipl.GetNParty( nNewPlayer ) != theDipl.GetNParty( GetPlayer() ) )
			ChangePlayer( nNewPlayer );
	}
}

void CBuildingSimple::CheckHitPoints()
{
	if ( theBonusSystem.IsKeyBuilding( nLinkID ) && GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
	{
		if ( GetHitPoints() < GetStats()->fMaxHP * g_fKeyPointDamageThreshold )
			updater.AddUpdate( CreateStatusUpdate( EUS_KEY_POINT_DAMAGED, true, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
		else
			updater.AddUpdate( CreateStatusUpdate( EUS_KEY_POINT_DAMAGED, false, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
	}
}

void CBuildingSimple::SetHitPoints( const float fNewHP )
{
	CBuilding::SetHitPoints( fNewHP );
	CheckHitPoints();
}

void CBuildingSimple::TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	float fActualDamage = fDamage;

	// Check for key buildings
	if ( theBonusSystem.IsKeyBuilding( nLinkID ) )
	{
		float fMinHP = GetMinHP();
		if ( GetHitPoints() - fDamage < fMinHP && GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
			ChangePlayer( theDipl.GetNeutralPlayer() );

		if ( fDamage > GetHitPoints() )
		{
			fActualDamage = GetHitPoints() - fMinHP;
		}
	}

	CBuilding::TakeDamage( fActualDamage, bFromExplosion, nPlayerOfShoot, pShotUnit );
	CheckHitPoints();
}

void CBuildingSimple::Die( const float fDamage )
{
	CBuilding::Die( fDamage );
}

//*******************************************************************
//*										CBuilding::SHealthySort												*
//*******************************************************************

bool CBuilding::SHealthySort::operator()( const CPtr<CSoldier> &a, const CPtr<CSoldier> &b ) 
{ 
	return a->GetHitPoints() < b->GetHitPoints(); 
}

//*******************************************************************
//*											CBuilding::SIllSort													*
//*******************************************************************

bool CBuilding::SIllSort::operator()( const CPtr<CSoldier> &a, const CPtr<CSoldier> &b ) 
{ 
	return a->GetHitPoints() > b->GetHitPoints(); 
}

//*******************************************************************
//*												  CBuilding																*
//*******************************************************************

BASIC_REGISTER_CLASS( CBuilding );

CBuilding::CBuilding( const SBuildingRPGStats *_pStats, const CVec3 &center, const WORD _wDir, const float fHP, const int nFrameIndex, int _nLinkID )
: pStats( _pStats ), CGivenPassabilityStObject( center, fHP, _wDir, nFrameIndex ), pLockingUnit( 0 ),
	nOveralPlaces( _pStats->nRestSlots + _pStats->nMedicalSlots + _pStats->aiSlots.size() ),
	medical( _pStats->nMedicalSlots ), fire( _pStats->aiSlots.size() ), rest( _pStats->nRestSlots ),
	bAlarm( false ), nLinkID( _nLinkID ),
	turrets( _pStats->aiSlots.size() ), guns( _pStats->aiSlots.size() ),
	nextSegmTime( curTime ), lastDistibution( 0 ), nLastFreeFireSoldierChoice( 0 ),
	firePlace2Soldier( _pStats->aiSlots.size() ),
	nLastPlayer( theDipl.GetNeutralPlayer() ), nScriptID( -1 ), bShouldEscape( false ), bEscaped( false ), timeOfDeath( 0 ),
	lastLeave( theDipl.GetNPlayers() ), bKeyBuilding( false ), wDir( _wDir ),
	nIterator( -1 ), startOfRest( 0 ), bNewtralInfantryInside( false )
{
	SetAlive( fHP > 0.0f );
	memset( &lastLeave[0], 0, sizeof(lastLeave[0]) * lastLeave.size() );

	Init();

	for ( int i = 0; i < pStats->aiSlots.size(); ++i )
	{
		guns[i] = new CMechUnitGuns();
		if ( pStats->aiSlots[i].gun.pWeapon != 0 )
		{
			turrets[i] = new CMountedTurret( this, i );
			
			int nGuns = 0;
			guns[i]->AddGun( CMountedGunsFactory( this, static_cast_ptr<CMountedTurret*>(turrets[i]), i ), 0, i, pStats->aiSlots[i].gun.pWeapon, &nGuns, pStats->aiSlots[i].gun.nAmmo );
		}
	}

	InitObservationPlaces();
	bKeyBuilding = theBonusSystem.IsKeyBuilding( nLinkID );
	if ( bKeyBuilding  )
		theStatObjs.RegisterSegment( this );
}

void CBuilding::InitObservationPlaces()
{
	observationPlaces.SetSizes( 4, 4 );
	observationPlaces.FillEvery( 0 );
	sides.resize( 4 );
	firePlace2Observation.resize( pStats->aiSlots.size(), -1 );

	SRect boundRect;
	GetBoundRect( &boundRect );
	CVec2 vCenter( boundRect.center );

	vector<int> takenSlots( pStats->aiSlots.size(), 0 );

	for ( int i = 0; i < 4; ++i )
	{
		int nLeftSlot = -1;
		WORD wLeftDiff = 65535;
		int nMiddleSlot = -1;
		WORD wMiddleDiff = 65535;
		int nRightSlot = -1;
		WORD wRightDiff = 65535;

		const CVec2 vRightPoint = boundRect.v[i] - vCenter; 
		const CVec2 vLeftPoint = boundRect.v[(i+1) % 4] - vCenter;
		const CVec2 vMiddlePoint = vRightPoint + vLeftPoint;
		WORD wRightDir = GetDirectionByVector( vRightPoint );
		WORD wLeftDir = GetDirectionByVector( vLeftPoint );
		WORD wMiddleDir = GetDirectionByVector( vMiddlePoint );

		for ( int j = 0; j < pStats->aiSlots.size(); ++j )
		{
			const CVec2 vSlotDir = CVec2( pStats->aiSlots[j].vPos.x, pStats->aiSlots[j].vPos.y ) + CVec2(GetCenter().x,GetCenter().y) - vCenter;
			const WORD wSlotDir = GetDirectionByVector( vSlotDir ) + wDir;

			if ( takenSlots[j] == 0 && DirsDifference( pStats->aiSlots[j].wDirection, wMiddleDir ) < 4000 )
			{
				if ( !IsInTheMinAngle( wSlotDir, wLeftDir, wRightDir ) )
				{
					if ( DirsDifference( wSlotDir, wLeftDir ) < DirsDifference( wSlotDir, wRightDir ) )
						wLeftDir = wSlotDir;
					else
						wRightDir = wSlotDir;
				}
				
				++sides[i].nFireSlots;
				takenSlots[j] = 1;

				const WORD wLocalLeftDiff = DirsDifference( wSlotDir, wLeftDir );
				if ( wLocalLeftDiff < wLeftDiff )
				{
					nLeftSlot = j;
					wLeftDiff = wLocalLeftDiff;
				}
			}
		}

		for ( int j = 0; j < pStats->aiSlots.size(); ++j )
		{
			const CVec2 vSlotDir = CVec2( pStats->aiSlots[j].vPos.x, pStats->aiSlots[j].vPos.y ) + CVec2(GetCenter().x,GetCenter().y) - vCenter;
			const WORD wSlotDir = GetDirectionByVector( vSlotDir ) + wDir;

			if ( nLeftSlot != j && DirsDifference( pStats->aiSlots[j].wDirection, wMiddleDir ) < 4000 )
			{
				const WORD wLocalRightDiff = DirsDifference( wSlotDir, wRightDir );
				if ( wLocalRightDiff < wRightDiff )
				{
					nRightSlot = j;
					wRightDiff = wLocalRightDiff;
				}
			}
		}

		for ( int j = 0; j < pStats->aiSlots.size(); ++j )
		{
			const CVec2 vSlotDir = CVec2( pStats->aiSlots[j].vPos.x, pStats->aiSlots[j].vPos.y ) + CVec2(GetCenter().x,GetCenter().y) - vCenter;
			const WORD wSlotDir = GetDirectionByVector( vSlotDir ) + wDir;

			if ( nLeftSlot != j && nRightSlot != j && DirsDifference( pStats->aiSlots[j].wDirection, wMiddleDir ) < 4000 )
			{
				const WORD wLocalMiddleDiff = DirsDifference( wSlotDir, wMiddleDir );
				if ( wLocalMiddleDiff < wMiddleDiff )
				{
					nMiddleSlot = j;
					wMiddleDiff = wLocalMiddleDiff;
				}
			}
		}

		if ( nRightSlot != -1 )
		{
			observationPlaces[i][sides[i].nObservationPoints] = nRightSlot;
			firePlace2Observation[nRightSlot] = (sides[i].nObservationPoints << 2) | i;
			++sides[i].nObservationPoints;
		}

		if ( nMiddleSlot != -1 )
		{
			observationPlaces[i][sides[i].nObservationPoints] = nMiddleSlot;
			firePlace2Observation[nMiddleSlot] = (sides[i].nObservationPoints << 2) | i;
			++sides[i].nObservationPoints;
		}

		if ( nLeftSlot != -1 )
		{
			observationPlaces[i][sides[i].nObservationPoints] = nLeftSlot;
			firePlace2Observation[nLeftSlot] = ( sides[i].nObservationPoints << 2 ) | i;
			++sides[i].nObservationPoints;
		}
	}
}

CAIUnit* CBuilding::GetIteratedUnit() 
{ 
	NI_VERIFY( !IsIterateFinished(), "Wrong fire unit to get", return 0 );
	return fire[nIterator];
}

void CBuilding::PopFromFire()
{
	// если ещё не пересадили в другой слот
	if ( fire.GetMaxEl()->GetSlot() != -1 )
		DelSoldierFromFirePlace( fire.GetMaxEl() );
}

void CBuilding::SetFiringUnitProperties( CSoldier *pUnit, const int nSlot, const int nIndex )
{
	pUnit->SetSlotInfo( nSlot, 0, nIndex );

	pUnit->SetVisionAngle( 32768 );
	pUnit->SetAngles( pStats->aiSlots[nSlot].wDirection + wDir - pStats->aiSlots[nSlot].wAngle, pStats->aiSlots[nSlot].wDirection + wDir + pStats->aiSlots[nSlot].wAngle );

	CVec2 vSlotPos( pStats->aiSlots[nSlot].vPos.x, pStats->aiSlots[nSlot].vPos.y );
	vSlotPos = MoveVectorByDirection( vSlotPos, wDir );
	const CVec3 vNewCoord( GetCenter().x + vSlotPos.x, GetCenter().y + vSlotPos.y, GetCenter().z );
	pUnit->SetCenter( vNewCoord );
	pUnit->CallUpdatePlacement();
	// поставить owner у mounted gun
	guns[nSlot]->SetOwner( pUnit );

	pUnit->WarFogChanged();
}

const BYTE CBuilding::GetFreeFireSlot()
{
	for ( int j = 0; j < firePlace2Soldier.size(); ++j )
	{
		if ( firePlace2Soldier[j] == 0 && turrets[j] != 0 )
		{
			nLastFreeFireSoldierChoice = (nLastFreeFireSoldierChoice + 1) % pStats->aiSlots.size();
			return j;
		}
	}
	
	int i = nLastFreeFireSoldierChoice;
	while ( i < firePlace2Soldier.size() && firePlace2Soldier[i] != 0 )
		++i;

	if ( i >= firePlace2Soldier.size() || firePlace2Soldier[i] != 0 )
	{
		 i = 0;
		 while ( i < nLastFreeFireSoldierChoice && firePlace2Soldier[i] != 0 )
			 ++i;

		NI_ASSERT( i < nLastFreeFireSoldierChoice, "Can't find empty fireplace" );	
	}

	nLastFreeFireSoldierChoice = (nLastFreeFireSoldierChoice + 1) % pStats->aiSlots.size();

	return i;
}

void CBuilding::PushToFire( CSoldier *pUnit )
{
	NI_ASSERT( pUnit->IsInSolidPlace() || pUnit->IsInFirePlace(), "Visible unit in a building" );
	pUnit->SetToFirePlace();
	
	const int nSlot = GetFreeFireSlot();
	PushSoldierToFirePlace( pUnit, nSlot );
}

void CBuilding::PushToMedical( CSoldier *pUnit )
{
	NI_ASSERT( pUnit->IsInSolidPlace() || pUnit->IsInFirePlace(), "Visible unit in a building" );

	pUnit->SetSlotInfo( -1, 1, medical.Size() );
	medical.Push( pUnit );
	pUnit->SetToSolidPlace();

	pUnit->SetVisionAngle( 0 );
}

void CBuilding::PushToRest( CSoldier *pUnit )
{
	NI_ASSERT( pUnit->IsInSolidPlace() || pUnit->IsInFirePlace(), "Visible unit in a building" );

	pUnit->SetSlotInfo( -1, 2, rest.Size() );
	rest.Push( pUnit );
	pUnit->SetToSolidPlace();

	pUnit->SetVisionAngle( 0 );
}

const CVec2 CBuilding::GetEntrancePoint( const int nEntrance ) const
{
	NI_ASSERT( nEntrance < GetNEntrancePoints(), "Wrong number of entrance point" );
	CVec2 vLocalPos( pStats->entrances[nEntrance].vPos.x, pStats->entrances[nEntrance].vPos.y );
	return CVec2(GetCenter().x,GetCenter().y) + MoveVectorByDirection( vLocalPos, wDir );
}

void CBuilding::GetEntranceData( CVec2 *pvPoint, WORD *pwDir, int nIndex ) const
{
	*pvPoint = GetEntrancePoint( nIndex );
	*pwDir = pStats->entrances[nIndex].nDir + wDir;
}

const float CBuilding::GetEscapeHitPoints() const
{
	return GetStats()->fMaxHP * SConsts::HP_PERCENT_TO_ESCAPE_FROM_BUILDING;
}

void CBuilding::AddSoldier( CSoldier *pUnit )
{
	NI_ASSERT( GetNFreePlaces() != 0, "No free places in the building" );

	bool bUpdateSelectability = false;
	// если это первый солдат здания, то зарегистрировать в сегментах
	if ( GetNFreePlaces() == nOveralPlaces )
	{
		nextSegmTime = curTime + SConsts::AI_SEGMENT_DURATION - 1;		
		theStatObjs.RegisterSegment( this );

		bUpdateSelectability = true;
		bShouldEscape = GetHitPoints() > GetEscapeHitPoints();
	}
	bEscaped = false;

	pUnit->SetToSolidPlace();
	pUnit->ApplyStatsModifier( pStats->pSoldierStatsModifier, true );

	const float fHP = pUnit->GetHitPoints();
	if ( fire.Size() < fire.GetReserved() )
		PushToFire( pUnit );
	else if ( medical.Size() == medical.GetReserved() )
		PushToRest( pUnit );
	else if ( fHP < pUnit->GetStats()->fMaxHP || rest.Size() == rest.GetReserved() )
		PushToMedical( pUnit );
	else
		PushToRest( pUnit );

	startOfRest = curTime;

	if ( bUpdateSelectability )
	{
		updater.AddUpdate( 0, ACTION_NOTIFY_SELECTABLE_CHANGED, this, IsSelectable() );
		updater.AddUpdate( 0, ACTION_NOTIFY_UPDATE_DIPLOMACY, this, -1 );

		nLastPlayer = GetPlayer();
	}
}

void CBuilding::SeatSoldierToMedicalSlot()
{
	NI_ASSERT( medical.Size() < medical.GetReserved(), "Wrong call of SeatSoldierToMedicalSlot" );

	if ( !rest.IsEmpty() && rest.GetMaxEl()->GetHitPoints() < rest.GetMaxEl()->GetStats()->fMaxHP )
	{
		CSoldier *pSoldier = rest.GetMaxEl();
		rest.Pop();
		PushToMedical( pSoldier );
	}
}

void CBuilding::SeatSoldierToFireSlot()
{
	NI_ASSERT( fire.Size() < fire.GetReserved(), "Wrong call of SeatSoldierToFireSlot" );

	if ( medical.IsEmpty() && rest.IsEmpty() )
		return;

	if ( medical.IsEmpty() )
	{
		CSoldier *pSoldier = rest.GetMaxEl();
		rest.Pop();
		PushToFire( pSoldier );
	}
	else if ( rest.IsEmpty() )
	{
		CSoldier *pSoldier = medical.GetMaxEl();
		medical.Pop();
		PushToFire( pSoldier );
	}
	else if ( medical.GetMaxEl()->GetHitPoints() > rest.GetMaxEl()->GetHitPoints() )
	{
		CSoldier *pSoldier = medical.GetMaxEl();
		medical.Pop();
		PushToFire( pSoldier );
		SeatSoldierToMedicalSlot();
	}
	else
	{
		CSoldier *pSoldier = rest.GetMaxEl();
		rest.Pop();
		PushToFire( pSoldier );
	}
}

void CBuilding::DelSoldierFromFirePlace( CSoldier *pSoldier )
{
	const SStaticObjectSlotInfo slotInfo = pSoldier->GetSlotInfo();
	NI_ASSERT( slotInfo.nType == 0, StrFmt( "Wrong of soldier slot type (%d)", slotInfo.nType ) );

	// CRAP{
	if ( slotInfo.nIndex < fire.Size() )
	// CRAP}
	{
		fire[slotInfo.nIndex] = 0;
		fire.Erase( slotInfo.nIndex );
	}

	// CRAP{
	if ( slotInfo.nSlot < firePlace2Soldier.size() )
	// CRAP}
	{
		firePlace2Soldier[slotInfo.nSlot] = 0;

		// в observation point
		if ( firePlace2Observation[slotInfo.nSlot] != -1 )
		{
			const int nSide = firePlace2Observation[slotInfo.nSlot] & 3;
			--sides[nSide].nSoldiersInObservationPoints;
		}
	}

	pSoldier->SetSlotInfo( -1, -1, -1 );
}

void CBuilding::DelSoldierFromMedicalPlace( CSoldier *pSoldier )
{
	const SStaticObjectSlotInfo slotInfo = pSoldier->GetSlotInfo();
	NI_ASSERT( slotInfo.nType == 1, StrFmt( "Wrong of soldier slot type (%d)", slotInfo.nType ) );

	medical[slotInfo.nIndex] = 0;
	medical.Erase( slotInfo.nIndex );

	pSoldier->SetSlotInfo( -1, -1, -1 );
}

void CBuilding::DelSoldierFromRestPlace( CSoldier *pSoldier )
{
	const SStaticObjectSlotInfo slotInfo = pSoldier->GetSlotInfo();
	NI_ASSERT( slotInfo.nType == 2, StrFmt( "Wrong of soldier slot type (%d)", slotInfo.nType ) );

	// CRAP{
	if ( slotInfo.nIndex < rest.Size() )
	// CRAP}
	{
		rest[slotInfo.nIndex] = 0;
		rest.Erase( slotInfo.nIndex );
	}

	pSoldier->SetSlotInfo( -1, -1, -1 );
}

void CBuilding::DelSoldier( CSoldier *pUnit, const bool bFillEmptyFireplace )
{
	const SStaticObjectSlotInfo slotInfo = pUnit->GetSlotInfo();
	switch ( slotInfo.nType )
	{
		case 0:
			DelSoldierFromFirePlace( pUnit );
			if ( bFillEmptyFireplace )
				SeatSoldierToFireSlot();

			break;
		case 1:
			DelSoldierFromMedicalPlace( pUnit );
			if ( bFillEmptyFireplace )
				SeatSoldierToMedicalSlot();

			break;
		case 2:
			DelSoldierFromRestPlace( pUnit );

			break;
		default:
			NI_ASSERT( false, "Wrong slot info of unit" );
	}

	pUnit->ApplyStatsModifier( pStats->pSoldierStatsModifier, false );

	if ( fire.Size() == 0 && medical.Size() == 0 && rest.Size() == 0 )
	{
		updater.AddUpdate( 0, ACTION_NOTIFY_UPDATE_DIPLOMACY, this, -1 );
		updater.AddUpdate( 0, ACTION_NOTIFY_SELECTABLE_CHANGED, this, IsSelectable() );
	}
}

void CBuilding::SoldierDamaged( CSoldier *pUnit )
{
	const SStaticObjectSlotInfo slotInfo = pUnit->GetSlotInfo();
	switch ( slotInfo.nType )
	{
		case 0:
			fire[slotInfo.nIndex] = 0;
			fire.Erase( slotInfo.nIndex );
			pUnit->SetSlotInfo( slotInfo.nSlot, 0, fire.Size() );
			fire.Push( pUnit );

			break;
		case 1:
			medical[slotInfo.nIndex] = 0;
			medical.Erase( slotInfo.nIndex );
			pUnit->SetSlotInfo( -1, 1, medical.Size() );
			medical.Push( pUnit );

			break;
		case 2:
			rest[slotInfo.nIndex] = 0;
			rest.Erase( slotInfo.nIndex );
			pUnit->SetSlotInfo( -1, 2, rest.Size() );
			rest.Push( pUnit );

			break;
		default:
			NI_ASSERT( false, "Wrong slot info of unit" );
	}

	Alarm();
}

void CBuilding::GoOutFromEntrance( const int nEntrance, class CSoldier *pUnit )
{
	NI_ASSERT( nEntrance < pStats->entrances.size(), "Wrong number of entrance" );
	DelInsider( pUnit );
}

void CBuilding::SwapFireMed()
{
	CPtr<CSoldier> pFireSoldier = fire.GetMaxEl();
	PopFromFire();
	CPtr<CSoldier> pMedicalSoldier = medical.GetMaxEl();
	medical.Pop();
	PushToFire( pMedicalSoldier );
	PushToMedical( pFireSoldier );
}

void CBuilding::SwapRestMed()
{
	CPtr<CSoldier> pMedicalSoldier = medical.GetMaxEl();
	medical.Pop();
	CPtr<CSoldier> pRestSoldier = rest.GetMaxEl();
	rest.Pop();

	PushToMedical( pRestSoldier ); 
	PushToRest( pMedicalSoldier );
}

bool CBuilding::IsIllInRest()
{
	return !rest.IsEmpty() && rest.GetMaxEl()->GetHitPoints() < rest.GetMaxEl()->GetStats()->fMaxHP;
}

bool CBuilding::IsIllInFire()
{
	return !fire.IsEmpty() && fire.GetMaxEl()->GetHitPoints() < fire.GetMaxEl()->GetStats()->fMaxHP;
}

void CBuilding::DistributeAll()
{
	// обменять тех, кто вылечился в medical с больными в fire или rest	
	while ( !medical.IsEmpty() && medical.GetMaxEl()->GetHitPoints() == medical.GetMaxEl()->GetStats()->fMaxHP &&	( IsIllInFire() || IsIllInRest() ) )
	{
		const bool bIllInFire = IsIllInFire();
		const bool bIllInRest = IsIllInRest();

		if ( bIllInFire && bIllInRest )
		{
			if ( fire.GetMaxEl()->GetHitPoints() < rest.GetMaxEl()->GetHitPoints() )
				SwapFireMed();
			else
				SwapRestMed();
		}
		else if ( bIllInFire )
			SwapFireMed();
		else
			SwapRestMed();
	}

	// выгнать халявщиков из medical places в fireplaces
	while ( !medical.IsEmpty() && medical.GetMaxEl()->GetHitPoints() == medical.GetMaxEl()->GetStats()->fMaxHP &&	fire.Size() != fire.GetReserved() )
	{
		CSoldier *pSoldier = medical.GetMaxEl();
		medical.Pop();
		PushToFire( pSoldier );
	}

	// выгнать халявщиков из medical places в restplaces
	while ( !medical.IsEmpty() && medical.GetMaxEl()->GetHitPoints() == medical.GetMaxEl()->GetStats()->fMaxHP &&	rest.Size() != rest.GetReserved() )
	{
		CSoldier *pSoldier = medical.GetMaxEl();
		medical.Pop();
		PushToRest( pSoldier );
	}

	// загнать тех, кого возмножно, лечиться
	while ( medical.Size() != medical.GetReserved() && ( IsIllInRest() || IsIllInFire() ) )
	{
		if ( !fire.IsEmpty() && !rest.IsEmpty() )
		{
			if ( fire.GetMaxEl()->GetHitPoints() < rest.GetMaxEl()->GetHitPoints() )
			{
				CSoldier *pSoldier = fire.GetMaxEl();
				DelSoldierFromFirePlace( pSoldier );
				PushToMedical( pSoldier );
			}
			else
			{
				CSoldier *pSoldier = rest.GetMaxEl();
				rest.Pop();
				PushToMedical( pSoldier );
			}
		}
		else if ( !rest.IsEmpty() )
		{
			CSoldier *pSoldier = rest.GetMaxEl();
			rest.Pop();
			PushToMedical( pSoldier );
		}
		else
		{
			CSoldier *pSoldier = fire.GetMaxEl();
			DelSoldierFromFirePlace( pSoldier );
			PushToMedical( pSoldier );
		}
	}

	SetSoldiersToObservationPoints();
	CentreSoldiersInObservationPoints();
}

void CBuilding::CentreSoldiersInObservationPoints()
{
	for ( int i = 0; i < fire.Size(); ++i )
	{
		CSoldier *pSoldier = fire[i];
		if ( IsSoldierInObservationPoint( pSoldier ) )
		{
			int nSide = firePlace2Observation[pSoldier->GetSlot()] & 3;
			if ( sides[nSide].nSoldiersInObservationPoints == 1 && GetMiddleObservationPoint( nSide ) != pSoldier->GetSlot() )
			{
				DelSoldierFromFirePlace( pSoldier );
				PushSoldierToFirePlace( pSoldier, GetMiddleObservationPoint( nSide ) );
			}
		}
	}
}

bool CBuilding::TryToPushRestSoldierToObservation( CSoldier *pRestingSoldier )
{
	// по сторонам
	for ( int j = 0; j < 4; ++j )
	{
		// по точкам наблюдения в сторонах
		for ( int k = 0; k < sides[j].nObservationPoints; ++k )
		{
			if ( CSoldier *pSoldierInPoint = firePlace2Soldier[observationPlaces[j][k]] )
			{
				if ( IsBetterChangeObservationSoldier( pRestingSoldier, pSoldierInPoint ) )
				{
					SStaticObjectSlotInfo slotInfo = pRestingSoldier->GetSlotInfo();
					NI_ASSERT( slotInfo.nType == 2, "Wrong slot info of resting soldier" );

					rest[slotInfo.nIndex] = 0;
					rest.Erase( slotInfo.nIndex );

					DelSoldierFromFirePlace( pSoldierInPoint );
					PushToRest( pSoldierInPoint );
					PushSoldierToFirePlace( pRestingSoldier, observationPlaces[j][k] );

					return true;
				}
			}
		}
	}

	return false;
}

bool CBuilding::TryToPushFireSoldierToObservation( CSoldier *pFiringSoldier )
{
	// по сторонам
	for ( int j = 0; j < 4; ++j )
	{
		// по точкам наблюдения в сторонах
		for ( int k = 0; k < sides[j].nObservationPoints; ++k )
		{
			if ( CSoldier *pSoldierInPoint = firePlace2Soldier[observationPlaces[j][k]] )
			{
				if ( IsBetterChangeObservationSoldier( pFiringSoldier, pSoldierInPoint ) )
				{
					const int nFiringSoldierSlot = pFiringSoldier->GetSlot();
					DelSoldierFromFirePlace( pFiringSoldier );
					DelSoldierFromFirePlace( pSoldierInPoint );

					PushSoldierToFirePlace( pFiringSoldier, observationPlaces[j][k] );
					PushSoldierToFirePlace( pSoldierInPoint, nFiringSoldierSlot );

					return true;
				}
			}
		}
	}

	return false;
}

void CBuilding::PushSoldierToFirePlace( CSoldier *pUnit, const int nFirePlace )
{
	pUnit->SetToFirePlace();
	
	pUnit->SetSlotInfo( -1, 0, fire.Size() );
	const int nIndex = fire.Push( pUnit );
	SetFiringUnitProperties( pUnit, nFirePlace, nIndex );

	firePlace2Soldier[nFirePlace] = pUnit;

	// это - observation point
	if ( firePlace2Observation[nFirePlace] != -1 )
	{
		// увеличить количество солдат в observation point на стороне
		const int nSide = firePlace2Observation[nFirePlace] & 3;
		++sides[nSide].nSoldiersInObservationPoints;
	}
}

void CBuilding::PushSoldierToObservationPoint( CSoldier *pSoldier, const int nSide )
{
	if ( sides[nSide].nObservationPoints != 0  )
	{
		// ещё никто не сидит
		if ( sides[nSide].nSoldiersInObservationPoints == 0 )
		{
			const int nFirePlace = GetMiddleObservationPoint( nSide );
			PushSoldierToFirePlace( pSoldier, nFirePlace );
		}
		// сидит только один
		else if ( sides[nSide].nSoldiersInObservationPoints == 1 && sides[nSide].nObservationPoints > 1 )
		{
			int nLeftPoint, nRightPoint;
			GetSidesObservationPoints( nSide, &nLeftPoint, &nRightPoint );

			CSoldier *pMiddleSoldier = GetSoldierOnSide( nSide );
			DelSoldierFromFirePlace( pMiddleSoldier );

			PushSoldierToFirePlace( pSoldier, nLeftPoint );
			PushSoldierToFirePlace( pMiddleSoldier, nRightPoint );
		}
		else
			NI_ASSERT( false, "Can't push soldier to observation point" );
	}
	else
		NI_ASSERT( false, "Can't push soldier to observation point" );
}

bool CBuilding::IsSoldierInObservationPoint( CSoldier *pSoldier ) const
{
	if ( !pSoldier->IsInFirePlace() )
		return false;
	else
	{
		const int nFireSlot = pSoldier->GetSlot();
		return firePlace2Observation[nFireSlot] != -1;
	}
}

void CBuilding::SetSoldiersToObservationPoints()
{
	for ( int i = 0; i < rest.Size();	++i )
	{
		CSoldier *pRestingSoldier = rest[i];		
		const int nSide = ChooseSideToSetSoldier( pRestingSoldier );
		// во всех точках наблюдения есть солдаты
		if ( nSide == -1 || fire.Size() == fire.GetReserved() )
		{
			if ( TryToPushRestSoldierToObservation( pRestingSoldier ) )
				return;
		}
		else
		{
			rest[i] = 0;
			rest.Erase( i );
			PushSoldierToObservationPoint( pRestingSoldier, nSide );
			return;
		}
	}

	// рассадить солдат не в точках наблюдения
	for ( int i = 0; i < fire.Size(); ++i )
	{
		CSoldier *pSoldier = fire[i];
		if ( !IsSoldierInObservationPoint( pSoldier ) )
		{
			const int nSide = ChooseSideToSetSoldier( pSoldier );
			if ( nSide == -1 )
			{
				if ( TryToPushFireSoldierToObservation( pSoldier ) )
					return;
			}
			else
			{
				DelSoldierFromFirePlace( pSoldier );
				PushSoldierToObservationPoint( pSoldier, nSide );
				return;
			}
		}
	}

	// рассадить солдат в точках наблюдения
	int nIndexToTry = -1;
	int nSoldierInTrySide = -1;
	for ( int i = 0; i < fire.Size(); ++i )
	{
		if ( IsSoldierInObservationPoint( fire[i] ) )
		{
			const int nSide = firePlace2Observation[fire[i]->GetSlot()] & 3;
			if ( nIndexToTry == -1 || nSoldierInTrySide < sides[nSide].nSoldiersInObservationPoints )
			{
				nIndexToTry = i;
				nSoldierInTrySide = sides[nSide].nSoldiersInObservationPoints;
			}
		}
	}

	if ( nIndexToTry != -1 )
	{
		CSoldier *pSoldier = fire[nIndexToTry];

		const int nSide = ChooseSideToSetSoldier( pSoldier );
		if ( nSide == -1 )
			TryToPushFireSoldierToObservation( pSoldier );
		else
		{
			DelSoldierFromFirePlace( pSoldier );
			PushSoldierToObservationPoint( pSoldier, nSide );
		}
	}
}

void CBuilding::DistributeNonFires()
{
	// обменять тех, кто вылечился в medical с больными в rest
	while ( !medical.IsEmpty() && medical.GetMaxEl()->GetHitPoints() == medical.GetMaxEl()->GetStats()->fMaxHP && IsIllInRest() )
	{
		CPtr<CSoldier> pMedicalSoldier = medical.GetMaxEl();
		medical.Pop();
		CPtr<CSoldier> pRestSoldier = rest.GetMaxEl();
		rest.Pop();

		PushToMedical( pRestSoldier ); 
		PushToRest( pMedicalSoldier );
	}

	// выгнать тех, кто вылечился
	while ( rest.Size() < rest.GetReserved() && !medical.IsEmpty() && medical.GetMaxEl()->GetHitPoints() == medical.GetMaxEl()->GetStats()->fMaxHP )
	{
		CSoldier *pSoldier = medical.GetMaxEl();
		medical.Pop();
		PushToRest( pSoldier );
	}

	// загнать больных из rest лечиться
	while ( IsIllInRest() && medical.Size() < medical.GetReserved() )
	{
		CSoldier *pSoldier = rest.GetMaxEl();
		rest.Pop();
		PushToMedical( pSoldier );
	}
}

void CBuilding::DistributeFiringSoldiers()
{
	// не все fireslots заняты
	if ( fire.Size() < fire.GetReserved() )
	{
		for ( int i = 0; i < fire.Size(); ++i )
		{
			// не стреляет
			if ( fire[i]->GetState()->GetName() == EUSN_REST_IN_BUILDING )
			{
				CSoldier *pSoldier = fire[i];

				bool bShouldWatch = true;
				for ( int j = 0; j < 4; ++j )
					bShouldWatch = bShouldWatch && ( sides[j].nSoldiersInObservationPoints == 0 );

				if ( bShouldWatch )
				{
					const int nSide = ChooseSideToSetSoldier( pSoldier );
					NI_ASSERT( nSide != -1, "Wrong side chosen" );

					DelSoldierFromFirePlace( pSoldier );
					PushSoldierToObservationPoint( pSoldier, nSide );
				}
				else
				{
					nLastFreeFireSoldierChoice += NRandom::Random( 0, pStats->aiSlots.size() );
					const int nNewFireSlot = GetFreeFireSlot();

					DelSoldierFromFirePlace( pSoldier );
					PushSoldierToFirePlace( pSoldier, nNewFireSlot );
				}
			}
		}
	}
}

void CBuilding::ExchangeSoldiersToTurrets()
{
	for ( int i = 0; i < turrets.size(); ++i )
	{
		if ( turrets[i] != 0 && firePlace2Soldier[i] == 0 )
		{
			int j = 0;
			while ( j < firePlace2Soldier.size() && ( firePlace2Soldier[j] == 0 || turrets[j] == 0 ) )
				++j;

			if ( j < firePlace2Soldier.size() )
			{
				CPtr<CSoldier> pSoldier = firePlace2Soldier[j];
				NI_ASSERT( j == pSoldier->GetSlotInfo().nSlot, "Wrong soldier slot info" );
				DelSoldierFromFirePlace( pSoldier );
				PushSoldierToFirePlace( pSoldier, i );
			}
			else
				return;
		}
	}
}

void CBuilding::Segment()
{
	nextSegmTime = curTime + NRandom::Random( 2 * SConsts::AI_SEGMENT_DURATION, 10 * SConsts::AI_SEGMENT_DURATION );

	if ( !IsUnitsInside() )
	{
		if ( TYPE_MAIN_RU_STORAGE != pStats->etype && TYPE_TEMP_RU_STORAGE != pStats->etype && !theBonusSystem.IsKeyBuilding( nLinkID ) )
			theStatObjs.UnregisterSegment( this );

		bShouldEscape = false;
		bEscaped = false;
		timeOfDeath = 0;
	}
	// на всякий случай
	else if ( timeOfDeath != 0 && timeOfDeath + 2000 < curTime )
	{
		KillAllInsiders();
		timeOfDeath = 0;
	}
	else if ( !CStormableObject::Segment() )
	{
		// полечить
		const float fInc = SConsts::AI_SEGMENT_DURATION * SConsts::CURE_SPEED_IN_BUILDING;
		for ( int i = 0; i < medical.Size(); ++i )
		{
			const float fWantedInc = Min( fInc, medical[i]->GetStats()->fMaxHP - medical[i]->GetHitPoints() );
			medical[i]->IncreaseHitPoints( fWantedInc );
		}

		// обработать alarms
		if ( bAlarm )
		{
			while ( !fire.Size() == fire.GetReserved() && ( !medical.IsEmpty() || !rest.IsEmpty() ) )
				SeatSoldierToFireSlot();

			bAlarm = false;
			startOfRest = curTime;
		}
		else
		{
			for ( int i = 0; i < fire.Size(); ++i )
			{
				if ( fire[i]->GetState()->GetName() != EUSN_REST_IN_BUILDING )
				{
					startOfRest = curTime;
					break;
				}
			}
		}

		if ( curTime - lastDistibution >= 4000 || curTime - startOfRest < SConsts::TIME_OF_BUILDING_ALARM && curTime - lastDistibution >= 3500 )
		{
			lastDistibution = curTime;			

			// тревога
			if ( curTime - startOfRest < SConsts::TIME_OF_BUILDING_ALARM )
				DistributeFiringSoldiers();

			// всем можно лечиться
			if ( curTime - startOfRest >= SConsts::TIME_OF_BUILDING_ALARM )
				DistributeAll();
			else
				// только не стреляющим можно лечиться
				DistributeNonFires();
		}

		// загнать всех халявщиков в fireplaces
		while ( !rest.IsEmpty() && fire.Size() < fire.GetReserved() )
		{
			CSoldier *pSoldier = rest.GetMaxEl();
			rest.Pop();
			PushToFire( pSoldier );
		}

		if ( pLockingUnit && ( !pLockingUnit->IsRefValid() && !pLockingUnit->IsAlive() ) )
			Unlock( pLockingUnit );
	}

	ExchangeSoldiersToTurrets();

	// сегменты у turrets
	for ( int i = 0; i < fire.Size(); ++i )
	{
		if ( IsValidObj( fire[i] ) )
		{
			if ( fire[i]->GetSlot() >= 0 && fire[i]->GetSlot() < turrets.size() && turrets[fire[i]->GetSlot()] != 0 )
				turrets[fire[i]->GetSlot()]->Segment();
		}
	}
	if ( theDipl.GetNParty( GetPlayer() ) == theDipl.GetNeutralParty() && GetNDefenders() != 0 )
	{
		if ( !bNewtralInfantryInside )
		{
			bNewtralInfantryInside = true;
			updater.AddUpdate( 0, ACTION_NOTIFY_MODIFY_ENTRANCE_STATE, this, false );
		}
	}
	else if ( bNewtralInfantryInside )
	{
		bNewtralInfantryInside = false;
		updater.AddUpdate( 0, ACTION_NOTIFY_MODIFY_ENTRANCE_STATE, this, true );
	}
}

void CBuilding::KillAllInsiders()
{
	list< CPtr<CSoldier> > dead;

	for ( int i = 0; i < medical.Size(); ++i )
		dead.push_back( medical[i] );

	for ( int i = 0; i < rest.Size(); ++i )
		dead.push_back( rest[i] );

	for ( int i = 0; i < fire.Size(); ++i )
		dead.push_back( fire[i] );

	for ( list< CPtr<CSoldier> >::iterator iter = dead.begin(); iter != dead.end(); ++iter )
		(*iter)->Die( false, 0 );
}

void CBuilding::Die( const float fDamage )
{
	if ( !bEscaped )
		KillAllInsiders();

	RemoveTransparencies();
	timeOfDeath = curTime;
}

void CBuilding::SetHitPoints( const float fNewHP ) 
{ 
	if ( fNewHP != fHP )
	{
		fHP = Min( fNewHP, GetStats()->fMaxHP );
		SetAlive( fHP > 0.0f );
		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );

		if ( fHP > GetEscapeHitPoints() )
			bShouldEscape = true;

		if ( fHP > 0.0f )
			timeOfDeath = 0.0f;
		else 
			RemoveTransparencies();
	}	
}

void CBuilding::DriveOut( CSoldier *pSoldier, hash_set<int> *pFormations )
{
	NI_ASSERT( GetNEntrancePoints() != 0, "building without entrance points" );

	CFormation *pFormation = pSoldier->GetFormation();
	const int nFormationID = pSoldier->GetFormation()->GetUniqueId();
	if ( GetNEntrancePoints() && pFormations->find( nFormationID ) == pFormations->end() )
	{
		pFormations->insert( nFormationID );
		const int nEntrancePoint = NRandom::Random( 0, GetNEntrancePoints() - 1 );
		const CVec2 vEntrancePoint( GetEntrancePoint( nEntrancePoint ) );
		CVec2 vDirFromCenter( vEntrancePoint - CVec2(GetCenter().x,GetCenter().y) );
		Normalize( &vDirFromCenter );
		const CVec2 vPointsToGo( vEntrancePoint + vDirFromCenter * SAIConsts::TILE_SIZE * 3.0f );

		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_LEAVE, vPointsToGo ), pFormation, false );
	}
}

void CBuilding::TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	if ( fHP > 0 )
	{
		Alarm();

		if ( bFromExplosion )
		{
			if ( theCheats.GetFirstShoot( nPlayerOfShoot ) == 1 )
				fHP = 0.0f;
			else
				fHP -= fDamage;

			fHP = Max( fHP, ( theBonusSystem.IsKeyBuilding( nLinkID ) ) ? 0.1f : 0.0f );
			SetAlive( fHP > 0.0f );
			bEscaped = GetHitPoints() <= GetEscapeHitPoints() && bShouldEscape;
			if ( bEscaped )
			{
				hash_set<int> formations;
				for ( int i = 0; i < fire.Size(); ++i )
					DriveOut( fire[i], &formations );
				for ( int i = 0; i < medical.Size(); ++i )
					DriveOut( medical[i], &formations );
				for ( int i = 0; i < rest.Size(); ++i )
					DriveOut( rest[i], &formations );
			}
			
			// все убиты
			if ( GetHitPoints() == 0.0f )
			{
				// хранилища не умирают
				if ( TYPE_MAIN_RU_STORAGE != pStats->etype &&
						 TYPE_TEMP_RU_STORAGE != pStats->etype )
				{
					theStatistics.ObjectDestroyed( nPlayerOfShoot );

					if ( !bEscaped )
					{
						for ( int i = 0; i < fire.Size(); ++i )
						{
							if ( pShotUnit )
								pShotUnit->EnemyKilled( fire[i] );

							theStatistics.UnitKilled( nPlayerOfShoot, fire[i]->GetPlayer(), fire[i]->GetStats()->fExpPrice, 
								pShotUnit ? pShotUnit->GetReinforcementType() : NDb::_RT_NONE, fire[i]->GetReinforcementType(), true );
						}
						for ( int i = 0; i < medical.Size(); ++i )
						{
							if ( pShotUnit )
								pShotUnit->EnemyKilled( medical[i] );

							theStatistics.UnitKilled( nPlayerOfShoot, medical[i]->GetPlayer(), medical[i]->GetStats()->fExpPrice, 
								pShotUnit ? pShotUnit->GetReinforcementType() : NDb::_RT_NONE, medical[i]->GetReinforcementType(), true );
						}
						for ( int i = 0; i < rest.Size(); ++i )
						{
							if ( pShotUnit )
								pShotUnit->EnemyKilled( rest[i] );

							theStatistics.UnitKilled( nPlayerOfShoot, rest[i]->GetPlayer(), rest[i]->GetStats()->fExpPrice, 
								pShotUnit ? pShotUnit->GetReinforcementType() : NDb::_RT_NONE, rest[i]->GetReinforcementType(), true );
						}
					}
				}

				Die( fDamage );
			}
			else
			{
				if ( !bEscaped )
				{
					const float fProbability = fDamage / pStats->fMaxHP;

					list< CPtr<CSoldier> > dead;								
					for ( int i = 0; i < medical.Size(); ++i )
					{
						// не жилец
						if ( NRandom::Random( 0.0f, 1.0f ) < fProbability )
							dead.push_back( medical[i] );
					}

					for ( int i = 0; i < rest.Size(); ++i )
					{
						// не жилец
						if ( NRandom::Random( 0.0f, 1.0f ) < fProbability )
							dead.push_back( rest[i] );
					}
						
					for ( int i = 0; i < fire.Size(); ++i )
					{
						// не жилец
						if ( NRandom::Random( 0.0f, 1.0f ) < fProbability )
							dead.push_back( fire[i] );
					}

					for ( list< CPtr<CSoldier> >::iterator iter = dead.begin(); iter != dead.end(); ++iter )
					{
						CSoldier *pSoldier = *iter;

						if ( pShotUnit )
							pShotUnit->EnemyKilled( pSoldier );
						
						theStatistics.UnitKilled( nPlayerOfShoot, dead.front()->GetPlayer(), 
							pSoldier->GetStats()->fPrice, pShotUnit ? pShotUnit->GetReinforcementType() : NDb::_RT_NONE, pSoldier->GetReinforcementType(), true );

						pSoldier->Die( false, 0 );
					}
				}

				WasHit();
			}

			updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
		}
	}
}

void CBuilding::Alarm()
{
	bAlarm = true;
}

const int CBuilding::GetNFreePlaces() const 
{ 
	if ( GetHitPoints() > 0 )
		return nOveralPlaces - medical.Size() - fire.Size() - rest.Size();

	return 0;
}

bool CBuilding::IsGoodPointForRunIn( const SVector &point, const int nEntrance, const float fMinDist ) const
{
	return fabs2( point.ToCVec2() - GetEntrancePoint( nEntrance ) ) <= sqr( float( SConsts::TILE_SIZE ) * 6.0f + fMinDist );
}

const int CBuilding::GetNDefenders() const
{
	return medical.Size() + rest.Size() + fire.Size();
}

class CSoldier* CBuilding::GetUnit( const int n ) const
{
	NI_ASSERT( n < GetNDefenders(), "Wrong number of unit to get from defenders of a building" );

	if ( n < fire.Size() )
		return fire[n];
	else if ( n < fire.Size() + rest.Size() )
		return rest[n - fire.Size()];
	return medical[n - fire.Size() - rest.Size()];
}

const BYTE CBuilding::GetPlayer() const
{
	if ( !fire.IsEmpty() )
		return fire[0]->GetPlayer();
	else if ( !rest.IsEmpty() )
		return rest[0]->GetPlayer();
	else if ( !medical.IsEmpty() )
		return medical[0]->GetPlayer();

	return theDipl.GetNeutralPlayer();
}

void CBuilding::Lock( CCommonUnit *pUnit )
{
	pLockingUnit = pUnit;
}

bool CBuilding::IsLocked( const int nPlayer ) const
{
	return pLockingUnit != 0;
}

void CBuilding::Unlock( CCommonUnit *pUnit )
{
	pLockingUnit = 0;
}

const int CBuilding::GetNGunsInFireSlot( const int nSlot )
{
	NI_ASSERT( nSlot < guns.size(), StrFmt( "Wrong number of slot (%d)", nSlot ) );

	return guns[nSlot]->GetNTotalGuns();
}

CBasicGun* CBuilding::GetGunInFireSlot( const int nSlot, const int nGun )
{
	NI_ASSERT( nSlot < guns.size(), StrFmt( "Wrong number of slot (%d)", nSlot ) );

	if ( nGun >= guns[nSlot]->GetNTotalGuns() || guns[nSlot]->GetGun(nGun)->GetWeapon() == 0 )
		return 0;

	return guns[nSlot]->GetGun(nGun);
}

CTurret* CBuilding::GetTurretInFireSlot( const int nSlot )
{
	NI_ASSERT( nSlot < guns.size(), StrFmt( "Wrong number of slot (%d)", nSlot ) );
	return turrets[nSlot];
}

float CBuilding::GetMaxFireRangeInSlot( const int nSlot ) const
{
	NI_ASSERT( nSlot < guns.size(), StrFmt( "Wrong number of slot (%d)", nSlot ) );
	return guns[nSlot]->GetMaxFireRange( 0 );
}

bool CBuilding::IsSoldierVisible( const int nParty, const CVec2 &center, bool bCamouflated, const float fCamouflage ) const
{
	if ( nParty == theDipl.GetNParty( GetPlayer() ) )
		return true;

	for ( int i = 0; i < theDipl.GetNPlayers(); ++i )
	{
		if ( theDipl.GetDiplStatusForParties( nParty, theDipl.GetNParty( i ) ) == EDI_FRIEND && 
				 GetNFriendlyAttackers( i ) > 0 )
			return true;
	}

	if ( g_bNewLock == 0 )
	{
		CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > pass;
		GetPassability( &pass );

		const int nSizeX = pass.GetMaxX() - pass.GetMinX();
		const int nSizeY = pass.GetMaxY() - pass.GetMinY();

		const int nYShift = ( center.y - pass.GetMinY() );
		const int nXShift = ( center.x - pass.GetMinX() );

		const int nYDown = ( nYShift <= nSizeY / 2 ) ? 0 : nSizeY / 2;
		const int nXDown = ( nXShift <= nSizeX / 2 ) ? 0 : nSizeX / 2;
		const int nYUp	 = ( nYShift <= nSizeY / 2 ) ? nSizeY / 2 + 1 : nSizeY;
		const int nXUp   = ( nXShift <= nSizeX / 2 ) ? nSizeX / 2 + 1 : nSizeX;

		for ( int y = pass.GetMinY() + nYDown; y < pass.GetMinY() + nYUp; y += SConsts::TILE_SIZE )
		{
			for ( int x = pass.GetMinX() + nXDown; x < pass.GetMinX() + nXUp; x += SConsts::TILE_SIZE )
			{
				if ( pass.GetVal( CVec2( x, y ) ) )
				{
					const SVector tile( AICellsTiles::GetTile( x, y ) );
					if ( theWarFog.IsUnitVisible( tile, nParty, bCamouflated /*, fCamouflage*/ ) )
						return true;
				}
			}
		}
	}
	else
	{
		const CTRect<float> &aabbRect = GetPassProfile()->GetAABBRect();
		const CTPoint<float> vAABBCenter = aabbRect.GetCenter();
		vector<SVector> tiles = GetPassProfile()->GetTilesUnder();
		for ( int i = 0; i < tiles.size(); ++i )
		{
			CVec2 vTileCenter = AICellsTiles::GetPointByTile( tiles[i] );

			if ( Sign( vTileCenter.x - vAABBCenter.x ) * Sign( center.x - vAABBCenter.x ) >= 0 &&
					Sign( vTileCenter.y - vAABBCenter.y ) * Sign( center.y - vAABBCenter.y ) >= 0 &&
					theWarFog.IsUnitVisible( tiles[i], nParty, bCamouflated /*, fCamouflage*/ ) )
			{
				return true;
			}
		}
	}

	return false;
}

bool CBuilding::ChooseEntrance( class CCommonUnit *pUnit, const CVec2 &vPoint, int *pnEntrance ) const
{
	int nMinDistance = 1000000;
	*pnEntrance = -1;

	for ( int i = 0; i < GetNEntrancePoints(); ++i )
	{
		CPtr<IStaticPath> pPath = CreateStaticPathToPoint( GetEntrancePoint( i ), vPoint, VNULL2, pUnit, true, GetAIMap() );
		if ( pPath != 0 && ( *pnEntrance == -1 || pPath->GetLength() < nMinDistance )/* && fabs2( ( pPath->GetFinishPoint() - vPoint ) ) <= sqr( 2 * SConsts::GOOD_LAND_DIST )*/ )
		{
			nMinDistance = pPath->GetLength();
			*pnEntrance = i;
		}
	}

	if ( *pnEntrance == -1 && GetNEntrancePoints() > 0 )
		*pnEntrance = 0;

	return ( *pnEntrance != -1 );
}

bool CBuilding::IsSelectable() const
{
	return GetPlayer() == theDipl.GetMyNumber() &&
				 ( fire.Size() != 0 || medical.Size() != 0 || rest.Size() != 0 );
}

bool CBuilding::CanUnitGoThrough( const EAIClasses &eClass ) const
{
	return ( pStats->nAIPassabilityClass & eClass ) == 0;
}

bool CBuilding::CanRotateSoldier( CSoldier *pSoldier ) const
{
/*	
	// отдыхает
	if ( pSoldier->GetState() && pSoldier->GetState()->IsRestState() ) )
	{
		// солдат в fireplace
		// или не в fireplace, но во время тревоги, или не лечится, или лечится, но уже вылечился 
		if ( pSoldier->IsInFirePlace() || 
				 pSoldier->IsInSolidPlace() && 
				 ( bAlarm || pSoldier->GetSoliderPlaceParameter() != 1 || pSoldier->GetHitPoints() == pSoldier->GetStats()->fMaxHP ) )
			return true;
	}
*/
	return false;
}

void CBuilding::ExchangeUnitToFireplace( CSoldier *pSoldier, int nFirePlace )
{
	CSoldier *pDeletedSoldier = GetSoldierInFireplace( nFirePlace );
	if ( pDeletedSoldier )
		DelSoldier( pDeletedSoldier, false );

	DelSoldier( pSoldier, false );

	PushSoldierToFirePlace( pSoldier, nFirePlace );

	if ( pDeletedSoldier )
		AddSoldier( pDeletedSoldier );
}

const int CBuilding::GetNFirePlaces() const
{
	return pStats->aiSlots.size();
}

CSoldier* CBuilding::GetSoldierInFireplace( const int nFireplace) const
{
	return firePlace2Soldier[nFireplace];
}

void CBuilding::SSwapAction::operator()( CPtr<CSoldier> pSoldier1, CPtr<CSoldier> pSoldier2, const int nSoldier1Index, const int nSoldier2Index )
{
	if ( pSoldier1 )
		pSoldier1->SetSlotIndex( nSoldier2Index );
	if ( pSoldier2 )
		pSoldier2->SetSlotIndex( nSoldier1Index );
}

bool CBuilding::IsBetterChangeObservationSoldier( CSoldier *pSoldier, CSoldier *pSoldierInPoint )
{
	return pSoldier->GetSightRadius() > pSoldierInPoint->GetSightRadius();
}

CSoldier* CBuilding::GetSoldierOnSide( const int nSide )
{
	NI_ASSERT( nSide < 4, StrFmt( "Wrong side passed (%d)", nSide ) );
	for ( int i = 0; i < sides[nSide].nObservationPoints; ++i )
	{
		if ( firePlace2Soldier[observationPlaces[nSide][i]] != 0 )
			return firePlace2Soldier[observationPlaces[nSide][i]];
	}

	return 0;
}

const int CBuilding::GetMiddleObservationPoint( const int nSide ) const
{
	NI_ASSERT( nSide < 4, StrFmt( "Wrong side passed (%d)", nSide ) );

	if ( sides[nSide].nObservationPoints == 0 )
		return -1;
	else if ( sides[nSide].nObservationPoints < 3 )
		return observationPlaces[nSide][0];
	else
		return observationPlaces[nSide][1];
}

void CBuilding::GetSidesObservationPoints( const int nSide, int *pnLeftPoint, int *pnRightPoint ) const
{
	NI_ASSERT( nSide < 4, StrFmt( "Wrong side passed (%d)", nSide ) );

	if ( sides[nSide].nObservationPoints == 0 )
		*pnLeftPoint = *pnRightPoint = -1;
	else if ( sides[nSide].nObservationPoints == 1 )
		*pnLeftPoint = *pnRightPoint = observationPlaces[nSide][0];
	else if ( sides[nSide].nObservationPoints == 2 )
	{
		*pnRightPoint = observationPlaces[nSide][0];
		*pnLeftPoint = observationPlaces[nSide][1];
	}
	else if ( sides[nSide].nObservationPoints == 3 )
	{
		*pnRightPoint = observationPlaces[nSide][0];
		*pnLeftPoint = observationPlaces[nSide][2];
	}
	else
		NI_ASSERT( false, StrFmt( "Wrong number of observation points (%d)", sides[nSide].nObservationPoints ) );
}

const int CBuilding::ChooseSideToSetSoldier( CSoldier *pSoldier ) const
{
	int nSoldierSide = -1;
	if ( pSoldier->IsInFirePlace() )
	{
		const int nSoldierSlot = pSoldier->GetSlot();
		if ( firePlace2Observation[nSoldierSlot] != -1 )
			nSoldierSide = firePlace2Observation[nSoldierSlot] & 3;
	}
	
	int nBestSide = -1;
	int nBestSideSoldiers = -1;
	for ( int i = 0; i < 4; ++i )
	{
		int nSoldiersInObservationPoints = sides[i].nSoldiersInObservationPoints;
		if ( i == nSoldierSide )
			--nSoldiersInObservationPoints;
			
		if ( sides[i].nObservationPoints > 0 && nSoldiersInObservationPoints < 2 && nSoldiersInObservationPoints < sides[i].nObservationPoints )
		{
			if ( nBestSide == -1 )
			{
				nBestSide = i;
				nBestSideSoldiers = nSoldiersInObservationPoints;
			}
			else if ( nSoldiersInObservationPoints < nBestSideSoldiers )
			{
				nBestSide = i;
				nBestSideSoldiers = nSoldiersInObservationPoints;
			}
			else if ( nSoldiersInObservationPoints == nBestSideSoldiers && NRandom::Random( 0.0f, 1.0f ) < 0.5f )
			{
				nBestSide = i;
				nBestSideSoldiers = nSoldiersInObservationPoints;
			}
		}
	}

	return nBestSide;
}

const bool CBuilding::IsVisibleForDiplomacyUpdate()
{ 
	// this is storage
	if ( TYPE_TEMP_RU_STORAGE == pStats->etype )
		return true;
	// мы забежали/выбежали
	if ( theDipl.GetNParty( nLastPlayer ) == theDipl.GetMyParty() ||
				theDipl.GetNParty( GetPlayer() ) == theDipl.GetMyParty())
		return true;
	// враг забежал
	else if ( GetPlayer() != theDipl.GetNeutralPlayer() )
		return IsAnyInsiderVisible();
	// враг выбежал
	else
		return IsVisible( theDipl.GetMyParty() );
}

void CBuilding::SetLastLeaveTime( const int nPlayer )
{
	lastLeave[nPlayer] = curTime;
}

void CBuilding::GetRPGStats( struct SAINotifyRPGStats *pStats )
{
	CGivenPassabilityStObject::GetRPGStats( pStats );
	pStats->nSupply = 0;
	for ( int n = 0; n < turrets.size(); ++n )
	{
		if ( turrets[n] && guns[n] )
		{
			for ( int i = 0; i < guns[n]->GetNCommonGuns(); ++i )
			{
				const SBaseGunRPGStats &pGunStats = guns[n]->GetCommonGunStats( n );
				SAINotifyRPGStats::SWeaponAmmo ammo;
				ammo.pStats = pGunStats.pWeapon;
				ammo.nAmmo = guns[n]->GetNAmmo( n );
				pStats->ammo.push_back( ammo );
			}
			return;
		}
	}
}

const float CBuilding::GetMinHP() const
{
	if ( theBonusSystem.IsKeyBuilding( nLinkID ) )
		return 0.01f * GetStats()->fMaxHP;
	else
		return 0.0f;
}

