#include "stdafx.h"

#include "TankStates.h"
#include "Commands.h"
#include "ArtilleryStates.h"
#include "Technics.h"
#include "Soldier.h"
#include "Guns.h"
#include "NewUpdater.h"
#include "GroupLogic.h"
#include "TransportStates.h"
#include "Formation.h"
#include "ArtilleryBulletStorage.h"
#include "TechnicsStates.h"
#include "UnitsIterators2.h"
#include "StaticObjectsIters.h"
#include "FeedBackSystem.h"

REGISTER_SAVELOAD_CLASS( 0x1108D493, CTankStatesFactory );

extern CFeedBackSystem theFeedBackSystem;
extern CGroupLogic theGroupLogic;
extern NTimer::STime curTime;
extern CEventUpdater updater;
extern CDiplomacy theDipl;

//*******************************************************************
//*										  CTankStatesFactory													*
//*******************************************************************

CPtr<CTankStatesFactory> CTankStatesFactory::pFactory = 0;

IStatesFactory* CTankStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CTankStatesFactory();

	return pFactory;
}

bool CTankStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		( cmdType == ACTION_COMMAND_DIE							||
			cmdType == ACTION_COMMAND_MOVE_TO					||
			cmdType == ACTION_COMMAND_REVERSE_TO				||
			cmdType == ACTION_COMMAND_ATTACK_UNIT			||
			cmdType == ACTION_COMMAND_ATTACK_OBJECT		||
			cmdType == ACTION_COMMAND_ROTATE_TO				||
			cmdType == ACTION_MOVE_BY_DIR							||
			cmdType == ACTION_COMMAND_SWARM_TO				||
			cmdType == ACTION_COMMAND_GUARD						||
			cmdType == ACTION_COMMAND_TRACK_TARGETING	||
			cmdType == ACTION_COMMAND_RANGE_AREA			||
			cmdType == ACTION_COMMAND_ART_BOMBARDMENT ||
			cmdType == ACTION_COMMAND_FIRE_ROCKETS		||
			cmdType == ACTION_COMMAND_DISAPPEAR				||
			cmdType == ACTION_MOVE_LEAVE_SELF_ENTRENCH ||
			cmdType == ACTION_COMMAND_FOLLOW					||
			cmdType == ACTION_COMMAND_FOLLOW_NOW			||
			cmdType == ACTION_COMMAND_UNLOAD					||
			cmdType == ACTION_COMMAND_WAIT_FOR_UNITS	||
			cmdType == ACTION_COMMAND_SWARM_ATTACK_UNIT ||
			cmdType == ACTION_COMMAND_ENTRENCH_SELF ||
			cmdType == ACTION_COMMAND_SWARM_ATTACK_OBJECT  ||
			cmdType == ACTION_MOVE_WAIT_FOR_TRUCKREPAIR ||
			cmdType == ACTION_MOVE_TO_NOT_PRESIZE ||
			cmdType == ACTION_COMMAND_ROTATE_TO_DIR ||
			cmdType == ACTION_COMMAND_CHANGE_SHELLTYPE ||
			cmdType == ACTION_COMMAND_STAND_GROUND ||
			cmdType == ACTION_COMMAND_MOVE_TO_GRID ||
			cmdType == ACTION_COMMAND_CAMOFLAGE_MODE ||
			cmdType == ACTION_COMMAND_ADAVNCED_CAMOFLAGE_MODE ||
			cmdType == ACTION_COMMAND_EXACT_SHOT ||
			cmdType == ACTION_MOVE_SELF_ENTRENCH ||
			cmdType == ACTION_COMMAND_MECH_ENTER||
			cmdType == ACTION_MOVE_MECH_ENTER_NOW ||
			cmdType == ACTION_MOVE_ONBOARD_ATTACK_UNIT ||
			cmdType == ACTION_COMMAND_WAIT  ||
			cmdType == ACTION_COMMAND_SUPPORT_FIRE  ||
			cmdType == ACTION_MOVE_BY_FORMATION ||
			cmdType == ACTION_COMMAND_HOLD_SECTOR ||
			cmdType == ACTION_COMMAND_PATROL
		);
}

