#include "stdafx.h"

#include "Stats_B2_M1/AnimationType.h"
#include "AnimUnit.h"
#include "AIClassesID.h"
#include "NewUpdater.h"
#include "GroupLogic.h"
#include "Building.h"
#include "Commands.h"
#include "UnitsIterators2.h"
#include "UnitsIterators.h"
#include "AntiArtilleryManager.h"
#include "Cheats.h"
#include "AntiArtillery.h"
#include "Turret.h"
#include "UnitGuns.h"
#include "Shell.h"
#include "AckManager.h"
#include "Probability.h"
#include "Artillery.h"
#include "Aviation.h"
#include "CombatEstimator.h"
#include "Formation.h"
#include "Statistics.h"
#include <float.h>
#include "General.h"
#include "ShootEstimatorInternal.h"
#include "AIUnitInfoForGeneral.h"
#include "GeneralConsts.h"
#include "ScanLimiter.h"
#include "FormationStates.h"
#include "Graveyard.h"
#include "StaticObjectsIters.h"
#include "ExecutorContainer.h"
#include "ExecutorPlaceCharge.h"
#include "ExecutorAmbush.h"
#include "ExecutorCaution.h"
#include "ExecutorAdrenalineRush.h"
#include "ExecutorCounterFire.h"
#include "ExecutorCamouflage.h"
#include "ExecutorSmokeShots.h"
#include "ExecutorLinkedGrenades.h"
#include "ExecutorExactShot.h"
#include "ExecutorTrackTargetting.h"
#include "ExecutorSpyMode.h"
#include "AILogicInternal.h"
#include "ArtilleryStates.h"

#include "Common_RTS_AI/StaticMapHeights.h"

#include "ScenarioTracker.h"
#include "ExecutorSoldierEntrench.h"
#include "ExecutorPlaneDropBombs.h"

#include "UnitCreation.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "System/Commands.h"

#include "BalanceTest.h"
#include "FeedbackSystem.h"
#include "DBAIConsts.h"
#include "GlobalWarFog.h"

extern CFeedBackSystem theFeedBackSystem;
extern CBalanceTest theBalanceTest;
extern CExecutorContainer theExecutorContainer;
extern CUnitCreation theUnitCreation;
extern CSupremeBeing theSupremeBeing;
extern CStaticObjects theStatObjs;
extern CCombatEstimator theCombatEstimator;
extern CAckManager theAckManager;
extern CEventUpdater updater;
extern CGroupLogic theGroupLogic;
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CUnits units;
extern CGlobalWarFog theWarFog;
extern CAntiArtilleryManager theAAManager;
extern SCheats theCheats;
extern CStatistics theStatistics;
extern CScanLimiter theScanLimiter;
extern CDifficultyLevel theDifficultyLevel;
extern CGraveyard theGraveyard;
extern bool g_bUseRoundUnits;

static bool g_bUseSmartScan = true;

static DWORD g_dwTimeForDangerousTurnRound = 3000;

const int MAX_TARGETS_CACHE_SIZE = 15;

REGISTER_SAVELOAD_CLASS( 0x1508D4AC, CAIUnitInfoForGeneral );

//*******************************************************************
//*											  CAIUnit																		*
//*******************************************************************

BASIC_REGISTER_CLASS( CAIUnit );

bool IsMapFullyFree( const SRect &rect, CAIUnit *pUnit )
{
	if ( GetAIMap()->IsRectOnLockedTiles( rect, EAC_ANY ) )
		return false;

	int nMinX = (std::min)( (std::min)( rect.v1.x, rect.v2.x ), (std::min)( rect.v3.x, rect.v4.x ) );
	int nMinY = (std::min)( (std::min)( rect.v1.y, rect.v2.y ), (std::min)( rect.v3.y, rect.v4.y ) );
	int nMaxX = (std::max)( (std::max)( rect.v1.x, rect.v2.x ), (std::max)( rect.v3.x, rect.v4.x ) );
	int nMaxY = (std::max)( (std::max)( rect.v1.y, rect.v2.y ), (std::max)( rect.v3.y, rect.v4.y ) );

	const CVec2 vAABBHalfSize( ( nMinX + nMaxX ) * 0.5f, ( nMinY + nMaxY ) * 0.5f );

	for ( CUnitsIter<0,3> iter( 0, ANY_PARTY, rect.center, (std::max)( vAABBHalfSize.x, vAABBHalfSize.y ) ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pIterUnit = *iter;
		if ( pIterUnit && pIterUnit->IsRefValid() )
		{
			if ( pIterUnit != pUnit && 
				( pIterUnit->IsAlive() || !pIterUnit->GetStats()->IsInfantry() ) &&
				pIterUnit->GetUnitRect().IsIntersected( rect ) )
				return false;
		}
	}

	return true;
}

const CVec2 CAIUnit::GetGunCenter( const int nGun, const int nPlatform ) const
{ 
	const NDb::SUnitBaseRPGStats *pStats = GetStats();
	if ( pStats->IsInfantry() )
		return GetCenterPlain();

	const NDb::SMechUnitRPGStats* pMechStats = checked_cast<const NDb::SMechUnitRPGStats*>( pStats );
	const CVec3 vGunPos = pMechStats->GetPlatform( nUniqueID, nPlatform ).vAIRotatePointPos;
	const CVec2 vGunPos2D( vGunPos.y, -vGunPos.x );
	const CVec2 vCenter2D( GetCenterPlain() );
	const CVec2 vDir( GetFrontDirectionVector() );
	return vCenter2D + (vGunPos2D ^ vDir);
} 

CAIUnit * CAIUnit::GetUnitByUniqueID( const int nUniqueID )
{
	return checked_cast<CAIUnit*>( CLinkObject::GetObjectByUniqueId( nUniqueID ) );
}

void CAIUnit::SendNTotalKilledUnits( const int nPlayerOfShoot, NDb::EReinforcementType eKillerType, NDb::EReinforcementType eDeadType )
{
	theStatistics.UnitKilled( nPlayerOfShoot, GetPlayer(), GetStats()->fExpPrice, eKillerType, eDeadType, IsInfantry() );
}

const SAINotifyHitInfo::EHitType CAIUnit::ProcessExactHit( const SRect &combatRect, const CVec2 &explCoord, const int nRandPiercing, const int nRandArmor ) const
{
	// попали по комбат системе
	if ( combatRect.IsPointInside( explCoord ) )
	{
		// пробили
		if ( nRandPiercing >= nRandArmor && !IsSavedByCover() )
			return SAINotifyHitInfo::EHT_HIT;
		else
			return SAINotifyHitInfo::EHT_REFLECT;
	}
	else
		return SAINotifyHitInfo::EHT_MISS;
}

const int CAIUnit::GetRandArmorByDir( const int nArmorDir, const WORD wAttackDir, const SRect &unitRect )
{
	switch ( nArmorDir )
	{
		case 0:	return GetRandomArmor( unitRect.GetSide( wAttackDir ) );
		case 1: return GetRandomArmor( RPG_BOTTOM );
		case 2: return GetRandomArmor( RPG_TOP );
		default: NI_VERIFY( false, "Wrong armor dir", return 0 );
	}

	return 0;
}

bool CAIUnit::ProcessCumulativeExpl( CExplosion *pExpl, const int nArmorDir, const bool bFromExpl )
{
	if ( !IsInSolidPlace() )
	{
		SRect unitRect = GetUnitRect();
		const CVec3 vExplCoord3D( pExpl->GetExplCoordinates() );
		const CVec2 vExplCoord( vExplCoord3D.x, vExplCoord3D.y );

		// попали визуально
		// DebugTrace( "Player %d: %2.3f, %s", player, fabs( GetCenterPlain() - vExplCoord ), unitRect.IsPointInside( vExplCoord ) ? "true" : "false" );
		if ( fabs( GetVisZ() - vExplCoord3D.z ) <= AI_TILES_IN_VIS_TILE * AI_TILE_SIZE && unitRect.IsPointInside( vExplCoord ) )
		{
			const int nRandArmor = GetRandArmorByDir( nArmorDir, pExpl->GetAttackDir(), unitRect );
			// если бьёт снизу, or with ignore AABBCoef ability on, то не сжимать
			CAIUnit *pWhoFired = pExpl->GetWhoFire();
			if ( nArmorDir != 1 && (IsValidObj(pWhoFired) && !pWhoFired->IsIgnoreAABBCoeff()) )
				unitRect.Compress( GetRemissiveCoeff() ); 

			const SAINotifyHitInfo::EHitType eHitType = ProcessExactHit( unitRect, vExplCoord, pExpl->GetRandomPiercing(), nRandArmor );
			
			if ( pExpl->GetWhoFire() )
			{
				if ( timeLastAttackedAck == 0 )
				{
					if ( GetPlayer() == theDipl.GetMyNumber() )
						theFeedBackSystem.AddFeedbackAndForget( nUniqueID, GetCenterPlain(), EFB_UNDER_ATTACK, -1 );

					timeLastAttackedAck = curTime;
				}
			}

			if ( eHitType == SAINotifyHitInfo::EHT_HIT || theCheats.GetFirstShoot( pExpl->GetPlayerOfShoot() ) == 1 
				|| (IsValidObj(pWhoFired) && pWhoFired->IsTargetingTrack()) )
				// Always hit if cheating or track targetting
				TakeDamage( pExpl->GetRandomDamage(), &pExpl->GetShellStats(), pExpl->GetPlayerOfShoot(), pExpl->GetWhoFire() );

			if ( !GetStats()->IsInfantry() )
				pExpl->AddHitToSend( new CHitInfo( pExpl, this, eHitType, vExplCoord3D ) );
			else
			{
				CSoldier *pSoldier = checked_cast<CSoldier*>(this);
				if ( !pSoldier->IsInBuilding() )
					pExpl->AddHitToSend( new CHitInfo( pExpl, this, eHitType, vExplCoord3D ) );
			}

			return true;
		}
	}

	return false;
}

bool CAIUnit::ProcessAreaDamage( const class CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius )
{
	const CVec3 vExplCoord3D( pExpl->GetExplCoordinates() );

	if ( !IsInSolidPlace() && fabs( GetVisZ() - vExplCoord3D.z ) < 2.0f * SConsts::TILE_SIZE && !IsSavedByCover() )
	{
		SRect unitRect = GetUnitRect();
		if ( nArmorDir != 1 )
			unitRect.Compress( GetRemissiveCoeff() );

		if ( unitRect.IsIntersectCircle( CVec2(vExplCoord3D.x, vExplCoord3D.y ), fSmallRadius ) && 
				GetRandArmorByDir( nArmorDir, pExpl->GetAttackDir(), unitRect ) <= SConsts::ARMOR_FOR_AREA_DAMAGE )
		{
			TakeDamage( pExpl->GetRandomDamage() * SConsts::AREA_DAMAGE_COEFF, &pExpl->GetShellStats(), pExpl->GetPlayerOfShoot(), pExpl->GetWhoFire() );
			return true;
		}
	}

	return false;
}

bool CAIUnit::ProcessBurstExpl( CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius )
{
	// нет точного попадания
	if ( !ProcessCumulativeExpl( pExpl, nArmorDir, true ) )
	{
		ProcessAreaDamage( pExpl, nArmorDir, fRadius, fSmallRadius );
		return false;
	}
	return true;
}

void CAIUnit::IncreaseHitPoints( const float fInc ) 
{ 
	const float fMaxHP = GetStats()->fMaxHP;
	
	if ( fHitPoints != fMaxHP )
	{
		const float fFormerHP = fHitPoints ;
		fHitPoints = (std::min)( fMaxHP, fHitPoints + fInc );
		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
	}
}

int nCntGuns = 0;
float fRange = 0;
void CAIUnit::Init( const CVec2 &center, const int z, const float fHP, const WORD dir, const BYTE _player, ICollisionsCollector *pCollisionsCollector )
{
	lastScanTime = curTime;
	realScanDuration = NRandom::Random( 1500, 2500 );
	targetScanRandom = NRandom::Random( 800, 1000 );
	//nAbilityLevel = -1;
	bTrampled = false;
	pStatsModifiers = new NDb::SUnitStatsModifier;
	bIgnoreAABBCoeff = false;
	fCamoflage = 1.0f;
	bRestInside = false;
	eReinforcementType = NDb::_RT_NONE;
	timeToDeath = 0;
	bVirtualTankPit = false;
	bIsInTankPit = false;
	creationTime = curTime;
	player = _player;
	fHitPoints = fHP;
	fTakenDamagePower = 0.0f;
	nGrenades = 0;
	bFreeEnemySearch = false;
	SetAlive( true );
//	bVisibleByPlayer = false;
	targetScanRandom = 0;

	nVisIndexInUnits = 0;

	bRevealed = false;
	bQueredToReveal = false;
	nextRevealCheck = 0;
	vPlaceOfReveal = VNULL2;

	pAnimUnit = GetStats()->IsInfantry() ?
							MakeObject<IAnimUnit>( AI_ANIM_UNIT_SOLDIER ) : MakeObject<IAnimUnit>( AI_ANIM_UNIT_MECH );
	pAnimUnit->Init( this );

	CCommonUnit::Init( CVec3( center, z ), dir, pCollisionsCollector );
	
	{//reinforcement price init
		const SUnitBaseRPGStats *pStats = GetStats();
		if ( pStats->GetTypeID() == SMechUnitRPGStats::typeID )
			SetPrice( checked_cast<const SMechUnitRPGStats*>( pStats )->fReinforcementPrice );
	}


	//
	InitGuns();
	dwForbiddenGuns = InitSupportAntiAircraftGuns();

	//
	fCamoflage = 1.0f;
	wVisionAngle = SConsts::STANDART_VIS_ANGLE;

	camouflateTime = 0;
	lastAckTime = 0;

	pUnitInfoForGeneral = new CAIUnitInfoForGeneral( this );

	theGroupLogic.RegisterSegments( this, dynamic_cast<CAILogic*>(Singleton<IAILogic>())->IsFirstTime(), true );
	theGroupLogic.RegisterPathSegments( this, dynamic_cast<CAILogic*>(Singleton<IAILogic>())->IsFirstTime() );

	bAlwaysVisible = ( GetStats()->szParentName == "CoastBattery_Todt" );
	visible4Party.resize( 3, false );
	lastTimeOfVis.resize( 3, 0 );
	bCountToDissapear.resize( 3, false );
	
	bHoldingSector = false;
	bTargetingTrack = false;
	
	// initially dead units
	if ( fHP == 0.0f )
	{
		bAlwaysVisible = true;
		timeToDeath = 0;
	}
	UpdateUnitProfile();
	StaticLockTiles();
	timeLastAttacked = 0;
	CheckAmmoStatus();
	//NI_ASSERT( CAIUnit::GetSightRadius() / ( 2 * SConsts::TILE_SIZE ) <= theWarFog.GetMaxRadius(),
	//	StrFmt( "Invalid sight radius for unit %s", NDb::GetResName( GetStats() ) ) );
}

void CAIUnit::GetNewUnitInfo( SNewUnitInfo *pNewUnitInfo )
{
	GetPlacement( pNewUnitInfo, 0 );
	
	pNewUnitInfo->pStats = GetStats();
	pNewUnitInfo->eDipl = theDipl.GetDiplStatus( theDipl.GetMyNumber(), GetPlayer() );
	pNewUnitInfo->nFrameIndex = /*GetScenarioUnit() ? GetScenarioUnit()->GetScenarioID() : */-1;
	pNewUnitInfo->fHitPoints = GetHitPoints();
	pNewUnitInfo->fResize = 1.0f;
	pNewUnitInfo->nPlayer = GetPlayer();
	pNewUnitInfo->eReinfType = GetReinforcementType();
	// Since this level is used to count abilities, send ability level
	pNewUnitInfo->nExpLevel = theStatistics.GetAbilityLevel( GetPlayer(), GetReinforcementType() );
}
/*

void CAIUnit::GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff )
{ 
	NI_ASSERT( timeDiff <= SConsts::AI_SEGMENT_DURATION, StrFmt( "wrong segment time %i", timeDiff ) );
	const CVec3 vSpeed( GetSpeed() * GetDirectionVector(), 0.0f );
	const CVec3 vPosition( GetCenter() - timeDiff * vSpeed );

	pPlacement->center.x = vPosition.x;
	pPlacement->center.y = vPosition.y;
	pPlacement->z = vPosition.z;
	pPlacement->dir = GetFrontDirection();

	pPlacement->fSpeed = GetSpeed();
	pPlacement->dwNormal = GetNormale( pPlacement->center );
	pPlacement->nObjUniqueID = GetUniqueID();

	const SVector tile = AICellsTiles::GetTile( pPlacement->center );
	pPlacement->cSoil = GetTerrain()->GetSoilType( GetCenterTile() );
}
*/

void CAIUnit::PrepareToDelete()
{
	if ( IsAlive() )
	{
		SetAlive( false );
		if ( theBalanceTest.IsActive() )
			theBalanceTest.UnitDead( this );

		Stop();

		GetTerrain()->RemoveTemporaryUnlocking( GetUniqueId() );

		if ( pAntiArtillery != 0 )
			theAAManager.RemoveAA( pAntiArtillery );

		theGroupLogic.DelUnitFromGroup( this );
		theGroupLogic.DelUnitFromSpecialGroup( this );
		DelCmdQueue( GetUniqueId() );
		
		pAntiArtillery = 0;

		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
		theAckManager.UnitDead( this );
		theCombatEstimator.DelUnit( this );
		pUnitInfoForGeneral->Die();
		theFeedBackSystem.RemovedAllFeedbacks( nUniqueID );

		GetState()->TryInterruptState( 0 );
		SetOffTankPit();

		theGroupLogic.UnregisterSegments( this );
	}
}

void CAIUnit::Disappear()
{
	PrepareToDelete();
	theGraveyard.AddToDissapeared( this );
}

void CAIUnit::DieTrain( const float fDamage )
{
	if ( IsAlive() )
	{
		SetAlive( false );

		if ( GetStats()->etype == RPG_TYPE_TRAIN_LOCOMOTIVE || GetStats()->etype == RPG_TYPE_TRAIN_ARMOR )
			Stop();
		GetState()->TryInterruptState( 0 );
		
		if ( pAntiArtillery != 0 )
			theAAManager.RemoveAA( pAntiArtillery );

		theGroupLogic.DelUnitFromGroup( this );
		theGroupLogic.DelUnitFromSpecialGroup( this );
		DelCmdQueue( GetUniqueId() );
		
		pAntiArtillery = 0;
		CalcVisibility( true );

		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );

		//theGraveyard.AddToSoonBeDead( this, fDamage );
		//const int nFatality = ChooseFatality( fDamage );
		//updater.AddUpdate( 0, ACTION_NOTIFY_DEAD_UNIT, new CDeadUnit( this, curTime, GetDieAction(), nFatality, false ), -1 );

		theAckManager.UnitDead( this );
		theCombatEstimator.DelUnit( this );
		
		//ForceLockTiles();
		if ( GetParty() < 2 )
			theWarFog.DeleteUnit( GetUniqueId() );
	}
}

