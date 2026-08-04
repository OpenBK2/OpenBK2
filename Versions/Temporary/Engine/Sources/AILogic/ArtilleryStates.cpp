#include "stdafx.h"

#include "ArtilleryStates.h"
#include "TechnicsStates.h"
#include "Commands.h"
#include "Guns.h"
#include "Artillery.h"
#include "UnitsIterators2.h"
#include "GroupLogic.h"
#include "Formation.h"
#include "Soldier.h"
#include "Turret.h"
#include "Aviation.h"
#include "NewUpdater.h"
#include "ArtilleryPaths.h"
#include "ArtilleryBulletStorage.h"
#include "StaticObjectsIters.h"
#include "ExecutorContainer.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "TankStates.h"
#include "FeedBackSystem.h"
// for profiling
#include "TimeCounter.h"

REGISTER_SAVELOAD_CLASS( 0x1108D4D5, CArtilleryAttackState );
REGISTER_SAVELOAD_CLASS( 0x1108D4D6, CArtilleryAttackCommonStatObjState );
REGISTER_SAVELOAD_CLASS( 0x1108D4D7, CArtilleryRestState );
REGISTER_SAVELOAD_CLASS( 0x1108D4D8, CArtilleryAttackAviationState );
REGISTER_SAVELOAD_CLASS( 0x1108D4A2, CArtilleryInstallTransportState );
REGISTER_SAVELOAD_CLASS( 0x1108D4A4, CArtilleryUninstallTransportState );
REGISTER_SAVELOAD_CLASS( 0x1108D4A5, CArtilleryBeingTowedState );
REGISTER_SAVELOAD_CLASS( 0x1108D4A6, CArtilleryStatesFactory );
REGISTER_SAVELOAD_CLASS( 0x1108D4A7, CArtilleryMoveToState );
REGISTER_SAVELOAD_CLASS( 0x1108D4A8, CArtilleryTurnToPointState );
REGISTER_SAVELOAD_CLASS( 0x1108D4A9, CArtilleryBombardmentState );
REGISTER_SAVELOAD_CLASS( 0x1108D4AA, CArtilleryRangeAreaState );

extern CFeedBackSystem theFeedBackSystem;
extern CGroupLogic theGroupLogic;
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CStaticObjects theStatObjs;
extern CEventUpdater updater;
extern NAI::CTimeCounter timeCounter;
extern CExecutorContainer theExecutorContainer;

//*******************************************************************
//*										 CArtilleryStatesFactory											*
//*******************************************************************

CPtr<CArtilleryStatesFactory> CArtilleryStatesFactory::pFactory = 0;

IStatesFactory* CArtilleryStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CArtilleryStatesFactory();

	return pFactory;
}

bool CArtilleryStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		( cmdType == ACTION_COMMAND_DIE							||
			cmdType == ACTION_COMMAND_MOVE_TO					||
			cmdType == ACTION_COMMAND_REVERSE_TO				||
			cmdType == ACTION_COMMAND_ATTACK_UNIT			||
			cmdType == ACTION_COMMAND_ATTACK_OBJECT		||
			cmdType == ACTION_COMMAND_ROTATE_TO				||
			cmdType == ACTION_COMMAND_GUARD						||
			cmdType == ACTION_COMMAND_TRACK_TARGETING	||
			cmdType == ACTION_COMMAND_RANGE_AREA			||
			cmdType == ACTION_COMMAND_INSTALL					||
			cmdType == ACTION_COMMAND_UNINSTALL				||
			cmdType == ACTION_COMMAND_ART_BOMBARDMENT	||
			cmdType == ACTION_COMMAND_FIRE_ROCKETS		||
			cmdType == ACTION_COMMAND_USE_FLAMETHROWER ||
			cmdType == ACTION_COMMAND_DISAPPEAR				||
			cmdType == ACTION_MOVE_BEING_TOWED				||
			cmdType == ACTION_COMMAND_LEAVE						||
			cmdType == ACTION_MOVE_IDLE								||
			cmdType == ACTION_COMMAND_SWARM_ATTACK_UNIT ||
			cmdType == ACTION_COMMAND_CHANGE_SHELLTYPE ||
			cmdType == ACTION_MOVE_LEAVE_SELF_ENTRENCH	||
			cmdType == ACTION_MOVE_SELF_ENTRENCH || 
			cmdType == ACTION_COMMAND_SWARM_TO ||
			cmdType == ACTION_COMMAND_ENTRENCH_SELF ||
			cmdType == ACTION_COMMAND_ROTATE_TO_DIR ||
			cmdType == ACTION_COMMAND_FOLLOW ||
			cmdType == ACTION_COMMAND_FOLLOW_NOW ||
			cmdType == ACTION_COMMAND_STAND_GROUND ||
			cmdType == ACTION_COMMAND_MOVE_TO_GRID ||
			cmdType == ACTION_COMMAND_WAIT ||
			cmdType == ACTION_COMMAND_UNLOAD ||
			cmdType == ACTION_MOVE_ONBOARD_ATTACK_UNIT ||
			cmdType == ACTION_COMMAND_COUNTER_FIRE ||
			cmdType == ACTION_COMMAND_EXACT_SHOT ||
			cmdType == ACTION_COMMAND_HOLD_SECTOR ||
			cmdType == ACTION_COMMAND_SUPPORT_FIRE 
		);
}

