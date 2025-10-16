#include "stdafx.h"

#include "ArtRocketStates.h"
#include "TechnicsStates.h"
#include "Commands.h"
#include "Artillery.h"
#include "ArtilleryStates.h"
#include "Guns.h"
#include "GroupLogic.h"
#include "Technics.h"
#include "Turret.h"
#include "ArtilleryBulletStorage.h"
#include "FeedbackSystem.h"
#include "Diplomacy.h"

#include "AILogic_export.h"

#include <fmt/format.h>

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D4AC, CArtRocketStatesFactory );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D4AD, CArtRocketAttackGroundState );

extern CDiplomacy theDipl;
extern CFeedBackSystem theFeedBackSystem;
extern NTimer::STime curTime;
extern CGroupLogic theGroupLogic;

//*******************************************************************
//*										 CArtRocketStatesFactory											*
//*******************************************************************

CPtr<CArtRocketStatesFactory> CArtRocketStatesFactory::pFactory = 0;

IStatesFactory* CArtRocketStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CArtRocketStatesFactory();

	return pFactory;
}

bool CArtRocketStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		( cmdType == ACTION_COMMAND_DIE							||
			cmdType == ACTION_COMMAND_MOVE_TO					||
			cmdType == ACTION_COMMAND_REVERSE_TO				||
			cmdType == ACTION_COMMAND_ART_BOMBARDMENT	||
			cmdType == ACTION_COMMAND_FIRE_ROCKETS		||
			cmdType == ACTION_COMMAND_USE_FLAMETHROWER ||
			cmdType == ACTION_COMMAND_ROTATE_TO				||
			cmdType == ACTION_COMMAND_INSTALL					||
			cmdType == ACTION_COMMAND_UNINSTALL				||
			cmdType == ACTION_COMMAND_DISAPPEAR				||
			cmdType == ACTION_MOVE_IDLE								||
			cmdType == ACTION_COMMAND_GUARD						||
			cmdType == ACTION_MOVE_BEING_TOWED				||
			cmdType == ACTION_COMMAND_ATTACK_OBJECT		||
			cmdType == ACTION_COMMAND_ENTRENCH_SELF		||
			cmdType == ACTION_COMMAND_CHANGE_SHELLTYPE ||
			cmdType == ACTION_MOVE_LEAVE_SELF_ENTRENCH	||
			cmdType == ACTION_COMMAND_ROTATE_TO_DIR ||
			cmdType == ACTION_COMMAND_STAND_GROUND ||
			cmdType == ACTION_COMMAND_MOVE_TO_GRID ||
			cmdType == ACTION_COMMAND_SWARM_TO ||
			cmdType == ACTION_COMMAND_WAIT ||
			cmdType == ACTION_COMMAND_UNLOAD ||
			cmdType == ACTION_COMMAND_MECH_ENTER ||
			cmdType == ACTION_MOVE_MECH_ENTER_NOW ||
			cmdType == ACTION_COMMAND_HOLD_SECTOR ||
			cmdType == ACTION_MOVE_ONBOARD_ATTACK_UNIT
		);
}