IUnitState* CTankStatesFactory::ProduceState( class CQueueUnit *pObj, CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CTank*>( pObj ) != 0, "Wrong unit type" );
	CTank *pUnit = checked_cast<CTank*>( pObj );
	
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();	
	IUnitState* pResult = 0;
	bool bSwarmAttack = false;
	
	switch ( cmd.nCmdType )
	{	
	case ACTION_COMMAND_WAIT:
		pResult = CMechUnitRestState::Instance( pUnit, cmd.vPos, pUnit->GetDirection(), 0, cmd.fNumber );

		break;
	case ACTION_COMMAND_UNLOAD:
		{
			if ( pUnit->GetState()->GetName() == EUSN_MECHUNIT_REST_ON_BOARD )
			{
				CMechUnitInsideMechUnitState *pState ( checked_cast<CMechUnitInsideMechUnitState*>( pUnit->GetState() ) );
				pState->Unload( pUnit->GetCenterPlain() );
				pResult = pState;
			}
			else
			{
				CCommonUnit * pUnitToUnload = checked_cast<CCommonUnit*>( GetObjectByCmd( cmd ) );
				if ( pUnitToUnload )
				{
					if ( pUnitToUnload->IsFormation() )
					{
						EActionLeaveParam eLeaveParam = EActionLeaveParam ( int( cmd.fNumber ) );
						pResult = new CTransportLandState( pUnit, eLeaveParam, cmd.vPos, checked_cast<CFormation*>( GetObjectByCmd( cmd ) ) );
					}
					else // mech unit
					{
						if ( pUnitToUnload->GetState()->GetName() == EUSN_MECHUNIT_REST_ON_BOARD )
						{
							CMechUnitInsideMechUnitState *pState ( checked_cast<CMechUnitInsideMechUnitState*>( pUnitToUnload->GetState() ) );
							if ( pState )
								pState->Unload( pUnit->GetCenterPlain() );
						}
						pResult = 0;
					}
				}
				else
					pResult = new CTransportLandState( pUnit, ALP_POSITION_VALID, cmd.vPos, checked_cast<CFormation*>( GetObjectByCmd( cmd ) ) );
			}
		}

		break;
	case ACTION_MOVE_ONBOARD_ATTACK_UNIT:
		{
			CObjectBase * pObj = GetObjectByCmd( cmd );
			CMechUnitInsideMechUnitState *pState ( checked_cast<CMechUnitInsideMechUnitState*>( pUnit->GetState() ) );
			if ( pObj )
			{
				CAIUnit * pEnemy = checked_cast<CAIUnit*>( pObj );
				pState->AttackTarget( pEnemy );
			}
			pResult = pState;
		}

		break;
	case ACTION_COMMAND_MECH_ENTER:
	case ACTION_MOVE_MECH_ENTER_NOW:
		{
			CObjectBase * pObj = GetObjectByCmd( cmd );
			if ( pObj )
			{
				CMilitaryCar * pTransport = checked_cast<CMilitaryCar*>( pObj );
				pResult = new CMechUnitInsideMechUnitState( pUnit, pTransport, cmd.nCmdType == ACTION_MOVE_MECH_ENTER_NOW );
			}
		}
		break;
	
	case ACTION_MOVE_TO_NOT_PRESIZE:
		pResult = CMoveToPointNotPresize::Instance( pUnit, cmd.vPos, cmd.fNumber );

		break;

	case ACTION_MOVE_WAIT_FOR_TRUCKREPAIR:
		pResult = CMechUnitRestState::Instance( pUnit, cmd.vPos, cmd.fNumber, 1, -1 );

		break;
	case ACTION_COMMAND_DIE:
		NI_ASSERT( false, "Command to die in the queue" );

		break;
	case ACTION_MOVE_SELF_ENTRENCH:
		pResult = CMechUnitEntrenchSelfState::Instance( pUnit );
		
		break;
	case ACTION_MOVE_LEAVE_SELF_ENTRENCH:
		pUnit->ResetHoldSector();
		pResult = CTankPitLeaveState::Instance( pUnit );

		break;
	case ACTION_COMMAND_MOVE_TO:
	case ACTION_COMMAND_REVERSE_TO:
		{
			pUnit->UnsetFollowState();

			if ( pUnit->IsTrackDamaged() )
				pUnit->SendAcknowledgement( pCommand, ACK_CANNOT_MOVE_TRACK_DAMAGED, !pCommand->IsFromAI() );
			else
			{
				pUnit->ResetHoldSector();
				// The part: pUnit->GetPlayer() != theDipl.GetMyNumber() likely causes ASYNC in MP!
				// The decent way to handle it is to simply disable AI/player checks in MP - any unit will move out of a tank pit!
				if (pUnit->IsInTankPit() && (theDipl.IsNetGame() || ( !theDipl.IsNetGame() && ( !pCommand->IsFromAI() || pUnit->GetPlayer() != theDipl.GetMyNumber() ) ) ) )// сначала выйти из TankPit, потом поехать куда послали
				{
					theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pUnit );
					//theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, float(int(PARAM_ABILITY_OFF)), false ), pUnit );
					theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pUnit );
				}
				else
					pResult = CSoldierMoveToState::Instance( pUnit, cmd.vPos, cmd.nCmdType == ACTION_COMMAND_REVERSE_TO );
			}
		}
		break;
	case ACTION_COMMAND_ENTRENCH_SELF:
		pResult = CMechUnitEntrenchSelfState::Instance( pUnit );

		break;
	case ACTION_COMMAND_TRACK_TARGETING:
		{
			const NDb::ESpecialAbilityParam action = NDb::ESpecialAbilityParam(int(cmd.fNumber));
			if ( action != PARAM_ABILITY_AUTOCAST_ON &&
				action != PARAM_ABILITY_AUTOCAST_OFF )
			{
				pUnit->SetTrackTargeting( true );
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ATTACK_UNIT, cmd.nObjectID ), pUnit );
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
					theGroupLogic.InsertUnitCommand( newCmd, pUnit );
					theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ROTATE_TO, cmd.vPos ), pUnit );
					if ( pUnit->IsInTankPit() )
						theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pUnit );
				}
				break;
			case 1:
				pUnit->ResetHoldSector();
				break;
			case 2:
				pResult = CSoldierEnterHoldSectorState::Instance( pUnit );
				break;
			default:
				NI_ASSERT( false, "Wrong param to ACTION_COMMAND_HOLD_SECTOR" );
				break;
		}
		break;
	case ACTION_COMMAND_SWARM_ATTACK_UNIT:
		bSwarmAttack = true;
	case ACTION_COMMAND_ATTACK_UNIT:
		{
			if ( GetObjectByCmd( cmd ) && IsValid( GetObjectByCmd( cmd ) ) )
			{
				CONVERT_OBJECT( CAIUnit, pTarget, GetObjectByCmd( cmd ), "Wrong unit to attack" );

				if ( pTarget->IsAlive() )
				{
					if ( pTarget->GetStats()->IsInfantry() && checked_cast<CSoldier*>(pTarget)->IsInBuilding() )
						pResult = CSoldierAttackUnitInBuildingState::Instance( pUnit, checked_cast<CSoldier*>(pTarget), cmd.fNumber == 0, bSwarmAttack );
					else
						pResult = CMechAttackUnitState::Instance( pUnit, checked_cast<CAIUnit*>( GetObjectByCmd( cmd ) ), cmd.fNumber == 0, bSwarmAttack );
				}
			}
			else
				pUnit->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, !pCommand->IsFromAI() );
		}

		break;
	case ACTION_COMMAND_SWARM_ATTACK_OBJECT:
		bSwarmAttack = true;
	case ACTION_COMMAND_ATTACK_OBJECT:
		{
			pUnit->ResetHoldSector();
			CONVERT_OBJECT( CStaticObject, pStaticObj, GetObjectByCmd( cmd ), "Wrong object to attack" );
			// attack the artillery
			if ( pStaticObj->GetObjectType() == ESOT_ARTILLERY_BULLET_STORAGE )
			{
				pCommand->ToUnitCmd().nCmdType = bSwarmAttack ? ACTION_COMMAND_SWARM_ATTACK_UNIT : ACTION_COMMAND_ATTACK_UNIT;
				pCommand->ToUnitCmd().nObjectID = checked_cast<CArtilleryBulletStorage*>(pStaticObj)->GetOwner()->GetUniqueId();
				pCommand->ToUnitCmd().fNumber = 0;
				pResult = ProduceState( pObj, pCommand );
			}
			else
				pResult = CSoldierAttackCommonStatObjState::Instance( pUnit, pStaticObj, bSwarmAttack );
		}

		break;

	case ACTION_COMMAND_ROTATE_TO_DIR:
	case ACTION_COMMAND_ROTATE_TO:
		if ( pUnit->IsTrackDamaged() )
		{
			pUnit->SendAcknowledgement( pCommand, ACK_CANNOT_MOVE_TRACK_DAMAGED, !pCommand->IsFromAI() );
		}
		else if ( !pUnit->NeedDeinstall() )
		{
			pUnit->ResetHoldSector();
			if (pUnit->IsInTankPit() && (theDipl.IsNetGame() || ( !theDipl.IsNetGame() && ( !pCommand->IsFromAI() || pUnit->GetPlayer() != theDipl.GetMyNumber() ) ) ) ) // сначала выйти из TankPit, потом поехать куда послали
			{
				theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pUnit );
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pUnit );
				//theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, float(int(PARAM_ABILITY_OFF)), false ), pUnit );
			}
			else
			{
				if ( cmd.nCmdType == ACTION_COMMAND_ROTATE_TO )
					pResult = CSoldierTurnToPointState::Instance( pUnit, cmd.vPos );
				else
				{
					CVec2 vDir = cmd.vPos;
					Normalize( &vDir );
					pResult = CSoldierTurnToPointState::Instance( pUnit, pUnit->GetCenterPlain() + vDir );
				}
			}
		}

		break;
	case ACTION_MOVE_BY_DIR:
		if ( !pUnit->NeedDeinstall() )
			pResult = CSoldierMoveByDirState::Instance( pUnit, cmd.vPos );

		break;
	case ACTION_COMMAND_SWARM_TO:
		if ( pUnit->IsTrackDamaged() )
			pUnit->SendAcknowledgement( pCommand, ACK_CANNOT_MOVE_TRACK_DAMAGED, !pCommand->IsFromAI() );
		else 
		{
			pUnit->ResetHoldSector();
			if (pUnit->IsInTankPit() && (theDipl.IsNetGame() || ( !theDipl.IsNetGame() && ( !pCommand->IsFromAI() || pUnit->GetPlayer() != theDipl.GetMyNumber() ) ) ) ) // сначала выйти из TankPit, потом поехать куда послали
			{
				theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pUnit );
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pUnit );
				//theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, float(int(PARAM_ABILITY_OFF)), false ), pUnit );
			}
			else				
				pResult = CCommonSwarmState::Instance( pUnit, cmd.vPos, cmd.fNumber );
		}
		
		break;
	case ACTION_COMMAND_SUPPORT_FIRE:
		{
			if ( !pUnit->GetFirstArtilleryGun() )
			{
				NI_ASSERT( false, "Support Fire ability given to non-artillery unit" );
				break;
			}
			CONVERT_OBJECT( CAIUnit, pTarget, GetObjectByCmd( cmd ), "Wrong unit to support" );
			if ( pTarget )
				pResult = CMechUnitSupportFireState::Instance( pUnit, pTarget );
		}
		break;
	case ACTION_COMMAND_GUARD:
		pResult = CMechUnitRestState::Instance( pUnit, cmd.vPos, cmd.fNumber, 0, -1 );

		break;
	/*case ACTION_COMMAND_AMBUSH:					//Removed: ambush is now an ability (via Executor), not a state
		pResult = CCommonAmbushState::Instance( pUnit );
		
		break;*/
	case ACTION_COMMAND_ART_BOMBARDMENT:
	case ACTION_COMMAND_FIRE_ROCKETS:
		if ( pUnit->GetFirstArtilleryGun() != 0 )
		{ 
			if ( pUnit->GetFirstArtilleryGun()->CanShootToPointWOMove( cmd.vPos, 0.0f ) )
			{
				// FIRE_ROCKETS is deliberately finite even for non-rocket artillery weapons.
				const int nBurstCount = cmd.nCmdType == ACTION_COMMAND_FIRE_ROCKETS ? 1 : int( cmd.fNumber );
				pResult = CArtilleryBombardmentState::Instance( pUnit, cmd.vPos, nBurstCount );
			}
			else
				pUnit->SendAcknowledgement( pCommand, pUnit->GetFirstArtilleryGun()->GetRejectReason(), !pCommand->IsFromAI() );
		}

		break;
	case ACTION_COMMAND_RANGE_AREA:
		if ( pUnit->GetFirstArtilleryGun() != 0 && pUnit->GetFirstArtilleryGun()->CanShootToPointWOMove( cmd.vPos, 0.0f ) )
			pResult = CArtilleryRangeAreaState::Instance( pUnit, cmd.vPos );

		break;
	case ACTION_COMMAND_FOLLOW:
		{
			CONVERT_OBJECT( CCommonUnit, pUnitToFollow, GetObjectByCmd( cmd ), "Wrong unit to follow" );
			if ( pUnit->IsTrackDamaged() )
				pUnit->SendAcknowledgement( pCommand, ACK_CANNOT_MOVE_TRACK_DAMAGED, !pCommand->IsFromAI() );
			else
			{
				pUnit->ResetHoldSector();
				if (pUnit->IsInTankPit() && (theDipl.IsNetGame() || ( !theDipl.IsNetGame() && ( !pCommand->IsFromAI() || pUnit->GetPlayer() != theDipl.GetMyNumber() ) ) ) ) // сначала выйти из TankPit, потом поехать куда послали
				{
					theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pUnit );
					theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_LEAVE_SELF_ENTRENCH ), pUnit );
					//theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, float(int(PARAM_ABILITY_OFF)), false ), pUnit );
				}
				else
					pUnit->SetFollowState( pUnitToFollow );
			}
		}

		break;
	case ACTION_COMMAND_FOLLOW_NOW:
		{
			CONVERT_OBJECT( CCommonUnit, pUnitToFollow, GetObjectByCmd( cmd ), "Not common unit in follow command" );
			pResult = CFollowState::Instance( pUnit, pUnitToFollow );
		}

		break;
	case ACTION_COMMAND_WAIT_FOR_UNITS:
		{
			CONVERT_OBJECT( CFormation, pFormationToWait, GetObjectByCmd( cmd ), "Wrong unit to wait" );
			pResult = CTransportWaitPassengerState::Instance( pUnit, pFormationToWait );
		}

		break;
	case ACTION_COMMAND_CHANGE_SHELLTYPE:
		if ( pUnit->GetFirstArtilleryGun() != 0 )
			pUnit->SetActiveShellType( static_cast<NDb::SWeaponRPGStats::SShell::EShellDamageType>( int(cmd.fNumber)) );

		break;
	case ACTION_COMMAND_STAND_GROUND:
		pUnit->Stop();
		pUnit->UnsetFollowState();				
		pUnit->SetBehaviourMoving( SBehaviour::EMHoldPos );

		break;
	case ACTION_COMMAND_MOVE_TO_GRID:
		pResult = CCommonMoveToGridState::Instance( pUnit, cmd.vPos, GetVectorByDirection( cmd.fNumber ) );

		break;
	case ACTION_COMMAND_CAMOFLAGE_MODE:
	case ACTION_COMMAND_ADAVNCED_CAMOFLAGE_MODE:
		break;
	case ACTION_MOVE_BY_FORMATION:
		pResult = CMoveByFormationState::Instance( pUnit, cmd.nObjectID );
		break;
	case ACTION_COMMAND_EXACT_SHOT:
		{
			const NDb::ESpecialAbilityParam action = NDb::ESpecialAbilityParam(int(cmd.fNumber));
			if ( action != PARAM_ABILITY_AUTOCAST_ON &&
				action != PARAM_ABILITY_AUTOCAST_OFF )
			{
				// Set IgnoreAABB on
				pUnit->SetIgnoreAABBCoeff( true );

				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ATTACK_UNIT, cmd.nObjectID ), pUnit );
			}
		}
		break;
	case ACTION_COMMAND_PATROL:
		{
			CVec2 vTarget( cmd.vPos );
			if ( pUnit->CanMove() && !pUnit->IsTrackDamaged() )
				pResult = CCommonPatrolState::Instance( pUnit, vTarget );
			else
			{
				if ( pUnit->IsTrackDamaged() )
				{
					pUnit->SendAcknowledgement( pCommand, ACK_CANNOT_MOVE_TRACK_DAMAGED, !pCommand->IsFromAI() );
				}
				else
				{
					pUnit->SendAcknowledgement( pCommand, ACK_NEGATIVE, !pCommand->IsFromAI() );
				}
			}
		}
		break;
	default:
		NI_ASSERT( false, "Wrong command" );
	}

	return pResult;
}

