#include "stdafx.h"

#include "HelicopterStates.h"
#include "Helicopter.h"
#include "AIClassesID.h"
#include "Commands.h"
#include "CommonStates.h"
#include "Guns.h"
#include "GroupLogic.h"
#include "LinkObject.h"
#include "Weather.h"
#include "DBAIConsts.h"
#include "NewUpdater.h"
#include "Shell.h"
#include "AILogicInternal.h"
#include "AIMap.h"
#include "Common_RTS_AI/StaticMapHeights.h"

REGISTER_SAVELOAD_CLASS( 0x1919A2C2, CHelicopterStatesFactory );
REGISTER_SAVELOAD_CLASS( 0x1919A2C3, CHelicopterRestState );
REGISTER_SAVELOAD_CLASS( 0x1919A2C4, CHelicopterMoveState );
REGISTER_SAVELOAD_CLASS( 0x1919A2C5, CHelicopterRotateState );
REGISTER_SAVELOAD_CLASS( 0x1919A2C6, CHelicopterAttackUnitState );
REGISTER_SAVELOAD_CLASS( 0x1919A2C7, CHelicopterLeaveState );
REGISTER_SAVELOAD_CLASS( 0x1919A2C8, CHelicopterFlyDeadState );

CPtr<CHelicopterStatesFactory> CHelicopterStatesFactory::pFactory = 0;

extern CGroupLogic theGroupLogic;
extern CWeather theWeather;
extern CEventUpdater updater;
extern CShellsStore theShellsStore;
extern NTimer::STime curTime;

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
		return new CHelicopterMoveState( pHelicopter, cmd.vPos, false );
	case ACTION_COMMAND_SWARM_TO:
		return new CHelicopterMoveState( pHelicopter, cmd.vPos, true );
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
	case ACTION_MOVE_PLANE_LEAVE:
		return new CHelicopterLeaveState( pHelicopter, VNULL2, false );
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

	if ( pHelicopter->IsNearTarget( vTarget, 2.0f * SConsts::TILE_SIZE ) )
	{
		pHelicopter->BeginHover();
		Finish();
	}
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
		( pTarget->IsAviation() || !pCheckGun->GetGun().bTargetAAOnly ) &&
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
: CHelicopterBaseState( pUnit )
{}

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