IUnitState* CArtilleryStatesFactory::ProduceState( class CQueueUnit *pObj, CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CArtillery*>( pObj ) != 0, "Wrong unit type" );
	CArtillery *pArtillery = checked_cast<CArtillery*>( pObj );

	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();	
	IUnitState* pResult = 0;
	bool bSwarmAttack = false;
	
	switch ( cmd.nCmdType )
	{
	case ACTION_COMMAND_MECH_ENTER:
	case ACTION_MOVE_MECH_ENTER_NOW:
		{
			CObjectBase * pObj = GetObjectByCmd( cmd );
			if ( pObj )
			{
				CMilitaryCar * pTransport = checked_cast<CMilitaryCar*>( pObj );
				pResult = new CMechUnitInsideMechUnitState( pArtillery, pTransport, cmd.nCmdType == ACTION_MOVE_MECH_ENTER_NOW );
			}
		}
		break;
	case ACTION_COMMAND_WAIT:
		pResult = CArtilleryRestState::Instance( pArtillery, cmd.vPos, pArtillery->GetDirection(), cmd.fNumber );
		break;
	case ACTION_COMMAND_UNLOAD:
		{
			CObjectBase * pObj = GetObjectByCmd( cmd );
			CMechUnitInsideMechUnitState *pState ( checked_cast<CMechUnitInsideMechUnitState*>( pArtillery->GetState() ) );
			if ( pObj )
			{
				pState->Unload( pArtillery->GetCenterPlain() );
			}
		}

		break;
	case ACTION_MOVE_ONBOARD_ATTACK_UNIT:
		{
			CObjectBase * pObj = GetObjectByCmd( cmd );
			CMechUnitInsideMechUnitState *pState ( checked_cast<CMechUnitInsideMechUnitState*>( pArtillery->GetState() ) );
			if ( pObj )
			{
				CAIUnit * pEnemy = checked_cast<CMilitaryCar*>( pObj );
				pState->AttackTarget( pEnemy );
			}
			pResult = pState;
		}

		break;
	
	case ACTION_COMMAND_ENTRENCH_SELF:
		//break;
	case ACTION_MOVE_SELF_ENTRENCH:
		pResult = CMechUnitEntrenchSelfState::Instance( pArtillery );

		break;
	case ACTION_MOVE_LEAVE_SELF_ENTRENCH:
			pArtillery->ResetHoldSector();
			pResult = CTankPitLeaveState::Instance( pArtillery );

			break;
		case ACTION_COMMAND_CHANGE_SHELLTYPE:
			pArtillery->SetActiveShellType( static_cast<NDb::SWeaponRPGStats::SShell::EShellDamageType>( int(cmd.fNumber)) );

			break;
		case ACTION_COMMAND_TRACK_TARGETING:
			{
				const NDb::ESpecialAbilityParam action = NDb::ESpecialAbilityParam(int(cmd.fNumber));
				if ( action != PARAM_ABILITY_AUTOCAST_ON &&
					action != PARAM_ABILITY_AUTOCAST_OFF )
				{
					pArtillery->SetTrackTargeting( true );
					theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ATTACK_UNIT, cmd.nObjectID ), pArtillery );
				}
			}
			break;

		case ACTION_COMMAND_HOLD_SECTOR:
			switch ( cmd.nNumber )
			{	
			case 0:
				{
					SAIUnitCmd newCmd( ACTION_COMMAND_HOLD_SECTOR );
					newCmd.nNumber = 2;
					theGroupLogic.InsertUnitCommand( newCmd, pArtillery );
					theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ROTATE_TO, cmd.vPos ), pArtillery );
					if ( pArtillery->IsInTankPit() )
						theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pArtillery );
				}
				break;
			case 1:
				pArtillery->ResetHoldSector();
				break;
			case 2:
				pResult = CSoldierEnterHoldSectorState::Instance( pArtillery );
				break;
			default:
				NI_ASSERT( false, "Wrong param to ACTION_COMMAND_HOLD_SECTOR" );
				break;
			}
			break;
			/*
		case ACTION_COMMAND_LEAVE:
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_STOP_THIS_ACTION), pArtillery->GetCrew(), false );
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_MOVE_TO, cmd.vPos), pArtillery->GetCrew(), false );
			pArtillery->DelCrew();

			break;*/
		case ACTION_MOVE_IDLE:
			pResult = CSoldierIdleState::Instance( pArtillery );

			break;
		case ACTION_MOVE_BEING_TOWED:
			{
				CONVERT_OBJECT( CAITransportUnit, pTransport, GetObjectByCmd( cmd ), "Wrong unit to attach artillery" );
				pResult = CArtilleryBeingTowedState::Instance( pArtillery, pTransport );
			}

			break;
		case ACTION_COMMAND_DIE:
			NI_ASSERT( false, "Command to die in the queue" );

			break;
		case ACTION_COMMAND_MOVE_TO:
		case ACTION_COMMAND_REVERSE_TO:
			pArtillery->ResetHoldSector();
			if ( pArtillery->IsInTankPit() && !pCommand->IsFromAI() )// сначала выйти из TankPit, потом поехать куда послали
			{
				theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pArtillery );
				//theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, float(int(PARAM_ABILITY_OFF)), false ), pArtillery );
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pArtillery );
			}
			else
			{
				pArtillery->UnsetFollowState();				
				pResult = CArtilleryMoveToState::Instance( pArtillery, cmd.vPos, cmd.nCmdType == ACTION_COMMAND_REVERSE_TO );
			}

			break;
		case	ACTION_COMMAND_SWARM_ATTACK_UNIT:
			bSwarmAttack = true;
		case ACTION_COMMAND_ATTACK_UNIT:
			if ( IsValid( GetObjectByCmd( cmd ) ) )
			{
				if( !cmd.bFromAI )
					pArtillery->SetActiveShellType( NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH );

				CAIUnit *pTarget = 0;
				CCommonUnit *pUnit = dynamic_cast<CCommonUnit*>( GetObjectByCmd( cmd ) );
				if ( pUnit->IsFormation() )
				{
					CDynamicCast<CFormation> pFormation = pUnit;
					if ( pFormation && pFormation->Size() > 0 )
						pTarget = (*pFormation)[0];
				}
				else
					pTarget = dynamic_cast<CAIUnit*>( pUnit );

				//CONVERT_OBJECT( CAIUnit, pTarget, GetObjectByCmd( cmd ), "Wrong unit to attack" );
				if ( IsValid( pTarget ) && pTarget->IsAlive() )
				{
					// act like a tank
					if ( pArtillery->GetStats()->nUninstallRotate == 0 && pArtillery->GetStats()->nUninstallTransport == 0 )
					{
						if ( pTarget->GetStats()->IsInfantry() && checked_cast<CSoldier*>(pTarget)->IsInBuilding() )
							pResult = CSoldierAttackUnitInBuildingState::Instance( pArtillery, checked_cast<CSoldier*>(pTarget), cmd.fNumber == 0, bSwarmAttack );
						else
							pResult = CMechAttackUnitState::Instance( pArtillery, pTarget, cmd.fNumber == 0, bSwarmAttack );
					}
					else if ( pTarget->GetStats()->IsAviation() )
						pResult = CArtilleryAttackAviationState::Instance( pArtillery, checked_cast<CAviation*>( pTarget ) );
					else
						pResult = CArtilleryAttackState::Instance( pArtillery, pTarget, cmd.fNumber == 0, bSwarmAttack );
				}
			}
			
			break;
		case ACTION_COMMAND_SWARM_ATTACK_OBJECT:
			bSwarmAttack = true;
		case ACTION_COMMAND_ATTACK_OBJECT:
			{
				CONVERT_OBJECT( CStaticObject, pStaticObj, GetObjectByCmd( cmd ), "Wrong static object to attack" );

				if ( pArtillery->GetStats()->nUninstallRotate == 0 && pArtillery->GetStats()->nUninstallTransport == 0 )
				{
					pArtillery->ResetHoldSector();
					if ( pArtillery->IsInTankPit() && !pCommand->IsFromAI() )
					{
						theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pArtillery );
						theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pArtillery );
						//theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, float(int(PARAM_ABILITY_OFF)), false ), pArtillery );
						break;
					}
				}

				// attack the artillery
				if ( pStaticObj->GetObjectType() == ESOT_ARTILLERY_BULLET_STORAGE )
				{
					pCommand->ToUnitCmd().nCmdType = ACTION_COMMAND_ATTACK_UNIT;
					pCommand->ToUnitCmd().nObjectID = checked_cast<CArtilleryBulletStorage*>(pStaticObj)->GetOwner()->GetUniqueId();
					pCommand->ToUnitCmd().fNumber = 0;

					pResult = ProduceState( pObj, pCommand );
				}
				else if ( pArtillery->GetStats()->nUninstallRotate == 0 && pArtillery->GetStats()->nUninstallTransport == 0 )
					pResult = CSoldierAttackCommonStatObjState::Instance( pArtillery, pStaticObj, bSwarmAttack );
				else
					pResult = CArtilleryAttackCommonStatObjState::Instance( pArtillery, pStaticObj );
			}

			break;
		case ACTION_COMMAND_ROTATE_TO:
			pArtillery->ResetHoldSector();
			if ( pArtillery->IsInTankPit() && !pCommand->IsFromAI() && pArtillery->GetStats()->etype != RPG_TYPE_ART_AAGUN )// сначала выйти из TankPit, потом поехать куда послали
			{
				theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pArtillery );
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pArtillery );
				//theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, float(int(PARAM_ABILITY_OFF)), false ), pArtillery );
			}
			else
				pResult = CArtilleryTurnToPointState::Instance( pArtillery, cmd.vPos );

			break;
		case ACTION_COMMAND_ROTATE_TO_DIR:
			pArtillery->ResetHoldSector();
			if ( pArtillery->IsInTankPit() && !pCommand->IsFromAI() )// сначала выйти из TankPit, потом поехать куда послали
			{
				theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pArtillery );
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pArtillery );
				//theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, float(int(PARAM_ABILITY_OFF)), false ), pArtillery );
			}
			else
			{
				CVec2 vDir = cmd.vPos;
				Normalize( &vDir );
				pResult = CArtilleryTurnToPointState::Instance( pArtillery, pArtillery->GetCenterPlain() + vDir );
			}

			break;
		/*case ACTION_COMMAND_AMBUSH:
			pResult = CCommonAmbushState::Instance( pArtillery );

			break;*/
		case ACTION_COMMAND_INSTALL:
			pResult = CArtilleryInstallTransportState::Instance( pArtillery );

			break;
		case ACTION_COMMAND_UNINSTALL:
			pResult = CArtilleryUninstallTransportState::Instance( pArtillery );

			break;
		case ACTION_COMMAND_ART_BOMBARDMENT:
		case ACTION_COMMAND_FIRE_ROCKETS:
			if ( pArtillery->GetFirstArtilleryGun() != 0 )
			{
				// A count of one makes the existing bombardment state finish after one complete burst.
				const int nBurstCount = cmd.nCmdType == ACTION_COMMAND_FIRE_ROCKETS ? 1 : int( cmd.fNumber );
				pResult = CArtilleryBombardmentState::Instance( pArtillery, cmd.vPos, nBurstCount );
			}
			else
				pArtillery->SendAcknowledgement( pCommand, ACK_NEGATIVE, !pCommand->IsFromAI() );

			break;
		case ACTION_COMMAND_USE_FLAMETHROWER:
			// Unlike artillery bombardment, this path ignores every non-flame shell.
			pResult = CArtilleryBombardmentState::Instance( pArtillery, cmd.vPos, 1, true );

			break;
		case ACTION_COMMAND_RANGE_AREA:
			if ( pArtillery->GetFirstArtilleryGun() != 0 )
				pResult = CArtilleryRangeAreaState::Instance( pArtillery, cmd.vPos );
			else
				pArtillery->SendAcknowledgement( pCommand, ACK_NEGATIVE, !pCommand->IsFromAI() );

			break;
		case ACTION_COMMAND_GUARD:
			pResult = CArtilleryRestState::Instance( pArtillery, cmd.vPos, cmd.fNumber, -1 );

			break;
		case ACTION_COMMAND_SWARM_TO:
			// сначала выйти из TankPit, потом поехать куда послали			
			pArtillery->ResetHoldSector();
			if ( pArtillery->IsInTankPit() && !pCommand->IsFromAI() )
			{
				theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pArtillery );
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pArtillery );
				//theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, float(int(PARAM_ABILITY_OFF)), false ), pArtillery );
			}
			else if ( !pArtillery->CanMove() )
				pArtillery->SendAcknowledgement( pCommand, ACK_NEGATIVE, !pCommand->IsFromAI() );
			else 
				pResult = CCommonSwarmState::Instance( pArtillery, cmd.vPos, cmd.fNumber );
			
			break;
		case ACTION_COMMAND_FOLLOW:
			{
				CONVERT_OBJECT( CCommonUnit, pCommonUnit, GetObjectByCmd( cmd ), "Not common unit in follow command" );
				pArtillery->SetFollowState( pCommonUnit );
			}

			break;
		case ACTION_COMMAND_FOLLOW_NOW:
			{
				CONVERT_OBJECT( CCommonUnit, pCommonUnit, GetObjectByCmd( cmd ), "Not common unit in follow command" );
				pResult = CFollowState::Instance( pArtillery, pCommonUnit );
			}

			break;
		case ACTION_COMMAND_STAND_GROUND:
			pArtillery->Stop();
			pArtillery->UnsetFollowState();				
			pArtillery->SetBehaviourMoving( SBehaviour::EMHoldPos );

			break;
		case ACTION_COMMAND_MOVE_TO_GRID:
			pResult = CCommonMoveToGridState::Instance( pArtillery, cmd.vPos, GetVectorByDirection( cmd.fNumber ) );

			break;
		case ACTION_COMMAND_COUNTER_FIRE:
			{
				const NDb::ESpecialAbilityParam action = NDb::ESpecialAbilityParam( int( cmd.fNumber ) );
				if ( action == PARAM_ABILITY_AUTOCAST_ON || 
					action == PARAM_ABILITY_AUTOCAST_OFF )
					break;

				SExecutorEventParam par( EID_ABILITY_SET_TARGET, 0, pArtillery->GetUniqueId() );
				CExecutorEventSpecialAbilitySetTarget event( par, NDb::ABILITY_ANTIATRILLERY_FIRE, cmd.vPos );
				theExecutorContainer.RaiseEvent( event );
			}
			break;
		case ACTION_COMMAND_SUPPORT_FIRE:
			{
				CONVERT_OBJECT( CAIUnit, pAIUnit, GetObjectByCmd( cmd ), "Not common unit in support fire command" );
				pResult = CMechUnitSupportFireState::Instance( pArtillery, pAIUnit );

			}
			break;
		case ACTION_COMMAND_EXACT_SHOT:
			{
				const NDb::ESpecialAbilityParam action = NDb::ESpecialAbilityParam(int(cmd.fNumber));
				if ( action != PARAM_ABILITY_AUTOCAST_ON &&
					action != PARAM_ABILITY_AUTOCAST_OFF )
				{
					// Set IgnoreAABB on
					pArtillery->SetIgnoreAABBCoeff( true );

					theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ATTACK_UNIT, cmd.nObjectID ), pArtillery );
				}
			}
			break;