IUnitState* CTankStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	return CMechUnitRestState::Instance( checked_cast<CTank*>( pUnit ), CVec2( -1, -1 ), 0, 0, -1 );
}

//*******************************************************************
//*										CMechUnitSupportFireState											*
//*******************************************************************

IUnitState* CMechUnitSupportFireState::Instance( CAIUnit *pUnit, CAIUnit *pFriend )
{
	return new CMechUnitSupportFireState( pUnit, pFriend );
}

CMechUnitSupportFireState::CMechUnitSupportFireState( CAIUnit *_pUnit, CAIUnit *_pFriend )
: CStatusUpdatesHelper( EUS_SUPPORT_FIRE, _pUnit ), pUnit( _pUnit ), eState( ESFS_WAITING ), bFinish( false ), pFriend( _pFriend ), 
pGun( pUnit->GetFirstArtilleryGun() ), pFriendFormation( 0 )
{
	pUnit->Stop();

	bool bCanShootWOMove = pGun->CanShootToPointWOMove( GetPurposePoint(), 0 );
	bool bNeedTurn = false;

	if ( !bCanShootWOMove && pGun->GetRejectReason() == ACK_NOT_IN_ATTACK_ANGLE )
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
		{
			eState = ESFS_WAITING;
		}
		else
		{
			eState = ESFS_TURNING;
		}
	}
	else
	{
		pUnit->SendAcknowledgement( pUnit->GetFirstArtilleryGun()->GetRejectReason(), true );
		const EUnitAckType eReject = pUnit->GetFirstArtilleryGun()->GetRejectReason();
		pUnit->SendAcknowledgement( eReject );
		if ( eReject == ACK_NO_AMMO && pUnit->GetPlayer() == theDipl.GetMyNumber() )
			theFeedBackSystem.AddFeedbackAndForget( pUnit->GetUniqueID(), pUnit->GetCenterPlain(), EFB_NO_AMMO, -1 );

		pUnit->SetCommandFinished();
		bFinish = true;
	}

	if ( bNeedTurn && !pUnit->CheckTurn( 1.0f, GetPurposePoint() - pUnit->GetCenterPlain(), false, false ) )
	{
		pUnit->SendAcknowledgement( ACK_NEGATIVE, true );
		pUnit->SetCommandFinished();
		bFinish = true;
	}

	if ( pFriend->IsInfantry() )	// for infantry, remember formation
	{
		pFriendFormation = pFriend->GetFormation();
	}

	if ( !bFinish )
	{
		pGun->LockInCurAngle();
		lastCheck = curTime;

		// Determine search circle radius
		fSearchRadius = pFriend->GetSightRadius();

		// Apply bonus
		const NDb::SUnitSpecialAblityDesc *pSA = pUnit->GetUnitAbilityDesc( NDb::ABILITY_SUPPORT_FIRE );
		if ( pSA )
		{
			if( pSA->pStatsBonus )
				pUnit->ApplyStatsModifier( pSA->pStatsBonus, true );
		}
	}
}

