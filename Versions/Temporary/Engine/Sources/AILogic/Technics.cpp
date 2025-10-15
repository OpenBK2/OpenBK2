#include "stdafx.h"

#include "Technics.h"
#include "TankStates.h"
#include "TransportStates.h"
#include "UnitsIterators2.h"
#include "Commands.h"
#include "Formation.h"
#include "NewUpdater.h"
#include "Soldier.h"
#include "ArtilleryStates.h"
#include "StaticObjects.h"
#include "UnitGuns.h"
#include "Turret.h"
#include "Artillery.h"
#include "Cheats.h"
#include "ShootEstimatorInternal.h"
#include "Statistics.h"
#include "UnitCreation.h"
#include "GroupLogic.h"
#include "DifficultyLevel.h"
#include "PresizePath.h"
#include "Common_RTS_AI/StaticMapHeights.h"

#include "Stats_B2_M1/AdditionalActions.h"
#include "Stats_B2_M1/StatusUpdates.h"
#include "Misc/Checker.h"
#include "ExecutorTransportHealInfantry.h"
#include "SupportAAGun.h"
#include "ExecutorContainer.h"
#include "ExecutorWatchForEnemyUnloadPassangers.h"

// for profiling
#include "TimeCounter.h"
#include "FeedBackSystem.h"

#include "AILogic_export.h"

#include <algorithm>
#include <cstdint>

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D444, CTank );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D445, CAITransportUnit );

extern CFeedBackSystem theFeedBackSystem;
extern CGroupLogic theGroupLogic;
extern CUnitCreation theUnitCreation;
extern CStatistics theStatistics;
extern NTimer::STime curTime;
extern CEventUpdater updater;
extern CStaticObjects theStatObjs;
extern SCheats theCheats;
extern CDiplomacy theDipl;
extern CDifficultyLevel theDifficultyLevel;

extern NAI::CTimeCounter timeCounter;
extern CExecutorContainer theExecutorContainer;

//*******************************************************************
//*											CMilitaryCar																*
//*******************************************************************

BASIC_REGISTER_CLASS( AILOGIC, CMilitaryCar );

const bool CMilitaryCar::IsIdle() const
{
	if ( GetState()->GetName() == EUSN_MECHUNIT_REST_ON_BOARD )
		return false;
	return CAIUnit::IsIdle();
}

void CMilitaryCar::Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *_pStats, const float fHP, const uint16_t dir, const uint8_t player, ICollisionsCollector *pCollisionsCollector )
{
	bCanUnload = true;
	pStats = checked_cast<const SMechUnitRPGStats*>( _pStats );
	fDispersionBonus = 1.0f;
	
	CAIUnit::Init( center, z, fHP, dir, player, pCollisionsCollector );
	timeLastHeal = NRandom::Random( GetBehUpdateDuration() ); RecordRandomCall();


	//медицинские грузовички лечат пехоту в радиусе
	if ( CanCommandBeExecutedByStats( ACTION_COMMAND_HEAL_INFANTRY ) )
		pTheExecutorsContainer->Add( new CExecutorTransportHealInfantry( this ) );

	onBoard.resize( pStats->boardedMechUnitPosition.size() );
	// from time to time AI should scan for enemies and unload 
	// passengers if enemy found
	if ( theDipl.IsAIPlayer( GetPlayer() ) && pStats->nPassangers != 0 && !pStats->IsAviation() )
		theExecutorContainer.Add( new CExecutorWatchForEnemyUnloadPassangers( this ) );

}

const bool CMilitaryCar::CanBoard( CAIUnit *pUnit ) const
{
	if ( boarding.find( pUnit->GetUniqueId() ) != boarding.end() )
		return true;

	for ( int i = 0; i < onBoard.size(); ++i )
	{
		if ( !IsValidObj( onBoard[i] ) )
			return true;
	}
	return false;
}

uint32_t CMilitaryCar::InitSupportAntiAircraftGuns()
{
	// dissalow to shoot from AA guns if there is non AA guns.
	// in that case all AA guns will be controlled by special state.
	
	uint32_t dwDissalow = 0;
	for ( int nGun = 0; nGun < GetNGuns(); ++nGun )
	{
		CBasicGun * pGun = GetGun( nGun );
		const SBaseGunRPGStats &stats = pGun->GetGun();
		if ( stats.bTargetAAOnly )
		{
			// gun able to shoot only to planes
			dwDissalow |= (1<<nGun);
			// create special AA-only behavior
			CTurret *pTurret = pGun->GetTurret();
			NI_ASSERT( pTurret != 0, StrFmt("Anti Aircraft gun \"%s\" don't have turret", NDb::GetResName(pStats)) );
			if ( pTurret )
			{
				CSupportAAGuns::iterator pos = supportAAGuns.find( pTurret->GetUniqueId() );
				if ( pos == supportAAGuns.end() )
				{
					supportAAGuns[pTurret->GetUniqueId()] = new CSupportAAGun( this );
					pos = supportAAGuns.find( pTurret->GetUniqueId() );
				}
				pos->second->AddGunNumber( nGun );
			}
		}
	}
	return dwDissalow;
}