default:
			NI_ASSERT( false, "Wrong command" );
	}

	return pResult;
}

IUnitState* CArtilleryStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	NI_ASSERT( dynamic_cast<CArtillery*>( pUnit ) != 0, "Wrong unit type" );	
	CArtillery * pArt = checked_cast<CArtillery*>( pUnit );
	return CArtilleryRestState::Instance( pArt, CVec2( -1, -1 ), 0, -1 );
}

//*******************************************************************
//*										  CArtilleryMoveToState												*
//*******************************************************************

IUnitState* CArtilleryMoveToState::Instance( CArtillery *pArtillery, const CVec2 &point, const bool bForceReverse )
{
	return new CArtilleryMoveToState( pArtillery, point, bForceReverse );
}

CArtilleryMoveToState::CArtilleryMoveToState( CArtillery *_pArtillery, const CVec2 &_point, const bool _bForceReverse )
: pArtillery( _pArtillery ), startTime( curTime ), eState( EAMTS_WAIT_FOR_PATH ), bToFinish( false ),
	bForceReverse( _bForceReverse )
{
	pArtillery->UnlockTiles();
	pArtillery->FixUnlocking();
}

void CArtilleryMoveToState::SetReversePathMode( const bool bEnable )
{
	if ( !pArtillery )
		return;

	ISmoothPath *pDefaultPath = pArtillery->GetDefaultPath();
	ISmoothPath *pSmoothPath = pArtillery->GetSmoothPath();
	if ( pDefaultPath )
		pDefaultPath->SetForceGoBackward( bEnable );
	if ( pSmoothPath && pSmoothPath != pDefaultPath )
		pSmoothPath->SetForceGoBackward( bEnable );
}

void CArtilleryMoveToState::Segment()
{
	if ( pArtillery->MustHaveCrewToOperate() && pArtillery->HasSlaveTransport() )
	{
		const CVec2 &vPos = pArtillery->GetCurCmd()->ToUnitCmd().vPos;
		if ( pArtillery->HasSlaveTransport() && pArtillery->GetSlaveTransport()->GetState()->GetName() == EUSN_REST )
		{
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_TAKE_ARTILLERY,pArtillery->GetUniqueId()), pArtillery->GetSlaveTransport(), false );
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_DEPLOY_ARTILLERY,vPos ), pArtillery->GetSlaveTransport(), true );
		}
	}
	else
	{
		switch ( eState )
		{
		case EAMTS_WAIT_FOR_PATH:
			if ( curTime - startTime >= TIME_OF_WAITING )
			{
				pStaticPath = pArtillery->GetCurCmd()->CreateStaticPath( pArtillery );
				if ( pStaticPath )
				{
					pArtillery->UnfixUnlocking();				
					if ( pArtillery->IsInstalled() )
					{
						pArtillery->LockTiles();			
						pArtillery->InstallAction( ACTION_NOTIFY_UNINSTALL_MOVE );
						eState = EAMTS_UNINSTALLING;
					}
					else
						eState = EAMTS_START_MOVING;
				}
				else
				{
					pArtillery->SetCommandFinished();
					pArtillery->SendAcknowledgement( ACK_NEGATIVE, true );
				}
				
			}
			break;
		case EAMTS_UNINSTALLING:
			if ( pArtillery->IsUninstalled() )
			{
				if ( bToFinish )
				{
					SetReversePathMode( false );
					pArtillery->SetCommandFinished();
				}
				else
					eState = EAMTS_START_MOVING;
			}

			break;
		case EAMTS_START_MOVING:
			{
				if ( !pStaticPath )
				{
					SetReversePathMode( false );
					pArtillery->SetCommandFinished();
				}
				else
				{
					// Set the policy before and after Init so rebuilt smooth paths keep forced reverse.
					SetReversePathMode( bForceReverse );
					pArtillery->SendAlongPath( pStaticPath, pArtillery->GetGroupShift(), !bForceReverse );
					SetReversePathMode( bForceReverse );
					eState = EAMTS_MOVING;
				}
			}

			break;
		case EAMTS_MOVING:
			if ( bForceReverse )
				SetReversePathMode( true );
			if ( pArtillery->IsOperable() )
			{
				if ( pArtillery->IsIdle() )
				{
					SetReversePathMode( false );
					pArtillery->SetCommandFinished();
				}
			}
			else if ( !pArtillery->HasServeCrew() )
			{
				SetReversePathMode( false );
				pArtillery->SetCommandFinished();
			}

			break;
		}
	}
}