void CAIUnit::Die( const bool fromExplosion, const float fDamage )
{
	if ( IsAlive() )
	{
		if ( GetStats()->IsTrain() )
			DieTrain( fDamage );
		else
		{
			if ( GetStats()->etype == RPG_TYPE_SNIPER && GetPlayer() == theDipl.GetMyNumber() )
				updater.AddUpdate( EFB_SNIPER_DEAD, MAKELONG( GetCenter().x, GetCenter().y ) );
			
			if ( theBalanceTest.IsActive() && !theBalanceTest.HasShoot( GetPlayer() ) )
			{
				for ( int i = 0; i < GetNCommonGuns(); ++i )
				{
					const SBaseGunRPGStats &rStats = GetCommonGunStats( i );
					if ( rStats.nAmmo != GetNAmmo( i ) )
					{
						theBalanceTest.SetShoot( GetPlayer() );
						break;
					}
				}
			}
			PrepareToDelete();

			//{
			// for soldier's only, to ensure that soldiers don't die simualteneously
			// better to create executors for that
			theGraveyard.AddToSoonBeDead( this, fDamage );

			if ( fromExplosion && GetStats()->IsInfantry() && IsFree() )
				timeToDeath = curTime + NRandom::Random( 0, 1000 );
			else
				timeToDeath = curTime;
			//}

		}

		theStatistics.UnitDead( this );
	}
}

const bool CAIUnit::IsVisible( const BYTE cParty ) const
{
	return visible4Party[cParty];
}

bool CAIUnit::CalculateUnitVisibility4Party( const BYTE party )
{
	const int bVisibility = CalculateUnitVisibility4PartyInner( party );
	return UpdateUnitVisibilityForParty( party, bVisibility );
}

bool CAIUnit::CalculateUnitVisibility4PartyInner( const BYTE party ) const
{
	// NI_ASSERT( !bAlwaysVisible, "always visible, but not coast battery" );
	if ( bAlwaysVisible )
		return true;
	else if ( theDipl.GetNeutralParty() == party )
		return false;
	else
	{
		const BYTE cParty = theDipl.GetNParty( player );

		return GetStats()->IsAviation() || party == cParty || theWarFog.IsUnitVisible( GetCenterTile(), party, IsCamoulflated());
	}
}

void CAIUnit::UpdateCrewAndTruckVisibility( BYTE party,  bool bNewVisibility ) 
{
	if ( GetStats()->IsArtillery() )
	{
		CFormation *pServingFormation = (checked_cast<CArtillery*>(this))->GetCrew();
		if ( pServingFormation )
		{
			for ( int i = 0; i < pServingFormation->Size(); ++i )
			{
				(*pServingFormation)[i]->SetVisibility( party, bNewVisibility );
				if ( party == theCheats.GetNPartyForWarFog() )
					updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_VISIBILITY, (*pServingFormation)[i], bNewVisibility );
			}
		}
		if ( GetState()->GetName() == EUSN_BEING_TOWED )
		{
			CAIUnit * pTransport = checked_cast<CArtilleryBeingTowedState*>( GetState() )->GetTowingTransport();
			if ( pTransport )
			{
				pTransport->SetVisibility( party, bNewVisibility );
				if ( party == theCheats.GetNPartyForWarFog() )
					updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_VISIBILITY, pTransport, bNewVisibility );
			}
		}
	}	
}

bool CAIUnit::UpdateUnitVisibilityForParty( const BYTE party, const bool bVisibility )
{
	const int bVisibilityChanged = int(bVisibility) != visible4Party[party];
	int bNewVisibility = visible4Party[party];
	if ( lastTimeOfVis[party] == 0 && ( party == theCheats.GetNPartyForWarFog() ||
																			party == theDipl.GetMyParty()) )
	{
		UpdateCrewAndTruckVisibility( party, bVisibility );
		updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_VISIBILITY, this, bVisibility );
		lastTimeOfVis[party] = curTime;
	}

	if ( bVisibilityChanged )
	{
		if ( !bVisibility )
		{
			// remember time to switch off visibility 
			if ( bCountToDissapear[party] )
			{
				// прошло достаточно большое время после ухода в невидимость (чтобы не мигал)
				if ( lastTimeOfVis[party] + SConsts::RESIDUAL_VISIBILITY_TIME < curTime )
				{
					bCountToDissapear[party] = false;
					bNewVisibility = bVisibility;
				}
			}
			else
			{
				lastTimeOfVis[party] = curTime;
				bCountToDissapear[party] = true;
			}
		}
		// виден 
		else 
		{
			bCountToDissapear[party] = false;
			bNewVisibility = bVisibility;
		}

		if ( visible4Party[party] != bNewVisibility )
		{
			if ( party == theCheats.GetNPartyForWarFog() ) // update client if visibility for client changed
			{
				updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_VISIBILITY, this, bNewVisibility );
				
				if ( theDipl.GetMyParty() != GetParty() )
				{
					if ( bNewVisibility && GetStats()->IsAviation() )
						theFeedBackSystem.AddFeedbackAndForget( GetUniqueId(), GetCenterPlain(), EFB_ENEMY_SIGHTED, -1 );
				}
				if ( IsInTankPit() )
					UpdateTankPitVisibility( bVisibilityChanged, bNewVisibility );
				if ( GetStats()->IsArtillery() )
					(checked_cast<CArtillery*>(this))->UpdateAmmoBoxVisibility( bVisibilityChanged, bNewVisibility );
			}

			UpdateCrewAndTruckVisibility( party, bNewVisibility );
		}
		return bNewVisibility;
	}
	return visible4Party[party]; // not changed
}

const DWORD CAIUnit::GetNormale() const
{
	return CCommonUnit::GetNormale( GetCenterPlain() );
}

void CAIUnit::GetTilesForVisibility( CTilesSet *pTiles ) const 
{ 
	pTiles->clear();
	const SVector tile = GetCenterTile();
	if ( GetAIMap()->IsTileInside( tile ) )
		pTiles->push_back( tile );
}

bool CAIUnit::ShouldSuspendAction( const EActionNotify &eAction ) const
{
	return 
		( (eAction == ACTION_NOTIFY_DEAD_UNIT && curTime != 0 && !theGraveyard.IsDiedVisible( GetUniqueID() ) )|| 
			eAction == ACTION_NOTIFY_GET_DEAD_UNITS_UPDATE ||
			eAction == ACTION_NOTIFY_NEW_ST_OBJ );
}

void CAIUnit::CheckForReveal()
{
	if ( !theDipl.IsNetGame() && nextRevealCheck <= curTime )
	{
		nextRevealCheck = curTime + 1000 + NRandom::Random( 0, 1000 );
		if ( bQueredToReveal )
		{
			bQueredToReveal = false;
			if ( !bRevealed )
			{
				bRevealed = NRandom::Random( 0.0f, 1.0f ) < SConsts::REVEAL_INFO[GetStats()->etype].fRevealByQuery;
				if ( bRevealed )
					timeOfReveal = curTime;
			}

			vPlaceOfReveal = GetCenterPlain();
		}
		else if ( bRevealed )
		{
			const int nType = GetStats()->etype;
			bRevealed =
				timeOfReveal + SConsts::REVEAL_INFO[nType].nTimeOfReveal >= curTime &&
				fabs2( GetCenterPlain() - vPlaceOfReveal ) < sqr(SConsts::REVEAL_INFO[nType].fForgetRevealDistance) && 
				NRandom::Random( 0.0f, 1.0f ) < 1 - SConsts::REVEAL_INFO[nType].fRevealByMovingOff;
		}
	}
}

void CAIUnit::Segment()
{
	if ( IsAlive() )
		CCommonUnit::Segment();

	bIsInTankPit = IsValidObj( pTankPit );

	if ( pAntiArtillery )
	{
		pAntiArtillery->Segment( IsVisible( theDipl.GetMyParty() ) );
		if ( theDipl.GetNeutralPlayer() != GetPlayer() )
		{
			const int nEnemyParty = 1 - GetParty();
			pUnitInfoForGeneral->UpdateAntiArtFire( 
				pAntiArtillery->GetLastHeardTime( nEnemyParty ), 
				pAntiArtillery->GetRevealCircle( nEnemyParty ).center 
			);
		}
	}
	
	AnimationSegment();	

	CalcVisibility( false );
	CheckForReveal();
	units.UpdateUnitVis4Enemy( this );
	if ( timeLastAttackedAck != 0 && timeLastAttackedAck + 20000 < curTime )
	{
		// if ( GetPlayer() == theDipl.GetMyNumber() )
			// theFeedBackSystem.RemoveFeedback( nUniqueID, EFB_UNDER_ATTACK );
		timeLastAttackedAck = 0;
	}
}