void CMilitaryCar::AddBoardingMechUnit( CAIUnit *pUnit )
{
	boarding[pUnit->GetUniqueId()] = pUnit;
}

void CMilitaryCar::SetOnBoard( CAIUnit *pUnit, const bool bOnBoard )
{
	//onBoard[pUnit->GetUniqueId()] = pUnit;
	for ( int i = 0; i < onBoard.size(); ++i )
	{
		if ( bOnBoard )
		{
			if ( !IsValidObj( onBoard[i] ) )
			{
				onBoard[i] = pUnit;
				boardOrder[pUnit->GetUniqueId()] = i; 
				pUnit->SetSmoothPath( new CMechUnitRestOnBoardPath ( pUnit, this ) );
				return;
			}
		}
		else
		{
			if ( onBoard[i] == pUnit )
			{
				onBoard[i] = 0;
				pUnit->RestoreSmoothPath();
				return;
			}
		}
	}
}

void CMilitaryCar::RemoveBoardingMechUnit( CAIUnit *pUnit )
{
	boarding.erase( pUnit->GetUniqueId() );
}

void CMilitaryCar::GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff )
{
	CAIUnit::GetPlacement( pPlacement, timeDiff );
	//CRAP{ DON'T KNOW HOW TO DO
	if ( GetState()->GetName() == EUSN_MECHUNIT_REST_ON_BOARD )
	{
		CDynamicCast<CMilitaryCar> pInside = GetObjInside();
		if ( IsValidObj( pInside ) )
		{
			const CVec3 vSpeed( GetSpeed() * GetDirectionVector(), 0.0f );
			const CVec3 vPosition( pInside->GetBoardedPosition( this, timeDiff ) );

			pPlacement->center.x = vPosition.x;
			pPlacement->center.y = vPosition.y;
			pPlacement->z = vPosition.z;
			pPlacement->dir = pInside->GetBoardedDirection( this, timeDiff );
			pPlacement->fWaterCoeff = 0.0f;
		}
	}
	//CRAP}
}

const uint16_t CMilitaryCar::GetBoardedDirection( CAIUnit *pUnit, const NTimer::STime timeDiff ) const
{
	CBoardOrder::const_iterator pos = boardOrder.find( pUnit->GetUniqueId() );
	NI_ASSERT( pos != boardOrder.end(), "unit is not borded" );
	if ( pos != boardOrder.end() )
	{
		return uint16_t(pStats->boardedMechUnitPosition[pos->second].nDirection) + GetFrontDirection();
	}
	return 0;
}

const CVec3 CMilitaryCar::GetBoardedPosition( CAIUnit *pUnit, const NTimer::STime timeDiff ) const
{
	CBoardOrder::const_iterator pos = boardOrder.find( pUnit->GetUniqueId() );
	NI_ASSERT( pos != boardOrder.end(), "unit is not borded" );
	if ( pos != boardOrder.end() )
	{
		const CVec3 vBoardPos( pStats->boardedMechUnitPosition[pos->second].vPos );
		const CVec2 vBoardPos2D( vBoardPos.y, -vBoardPos.x );
		
		const CVec3 vSpeed( GetSpeed() * GetDirectionVector(), 0.0f );
		const CVec3 vCenter( GetCenter() - timeDiff * vSpeed );

		const CVec2 vCenter2D( vCenter.x, vCenter.y );
		const CVec2 vDir( GetFrontDirectionVector() );

		return CVec3( vCenter2D + (vBoardPos2D ^ vDir), vCenter.z + vBoardPos.z );
	}
	else
		return VNULL3;
}

void CMilitaryCar::InitGuns()
{
	if ( pStats->GetPlatformsSize( GetUniqueId() ) > 1 )
	{
		const int nTurrets = pStats->GetPlatformsSize( GetUniqueId() ) - 1;
		turrets.resize( nTurrets );

		for ( int i = 0; i < nTurrets; ++i )
		{
			const SMechUnitRPGStats::SPlatform &platform = pStats->GetPlatform( GetUniqueId(), i+1 );
			bool bBackGuns = true;
			for ( int n = 0; n < platform.guns.size() && bBackGuns; ++n )
			{
				if ( platform.guns[n].wDirection < 16384 || platform.guns[n].wDirection > 49152 )
					bBackGuns = false;
			}
			turrets[i] = new CUnitTurret( this, i + 1,
																		platform.wHorizontalRotationSpeed, platform.wVerticalRotationSpeed,
																		platform.constraint.wMax, platform.constraintVertical.wMax, bBackGuns );
		}
	}

	pGuns = new CMechUnitGuns;
	pGuns->Init( this );

	SetShootEstimator( new CTankShootEstimator( this ) );
}

const CVec2 CMilitaryCar::GetGunCenter( const int nGun ) const
{
	return GetCenterPlain();
}

