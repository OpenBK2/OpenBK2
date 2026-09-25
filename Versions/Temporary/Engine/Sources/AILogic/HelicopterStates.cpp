#include "stdafx.h"

#include "HelicopterStates.h"
#include "Helicopter.h"
#include "AIClassesID.h"
#include "Commands.h"
#include "CommonStates.h"
#include "Formation.h"
#include "Soldier.h"
#include "Randomize.h"
#include "Guns.h"
#include "GroupLogic.h"
#include "LinkObject.h"
#include "Weather.h"
#include "DBAIConsts.h"
#include "NewUpdater.h"
#include "Shell.h"
#include "AILogicInternal.h"
#include "AIMap.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/StaticMapHeights.h"

REGISTER_SAVELOAD_CLASS( 0x1919A2C2, CHelicopterStatesFactory );
REGISTER_SAVELOAD_CLASS( 0x1919A2C3, CHelicopterRestState );
REGISTER_SAVELOAD_CLASS( 0x1919A2C4, CHelicopterMoveState );
REGISTER_SAVELOAD_CLASS( 0x1919A2C5, CHelicopterRotateState );
REGISTER_SAVELOAD_CLASS( 0x1919A2C6, CHelicopterAttackUnitState );
REGISTER_SAVELOAD_CLASS( 0x1919A2C7, CHelicopterLeaveState );
REGISTER_SAVELOAD_CLASS( 0x1919A2C8, CHelicopterFlyDeadState );
REGISTER_SAVELOAD_CLASS( 0x1919A2C9, CHelicopterUnloadState );

CPtr<CHelicopterStatesFactory> CHelicopterStatesFactory::pFactory = 0;

extern CGroupLogic theGroupLogic;
extern CWeather theWeather;
extern CEventUpdater updater;
extern CShellsStore theShellsStore;
extern NTimer::STime curTime;

// The same interval separates soldiers and the last/first soldiers of successive squads.
static const NTimer::STime HELICOPTER_DROP_INTERVAL = 2000;

IStatesFactory* CHelicopterStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CHelicopterStatesFactory();
	return pFactory;
}

bool CHelicopterStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand cmdType = pCommand->ToUnitCmd().nCmdType;
	return
		cmdType == ACTION_COMMAND_MOVE_TO ||
		cmdType == ACTION_COMMAND_SWARM_TO ||
		cmdType == ACTION_COMMAND_ROTATE_TO ||
		cmdType == ACTION_COMMAND_ROTATE_TO_DIR ||
		cmdType == ACTION_COMMAND_STAND_GROUND ||
		cmdType == ACTION_COMMAND_GUARD ||
		cmdType == ACTION_COMMAND_PATROL ||
		cmdType == ACTION_COMMAND_UNLOAD ||
		cmdType == ACTION_COMMAND_ATTACK_UNIT ||
		cmdType == ACTION_COMMAND_SWARM_ATTACK_UNIT ||
		cmdType == ACTION_MOVE_PLANE_LEAVE ||
		cmdType == ACTION_MOVE_FLY_DEAD ||
		cmdType == ACTION_COMMAND_DISAPPEAR ||
		cmdType == ACTION_COMMAND_STOP_THIS_ACTION ||
		cmdType == ACTION_COMMAND_DIE;
}