ETryStateInterruptResult CArtilleryMoveToState::TryInterruptState(class CAICommand *pCommand)
{ 
	SetReversePathMode( false );
	pArtillery->UnfixUnlocking();
	if ( pArtillery->MustHaveCrewToOperate() )
	{
		if ( eState == EAMTS_UNINSTALLING )
		{
			bToFinish = true;
			return TSIR_YES_WAIT;
		}
	}
	pArtillery->SetCommandFinished(); 
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										CArtilleryTurnToPointState										*
//*******************************************************************

IUnitState* CArtilleryTurnToPointState::Instance( CArtillery *pArtillery, const CVec2 &targCenter )
{
	return new CArtilleryTurnToPointState( pArtillery, targCenter );
}

CArtilleryTurnToPointState::CArtilleryTurnToPointState( CArtillery *_pArtillery, const CVec2 &_targCenter )
: CStatusUpdatesHelper( EUS_ROTATE, _pArtillery ), pArtillery( _pArtillery ), targCenter( _targCenter ), eState( EATRS_ESTIMATING ), lastCheck( 0 ), timeStart( curTime )
{
	pArtillery->Stop();
}

void CArtilleryTurnToPointState::Segment()
{
	if ( pArtillery->IsOperable() )
	{
		switch ( eState )
		{
			case EATRS_ESTIMATING:
				if ( pArtillery->GetStats()->etype == RPG_TYPE_ART_AAGUN && pArtillery->IsInstalled() )
				{
					for ( int i = 0; i < pArtillery->GetNTurrets(); ++i )
						pArtillery->GetTurret( i )->TurnHor( GetDirectionByVector( targCenter - pArtillery->GetCenterPlain() ) - pArtillery->GetFrontDirection() );

					eState = EATPS_TURNING;
				}
				else
				{
					if ( !pArtillery->CheckTurn( 1.0f, targCenter - pArtillery->GetCenterPlain(), false, false ) )
					{
						pArtillery->SendAcknowledgement( ACK_NEGATIVE );
						pArtillery->SetCommandFinished();
					}
					else
					{
						eState = EATPS_UNINSTALLING;
						if ( pArtillery->IsInstalled() )
							pArtillery->InstallAction( ACTION_NOTIFY_UNINSTALL_ROTATE );
					}
				}

				break;
			case EATPS_UNINSTALLING:
				if ( pArtillery->IsUninstalled() )
				{
					lastCheck = curTime;
					eState = EATPS_TURNING;
				}
				else if ( pArtillery->IsInstalled() )
					pArtillery->InstallAction( ACTION_NOTIFY_UNINSTALL_ROTATE );

				break;
			case EATPS_TURNING:
				if ( pArtillery->IsInstalled() && pArtillery->GetStats()->etype == RPG_TYPE_ART_AAGUN )
				{
					int i = 0;
					while ( i < pArtillery->GetNTurrets() && pArtillery->GetTurret( i )->IsFinished() )
						++i;

					if ( i >= pArtillery->GetNTurrets() )
						pArtillery->SetCommandFinished();
				}
				else if ( pArtillery->TurnToUnit( targCenter ) )
					pArtillery->SetCommandFinished();
				else
					lastCheck = curTime;

				break;
		}
		if ( eState != EATRS_ESTIMATING && curTime - timeStart > NGlobal::GetVar( "rotate_status_wait_time", 0 ) )
			InitStatus();
	}
	else if ( !pArtillery->HasServeCrew() )
		pArtillery->SetCommandFinished();
}

ETryStateInterruptResult CArtilleryTurnToPointState::TryInterruptState(class CAICommand *pCommand)
{ 
	if ( pArtillery->GetStats()->etype == RPG_TYPE_ART_AAGUN )
	{
		for ( int i = 0; i < pArtillery->GetNTurrets(); ++i )
			pArtillery->GetTurret( i )->StopTurning();
	}
	
	pArtillery->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CArtilleryTurnToPointState::GetPurposePoint() const
{
	return pArtillery->GetCenterPlain();
}

//*******************************************************************
//*										CArtilleryBombardmentState										*
//*******************************************************************

IUnitState* CArtilleryBombardmentState::Instance( CAIUnit *pUnit, const CVec2 &point, const int nShotCount, const bool bFlamethrowerOnly )
{
	return new CArtilleryBombardmentState( pUnit, point, nShotCount, bFlamethrowerOnly );
}

CArtilleryBombardmentState::CArtilleryBombardmentState( CAIUnit *_pUnit, const CVec2 &_point, const int _nShotCount, const bool _bFlamethrowerOnly )
: CStatusUpdatesHelper( EUS_SUPPRESSIVE_FIRE, _pUnit ), pUnit( _pUnit ), point( _point ), bStop( false ), eState( EABS_START ),
	bSaidNoAmmo( false ), nShotCount( _nShotCount ), bFlamethrowerOnly( _bFlamethrowerOnly ), bBurstStarted( false )
{
	if ( nShotCount == 0 )
		nShotCount = -1;
}

bool CArtilleryBombardmentState::IsFlamethrowerGun( const CBasicGun *pGun ) const
{
	return pGun && pGun->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_FLAME_THROWER;
}

CBasicGun* CArtilleryBombardmentState::ChoosePointFireGun( bool *pCanShootWOMove, bool *pNeedTurn ) const
{
	*pCanShootWOMove = false;
	*pNeedTurn = false;

	if ( !bFlamethrowerOnly )
	{
		CBasicGun *pGun = pUnit->GetFirstArtilleryGun();
		if ( !pGun )
			return 0;

		*pCanShootWOMove = pGun->CanShootToPointWOMove( point, 0.0f );
		if ( !*pCanShootWOMove && pGun->GetRejectReason() == ACK_NOT_IN_ATTACK_ANGLE && pUnit->CanRotate() )
		{
			*pCanShootWOMove = true;
			*pNeedTurn = true;
		}
		return pGun;
	}

	CBasicGun *pFirstFlamethrower = 0;
	CBasicGun *pTurnCandidate = 0;
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
	{
		CBasicGun *pGun = pUnit->GetGun( i );
		if ( !IsFlamethrowerGun( pGun ) )
			continue;

		if ( !pFirstFlamethrower )
			pFirstFlamethrower = pGun;
		if ( pGun->CanShootToPointWOMove( point, 0.0f ) )
		{
			*pCanShootWOMove = true;
			return pGun;
		}
		if ( !pTurnCandidate && pGun->GetRejectReason() == ACK_NOT_IN_ATTACK_ANGLE && pUnit->CanRotate() )
			pTurnCandidate = pGun;
	}

	if ( pTurnCandidate )
	{
		*pCanShootWOMove = true;
		*pNeedTurn = true;
		return pTurnCandidate;
	}
	return pFirstFlamethrower;
}

bool CArtilleryBombardmentState::StartFlamethrowerBurst()
{
	bool bStarted = false;
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
	{
		CBasicGun *pGun = pUnit->GetGun( i );
		if ( IsFlamethrowerGun( pGun ) && pGun->CanShootToPointWOMove( point, 0.0f ) )
		{
			// Start each flame shell directly so linked cannon or machine-gun shells cannot join the order.
			pGun->StartPointBurst( point, false, false );
			bStarted = true;
		}
	}
	return bStarted;
}

bool CArtilleryBombardmentState::IsAnyFlamethrowerGunFiring() const
{
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
	{
		CBasicGun *pGun = pUnit->GetGun( i );
		if ( IsFlamethrowerGun( pGun ) && pGun->IsFiring() )
			return true;
	}
	return false;
}

bool CArtilleryBombardmentState::IsAnyFlamethrowerGunBursting() const
{
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
	{
		CBasicGun *pGun = pUnit->GetGun( i );
		if ( IsFlamethrowerGun( pGun ) && pGun->IsBursting() )
			return true;
	}
	return false;
}

void CArtilleryBombardmentState::StopFlamethrowerGuns()
{
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
	{
		CBasicGun *pGun = pUnit->GetGun( i );
		if ( IsFlamethrowerGun( pGun ) )
			pGun->StopFire();
	}
}

void CArtilleryBombardmentState::Segment()
{
	if ( pUnit->IsOperable() )
	{
		switch ( eState )
		{
			case EABS_START:
				{
					pUnit->Stop();

					bool bCanShootWOMove = false;
					bool bNeedTurn = false;
					CBasicGun *pPointFireGun = ChoosePointFireGun( &bCanShootWOMove, &bNeedTurn );
					if ( !pPointFireGun )
					{
						pUnit->SendAcknowledgement( ACK_NEGATIVE );
						pUnit->SetCommandFinished();
						break;
					}
					
					if ( bCanShootWOMove )
					{
						if ( !bNeedTurn )
							eState = EABS_FIRING;
						else
						{
							if ( pUnit->IsInTankPit() )
							{
								theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pUnit );
								return;
							}
							eState = EABS_TURNING;
							SetStatus( EUS_ROTATE );
						}

						CExistingObject *pPit = pUnit->GetTankPit();
						if ( pPit )
							pPit->UnlockTiles();

						if ( bNeedTurn && !pUnit->CheckTurn( 1.0f, point - pUnit->GetCenterPlain(), false, false ) )
						{
							pUnit->SendAcknowledgement( ACK_NEGATIVE );
							pUnit->SetCommandFinished();
						}
						else
							InitStatus();
						if ( pPit )
							pPit->LockTiles();
					}
					else
					{
						const EUnitAckType eReject = pPointFireGun->GetRejectReason();
						pUnit->SendAcknowledgement( eReject );
						if ( eReject == ACK_NO_AMMO && pUnit->GetPlayer() == theDipl.GetMyNumber() )
							theFeedBackSystem.AddFeedbackAndForget( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), EFB_NO_AMMO, -1 );
						pUnit->SetCommandFinished();
					}
				}

				break;
			case EABS_TURNING:
				if ( pUnit->TurnToTarget( point ) )
				{
					eState = EABS_FIRING;
					SetStatus( EUS_SUPPRESSIVE_FIRE );
				}

				break;
			case EABS_FIRING:
				if ( bFlamethrowerOnly )
				{
					if ( bStop && !IsAnyFlamethrowerGunBursting() )
						TryInterruptState( 0 );
					else if ( pUnit->CanShoot() )
					{
						if ( !bBurstStarted )
						{
							bBurstStarted = true;
							if ( !StartFlamethrowerBurst() )
							{
								bool bCanShootWOMove = false;
								bool bNeedTurn = false;
								CBasicGun *pGun = ChoosePointFireGun( &bCanShootWOMove, &bNeedTurn );
								pUnit->SendAcknowledgement( pGun ? pGun->GetRejectReason() : ACK_NEGATIVE );
								pUnit->SetCommandFinished();
							}
						}
						else if ( !IsAnyFlamethrowerGunFiring() )
							pUnit->SetCommandFinished();
					}
					break;
				}
				if ( 
						 !pUnit->GetFirstArtilleryGun()->IsBursting() && 
						 ( bStop || !pUnit->GetFirstArtilleryGun()->CanShootToPointWOMove( point, 0 ) )
					 )
				 pUnit->SetCommandFinished();
				else if ( pUnit->CanShoot() )
				{
					if ( !pUnit->GetFirstArtilleryGun()->IsFiring() )
					{
						if ( nShotCount == 0 )
							TryInterruptState( 0 );
						else
						{
							pUnit->GetFirstArtilleryGun()->StartPointBurst( point, false );
							bSaidNoAmmo = false;
							--nShotCount;
						}
					}
					else if ( pUnit->GetFirstArtilleryGun()->GetNAmmo() == 0 )
					{
						if ( !bSaidNoAmmo )
						{
							pUnit->SendAcknowledgement( ACK_NO_AMMO );
							if ( pUnit->GetPlayer() == theDipl.GetMyNumber() )
								theFeedBackSystem.AddFeedbackAndForget( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), EFB_NO_AMMO, -1 );
							bSaidNoAmmo = true;
							if ( NGlobal::GetVar( "stop_on_no_ammo", 0 ) )
								pUnit->SetCommandFinished();
						}
					}
				}

				break;
			default: NI_ASSERT( false, "Wrong state" );
		}
	}
}