void CMechUnitSupportFireState::CheckArea()
{
	// по юнитам
	pUnit->ResetShootEstimator( 0, false, pUnit->GetForbiddenGuns() );
	for ( CUnitsIter<1,2> iter( pUnit->GetParty(), EDI_ENEMY, pFriend->GetCenterPlain(), fSearchRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pTarget = *iter;
		if ( pTarget->IsVisible( pUnit->GetParty() ) &&
			fabs2( pTarget->GetCenterPlain() - pFriend->GetCenterPlain() ) <= sqr( fSearchRadius ) &&
			pGun->CanShootToUnitWOMove( pTarget ) )
			pUnit->AddUnitToShootEstimator( pTarget );
	}

	// враг найден
	if ( pEnemy = pUnit->GetBestShootEstimatedUnit() )
	{
		eState = ESFS_SHOOT_UNIT;
		pGun->StartEnemyBurst( pEnemy, true );
	}
	// по объектам
	else
	{
		for ( CStObjCircleIter<true> iter( pFriend->GetCenterPlain(), fSearchRadius ); !iter.IsFinished() && pObj == 0; iter.Iterate() )
		{
			CExistingObject *pIteratingObject = *iter;
			if ( pIteratingObject->GetObjectType() == ESOT_BUILDING && theDipl.GetDiplStatus( pIteratingObject->GetPlayer(), pUnit->GetPlayer() ) == EDI_ENEMY && 
				pIteratingObject->IsVisible( pUnit->GetParty() ) && pIteratingObject->GetNDefenders() > 0 &&
				fabs2( pIteratingObject->GetAttackCenter( pFriend->GetCenterPlain() ) - pFriend->GetCenterPlain() ) <= sqr( fSearchRadius ) && pGun->CanShootToObjectWOMove( pIteratingObject ) )
			{
				pObj = pIteratingObject;
			}
		}

		if ( pObj != 0 )
		{
			eState = ESFS_SHOOT_OBJECT;
			pGun->StartPointBurst( pObj->GetAttackCenter( pUnit->GetCenterPlain() ), true );
		}
	}
}

void CMechUnitSupportFireState::Segment()
{
	InitStatus();
	if ( !pFriend->IsAlive() && pFriendFormation && pFriendFormation->IsAlive() )			// Infantry unit dead, select next in formation
	{
		pFriend = pFriendFormation->operator[]( 0 );
	}

	if ( pUnit->IsOperable() && ( eState == ESFS_TURNING || pUnit->CanShoot() ) )
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
				case ESFS_TURNING:
					if ( pUnit->IsInTankPit() )
					{
						theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pUnit );
						return;
					}

					if ( pUnit->TurnToTarget( pFriend->GetCenterPlain() ) )
					{
						eState = ESFS_WAITING;
					}

					break;
				case ESFS_WAITING:
					{
						// If supported unit has moved, move accordingly
						if ( !pGun->CanShootToUnitWOMove( pFriend ) )
						{
							eState = ESFS_TURNING;
						}
						else
						{
							// Look for targets
							if ( curTime - lastCheck >= CHECK_TIME )
								CheckArea();
						}
					}
					break;
				case ESFS_SHOOT_UNIT:
					if ( IsValidObj( pEnemy ) && pEnemy->IsVisible( pUnit->GetParty() ) && 
						pGun->CanShootToUnitWOMove( pEnemy ) && fabs2( pEnemy->GetCenterPlain() - pFriend->GetCenterPlain() ) <= sqr( fSearchRadius ) )
						pGun->StartEnemyBurst( pEnemy, false );
					else
					{
						eState = ESFS_WAITING;
						lastCheck = curTime - CHECK_TIME;
						pEnemy = 0;
					}

					break;
				case ESFS_SHOOT_OBJECT:
					if ( IsValidObj( pObj ) && theDipl.GetDiplStatus( pObj->GetPlayer(), pUnit->GetPlayer() ) == EDI_ENEMY )
						pGun->StartPointBurst( pObj->GetAttackCenter( pUnit->GetCenterPlain() ), false );
					else
					{
						eState = ESFS_WAITING;
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
			}
		}
	}
}