IUnitState* CHelicopterStatesFactory::ProduceState( CQueueUnit *pObj, CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CHelicopter*>( pObj ) != 0, "Wrong unit type" );
	CHelicopter *pHelicopter = checked_cast<CHelicopter*>( pObj );
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();

	switch ( cmd.nCmdType )
	{
	case ACTION_COMMAND_MOVE_TO:
		return new CHelicopterMoveState( pHelicopter, pCommand->GetMoveTarget( pHelicopter ), false );
	case ACTION_COMMAND_SWARM_TO:
		return new CHelicopterMoveState( pHelicopter, pCommand->GetMoveTarget( pHelicopter ), true );
	case ACTION_COMMAND_ROTATE_TO:
		return new CHelicopterRotateState( pHelicopter, cmd.vPos );
	case ACTION_COMMAND_ROTATE_TO_DIR:
		{
			CVec2 vDir = cmd.vPos;
			Normalize( &vDir );
			return new CHelicopterRotateState( pHelicopter, pHelicopter->GetCenterPlain() + vDir );
		}
	case ACTION_COMMAND_STAND_GROUND:
		pHelicopter->Stop();
		pHelicopter->SetBehaviourMoving( SBehaviour::EMHoldPos );
		return new CHelicopterRestState( pHelicopter );
	case ACTION_COMMAND_GUARD:
		pHelicopter->Stop();
		return new CHelicopterRotateState( pHelicopter,
			pHelicopter->GetCenterPlain() + GetVectorByDirection( WORD( cmd.fNumber ) ) );
	case ACTION_COMMAND_PATROL:
		{
			CVec2 vTarget( cmd.vPos );
			return CCommonPatrolState::Instance( pHelicopter, vTarget );
		}
	case ACTION_COMMAND_SWARM_ATTACK_UNIT:
	case ACTION_COMMAND_ATTACK_UNIT:
		{
			CObjectBase *pObj = GetObjectByCmd( cmd );
			if ( IsValid( pObj ) )
			{
				CONVERT_OBJECT( CAIUnit, pTarget, pObj, "Wrong unit to attack" );
				if ( pTarget->IsAlive() )
					return new CHelicopterAttackUnitState( pHelicopter, pTarget, cmd.nCmdType == ACTION_COMMAND_SWARM_ATTACK_UNIT );
			}
			pHelicopter->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, !pCommand->IsFromAI() );
			return 0;
		}
	case ACTION_COMMAND_UNLOAD:
		{
			// Passenger slots specify a squad and no destination; ground clicks unload all.
			CFormation *pSquad = dynamic_cast<CFormation*>( GetObjectByCmd( cmd ) );
			if ( cmd.nObjectID != 0 && !IsValidObj( pSquad ) )
			{
				pHelicopter->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, !pCommand->IsFromAI() );
				return 0;
			}
			const CVec2 vTarget = int( cmd.fNumber ) == ALP_POSITION_INVALID ?
				pHelicopter->GetCenterPlain() : pCommand->GetMoveTarget( pHelicopter );
			return new CHelicopterUnloadState( pHelicopter, vTarget, pSquad );
		}
	case ACTION_MOVE_PLANE_LEAVE:
		return new CHelicopterLeaveState( pHelicopter, pHelicopter->GetLeavePoint(), false );
	case ACTION_MOVE_FLY_DEAD:
		return new CHelicopterFlyDeadState( pHelicopter );
	case ACTION_COMMAND_DISAPPEAR:
		pHelicopter->Disappear();
		return 0;
	case ACTION_COMMAND_STOP_THIS_ACTION:
		pHelicopter->Stop();
		return new CHelicopterRestState( pHelicopter );
	case ACTION_COMMAND_DIE:
		NI_ASSERT( false, "Command to die in the queue" );
		return 0;
	}

	NI_ASSERT( false, "Wrong helicopter command" );
	return 0;
}

IUnitState* CHelicopterStatesFactory::ProduceRestState( CQueueUnit *pUnit )
{
	NI_ASSERT( dynamic_cast<CHelicopter*>( pUnit ) != 0, "Wrong unit type" );
	return new CHelicopterRestState( checked_cast<CHelicopter*>( pUnit ) );
}

void CHelicopterBaseState::Finish()
{
	if ( pHelicopter )
	{
		OnFinish();
		pHelicopter->SetCommandFinished();
	}
}

bool CHelicopterBaseState::ShouldLeaveMap() const
{
	return pHelicopter && ( pHelicopter->GetFuel() <= 0.0f ||
		( theWeather.IsActive() && pHelicopter->GetUnitAbilityDesc( NDb::ABILITY_MASTER_PILOT ) == 0 ) );
}