ETryStateInterruptResult CArtilleryBombardmentState::TryInterruptState( CAICommand *pCommand )
{
	if ( bFlamethrowerOnly )
	{
		if ( pCommand == 0 || !IsAnyFlamethrowerGunBursting() )
		{
			StopFlamethrowerGuns();
			pUnit->SetCommandFinished();
			return TSIR_YES_IMMIDIATELY;
		}
		bStop = true;
		return TSIR_YES_WAIT;
	}

	if ( pCommand == 0 || !pUnit->GetFirstArtilleryGun()->IsBursting() )
	{
		pUnit->GetFirstArtilleryGun()->StopFire();

		pUnit->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	bStop = true;
	return TSIR_YES_WAIT;
}

//*******************************************************************
//*										CArtilleryRangeAreaState											*
//*******************************************************************

IUnitState* CArtilleryRangeAreaState::Instance( CAIUnit *pUnit, const CVec2 &point )
{
	return new CArtilleryRangeAreaState( pUnit, point );
}

CArtilleryRangeAreaState::CArtilleryRangeAreaState( CAIUnit *_pUnit, const CVec2 &_point )
: CStatusUpdatesHelper( EUS_ZEROING_IN, _pUnit ), pUnit( _pUnit ), point( _point ), eState( ERAS_RANGING ), bFinish( false ),
	pGun( _pUnit->GetFirstArtilleryGun() ), nShootsLast( SConsts::SHOOTS_TO_RANGE ),
	bFired( true )
{
	pUnit->Stop();

	bool bCanShootWOMove = pUnit->GetFirstArtilleryGun()->CanShootToPointWOMove( point, 0 );
	bool bNeedTurn = false;

	if ( !bCanShootWOMove && pUnit->GetFirstArtilleryGun()->GetRejectReason() == ACK_NOT_IN_ATTACK_ANGLE )
	{
		if ( pUnit->CanRotate() )
		{
			bCanShootWOMove = true;
			bNeedTurn = true;
		}
	}
	
	if ( bCanShootWOMove )
	{
		if ( !bNeedTurn )
			eState = ERAS_RANGING;
		else
		{
			eState = ERAS_TURNING;
		}
	}
	else
	{
		const EUnitAckType eReject = pUnit->GetFirstArtilleryGun()->GetRejectReason();
		pUnit->SendAcknowledgement( eReject );
		if ( eReject == ACK_NO_AMMO && pUnit->GetPlayer() == theDipl.GetMyNumber() )
			theFeedBackSystem.AddFeedbackAndForget( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), EFB_NO_AMMO, -1 );

		pUnit->SetCommandFinished();
	}

	if ( bNeedTurn && !pUnit->CheckTurn( 1.0f, point - pUnit->GetCenterPlain(), false, false ) )
	{
		pUnit->SendAcknowledgement( ACK_NEGATIVE, true );
		pUnit->SetCommandFinished();
	}

	// Determine search circle radius
	const NDb::SUnitSpecialAblityDesc *pSA = pUnit->GetUnitAbilityDesc( NDb::ABILITY_ZEROING_IN );
	fSearchRadius = pSA ? pSA->fParameter : SConsts::RANGED_AREA_RADIUS;
}

void CArtilleryRangeAreaState::CheckArea()
{
	// по юнитам
	pUnit->ResetShootEstimator( 0, false, pUnit->GetForbiddenGuns() );
	for ( CUnitsIter<1,2> iter( pUnit->GetParty(), EDI_ENEMY, point, fSearchRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pTarget = *iter;
		if ( pTarget->IsVisible( pUnit->GetParty() ) &&
				 fabs2( pTarget->GetCenterPlain() - point ) <= sqr( fSearchRadius ) &&
				 pGun->CanShootToUnitWOMove( pTarget ) )
			pUnit->AddUnitToShootEstimator( pTarget );
	}

	// враг найден
	if ( pEnemy = pUnit->GetBestShootEstimatedUnit() )
	{
		eState = ERAS_SHOOT_UNIT;
		pGun->StartEnemyBurst( pEnemy, true );
	}
	// по объектам
	else
	{
		for ( CStObjCircleIter<true> iter( point, fSearchRadius ); !iter.IsFinished() && pObj == 0; iter.Iterate() )
		{
			CExistingObject *pIteratingObject = *iter;
			if ( pIteratingObject->GetObjectType() == ESOT_BUILDING && theDipl.GetDiplStatus( pIteratingObject->GetPlayer(), pUnit->GetPlayer() ) == EDI_ENEMY && 
					 pIteratingObject->IsVisible( pUnit->GetParty() ) && pIteratingObject->GetNDefenders() > 0 &&
					 fabs2( pIteratingObject->GetAttackCenter( point ) - point ) <= sqr( fSearchRadius ) && pGun->CanShootToObjectWOMove( pIteratingObject ) )
			{
				pObj = pIteratingObject;
			}
		}

		if ( pObj != 0 )
		{
			eState = ERAS_SHOOT_OBJECT;
			pGun->StartPointBurst( pObj->GetAttackCenter( pUnit->GetCenterPlain() ), true );
		}
	}
}

void CArtilleryRangeAreaState::Segment()
{
	InitStatus();
	if ( pUnit->IsOperable() && ( eState == ERAS_TURNING || pUnit->CanShoot() ) )
	{
		const bool bFiringNow = pGun->IsFiring();
		if ( !bFiringNow )
		{
			if ( bFinish )
				FinishCommand();
			else
			{
				switch ( eState )
				{
					case ERAS_TURNING:
						if ( pUnit->IsInTankPit() )
						{
							theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pUnit );
							return;
						}
				
						if ( pUnit->TurnToTarget( point ) )
							eState = ERAS_RANGING;

						break;
					case ERAS_RANGING:
						pGun->StartPointBurst( point, nShootsLast == SConsts::SHOOTS_TO_RANGE );
						if ( bFired )
							--nShootsLast;
						bFired = false;

						if ( nShootsLast == 0 )
						{
							pUnit->SetDispersionBonus( SConsts::RANDGED_DISPERSION_RADIUS_BONUS );
							eState = ERAS_WAITING;
							pGun->LockInCurAngle();
							lastCheck = curTime;
							//CCircle searchCircle( point, fSearchRadius );
							//DebugInfoManager()->CreateCircle( OBJECT_ID_FORGET, searchCircle, NAIVisInfo::GREEN );
						}

						break;
					case ERAS_WAITING:
						if ( curTime - lastCheck >= CHECK_TIME )
							CheckArea();

						break;
					case ERAS_SHOOT_UNIT:
						if ( IsValidObj( pEnemy ) && pEnemy->IsVisible( pUnit->GetParty() ) && 
								 pGun->CanShootToUnitWOMove( pEnemy ) && fabs2( pEnemy->GetCenterPlain() - point ) <= sqr( fSearchRadius ) )
							pGun->StartEnemyBurst( pEnemy, false );
						else
						{
							eState = ERAS_WAITING;
							lastCheck = curTime - CHECK_TIME;
							pEnemy = 0;
						}

						break;
					case ERAS_SHOOT_OBJECT:
						if ( IsValidObj( pObj ) && theDipl.GetDiplStatus( pObj->GetPlayer(), pUnit->GetPlayer() ) == EDI_ENEMY )
							pGun->StartPointBurst( pObj->GetAttackCenter( pUnit->GetCenterPlain() ), false );
						else
						{
							eState = ERAS_WAITING;
							lastCheck = curTime - CHECK_TIME;
							pObj = 0;
						}

						break;
				}
			}
		}
		else	
		{
			if ( pGun->GetNAmmo() == 0 )
			{
				pUnit->SendAcknowledgement( ACK_NO_AMMO );
				if ( pUnit->GetPlayer() == theDipl.GetMyNumber() )
					theFeedBackSystem.AddFeedbackAndForget( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), EFB_NO_AMMO, -1 );
				if ( NGlobal::GetVar( "stop_on_no_ammo", 0 ) )
					FinishCommand();
			}
			
			bFired = true;
		}
	}
}