const CVec2 CMilitaryCar::GetEntrancePoint() const
{
	const CVec2 vFrontDir = GetVectorByDirection( GetFrontDirection() );
	const CVec2 vTurn( vFrontDir.y, -vFrontDir.x );
	const CVec2 vEntrNow( (pStats->vEntrancePoint) ^ vTurn );

	CVec2 vResult( GetCenterPlain() + vEntrNow );

	const SVector tile = AICellsTiles::GetTile( vResult );
	if ( GetTerrain()->IsLocked( tile, EAC_HUMAN ) )
	{
		CVec2 vDir( vEntrNow );
		Normalize( &vDir );

		vResult += vDir * SConsts::TILE_SIZE / 2;

		if ( GetTerrain()->IsLocked( tile, EAC_HUMAN ) )
			vResult += vDir * SConsts::TILE_SIZE / 2;
	}

	return vResult;
}

void CMilitaryCar::AddPassenger( CSoldier *pUnit )
{
	if ( pass.empty() )
	{
		for ( int i = 0; i < GetNTurrets(); ++i )
			GetTurret( i )->SetRotateTurretState( false );
	}
	
	pass.push_back( pUnit );
	updater.AddUpdate( 0, ACTION_NOTIFY_ENTRANCE_STATE, pUnit, -1 );
	pUnit->ApplyStatsModifier( pStats->pInnerUnitBonus, true );
}

void CMilitaryCar::PrepareToDelete()
{
	// всех сидящих внутри - убить.
	while ( GetNPassengers() )
		GetPassenger( 0 )->Die( false, 0 );

	for ( std::vector<CPtr<CAIUnit> >::iterator it = onBoard.begin(); it != onBoard.end(); ++it )
	{
		if ( IsValidObj( *it ) )
			(*it)->Die( false, 0 );
	}
	CAIUnit::PrepareToDelete();
}

void CMilitaryCar::SendNTotalKilledUnits( const int nPlayerOfShoot, NDb::EReinforcementType eKillerType, NDb::EReinforcementType eDeadType )
{
	for ( std::list<CPtr<CSoldier> >::const_iterator iter = pass.begin(); iter != pass.end(); ++iter )
	{
		CAIUnit *pUnit = *iter;
		
		theStatistics.UnitKilled( nPlayerOfShoot, pUnit->GetPlayer(), pUnit->GetStats()->fExpPrice, eKillerType, eDeadType, true );
	}

	theStatistics.UnitKilled( nPlayerOfShoot, GetPlayer(), GetStats()->fExpPrice, eKillerType, eDeadType, false );
}

const CVec2 CMilitaryCar::GetPassengerCoordinates( const int n )
{
	const int nPass = GetNPassengers();
	// солдат на стороне
	const int nSoldiersOnSide = ( n > nPass / 2 ) ? ( nPass / 2 + nPass % 2 ) : ( nPass / 2 );
	// знак, отвечающий за сторону, на которой сидит солдат (поворот frontDirVec - влево или вправо)
	float fSideSign = ( n > nPass / 2 ) ? -1.0f : 1.0f;

	const int nSoldierIndex = ( n <= nPass / 2 ) ? n : n - nPass / 2;

	const float fSideHalfLen = GetStats()->vAABBHalfSize.y / 2.0f;
	const CVec2 vFrontDirVec = GetVectorByDirection( GetFrontDirection() );

	const CVec2 vShift = 
			( 2 * fSideHalfLen * (float)nSoldierIndex / float( nSoldiersOnSide + 1 ) - fSideHalfLen ) * vFrontDirVec +
			 fSideSign * 3.0f / 4.0f * CVec2( -vFrontDirVec.y, vFrontDirVec.x );

	return GetCenterPlain() + vShift;
}

void CMilitaryCar::Segment()
{
	CAIUnit::Segment();

	for ( CSupportAAGuns::iterator it = supportAAGuns.begin(); it != supportAAGuns.end(); ++it )
		it->second->Segment();

	pGuns->Segment();
	for ( int i = 0; i < GetNTurrets(); ++i )
		GetTurret( i )->Segment();

	if ( pLockingUnit != 0 && ( !pLockingUnit->IsRefValid() || !pLockingUnit->IsAlive() ) )
		pLockingUnit = 0;
	
	// CRAR{ соптимизировать!, не посылать, если координаты не изменились
	int i = 0;
	for ( std::list<CPtr<CSoldier> >::iterator iter = pass.begin(); iter != pass.end(); ++iter, ++i )
	{
		CSoldier *pSoldier = *iter;
		const CVec2 vPassCoord( GetPassengerCoordinates( i ) );
		const CVec3 vCenter( vPassCoord, GetHeights()->GetZ( vPassCoord ) );

		pSoldier->SetCenter( vCenter, !pSoldier->IsInSolidPlace() );
		
		NI_ASSERT( pSoldier->IsInTransport(), "Wrong state of the intransport soldier" );

		updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, pSoldier, -1 );
	}
	// CRAP}

	// check if unit can carry mech units and entrance is locked for boarded units
	const SVector vEntranceTile( AICellsTiles::GetTileNC( GetEntrancePoint() ) );
	bool bNewCanUnload = GetAIMap()->IsTileInside( vEntranceTile ) && 
		!GetTerrain()->IsLocked( vEntranceTile, EAC_TERRAIN ) /*&& !IsTowing()*/;

	if ( !bNewCanUnload )
		bNewCanUnload = IsTowing();
	if ( bNewCanUnload != bCanUnload )
	{
		updater.AddUpdate( 0, ACTION_NOTIFY_MODIFY_ENTRANCE_STATE, this, bNewCanUnload );
		bCanUnload = bNewCanUnload;
	}
}