void CAIUnit::FreezeSegment()
{
	if ( !theDipl.IsNetGame() )
	{
		if ( GetPlayer() != theDipl.GetMyNumber() && !GetStats()->IsAviation() )
		{
			if ( IsVisibleByPlayer() && GetSpeed() != 0.0f )
				theCombatEstimator.AddUnit( this );
			else
				theCombatEstimator.DelUnit( this );
		}

		pUnitInfoForGeneral->Segment();
	}

	// обработка TankPit
	if ( !IsValidObj( pTankPit ) )
	{
		pTankPit = 0;
		bIsInTankPit = false;
	}

	if ( creationTime + 1000 < curTime )
		CCommonUnit::FreezeSegment();

	/*
	if ( GetState() && GetState()->IsRestState() || GetState()->GetName() == EUSN_USE_SPYGLASS )
		AnalyzeCamouflage();
	*/

	CalcVisibility( false );
	CheckForReveal();

	units.UpdateUnitVis4Enemy( this );
}

const bool CAIUnit::IsVisibleByPlayer()
{
	return visible4Party[theCheats.GetNPartyForWarFog()];
//	return bVisibleByPlayer;
}

void CAIUnit::CalcVisibility( const bool bIgnoreTime )
{
	if ( bIgnoreTime || curTime >= creationTime + SConsts::AI_SEGMENT_DURATION * SConsts::SHOW_ALL_TIME_COEFF )
	{
		for ( int i = 0; i < 3; ++i )
		{
			const bool bVisibility = CalculateUnitVisibility4Party( i );
			const bool bVisibilityChanged = bool(visible4Party[i]) != bVisibility;
			visible4Party[i] = bVisibility;
			if ( bVisibilityChanged )
			{
				theSupremeBeing.SetUnitVisible( this, i, bVisibility );
				if ( theDipl.GetNeutralPlayer() != GetPlayer() && !theDipl.IsAIPlayer( GetPlayer() ) && theDipl.IsAIPlayer( i ) )
					pUnitInfoForGeneral->UpdateVisibility( bVisibility );

				if ( bVisibility && GetParty() == theDipl.GetNeutralParty() )
					UpdateUnitsRequestsForResupply();
			}
		}
		creationTime = curTime;
	}
}

void CAIUnit::GetRPGStats( SAINotifyRPGStats *pStats ) 
{ 
	pStats->fHitPoints = GetHitPoints();
	pStats->nObjUniqueID = GetUniqueId();

	pStats->nSupply = 0;
	for ( int i = 0; i < GetNCommonGuns(); ++i )
	{
		const SBaseGunRPGStats &pGunStats = GetCommonGunStats( i );
		SAINotifyRPGStats::SWeaponAmmo ammo;
		ammo.pStats = pGunStats.pWeapon;
		ammo.nAmmo = GetNAmmo( i );
		pStats->ammo.push_back( ammo );
	}
	
	pStats->time = curTime;
}

bool CAIUnit::CanCommandBeExecuted( CAICommand *pCommand )
{
	return GetStats()->HasCommand( int( pCommand->ToUnitCmd().nCmdType ) );
}

bool CAIUnit::CanCommandBeExecutedByStats( CAICommand *pCommand )
{
	return CanCommandBeExecutedByStats( pCommand->ToUnitCmd().nCmdType);
}

bool CAIUnit::CanCommandBeExecutedByStats( int nCmd ) const
{
	return GetStats()->HasCommand( nCmd );
}

const float CAIUnit::GetSightRadius() const
{
	return GetStatsModifier()->sightRange.Get( GetStats()->fSight ) * SConsts::TILE_SIZE;
}

void CAIUnit::ApplyStatsModifier( const NDb::SUnitStatsModifier *pModifier, const bool bForward )
{
	if ( pModifier )
	{
		pStatsModifiers->Apply( *pModifier, bForward );
		//NI_ASSERT( CAIUnit::GetSightRadius() / ( 2 * SConsts::TILE_SIZE ) <= theWarFog.GetMaxRadius(),
		//	StrFmt( "DESIGNERS BUG: Invalid sight radius for unit %s after appling modificator %s", NDb::GetResName( GetStats() ), NDb::GetResName( pModifier ) ) );
	}
}

const float CAIUnit::GetCamouflage() const 
{
	return fCamoflage;
}

void CAIUnit::TakeDamage( const float _fDamage, const SWeaponRPGStats::SShell *pShell, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	if ( fHitPoints > 0 )
	{
		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );

		const float fFormerHP = fHitPoints;
		const int nPartyOfShoot = theDipl.GetNParty( nPlayerOfShoot );
		const float fDamage = nPartyOfShoot == theDipl.GetNeutralParty() ? _fDamage : GetStatsModifier()->durability.Get( _fDamage );
		if ( ( fHitPoints -= fDamage * ( 1 - theCheats.GetImmortals( GetPlayer() ) ) ) <= 0 || theCheats.GetFirstShoot( nPlayerOfShoot ) == 1 )
		{
			if ( pShotUnit && pShotUnit->IsAlive() )
			{
				pShotUnit->EnemyKilled( this );
				SendNTotalKilledUnits( nPlayerOfShoot, pShotUnit->GetReinforcementType(), GetReinforcementType() );
			}
			else
			{
				SendNTotalKilledUnits( nPlayerOfShoot, NDb::_RT_NONE, GetReinforcementType() );			// It was a mine (or the unit is dead)
			}

			bool bDisappear = false;
			if ( !IsFree() )
			{
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_DISAPPEAR ), this, false );
				bDisappear = true;
			}
			// убить либо удалить с карты
			if ( !GetStats()->IsAviation() && GetTerrain()->IsBridge( GetCenterTile() ) )
			{
				STerrainModeSetter modeSetter( ELM_STATIC, GetTerrain() );
				if ( !bDisappear )
				{
					bDisappear = GetTerrain()->IsLocked( GetCenterTile(), EAC_ANY );
					if ( bDisappear )
						theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_DISAPPEAR ), this, false );
				}
				//if ( GetStats()->IsInfantry() )
					theGraveyard.AddBridgeKilledSoldier( GetCenterTile(), this );
			}

			if ( !bDisappear )
			{
				const bool bFromExpl = ( pShell != 0 && pShell->fArea2 != 0 );
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_DIE, fDamage, bFromExpl ), checked_cast<CAIUnit*>( this ), false );
			}
		}
		else // юнит не умер
		{
			const SUnitBaseRPGStats *pStats = GetStats();
			if ( !pStats->IsAviation() )
			{
				const float fMaxHP = pStats->fMaxHP;
				if ( fFormerHP / fMaxHP > SConsts::LOW_HP_PERCENTAGE && fHitPoints / fMaxHP <= SConsts::LOW_HP_PERCENTAGE )
					SendAcknowledgement( NDb::ACK_LOW_HIT_POINTS );

				if ( fFormerHP == fMaxHP && fHitPoints != fMaxHP )
					theSupremeBeing.UnitAskedForResupply( this, pStats->IsInfantry() ? ERT_MEDICINE : ERT_REPAIR, true );
			}

			if ( IsValid( pShotUnit ) && !pShotUnit->IsAviation() && theDipl.GetDiplStatus( pShotUnit->GetPlayer(), GetPlayer() ) == EDI_ENEMY )
				timeLastAttacked = curTime;
		}
	}
}

const bool CAIUnit::CanTurnRound() const
{
	return curTime > timeLastAttacked + g_dwTimeForDangerousTurnRound;
}

void CAIUnit::Fired( const float fGunRadius, const int nGun )
{
	if ( pAntiArtillery != 0 && fGunRadius != 0.0f )
		pAntiArtillery->Fired( fGunRadius, GetCenterPlain() );

	if ( !GetStats()->IsAviation() )
	{
		const int nAmmo = GetNAmmo( nGun );
		const int nMaxAmmo = GetCommonGunStats( nGun ).nAmmo;
		if ( nAmmo < nMaxAmmo / 3 && ( nAmmo + 1 ) >= nMaxAmmo / 3 )
		{
			theSupremeBeing.UnitAskedForResupply( this, ERT_RESUPPLY, true );
		}
		if ( nAmmo == 0 && GetPlayer() == theDipl.GetMyNumber() )
		{
			theFeedBackSystem.AddFeedbackAndForget( GetUniqueID(), GetCenterPlain(), EFB_NO_AMMO, -1 );
		}
	}
}

void CAIUnit::UpdateUnitsRequestsForResupply()
{
	if ( !theDipl.IsNetGame() )
	{
		if ( !IsAlive() || IsRefInvalid() ) return;
			// when gun crew is killed, ask to refill it again
		if ( theDipl.GetNParty( GetPlayer() ) == theDipl.GetNeutralParty() )
		{
			if ( GetStats()->IsArtillery() )
				theSupremeBeing.UnitAskedForResupply( this, ERT_HUMAN_RESUPPLY, true );
		}
		else
		{
			const SUnitBaseRPGStats * pStats = GetStats();
			if ( !pStats->IsAviation() )
			{
				const int nCommonGuns = GetNCommonGuns();
				for ( int i = 0; i < nCommonGuns; ++i )
				{
					const int nAmmo = GetNAmmo( i );
					const int nMaxAmmo = GetCommonGunStats( i ).nAmmo;
					if ( nAmmo < nMaxAmmo / 3 )
					{
						theSupremeBeing.UnitAskedForResupply( this, ERT_RESUPPLY, true );
						break;
					}
				}
			}
			
			const float fMaxHP = pStats->fMaxHP;
			if ( GetHitPoints() != fMaxHP )
			{
				if ( pStats->IsInfantry() )
					theSupremeBeing.UnitAskedForResupply( this, ERT_MEDICINE, true );
				else if ( !pStats->IsAviation() )
					theSupremeBeing.UnitAskedForResupply( this, ERT_REPAIR, true );
			}
		}
	}
}

void CAIUnit::ChangePlayer( const BYTE cPlayer )
{
	const BYTE cOldPlayer = GetPlayer();
	if ( GetParty() != theDipl.GetNParty( cPlayer ) )
	{
		theFeedBackSystem.RemovedAllFeedbacks( nUniqueID );
		if ( !theDipl.IsNetGame() )
		{
			theSupremeBeing.UnitChangedParty( this, theDipl.GetNParty( cPlayer ) );
		}
	}

	if ( theBalanceTest.IsActive() && !theBalanceTest.HasShoot( GetPlayer() ) )
	{
		for ( int i = 0; i < GetNCommonGuns(); ++i )
		{
			const SBaseGunRPGStats &rStats = GetCommonGunStats( i );
			if ( rStats.nAmmo != GetNAmmo( i ) )
			{
				theBalanceTest.SetShoot( GetPlayer() );
				break;
			}
		}
	}
	units.ChangePlayer( this, cPlayer );
	SetSelectable( GetPlayer() == theDipl.GetMyNumber(), true );
	updater.AddUpdate( 0, ACTION_NOTIFY_UPDATE_DIPLOMACY, this, -1 );
	
	SWarFogUnitInfo fogInfo;
	GetFogInfo( &fogInfo );
	theWarFog.ChangeParty( GetUniqueId(), fogInfo, theDipl.GetNParty( cPlayer ) );

	const int nGroup = GetNGroup();
	if ( nGroup > 0 )
	{
		theGroupLogic.DelUnitFromGroup( this );
		if ( theGroupLogic.BeginGroup( nGroup ) == theGroupLogic.EndGroup() )
			theGroupLogic.UnregisterGroup( nGroup );
	}


	UpdateUnitsRequestsForResupply();

	updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );

	const int nShellType = GetGuns()->GetActiveShellType();
	updater.AddUpdate( 0, ACTION_NOTIFY_SHELLTYPE_CHANGED, this, nShellType );

	for ( int i = 0; i < GetNGuns(); ++i )
	{
		// for initialization of gun owner party
		// infantry can possess of artillery gun, so we check for owner
		if ( GetGun(i)->GetOwner() == this )
			GetGun( i )->SetOwner( this );
	}
	
	if ( pAntiArtillery )
		pAntiArtillery->SetParty( GetParty() );
	
	if ( GetFormation() )
		GetFormation()->ChangePlayer( cPlayer );

	InitSpecialAbilities();
}

float CAIUnit::GetRemissiveCoeff() const
{
	return GetStatsModifier()->smallAABBCoeff.Get( GetStats()->fSmallAABBCoeff );
}

const NTimer::STime CAIUnit::GetTimeToCamouflage() const
{
	return SConsts::TIME_BEFORE_CAMOUFLAGE;
}