void CArtilleryRangeAreaState::FinishCommand()
{
	if ( IsValid( pGun ) )
		pGun->StopFire();

	pUnit->GetFirstArtilleryGun()->UnlockCurAngle();
	pUnit->SetDispersionBonus( 1.0f );	
	pUnit->SetCommandFinished();
}

ETryStateInterruptResult CArtilleryRangeAreaState::TryInterruptState(class CAICommand *pCommand)
{
	if ( pCommand == 0 || !IsValid( pGun ) || !pGun->IsBursting() )
	{
		FinishCommand();
		return TSIR_YES_IMMIDIATELY;
	}
	bFinish = true;
	return TSIR_YES_WAIT;
}

void CArtilleryRangeAreaState::GetRangeCircle( CCircle *pCircle ) const
{
	pCircle->center = point;
	pCircle->r = SConsts::RANGED_AREA_RADIUS;
}

bool CArtilleryRangeAreaState::IsAttacksUnit() const
{
	return eState == ERAS_SHOOT_UNIT;
}

CAIUnit* CArtilleryRangeAreaState::GetTargetUnit() const
{
	if ( IsAttacksUnit() )
		return pEnemy;
	else
		return 0;
}

//*******************************************************************
//*										CArtilleryInstallTransportState								*
//*******************************************************************

IUnitState* CArtilleryInstallTransportState::Instance( CArtillery *pArtillery )
{
	return new CArtilleryInstallTransportState( pArtillery );
}

CArtilleryInstallTransportState::CArtilleryInstallTransportState( CArtillery *_pArtillery )
: pArtillery( _pArtillery ), eState( AITS_WAITING_FOR_CREW )
{
}

void CArtilleryInstallTransportState::Segment()
{
//	CFormation * pCrew = pArtillery->GetCrew();
	switch ( eState )
	{
	case AITS_WAITING_FOR_CREW:
		if ( pArtillery->MustHaveCrewToOperate() )
		{
			pArtillery->InstallAction( ACTION_NOTIFY_INSTALL_TRANSPORT );
			eState = AITS_INSTALLING;
		}
		else if ( !pArtillery->HasServeCrew() )
		{
			pArtillery->InstallAction( ACTION_NOTIFY_INSTALL_TRANSPORT );
			pArtillery->SetCommandFinished();
		}
		else if ( pArtillery->IsOperable() )
		{
			pArtillery->InstallAction( ACTION_NOTIFY_INSTALL_TRANSPORT );
			eState = AITS_INSTALLING;
		}

		break;
	case AITS_INSTALLING:
		if ( pArtillery->IsInstalled() )
			pArtillery->SetCommandFinished();
		break;
	}
}

ETryStateInterruptResult CArtilleryInstallTransportState::TryInterruptState(class CAICommand *pCommand)
{
	if ( 0 == pCommand )
	{
		pArtillery->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	return TSIR_YES_WAIT;
}

const CVec2 CArtilleryInstallTransportState::GetPurposePoint() const
{
	return pArtillery->GetCenterPlain();
}

//*******************************************************************
//*										CArtilleryUninstallTransportState							*
//*******************************************************************

IUnitState* CArtilleryUninstallTransportState::Instance( CArtillery *pArtillery )
{
	return new CArtilleryUninstallTransportState( pArtillery );
}

CArtilleryUninstallTransportState::CArtilleryUninstallTransportState( CArtillery *_pArtillery )
: pArtillery( _pArtillery )
{
	if ( pArtillery->IsUninstalled() && pArtillery->GetCurUninstallAction() == ACTION_NOTIFY_UNINSTALL_TRANSPORT )
	{
		pArtillery->SetCommandFinished();
		//CRAP{ данный state создается в сегменте QueueUnit, после чего флаг bCmdFinished сбрасывается в false (в конце
		// процедуры сегмента.
		eState = AUTS_WAIT_FOR_UNINSTALL;
		//CRAP}
	}
	else if ( !pArtillery->IsInstalled() )
		eState = AUTS_WAIT_FOR_UNINSTALL;
	else
		eState = AUTS_INSTALLING;
}

void CArtilleryUninstallTransportState::Segment()
{
	switch( eState )
	{
	case AUTS_WAIT_FOR_UNINSTALL:
		// дождаться, пока пушка деинсталлируется
		//( в случае если злоумышленники деинсталлировали ее в момент подцепления)
		if ( pArtillery->IsUninstalled() )
		{
			if ( pArtillery->GetCurUninstallAction() == ACTION_NOTIFY_UNINSTALL_TRANSPORT )
				eState = AUTS_WAIT_FOR_UNINSTALL_TRANSPORT;
			else
			{
				pArtillery->InstallBack( false );
				eState = AUTS_INSTALLING; // 
			}
		}
		// break убран сознательно
	case AUTS_INSTALLING:
		if ( pArtillery->IsInstalled() )
		{
			pArtillery->InstallAction( ACTION_NOTIFY_UNINSTALL_TRANSPORT );
			eState = AUTS_WAIT_FOR_UNINSTALL_TRANSPORT;
		}
		break;

	case AUTS_WAIT_FOR_UNINSTALL_TRANSPORT:
		if ( pArtillery->IsUninstalled() )
		{
			pArtillery->SetCommandFinished();
		}
	}
}

ETryStateInterruptResult CArtilleryUninstallTransportState::TryInterruptState(class CAICommand *pCommand)
{
	if ( !pCommand )
	{
		pArtillery->SetCommandFinished();
	}
	else if ( pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_BEING_TOWED )
	{
		return TSIR_YES_WAIT;
	}
	return TSIR_NO_COMMAND_INCOMPATIBLE; // этот стейт завершается сам
}

const CVec2 CArtilleryUninstallTransportState::GetPurposePoint() const
{
	return pArtillery->GetCenterPlain();
}

//*******************************************************************
//*										CArtilleryBeingTowedState											*
//*******************************************************************

IUnitState* CArtilleryBeingTowedState::Instance( class CArtillery *pArtillery, class CAITransportUnit * pTransport )
{
	return new CArtilleryBeingTowedState( pArtillery, pTransport );
}

CArtilleryBeingTowedState::CArtilleryBeingTowedState( CArtillery *pArtillery, CAITransportUnit * _pTransport )
: pArtillery( pArtillery ), pTransport ( _pTransport ), wLastTagDir ( _pTransport->GetFrontDirection() ),
	vLastTagCenter ( _pTransport->GetCenterPlain() ), bInterrupted( false ), timeLastUpdate( curTime )
{
	pArtillery->SetSelectable( false, true );

	pPath = new CArtilleryBeingTowedPath( 0.0f, pArtillery->GetCenterPlain(), VNULL2 );
	pPath->Init( 0.0f, pArtillery->GetCenterPlain(), VNULL2 );
	pArtillery->SetSmoothPath( pPath );
	pArtillery->SetGoForward( false );

	updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, pArtillery, -1 );
}