IUnitState* CArtRocketStatesFactory::ProduceState( class CQueueUnit *pObj, CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CArtillery*>( pObj ) != 0, "Wrong unit type" );
	CArtillery *pArtillery = checked_cast<CArtillery*>( pObj );

	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();	
	IUnitState* pResult = 0;
	
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

		case ACTION_MOVE_LEAVE_SELF_ENTRENCH:
			pArtillery->ResetHoldSector();
			pResult = CTankPitLeaveState::Instance( pArtillery );

			break;

		case ACTION_COMMAND_ENTRENCH_SELF:
			pResult = CMechUnitEntrenchSelfState::Instance( pArtillery );

			break;
		case ACTION_COMMAND_HOLD_SECTOR:
			switch ( cmd.nNumber )
			{	
			case 0:
				{
					theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ROTATE_TO, cmd.vPos ), pArtillery );
					SAIUnitCmd newCmd( ACTION_COMMAND_HOLD_SECTOR );
					newCmd.nNumber = 2;
					theGroupLogic.InsertUnitCommand( newCmd, pArtillery );
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
		case ACTION_MOVE_BEING_TOWED:
			{
				CONVERT_OBJECT( CAITransportUnit, pTransportUnit, GetObjectByCmd( cmd ), "Wrong unit to attach artillery" );
				pResult = CArtilleryBeingTowedState::Instance( pArtillery, pTransportUnit );
			}

			break;
		case ACTION_COMMAND_DIE:
			NI_ASSERT( false, "Command to die in the queue" );

			break;
		case ACTION_MOVE_IDLE:
			pResult = CSoldierIdleState::Instance( pArtillery );
			
			break;
		case ACTION_COMMAND_MOVE_TO:
		case ACTION_COMMAND_REVERSE_TO:
			{
				pArtillery->UnsetFollowState();				
				pArtillery->ResetHoldSector();
				if ( pArtillery->IsInTankPit() )// сначала выйти из TankPit, потом поехать куда послали
				{
					theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pArtillery );
					theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pArtillery );
				}
				else 
					pResult = CArtilleryMoveToState::Instance( pArtillery, cmd.vPos, cmd.nCmdType == ACTION_COMMAND_REVERSE_TO );
			}

			break;
		case ACTION_COMMAND_ART_BOMBARDMENT:
		case ACTION_COMMAND_FIRE_ROCKETS:
			pResult = CArtRocketAttackGroundState::Instance( pArtillery, cmd.vPos );

			break;
		case ACTION_COMMAND_USE_FLAMETHROWER:
			// Rocket artillery uses its normal state above; this ability explicitly selects flame shells instead.
			pResult = CArtilleryBombardmentState::Instance( pArtillery, cmd.vPos, 1, true );

			break;
		case ACTION_COMMAND_ROTATE_TO:
			pArtillery->ResetHoldSector();
			if ( pArtillery->IsInTankPit() && !pCommand->IsFromAI() )// сначала выйти из TankPit, потом поехать куда послали
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
			}
			else
			{
				CVec2 vDir = cmd.vPos;
				Normalize( &vDir );
				pResult = CArtilleryTurnToPointState::Instance( pArtillery, pArtillery->GetCenterPlain() + vDir );
			}

			break;
		case ACTION_COMMAND_GUARD:
			pResult = CArtilleryRestState::Instance( pArtillery, cmd.vPos, cmd.fNumber, -1 );

			break;
		case ACTION_COMMAND_INSTALL:
			pResult = CArtilleryInstallTransportState::Instance( pArtillery );

			break;
		case ACTION_COMMAND_UNINSTALL:
			pResult = CArtilleryUninstallTransportState::Instance( pArtillery );

			break;
		case ACTION_COMMAND_ATTACK_OBJECT:
			{
				CONVERT_OBJECT( CStaticObject, pStaticObj, GetObjectByCmd( cmd ), "Wrong static object to attack" );
				// attack the artillery
				if ( pStaticObj->GetObjectType() == ESOT_ARTILLERY_BULLET_STORAGE )
				{
					pResult = CArtRocketAttackGroundState::Instance( pArtillery, checked_cast<CArtilleryBulletStorage*>(pStaticObj)->GetOwner()->GetCenterPlain() );
				}
				else
					pResult = CArtilleryAttackCommonStatObjState::Instance( pArtillery, pStaticObj );

			}

			break;
		case ACTION_COMMAND_CHANGE_SHELLTYPE:
			pArtillery->SetActiveShellType( static_cast<NDb::SWeaponRPGStats::SShell::EShellDamageType>( int(cmd.fNumber)) );

			break;
		case ACTION_COMMAND_STAND_GROUND:
			pArtillery->Stop();
			pArtillery->UnsetFollowState();				
			pArtillery->SetBehaviourMoving( SBehaviour::EMHoldPos );

			break;
		case ACTION_COMMAND_MOVE_TO_GRID:
			pResult = CCommonMoveToGridState::Instance( pArtillery, cmd.vPos, GetVectorByDirection( cmd.fNumber ) );

			break;
		case ACTION_COMMAND_SWARM_TO:
			// сначала выйти из TankPit, потом поехать куда послали			
			pArtillery->ResetHoldSector();
			if ( pArtillery->IsInTankPit() && !pCommand->IsFromAI() )
			{
				theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pArtillery );
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pArtillery );
			}
			else if ( !pArtillery->CanMove() )
				pArtillery->SendAcknowledgement( pCommand, ACK_NEGATIVE, !pCommand->IsFromAI() );
			else 
				pResult = CCommonSwarmState::Instance( pArtillery, cmd.vPos, cmd.fNumber );
			
			break;
		default:
			NI_ASSERT( false, "Wrong command" );
	}

	return pResult;
}

IUnitState* CArtRocketStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	NI_ASSERT( dynamic_cast<CArtillery*>( pUnit ) != 0, "Wrong unit type" );	
	CArtillery * pArt = checked_cast<CArtillery*>( pUnit );
	return CArtilleryRestState::Instance( pArt, CVec2( -1, -1 ), 0, -1 );
}

//*******************************************************************
//*										 CArtRocketAttackGroundState									*
//*******************************************************************

IUnitState* CArtRocketAttackGroundState::Instance( CArtillery *pArtillery, const CVec2 &point )
{
	return new CArtRocketAttackGroundState( pArtillery, point );
}