ETryStateInterruptResult CHelicopterBaseState::TryInterruptState( CAICommand *pCommand )
{
	if ( pHelicopter )
	{
		pHelicopter->Stop();
		pHelicopter->SetCommandFinished();
	}
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CHelicopterBaseState::GetPurposePoint() const
{
	return pHelicopter ? pHelicopter->GetCenterPlain() : CVec2( -1.0f, -1.0f );
}

void CHelicopterRestState::Segment()
{
	if ( ShouldLeaveMap() )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_PLANE_LEAVE ), pHelicopter, false );
		return;
	}

	pHelicopter->Stop();
	pHelicopter->DecFuel( true );
	// Standing helicopters still scan their immediate weapons/sight range, but do not chase unless a target is found.
	pHelicopter->AnalyzeTargetScanWithoutMoving( 0, false, false );
}

CHelicopterMoveState::CHelicopterMoveState( CHelicopter *pUnit, const CVec2 &_vTarget, const bool _bScanTargets )
: CHelicopterBaseState( pUnit ), vTarget( _vTarget ), bScanTargets( _bScanTargets )
{
	pHelicopter->BeginMoveTo( vTarget );
	// A swarm order must inspect its route immediately instead of inheriting an old scan cooldown.
	if ( bScanTargets )
		pHelicopter->ResetTargetScan();
}

void CHelicopterMoveState::Segment()
{
	if ( ShouldLeaveMap() )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_PLANE_LEAVE ), pHelicopter, false );
		return;
	}

	pHelicopter->BeginMoveTo( vTarget );
	pHelicopter->DecFuel( false );

	if ( bScanTargets )
		pHelicopter->AnalyzeTargetScan( 0, false, false );

	// Swarming helicopters must settle on their individual formation slots; the broad
	// movement tolerance makes several nearby slots look like the same destination.
	const float fArrivalRadius = bScanTargets ? 0.25f * SConsts::TILE_SIZE : 2.0f * SConsts::TILE_SIZE;
	if ( pHelicopter->IsNearTarget( vTarget, fArrivalRadius ) )
	{
		pHelicopter->BeginHover();
		Finish();
	}
}

CHelicopterUnloadState::CHelicopterUnloadState( CHelicopter *pUnit, const CVec2 &_vTarget, CFormation *_pUnload )
: CHelicopterBaseState( pUnit ), vTarget( _vTarget ), pUnload( _pUnload ),
	bUnloadOneSquad( _pUnload != 0 ), timeNextDrop( 0 ), nNextPassenger( 0 ), bFinishAfterSquad( false )
{
}

bool CHelicopterUnloadState::IsPassengerAboard( CSoldier *pSoldier ) const
{
	return IsValidObj( pSoldier ) && pSoldier->IsAlive() && pSoldier->IsInTransport() &&
		pSoldier->GetTransportUnit() == pHelicopter;
}

CFormation* CHelicopterUnloadState::GetNextSquad() const
{
	// Search this helicopter's manifest, never another transport's members of a squad.
	for ( int i = 0; i < pHelicopter->GetNPassengers(); ++i )
	{
		CSoldier *pSoldier = pHelicopter->GetPassenger( i );
		if ( IsPassengerAboard( pSoldier ) )
		{
			CFormation *pSquad = pSoldier->GetFormation();
			if ( IsValidObj( pSquad ) && ( !bUnloadOneSquad || pSquad == pUnload ) )
				return pSquad;
		}
	}
	return 0;
}