CSoldier* CMilitaryCar::GetPassenger( const int n )
{
	NI_ASSERT( n < pass.size(), "Wrong number of passenger" );

	std::list< CPtr<CSoldier> >::iterator pos = pass.begin();
	advance( pos, n );
	return *pos;
}

void CMilitaryCar::ClearAllPassengers()
{
	pass.clear();
}

void CMilitaryCar::DelPassenger( const int n )
{
	CheckRange( pass, n );
	std::list< CPtr<CSoldier> >::iterator pos = pass.begin();
	advance( pos, n );
	(*pos)->ApplyStatsModifier( pStats->pInnerUnitBonus, false );
	pass.erase( pos );
	//
	if ( pass.empty() )
	{
		for ( int i = 0; i < GetNTurrets(); ++i )
			GetTurret( i )->SetRotateTurretState( true );
	}
}

void CMilitaryCar::DelPassenger( CSoldier *pSoldier )
{
	NI_ASSERT( find( pass.begin(), pass.end(), pSoldier ) != pass.end(), "Intransport soldier not found" );
	pass.remove( pSoldier );
	pSoldier->ApplyStatsModifier( pStats->pInnerUnitBonus, false );

	//
	if ( pass.empty() )
	{
		for ( int i = 0; i < GetNTurrets(); ++i )
			GetTurret( i )->SetRotateTurretState( true );
	}
}

float CMilitaryCar::GetDistanceToLandPoint() const
{
	return GetStats()->vAABBHalfSize.y + SConsts::GOOD_LAND_DIST;
}

float CMilitaryCar::GetMaxFireRange() const
{
	return pGuns->GetMaxFireRange( this );
}

void CMilitaryCar::Lock( CFormation *_pLockingUnit ) 
{ 
	NI_ASSERT( pLockingUnit == 0, "Transport is already locked" ); 
	pLockingUnit = _pLockingUnit; 
}

int CMilitaryCar::GetNGuns() const { return pGuns->GetNTotalGuns(); }

CBasicGun* CMilitaryCar::GetGun( const int n ) const { return pGuns->GetGun( n ); }

CBasicGun* CMilitaryCar::ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime ) 
{ 
	return pGuns->ChooseGunForStatObj( this, pObj, pTime ); 
}

const bool CMilitaryCar::CanShootToPlanes() const 
{ 
	return pGuns->CanShootToPlanes(); 
}

CBasicGun* CMilitaryCar::GetFirstArtilleryGun() const
{
	return pGuns->GetFirstArtilleryGun(); 
}

void CMilitaryCar::GetRangeArea( SShootAreas *pRangeArea ) const
{
	new ( pRangeArea ) SShootArea();
	if ( GetState()->GetName() == EUSN_RANGING )
	{
		CCircle rangeCircle;
		checked_cast<const CArtilleryRangeAreaState*>(GetState())->GetRangeCircle( &rangeCircle );
		pRangeArea->areas.push_back( SShootArea() );

		SShootArea &area = pRangeArea->areas.back();
		area.eType = SShootArea::ESAT_RANGE_AREA;
		area.fMinR = 0.0f;
		area.fMaxR = rangeCircle.r;
		area.vCenter3D = CVec3( rangeCircle.center, 0.0f );
		area.wStartAngle = 65535;
		area.wFinishAngle = 65535;
	}
}

const CVec3 CMilitaryCar::GetHookPoint3D() const
{
	const CVec3 vTraNormale = DWORDToVec3( GetNormale() );
	const CVec2 vTraDir = GetVectorByDirection( GetFrontDirection() );
	CVec3 vTraDir3D;
	vTraDir3D.x = vTraDir.x;
	vTraDir3D.y = vTraDir.y;
	vTraDir3D.z = ( -vTraDir3D.x * vTraNormale.x - vTraDir3D.y * vTraNormale.y ) / vTraNormale.z;
	Normalize( &vTraDir3D );

	const CVec2 vTraCenter( GetCenterPlain() );
	CVec3 vTraCenter3D( vTraCenter, GetHeights()->GetVisZ( vTraCenter.x, vTraCenter.y ) );

	return vTraCenter3D + vTraDir3D * checked_cast<const SMechUnitRPGStats*>( GetStats() )->vTowPoint.y;
}

const CVec2 CMilitaryCar::GetHookPoint() const
{
	const CVec3 vHookPoint3D( GetHookPoint3D() );
	return CVec2( vHookPoint3D.x, vHookPoint3D.y );
}