void CAIUnit::CheckAmmoStatus()
{
	EUnitStatus eStats = EUS_RESUPPLY_GROUP;
	const int nCommonGuns = GetNCommonGuns();
	for ( int i = 0; i < nCommonGuns; ++i )
	{
		const int nAmmo = GetNAmmo( i );
		const int nMaxAmmo = GetCommonGunStats( i ).nAmmo;
		if ( nAmmo == 0 )
		{
			eStats = EUS_NO_AMMO;
			break;
		}
		else if ( nAmmo < nMaxAmmo / 3 && eStats == EUS_RESUPPLY_GROUP )
			eStats = EUS_LOW_AMMO;
	}
	updater.AddUpdate( CreateStatusUpdate( eStats, true, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
}

void CAIUnit::ChangeAmmo( const int nCommonGun, const int nAmmo ) 
{ 
	CUnitGuns * pGuns = GetGuns();
//	const int nFormerAmmo = pGuns->GetNAmmo( nCommonGun );
	pGuns->ChangeAmmo( nCommonGun, nAmmo ); 
	updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
	CheckAmmoStatus();
}

void CAIUnit::CreateAntiArtillery( const float fMaxRevealRadius )
{
	pAntiArtillery = new CAntiArtillery( this );
	pAntiArtillery->Init( fMaxRevealRadius, GetParty() );
}

const float CAIUnit::GetKillSpeed( CAIUnit *pEnemy, const DWORD dwGuns ) const
{
	float fSpeed = 0;
	for ( int i = 0; i < sizeof(dwGuns) * 8 && i < GetNGuns(); ++i )
	{
		if ( (dwGuns & i<<i) )
			fSpeed += GetKillSpeed( pEnemy, GetGun( i ) );
	}

	return fSpeed;
}

const float CAIUnit::GetKillSpeed( const SHPObjectRPGStats *pStats, const CVec2 &vCenter, const DWORD dwGuns ) const
{
	float fSpeed = 0;
	const int nGuns = (std::min)( int(sizeof(dwGuns) * 8), GetNGuns() );
	for ( int i = 0; i < nGuns; ++i )
	{
		if ( (dwGuns & i<<i) )
			fSpeed += GetKillSpeed( pStats, vCenter, GetGun( i ) );
	}

	return fSpeed;
}

const float CAIUnit::GetKillSpeed( const SHPObjectRPGStats *pStats, const CVec2 &vCenter, CBasicGun *pGun ) const
{
	float fPiercingProbability = 0.0f;

	static const float weights[4] = { 1.2f, 0.6f, 0.3f, 0.6f };
	
	const int nPiercing = pGun->GetPiercing();
	const int nPiercingRandom = pGun->GetPiercingRandom();
	const int nMinPiercing = nPiercing - nPiercingRandom;
	const int nMaxPiercing = nPiercing + nPiercingRandom;

	if ( pGun->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE )
	{
		for ( int i = 0; i < 4; ++i )
		{
			if ( pGun->CanBreach( pStats, i ) )
			{
				const int nMinArmor = pStats->GetMinPossibleArmor( i );
				const int nMaxArmor = pStats->GetMaxPossibleArmor( i );
				fPiercingProbability += CalculateProbability( nMinPiercing, nMinArmor, nMaxPiercing, nMaxArmor ) * weights[i];
			}
		}
		fPiercingProbability /= ( weights[0] + weights[1] + weights[2] + weights[3] );
	}
	else
	{
		if ( pGun->CanBreach( pStats, RPG_TOP ) )
		{
			const int nMinArmor = pStats->GetMinPossibleArmor( RPG_TOP );
			const int nMaxArmor = pStats->GetMaxPossibleArmor( RPG_TOP );
			fPiercingProbability = CalculateProbability( nMinPiercing, nMinArmor, nMaxPiercing, nMaxArmor );
		}
	}

	

	if ( fPiercingProbability == 0.0f )
		return 0;
	else
	{
		const float fMaxHP = pStats->fMaxHP;

		const int nAmmoPerBurst = pGun->GetWeapon()->nAmmoPerBurst;
		if ( nAmmoPerBurst > 0 )
		{
			const int nBursts = ( fMaxHP / fPiercingProbability ) / ( pGun->GetDamage() * nAmmoPerBurst ) + 1;

			const float fTimeToKill =
				pGun->GetAimTime( false ) + nBursts * ( pGun->GetRelaxTime( false ) + pGun->GetFireRate() * nAmmoPerBurst );

			NI_ASSERT( _finite( nBursts ) != 0, "Wrong nBursts (infinity)" );
			NI_ASSERT( _finite( fTimeToKill ) != 0, "Wrong fTimeToKill (infinity)" );
			NI_ASSERT( fTimeToKill != 0, "Wrong fTimeToKill (0)" );

			return fMaxHP / fTimeToKill;
		}
		else
			return 1.0f;		// For the projector
	}
}

const float CAIUnit::GetKillSpeed( CAIUnit *pEnemy, CBasicGun *pGun ) const
{
	float fPiercingProbability = 0.0f;

	static const float weights[4] = { 1.2f, 0.6f, 0.3f, 0.6f };

	const int nPiercing = pGun->GetPiercing();
	const int nPiercingRandom = pGun->GetPiercingRandom();
	const int nMinPiercing = nPiercing - nPiercingRandom;
	const int nMaxPiercing = nPiercing + nPiercingRandom;

	for ( int i = 0; i < 4; ++i )
	{
		if ( pGun->CanBreach( pEnemy, i ) )
		{
			const int nMinArmor = pEnemy->GetMinPossibleArmor( i );
			const int nMaxArmor = pEnemy->GetMaxPossibleArmor( i );

			fPiercingProbability += CalculateProbability( nMinPiercing, nMinArmor, nMaxPiercing, nMaxArmor ) * weights[i];
		}
	}

	fPiercingProbability /= ( weights[0] + weights[1] + weights[2] + weights[3] );

	if ( fPiercingProbability == 0.0f )
		return 0;
	else
	{
		float fTimeToGo = 0;

		const float fMaxHP = pEnemy->GetStats()->fMaxHP;
		const float fCover = IsValidObj( pEnemy ) ? pEnemy->GetCover() : 1.0f;

		const int nAmmoPerBurst = pGun->GetWeapon()->nAmmoPerBurst;

		if ( nAmmoPerBurst > 0 )
		{
			const int nBursts = ( fMaxHP / ( fPiercingProbability * fCover ) ) / ( pGun->GetDamage() * nAmmoPerBurst ) + 1;

			const float fTimeToKill = 
				pGun->GetAimTime( false ) + nBursts * ( pGun->GetRelaxTime( false ) + pGun->GetFireRate() * nAmmoPerBurst );

			NI_ASSERT( _finite( nBursts ) != 0, "Wrong nBursts (infinity)" );
			NI_ASSERT( _finite( fTimeToKill ) != 0, "Wrong fTimeToKill (infinity)" );
			NI_ASSERT( fTimeToKill != 0, "Wrong fTimeToKill (0)" );

			return fMaxHP / ( fTimeToKill + fTimeToGo );
		}
		else
			return 1.0f; // For the projector
	}
}

const float CAIUnit::GetKillSpeed( CAIUnit *pEnemy ) const
{
	int i = 0;
	while ( i < GetNGuns() && !GetGun(i)->CanShootToUnit( pEnemy ) )
		++i;

	if ( i >= GetNGuns() )
		return 0;

	return GetKillSpeed( pEnemy, GetGun( i ) );
}

void CAIUnit::UpdateTakenDamagePower( const float fUpdate ) 
{ 
	if ( fTakenDamagePower <= 0.0f && fUpdate >= 0 && !GetState()->IsAttackingState() )
	{
		SendAcknowledgement( NDb::ACK_UNDER_ATTACK, true );
	}
	const float fOld = fTakenDamagePower;
	//	NI_ASSERT( fTakenDamagePower + fUpdate >= -0.0001f, "Wrong taken damage power" );
	if ( fTakenDamagePower + fUpdate >= -0.0001f )
	{
		fTakenDamagePower += fUpdate; 
		if ( fTakenDamagePower < 0 && fTakenDamagePower >= -0.0001f )
			fTakenDamagePower = 0.0f;

		NI_ASSERT( fTakenDamagePower >= 0, "Wrong taken damage power" );
	}
}

void CAIUnit::ResetTargetScan()
{
	GetLastBehTime() = 0;
//	SetTargetScanRandom();	
	targetScanRandom = NRandom::Random( 800, 1000 );
}

void CAIUnit::ResetGunChoosing()
{
	GetLastBehTime() = curTime;
//	SetTargetScanRandom();	
	targetScanRandom = NRandom::Random( 800, 1000 );
}

CBasicGun* CAIUnit::AnalyzeGunChoose( CAIUnit *pEnemy )
{
	if ( IsTimeToAnalyzeTargetScan() )
	{
		GetLastBehTime() = curTime;
		SetTargetScanRandom();

		ResetShootEstimator( pEnemy, false, dwForbiddenGuns );
		return GetBestShootEstimatedGun();
	}
	else
		return 0;
}

void CAIUnit::SetTargetScanRandom()
{
	if ( GetStats()->IsInfantry() )
	{
		if ( GetState() && GetState()->IsAttackingState() )
		{
			targetScanRandom = NRandom::Random( 800, 2500 );
		}
		else
		{
			targetScanRandom = NRandom::Random( 800, 3500 );
		}
	}
	else
	{	
		targetScanRandom = NRandom::Random( 800, 1000 );
	}
}

IObstacle* CAIUnit::LookForObstacle()
{
	if ( CanShoot() && theSupremeBeing.MustShootToObstacles( GetPlayer() ) )
	{
		CShootEstimatorForObstacles estimator( this );
		theStatObjs.EnumObstaclesInRange( GetCenterPlain(), GetSightRadius(), &estimator );
		
		return estimator.GetBest();
	}

	return 0;
}

const float CAIUnit::GetTargetScanRadius()
{
	// дальнобойное AI орудие
	if ( theDipl.IsAIPlayer( GetPlayer() ) && GetFirstArtilleryGun() != 0 && !DoesReservePosExist() )
		return GetFirstArtilleryGun()->GetFireRange( 0 );
	else if ( GetStats()->etype == RPG_TYPE_OFFICER )
		return (std::min)( GetGun(0)->GetFireRange( 0 ) * SConsts::OFFICER_COEFFICIENT_FOR_SCAN, GetSightRadius() );
	else if ( GetStats()->IsArtillery() )
		return (std::min)( GetMaxFireRange(), SConsts::MAX_FIRE_RANGE_TO_SHOOT_BY_LINE );
	else
	{
		const float fCallForHelpRadius = theDipl.IsAIPlayer( GetPlayer() ) ? SConsts::AI_CALL_FOR_HELP_RADIUS : SConsts::CALL_FOR_HELP_RADIUS;
		float fFireRange = GetSightRadius();
		for(int i = 0; i < GetNGuns(); i++)
			fFireRange = (std::max)(fFireRange, GetGun(i)->GetFireRange(0.0f));

		return (std::min)( fCallForHelpRadius, fFireRange );
	}
}

bool CAIUnit::LookForTargetInRange(  CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, CBasicGun **pGun, const float fRange, const bool bIteratePlanes, const bool bIterateBuildings )
{
	if ( curTime < lastScanTime + realScanDuration )
	{
		for ( std::list< CPtr<CAIUnit> >::iterator it = targetsCache.begin(); it != targetsCache.end(); )
		{
			CPtr<CAIUnit> pCachedEnemy = *it;
			if ( IsValid( pCachedEnemy ) && pCachedEnemy->IsAlive() && theDipl.GetDiplStatus( GetPlayer(), pCachedEnemy->GetPlayer() ) == EDI_ENEMY )
			{
				AddUnitToShootEstimator( pCachedEnemy );
				break;
			}
			else
				it = targetsCache.erase( it );
		}
		*pBestTarget = GetBestShootEstimatedUnit();
		*pGun = GetBestShootEstimatedGun();
		return true;
	}
	targetsCache.clear();

	const CVec2 vCenter( GetCenterPlain() );

	for ( CUnitsIter<1,3> iter( GetParty(), EDI_ENEMY, vCenter, fRange ); theScanLimiter.CanScan() && !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pTarget = *iter;
		if ( IsValidObj( pTarget ) && pTarget->IsNoticableByUnit( this, fRange ) && pTarget->GetState()->GetName() != EUSN_PARTROOP )
		{
			// для вражеской артиллерии добавить всех артиллеристов
			bool bDoNotAddThisUnit = false;
			if ( pTarget->GetStats()->IsArtillery() )
			{
				CArtillery *pArt = checked_cast<CArtillery*>( pTarget ) ;
				if ( pArt->HasServeCrew() )
				{
					CFormation *pCrew = pArt->GetCrew();
					for ( int i=0; i < pCrew->Size(); ++i )
					{
						CSoldier *pSoldier = (*pCrew)[i];
						if ( pSoldier->IsNoticableByUnit( this, fRange ) )
						{
							AddUnitToShootEstimator( pSoldier );
							if ( targetsCache.size() < MAX_TARGETS_CACHE_SIZE )
								targetsCache.push_back( pSoldier );
							bDoNotAddThisUnit = true;
						}
					}
				}
			}
			if ( !bDoNotAddThisUnit )
			{
				AddUnitToShootEstimator( pTarget );
				if ( targetsCache.size() < MAX_TARGETS_CACHE_SIZE )
					targetsCache.push_back( pTarget );
			}
		}
	}

	// по юнитам в домиках
	if ( bIterateBuildings )
	{
		for ( CStObjCircleIter<true> iter( GetCenterPlain(), fRange ); theScanLimiter.CanScan() && !iter.IsFinished(); iter.Iterate() )
		{
			CExistingObject *pTarget = *iter;

			for ( int i = 0; i < pTarget->GetNDefenders(); ++i )
			{
				if ( theDipl.GetDiplStatus( GetPlayer(), pTarget->GetPlayer() ) == EDI_ENEMY && pTarget->GetUnit( i )->IsNoticableByUnit( this, fRange ) )
				{
					AddUnitToShootEstimator( pTarget->GetUnit( i ) );
					if ( targetsCache.size() < MAX_TARGETS_CACHE_SIZE )
						targetsCache.push_back( pTarget->GetUnit( i ) );
				}
			}
		}
	}

	*pBestTarget = GetBestShootEstimatedUnit();
	*pGun = GetBestShootEstimatedGun();

	// зенитная артиллерия
	if ( bIteratePlanes && *pBestTarget == 0 && CanShootToPlanes() )
	{
		CShootEstimatorLighAA estimatorAA;
		estimatorAA.Init( this );

		for ( CPlanesIter iter; !iter.IsFinished(); iter.Iterate() )
		{
			if ( theDipl.GetDiplStatus( GetPlayer(), (*iter)->GetPlayer() ) == EDI_ENEMY )
				estimatorAA.AddUnit( *iter );
		}

		*pBestTarget = estimatorAA.GetBestUnit();
		*pGun = 0;
	}

	return *pBestTarget;
}

void CAIUnit::LookForTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, CBasicGun **pGun )
{
	if ( IsValid( pCurTarget ) && pCurTarget->IsAlive() )
		return;

	if ( CanShoot() )
	{
		ResetShootEstimator( pCurTarget, bDamageUpdated, dwForbiddenGuns );
		bool bHadTarget = pCurTarget != 0;
		const float fRange = GetTargetScanRadius();
		
		if ( !g_bUseSmartScan 
			|| !LookForTargetInRange( pCurTarget, bDamageUpdated, pBestTarget, pGun, fRange * 0.125f, false, false ) 
			|| !LookForTargetInRange( pCurTarget, bDamageUpdated, pBestTarget, pGun, fRange * 0.250f, false, false ) 
			|| !LookForTargetInRange( pCurTarget, bDamageUpdated, pBestTarget, pGun, fRange * 0.500f, false, false ) 
			|| !LookForTargetInRange( pCurTarget, bDamageUpdated, pBestTarget, pGun, fRange * 0.750f, false, true ) )
				LookForTargetInRange( pCurTarget, bDamageUpdated, pBestTarget, pGun, fRange, true, true );
		
		if ( curTime >= lastScanTime + realScanDuration )
		{
			lastScanTime = curTime;
			if ( GetStats()->IsInfantry() )
			{
				if ( GetState() && GetState()->IsAttackingState() )
					realScanDuration = NRandom::Random( 2500, 6000 );
				else
					realScanDuration = NRandom::Random( 3500, 7000 );
			}
			else
				realScanDuration = NRandom::Random( 1500, 3000 );
		}


		if ( !bHadTarget && *pBestTarget && !(*pBestTarget)->IsAviation() ) // target just found
			SendAcknowledgement( NDb::ACK_ENEMY_FOUND, GetParty() == theDipl.GetMyParty() );
	}
	else
	{
		*pBestTarget = 0;
		*pGun = 0;
	}
}

void CAIUnit::LookForFarTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, CBasicGun **pGun )
{
	CAIUnit *pChosenTarget = *pBestTarget;
	CBasicGun *pChosenGun = *pGun;
	
	SetCircularAttack( true );
	bFreeEnemySearch = true;
	bool bHadTarget = pCurTarget != 0;

	CAIUnit::LookForTarget( pCurTarget, bDamageUpdated, pBestTarget, pGun );
	// цель вне конуса обстрела есть
	if ( (*pBestTarget) != 0 )
	{
		// цель уже под сильным огнём
		const float fDamagePower = (*pBestTarget)->GetTakenDamagePower();

		if ( fDamagePower != 0 )
		{
			const float fKillEnemyTime = (*pBestTarget)->GetHitPoints() / fDamagePower;
			if ( fKillEnemyTime < 2000 && ( (*pBestTarget)->GetStats()->IsInfantry() || (*pBestTarget)->GetStats()->IsTransport() ) )
			{
				*pBestTarget = pChosenTarget;
				*pGun = pChosenGun;
			}
		}
	}
	else
	{
		*pBestTarget = pChosenTarget;
		*pGun = pChosenGun;
	}
	if ( !bHadTarget && *pBestTarget && !(*pBestTarget)->IsAviation() ) // target just found
		SendAcknowledgement( NDb::ACK_ENEMY_FOUND, GetParty() == theDipl.GetMyParty() );

	bFreeEnemySearch = false;
	SetCircularAttack( false );
}