bool CHelicopterUnloadState::FindDropPoint( const CVec2 &vPreferred, CVec3 *pDropPoint ) const
{
	STerrainModeSetter terrainMode( ELM_ALL, GetTerrain() );
	const SVector centerTile = AICellsTiles::GetTile( vPreferred );
	const float fDropZ = pHelicopter->GetCenter().z;
	auto TryPoint = [&]( const CVec2 &vPoint )
	{
		const SVector tile = AICellsTiles::GetTile( vPoint );
		// The parachute path divides by its fall time, so require positive altitude too.
		if ( vPoint.x < 0.0f || vPoint.y < 0.0f ||
			!GetAIMap()->IsTileInside( tile ) || GetTerrain()->IsLocked( tile, EAC_HUMAN ) ||
			fDropZ - GetHeights()->GetZ( vPoint ) < 1.0f )
			return false;
		*pDropPoint = CVec3( vPoint, fDropZ );
		return true;
	};
	if ( TryPoint( vPreferred ) )
		return true;
	// Match the parachute path's bounded search, using a fixed order for multiplayer/replays.
	for ( int x = centerTile.x - SConsts::PARADROP_SPRED; x < centerTile.x + SConsts::PARADROP_SPRED; ++x )
		for ( int y = centerTile.y - SConsts::PARADROP_SPRED; y < centerTile.y + SConsts::PARADROP_SPRED; ++y )
			if ( TryPoint( AICellsTiles::GetPointByTile( SVector( x, y ) ) ) )
				return true;
	return false;
}

bool CHelicopterUnloadState::PrepareSquad( CFormation *pSquad )
{
	// Validate the complete squad only before its first soldier jumps.
	for ( int i = 0; i < pSquad->Size(); ++i )
	{
		CSoldier *pSoldier = (*pSquad)[i];
		if ( IsValidObj( pSoldier ) && pSoldier->IsAlive() && !IsPassengerAboard( pSoldier ) )
			return false;
	}

	// One synchronized random offset per squad; save its planned positions so loading
	// mid-drop neither rerolls the squad nor consumes additional random numbers.
	CVec2 vSquadOffset;
	RandUniformlyInCircle( 4.0f * SConsts::TILE_SIZE, &vSquadOffset );
	std::vector<CPtr<CSoldier> > passengers;
	std::vector<CVec3> positions;
	for ( int i = 0; i < pHelicopter->GetNPassengers(); ++i )
	{
		CSoldier *pSoldier = pHelicopter->GetPassenger( i );
		if ( !IsPassengerAboard( pSoldier ) || pSoldier->GetFormation() != pSquad )
			continue;

		const int nPassenger = passengers.size();
		const CVec2 vOffset = GetVectorByDirection( WORD( pHelicopter->GetFrontDirection() + nPassenger * 8192 ) ) *
			( ( 1 + nPassenger / 8 ) * SConsts::TILE_SIZE );
		CVec3 vDropPoint;
		if ( !FindDropPoint( pHelicopter->GetCenterPlain() + vSquadOffset + vOffset, &vDropPoint ) )
			return false;
		passengers.push_back( pSoldier );
		positions.push_back( vDropPoint );
	}
	if ( passengers.empty() )
		return false;

	dropPassengers.swap( passengers );
	dropPoints.swap( positions );
	nNextPassenger = 0;
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_PARACHUTE ), pSquad, false );
	return true;
}