void CMilitaryCar::LookForTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, CBasicGun **pGun )
{
	CAIUnit::LookForTarget( pCurTarget, bDamageUpdated, pBestTarget, pGun );

	if ( GetFirstArtilleryGun() != 0 )
	{
		// в радиусе цели нет
		if ( *pBestTarget == 0 && 
				 ( pCurTarget == 0 || pCurTarget->GetStats()->IsInfantry() ) && theDipl.IsAIPlayer( GetPlayer() ) )
		{
			LookForFarTarget( pCurTarget, bDamageUpdated, pBestTarget, pGun );
		}
	}
}

const bool CMilitaryCar::CanUnitTrampled( const CBasePathUnit *pTramplerUnit ) const
{
	return false;
}

//*******************************************************************
//*															CTank																*
//*******************************************************************

void CTank::Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const uint16_t dir, const uint8_t player, ICollisionsCollector *pCollisionsCollector )
{
	bTrackDamaged = false;
	wDangerousDir = 0;
	bDangerousDirSet = false;
	bDangerousDirSetInertia = false;
	nextTimeOfDangerousDirScan = 0;
	lastTimeOfDangerousDirChanged = 0;

	wDangerousDirUnderFire = 0;
	fDangerousDamageUnderFire = -1.0f;

	CMilitaryCar::Init( center, z, pStats, fHP, dir, player, pCollisionsCollector );
}

IStatesFactory* CTank::GetStatesFactory() const
{ 
	return CTankStatesFactory::Instance(); 
}