void CAIUnit::SetCircularAttack( const bool bCanAttack )
{
	for ( int i = 0; i < GetNGuns(); ++i )
		GetGun( i )->SetCircularAttack( bCanAttack );
}

bool CAIUnit::IsTimeToAnalyzeTargetScan() const
{
	return curTime - GetLastBehTime() >= GetBehUpdateDuration() + targetScanRandom;
}

const bool CAIUnit::AttackTarget( CAIUnit *pTarget, CAIUnit *pCurTarget )
{
	if ( IsValidObj( pTarget ) && ( pCurTarget == 0 || pTarget != pCurTarget ) )
	{
		if ( GetState()->GetName() == EUSN_MECHUNIT_REST_ON_BOARD )
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_ONBOARD_ATTACK_UNIT, pTarget->GetUniqueId() ), this, false );
		else if ( GetStats()->etype == RPG_TYPE_ART_ROCKET )
			theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ART_BOMBARDMENT, pTarget->GetCenterPlain() ), this );
		else
		{
			const bool bAllowMoving = !GetState()->IsRestState();
			if ( bAllowMoving )
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_SWARM_ATTACK_UNIT, pTarget->GetUniqueId() ), this );
			else
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ATTACK_UNIT, pTarget->GetUniqueId() ), this );
		}
		return true;
	}
	return false;
}

BYTE CAIUnit::AnalyzeTargetScan( CAIUnit *pCurTarget, const bool bDamageUpdated, const bool bScanForObstacles, CObjectBase *pCheckBuilding )
{
	if ( IsTimeToAnalyzeTargetScan() && theScanLimiter.CanScan() )
	{
		GetLastBehTime() = curTime;
		SetTargetScanRandom();
	//		CAICommand *pCommand = GetCurCmd();

		// если огонь по всему
		if ( GetBehaviourFire() == SBehaviour::EFAtWill )
		{
			// найдена цель
			CAIUnit *pTarget = 0;
			CBasicGun *pGun = 0;

			LookForTarget( pCurTarget, bDamageUpdated, &pTarget, &pGun );

			if ( pTarget != 0 && pCheckBuilding != 0 && pTarget->GetStats()->IsInfantry() &&
					 checked_cast<CSoldier*>(pTarget)->IsInBuilding() && checked_cast<CSoldier*>(pTarget)->GetBuilding() == pCheckBuilding )
				pTarget = 0;

			if ( AttackTarget( pTarget, pCurTarget ) )
				return 3;
			else if ( !pTarget && bScanForObstacles )
			{
				// нет врагов, пострелять по препятствиям, если нужно
				IObstacle *pObstacle = LookForObstacle();
				if ( pObstacle )
				{
					pObstacle->IssueUnitAttackCommand( this );
					return 3;
				}
			}
		}

		return 2;
	}
	else
		return AttackTarget( GetBestShootEstimatedUnit(), pCurTarget ) ? 3 : 0;
}

void CAIUnit::SetAmbush()
{
	updater.AddUpdate( 0, ACTION_NOTIFY_SET_AMBUSH, this, -1 );
}

void CAIUnit::RemoveAmbush()
{
	updater.AddUpdate( 0, ACTION_NOTIFY_REMOVE_AMBUSH, this, -1 );
}

void CAIUnit::GetFogInfo( SWarFogUnitInfo *pInfo ) const
{
	pInfo->vPos.x = GetCenterTile().x / AI_TILES_IN_VIS_TILE;
	pInfo->vPos.y = GetCenterTile().y / AI_TILES_IN_VIS_TILE;
	pInfo->bPlane = GetStats()->IsAviation() || IsInSolidPlace();
	pInfo->nRadius = IsInSolidPlace() ? 0 : GetSightRadius() / SConsts::TILE_SIZE / AI_TILES_IN_VIS_TILE;
	pInfo->sector = SSector();
}

void CAIUnit::WarFogChanged()
{
	if ( IsAlive() )
	{
		bool bShouldUpdateWarfog = true;
		if ( CFormation *pFormation = GetFormation() )
		{
			IUnitState *pState = pFormation->GetState();
			bShouldUpdateWarfog = !pState || pState->GetName() != EUSN_GUN_CREW_STATE;
		}

		if ( bShouldUpdateWarfog && GetParty() < 2 )
		{
			SWarFogUnitInfo fogInfo;
			GetFogInfo( &fogInfo );
			theWarFog.UpdateUnit( GetUniqueId(), fogInfo );
		}
	}
}

const int CAIUnit::ChooseFatality( const float fDamage )
{
	const SUnitBaseRPGStats *pStats = GetStats();
	

	// for ships play fatality on deep water & ordinary death on shallow water
	if ( !GetTerrain()->IsLocked( GetCenterTile(), EAC_WATER ) && !GetTerrain()->IsLocked( GetCenterTile(), EAC_TERRAIN ) )
	{
		// shallow water 
		return -1;
	}
	if ( !GetTerrain()->IsLocked( GetCenterTile(), EAC_WATER ) && GetTerrain()->IsLocked( GetCenterTile(), EAC_TERRAIN ) || // deep water
			 (
			 NRandom::Random( 0.0f, 1.0f ) < SConsts::FATALITY_PROBABILITY ||
			 fDamage / GetStats()->fMaxHP > SConsts::DAMAGE_FOR_MASSIVE_DAMAGE_FATALITY && 
			 NRandom::Random( 0.0f, 0.1f ) < SConsts::MASSIVE_DAMAGE_FATALITY_PROBABILITY
			 )
		 )
	{
		if ( pStats->animdescs.size() > NDb::ANIMATION_DEATH_FATALITY && !pStats->animdescs[NDb::ANIMATION_DEATH_FATALITY].anims.empty() )
		{
			const int nFatality = NRandom::Random( pStats->animdescs[NDb::ANIMATION_DEATH_FATALITY].anims.size() );

			const int nRect = pStats->animdescs[NDb::ANIMATION_DEATH_FATALITY].anims[nFatality].nAABB_A;
			CVec2 vRectCenter(VNULL2), vRectHalfSize(VNULL2);
			if ( !pStats->aabb_as.empty() && nRect != -1 && pStats->aabb_as.size() > nRect )
			{
				vRectCenter = pStats->aabb_as[nRect].vCenter;
				vRectHalfSize = pStats->aabb_as[nRect].vHalfSize;
			}
			SRect fatalityRect;

			const CVec2 vFrontDir = GetVectorByDirection( GetFrontDirection() );
			const CVec2 vRectTurn( vFrontDir.y, -vFrontDir.x );
			
			fatalityRect.InitRect( GetCenterPlain() + ( (vRectCenter) ^ vRectTurn ), vFrontDir, vRectHalfSize.y, vRectHalfSize.x );

			UnlockTiles();
			bool bFree = IsMapFullyFree( fatalityRect, this );
			LockTiles();

			if ( bFree )
				return nFatality;
		}
	}
	if ( pStats->animdescs.size() < NDb::ANIMATION_DEATH || pStats->animdescs[NDb::ANIMATION_DEATH].anims.empty() )
		return -1;
	else
		return -1 * int( NRandom::Random( pStats->animdescs[NDb::ANIMATION_DEATH].anims.size() ) + 2 );
}

void CAIUnit::InitializeShootArea( SShootArea *pArea, CBasicGun *pGun, const float fRangeMin, const float fRangeMax ) const
{
	pArea->vCenter3D = CVec3( GetCenterPlain(), 0.0f );
	pArea->fMinR = fRangeMin;
	pArea->fMaxR = fRangeMax;
	
	if ( NeedDeinstall() || !CanMove() && !CanRotate() )
	{
		const WORD wConstraint = 
			pGun ?
			pGun->GetTurret() ? pGun->GetTurret()->GetHorTurnConstraint() : pGun->GetWeapon()->wDeltaAngle :
			65535;

		if ( wConstraint >= 32767 )
		{
			pArea->wStartAngle = 65535;
			pArea->wFinishAngle = 65535;
		}
		else
		{
			const WORD wFrontDir = GetFrontDirection();
			pArea->wStartAngle = wFrontDir - wConstraint;
			pArea->wFinishAngle = wFrontDir + wConstraint;
		}
	}
}