void CHelicopterUnloadState::Segment()
{
	if ( ShouldLeaveMap() )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_PLANE_LEAVE ), pHelicopter, false );
		return;
	}
	if ( dropPassengers.empty() )
	{
		CFormation *pSquad = GetNextSquad();
		if ( !pSquad || vTarget.x < 0.0f || vTarget.y < 0.0f ||
			!GetAIMap()->IsTileInside( AICellsTiles::GetTile( vTarget ) ) )
		{
			pHelicopter->Stop();
			pHelicopter->SendAcknowledgement( ACK_NEGATIVE );
			Finish();
			return;
		}
		if ( !pHelicopter->IsNearTarget( vTarget, 0.25f * SConsts::TILE_SIZE ) )
		{
			pHelicopter->BeginMoveTo( vTarget );
			pHelicopter->DecFuel( false );
			return;
		}
	}
	pHelicopter->Stop();
	pHelicopter->DecFuel( true );
	if ( curTime < timeNextDrop )
		return;

	if ( dropPassengers.empty() && !PrepareSquad( GetNextSquad() ) )
	{
		pHelicopter->SendAcknowledgement( ACK_NEGATIVE );
		Finish();
		return;
	}

	// A passenger can die while waiting; skip missing members without releasing a
	// different squad early or letting a stale manifest reference touch another unit.
	while ( nNextPassenger < dropPassengers.size() && !IsPassengerAboard( dropPassengers[nNextPassenger] ) )
		++nNextPassenger;
	if ( nNextPassenger < dropPassengers.size() )
	{
		CVec3 vDropPoint;
		const CVec3 &vPlannedPoint = dropPoints[nNextPassenger];
		if ( !FindDropPoint( CVec2( vPlannedPoint.x, vPlannedPoint.y ), &vDropPoint ) )
		{
			// Ground units may block a previously safe landing spot during the sequence.
			// Keep the remaining soldiers aboard and retry without abandoning their squad.
			timeNextDrop = curTime + HELICOPTER_DROP_INTERVAL;
			return;
		}

		CSoldier *pSoldier = dropPassengers[nNextPassenger];
		pSoldier->SetFree();
		pSoldier->SetCenter( vDropPoint, false );
		pSoldier->SetSelectable( false, true );
		pHelicopter->DelPassenger( pSoldier );
		pSoldier->GetState()->TryInterruptState( 0 );
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_PARACHUTE, pHelicopter->GetUniqueID() ), pSoldier, false );
		pSoldier->SetCenter( vDropPoint );
		++nNextPassenger;
		timeNextDrop = curTime + HELICOPTER_DROP_INTERVAL;
	}
	if ( nNextPassenger == dropPassengers.size() )
	{
		dropPassengers.clear();
		dropPoints.clear();
		nNextPassenger = 0;
		if ( bUnloadOneSquad || bFinishAfterSquad || !GetNextSquad() )
			Finish();
	}
}

ETryStateInterruptResult CHelicopterUnloadState::TryInterruptState( CAICommand *pCommand )
{
	// Death and forced departure must still run immediately. Ordinary orders, including
	// Stop (passed as a null command), wait for the current squad's final passenger.
	const bool bForcedDeparture = pCommand &&
		( pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_FLY_DEAD ||
			pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_DISAPPEAR ||
			pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_PLANE_LEAVE );
	if ( !bForcedDeparture && pHelicopter && pHelicopter->IsAlive() && !dropPassengers.empty() )
	{
		bFinishAfterSquad = true;
		return TSIR_YES_WAIT;
	}
	return CHelicopterBaseState::TryInterruptState( pCommand );
}

CHelicopterRotateState::CHelicopterRotateState( CHelicopter *pUnit, const CVec2 &_vTarget )
: CHelicopterBaseState( pUnit ), vTarget( _vTarget )
{
}

void CHelicopterRotateState::Segment()
{
	if ( ShouldLeaveMap() )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_PLANE_LEAVE ), pHelicopter, false );
		return;
	}

	pHelicopter->BeginHover();
	pHelicopter->SetAttackTilt( false );
	pHelicopter->AimAtPoint( vTarget );
	pHelicopter->DecFuel( true );

	if ( DirsDifference( pHelicopter->GetFrontDirection(), GetDirectionByVector( vTarget - pHelicopter->GetCenterPlain() ) ) < 256 )
		Finish();
}

CHelicopterAttackUnitState::CHelicopterAttackUnitState( CHelicopter *pUnit, CAIUnit *_pTarget, const bool _bSwarmAttack )
: CHelicopterBaseState( pUnit ), pTarget( _pTarget ), pGun( 0 ), bSwarmAttack( _bSwarmAttack ), bAirModifierApplied( false )
{
	RefreshGun();
}

// CHelicopterAttackUnitState::~CHelicopterAttackUnitState()
// {
// 	damageUpdater.UnsetDamageFromEnemy( pTarget );
// 	ApplyAirModifier( false );
// }

void CHelicopterAttackUnitState::OnFinish()
{
	StopAllGuns();
	damageUpdater.UnsetDamageFromEnemy( pTarget );
	ApplyAirModifier( false );
}

void CHelicopterAttackUnitState::ApplyAirModifier( const bool bApply )
{
	const SMechUnitRPGStats *pStats = pHelicopter ? checked_cast<const SMechUnitRPGStats*>( pHelicopter->GetStats() ) : 0;
	if ( !pStats || !pStats->pGAPAirAttackModifier )
		return;
	if ( bAirModifierApplied == bApply )
		return;

	pHelicopter->ApplyStatsModifier( pStats->pGAPAirAttackModifier, bApply );
	bAirModifierApplied = bApply;
}