void CTank::TakeDamage( const float fDamage, const SWeaponRPGStats::SShell *pShell, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	if ( pShotUnit && pShotUnit->IsTargetingTrack() )
	{
		const NDb::SUnitSpecialAblityDesc *pSA = GetUnitAbilityDesc(NDb::ABILITY_TRACK_TARGETING);
		const float fTrackDamageCoeff = pSA ? pSA->fParameter : SConsts::TRACK_TARGETING_DAMAGE_MODIFIER;
		CMilitaryCar::TakeDamage( fDamage * fTrackDamageCoeff, pShell, nPlayerOfShoot, pShotUnit );
	}
	else
		CMilitaryCar::TakeDamage( fDamage , pShell, nPlayerOfShoot, pShotUnit );

	// обработать specials
	if ( IsAlive() && theCheats.GetImmortals( theDipl.GetNParty( nPlayerOfShoot ) ) != 1 )
	{
		// отрывает гусеницу, всегда, даже на Easy, дизайнеры захотели
		// cannot recive ->special from DB
		if ( /*theDifficultyLevel.GetLevel() != 0 &&*/ pShell && ( (NRandom::Random( 0.0f, 1.0f ) < pShell->fBrokeTrackProbability) & RecordRandomCall() ) || ( pShotUnit && pShotUnit->IsTargetingTrack() ) )
		{
			Stop();
			updater.AddUpdate( 0, ACTION_NOTIFY_BREAK_TRACK, this, -1 );
			updater.AddUpdate( CreateStatusUpdate( EUS_TRACK_DAMAGED, true, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
			
			if ( GetPlayer() == theDipl.GetMyNumber() && pShotUnit == 0 ) // only mines are needed
				theFeedBackSystem.AddFeedbackAndForget( GetUniqueID(), GetCenterPlain(), EFB_TRACK_BROKEN, -1 );
			bTrackDamaged = true;
			TrackDamagedState( true );
			if ( pShotUnit )
				pShotUnit->SetTrackTargeting( false );			// CRAP: Turn TT off immediately
		}
	}
}

void CTank::RepairTrack() 
{
	if ( bTrackDamaged )
	{
		bTrackDamaged = false;
		// theFeedBackSystem.RemoveFeedback( GetUniqueID(), EFB_TRACK_BROKEN );
		TrackDamagedState( false );
		updater.AddUpdate( 0, ACTION_NOTIFY_REPAIR_TRACK, this, -1 );
		updater.AddUpdate( CreateStatusUpdate( EUS_TRACK_DAMAGED, false, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, this, -1 );
	}
}

bool CTank::CanTurnToFrontDir( const uint16_t wDir )
{ 
	return !bTrackDamaged && CAIUnit::CanTurnToFrontDir( wDir ) && !IsInTankPit();
}

const bool CTank::CanMove() const
{
	return CMilitaryCar::CanMove() && !bTrackDamaged && !IsInTankPit();
}

const bool CTank::CanMoveCritical() const
{
	return CMilitaryCar::CanMoveCritical() && !bTrackDamaged && !IsInTankPit();
}

const bool CTank::CanRotate() const
{ 
	return GetStats()->fSpeed != 0 && 
		//CanMove() && 
		!bTrackDamaged && !IsInTankPit() &&
		!IsRestInside() &&
		CMilitaryCar::CanRotate() && !bTrackDamaged && !IsInTankPit();
}

void CTank::ScanForDangerousDir()
{
	if ( nextTimeOfDangerousDirScan < curTime )
	{
		const float fR = 1.3f * (std::max)( GetSightRadius(), (std::min)( GetMaxFireRange(), SConsts::MAX_FIRE_RANGE_TO_SHOOT_BY_LINE ) );
		const CVec2 vCenter( GetCenterPlain() );
		const int nParty = GetParty();

		float fDangerousDamage = fDangerousDamageUnderFire;
		bool bNewDangerousDirSet = fDangerousDamageUnderFire > 0.0f;
		if ( bNewDangerousDirSet )
		{
			wDangerousDir = wDangerousDirUnderFire;
		}
		fDangerousDamageUnderFire = -1.0f;

		for ( CUnitsIter<1,3> iter( nParty, EDI_ENEMY, vCenter, fR ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pUnit = *iter;
			if ( IsValidObj(pUnit) && ( pUnit->IsVisible( nParty ) || pUnit->IsRevealed() ) && pUnit->GetCenter().z <= 0.0f )
			{
				const float fDamage = pUnit->GetMaxDamage( this );
				if ( fDamage > fDangerousDamage )
				{
					fDangerousDamage = fDamage;
					wDangerousDir = GetDirectionByVector( pUnit->GetCenterPlain() - vCenter );
					bNewDangerousDirSet = true;
				}
			}
		}

		// changed
		if ( bDangerousDirSet != bNewDangerousDirSet )
		{
			lastTimeOfDangerousDirChanged = curTime;

			if ( bNewDangerousDirSet || lastTimeOfDangerousDirChanged + 3000 < curTime )
				bDangerousDirSetInertia = bNewDangerousDirSet;
		}
		else if ( bDangerousDirSetInertia != bDangerousDirSet && lastTimeOfDangerousDirChanged + 3000 < curTime )
			bDangerousDirSetInertia = bDangerousDirSet;

		bDangerousDirSet = bNewDangerousDirSet;
		nextTimeOfDangerousDirScan = curTime + 1000 + NRandom::Random( 0, 2000 ); RecordRandomCall();
	}
}

void CTank::Grazed( CAIUnit *pUnit )
{
	if ( IsValidObj( pUnit ) && !pUnit->GetStats()->IsAviation() )
	{
		const float fDamage = pUnit->GetMaxDamage( this );
		if ( fDamage > fDangerousDamageUnderFire )
		{
			fDangerousDamageUnderFire = fDamage;
			wDangerousDirUnderFire = GetDirectionByVector( pUnit->GetCenterPlain() - GetCenterPlain() );
		}
	}
}

void CTank::Segment()
{
	CMilitaryCar::Segment();
	ScanForDangerousDir();
}

bool CTank::CanMoveAfterUserCommand() const
{
	return !IsTrackDamaged() && CAIUnit::CanMoveAfterUserCommand();
}

//*******************************************************************
//*														CAITransportUnit											*
//*******************************************************************

//BASIC_REGISTER_CLASS( CAITransportUnit );

void CAITransportUnit::Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *_pStats, const float fHP, const uint16_t dir, const uint8_t player, ICollisionsCollector *pCollisionsCollector )
{
	CMilitaryCar::Init( center, z, _pStats, fHP, dir, player, pCollisionsCollector );
	fResursUnits = SConsts::TRANSPORT_RU_CAPACITY;
}

void CAITransportUnit::SetResursUnitsLeft( float _fResursUnits ) 
{ 
	if ( fResursUnits != _fResursUnits )
	{
		fResursUnits = _fResursUnits; 
		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
	}
}

void CAITransportUnit::Segment()
{
	CMilitaryCar::Segment();

	if ( pTowedArtillery && !IsTowing() ) // убили буксоируемую пушку
	{
		updater.AddUpdate( 0, ACTION_NOTIFY_STATE_CHANGED, this, ECS_UNHOOK_CANNON );
		pTowedArtillery = 0;
	}
}

void CAITransportUnit::DecResursUnitsLeft( float dRU ) 
{
	theStatistics.ResourceUsed( GetPlayer(), dRU );
	SetResursUnitsLeft( fResursUnits - dRU );
}

IStatesFactory* CAITransportUnit::GetStatesFactory() const 
{ 
	return CTransportStatesFactory::Instance(); 
}

bool CAITransportUnit::HasTowedArtilleryCrew() const 
{ 
	return IsValidObj( pTowedArtilleryCrew ); 
}

void CAITransportUnit::SetTowedArtilleryCrew( class CFormation *pFormation ) 
{ 
	pTowedArtilleryCrew = pFormation; 
}

CFormation * CAITransportUnit::GetTowedArtilleryCrew() 
{ 
	return pTowedArtilleryCrew; 
}

void CAITransportUnit::SetMustTow( class CAIUnit *_pUnit ) 
{ 
	pMustTow = _pUnit; 
}

bool CAITransportUnit::IsMustTow() const 
{ 
	return IsValidObj( pMustTow );
}

bool CAITransportUnit::CanCommandBeExecuted( CAICommand *pCommand )
{
	return	CMilitaryCar::CanCommandBeExecuted( pCommand ) &&
					
		( !IsValidObj( pTowedArtillery ) ||
			pCommand->ToUnitCmd().nCmdType != ACTION_COMMAND_TAKE_ARTILLERY || 
			pCommand->ToUnitCmd().nCmdType != ACTION_COMMAND_UNLOAD ) ;
}

bool CAITransportUnit::IsTowing() const 
{ 
	return IsValidObj( pTowedArtillery );
}

bool CAITransportUnit::UpdateExternalLoaders()
{
	for ( CExternLoaders::iterator it = externLoaders.begin(); it != externLoaders.end(); )
	{
		if ( !IsValidObj(*it) )
			it = externLoaders.erase( it );
		else
			++it;
	}
	return externLoaders.empty();
}

void CAITransportUnit::Die( const bool fromExplosion, const float fDamage )
{
	CMilitaryCar::Die( fromExplosion, fDamage );
	for ( CExternLoaders::iterator it = externLoaders.begin(); it != externLoaders.end(); )
	{
		FreeLoaders( *it, this );
		it = externLoaders.erase( it );
	}
}

void CAITransportUnit::GetRPGStats( SAINotifyRPGStats *pStats )
{
	CMilitaryCar::GetRPGStats( pStats );
	if ( GetNCommonGuns() == 0 )
		pStats->nSupply = fResursUnits / SConsts::TRANSPORT_RU_CAPACITY * 1000.0f;
}

void CAITransportUnit::SetTowedArtillery( CArtillery * _pTowedArtillery) 
{ 
	if ( IsValid( pTowedArtillery ) && !IsValid( _pTowedArtillery ) )
		pTowedArtillery->SetBeingHooked( 0 );
	pTowedArtillery = _pTowedArtillery; 
	updater.AddUpdate( 0, ACTION_NOTIFY_STATE_CHANGED, this, GetUnitState() );
}

const int CAITransportUnit::GetUnitState() const
{
	if ( pTowedArtillery )
		return ECS_HOOK_CANNON;
	else
		return ECS_UNHOOK_CANNON;
}

void CAITransportUnit::FreeLoaders( CFormation *pLoaderSquad, CAITransportUnit * pTransport ) 
{
	//kill loaders in transport
	if ( pLoaderSquad && pLoaderSquad->IsRefValid() && pLoaderSquad->IsAlive() )
	{
		std::list<CSoldier*> soldiersInside;
		const int nSize = pLoaderSquad->Size();
		for ( int i = 0; i < nSize; ++i )
		{
			CSoldier * pSold = (*pLoaderSquad)[i];
			if ( pSold->IsInSolidPlace() )
				soldiersInside.push_back( pSold );
		}
		while ( !soldiersInside.empty() )
		{
			(*soldiersInside.begin())->Die( false, 0.0f );
			soldiersInside.pop_front();
		}
	}

	// free others
	if ( pLoaderSquad && pLoaderSquad->IsRefValid() && pLoaderSquad->IsAlive() )
	{
		pLoaderSquad->SetResupplyable( true );
		theUnitCreation.SendFormationToWorld( pLoaderSquad, pLoaderSquad->GetPlayer() == theDipl.GetMyNumber() );
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_MOVE_TO, pLoaderSquad->GetCenterPlain()), pLoaderSquad, false );
	}
	if ( pTransport && pTransport->IsRefValid() && pTransport->IsAlive() )
		pTransport->Unlock();
}

void CAITransportUnit::PrepareLoaders( CFormation *pLoaderSquad, CAITransportUnit * pTransport ) 
{
	NI_ASSERT( pTransport->IsRefValid() && pTransport->IsAlive(), " not valid transport passed" );

	updater.AddUpdate( 0, ACTION_NOTIFY_NEW_UNIT, pLoaderSquad, -1 );
	const CVec3 vEntrancePoint( GetHeights()->Get3DPoint( pTransport->GetEntrancePoint() ) );
	pLoaderSquad->SetResupplyable( false );
	pLoaderSquad->SetCenter( vEntrancePoint );
	pLoaderSquad->SetFree();

	for ( int i = 0; i < pLoaderSquad->Size(); ++ i )
	{
		CPtr<CSoldier> pLandUnit = (*pLoaderSquad)[i];
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_UNIT, pLandUnit, -1 );
		pLandUnit->InitSpecialAbilities();
		updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_VISIBILITY, pLandUnit, pLandUnit->IsVisibleByPlayer() );
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_FORMATION, pLandUnit, -1 );
		updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, pLandUnit, -1 );

		if ( pLandUnit->IsInSolidPlace() )
			pLandUnit->SetCenter( vEntrancePoint, false );
		else
			pLandUnit->SetCenter( vEntrancePoint );

		pLandUnit->SetFree();
	}				
	pLoaderSquad->SetSelectable( false, true );
}