void CArtilleryBeingTowedState::Segment()
{
	if ( !IsValidObj( pTransport ) )
	{
		pArtillery->ChangePlayer( theDipl.GetNeutralPlayer() );
		TryInterruptState( 0 );
	}
	// изменилось положение буксира
	else if (	wLastTagDir != pTransport->GetFrontDirection() || vLastTagCenter != pTransport->GetCenterPlain() )
	{
		CVec2 vFormerPos( pArtillery->GetCenterPlain() );

		const CVec2 tagUnitDir = GetVectorByDirection( pTransport->GetFrontDirection() );
		const CVec2 hookPoint = pTransport->GetHookPoint();

		// новое направление
		CVec2 newDirVec = hookPoint - pArtillery->GetCenterPlain();

		// если начало движения и тягач не попал точкой прицепления в точку прицепления артиллерии (точка ближе, чем надо)
		bool bZeroSpeed = false;
		if ( newDirVec  *  tagUnitDir < 0 )
		{
			bZeroSpeed = true;
			const CVec2 perpVec( -tagUnitDir.y, tagUnitDir.x ); 
			
			if ( perpVec*newDirVec >= 0 )
				newDirVec = perpVec;
			else
				newDirVec = CVec2( tagUnitDir.y, -tagUnitDir.x );
		}

		pArtillery->SetDirectionVec( newDirVec );

		wLastTagDir = pTransport->GetFrontDirection();
		vLastTagCenter = pTransport->GetCenterPlain();

		const CVec2 vArtilleryHookPoint = pArtillery->GetHookPoint();
		const CVec2 vShiftFromHookPointToArtillery = pArtillery->GetCenterPlain() - vArtilleryHookPoint;
		const CVec2 coord = hookPoint + vShiftFromHookPointToArtillery;

		pArtillery->SetCenter( CVec3( coord.x, coord.y, GetHeights()->GetZ( coord ) ) );

		const float fSpeed = ( bZeroSpeed ? 0 : fabs(coord - vFormerPos) ) / ( curTime - timeLastUpdate );
		pPath->Init( fSpeed, coord, ( coord - vFormerPos ) / ( curTime - timeLastUpdate ) );
		pArtillery->SetSmoothPath( pPath );
		pArtillery->UnlockTiles();
	}
	else
	{
		pPath->Init( 0.0f, pArtillery->GetCenterPlain(), VNULL2 );
		pArtillery->LockTiles();
	}

	timeLastUpdate = curTime;
}

ETryStateInterruptResult CArtilleryBeingTowedState::TryInterruptState( CAICommand *pCommand )
{
	if ( pCommand == 0 )
	{
		if ( IsValidObj( pTransport ) )
			pArtillery->SetSelectable( false, true );

		pArtillery->SetGoForward( true );
		bInterrupted = true;
		pArtillery->SetCommandFinished();
		pArtillery->RestoreSmoothPath();
		pArtillery->Stop();
		pArtillery->LockTiles();

		updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, pArtillery, -1 );

		return TSIR_YES_IMMIDIATELY;
	}
	return TSIR_NO_COMMAND_INCOMPATIBLE;
}

const CVec2 CArtilleryBeingTowedState::GetPurposePoint() const
{
	return pArtillery->GetCenterPlain();
}

CAITransportUnit* CArtilleryBeingTowedState::GetTowingTransport() const 
{ 
	return pTransport;
}

//*******************************************************************
//*										CArtilleryAttackState													*
//*******************************************************************

IUnitState* CArtilleryAttackState::Instance( CArtillery *pArtillery, CAIUnit *pEnemy, bool bAim, const bool bSwarmAttack )
{
	return new CArtilleryAttackState( pArtillery, pEnemy, bAim, bSwarmAttack );
}

CArtilleryAttackState::CArtilleryAttackState( CArtillery *_pArtillery, CAIUnit *_pEnemy, bool _bAim, bool _bSwarmAttack )
: pArtillery( _pArtillery ), pEnemy( _pEnemy ), bAim( _bAim ), eState( EAS_NONE ), CFreeFireManager( _pArtillery ), 
	bFinish( false ), bSwarmAttack( _bSwarmAttack ), nEnemyParty( _pEnemy->GetParty() )
{
	pArtillery->Stop();
	if ( _pEnemy->GetStats()->IsAviation() )
		FinishState();
/*
	if ( bSwarmAttack )
		pArtillery->ResetTargetScan();
*/
}

void CArtilleryAttackState::FinishState()
{
	for ( int i = 0; i < pArtillery->GetNGuns(); ++i )
		pArtillery->GetGun( i )->StopFire();

	damageToEnemyUpdater.UnsetDamageFromEnemy( pEnemy );
	pArtillery->SetCommandFinished();
	if ( eState = EAS_ROTATING )
		updater.AddUpdate( CreateStatusUpdate( EUS_ROTATE, false ), ACTION_NOTIFY_UPDATE_STATUS, pArtillery, -1 );
}

void CArtilleryAttackState::Segment()
{
	if ( pArtillery->IsOperable() )
	{
		if ( ( !IsValid(pGun) || !pGun->IsBursting() ) && 
		 		 ( !IsValidObj( pEnemy ) || !pEnemy->IsVisible( pArtillery->GetParty() ) || bFinish ||
					 pEnemy->GetParty() != nEnemyParty ) )
			FinishState();
		else if ( pArtillery->IsInstalled() || eState == EAS_NONE || eState == EAS_ROTATING )
		{
			damageToEnemyUpdater.SetDamageToEnemy( pArtillery, pEnemy, pGun );
			// если можно перевыбирать цель, то выбрать цель
			if ( bSwarmAttack )
				pArtillery->AnalyzeTargetScan( pEnemy, damageToEnemyUpdater.IsDamageUpdated(), false );

			switch ( eState )
			{
				case EAS_NONE:
					pArtillery->ResetShootEstimator( pEnemy, false, pArtillery->GetForbiddenGuns() );
					pGun = pArtillery->GetBestShootEstimatedGun();

					if ( pGun == 0 )
					{
						bAim = true;
						if ( pArtillery->DoesExistRejectGunsReason( ACK_NOT_IN_ATTACK_ANGLE ) )
						{
							const CVec2 vDirToRotate = pEnemy->GetCenterPlain() - pArtillery->GetCenterPlain();
							wDirToRotate = GetDirectionByVector( vDirToRotate );
							if ( !pArtillery->CheckTurn( 1.0f, vDirToRotate, false, false ) )
							{
								pArtillery->SendAcknowledgement( ACK_NOT_IN_ATTACK_ANGLE );
								FinishState();
							}
							else
							{
								if ( pArtillery->IsInTankPit() )
								{
									theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pArtillery );
									return;
								}
								updater.AddUpdate( CreateStatusUpdate( EUS_ROTATE, true ), ACTION_NOTIFY_UPDATE_STATUS, pArtillery, -1 );
								eState = EAS_ROTATING;
							}
						}
						else
						{
							// if enemy is artillery - try attack crew
							if ( pEnemy->GetStats()->IsArtillery() )
							{
								CFormation *pCrew = checked_cast_ptr<CArtillery*>(pEnemy)->GetCrew();
								if ( pCrew )
								{
									pEnemy = (*pCrew)[0];
									pArtillery->ResetShootEstimator( (*pCrew)[0], false, pArtillery->GetForbiddenGuns() );
								}
								pGun = pArtillery->GetBestShootEstimatedGun();
							}
							if ( !pGun )
							{
								pArtillery->SendAcknowledgement( pArtillery->GetGunsRejectReason() );
								FinishState();
							}
							else
							{
								
								pArtillery->Lock( pGun );
								if ( pGun->GetTurret() )
									pGun->GetTurret()->Lock( pGun );

								eState = EAS_FIRING;
							}
						}
					}
					else
					{
						pArtillery->Lock( pGun );
						if ( pGun->GetTurret() )
							pGun->GetTurret()->Lock( pGun );

						eState = EAS_FIRING;
					}

					break;
				case EAS_ROTATING:
					if ( pArtillery->TurnToDir( wDirToRotate, false ) )
					{
						updater.AddUpdate( CreateStatusUpdate( EUS_ROTATE, false ), ACTION_NOTIFY_UPDATE_STATUS, pArtillery, -1 );
						eState = EAS_NONE;
					}

					break;
				case EAS_FIRING:
					if ( pArtillery->IsInstalled() )
					{
						Analyze( pArtillery, pGun );
						
						if ( !pGun->IsFiring() )
						{
							if ( pGun->CanShootToUnit( pEnemy ) )
							{
								pGun->StartEnemyBurst( pEnemy, bAim );
								bAim = false;
							}
							else
								FinishState();
//								eState = EAS_NONE;
						}
						else if ( pGun->GetNAmmo() == 0 )
						{
							pArtillery->SendAcknowledgement( ACK_NO_AMMO );
							if ( pArtillery->GetPlayer() == theDipl.GetMyNumber() )
								theFeedBackSystem.AddFeedbackAndForget( pArtillery->GetUniqueID(), pArtillery->GetCenterPlain(), EFB_NO_AMMO, -1 );
							if ( NGlobal::GetVar( "stop_on_no_ammo", 0 ) )
								FinishState();
						}
					}
					else if ( !pArtillery->IsInInstallAction() )
						pArtillery->ForceInstallAction();
			}
		}
	}
	else if ( !pArtillery->HasServeCrew() )
		FinishState();
}