void CAIUnit::GetShootAreas( SShootAreas *pShootAreas, int *pnAreas ) const
{
	//construct( pShootAreas );
	*pnAreas = 0;

	if ( GetFirstArtilleryGun() != 0 )
	{
		*pnAreas = 1;
		pShootAreas->areas.push_back( SShootArea() );
		pShootAreas->areas.back().eType = SShootArea::ESAT_BALLISTIC;
	
		CBasicGun *pGun = GetFirstArtilleryGun();
		InitializeShootArea( &(pShootAreas->areas.back()), pGun, pGun->GetWeapon()->fRangeMin, pGun->GetFireRangeMax() );
	}
//	else
	{
		float fMaxFireRange = -1.0f;
		float fMinFireRange = 1000000.0f;
		CBasicGun *pGun = 0;
		for ( int i = 0; i < GetNGuns(); ++i )
		{
			if ( GetGun( i )->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE ||
					 GetGun( i )->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE ||
					 GetGun( i )->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_ROCKET
					)
			{
				CBasicGun *pGunI = GetGun( i );
				const float fLocalFireRange = pGunI->GetFireRange( 0.0f );
				if ( fLocalFireRange > fMaxFireRange )
					fMaxFireRange = fLocalFireRange;

				const float fLocalMinFireRange = pGunI->GetWeapon()->fRangeMin;
				if ( fLocalMinFireRange < fMinFireRange )
					fMinFireRange = fLocalMinFireRange;

				if ( !pGun || !pGun->IsOnTurret() && pGunI->IsOnTurret() )
					pGun = pGunI;
				else if ( pGun && !pGun->IsOnTurret() && !pGunI->IsOnTurret() )
				{
					if ( pGun->GetWeapon()->wDeltaAngle < pGunI->GetWeapon()->wDeltaAngle )
						pGun = pGunI;
				}
				else if ( pGun && pGun->IsOnTurret() && pGunI->IsOnTurret() )
				{
					if ( pGun->GetHorTurnConstraint() < pGunI->GetHorTurnConstraint() )
						pGun = pGunI;
				}
			}
		}
		if ( fMaxFireRange != -1.0f )
		{
			*pnAreas = 1;
			pShootAreas->areas.push_back( SShootArea() );
			pShootAreas->areas.back().eType = SShootArea::ESAT_LINE;
			InitializeShootArea( &(pShootAreas->areas.back()), pGun, fMinFireRange, fMaxFireRange );
		}
	}

	if ( GetStats()->etype == RPG_TYPE_ART_AAGUN )
	{
		*pnAreas = 1;
		pShootAreas->areas.push_back( SShootArea() );
		pShootAreas->areas.back().eType = SShootArea::ESAT_AA;
		InitializeShootArea( &(pShootAreas->areas.back()), 0, 0.0f, GetMaxFireRange() );
	}
}