void CAITransportUnit::AddExternLoaders( CFormation *pLoaders )
{ 
	if ( pLoaders && pLoaders->IsRefValid() && pLoaders->IsAlive() )
		externLoaders.push_back( pLoaders );
}

bool CAITransportUnit::CanHookUnit( CAIUnit *pUnitToHook ) const
{
	if ( CanCommandBeExecutedByStats( ACTION_COMMAND_TAKE_ARTILLERY ) )
	{
		const float fWeight = pUnitToHook->GetStats()->fWeight;
		const float fTowForce = checked_cast<const SMechUnitRPGStats*>(GetStats())->fTowingForce; 

		return fWeight <= fTowForce;
	}
	else
		return false;
}

const int CAITransportUnit::GetNUnitToTakeArtillery( bool bPlaceInQueue, CAIUnit *pUnitToTake )
{
	float fMinDist = 0.0f;
	CCommonUnit *pBestUnit = 0;
	
	const int nGroup = GetNGroup();
	for ( int i = theGroupLogic.BeginGroup( nGroup ); i != theGroupLogic.EndGroup(); i = theGroupLogic.Next( i ) )
	{
		CCommonUnit *pUnit = theGroupLogic.GetGroupUnit( i );
		const bool bDoItNow = !bPlaceInQueue || pUnit->IsEmptyCmdQueue() ||
												  pUnit->GetNextCommand()->ToUnitCmd().nCmdType == ACTION_COMMAND_STOP;
													
		if ( pUnit->CanHookUnit( pUnitToTake ) && ( !bDoItNow || !pUnit->IsTowing() ) )
		{
			if ( ( !pUnit->GetState() ||  pUnit->GetState()->GetName() != EUSN_HOOK_ARTILLERY ) &&
					 ( pUnit->IsEmptyCmdQueue() || pUnit->GetLastCommand()->ToUnitCmd().nCmdType != ACTION_COMMAND_TAKE_ARTILLERY ) )
			{
				const float fDist = fabs2( pUnit->GetCenterPlain() - pUnitToTake->GetCenterPlain() );
				if ( pBestUnit == 0 || fMinDist > fDist )
				{
					fMinDist = fDist;
					pBestUnit = pUnit;
				}
			}
		}
	}

	if ( pBestUnit == 0 )
		return -2;
	else
		return pBestUnit->GetUniqueId();
}