CArtRocketAttackGroundState::CArtRocketAttackGroundState( CArtillery *_pArtillery, const CVec2 &_point )
: pArtillery( _pArtillery ), point( _point ), bFired( false ), eState( EAGS_FIRING ), wDirToRotate( 0 ), bFinished( false ),
bSaidNoAmmo( false )
{
	pArtillery->Stop();
	NI_ASSERT( pArtillery->GetFirstArtilleryGun() != 0, "ArtRocket unit doesn't have an aritillery gun" );
	bool bCanShoot = pArtillery->GetFirstArtilleryGun()->CanShootToPointWOMove( point, 0 );

	if ( !bCanShoot )
	{
		if ( pArtillery->GetFirstArtilleryGun()->GetRejectReason() == ACK_NOT_IN_ATTACK_ANGLE )
		{
			const CVec2 vDirToRotate = point - pArtillery->GetCenterPlain();
			if ( !pArtillery->CheckTurn( 1.0f, vDirToRotate, false, false ) )
			{
				pArtillery->SendAcknowledgement( ACK_INVALID_TARGET, true );
				bFinished = true;
			}
			else
			{
				wDirToRotate = GetDirectionByVector( vDirToRotate );
				eState = EAGS_ROTATING;
			}
		}
		else
		{
			const EUnitAckType eReject = pArtillery->GetFirstArtilleryGun()->GetRejectReason();
			pArtillery->SendAcknowledgement( eReject, true );
			if ( eReject == ACK_NO_AMMO && pArtillery->GetPlayer() == theDipl.GetMyNumber() )
				theFeedBackSystem.AddFeedbackAndForget( pArtillery->GetUniqueID(), pArtillery->GetCenterPlain(), EFB_NO_AMMO, -1 );

			bFinished = true;
		}
	}
	else
		eState = EAGS_FIRING;
}

void CArtRocketAttackGroundState::Segment()
{
	NI_ASSERT( pArtillery->GetFirstArtilleryGun() != 0, "Rocket unit doesn't have any ballistic guns" );
	if ( bFinished )
		pArtillery->SetCommandFinished();
	else
	{
		switch ( eState )
		{
			case EAGS_ROTATING:
				if ( pArtillery->TurnToDir( wDirToRotate, false ) )
				{
					eState = EAGS_FIRING;
					pArtillery->ForceInstallAction();
				}

				break;
			case EAGS_FIRING:
				if ( pArtillery->IsInstalled() )
				{
					if ( !pArtillery->GetFirstArtilleryGun()->IsFiring() )
					{
						bSaidNoAmmo = false;
						if ( bFired )
						{
							CBasicGun *pGun = pArtillery->GetFirstArtilleryGun();							
							CTurret *pTurret = pGun->GetTurret();
							pTurret->Unlock( pArtillery->GetFirstArtilleryGun() );
							const int nGuns = pArtillery->GetNGuns();
							for ( int i = 0; i < nGuns; ++i )
							{
								CBasicGun *pGun = pArtillery->GetGun( i );
								if ( pTurret == pGun->GetTurret() )
									pGun->StopFire();
							}

							pArtillery->SetCommandFinished();
							if ( pArtillery->GetFirstArtilleryGun()->GetNAmmo() == 0 )
							{
								pArtillery->SendAcknowledgement( ACK_NO_AMMO );
								if ( pArtillery->GetPlayer() == theDipl.GetMyNumber() )
									theFeedBackSystem.AddFeedbackAndForget( pArtillery->GetUniqueID(), pArtillery->GetCenterPlain(), EFB_NO_AMMO, -1 );
							}
						}
						else
						{
							CBasicGun *pGun = pArtillery->GetFirstArtilleryGun();
							pGun->GetTurret()->Lock( pGun );
							pGun->StartPointBurst( point, true );

							bFired = true;
						}
					}
					else if ( pArtillery->GetFirstArtilleryGun()->GetNAmmo() == 0 )
					{
						if ( !bSaidNoAmmo )
						{
							pArtillery->SendAcknowledgement( ACK_NO_AMMO );
							if ( pArtillery->GetPlayer() == theDipl.GetMyNumber() )
								theFeedBackSystem.AddFeedbackAndForget( pArtillery->GetUniqueID(), pArtillery->GetCenterPlain(), EFB_NO_AMMO, -1 );
							bSaidNoAmmo = true;
							if ( NGlobal::GetVar( "stop_on_no_ammo", 0 ) )
								pArtillery->SetCommandFinished();
						}
					}
				}

				break;
			default: NI_ASSERT( false, fmt::format( "Wrong CArtRocketAttackGroundState ({})", (int)eState ) );
		}
	}
}

ETryStateInterruptResult CArtRocketAttackGroundState::TryInterruptState(class CAICommand *pCommand)
{
	if ( pCommand == 0 || !pArtillery->GetFirstArtilleryGun()->IsBursting() )
	{
		pArtillery->GetFirstArtilleryGun()->StopFire();
		pArtillery->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	return TSIR_YES_WAIT;
}