ETryStateInterruptResult CArtilleryAttackState::TryInterruptState( CAICommand *pCommand )
{
	if ( !pCommand )
	{
		FinishState();
		return TSIR_YES_IMMIDIATELY;
	}
	
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	if ( cmd.nCmdType == ACTION_COMMAND_ATTACK_UNIT && GetObjectByCmd( cmd ) == pEnemy )
		return TSIR_NO_COMMAND_INCOMPATIBLE;

	bFinish = true;
	return TSIR_YES_WAIT;
}

const CVec2 CArtilleryAttackState::GetPurposePoint() const
{
	if ( IsValidObj( pEnemy ) )
		return pEnemy->GetCenterPlain();
	return CVec2( -1.0f, -1.0f );
}

CAIUnit* CArtilleryAttackState::GetTargetUnit() const
{
	return pEnemy;
}

//*******************************************************************
//*									CArtilleryAttackCommonStatObjState							*
//*******************************************************************

IUnitState* CArtilleryAttackCommonStatObjState::Instance( CArtillery *pArtillery, CStaticObject *pObj )
{
	return new CArtilleryAttackCommonStatObjState( pArtillery, pObj );
}

CArtilleryAttackCommonStatObjState::CArtilleryAttackCommonStatObjState( CArtillery *_pArtillery, CStaticObject *_pObj )
: pArtillery( _pArtillery ), pObj( _pObj ), eState( EAS_NONE ), CFreeFireManager( _pArtillery ), bFinish( false ), bAim( true )
{
	pArtillery->Stop();
}

void CArtilleryAttackCommonStatObjState::FinishState()
{
	for ( int i = 0; i < pArtillery->GetNGuns(); ++i )
		pArtillery->GetGun( i )->StopFire();

	pArtillery->SetCommandFinished();
}

void CArtilleryAttackCommonStatObjState::Segment()
{
	if ( pArtillery->IsOperable() )
	{
		if ( !IsValidObj( pObj ) && ( !IsValid( pGun ) || !pGun->IsBursting() ) ||
				 bFinish && ( !IsValid( pGun ) || !pGun->IsBursting() ) )
			 FinishState();
		else if ( pArtillery->IsInstalled() || eState == EAS_ROTATING || eState == EAS_NONE )
		{
			switch ( eState )
			{
				case EAS_NONE:
					pGun = pArtillery->ChooseGunForStatObjWOTime( pObj );

					if ( pGun == 0 )
					{
						bAim = true;
						if ( pArtillery->DoesExistRejectGunsReason( ACK_NOT_IN_ATTACK_ANGLE ) )
						{
							const CVec2 vDirToRotate = pObj->GetAttackCenter( pArtillery->GetCenterPlain() ) - pArtillery->GetCenterPlain();
							wDirToRotate = GetDirectionByVector( vDirToRotate );
							if ( !pArtillery->CheckTurn( 1.0f, vDirToRotate, false, false ) )
							{
								pArtillery->SendAcknowledgement( ACK_NOT_IN_ATTACK_ANGLE );
								FinishState();
							}
							else
							{
								if ( pArtillery->IsInTankPit() )
								{
									theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pArtillery );
									return;
								}
								eState = EAS_ROTATING;
							}
						}
						else
						{
							pArtillery->SendAcknowledgement( pArtillery->GetGunsRejectReason() );
							FinishState();
						}
					}
					else
					{
						pArtillery->Lock( pGun );
						if ( pGun->GetTurret() )
							pGun->GetTurret()->Lock( pGun );

						eState = EAS_FIRING;
					}

					break;
				case EAS_ROTATING:
					if ( pArtillery->TurnToDir( wDirToRotate, false ) )
						eState = EAS_NONE;

					break;
				case EAS_FIRING:
					if ( pArtillery->IsInstalled() )
					{
						Analyze( pArtillery, pGun );
						
						if ( !pGun->IsFiring() )
						{
							if ( pGun->CanShootToObject( pObj ) )
							{
								pGun->StartPointBurst( pObj->GetAttackCenter( pArtillery->GetCenterPlain() ), bAim );
								bAim = false;
							}
							else
								FinishState();
//								eState = EAS_NONE;
						}
						else if ( pGun->GetNAmmo() == 0 )
						{
							pArtillery->SendAcknowledgement( ACK_NO_AMMO );
							if ( pArtillery->GetPlayer() == theDipl.GetMyNumber() )
								theFeedBackSystem.AddFeedbackAndForget( pArtillery->GetUniqueID(), pArtillery->GetCenterPlain(), EFB_NO_AMMO, -1 );
							if ( NGlobal::GetVar( "stop_on_no_ammo", 0 ) )
								FinishState();
						}
					}
					else if ( !pArtillery->IsInInstallAction() )
						pArtillery->ForceInstallAction();
			}
		}
	}
	else if ( !pArtillery->HasServeCrew() )
		FinishState();
}

ETryStateInterruptResult CArtilleryAttackCommonStatObjState::TryInterruptState( CAICommand *pCommand )
{
	if ( !pCommand )
	{
		FinishState();
		return TSIR_YES_IMMIDIATELY;
	}
	
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	if ( cmd.nCmdType == ACTION_COMMAND_ATTACK_OBJECT && GetObjectByCmd( cmd ) == pObj )
		return TSIR_NO_COMMAND_INCOMPATIBLE;

	bFinish = true;
	return TSIR_YES_WAIT;
}

const CVec2 CArtilleryAttackCommonStatObjState::GetPurposePoint() const
{
	if ( IsValidObj( pObj ) && pArtillery && pArtillery->IsRefValid() && pArtillery->IsAlive() )
		return pObj->GetAttackCenter( pArtillery->GetCenterPlain() );
	return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*												CArtilleryRestState												*
//*******************************************************************

IUnitState* CArtilleryRestState::Instance( CArtillery *pArtillery, const CVec2 &guardPoint, const WORD _wDir, const float _fTimeToWait )
{
	return new CArtilleryRestState( pArtillery, guardPoint, _wDir, _fTimeToWait );
}

CArtilleryRestState::CArtilleryRestState( CArtillery *_pArtillery, const CVec2 &guardPoint, const WORD _wDir, const float _fTimeToWait  )
: pArtillery( _pArtillery ), CMechUnitRestState( _pArtillery, guardPoint, _wDir, 0, _fTimeToWait )
{
	pArtillery->Stop();
}

void CArtilleryRestState::Segment()
{
	if ( pArtillery->IsInstalled() )
		CMechUnitRestState::Segment();
}

//*******************************************************************
//*										CArtilleryAttackAviationState									*
//*******************************************************************

IUnitState* CArtilleryAttackAviationState::Instance( CArtillery *pArtillery, CAviation *pPlane )
{
	return new CArtilleryAttackAviationState( pArtillery, pPlane );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
CArtilleryAttackAviationState::CArtilleryAttackAviationState( CArtillery *_pArtillery, CAviation *pPlane )
: pArtillery( _pArtillery ), CSoldierAttackAviationState( _pArtillery, pPlane )
{
}

void CArtilleryAttackAviationState::Segment()
{
	if ( pArtillery->IsInstalled() )
		CSoldierAttackAviationState::Segment();
}