void CAITransportUnit::UnitCommand( CAICommand *pCommand, bool bPlaceInQueue, bool bOnlyThisUnitCommand )
{
	if ( !bOnlyThisUnitCommand && pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_TAKE_ARTILLERY )
	{
		if ( pCommand->ToUnitCmd().nObjectID == 0 ) 
		{
			CMilitaryCar::UnitCommand( pCommand, bPlaceInQueue, bOnlyThisUnitCommand );
			return;
		}

		CAIUnit *pArtillery = dynamic_cast<CAIUnit*>( GetObjectByCmd( pCommand->ToUnitCmd() ));
		if ( pArtillery )
		{
			int nUnitToTakeArtillery = pCommand->GetFlag();
			if ( nUnitToTakeArtillery == -1 )
			{
				nUnitToTakeArtillery = GetNUnitToTakeArtillery( bPlaceInQueue, pArtillery );
				pCommand->SetFlag( nUnitToTakeArtillery );
			}

			// can't hook artillery
			if ( nUnitToTakeArtillery == -2 )
			{
				if ( !CanHookUnit( pArtillery ) & !bPlaceInQueue )
					SendAcknowledgement( pCommand, ACK_NEGATIVE_NOTIFICATION );
			}

			if ( nUnitToTakeArtillery == GetUniqueId() )
				CMilitaryCar::UnitCommand( pCommand, bPlaceInQueue, bOnlyThisUnitCommand );
			else if ( !bPlaceInQueue )
				UnitCommand( new CAICommand( SAIUnitCmd( ACTION_COMMAND_STOP ) ), false, bOnlyThisUnitCommand );
		}
	}
	else
		CMilitaryCar::UnitCommand( pCommand, bPlaceInQueue, bOnlyThisUnitCommand );
}

const bool CAITransportUnit::CheckTurn( const float fRectCoeff, const CVec2 &vDir, const bool bWithUnits, const bool bCanGoBackward ) const
{
	if ( pTowedArtillery )
		GetTerrain()->TemporaryUnlockUnitProfile( pTowedArtillery->GetUniqueID(), pTowedArtillery->GetUnitProfile(), pTowedArtillery->IsLockingTiles(), false );

	const bool bResult = CMilitaryCar::CheckTurn( fRectCoeff, vDir, bWithUnits, bCanGoBackward );

	if ( pTowedArtillery )
		GetTerrain()->RemoveTemporaryUnlocking( pTowedArtillery->GetUniqueID() );

	return bResult;
}

bool CAITransportUnit::CalculateUnitVisibility4Party( const uint8_t party )
{
	if ( IsTowing() )
		return IsVisible( party );
	else
		return CMilitaryCar::CalculateUnitVisibility4Party( party );
}