bool CHelicopterAttackUnitState::IsGunCompatible( CBasicGun *pCheckGun ) const
{
	if ( !pCheckGun || !IsValidObj( pTarget ) )
		return false;

	const NDb::SWeaponRPGStats::SShell &shell = pCheckGun->GetShell();
	return
		pCheckGun->GetNAmmo() > 0 &&
		shell.eDamageType == NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH &&
		shell.etrajectory != NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB &&
		( pTarget->IsAviation() || ( !pCheckGun->GetGun().bTargetAAOnly && !shell.IsSAMTrajectory() ) ) &&
		( pCheckGun->CanBreakArmor( pTarget ) || pHelicopter->IsTargetingTrack() );
}

bool CHelicopterAttackUnitState::RefreshGun()
{
	if ( !IsValidObj( pTarget ) )
		return false;

	pHelicopter->ResetShootEstimator( pTarget, false, pHelicopter->GetForbiddenGuns() );
	pGun = pHelicopter->GetBestShootEstimatedGun();
	if ( IsGunCompatible( pGun ) )
		return true;
	pGun = 0;

	// The normal estimator treats an aviation unit as locked, so a gun can be rejected
	// merely because the helicopter has not yet moved into range or turned into its arc.
	const bool bCanPursue =
		pHelicopter->DoesExistRejectGunsReason( ACK_NOT_IN_FIRE_RANGE ) ||
		pHelicopter->DoesExistRejectGunsReason( ACK_NOT_IN_ATTACK_ANGLE );
	if ( !bCanPursue )
		return false;

	const DWORD dwForbiddenGuns = pHelicopter->GetForbiddenGuns();
	for ( int i = 0; i < pHelicopter->GetNGuns(); ++i )
	{
		CBasicGun *pCandidate = pHelicopter->GetGun( i );
		if ( ( dwForbiddenGuns & ( 1UL << i ) ) != 0 ||
			!IsGunCompatible( pCandidate ) )
			continue;

		// Keep this weapon selected while movement and body rotation make the shot possible.
		pGun = pCandidate;
		return true;
	}

	return false;
}

void CHelicopterAttackUnitState::StartAvailableGuns()
{
	const DWORD dwForbiddenGuns = pHelicopter->GetForbiddenGuns();
	for ( int i = 0; i < pHelicopter->GetNGuns(); ++i )
	{
		CBasicGun *pAvailableGun = pHelicopter->GetGun( i );
		if ( ( dwForbiddenGuns & ( 1UL << i ) ) != 0 ||
			!IsGunCompatible( pAvailableGun ) ||
			pAvailableGun->IsFiring() ||
			pAvailableGun->IsRelaxing() ||
			!pAvailableGun->CanShootToUnitWOMove( pTarget ) )
		{
			continue;
		}

		// Fixed weapons wait for body alignment. Turret weapons may start now and rotate independently.
		if ( pAvailableGun->IsOnTurret() || pAvailableGun->CanShootWOGunTurn( pTarget, 1 ) )
			pAvailableGun->StartEnemyBurst( pTarget, false );
	}
}

void CHelicopterAttackUnitState::StopAllGuns()
{
	if ( !pHelicopter )
		return;

	for ( int i = 0; i < pHelicopter->GetNGuns(); ++i )
		pHelicopter->GetGun( i )->StopFire();
}