void CMechUnitSupportFireState::FinishCommand()
{
	if ( IsValid( pGun ) )
		pGun->StopFire();

	pGun->UnlockCurAngle();
	pUnit->SetCommandFinished();

	// Remove bonus
	const NDb::SUnitSpecialAblityDesc *pSA = pUnit->GetUnitAbilityDesc( NDb::ABILITY_SUPPORT_FIRE );
	if ( pSA )
	{
		if( pSA->pStatsBonus )
			pUnit->ApplyStatsModifier( pSA->pStatsBonus, false );
	}
}

ETryStateInterruptResult CMechUnitSupportFireState::TryInterruptState( class CAICommand *pCommand )
{
	if ( pCommand == 0 || !IsValid( pGun ) || !pGun->IsBursting() )
	{
		FinishCommand();
		return TSIR_YES_IMMIDIATELY;
	}
	bFinish = true;
	return TSIR_YES_WAIT;
}

bool CMechUnitSupportFireState::IsAttacksUnit() const
{
	return eState == ESFS_SHOOT_UNIT;
}

CAIUnit* CMechUnitSupportFireState::GetTargetUnit() const
{
	if ( IsAttacksUnit() )
		return pEnemy;
	else
		return 0;
}

REGISTER_SAVELOAD_CLASS( 0x1915D340, CMechUnitSupportFireState );