void CAIUnit::SetInTankPit( CExistingObject *_pTankPit )
{ 
	pTankPit = _pTankPit;
	bVirtualTankPit = false;
	bIsInTankPit = true;
	ApplyStatsModifier( checked_cast<const NDb::SMechUnitRPGStats*>(pTankPit->GetStats())->pInnerUnitBonus, true );

	if ( IsInTankPit() )
		UpdateTankPitVisibility( true, visible4Party[theCheats.GetNPartyForWarFog()] );

	updater.AddUpdate( CreateStatusUpdate( EUS_IN_TANK_PIT, IsInTankPit(), 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
}

void CAIUnit::UpdateTankPitVisibility( const bool bVisibilityChanged, const bool bVisible )
{
	if ( !bVisibilityChanged )
		return;

	if ( bVisible )
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ST_OBJ, GetTankPit(), -1 );
	else
		updater.AddUpdate( 0, ACTION_NOTIFY_DELETED_ST_OBJ, GetTankPit(), -1 );
}

void CAIUnit::SetOffTankPit()
{
	if ( IsValidObj( pTankPit ) )
	{
		ApplyStatsModifier( checked_cast<const NDb::SMechUnitRPGStats*>(pTankPit->GetStats())->pInnerUnitBonus, false );
		theStatObjs.DeleteInternalObjectInfo( pTankPit );
	}
	pTankPit = 0;
	bVirtualTankPit = false;
	bIsInTankPit = false;
	updater.AddUpdate( CreateStatusUpdate( EUS_IN_TANK_PIT, IsInTankPit(), 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
}

void CAIUnit::SetCamoulfage()
{
	if ( GetStats()->fCamouflage == 0 ) // sniper or other unit with camo ability
	{
		fCamoflage = 0.0f;
		SetBehaviourFire( SBehaviour::EFNoFire );
	}
	else
    fCamoflage = Clamp( 1.0f - GetStatsModifier()->camouflage.Get( GetStats()->fCamouflage ), 0.0f, 1.0f );

	updater.AddUpdate( 0, ACTION_NOTIFY_SET_CAMOUFLAGE, this, -1 );
}

void CAIUnit::RemoveCamouflage( ECamouflageRemoveReason eReason )
{
	if ( CanCommandBeExecutedByStats( GetCommandByAbility( ABILITY_CAMOFLAGE_MODE ) ) || CanCommandBeExecutedByStats( GetCommandByAbility( ABILITY_ADAVNCED_CAMOFLAGE_MODE ) ) )
	{
		switch( eReason )
		{
			case ECRR_SELF_SHOOT:
				{
					SExecutorEventParam par( EID_FIRE_CAMOFLATED, 0, GetUniqueId() );
					theExecutorContainer.RaiseEvent( par );
				}
				break;
			case ECRR_USER_COMMAND:
				fCamoflage = 1.0f;
				updater.AddUpdate( 0, ACTION_NOTIFY_REMOVE_CAMOUFLAGE, this, -1 );
				SetBehaviourFire( SBehaviour::EFAtWill );
				break;
		}
	}
	else if ( GetStats()->fCamouflage != 0 )
	{
		fCamoflage = 1.0f;
		updater.AddUpdate( 0, ACTION_NOTIFY_REMOVE_CAMOUFLAGE, this, -1 );
	}
}

void CAIUnit::StartCamouflating()
{
	camouflateTime = curTime;
}

/*void CAIUnit::InitAviationPath()
{
	const SMechUnitRPGStats * pStats = checked_cast<const SMechUnitRPGStats*>( GetStats() );
	pPathUnit->InitAviationPath( pStats );
}*/

void CAIUnit::AnalyzeCamouflage()
{
	if ( GetStats()->fCamouflage != 0 )
	{
		if ( !IsIdle() )
		{
			camouflateTime = curTime;
			if ( IsCamoulflated() )
				RemoveCamouflage( ECRR_USER_COMMAND );
		}
		else
		{
			const BYTE cParty = theDipl.GetNParty( player );
			if ( cParty != theDipl.GetNeutralParty() && IsVisible( 1 - cParty ) ||
					 cParty == theDipl.GetNeutralParty() && ( IsVisible( 0 ) || IsVisible( 1 ) ) )
			{
				camouflateTime = curTime;
				if ( IsCamoulflated() )
					RemoveCamouflage( ECRR_USER_COMMAND );
			}
			else if ( !IsCamoulflated() && curTime - camouflateTime >= GetTimeToCamouflage() )
				SetCamoulfage();
		}
	}
}

void CAIUnit::EnemyKilled( CAIUnit *pEnemy )
{
	if ( pEnemy && pEnemy->IsRefValid() &&
			 theDipl.GetDiplStatusForParties( GetParty(), pEnemy->GetParty() ) == EDI_ENEMY )
	{
		if ( pEnemy->IsVisible( GetParty() ) )
		{
			const EUnitRPGType eType = pEnemy->GetStats()->etype;
			
			SendAcknowledgement( ACK_KILLED_ENEMY, true );
		}
		
		theStatistics.IncreasePlayerExperience( GetPlayer(), GetReinforcementType(), pEnemy->GetPriceMax() );

		float fPrice = pEnemy->GetStats()->fPrice;
		if ( pEnemy->IsInFormation() )
		{
			CFormation* pFormation = pEnemy->GetFormation();
			if ( pFormation->Size() == 1 && (*pFormation)[0] == pEnemy && pFormation->GetState()->GetName() == EUSN_GUN_CREW_STATE )
			{
				CAIUnit *pArtillery = checked_cast<CFormationGunCrewState*>( pFormation->GetState() )->GetArtillery();
				theStatistics.IncreasePlayerExperience( GetPlayer(), GetReinforcementType(), pArtillery->GetPriceMax() );
				fPrice += pArtillery->GetStats()->fPrice;
			}
		}
	}
}

bool CAIUnit::IsNoticableByUnit( CCommonUnit *pUnit, const float fNoticeRadius )
{
	const bool bRadiusOk = fabs2( pUnit->GetCenter() - GetCenter() ) <= sqr( fNoticeRadius );

	return 
		bRadiusOk && ( IsVisible( pUnit->GetParty() ) || IsRevealed() && pUnit->CanMove() && !pUnit->NeedDeinstall() );
}

void CAIUnit::UpdateArea( const EActionNotify eAction ) 
{
	updater.AddUpdate( 0, eAction, this, -1 );
}

void CAIUnit::SendAcknowledgement( CAICommand *pCommand, EUnitAckType ack, bool bForce )
{
	const SUnitBaseRPGStats *pStats = GetStats();
	if ( pStats && GetPlayer() == theDipl.GetMyNumber() && ( bForce || pCommand && pCommand->IsRefValid() && !pCommand->IsFromAI() ) )
	{
		bool bSend = false;
		int nSet = 0;
		if ( pStats->IsInfantry() && GetStats()->etype == RPG_TYPE_ENGINEER )
		{
			//nu i krivzna ...
			IUnitState *pCurrentState = checked_cast<CSoldier*>(this)->GetFormation()->GetState();
			if ( pCurrentState && pCurrentState->GetName() == EUSN_GUN_CREW_STATE )
			{
				CArtillery* pArtillery = checked_cast<CFormationGunCrewState*>(pCurrentState)->GetArtillery();
				bSend = true;
				if ( pArtillery->GetStats()->etype == RPG_TYPE_ART_HEAVY_GUN || pArtillery->GetStats()->etype == RPG_TYPE_ART_HOWITZER )
					nSet = 1;
			}
		}
		else if ( pStats->IsTrain() && ( ack == ACK_CANNOT_MOVE_TRACK_DAMAGED ) )
		{
			bSend = true;
			ack = ACK_NEGATIVE; //ACK_CANNOT_FIND_PATH_TO_TARGET;
		}
		else
			bSend = true;

		if ( bSend )
			theAckManager.AddAcknowledgment( ack, this, nSet );
	}
}

void CAIUnit::SendAcknowledgement( EUnitAckType ack, bool bForce )
{
	SendAcknowledgement( GetCurCmd(), ack, bForce );
}

EUnitAckType CAIUnit::GetGunsRejectReason() const
{
	EUnitAckType eRejectReason = GetGuns()->GetRejectReason();
	return ( eRejectReason == ACK_NONE ) ? ACK_NEGATIVE : eRejectReason;
}

bool CAIUnit::DoesExistRejectGunsReason( const EUnitAckType &ackType ) const
{
	return GetGuns()->DoesExistRejectReason( ackType );
}

const int CAIUnit::GetMinArmor() const
{
	return GetStats()->nMinArmor;
}

const int CAIUnit::GetMaxArmor() const
{
	return GetStats()->nMaxArmor;
}

const int CAIUnit::GetMinPossibleArmor( const int nSide ) const
{
	return GetStats()->GetMinPossibleArmor( nSide );
}

const int CAIUnit::GetMaxPossibleArmor( const int nSide ) const
{
	return GetStats()->GetMaxPossibleArmor( nSide );
}

const int CAIUnit::GetArmor( const int nSide ) const
{
	return GetStats()->GetArmor( nSide );
}

const int CAIUnit::GetRandomArmor( const int nSide ) const
{
	return GetStats()->GetRandomArmor( nSide );
}

const float CAIUnit::GetCover() const 
{ 
	return  GetStatsModifier()->cover.Get( 1.0f );
}

bool CAIUnit::IsSavedByCover() const
{ 
	return 
		NRandom::Random( 0.0f, 1.0f ) >= GetCover();
}

const int CAIUnit::GetNCommonGuns() const { return GetGuns() ? GetGuns()->GetNCommonGuns() : 0; }

const SBaseGunRPGStats& CAIUnit::GetCommonGunStats( const int nCommonGun ) const { return GetGuns()->GetCommonGunStats( nCommonGun ); }

int CAIUnit::GetNAmmo( const int nCommonGun ) const { return GetGuns()->GetNAmmo( nCommonGun ); }

bool CAIUnit::IsCommonGunFiring( const int nCommonGun ) const { return GetGuns()->IsCommonGunFiring( nCommonGun ); }

const float CAIUnit::GetPassability() const { return GetStats()->fPassability; }

void CAIUnit::SetDirection( const WORD wDirection )
{ 
	CBasePathUnit::SetDirection( wDirection );

	if ( GetVisionAngle() != 32768 )
		WarFogChanged();
}

void CAIUnit::UnRegisterAsBored( const enum EUnitAckType eBoredType )
{
	theAckManager.UnRegisterAsBored( eBoredType, this );
}

void CAIUnit::RegisterAsBored( const enum EUnitAckType eBoredType )
{
	theAckManager.RegisterAsBored( eBoredType, this );
}

bool const CAIUnit::IsTrain() const
{
	return GetStats()->IsTrain();
}

const float CAIUnit::GetTimeToForget() const
{
	return SGeneralConsts::TIME_TO_FORGET_UNIT;
}

CAIUnitInfoForGeneral* CAIUnit::GetUnitInfoForGeneral() const
{
	return pUnitInfoForGeneral;
}

void CAIUnit::SetLastVisibleTime( const NTimer::STime time )
{
	pUnitInfoForGeneral->SetLastVisibleTime( time );
}

const bool CAIUnit::CanMove() const
{ 
	return
		!IsRestInside() &&
		GetBehaviourMoving() != SBehaviour::EMHoldPos &&
		GetStats()->fSpeed != 0 ; //optimisation (returned true in CBasePathUnit) "&& CCommonUnit::CanMove();"
}

const bool CAIUnit::CanMoveCritical() const
{
	return
		GetStats()->fSpeed != 0 && CanMove();
}

const bool CAIUnit::CanRotate() const
{
	return ( !IsRestInside() ) && ( GetStats()->IsInfantry() || GetStats()->fRotateSpeed != 0.0f );
}

float CAIUnit::GetPriceMax() const
{
	return GetStats()->fPrice;
}

void CAIUnit::UnitCommand( CAICommand *pCommand, bool bPlaceInQueue, bool bOnlyThisUnitCommand )
{
	if ( !bOnlyThisUnitCommand )
	{
		SetBehaviourMoving( SBehaviour::EMRoaming );
		if ( GetStats()->IsInfantry() )
			UnlockTiles();
	}
	if ( !pCommand->ToUnitCmd().bFromAI )
	{
		SExecutorEventParam par( EID_NEW_COMMAND_RECIEVED, 0, GetUniqueId() );
		theExecutorContainer.RaiseEvent( par );
	}
	CCommonUnit::UnitCommand( pCommand, bPlaceInQueue, bOnlyThisUnitCommand );
}

void CAIUnit::Lock( const CBasicGun *pGun )
{
	if ( !GetStats()->IsTrain() )
		CCommonUnit::Lock( pGun );
}

void CAIUnit::Unlock( const CBasicGun *pGun )
{
	if ( !GetStats()->IsTrain() )
		CCommonUnit::Unlock( pGun );
}

bool CAIUnit::IsLocked( const CBasicGun *pGun ) const
{
	if ( GetStats()->IsTrain() || bRestInside )
		return true;
	else
		return CCommonUnit::IsLocked( pGun );
}

void CAIUnit::SetActiveShellType( const NDb::SWeaponRPGStats::SShell::EShellDamageType eShellType )
{
	if ( GetGuns()->SetActiveShellType( eShellType ) )
		updater.AddUpdate( 0, ACTION_NOTIFY_SHELLTYPE_CHANGED, this, static_cast<int>(eShellType) );
}

void CAIUnit::AnimationSet( int nAnimation )
{
	pAnimUnit->AnimationSet( nAnimation );
}

void CAIUnit::AnimationSegment()
{
	pAnimUnit->Segment();
}

void CAIUnit::Moved()
{
	pAnimUnit->Moved();
}

void CAIUnit::Stopped()
{
	pAnimUnit->Stopped();
}

void CAIUnit::StopCurAnimation()
{
	pAnimUnit->StopCurAnimation();
}

void CAIUnit::WantedToReveal( CAIUnit *pWhoRevealed )
{
	if ( !theDipl.IsNetGame() &&
			 theDipl.GetMyNumber() == GetPlayer() && 
			 theDipl.GetDiplStatus( pWhoRevealed->GetPlayer(), GetPlayer() ) == EDI_ENEMY )
		bQueredToReveal = true;
}

bool CAIUnit::IsRevealed() const
{
	return bRevealed;
}

const bool CAIUnit::IsInfantry() const
{
	return GetStats()->IsInfantry();
}

bool CAIUnit::CanMoveAfterUserCommand() const
{
	return GetStats()->HasCommand( ACTION_COMMAND_MOVE_TO ) && 
				 GetStats()->fSpeed != 0 && CanMove();
}

void CAIUnit::NotifyAbilityRun( class CAICommand * pCommand )
{
	// if the command is ability command
	const NDb::EUnitSpecialAbility eAbility = (NDb::EUnitSpecialAbility)GetAbilityByCommand( pCommand->ToUnitCmd().nCmdType );
	if ( NDb::ABILITY_NOT_ABILITY == eAbility ) 
		return;

	const NDb::SUnitSpecialAblityDesc *pDesc = GetUnitAbilityDesc( eAbility );
	if ( pDesc )
	{
		const NDb::ESpecialAbilityParam action = NDb::ESpecialAbilityParam(int(pCommand->ToUnitCmd().fNumber));
		ProduceEventByAction( eAbility, action, pCommand );
	}
}
#include "ExecutorSniperCamouflage.h"
#include "ExecutorThrowGrenade.h"

void CAIUnit::UpdateEnableAblitiy( const int nAbility )
{
	CPtr<SAISpecialAbilityUpdate> pUpdate = new SAISpecialAbilityUpdate;
	pUpdate->info.nAbilityType = nAbility;
	pUpdate->info.state.eState = EASS_READY_TO_ON;
	pUpdate->info.state.bAutocast = false;
	pUpdate->info.fCurValue = 1.0f;
	pUpdate->info.nObjUniqueID = GetUniqueId();
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, this, -1 );
}

const int CAIUnit::GetXPLevel()
{
	return 1 + theStatistics.GetXPLevel( GetPlayer(), GetReinforcementType() );
}

const int CAIUnit::GetAbilityLevel()
{
	//return nAbilityLevel <= 0 ? 1 + theStatistics.GetAbilityLevel( GetPlayer(), GetReinforcementType() ) : nAbilityLevel;
	return 1 + theStatistics.GetAbilityLevel( GetPlayer(), GetReinforcementType() );
}

void CAIUnit::InitSpecialAbilities( int nFromLevel )
{
	NI_ASSERT( GetStats()->GetActions() != 0, StrFmt("Empty actions set for unit \"%s\" of type \"%s\"", NDb::GetResName(GetStats()), typeid(*GetStats()).name()) );
	pShootInMovementExecutor = 0;
	// start SpecialAbility executors (that are possible for this unit)
	const int nActiveAbilities = (std::min<int>)( GetStats()->GetActions()->specialAbilities.size(), GetAbilityLevel() );
	for ( int i = nFromLevel; i < nActiveAbilities; ++i )
	{
		NI_ASSERT( GetStats()->GetActions()->specialAbilities[i] != 0, StrFmt("Empty ability %d for unit \"%s\" of type \"%s\"", i, NDb::GetResName(GetStats()), typeid(*GetStats()).name()) )
		if ( GetStats()->GetActions()->specialAbilities[i] != 0 )
			InitAbility( GetStats()->GetActions()->specialAbilities[i]->eName );
	}
}

void CAIUnit::InitAbility( const NDb::EUnitSpecialAbility nAbility )
{
	// unit can run this command, and ability
	switch ( nAbility )
	{
	case NDb::ABILITY_DROP_BOMBS:
		{
			UpdateEnableAblitiy( nAbility );
			CPtr<CExecutor> pEx = new CExecutorPlaneDropBombsObject( this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );

			SExecutorEventParam par( EID_ABILITY_ACTIVATE_AUTOCAST, 0, GetUniqueId() );
			CExecutorEventSpecialAbility event( par, nAbility );
			pTheExecutorsContainer->RaiseEvent( event );
		}
		break;

		// state abilities (enabled upon map start)
	case NDb::ABILITY_USE_BINOCULAR:
		UpdateEnableAblitiy( nAbility );
		// not ability
	case NDb::ABILITY_NOT_ABILITY:
		break;
	case NDb::ABILITY_THROW_GRENADE:
	case NDb::ABILITY_THROW_ANTITANK_GRENADE:
		{
			CPtr<CExecutor> pEx = new CExecutorThrowGrenade( this, nAbility );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
			// Throw grenade is on Autocast by default (if not a sniper)
			if ( GetStats()->eDBtype != DB_RPG_TYPE_SNIPER )
			{
				SExecutorEventParam par( EID_ABILITY_ACTIVATE_AUTOCAST, 0, GetUniqueId() );
				CExecutorEventSpecialAbility event( par, nAbility );
				pTheExecutorsContainer->RaiseEvent( event );
			}
		}
		break;
	case NDb::ABILITY_AMBUSH:
		{
			CPtr<CExecutor> pEx = new CExecutorAmbush( this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_CAMOFLAGE_MODE:
		{
			CPtr<CExecutor> pEx = new CExecutorCamouflage( this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_ADAVNCED_CAMOFLAGE_MODE:
		{
			// create camoflage executor. register it on needed events.
			CPtr<CExecutor> pEx = new CExecutorSniperCamouflage( this, nAbility == NDb::ABILITY_ADAVNCED_CAMOFLAGE_MODE );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_PLACE_CHARGE:
	//case NDb::ABILITY_PLACE_CONTROLLED_CHARGE:
		{
			CPtr<CExecutor> pEx = new CExecutorPlaceCharge( this, nAbility );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_RADIO_CONTROLLED_MODE:
	//case NDb::ABILITY_DETONATE:
		{
			CPtr<CExecutor> pEx1 = new CExecutorPlaceCharge( this, NDb::ABILITY_PLACE_CONTROLLED_CHARGE );
			theExecutorContainer.Add( pEx1 );
			pEx1->RegisterOnEvents( &theExecutorContainer );

			CPtr<CExecutor> pEx2 = new CExecutorPlaceCharge( this, NDb::ABILITY_DETONATE );
			theExecutorContainer.Add( pEx2 );
			pEx2->RegisterOnEvents( &theExecutorContainer );					
			SExecutorEventParam par( EID_ABILITY_DISABLE, 0, GetUniqueId() );
			CExecutorEventSpecialAbility event( par, NDb::ABILITY_DETONATE );
			pTheExecutorsContainer->RaiseEvent( event );
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_HOLD_SECTOR:
		{
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_ENTRENCH_SELF:
		{
			CPtr<CExecutor> pEx = new CExecutorSoldierEntrench( this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
		}
		break;
	case NDb::ABILITY_TRACK_TARGETING:
		{
			CPtr<CExecutor> pEx = new CExecutorTrackTargetting( this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_CRITICAL_TARGETING:
		{
			CPtr<CExecutor> pEx = new CExecutorUnitBonus( NDb::ABILITY_CRITICAL_TARGETING, this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
		}
		break;
	case NDb::ABILITY_RAPID_FIRE_MODE:
	case NDb::ABILITY_RAPID_SHOT_MODE:			// Two enums?
		{
			CPtr<CExecutor> pEx = new CExecutorUnitBonus( NDb::ABILITY_RAPID_FIRE_MODE, this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
		}
		break;
	case NDb::ABILITY_FLAMETHROWER:
		break;																// Flamethrower is just an icon
	case NDb::ABILITY_MOBILE_FORTRESS:
		break;																// Mobile Fortress is just an icon
	case NDb::ABILITY_SUPRESS:
		UpdateEnableAblitiy( nAbility );
		break;																// Suppress fire is handled elsewhere
	case NDb::ABILITY_SUPPORT_FIRE:
		UpdateEnableAblitiy( nAbility );
		break;																// Support fire is handled elsewhere
	case NDb::ABILITY_PATROL:
		UpdateEnableAblitiy( nAbility );
		break;																// Patrol is handled elsewhere
	case NDb::ABILITY_ZEROING_IN:
		UpdateEnableAblitiy( nAbility );
		break;																// Zeroing In is handled elsewhere
	case NDb::ABILITY_CAUTION:
		{
			CPtr<CExecutor> pEx = new CExecutorCaution( this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
			UpdateEnableAblitiy( nAbility );
		}
		break;																
	case NDb::ABILITY_ADRENALINE_RUSH:
		{
			CPtr<CExecutor> pEx = new CExecutorAdrenalineRush( this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
			UpdateEnableAblitiy( nAbility );
		}
		break;																
	case NDb::ABILITY_MANUVERABLE_FIGHT_MODE:		// AKA Shoot In Movement
		{
			pShootInMovementExecutor = new CExecutorUnitBonus( NDb::ABILITY_MANUVERABLE_FIGHT_MODE, this );
			theExecutorContainer.Add( pShootInMovementExecutor );
			pShootInMovementExecutor->RegisterOnEvents( &theExecutorContainer );
		}
		break;
	case NDb::ABILITY_ANTIATRILLERY_FIRE:				// AKA Counter-Fire
		{
			NI_ASSERT( GetStats()->IsArtillery(), StrFmt("Skill (Counter-fire) suitable for artillery only (type %s, ID = \"%s\")", typeid(*GetStats()).name(), NDb::GetResName(GetStats())) );
			if ( GetStats()->IsArtillery() )
			{
				CArtillery *pArtUnit = dynamic_cast<CArtillery*>( this );
				CPtr<CExecutor> pEx = new CExecutorCounterFire( pArtUnit );
				theExecutorContainer.Add( pEx );
				pEx->RegisterOnEvents( &theExecutorContainer );
				UpdateEnableAblitiy( nAbility );
			}
		}
		break;
	case NDb::ABILITY_SMOKE_SHOTS:
		{
			CPtr<CExecutor> pEx = new CExecutorSmokeShots( this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
		}
		break;
	case NDb::ABILITY_FIRST_AID:
		{
			// formation creates executor
		}
		break;
	case NDb::ABILITY_MASTER_OF_STREETS:
		{
			// No executor (yet)
			// TODO
		}
		break;
	case NDb::ABILITY_LINKED_GRENADES:
		{
			CPtr<CExecutor> pEx = new CExecutorLinkedGrenades( this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_EXACT_SHOT:
		{
			CPtr<CExecutor> pEx = new CExecutorExactShot( this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_COVER_FIRE:
		{
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_EXACT_BOMBING:
		{			// It is just a stat bonus
			const NDb::SUnitSpecialAblityDesc *pSA = GetUnitAbilityDesc( NDb::ABILITY_EXACT_BOMBING );
			if ( pSA )
				ApplyStatsModifier( pSA->pStatsBonus, true );
		}
		break;
	case NDb::ABILITY_SPY_MODE:
		{
			NI_ASSERT( GetStats()->IsInfantry(), "Skill (Spy Mode) suitable for infantry only" );
			if ( !GetStats()->IsInfantry() )
				break;
			CPtr<CExecutor> pEx = new CExecutorSpyMode( this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_SKY_GUARD:
		{			// It is just a stat bonus
			const NDb::SUnitSpecialAblityDesc *pSA = GetUnitAbilityDesc( NDb::ABILITY_SKY_GUARD );
			if ( pSA )
				ApplyStatsModifier( pSA->pStatsBonus, true );
		}
		break;
	case NDb::ABILITY_MASTER_PILOT:
		{
			// No executor (yet)
			// TODO
		}
		break;
	case NDb::ABILITY_SURVIVAL:
		{			// It is just a stat bonus
			const NDb::SUnitSpecialAblityDesc *pSA = GetUnitAbilityDesc( NDb::ABILITY_SURVIVAL );
			if ( pSA )
				ApplyStatsModifier( pSA->pStatsBonus, true );
		}
		break;
	case NDb::ABILITY_TANK_HUNTER:
		{
			// No executor (yet)
			// TODO
		}
		break;
	case NDb::ABILITY_OVERLOAD_MODE:
		{
			UpdateEnableAblitiy( nAbility );
			CPtr<CExecutor> pEx = new CExecutorUnitBonus( NDb::ABILITY_OVERLOAD_MODE, this );
			theExecutorContainer.Add( pEx );
			pEx->RegisterOnEvents( &theExecutorContainer );
		}
		break;
	case NDb::ABILITY_DIG_TRENCHES:
		{
			UpdateEnableAblitiy( nAbility );
		}
		break;
	case NDb::ABILITY_REMOVE_MINE_FIELD:
		{
			UpdateEnableAblitiy( nAbility );
		}
		break;
	default:
		DebugTrace( "Unknown ability %i", nAbility );
//		NI_ASSERT( false, StrFmt( "Unknown ability %i", nAbility ) );
	}
}

const bool CAIUnit::CanShootInMovement()
{
	if ( GetStats()->IsAviation() )			// Aviation can always shoot in movement
		return true;

	if ( pShootInMovementExecutor ) 
	{				// Has this ability, check if it is active
		if ( pShootInMovementExecutor->GetState() == EASS_ACTIVE )
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

void CAIUnit::SetHoldSector()
{
	if ( !bHoldingSector )
	{
		if ( CUnitGuns* pGuns = GetGuns() )
		{
			if ( CBasicGun* pMainGun = pGuns->GetMainGun() )
			{
				Lock( pMainGun );
				if ( CTurret *pTurret = pMainGun->GetTurret() )
				{
					pTurret->Lock( pMainGun );
					pTurret->SetHorTurnConstraint( SConsts::SPY_GLASS_ANGLE );
				}
			}
		}
		SetVisionAngle( SConsts::SPY_GLASS_ANGLE );
		WarFogChanged();
		UnsetFollowState();				
		SetBehaviourMoving( SBehaviour::EMHoldPos );

		const NDb::SUnitSpecialAblityDesc *pSA = GetUnitAbilityDesc( NDb::ABILITY_HOLD_SECTOR );
		if ( pSA && pSA->pStatsBonus )
			ApplyStatsModifier( pSA->pStatsBonus, true );
		
		CPtr< SAISpecialAbilityUpdate > pUpdate = new SAISpecialAbilityUpdate;
		pUpdate->info.nAbilityType = NDb::ABILITY_HOLD_SECTOR;
		pUpdate->info.state = EASS_ACTIVE;
		pUpdate->info.nObjUniqueID = GetUniqueId();
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, this, -1 );
		
		updater.AddUpdate( CreateStatusUpdate( EUS_HOLD_SECTOR, true, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
		bHoldingSector = true;
	}
}

void CAIUnit::ResetHoldSector()
{
	if ( bHoldingSector )
	{
		if ( CUnitGuns* pGuns = GetGuns() )
		{
			if ( CBasicGun* pMainGun = pGuns->GetMainGun() )
			{
				Unlock( pMainGun );
				if ( CTurret *pTurret = pMainGun->GetTurret() )
				{
					pTurret->Unlock( pMainGun );
					pTurret->ResetHorTurnConstraint();
				}
			}
		}
		SetVisionAngle( 32768 );
		WarFogChanged();
		SetBehaviourMoving( SBehaviour::EMRoaming );
		const NDb::SUnitSpecialAblityDesc *pSA = GetUnitAbilityDesc( NDb::ABILITY_HOLD_SECTOR );
		if ( pSA && pSA->pStatsBonus )
			ApplyStatsModifier( pSA->pStatsBonus, false );
		
		CPtr< SAISpecialAbilityUpdate > pUpdate = new SAISpecialAbilityUpdate;
		pUpdate->info.nAbilityType = NDb::ABILITY_HOLD_SECTOR;
		pUpdate->info.state = EASS_READY_TO_ON;
		pUpdate->info.nObjUniqueID = GetUniqueId();
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, this, -1 );
 
		updater.AddUpdate( CreateStatusUpdate( EUS_HOLD_SECTOR, true, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
 		bHoldingSector = false;
	}
}

void CAIUnit::SetTrackTargeting( bool bOn )
{
	if ( bOn == bTargetingTrack )
		return;
	const NDb::SUnitSpecialAblityDesc *pDesc = GetUnitAbilityDesc( NDb::ABILITY_TRACK_TARGETING );
	if ( pDesc )
	{
		bTargetingTrack = bOn;
		if (  pDesc->pStatsBonus )
			ApplyStatsModifier(  pDesc->pStatsBonus, bOn );

		updater.AddUpdate( CreateStatusUpdate( EUS_TRACK_TARGET, bTargetingTrack, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
	}
}

void CAIUnit::SetIgnoreAABBCoeff( const bool bIgnore ) 
{ 
	if ( bIgnoreAABBCoeff != bIgnore )
	{
		// Apply stat bonus
		const NDb::SUnitSpecialAblityDesc *pDesc = GetUnitAbilityDesc( NDb::ABILITY_EXACT_SHOT );
		if ( pDesc )
		{
			bIgnoreAABBCoeff = bIgnore; 
			if (  pDesc->pStatsBonus )
				ApplyStatsModifier(  pDesc->pStatsBonus, bIgnore );

			updater.AddUpdate( CreateStatusUpdate( EUS_TRACK_TARGET, bIgnoreAABBCoeff, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
		}
	}
}


bool CAIUnit::IsRestInside() const
{
	return bRestInside && IsInfantry();
}

void CAIUnit::SetRestInside( const bool bInside, CAIUnit *pTransport )
{
	bRestInside = bInside;
	pObjInside = pTransport;
}

void CAIUnit::GetEntranceStateInfo( struct SAINotifyEntranceState *pInfo ) const
{
	pInfo->nTargetUniqueID = pObjInside ? checked_cast<CUpdatableObj*>(pObjInside)->GetUniqueId() : 0;
	pInfo->bEnter = IsRestInside();
	pInfo->nInfantryUniqueID = GetUniqueId();
}

const float CAIUnit::GetAimTimeBonus() const
{ 
	if ( bTargetingTrack )
		return SConsts::TRACK_TARGETING_AIM_BONUS;
	return 1.0f; 
}

const SRect & CAIUnit::GetUnitRect() const
{
	static SRect unitRect;

	const CVec2 vSize = GetStats()->vAABBHalfSize * SConsts::BOUND_RECT_FACTOR;
	unitRect.InitRect( GetCenterPlain() + GetCenterShift(), GetFrontDirectionVector(), vSize.y, vSize.x );

	return unitRect;
}

const CVec2 CAIUnit::GetCenterShift() const
{
	const CVec2 realDirVec( GetFrontDirectionVector() );
	const CVec2 dirPerp( realDirVec.y, -realDirVec.x );

	return CVec2( realDirVec * GetStats()->vAABBCenter.y + dirPerp * GetStats()->vAABBCenter.x );
}

const float CAIUnit::GetTurnSpeed() const
{
	return GetStatsModifier()->rotateSpeed.Get( GetStats()->fRotateSpeed );
}

const SUnitProfile &CAIUnit::GetUnitProfile() const
{
	return unitProfile;
}

const float CAIUnit::GetVisZ() const
{
	const CVec2 vPos( GetCenterPlain() );
	return GetHeights()->GetVisZ( vPos.x, vPos.y ) + GetZ();
}

void CAIUnit::UpdateUnitProfile()
{
	if ( IsRound() && !IsStaticUnit() )
	{
		unitProfile.bRect = false;
		unitProfile.circle.center = GetCenterPlain() + GetCenterShift();
		unitProfile.circle.r = GetStats()->boundCircle.fRadius;
	}
	else
	{
		unitProfile.bRect = true;
		unitProfile.rect = GetUnitRect();
	}
}

void CAIUnit::UpdatePlacement( const CVec3 &vOldPosition, const WORD wOldDirection, const bool bNeedUpdate )
{
	CCommonUnit::UpdatePlacement( vOldPosition, wOldDirection, bNeedUpdate );
	if ( bNeedUpdate )
		UpdateUnitProfile();
	if ( !IsInSolidPlace() )
		units.UnitChangedPosition( this, GetCenterPlain() );
}

NTimer::STime CAIUnit::GetDisappearInterval() const
{ 
	return SConsts::TIME_TO_DISAPPEAR; 
}

const NTimer::STime CAIUnit::GetBehUpdateDuration() const
{ 
	return SConsts::BEH_UPDATE_DURATION; 
}

void CAIUnit::UnitTrampled( const CBasePathUnit *pTramplerUnit )
{
	const CAIUnit *pUnit = dynamic_cast<const CAIUnit *>( pTramplerUnit );
	NI_ASSERT( pUnit != 0, "Cannot cast pTramplerUnit to CCommonUnit" );
	if ( pUnit == 0 )
		return;

	theStatistics.UnitKilled( pUnit->GetPlayer(), GetPlayer(), GetStats()->fExpPrice, pUnit->GetReinforcementType(), 
		GetReinforcementType(), IsInfantry() );

	bTrampled = true;
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_DIE, false ), this, false );
}

void CAIUnit::UpdateVisibilityForced()
{
	const bool bVisible = visible4Party[theDipl.GetMyParty()] || visible4Party[theCheats.GetNPartyForWarFog()] ;
	updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_VISIBILITY, this, bVisible );
}

const NDb::SUnitSpecialAblityDesc * CAIUnit::GetUnitAbilityDesc( const NDb::EUnitSpecialAbility eType )
{
	const NDb::EUnitSpecialAbility eAbility = ( eType == NDb::ABILITY_PLACE_CONTROLLED_CHARGE || eType == NDb::ABILITY_DETONATE ) ? NDb::ABILITY_RADIO_CONTROLLED_MODE : eType;
	const std::vector< CDBPtr< SUnitSpecialAblityDesc > > &abilities = GetStats()->GetActions()->specialAbilities;
	const int nMaxAbilityCount = (std::min<int>)( GetAbilityLevel(), abilities.size() );
	for ( int i = 0; i < nMaxAbilityCount; ++i )
	{
		NI_ASSERT( abilities[i], StrFmt("No ability desc. Unit \"%s\", ability %d", NDb::GetResName(GetStats()), i ) );
		if ( abilities[i]->eName == eAbility )
		{
			if ( eAbility == NDb::ABILITY_RADIO_CONTROLLED_MODE )
			{
				for ( int n = i+1; n < abilities.size(); ++n )
				{
					NI_ASSERT( abilities[n], StrFmt("No ability desc. Unit \"%s\", ability %d", NDb::GetResName(GetStats()), n ) );
					if ( abilities[n]->eName == eType )
						return abilities[n];
				}
				return 0;
			}
			else
				return abilities[i];
		}
	}
	return 0;
}

const ECollidingType CAIUnit::GetCollidingType( CBasePathUnit *pUnit ) const
{
	if ( GetState()->GetName() != EUSN_MECHUNIT_REST_ON_BOARD )
		return bRestInside ? ECT_NONE : ECT_ALL;
	return ECT_NONE;
}

const bool CAIUnit::CanLockTiles() const
{
	if ( GetState()->GetName() != EUSN_MECHUNIT_REST_ON_BOARD )
		return CCommonUnit::CanLockTiles();
	return false;

}

const bool CAIUnit::IsRound() const 
{ 
	return g_bUseRoundUnits && GetStats()->boundCircle.bIsRound; 
}

void CAIUnit::SetReinforcementType( const NDb::EReinforcementType eType )
{
	if ( !GetScenarioTracker() )
		return;
	// Change level stats bonus to the new type
	if ( eReinforcementType != eType ) 
	{
		bool bOldRemoved = false;
		bool bNewApplied = false;
		const NDb::SAIGameConsts *pConsts = dynamic_cast<CAILogic*>(Singleton<IAILogic>())->GetAIConsts();

		for ( int i = 0; i < pConsts->common.expLevels.size(); ++i )
		{
			const NDb::SAIExpLevel *pLevels = pConsts->common.expLevels[i];
			if ( !pLevels )
				continue;

			if ( pLevels->eDBType == eReinforcementType && !bOldRemoved )
			{
				int nXPLevel = GetScenarioTracker()->GetReinforcementXPLevel( player, eReinforcementType );
				nXPLevel = (std::min<int>)( pLevels->levels.size() - 1, nXPLevel );
				if ( nXPLevel < 0 )
					continue;
				if ( pLevels->levels[nXPLevel].pStatsBonus )
				{
					ApplyStatsModifier( pLevels->levels[nXPLevel].pStatsBonus, false );
					bOldRemoved = true;
				}
			}
			else if ( pLevels->eDBType == eType && !bNewApplied )
			{
				int nXPLevel = GetScenarioTracker()->GetReinforcementXPLevel( player, eType );
				nXPLevel = (std::min<int>)( pLevels->levels.size() - 1, nXPLevel );
				if ( nXPLevel < 0 )
					continue;
				if ( pLevels->levels[nXPLevel].pStatsBonus )
				{
					ApplyStatsModifier( pLevels->levels[nXPLevel].pStatsBonus, true );
					bNewApplied = true;
				}
			}
		}


		eReinforcementType = eType;
	}
}

START_REGISTER( AIUnit )
REGISTER_VAR_EX( "ai_use_smart_scan", NGlobal::VarBoolHandler, &g_bUseSmartScan, true, STORAGE_NONE );
REGISTER_VAR_EX( "AI.Common.TimeForDangerousTurnRound", NGlobal::VarIntHandler, &g_dwTimeForDangerousTurnRound, 3000, STORAGE_NONE );
FINISH_REGISTER