void CHelicopterAttackUnitState::Segment()
{
	if ( ShouldLeaveMap() )
	{
		StopAllGuns();
		ApplyAirModifier( false );
		damageUpdater.UnsetDamageFromEnemy( pTarget );
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_PLANE_LEAVE ), pHelicopter, false );
		return;
	}

	if ( !IsValidObj( pTarget ) || !pTarget->IsAlive() )
	{
		ApplyAirModifier( false );
		damageUpdater.UnsetDamageFromEnemy( pTarget );
		Finish();
		return;
	}

	ApplyAirModifier( pTarget->IsAviation() );
	pHelicopter->AimAtUnit( pTarget, pGun );
	pHelicopter->SetAttackTilt( !pTarget->IsAviation() );
	pHelicopter->DecFuel( false );
	if ( pGun )
		damageUpdater.SetDamageToEnemy( pHelicopter, pTarget, pGun );
	else
		damageUpdater.UnsetDamageFromEnemy( pTarget );

	if ( bSwarmAttack )
		pHelicopter->AnalyzeTargetScan( pTarget, damageUpdater.IsDamageUpdated(), false );

	if ( ( !IsGunCompatible( pGun ) || !pGun->CanShootToUnitWOMove( pTarget ) ) && !RefreshGun() )
	{
		pHelicopter->SendAcknowledgement( pHelicopter->GetGunsRejectReason() );
		ApplyAirModifier( false );
		damageUpdater.UnsetDamageFromEnemy( pTarget );
		Finish();
		return;
	}

	if ( pGun && pGun->CanShootToUnitWOMove( pTarget ) )
	{
		pHelicopter->BeginHover();
	}
	else
	{
		pHelicopter->BeginMoveTo( pTarget->GetCenterPlain() );
		pHelicopter->AimAtUnit( pTarget, pGun );
		pHelicopter->SetAttackTilt( !pTarget->IsAviation() );
	}

	// As with planes, fire every compatible weapon that can currently engage,
	// including longer-ranged weapons while the helicopter is still approaching.
	StartAvailableGuns();
}

ETryStateInterruptResult CHelicopterAttackUnitState::TryInterruptState( CAICommand *pCommand )
{
	StopAllGuns();
	damageUpdater.UnsetDamageFromEnemy( pTarget );
	ApplyAirModifier( false );
	return CHelicopterBaseState::TryInterruptState( pCommand );
}

const CVec2 CHelicopterAttackUnitState::GetPurposePoint() const
{
	return IsValidObj( pTarget ) ? pTarget->GetCenterPlain() : CVec2( -1.0f, -1.0f );
}

CHelicopterLeaveState::CHelicopterLeaveState( CHelicopter *pUnit, const CVec2 &_vTarget, const bool _bScanTargets )
: CHelicopterBaseState( pUnit ), vTarget( _vTarget ), bScanTargets( _bScanTargets )
{
	// The target is serialized with the state so save/load and multiplayer keep the same exit route.
}

void CHelicopterLeaveState::Segment()
{
	pHelicopter->BeginMoveTo( GetPurposePoint() );
	pHelicopter->DecFuel( false );
	if ( pHelicopter->IsNearTarget( GetPurposePoint(), 2.0f * SConsts::TILE_SIZE ) )
		pHelicopter->Disappear();
}

CHelicopterFlyDeadState::CHelicopterFlyDeadState( CHelicopter *pUnit )
: CHelicopterBaseState( pUnit ), bStarted( false )
{
}

void CHelicopterFlyDeadState::Segment()
{
	if ( !bStarted )
	{
		pHelicopter->StartDeathSpiral();
		bStarted = true;
	}

	if ( pHelicopter->IsDeathSpiralFinished() )
	{
		// Match plane ground impact: stop the visible model and produce the standard aviation crash burst.
		updater.AddUpdate( 0, ACTION_NOTIFY_DEADPLANE, pHelicopter, 0 );
		const NDb::SAIGameConsts *pConsts = Singleton<IAILogic>()->GetAIConsts();
		if ( pConsts->pAviationGroundCrashExplosion )
		{
			const CVec2 vGroundPoint( pHelicopter->GetCenterPlain() );
			theShellsStore.AddShell
				( new CInvisShell( curTime, new CBurstExpl( 0, pConsts->pAviationGroundCrashExplosion,
				CVec3( vGroundPoint, GetHeights()->GetZ( vGroundPoint ) ), VNULL3, 0, false, 1, true ), 0 ) );
		}
		pHelicopter->Disappear();
	}
}
