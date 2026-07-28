#include "stdafx.h"

#include "Stats_B2_M1/AnimationFromAction.h"
#include "SoldierStates.h"
#include "Commands.h"
#include "PathFinder.h"
#include "Entrenchment.h"
#include "Building.h"
#include "InTransportStates.h"
#include "InEntrenchmentStates.h"
#include "InBuildingStates.h"
#include "GroupLogic.h"
#include "NewUpdater.h"
#include "Common_RTS_AI/AIMap.h"
#include "AntiArtilleryManager.h"
#include "Diplomacy.h"
#include "Soldier.h"
#include "Formation.h"
#include "Turret.h"
#include "Aviation.h"
#include "UnitCreation.h"
#include "UnitGuns.h"
#include "ParatrooperPath.h"
#include "General.h"
#include "ArtilleryBulletStorage.h"
#include "StaticObjectsIters.h"
#include "ExecutorContainer.h"
#include "Common_RTS_AI/PathFinder.h"
#include "Stats_B2_M1/DBVisObj.h"
#include "Stats_B2_M1/AbilityActions.h"
// for profiling
#include "TimeCounter.h"
#include "AAFeedBacks.h"
#include "Common_RTS_AI/StandartDirPath.h"
#include "Artillery.h"
#include "System/Commands.h"
#include "GlobalWarFog.h"

extern CAAFeedBacks theAAFeedBacks;
extern CSupremeBeing theSupremeBeing;
extern CUnitCreation theUnitCreation;
extern CEventUpdater updater;
extern NTimer::STime curTime;
extern CStaticObjects theStatObjs;
extern CAntiArtilleryManager theAAManager;
extern CGroupLogic theGroupLogic;
extern CDiplomacy theDipl;
extern CExecutorContainer theExecutorContainer;
extern NAI::CTimeCounter timeCounter;
extern CGlobalWarFog theWarFog;

static float g_fRotateStatusWaitTime = 0.0f;
extern bool g_bAgressiveMovement;

START_REGISTER(StatusUpdateVars)
REGISTER_VAR_EX( "rotate_status_wait_time", NGlobal::VarFloatHandler, &g_fRotateStatusWaitTime, 0.0f, STORAGE_NONE );
FINISH_REGISTER

//*******************************************************************
//*										  CSoldierStatesFactory												*
//*******************************************************************

CPtr<CSoldierStatesFactory> CSoldierStatesFactory::pFactory = 0;

IStatesFactory* CSoldierStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CSoldierStatesFactory();

	return pFactory;
}

bool CSoldierStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		( cmdType == ACTION_COMMAND_DIE								||
			cmdType == ACTION_COMMAND_MOVE_TO						||
			cmdType == ACTION_COMMAND_ATTACK_UNIT				||
			cmdType == ACTION_COMMAND_ATTACK_OBJECT			||
			cmdType == ACTION_COMMAND_ROTATE_TO					||
			cmdType == ACTION_MOVE_BY_DIR								||
			cmdType == ACTION_COMMAND_ENTER							||
			cmdType == ACTION_COMMAND_IDLE_BUILDING			||
			cmdType == ACTION_COMMAND_IDLE_TRENCH				||
			cmdType == ACTION_COMMAND_SWARM_TO					||
			cmdType == ACTION_COMMAND_PARADE						||
			cmdType == ACTION_COMMAND_PLACEMINE_NOW			||
			cmdType == ACTION_COMMAND_CLEARMINE_RADIUS	||
			cmdType == ACTION_COMMAND_GUARD							||
//			cmdType == ACTION_COMMAND_AMBUSH						||
			cmdType == ACTION_COMMAND_ENTER_TRANSPORT_NOW ||
			cmdType == ACTION_COMMAND_IDLE_TRANSPORT		||
			cmdType == ACTION_COMMAND_DISAPPEAR ||
			cmdType == ACTION_MOVE_PARACHUTE ||
			cmdType == ACTION_COMMAND_USE_SPYGLASS ||
			cmdType == ACTION_MOVE_IDLE ||
			cmdType == ACTION_COMMAND_FOLLOW ||
			cmdType == ACTION_COMMAND_FOLLOW_NOW ||
			cmdType == ACTION_COMMAND_SNEAK_ON ||
			cmdType == ACTION_COMMAND_SNEAK_OFF ||
			cmdType == ACTION_COMMAND_FORM_FORMATION ||
			cmdType == ACTION_COMMAND_SWARM_ATTACK_UNIT ||
			cmdType == ACTION_COMMAND_SWARM_ATTACK_OBJECT ||
			cmdType == ACTION_COMMAND_ROTATE_TO_DIR ||
			cmdType == ACTION_COMMAND_USE ||
			cmdType == ACTION_COMMAND_STAND_GROUND ||
			cmdType == ACTION_COMMAND_MOVE_TO_GRID ||
			cmdType == ACTION_COMMAND_THROW_GRENADE ||
			cmdType == ACTION_COMMAND_THROW_ANTITANK_GRENADE ||
			cmdType == ACTION_COMMAND_SPY_MODE ||
			cmdType == ACTION_COMMAND_ENTRENCH_SELF ||
			cmdType == ACTION_MOVE_SELF_ENTRENCH ||
			cmdType == ACTION_MOVE_LEAVE_SELF_ENTRENCH
		);
}

IUnitState* CSoldierStatesFactory::ProduceState( class CQueueUnit *pObj, CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CAIUnit*>(pObj) != 0, "Wrong unit passed" );
	CAIUnit *pUnit = checked_cast<CAIUnit*>(pObj);
	
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	IUnitState* pResult = 0;

	bool bSwarmAttack = false;
	
	switch ( cmd.nCmdType )
	{
	case ACTION_MOVE_SELF_ENTRENCH:
	case ACTION_COMMAND_ENTRENCH_SELF:
		pResult = new CSoldierEntrenchSelfState( checked_cast<CSoldier*>( pUnit ) );

		break;
	case ACTION_MOVE_LEAVE_SELF_ENTRENCH:
		pResult = new CSoldierLeaveSelfEntrenchState( pUnit );

		break;
	case ACTION_COMMAND_DIE:
		NI_ASSERT( false, "Command to die in the queue" );

		break;
	case ACTION_COMMAND_SNEAK_OFF:
		{
			CONVERT_OBJECT( CSniper, pSniper, pUnit, "Not sniper passed" );				
			SExecutorEventParam par( EID_ABILITY_DEACTIVATE, 0, pSniper->GetUniqueId() );
			CExecutorEventSpecialAbilityDeactivate event( par, ABILITY_CAMOFLAGE_MODE );
			pTheExecutorsContainer->RaiseEvent( event );
		}
/*	case ACTION_COMMAND_AMBUSH:
		pResult = CCommonAmbushState::Instance( pUnit );
		
		break;*/
	case ACTION_MOVE_IDLE:
		pResult = CSoldierIdleState::Instance( pUnit );

		break;
	case ACTION_COMMAND_SPY_MODE:
		{
			const NDb::ESpecialAbilityParam action = NDb::ESpecialAbilityParam( int( cmd.fNumber ) );
			if ( action != PARAM_ABILITY_ON )
				break;

			auto pTargetUnit = dynamic_cast<CAIUnit*>(CLinkObject::GetObjectByUniqueId( cmd.nObjectID ));
			if ( !IsValid( pTargetUnit ) )
				break;
			
			CDBPtr<NDb::SHPObjectRPGStats> pTargetStats = pTargetUnit->GetStats();
			if ( !pTargetStats )
				break;

			SExecutorEventParam par( EID_ABILITY_SET_TARGET, 0, pUnit->GetUniqueId() );
			CExecutorEventSpecialAbilitySetTarget event( par, NDb::ABILITY_SPY_MODE, pTargetStats );
			theExecutorContainer.RaiseEvent( event );
		}
		break;
	case ACTION_COMMAND_USE_SPYGLASS:
		{
			CONVERT_OBJECT( CSoldier, pSoldier, pUnit, "ACTION_COMMAND_USE_SPYGLASS: not soldier passed" );
			pResult = CSoldierUseSpyglassState::Instance( pSoldier, cmd.vPos );
		}

		break;
	case ACTION_MOVE_PARACHUTE:
		{
			CONVERT_OBJECT( CSoldier, pSoldier, pUnit, "ACTION_MOVE_PARACHUTE: not soldier passed" );
			pResult = CSoldierParaDroppingState::Instance( pSoldier, checked_cast<CAviation*>( GetObjectByCmd( cmd ) ) );
		}

		break;
	case ACTION_COMMAND_MOVE_TO:
		pUnit->UnsetFollowState();
		pResult = CSoldierMoveToState::Instance( pUnit, cmd.vPos );

		break;
	case ACTION_COMMAND_SWARM_ATTACK_UNIT:
		bSwarmAttack = true;
	case ACTION_COMMAND_ATTACK_UNIT:
		{
			CONVERT_OBJECT( CAIUnit, pTarget, GetObjectByCmd( cmd ), "Wrong unit to attack" );

			if ( IsValidObj( pTarget ) )
			{
				if ( pTarget->GetStats()->IsInfantry() && checked_cast<CSoldier*>(pTarget)->IsInBuilding() )
					pResult = CSoldierAttackUnitInBuildingState::Instance( pUnit, checked_cast<CSoldier*>(pTarget), cmd.fNumber == 0, bSwarmAttack );
				else
				{
					if ( pTarget->GetStats()->IsInfantry() && checked_cast<CSoldier*>(pUnit)->IsInEntrenchment() )
						pResult = CSoldierAttackInEtrenchState::Instance( checked_cast<CSoldier*>(pUnit), checked_cast<CAIUnit*>( pTarget ), bSwarmAttack );
					else
						pResult = CSoldierAttackState::Instance( pUnit, checked_cast<CAIUnit*>(pTarget), cmd.fNumber == 0, bSwarmAttack, false );
				}
			}
			else
				pUnit->SendAcknowledgement( ACK_INVALID_TARGET );
		}

		break;
	case ACTION_COMMAND_SWARM_ATTACK_OBJECT:
		bSwarmAttack = true;
	case ACTION_COMMAND_ATTACK_OBJECT:
		{
			CONVERT_OBJECT( CStaticObject, pStaticObj, GetObjectByCmd( cmd ), "Wrong static object to attack" );

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
	case ACTION_COMMAND_THROW_GRENADE:
	case ACTION_COMMAND_THROW_ANTITANK_GRENADE:	
		{
			switch( (const EActionThrowGrenadeParam)(int)(cmd.nNumber) ) 
			{
			case ATGP_ATACK_UNIT:
				{
					CONVERT_OBJECT( CAIUnit, pTarget, GetObjectByCmd( cmd ), "Wrong unit to attack" );

					if ( IsValidObj( pTarget ) )
					{
						checked_cast<CSoldier*>( pUnit )->SetGrenadeFixed( true );
						if ( pTarget->GetStats()->IsInfantry() && checked_cast<CSoldier*>(pTarget)->IsInBuilding() )
							pResult = CSoldierAttackUnitInBuildingState::Instance( pUnit, checked_cast<CSoldier*>(pTarget), true, bSwarmAttack );
						else
						{
							if ( pTarget->GetStats()->IsInfantry() && checked_cast<CSoldier*>(pUnit)->IsInEntrenchment() )
								pResult = CSoldierAttackInEtrenchState::Instance( checked_cast<CSoldier*>(pUnit), checked_cast<CAIUnit*>( pTarget ), bSwarmAttack );
							else
								pResult = CSoldierAttackState::Instance( pUnit, checked_cast<CAIUnit*>(pTarget), true, bSwarmAttack, false );
						}
					}
					else
						pUnit->SendAcknowledgement( ACK_INVALID_TARGET );
				}
				break;
			case ATGP_ATACK_OBJECT:
				{
					CONVERT_OBJECT( CStaticObject, pStaticObj, GetObjectByCmd( cmd ), "Wrong static object to attack" );

					checked_cast<CSoldier*>( pUnit )->SetGrenadeFixed( true );
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
			case ATGP_ATACK_POINT:
				break;
			}
		}
		break;
	case ACTION_COMMAND_ROTATE_TO:
		pResult = CSoldierTurnToPointState::Instance( pUnit, cmd.vPos );

		break;
	case ACTION_COMMAND_ROTATE_TO_DIR:
		{
			CVec2 vDir = cmd.vPos;
			Normalize( &vDir );
			pResult = CSoldierTurnToPointState::Instance( pUnit, pUnit->GetCenterPlain() + vDir );
		}

		break;
	case ACTION_COMMAND_ENTER:
		if ( cmd.fNumber == 0 )
		{
			CONVERT_OBJECT( CBuilding, pBuilding, GetObjectByCmd( cmd ), "Wrong static object is passed, command enter to building" );
			pResult = CSoldierEnterState::Instance( pUnit, pBuilding );
		}
		else if ( cmd.fNumber == 1 )
		{
			CONVERT_OBJECT( CEntrenchment, pEntrenchment, GetObjectByCmd( cmd ), "Wrong static object is passed, command enter to entrenchment" );
			pResult = CSoldierEnterEntrenchmentState::Instance( pUnit, pEntrenchment );
		}
		else if  ( cmd.fNumber == 2 )
		{
			CONVERT_OBJECT( CEntrenchmentPart, pEntrenchmentPart, GetObjectByCmd( cmd ), "Wrong static object is passed, command enter to entrenchment part" );
			pResult = CSoldierEnterEntrenchmentState::Instance( pUnit, pEntrenchmentPart->GetOwner() );
		}
		else
			NI_ASSERT( false, StrFmt( "Wrong number %g in command Enter", cmd.fNumber ) );
		
		break;
	case ACTION_COMMAND_IDLE_BUILDING:
		{
			CONVERT_OBJECT( CBuilding, pBuilding, GetObjectByCmd( cmd ), "Wrong object to enter" );
			CONVERT_OBJECT( CSoldier, pSoldier, pUnit, "Wrong unit passed: soldier expected" );
			pResult = CSoldierRestInBuildingState::Instance( pSoldier, pBuilding );
		}

		break;
	case ACTION_COMMAND_IDLE_TRENCH:
		{
			CONVERT_OBJECT( CEntrenchment, pEntrenchment, GetObjectByCmd( cmd ), "Wrong static object is passed, command enter to entrenchment" );
			CONVERT_OBJECT( CSoldier, pSoldier, pUnit, "Wrong unit passed: soldier expected" );
			pResult = CSoldierRestInEntrenchmentState::Instance( pSoldier, pEntrenchment );
		}

		break;
	case ACTION_COMMAND_SWARM_TO:
		pResult = CCommonSwarmState::Instance( pUnit, cmd.vPos, cmd.fNumber );

		break;
	case ACTION_COMMAND_PARADE:
		pResult = CSoldierParadeState::Instance( pUnit );

		break;
	case ACTION_COMMAND_PLACEMINE_NOW:
		pResult = CSoldierPlaceMineNowState::Instance( pUnit, cmd.vPos, static_cast<EMineType>(int(cmd.fNumber)) );

		break;
	case ACTION_COMMAND_CLEARMINE_RADIUS:
		pResult = CSoldierClearMineRadiusState::Instance( pUnit, cmd.vPos );

		break;
	case ACTION_COMMAND_GUARD:
		pResult = CSoldierRestState::Instance( pUnit );

		break;
	case ACTION_COMMAND_ENTER_TRANSPORT_NOW:
		{
			CONVERT_OBJECT( CMilitaryCar, pCar, GetObjectByCmd( cmd ), "Not transport passed to enter to" );
			pResult = CSoldierEnterTransportNowState::Instance( pUnit, pCar );
		}

		break;
	case ACTION_COMMAND_IDLE_TRANSPORT:
		{
			CONVERT_OBJECT( CMilitaryCar, pCar, GetObjectByCmd( cmd ), "Not transport passed to idle" );
			CONVERT_OBJECT( CSoldier, pSoldier, pUnit, "Wrong unit passed: soldier expected" );
			pResult = CSoldierRestOnBoardState::Instance( pSoldier, pCar );
		}

		break;
	case ACTION_COMMAND_FOLLOW:
		NI_ASSERT( dynamic_cast<CCommonUnit*>(GetObjectByCmd( cmd )) != 0, "Not common unit in follow command" );
		pUnit->SetFollowState( checked_cast<CCommonUnit*>(GetObjectByCmd( cmd )) );

		break;
	case ACTION_COMMAND_FOLLOW_NOW:
		{
			CONVERT_OBJECT( CCommonUnit, pUnitToFollowTo, GetObjectByCmd( cmd ), "Not common unit in follow command" );
			pResult = CFollowState::Instance( pUnit, pUnitToFollowTo );
		}

		break;
	case ACTION_COMMAND_FORM_FORMATION:
		if ( pUnit->IsInFormation() )
		{
			CONVERT_OBJECT( CSoldier, pSoldier, pUnit, "Wrong unit for FORM_FORMATION command" );
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_FORM_FORMATION ), pSoldier, false );
		}

		break;
	case ACTION_COMMAND_USE:
		NI_ASSERT( cmd.fNumber >= 0, StrFmt( "Wrong number of use animation (%d)", (int)cmd.fNumber ) );
		pResult = CSoldierUseState::Instance( pUnit, EActionNotify((int)cmd.fNumber) );

		break;
	case ACTION_COMMAND_STAND_GROUND:
		pUnit->Stop();
		pUnit->UnsetFollowState();				
		pUnit->SetBehaviourMoving( SBehaviour::EMHoldPos );

		break;
	case ACTION_COMMAND_MOVE_TO_GRID:
		pResult = CCommonMoveToGridState::Instance( pUnit, cmd.vPos, GetVectorByDirection( cmd.fNumber ) );

		break;
	default:
		NI_ASSERT( false, "Wrong command" );
	}

	return pResult;
}

IUnitState* CSoldierStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	CSoldier *pSoldier = checked_cast<CSoldier*>( pUnit );
	if ( pSoldier->IsInEntrenchment() )
		return CSoldierRestInEntrenchmentState::Instance( pSoldier, pSoldier->GetEntrenchment() );
	else
		return CSoldierRestState::Instance( pSoldier );
}

//*******************************************************************
//*											  CSoldierRestState													*
//*******************************************************************

IUnitState* CSoldierRestState::Instance( CAIUnit *pUnit )
{
	return new CSoldierRestState( pUnit );
}

CSoldierRestState::CSoldierRestState( CAIUnit *_pUnit )
: pUnit( _pUnit ), nextMove( 0 ), bScanned( false )
{
	if ( pUnit->GetStats()->etype != RPG_TYPE_SNIPER )
		pUnit->StartCamouflating();
	pUnit->ResetTargetScan();

	if ( pUnit->GetFormation() )
		guardPoint = pUnit->GetUnitPointInFormation();

	pUnit->Stop();
}

const bool CSoldierRestState::CheckGuardPoint() const
{
	if ( pUnit->GetLastPusherUnit() == 0 )
		return true;
	const SRect rect = pUnit->GetLastPusherUnit()->GetUnitRect();
	SRect modifiedRect;
	modifiedRect.InitRect( rect.center, rect.dir, rect.lengthAhead * 1.8f, rect.lengthBack * 1.4f, rect.width );
	return !modifiedRect.IsPointInside( guardPoint );
}

void CSoldierRestState::Segment()
{
	if ( curTime >= nextMove && !pUnit->GetArtilleryIfCrew() )
	{
		bool bTargetFound = false;
		
		if ( pUnit->GetFormation()->IsInWaitingState() )
		{
			guardPoint = pUnit->GetUnitPointInFormation();
			const bool bNeedFormationPoint =
				pUnit->CanMoveForGuard() &&
				CheckGuardPoint() &&
				fabs2( guardPoint - pUnit->GetCenterPlain() ) > 200.0f;

			// While returning to formation, allow only attacks that can be fired from the current position.
			const BYTE cResult =
				bNeedFormationPoint ?
				pUnit->AnalyzeTargetScanWithoutMoving( 0, false, false ) :
				pUnit->AnalyzeTargetScan( 0, false, false );

			if ( cResult & 2 )
				bScanned = true;

			bTargetFound = cResult & 1;
		}
		else
			bScanned = true;

		if ( !bTargetFound )
		{
			if ( bScanned && curTime >= nextMove && pUnit->CanMoveForGuard() && pUnit->IsIdle() && CheckGuardPoint() )
			{
				const CVec2 vUnitCenter = pUnit->GetCenterPlain();

				bool bSent = false;
				guardPoint = pUnit->GetUnitPointInFormation();
 				if ( fabs2( guardPoint - vUnitCenter ) > 200.0f )
				{
					const CVec2 vBestGuardPoint = GetAIMap()->GetTerrain()->GetValidPoint( pUnit->GetBoundTileRadius(), pUnit->GetCenterPlain(), guardPoint, pUnit->GetAIPassabilityClass(), false, GetAIMap() );
					CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( vBestGuardPoint, VNULL2, pUnit, false, GetAIMap() );

					if ( pStaticPath )
					{
						pUnit->SendAlongPath( pStaticPath, VNULL2, true );
						pUnit->UnRegisterAsBored( ACK_BORED_IDLE );
						bSent = true;
					}
				}

				if ( !bSent )
					pUnit->RegisterAsBored( ACK_BORED_IDLE );

				nextMove = curTime + NRandom::Random( 1500, 2000 ); RecordRandomCall();

				pUnit->FreezeByState( true );
			}
		}
	}
}

ETryStateInterruptResult CSoldierRestState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->UnRegisterAsBored( ACK_BORED_IDLE );
	pUnit->SetCommandFinished();
	pUnit->FreezeByState( false );
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										  CSoldierAttackState													*
//*******************************************************************

IUnitState* CSoldierAttackState::Instance( CAIUnit *pUnit, CAIUnit *pEnemy, bool bAim, const bool bSwarmAttack, const bool _bPreferGrenade )
{
	return new CSoldierAttackState( pUnit, pEnemy, bAim, bSwarmAttack, _bPreferGrenade );
}

CSoldierAttackState::CSoldierAttackState( CAIUnit *_pUnit, CAIUnit *_pEnemy, bool _bAim, const bool _bSwarmAttack, const bool _bPreferGrenade )
: pUnit( _pUnit ), pEnemy( _pEnemy ),
	nextShootCheck( 0 ), lastEnemyTile( -1, -1 ), bAim( _bAim ), wLastEnemyDir( 0 ), pGun( 0 ), bFinish( false ),
	lastEnemyCenter( -1, -1 ), bSwarmAttack( _bSwarmAttack ),
	runUpToEnemy( _pUnit, _pEnemy, false ), 
	nEnemyParty( _pEnemy->GetParty() ),
	bPreferGrenade( _bPreferGrenade )
{
	ResetTime( pEnemy );
}

void CSoldierAttackState::FireNow()
{
	NI_ASSERT( pGun != 0, "Wrong gun descriptor" );

	pUnit->RegisterAsBored( ACK_BORED_ATTACK );
	pUnit->Stop();
	pGun->StartEnemyBurst( pEnemy, bAim );
	bAim = false;

	NI_ASSERT( runUpToEnemy.IsRunningToEnemy() == false, "Wrong state" );
}

void CSoldierAttackState::StopFire()
{
	if ( pGun )
		pGun->StopFire();

	pUnit->UnRegisterAsBored( ACK_BORED_ATTACK );
	damageToEnemyUpdater.UnsetDamageFromEnemy( pEnemy );
	runUpToEnemy.Finish();

	bFinish = true;
}

ETryStateInterruptResult CSoldierAttackState::TryInterruptState( CAICommand *pCommand )
{
	if ( !pCommand )
	{
		StopFire();
		pUnit->SetCommandFinished();

		return TSIR_YES_IMMIDIATELY;
	}
	
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	if ( cmd.nCmdType == ACTION_COMMAND_ATTACK_UNIT && GetObjectByCmd( cmd ) == pEnemy )
		return TSIR_NO_COMMAND_INCOMPATIBLE;

	if ( !pGun || !pGun->IsBursting() )
	{
		StopFire();
		pUnit->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else
	{
		bFinish = true;
		return TSIR_YES_WAIT;
	}
}

void CSoldierAttackState::StartAgain()
{
	if ( pGun )
	{
		pGun->StopFire();
		pGun = 0;
	}

	damageToEnemyUpdater.UnsetDamageFromEnemy( pEnemy );
}

void CSoldierAttackState::AnalyzeBruteMovingPosition()
{
	const bool bIdle = pUnit->IsIdle();
	const bool bEnemyVisible = pEnemy->IsVisible( pUnit->GetParty() );
	if ( !bEnemyVisible && !pEnemy->IsRevealed() )
	{
		if ( bIdle )
			StopFire();
	}
	else
	{
		// враг в зоне огня
		if ( ( bIdle || curTime >= nextShootCheck ) && bEnemyVisible && pGun->CanShootToUnitWOMove( pEnemy ) )
			FireNow();
		else if ( curTime >= nextShootCheck && pGun->TooCloseToFire( pEnemy ) )
		{
			pUnit->SendAcknowledgement( ACK_NOT_IN_FIRE_RANGE );
			StopFire();
		}
		else if ( ( g_bAgressiveMovement || theDipl.IsAIPlayer( pUnit->GetPlayer() ) ? true : bSwarmAttack )
			&& ( bIdle || curTime >= nextShootCheck && lastEnemyTile != pEnemy->GetCenterTile() ) )
		{
			// позиция врага сменилась или уже стоим
			const float fRandomDist = 5.0f * SConsts::TILE_SIZE;

			CPtr<IStaticPath> pStaticPath = 
				bEnemyVisible ? 
				CreateStaticPathForAttack( pUnit, pEnemy, pGun->GetWeapon()->fRangeMin, pGun->GetFireRange( 0 ), fRandomDist, true ) :
				CreateStaticPathToPoint( pEnemy->GetCenterPlain(), VNULL2, pUnit, false, GetAIMap() );

			if ( pStaticPath )
			{
				if ( pUnit->GetCenterTile() != pStaticPath->GetFinishTile() )
					bAim = true;
				pUnit->SendAlongPath( pStaticPath, VNULL2, true );
			}
			else
			{
				pUnit->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
				StopFire();
			}

			lastEnemyTile = pEnemy->GetCenterTile();
			nextShootCheck = curTime + SHOOTING_CHECK + NRandom::Random( 0, 500 ); RecordRandomCall();
		}
	}
}

void CSoldierAttackState::AnalyzeMovingPosition()
{
	const bool bIdle = pUnit->IsIdle();
	const bool bEnemyVisible=  pEnemy->IsVisible( pUnit->GetParty() );

	if ( !bEnemyVisible && !pEnemy->IsRevealed() )
	{
		if ( bIdle )
			StopFire();
	}
	else if ( ( curTime >= nextShootCheck || bIdle ) && bEnemyVisible && pGun->CanShootToUnitWOMove( pEnemy ) )
		FireNow();
	else if ( ( g_bAgressiveMovement || theDipl.IsAIPlayer( pUnit->GetPlayer() ) ? true : bSwarmAttack )
		&& ( bIdle || curTime >= nextShootCheck && lastEnemyTile != pEnemy->GetCenterTile() ) )
	{
		// в зоне поиска пути к стороне
		if ( pGun->InGoToSideRange( pEnemy ) )
		{
			lastEnemyTile = SVector( -1, -1 );
			state = ESAS_MOVING_TO_SIDE;
		}
		// слишком близко
		else if ( pGun->TooCloseToFire( pEnemy ) )
		{
			pUnit->SendAcknowledgement( ACK_NOT_IN_FIRE_RANGE );
			StopFire();
		}
		// позиция врага сменилась или уже стоим, но далеко, чтобы идти к стороне
		else if ( bIdle || lastEnemyTile != pEnemy->GetCenterTile() )
		{
			// путь до расстояния, достаточно близкого до юнита, чтобы оттуда потом бежать к стороне
			CPtr<IStaticPath> pStaticPath = 
				CreateStaticPathForAttack( pUnit, pEnemy, pGun->GetWeapon()->fRangeMin, 2 * pGun->GetFireRange( 0 ), 0.0f, true );

			if ( pStaticPath )
			{
				if ( pUnit->GetCenterTile() != pStaticPath->GetFinishTile() )
					bAim = true;
				pUnit->SendAlongPath( pStaticPath, VNULL2, true );
			}
			else
			{
				pUnit->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
				StopFire();
			}

			lastEnemyTile = pEnemy->GetCenterTile();			
			nextShootCheck = curTime + SHOOTING_CHECK + NRandom::Random( 0, 500 ); RecordRandomCall();
		}
	}
}

IStaticPath* CSoldierAttackState::BestSidePath()
{
	//DebugTrace( "CSoldierAttackState::BestSidePath() for unit %d", pUnit->GetUniqueID() );
	const CVec2 vUnitCenter = pUnit->GetCenterPlain();
	const CVec2 vEnemyCenter = pEnemy->GetCenterPlain();
	CVec2 vEnemyDir = pEnemy->GetDirectionVector();
	const float fEnemyLength = pEnemy->GetStats()->vAABBHalfSize.y;
	const float fEnemyWidth = pEnemy->GetStats()->vAABBHalfSize.x;

	CVec2 vSides[4];

	CVec2 vShift = vEnemyDir * fEnemyLength;
	vSides[0] = vEnemyCenter + vShift; // 0k
	vSides[2] = vEnemyCenter - vShift; // 32k

	vShift = vEnemyDir * fEnemyWidth;
	std::swap( vShift.x, vShift.y );
	vShift.x = -vShift.x;
	
	vSides[1] = vEnemyCenter + vShift; // 16k
	vSides[3] = vEnemyCenter - vShift; // 48k

	int vBestSides[2] = { 0, 0 }; // в этом массиве две стороны
	int nCount = 2;

	for ( WORD i = 0; i < 2; ++i )
	{
		if ( !pGun->CanBreach( pEnemy, i + 0 ) )
		{
			if ( pGun->CanBreach( pEnemy, i + 2 ) )
				vBestSides[i] = i*16384 + 32768;
			else
				--nCount;
		}
		else if ( !pGun->CanBreach( pEnemy, i + 2 ) || fabs2( vUnitCenter - vSides[i + 0] ) < fabs2( vUnitCenter - vSides[i + 2] ) )
			vBestSides[i] = i*16384;
		else
			vBestSides[i] = i*16384 + 32768;
	}

	if ( nCount == 0 )
		return 0;

	WORD wMinAngle = ( vBestSides[0] + vBestSides[1] )/nCount - 8192*nCount;

	if ( nCount == 2 && vBestSides[0] == 0 && vBestSides[1] == 49152 )
		wMinAngle = 40960;

	const WORD wAngle = wMinAngle + NRandom::Random( 0, 16384*nCount ); RecordRandomCall();
	const CVec2 vAngle = GetVectorByDirection( wAngle );

	IStaticPath *pBestPath = 
		CreateStaticPathForSideAttack( pUnit, pEnemy, 
		vAngle, //vSides[nBestSide] - vEnemyCenter,
		pGun->GetWeapon()->fRangeMin, pGun->GetFireRange( 0 ),
		8.0f * SConsts::TILE_SIZE, 8192, false );
	return pBestPath;
}

void CSoldierAttackState::AnalyzeMovingToSidePosition()
{
	const bool bIdle = pUnit->IsIdle();
	const bool bEnemyVisible = pEnemy->IsVisible( pUnit->GetParty() );

	if ( !bEnemyVisible && !pEnemy->IsRevealed() )
	{
		if ( bIdle )
			StopFire();
	}
	else if ( ( curTime >= nextShootCheck || bIdle ) && bEnemyVisible && pGun->CanShootToUnitWOMove( pEnemy ) )
		FireNow();
	else if ( curTime >= nextShootCheck )
	{
		const SVector curEnemyTile = pEnemy->GetCenterTile();
		const WORD wCurEnemyDir = pEnemy->GetDirection();
		// стоим или пора проверять позицию и враг сдвинулся или повернулся
		if ( ( g_bAgressiveMovement || theDipl.IsAIPlayer( pUnit->GetPlayer() ) ? true : bSwarmAttack ) &&
			( bIdle || lastEnemyTile != curEnemyTile || DirsDifference( wCurEnemyDir, wLastEnemyDir ) >= ENEMY_DIR_TOLERANCE ) )
		{
			// убежал слишком далеко
			if ( !pGun->InGoToSideRange( pEnemy ) )
				state = ESAS_MOVING;
			// в радиуса поиска пути к стороне
			else 
			{
				if ( IStaticPath *pStaticPath = BestSidePath() )
				{
					if ( pUnit->GetCenterTile() != pStaticPath->GetFinishTile() )
						bAim = true;
					pUnit->SendAlongPath( pStaticPath, VNULL2, true );
				}
				else
				{
					pUnit->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
					StopFire();
				}
			}

			nextShootCheck = curTime + SHOOTING_CHECK + NRandom::Random( 0, 500 ); RecordRandomCall();
			lastEnemyTile = curEnemyTile;
			wLastEnemyDir = wCurEnemyDir;
		}
	}
}

bool CSoldierAttackState::IsBruteMoving()
{
	const int nMinPossiblePiercing = pGun->GetMinPossiblePiercing();
	return
		!pUnit->CanMove() ||
		pEnemy->GetArmor(0) <= nMinPossiblePiercing &&
		pEnemy->GetArmor(1) <= nMinPossiblePiercing &&
		pEnemy->GetArmor(2) <= nMinPossiblePiercing &&
		pEnemy->GetArmor(3) <= nMinPossiblePiercing;
}

void CSoldierAttackState::Segment()
{
	if ( bFinish )
	{
		StopFire();
		pUnit->SetCommandFinished();
	}
	else if ( pGun == 0 )
	{
		if ( IsValidObj( pEnemy ) )
		{
			pUnit->ResetShootEstimator( pEnemy, false, pUnit->GetForbiddenGuns() );
			pGun = pUnit->GetBestShootEstimatedGun();
			if ( pGun == 0 )
			{
				// if enemy is artillery - try attack crew
				if ( pEnemy->GetStats()->IsArtillery() )
				{
					CFormation *pCrew = checked_cast_ptr<CArtillery*>(pEnemy)->GetCrew();
					if ( pCrew )
					{
						NI_ASSERT( pCrew->Size() > 0, "Artillery w/o crew found!" );
						if ( pCrew->Size() > 0 )
						{
							pUnit->ResetShootEstimator( (*pCrew)[0], false, pUnit->GetForbiddenGuns() );
							for ( int i = 1; i < pCrew->Size(); ++i )
								pUnit->AddUnitToShootEstimator( (*pCrew)[i] );
						}
					}
				}
				if ( pGun == 0 )
					pUnit->SendAcknowledgement( pUnit->GetGunsRejectReason() );
			}
		}
		const EUnitAckType eReject = pUnit->GetGunsRejectReason();
		//CRAP{ VOT ZAPLATKA TAK ZAPLATKA. if being told to attack unit out of range, being
		// in tankpit - leave tankpit and repeat
		if ( !pUnit->GetCurCmd()->IsFromAI() && pUnit->IsInTankPit() && eReject == ACK_NOT_IN_FIRE_RANGE )
		{
			theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pUnit );
			return;
		}
		//CRAP}
		if ( pGun == 0 )
			StopFire();
		else
		{
			if ( IsBruteMoving() )
				state = ESAS_BRUTE_MOVING;
			else
				state = ESAS_MOVING;
		}
	}
	else if ( !pGun->IsFiring() && !pGun->CanShootToUnit( pEnemy ) )
		StartAgain();
	else if ( pGun->GetNAmmo() == 0 )
		StopFire();
	else 
	{
		damageToEnemyUpdater.SetDamageToEnemy( pUnit, pEnemy, pGun );
		
		if ( !pGun->IsFiring() )
		{
			runUpToEnemy.Segment();			

			// если можно перевыбирать цель, то выбрать цель
//			if ( bSwarmAttack )
//				pUnit->AnalyzeTargetScan( pEnemy, damageToEnemyUpdater.IsDamageUpdated(), false );

			if ( !bFinish )
			{
				if ( !runUpToEnemy.IsRunningToEnemy() )
				{
					if ( !IsValidObj( pEnemy ) || pEnemy == pUnit || pEnemy->GetParty() != nEnemyParty )
					{
						if ( IsValidObj( pEnemy ) && !pEnemy->IsVisible( pUnit->GetParty() ) && !pEnemy->IsRevealed() )
							pUnit->SendAcknowledgement( ACK_DONT_SEE_THE_ENEMY );
						
						StopFire();
						pUnit->SetCommandFinished();
					}
					else switch ( state )
					{
						case ESAS_BRUTE_MOVING:
							AnalyzeBruteMovingPosition();

							break;
						case ESAS_MOVING:
							AnalyzeMovingPosition();

							break;
						case ESAS_MOVING_TO_SIDE:
							AnalyzeMovingToSidePosition();

							break;
					}
				}
			}
		}
	}
}

const CVec2 CSoldierAttackState::GetPurposePoint() const
{
	if ( IsValidObj( pEnemy ) )
		return pEnemy->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										  CSoldierMoveToState													*
//*******************************************************************

IUnitState* CSoldierMoveToState::Instance( CAIUnit *pUnit, const CVec2 &point, const bool bForceReverse )
{
	return new CSoldierMoveToState( pUnit, point, bForceReverse );
}

CSoldierMoveToState::CSoldierMoveToState( CAIUnit *_pUnit, const CVec2 &_point, const bool _bForceReverse )
: pUnit( _pUnit ), startTime( curTime ), bWaiting( true ), CFreeFireManager( _pUnit ),
	point( _point ), wDirToPoint( _pUnit->GetFrontDirection() ), bLongMove( false ), bForceReverse( _bForceReverse )
{
	pUnit->UnlockTiles();
	pUnit->FixUnlocking();
}

void CSoldierMoveToState::SetReversePathMode( const bool bEnable )
{
	if ( !pUnit )
		return;

	ISmoothPath *pDefaultPath = pUnit->GetDefaultPath();
	ISmoothPath *pSmoothPath = pUnit->GetSmoothPath();
	if ( pDefaultPath )
		pDefaultPath->SetForceGoBackward( bEnable );
	if ( pSmoothPath && pSmoothPath != pDefaultPath )
		pSmoothPath->SetForceGoBackward( bEnable );
}

void CSoldierMoveToState::Segment()
{
	if ( bWaiting )
	{
		if ( curTime - startTime >= TIME_OF_WAITING )
		{
			bWaiting = false;

			if ( CPtr<IStaticPath> pStaticPath = pUnit->GetCurCmd()->CreateStaticPath( pUnit ) )
			{
				bLongMove = pStaticPath->GetLength() > 50;
				point += pUnit->GetGroupShift();
				wDirToPoint = GetDirectionByVector( point - pUnit->GetCenterPlain() );

				CBasicGun *pMainGun = pUnit->GetGuns()->GetMainGun();
				if ( pMainGun )
					pUnit->Lock( pMainGun );
				// Set the policy before Init so path rebuilds inherit it, and disable forward turn arcs for reverse.
				SetReversePathMode( bForceReverse );
				pUnit->SendAlongPath( pStaticPath, pUnit->GetGroupShift(), !bForceReverse );
				SetReversePathMode( bForceReverse );
			}
			else
			{
				SetReversePathMode( false );
				pUnit->SendAcknowledgement( ACK_NEGATIVE );
				pUnit->SetCommandFinished();
			}

			pUnit->UnfixUnlocking();
		}
	}
	else
	{
		if ( bForceReverse )
			SetReversePathMode( true );
		if ( !pUnit->GetStats()->IsInfantry() && ( pUnit->GetBehaviourFire() == SBehaviour::EFAtWill || pUnit->CanShootInMovement() ) )
			CFreeFireManager::Analyze( pUnit, 0 );
		if ( pUnit->IsIdle() || pUnit->GetNextCommand() != 0 && fabs2( pUnit->GetCenterPlain() - point ) <= sqr( 2.5f * (float)SConsts::TILE_SIZE ) )
		{
			CBasicGun *pMainGun = pUnit->GetGuns()->GetMainGun();
			if ( pMainGun )
				pUnit->Unlock( pMainGun );
			
			if ( pUnit->GetNextCommand() == 0 )
			{
				const WORD wDir = pUnit->GetFrontDirection() == pUnit->GetDirection() ? wDirToPoint : wDirToPoint + 32768;
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_GUARD, pUnit->GetCenterPlain(), wDir ), pUnit, false );
				if ( bLongMove )
					pUnit->SendAcknowledgement( NDb::ACK_MOVE_END, pUnit->GetPlayer() == theDipl.GetMyNumber() );
			}

			SetReversePathMode( false );
			pUnit->SetCommandFinished();
		}
	}
}

ETryStateInterruptResult CSoldierMoveToState::TryInterruptState( class CAICommand *pCommand )
{ 
	SetReversePathMode( false );
	pUnit->UnfixUnlocking();

	CBasicGun *pMainGun = pUnit->GetGuns()->GetMainGun();
	if ( pMainGun )
		pUnit->Unlock( pMainGun );

	pUnit->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierMoveToState::GetPurposePoint() const
{
	return point;
}

//*******************************************************************
//*											CSoldierTurnToPointState										*
//*******************************************************************

IUnitState* CSoldierTurnToPointState::Instance( CAIUnit *pUnit, const CVec2 &targCenter )
{
	return new CSoldierTurnToPointState( pUnit, targCenter );
}

CSoldierTurnToPointState::CSoldierTurnToPointState( CAIUnit *_pUnit, const CVec2 &_targCenter )
: CStatusUpdatesHelper( EUS_ROTATE, _pUnit ), pUnit( _pUnit ), lastCheck( curTime ), timeStart( curTime ), targCenter( _targCenter )
{
	pUnit->Stop();
}

void CSoldierTurnToPointState::Segment()
{
	if ( pUnit->TurnToTarget( targCenter ) )
		TryInterruptState( 0 );
	else
	{
		lastCheck = curTime;
		if ( curTime - timeStart > g_fRotateStatusWaitTime )
			InitStatus();
	}
}

ETryStateInterruptResult CSoldierTurnToPointState::TryInterruptState( class CAICommand *pCommand )
{ 
	pUnit->SetCommandFinished(); 
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierTurnToPointState::GetPurposePoint() const
{
	if ( pUnit && pUnit->IsRefValid() && pUnit->IsAlive() )
		return pUnit->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*											CSoldierMoveByDirState											*
//*******************************************************************

IUnitState* CSoldierMoveByDirState::Instance( CAIUnit *pUnit, const CVec2 &vTarget )
{
	return new CSoldierMoveByDirState( pUnit, vTarget );
}

CSoldierMoveByDirState::CSoldierMoveByDirState( CAIUnit *_pUnit, const CVec2 &vTarget )
: pUnit( _pUnit )
{
	IPath *pPath = new CStandartDirPath( pUnit->GetCenterPlain(), Norm( vTarget - pUnit->GetCenterPlain() ), vTarget, GetAIMap()->GetTileSize() );
	pUnit->SendAlongPath( pPath );
}

void CSoldierMoveByDirState::Segment()
{
	if ( pUnit->IsIdle() )
		pUnit->SetCommandFinished();
}

ETryStateInterruptResult CSoldierMoveByDirState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierMoveByDirState::GetPurposePoint() const
{
	return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*											CSoldierEnterState													*
//*******************************************************************

IUnitState* CSoldierEnterState::Instance( CAIUnit *pUnit, CBuilding *pBuilding )
{
	return new CSoldierEnterState( pUnit, pBuilding );
}

CSoldierEnterState::CSoldierEnterState( CAIUnit *_pUnit, CBuilding *_pBuilding )
:	pUnit( _pUnit ), pBuilding( _pBuilding ), state( EES_START ), nEfforts( 0 )
{
	int i = 0;
	while ( i < pBuilding->GetNEntrancePoints() )
	{
		if ( pBuilding->IsGoodPointForRunIn( pUnit->GetCenterPlain(), i ) )
		{
			SetPathForRunUp();
			nEntrance = i;
			state = EES_RUN_UP;
			break;
		}
		else
			++i;
	}
}

bool CSoldierEnterState::SetPathForRunUp()
{
	CPtr<IStaticPath> pBestPath = pUnit->GetPathToBuilding( pBuilding, &nEntrance );
	
	if ( pBestPath == 0 )
		return false;
	else
	{
		pUnit->SendAlongPath( pBestPath, VNULL2, true );
		return true;
	}
}

void CSoldierEnterState::Segment()
{
	if ( !IsValidObj( pBuilding ) || pBuilding->GetNFreePlaces() == 0 )
		TryInterruptState( 0 );
	else
	{
		switch ( state )
		{
			case EES_START:
				if ( SetPathForRunUp() )
					state = EES_RUN_UP;
				else
					pUnit->SetCommandFinished();

				break;
			case EES_RUN_UP:
				if ( pUnit->IsIdle() )
				{
					if ( nEfforts < 3 && !pBuilding->IsGoodPointForRunIn( pUnit->GetCenterPlain(), nEntrance, SConsts::TILE_SIZE ) )
					{
						++nEfforts;
						state = EES_START;
					}
					else
					{
						pUnit->TurnToDirection( GetDirectionByVector( CVec2(pBuilding->GetCenter().x, pBuilding->GetCenter().y) - pUnit->GetCenterPlain() ), false, true );
						theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_BUILDING, pBuilding->GetUniqueId() ), pUnit, false );
					}
				}

				break;
		}
	}
}

ETryStateInterruptResult CSoldierEnterState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierEnterState::GetPurposePoint() const
{
	if ( IsValidObj( pBuilding ) )
		return CVec2(pBuilding->GetCenter().x,pBuilding->GetCenter().y);
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CSoldierEnterEntrenchmentState								*
//*******************************************************************

IUnitState* CSoldierEnterEntrenchmentState::Instance( CAIUnit *pUnit, CEntrenchment *pEntrenchment )
{
	return new CSoldierEnterEntrenchmentState( pUnit, pEntrenchment );
}

CSoldierEnterEntrenchmentState::CSoldierEnterEntrenchmentState( CAIUnit *_pUnit, CEntrenchment *_pEntrenchment )
: pUnit( _pUnit ), state( EES_START ), pEntrenchment( _pEntrenchment ), nTries( 5 )
{
	if ( pEntrenchment->IsPointInside( pUnit->GetCenterPlain() ) )
		state = EES_RUN;
}

bool CSoldierEnterEntrenchmentState::SetPathForRunIn()
{
	CPtr<IStaticPath> pPath = pUnit->GetPathToEntrenchment( pEntrenchment );

	if ( pPath )
	{
		pUnit->SendAlongPath( pPath, VNULL2, true );
		return true;
	}
	else
		return false;
}

void CSoldierEnterEntrenchmentState::EnterToEntrenchment()
{
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRENCH, pEntrenchment->GetUniqueId() ), pUnit, false );
}

void CSoldierEnterEntrenchmentState::Segment()
{
	switch ( state )
	{
		case EES_START:
		if ( nTries-- <= 0 )
		{
			pUnit->SetCommandFinished();
		}
		if ( SetPathForRunIn() )
			state = EES_RUN;
		else
			pUnit->SetCommandFinished();

			break;
		case EES_RUN:
			if ( !IsValidObj( pEntrenchment ) )
				pUnit->SetCommandFinished();
			else if ( pEntrenchment->IsPointInside( pUnit->GetCenterPlain() ) )
			{
				EnterToEntrenchment();
				state = EES_FINISHED;
			}
			else if ( pUnit->IsIdle() )
				state = EES_START;

			break;
	}
}

ETryStateInterruptResult CSoldierEnterEntrenchmentState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand || 
				pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_IDLE_TRENCH ||
				state != EES_FINISHED )
	{
		pUnit->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else
		return TSIR_YES_WAIT;
}

const CVec2 CSoldierEnterEntrenchmentState::GetPurposePoint() const
{
	return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*									CSoldierAttackCommonStatObjState								*	
//*******************************************************************

IUnitState* CSoldierAttackCommonStatObjState::Instance( CAIUnit *pUnit, CStaticObject *pObj, bool bSwarmAttack )
{
	return new CSoldierAttackCommonStatObjState( pUnit, pObj, bSwarmAttack );
}

CSoldierAttackCommonStatObjState::CSoldierAttackCommonStatObjState( CAIUnit *_pUnit, CStaticObject *pObj, bool _bSwarmAttack )
: pUnit( _pUnit ), CCommonAttackCommonStatObjState( _pUnit, pObj, _bSwarmAttack ),
bFinishAfterInsidersDead( bSwarmAttack )
{
}

void CSoldierAttackCommonStatObjState::FireNow()
{
	NI_ASSERT( pGun != 0, "Wrong gun descriptor" );	
	
	pUnit->Stop();
	if ( pGun->IsOnTurret() )
		pGun->GetTurret()->Lock( pGun );
	// выстрелить
	pGun->StartPointBurst( pObj->GetAttackCenter( pUnit->GetCenterPlain() ), bAim );
}

ETryStateInterruptResult CSoldierAttackCommonStatObjState::TryInterruptState( class CAICommand *pCommand )
{
	if ( pCommand == 0 || !IsValid( pGun ) || !pGun->IsBursting() )
	{
		FinishState();
		pUnit->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}

	if ( pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_ATTACK_UNIT ||
				pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_SWARM_ATTACK_UNIT )
	{
		if ( checked_cast<CAIUnit*>(GetObjectByCmd(pCommand->ToUnitCmd()))->GetStats()->IsInfantry() )
		{
			CSoldier *pSoldier = checked_cast<CSoldier*>( GetObjectByCmd( pCommand->ToUnitCmd() ) );
			if ( pSoldier->IsInBuilding() && pSoldier->GetBuilding() == pObj )
				return TSIR_NO_COMMAND_INCOMPATIBLE;
		}
	}

	bFinish = true;
	return TSIR_YES_WAIT;
}

void CSoldierAttackCommonStatObjState::Segment()
{
	if ( bFinishAfterInsidersDead && pObj->GetNDefenders() == 0 )
	{
		TryInterruptState( 0 );
		return;
	}
	if ( bSwarmAttack )
		pUnit->AnalyzeTargetScan( 0, false, false, pObj );
	CCommonAttackCommonStatObjState::Segment();
}

CAIUnit* CSoldierAttackCommonStatObjState::GetUnit() const
{ 
	return pUnit; 
}

const CVec2 CSoldierAttackCommonStatObjState::GetPurposePoint() const
{
	if ( GetTarget() && GetTarget()->IsRefValid() && GetTarget()->IsAlive() && pUnit && pUnit->IsRefValid() && pUnit->IsAlive() )
		return GetTarget()->GetAttackCenter( pUnit->GetCenterPlain() );
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*												CSoldierParadeState												*
//*******************************************************************

IUnitState* CSoldierParadeState::Instance( class CAIUnit *pUnit )
{
	if ( pUnit->GetStats()->IsInfantry() && checked_cast<CSoldier*>(pUnit)->IsInFormation() )
	{
		if ( CPtr<IStaticPath> pPath = CreateStaticPathToPoint( checked_cast<CSoldier*>(pUnit)->GetUnitPointInFormation(), VNULL2, pUnit, false, GetAIMap() ) )
		{
			pUnit->SendAlongPath( pPath, VNULL2, true );
			return new CSoldierParadeState( pUnit );
		}
		else
			return 0;
	}
	else
		return 0;
}

CSoldierParadeState::CSoldierParadeState( class CAIUnit *_pUnit )
: pUnit( _pUnit )
{
}

void CSoldierParadeState::Segment()
{
	if ( pUnit->IsIdle() )
	{
		pUnit->SetCommandFinished();
		// повернуться в нужном направлении
		pUnit->SetDirectionVec( pUnit->GetFormation()->GetUnitDir( pUnit ) );
	}
}

ETryStateInterruptResult CSoldierParadeState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierParadeState::GetPurposePoint() const
{
	if ( pUnit && pUnit->IsRefValid() && pUnit->IsAlive() )
		return pUnit->GetFormation()->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*												CSoldierPlaceMineNowState									*
//*******************************************************************

IUnitState* CSoldierPlaceMineNowState::Instance( class CAIUnit *pUnit, const CVec2 &point, const enum EMineType nType )
{
	return new CSoldierPlaceMineNowState( pUnit, point, nType );
}

CSoldierPlaceMineNowState::CSoldierPlaceMineNowState( class CAIUnit *_pUnit, const CVec2 &_point, const enum EMineType _nType )
: pUnit( _pUnit ), point( _point ), nType( _nType )
{
	updater.AddUpdate( 0, ACTION_NOTIFY_USE_DOWN, pUnit, -1 );
	beginAnimTime = curTime;
}

void CSoldierPlaceMineNowState::Segment()
{
	if ( pUnit->GetPlayer() == theDipl.GetNeutralPlayer() )
		pUnit->SetCommandFinished();
	else if ( curTime - beginAnimTime >= pUnit->GetStats()->GetAnimTime( GetAnimationFromAction( ACTION_NOTIFY_USE_DOWN ) ) )
	{	
		theUnitCreation.CreateMine( static_cast<EMineType>(nType), CVec3(point,0.0f), pUnit->GetPlayer() );
		pUnit->SetCommandFinished();		
	}
}

ETryStateInterruptResult CSoldierPlaceMineNowState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand )
	{
		pUnit->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}

	return TSIR_YES_WAIT;
}

const CVec2 CSoldierPlaceMineNowState::GetPurposePoint() const
{
	return point;
}

//*******************************************************************
//*											CSoldierClearMineRadiusState								*
//*******************************************************************

IUnitState* CSoldierClearMineRadiusState::Instance( CAIUnit *pUnit, const CVec2 &clearCenter )
{
	return new CSoldierClearMineRadiusState( pUnit, clearCenter );
}

CSoldierClearMineRadiusState::CSoldierClearMineRadiusState( CAIUnit *_pUnit, const CVec2 &_clearCenter )
: pUnit( _pUnit ), clearCenter( _clearCenter ), eState( EPM_START )
{
}

bool CSoldierClearMineRadiusState::FindMineToClear()
{
	float fBestDistance2 = 1e10;
	pMine = 0;
	
	CStObjCircleIter<false> iter( clearCenter, SConsts::MINE_CLEAR_RADIUS );
	while ( !iter.IsFinished() )
	{
		// мину никто не собирается удалять и она в радиусе осмотра
		if ( (*iter)->GetObjectType() == ESOT_MINE && 
				!checked_cast<CMineStaticObject*>(*iter)->IsBeingDisarmed() && 
				fabs2( CVec2((*iter)->GetCenter().x,(*iter)->GetCenter().y) - clearCenter ) <= sqr( float(SConsts::MINE_CLEAR_RADIUS) ) )
		{
			const float fDistanceToUnit2 = fabs2( CVec2((*iter)->GetCenter().x,(*iter)->GetCenter().y) - pUnit->GetCenterPlain() );			
			if ( fDistanceToUnit2 < fBestDistance2 )
			{
				if ( CPtr<IStaticPath> pPath = CreateStaticPathToPoint( CVec2((*iter)->GetCenter().x,(*iter)->GetCenter().y), VNULL2, pUnit, true, GetAIMap() ) )
				{
					pMine = checked_cast<CMineStaticObject*>(*iter);
					fBestDistance2 = fDistanceToUnit2;
				}
			}
		}

		iter.Iterate();
	}

	if ( pMine != 0 )
	{
		pMine->SetBeingDisarmed( true );
		pUnit->SendAlongPath( CreateStaticPathToPoint( CVec2(pMine->GetCenter().x,pMine->GetCenter().y), VNULL2, pUnit, true, GetAIMap() ), VNULL2, true );

		return true;
	}
	else
		return false;
}

void CSoldierClearMineRadiusState::Segment()
{
	if ( pMine != 0 && !IsValidObj( pMine ) )
	{
		pMine = 0;
		eState = EPM_START;
	}
	else switch ( eState )
	{
		case EPM_START:
			if ( FindMineToClear() )
				eState = EPM_MOVE;
			else
				pUnit->SetCommandFinished();

			break;
		case EPM_MOVE:
			if ( !pMine->IsRefValid() || !pMine->IsAlive() )
			{
				pMine = 0;
				eState = EPM_START;
			}
			else if ( mDistance( pUnit->GetCenterPlain(), CVec2(pMine->GetCenter().x,pMine->GetCenter().y) ) <= SConsts::TILE_SIZE )
			{
				pUnit->Stop();
				
				eState = EPM_WAITING; 
				updater.AddUpdate( 0, ACTION_NOTIFY_USE_DOWN, pUnit, -1 );
				beginAnimTime = curTime;			
			}
			else if ( pUnit->IsIdle() )
			{
				if ( mDistance( pUnit->GetCenterPlain(), CVec2( pMine->GetCenter().x,pMine->GetCenter().y ) ) <= 3 * SConsts::TILE_SIZE )
				{
					eState = EPM_WAITING; 
					updater.AddUpdate( 0, ACTION_NOTIFY_USE_DOWN, pUnit, -1 );
					beginAnimTime = curTime;			
				}
				else
				{
					if( IsValidObj( pMine ) )
						pMine->SetBeingDisarmed( false );
					eState = EPM_START;
				}
			}

			break;		
		case EPM_WAITING:
			if ( curTime - beginAnimTime >= pUnit->GetStats()->GetAnimTime( GetAnimationFromAction( ACTION_NOTIFY_USE_DOWN ) ) )
			{
				pMine->Delete();
				pMine = 0;
				eState = EPM_START;
			}

			break;
	}
}

ETryStateInterruptResult CSoldierClearMineRadiusState::TryInterruptState( class CAICommand *pCommand )
{
	if ( eState != EPM_WAITING || !pUnit->IsAlive() || !pCommand || pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_DISAPPEAR )
	{
		pUnit->SetCommandFinished();
		if( IsValidObj( pMine ) )
			pMine->SetBeingDisarmed( false );

		return TSIR_YES_IMMIDIATELY;
	}
	else
		return TSIR_YES_WAIT;
}

//*******************************************************************
//*										CSoldierAttackUnitInBuildingState							*
//*******************************************************************

void CSoldierAttackUnitInBuildingState::FireNow()
{
	pUnit->Stop();

	if ( pGun->IsOnTurret() )
		pGun->GetTurret()->Lock( pGun );
	// выстрелить
	pGun->StartEnemyBurst( pTarget, bAim );
	bAim = false;
}

IUnitState* CSoldierAttackUnitInBuildingState::Instance( CAIUnit *pUnit, CSoldier *pTarget, bool bAim, const bool bSwarmAttack )
{
	return new CSoldierAttackUnitInBuildingState( pUnit, pTarget, bAim, bSwarmAttack );
}

CSoldierAttackUnitInBuildingState::CSoldierAttackUnitInBuildingState( CAIUnit *_pUnit, CSoldier *pTarget, bool bAim, const bool bSwarmAttack )
: pUnit( _pUnit ), bTriedToShootBuilding( false ), CCommonAttackUnitInBuildingState( _pUnit, pTarget, bAim, bSwarmAttack )
{
}

void CSoldierAttackUnitInBuildingState::Segment()
{
	if ( IsValidObj( pTarget ) && !pTarget->IsInBuilding() )
		pUnit->SetCommandFinished();
	else if ( !bTriedToShootBuilding && IsValidObj( pTarget ) )
	{
		bTriedToShootBuilding = true;

		if ( !pUnit->GetStats()->IsInfantry() && pUnit->ChooseGunForStatObjWOTime( pTarget->GetBuilding() ) )
		{
			const EActionCommand cmd = bSwarmAttack ? ACTION_COMMAND_SWARM_ATTACK_OBJECT : ACTION_COMMAND_ATTACK_OBJECT;
			theGroupLogic.InsertUnitCommand( SAIUnitCmd( cmd, pTarget->GetBuilding()->GetUniqueId() ), pUnit );
			pUnit->SetCommandFinished();
		}
	}
	else
		CCommonAttackUnitInBuildingState::Segment();
}

ETryStateInterruptResult CSoldierAttackUnitInBuildingState::TryInterruptState( class CAICommand *pCommand )
{
	FinishState();
	return TSIR_YES_IMMIDIATELY;
}

CAIUnit* CSoldierAttackUnitInBuildingState::GetUnit() const
{  
	return pUnit; 
}

const CVec2 CSoldierAttackUnitInBuildingState::GetPurposePoint() const
{
	if ( GetTarget() && GetTarget()->IsRefValid() && GetTarget()->IsAlive() )
		return GetTarget()->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CSoldierEnterTransportNowState								*
//*******************************************************************

IUnitState* CSoldierEnterTransportNowState::Instance( CAIUnit *pUnit, CMilitaryCar *pTransport )
{
	return new CSoldierEnterTransportNowState( pUnit, pTransport );
}

CSoldierEnterTransportNowState::CSoldierEnterTransportNowState( CAIUnit *_pUnit, CMilitaryCar *_pTransport )
: pUnit( _pUnit ), pTransport( _pTransport ), eState( EETS_START ), 
	vLastTransportCenter( _pTransport->GetCenterPlain() ), wLastTransportDir( _pTransport->GetFrontDirection() )
{
}

void CSoldierEnterTransportNowState::Segment()
{
	if ( !IsValidObj( pTransport ) )
		pUnit->SetCommandFinished();
	else
	{
		// проверять не сдвинулся ли транспорт
		if ( curTime - timeLastTrajectoryUpdate > pUnit->GetBehUpdateDuration() )
		{
			if ( vLastTransportCenter != pTransport->GetCenterPlain() || wLastTransportDir != pTransport->GetFrontDirection() )
				eState = EETS_START;

			timeLastTrajectoryUpdate = curTime;
		}

		switch ( eState )
		{
			case EETS_START:
				{
					if ( CPtr<IStaticPath> pPath = CreateStaticPathToPoint( pTransport->GetEntrancePoint(), VNULL2, pUnit, true, GetAIMap() ) )
					{
						pUnit->SendAlongPath( pPath, VNULL2, true );
						eState = EETS_MOVING;
						timeLastTrajectoryUpdate = curTime;
					}
					else
						pUnit->SetCommandFinished();
				}

				break;
			case EETS_MOVING:
				if ( pUnit->IsIdle() )
				{
					pUnit->TurnToDirection( GetDirectionByVector( pTransport->GetCenterPlain() - pUnit->GetCenterPlain() ), false, true );

					theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pTransport->GetUniqueId()), pUnit, false );
					eState = EETS_FINISHED;
				}

				break;
		}
	}
}

ETryStateInterruptResult CSoldierEnterTransportNowState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierEnterTransportNowState::GetPurposePoint() const
{
	if ( IsValidObj( pTransport ) )
		return pTransport->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CSoldierParaDroppingState											*
//*******************************************************************

IUnitState* CSoldierParaDroppingState::Instance( class CSoldier *_pUnit, class CAviation *pPlane )
{
	return new CSoldierParaDroppingState( _pUnit, pPlane );
}

CSoldierParaDroppingState::CSoldierParaDroppingState( class CSoldier *_pUnit, class CAviation *pPlane )
: pUnit( _pUnit ), eState( ESPDS_FALLING_W_PARASHUTE ), timeToCloseParashute( 0 )
{
	if ( !IsValidObj( pPlane ) )
	{
		pUnit->Disappear();
		return;
	}
	pUnit->AllowLieDown( false );
	pUnit->SetSmoothPath( new CParatrooperPath( pUnit->GetCenter(), pUnit ) );
	
	pRememberedStats = checked_cast<const NDb::SInfantryRPGStats*>( pUnit->GetStats() );
	
	// подменить падающего солдата паращютистом
	// запустить анимацию открывания парашюта
	CPtr<SParadropStartFinishUpdate> pUpdate = new SParadropStartFinishUpdate;
	pUpdate->bStart = true;
	pUpdate->pNewSoldierVisObj = theUnitCreation.GetParatrooperVisObj( pUnit->GetPlayer() );
	pUpdate->pParachuteVisObj = theUnitCreation.GetParachuteVisObj();
	pUpdate->nObjUniqueID = pUnit->GetUniqueId();

	updater.AddUpdate( pUpdate, ACTION_NOTIFY_PARADROP_STARTED, pUnit, -1 );

	updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, pUnit, -1 );
	// change visibility for paradroper model
	updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_VISIBILITY, pUnit, pUnit->IsVisibleByPlayer() );
	updater.AddUpdate( 0, ACTION_NOTIFY_FALLING, pUnit, -1 );
}

void CSoldierParaDroppingState::Segment()
{
	bool bAgain = false;
	do
	{
		bAgain = false;

		switch( eState )
		{
		case ESPDS_FALLING_W_PARASHUTE:
			if ( pUnit->GetSmoothPath()->IsFinished() )
			{
				updater.AddUpdate( 0, ACTION_NOTIFY_CLOSE_PARASHUTE, pUnit, -1 );
				eState = ESPDS_WAIT_FOR_END_UPDATES;
				eStateToSwitch = ESPDS_CLOSING_PARASHUTE;
				timeToCloseParashute = curTime + theUnitCreation.GetRemoveParachuteTime();

			}

			break;
		case ESPDS_CLOSING_PARASHUTE:
			// дождаться, когда время оставшегося падения меньше или равно времени сбора парашюта
			if ( timeToCloseParashute <= curTime )
			{
				CPtr<SParadropStartFinishUpdate> pUpdate = new SParadropStartFinishUpdate;
				pUpdate->bStart = false;
				pUpdate->pNewSoldierVisObj = pUnit->GetStats()->pvisualObject;
				pUpdate->pParachuteVisObj = theUnitCreation.GetParachuteVisObj();
				pUpdate->nObjUniqueID = pUnit->GetUniqueId();
				updater.AddUpdate( pUpdate, ACTION_NOTIFY_PARADROP_STARTED, pUnit, -1 );
				eState = ESPDS_WAIT_FOR_END_UPDATES;
				eStateToSwitch = ESPDS_FINISH_STATE;
			}
			
			break;
		case ESPDS_FINISH_STATE:
			{
				pUnit->RestoreSmoothPath();
				pUnit->AllowLieDown( true );
				updater.AddUpdate( 0, pUnit->GetMovingAction(), pUnit, -1 );
				updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, pUnit, -1 );

				eState = ESPDS_WATING_FOR_ALL;
				const CVec2 vCenter = pUnit->GetCenterPlain();
				if ( vCenter.x < 0 || vCenter.y < 0 || 
		 				!GetAIMap()->IsTileInside( AICellsTiles::GetTile(vCenter) ) ||
						GetTerrain()->IsLocked( pUnit->GetCenterTile(), EAC_HUMAN ) )
				{
					pUnit->Disappear(); // при падении на технику - смерть.
				}
			}
			break;
		case ESPDS_WATING_FOR_ALL:
			
			break;
		case ESPDS_WAIT_FOR_END_UPDATES:
			// multiplayer out of sync cure
//			if ( curTime > timeLastEndUpdates )
			{
				eState = eStateToSwitch;
				bAgain = true;
			}
			break;
		}
	} while( bAgain );
}

ETryStateInterruptResult CSoldierParaDroppingState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand )
	{
		pUnit->SetRememberedStats( pRememberedStats );
		updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_DBID, pUnit, -1 );
		eState = ESPDS_WAIT_FOR_END_UPDATES;
		pUnit->SetCommandFinished();
	}

	return TSIR_YES_WAIT;
}

const CVec2 CSoldierParaDroppingState::GetPurposePoint() const
{
	if ( pUnit && pUnit->IsRefValid() && pUnit->IsAlive() )
		return pUnit->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CSoldierUseSpyglassState											*
//*******************************************************************

IUnitState* CSoldierUseSpyglassState::Instance( CSoldier *pSoldier, const CVec2 &point )
{
	return new CSoldierUseSpyglassState( pSoldier, point );
}

CSoldierUseSpyglassState::CSoldierUseSpyglassState( CSoldier *_pSoldier, const CVec2 &point )
: CStatusUpdatesHelper( EUS_USE_SPY_GLASS ), pSoldier( _pSoldier ), vPoint2Look( point )
{
	pSoldier->SendAcknowledgement( ACK_POSITIVE );
	pSoldier->Stop();
	//
	const	WORD wDir2Look = GetDirectionByVector( vPoint2Look - pSoldier->GetCenterPlain() );
	pSoldier->TurnToDirection( wDir2Look, false, true );
	pSoldier->SetOwnSightRadius( SConsts::SPY_GLASS_RADIUS );
	//NI_ASSERT( SConsts::SPY_GLASS_RADIUS / ( 2 * SConsts::TILE_SIZE ) <= theWarFog.GetMaxRadius(), "Invalid sight radius for SPY_GLASS_RADIUS" );
	pSoldier->SetVisionAngle( SConsts::SPY_GLASS_ANGLE );
//	pSoldier->SetAngles( wDir2Point - SConsts::SPY_GLASS_ANGLE, wDir2Point + SConsts::SPY_GLASS_ANGLE );
	pSoldier->WarFogChanged();

	pSoldier->StartCamouflating();

	if ( pSoldier->GetFormation() )
		SetUnit( pSoldier->GetFormation() );
	else
		SetUnit( pSoldier );

	SetLookAnimation();
}

void CSoldierUseSpyglassState::SetLookAnimation()
{
	if ( pSoldier->GetStats()->etype == RPG_TYPE_SNIPER && pSoldier->IsLying() )
		updater.AddUpdate( 0, ACTION_NOTIFY_USE_SPYGLASS_LYING, pSoldier, -1 );
	else
		updater.AddUpdate( 0, ACTION_NOTIFY_USE_SPYGLASS, pSoldier, -1 );
}

void CSoldierUseSpyglassState::Segment()
{
	InitStatus();
	if ( pSoldier->IsIdle() )
	{
		const	WORD wDir2Look = GetDirectionByVector( vPoint2Look - pSoldier->GetCenterPlain() );
		if ( pSoldier->GetDirection() != wDir2Look )
		{
			pSoldier->TurnToDirection( wDir2Look, false, true );
			SetLookAnimation();
		}
	}
}

ETryStateInterruptResult CSoldierUseSpyglassState::TryInterruptState( CAICommand *pCommand )
{
	pSoldier->RemoveOwnSightRadius();
	pSoldier->SetAngles( 0, 65535 );
	pSoldier->SetVisionAngle( 32768 );
	pSoldier->WarFogChanged();

	pSoldier->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierUseSpyglassState::GetPurposePoint() const
{
	if ( pSoldier && pSoldier->IsRefValid() && pSoldier->IsAlive() )	
		return pSoldier->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CSoldierAttackFormationState									*
//*******************************************************************

IUnitState* CSoldierAttackFormationState::Instance( CAIUnit *pUnit, CFormation *pTarget, const bool bSwarmAttack )
{
	return new CSoldierAttackFormationState( pUnit, pTarget, bSwarmAttack );
}

CSoldierAttackFormationState::CSoldierAttackFormationState( CAIUnit *_pUnit, CFormation *_pTarget, const bool _bSwarmAttack )
: pUnit ( _pUnit ), pTarget ( _pTarget ), bSwarmAttack( _bSwarmAttack )
{
}

void CSoldierAttackFormationState::Segment()
{
	if ( !IsValidObj( pTarget ) || pTarget->Size() == 0 )
	{
	}
	else
	{
		if ( bSwarmAttack )
		{
			theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_SWARM_ATTACK_FORMATION, pTarget->GetUniqueId() ), pUnit );
			theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_SWARM_ATTACK_UNIT, (*pTarget)[0]->GetUniqueId() ), pUnit );
		}
		else
		{
			theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_ATTACK_FORMATION, pTarget->GetUniqueId() ), pUnit );
			theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ATTACK_UNIT, (*pTarget)[0]->GetUniqueId() ), pUnit );
		}
	}

	TryInterruptState ( 0 ) ;

}

ETryStateInterruptResult CSoldierAttackFormationState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierAttackFormationState::GetPurposePoint() const
{
	if ( IsValidObj( pTarget ) )
		return pTarget->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CSoldierIdleState															*
//*******************************************************************

IUnitState* CSoldierIdleState::Instance( class CAIUnit *pUnit )
{
	return new CSoldierIdleState( pUnit );
}

CSoldierIdleState::CSoldierIdleState( class CAIUnit *pUnit )
: pUnit( pUnit )
{
}

void CSoldierIdleState::Segment()
{
}

ETryStateInterruptResult CSoldierIdleState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierIdleState::GetPurposePoint() const
{
	if ( pUnit && pUnit->IsRefValid() && pUnit->IsAlive() )	
		return pUnit->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CSoldierAttackAviationState										*
//*******************************************************************

CSoldierAttackAviationState::CSoldierAttackAviationState ( class CAIUnit *pUnit, class CAviation *pPlane )
: predictedFire( pUnit, pPlane )
{
	theSupremeBeing.SetAAVisible( pUnit, pPlane->GetParty(), true );

	// search gun able to shoot to planes
	bool bInitted = false;
	
	// AA guns must fire 2nd shells to aviation if possible. 
	// try 2nd shells
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
	{
		CBasicGun * pGun = pUnit->GetGun( i );
		if ( pGun->GetShellType() == 1 && pGun->GetWeapon()->nCeiling > 0 && pGun->GetTurret() != 0 )
		{
			predictedFire.AddGunNumber( i );
			bInitted = true;
			break;
		}
	}
	// if not inited, then try all shells
	if ( !bInitted )
	{
		for ( int i = 0; i < pUnit->GetNGuns(); ++i )
		{
			CBasicGun * pGun = pUnit->GetGun( i );
			if ( pGun->GetWeapon()->nCeiling > 0 && pGun->GetTurret() != 0 )
			{
				predictedFire.AddGunNumber( i );
				bInitted = true;
				break;
			}
		}
	}

	if ( !bInitted )
	{
		pUnit->SendAcknowledgement( 0, ACK_NOT_IN_FIRE_RANGE, pUnit->GetPlayer() == theDipl.GetMyNumber() );
		TryInterruptState( 0 );
	}
	else if ( pUnit->GetStats()->IsArtillery() )
	{
		NI_ASSERT( dynamic_cast<CArtillery*>(pUnit) != 0, "Wrong type of unit" );
		CArtillery *pArt = checked_cast<CArtillery*>(pUnit);
		if ( !pArt->IsInstalled() )
		{
			pArt->ForceInstallAction();
			return;
		}
	}
}

void CSoldierAttackAviationState::Segment()
{
	predictedFire.Segment();
	if ( predictedFire.IsFinishedTask() )
		TryInterruptState( 0 );
}

ETryStateInterruptResult CSoldierAttackAviationState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand )
	{
		theSupremeBeing.SetAAVisible( predictedFire.GetUnit(), predictedFire.GetUnit()->GetParty() == 0 ? 1 : 0, false );
		predictedFire.GetUnit()->SetCommandFinished();
	  predictedFire.Stop();
		return TSIR_YES_IMMIDIATELY;
	}
	
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	if ( cmd.nCmdType == ACTION_COMMAND_ATTACK_UNIT && GetObjectByCmd( cmd ) == predictedFire.GetPlane() )
		return TSIR_NO_COMMAND_INCOMPATIBLE;

	theSupremeBeing.SetAAVisible( predictedFire.GetUnit(), predictedFire.GetUnit()->GetParty() == 0 ? 1 : 0, false );
	return TSIR_YES_WAIT;
}

CAIUnit* CSoldierAttackAviationState::GetTargetUnit() const
{
	return predictedFire.GetPlane();
}

//*******************************************************************
//*										CSoldierFireMoraleShellState									*
//*******************************************************************

IUnitState* CSoldierFireMoraleShellState::Instance( CAIUnit * pUnit, const class CVec2 &vTarget )
{
	return new CSoldierFireMoraleShellState( pUnit, vTarget );
}

CSoldierFireMoraleShellState::CSoldierFireMoraleShellState( CAIUnit * pUnit, const class CVec2 &vTarget )
: pUnit( pUnit ), nMoraleGun( -1 ), vTarget( vTarget )
{
	// choose morale gun
	const int nGuns = pUnit->GetNGuns();
	for ( int i = 0; i < nGuns; ++i )
	{
		CBasicGun * pGun = pUnit->GetGun( i );
		if ( pGun->GetShell().eDamageType == NDb::SWeaponRPGStats::SShell::DAMAGE_MORALE )
		{
			if ( !pGun->CanShootToPoint( vTarget, 0.0f ) )
				pUnit->SendAcknowledgement( pGun->GetRejectReason(), true );
			else
			{
				pGun->StartPointBurst( vTarget, true );
				nMoraleGun = i;
				break;
			}
		}
	}
	
	if ( -1 != nMoraleGun )
	{
		pUnit->SendAcknowledgement( ACK_NEGATIVE, true );
		TryInterruptState( 0 );
	}
}

void CSoldierFireMoraleShellState::Segment()
{
	CBasicGun *pGun = pUnit->GetGun( nMoraleGun );
	if ( !pGun->IsFiring() )
		pGun->StartPointBurst( vTarget, false );
}

ETryStateInterruptResult CSoldierFireMoraleShellState::TryInterruptState( CAICommand *pCommand )
{
	if ( -1 != nMoraleGun && pUnit && pUnit->IsAlive() )
		pUnit->GetGun( nMoraleGun )->StopFire();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*												CSoldierUseState													*
//*******************************************************************

IUnitState* CSoldierUseState::Instance( CAIUnit *pUnit, const EActionNotify &eState )
{
	return new CSoldierUseState( pUnit, eState );
}

CSoldierUseState::CSoldierUseState( CAIUnit *_pUnit, const EActionNotify &_eState )
: pUnit( _pUnit ), eState( _eState )
{
	pUnit->Stop();
	updater.AddUpdate( 0, eState, pUnit, -1 );
	
	NI_ASSERT( eState == ACTION_NOTIFY_USE_UP || eState == ACTION_NOTIFY_USE_DOWN, StrFmt( "Wrong action (%d) in UseState", (int)eState ) );
}

void CSoldierUseState::Segment()
{
}

ETryStateInterruptResult CSoldierUseState::TryInterruptState( CAICommand *pCommand )
{
	pUnit->StopCurAnimation();
	pUnit->SetCommandFinished();
	
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										  CSoldierEntrenchSelfState*
//*******************************************************************

CSoldierEntrenchSelfState::CSoldierEntrenchSelfState ( class CSoldier *_pSoldier )
: pSoldier( _pSoldier ), fOldProgress( -1.0f )
{
	pSoldier->Stop();
	if ( !pSoldier->IsInTankPit() )
		updater.AddUpdate( 0, ACTION_NOTIFY_USE_DOWN, pSoldier, -1 );

	float fCoeff = 1.0f;
	int nDefaultTime = SConsts::ENTRENCH_SELF_TIME;
	for ( int i = 0; i < (std::min<int>) ( pSoldier->GetStats()->GetActions()->specialAbilities.size(), pSoldier->GetAbilityLevel() ); ++i )
	{
		const int nAbility = pSoldier->GetStats()->GetActions()->specialAbilities[i]->eName;
		if ( nAbility == NDb::ABILITY_MOBILE_FORTRESS ) 
		{
			const NDb::SUnitSpecialAblityDesc *pSA = pSoldier->GetStats()->GetActions()->specialAbilities[i];
			NI_ASSERT( pSA, "Ability desc (Mobile Fortress) not found");
			if ( pSA )
				fCoeff = pSA->fParameter;
		}
		else if ( nAbility == NDb::ABILITY_ENTRENCH_SELF ) 
		{
			const NDb::SUnitSpecialAblityDesc *pSA = pSoldier->GetStats()->GetActions()->specialAbilities[i];
			NI_ASSERT( pSA, "Ability desc (Entrench Self) not found");
			if ( pSA )
				nDefaultTime = pSA->nSwitchOnTime;
		}
	}
	timePlaceEntrench = curTime + fCoeff * nDefaultTime;
	timeStartEntrench = curTime;
}

void CSoldierEntrenchSelfState::Segment()
{
	if ( pSoldier->IsInTankPit() )
	{
		TryInterruptState( 0 );
		return;
	}

	if ( curTime > timePlaceEntrench )
	{
		//const SVector tile( AICellsTiles::GetTile( pSoldier->GetCenter() ) );
		const SVector tile( pSoldier->GetCenterTile() );
		if ( GetTerrain()->CanDigEntrenchment( tile.x, tile.y ) )
		{
			// create tank pit and set unit into it
			const SVector vDigTile( pSoldier->GetCenterTile() );
			const SMechUnitRPGStats *pStats = theUnitCreation.GetFoxHole( GetTerrain()->CanDigEntrenchment( vDigTile.x, vDigTile.y ) );
			std::list<SObjTileInfo> tilesToLock;
			CPtr<CExistingObject> pTankPit = theStatObjs.AddNewTankPit( pStats, CVec3(pSoldier->GetCenterPlain(),0.0f), pSoldier->GetFrontDirection(), 0, pStats->vAABBHalfSize, tilesToLock, pSoldier );
			if ( pTankPit )
			{
				pSoldier->SetInTankPit( pTankPit );
			}
		}
		else
			pSoldier->SetInVirtualTankPit();
		pSoldier->LieDownForce();

		CPtr<SAISpecialAbilityUpdate> pUpdate = new SAISpecialAbilityUpdate;
		pUpdate->info.nAbilityType = NDb::ABILITY_ENTRENCH_SELF;
		pUpdate->info.state = EASS_ACTIVE;
		pUpdate->info.fCurValue = 0.0f;
		pUpdate->info.nObjUniqueID = pSoldier->GetUniqueId();
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pSoldier, -1 );
		fOldProgress = -1.0f;

		TryInterruptState( 0 );
	}
	else
	{
		float fNewValue = float( curTime - timeStartEntrench ) / ( timePlaceEntrench - timeStartEntrench );

		if ( fabs( fOldProgress - fNewValue ) > 0.1f )
		{
			CPtr<SAISpecialAbilityUpdate> pUpdate = new SAISpecialAbilityUpdate;
			pUpdate->info.nAbilityType = NDb::ABILITY_ENTRENCH_SELF;
			pUpdate->info.state = EASS_SWITCHING_ON;
			pUpdate->info.fCurValue = fNewValue;
			pUpdate->info.nObjUniqueID = pSoldier->GetUniqueId();
			updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pSoldier, -1 );

			fOldProgress = fNewValue;
		}
	}
}

ETryStateInterruptResult CSoldierEntrenchSelfState::TryInterruptState( class CAICommand *pCommand )
{
	if ( pCommand )
		return TSIR_NO_COMMAND_INCOMPATIBLE;
	else
	{
		pSoldier->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
}

const CVec2 CSoldierEntrenchSelfState::GetPurposePoint() const 
{ 
	return pSoldier->GetCenterPlain(); 
}


//*******************************************************************
//*										  CSoldierLeaveSelfEntrenchState*
//*******************************************************************

CSoldierLeaveSelfEntrenchState::CSoldierLeaveSelfEntrenchState( class CAIUnit *_pSoldier )
: pSoldier( _pSoldier )
{
	pSoldier->SetOffTankPit();
	pSoldier->SetCommandFinished();

	CPtr<SAISpecialAbilityUpdate> pUpdate = new SAISpecialAbilityUpdate;
	pUpdate->info.nAbilityType = NDb::ABILITY_ENTRENCH_SELF;
	pUpdate->info.state = EASS_READY_TO_ON;
	pUpdate->info.fCurValue = 0.0f;
	pUpdate->info.nObjUniqueID = pSoldier->GetUniqueId();
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pSoldier, -1 );
}

void CSoldierLeaveSelfEntrenchState::Segment()
{
}

ETryStateInterruptResult CSoldierLeaveSelfEntrenchState::TryInterruptState( class CAICommand *pCommand )
{
	pSoldier->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierLeaveSelfEntrenchState::GetPurposePoint() const
{
	return pSoldier->GetCenterPlain();
}


REGISTER_SAVELOAD_CLASS( 0x1108D4E6, CSoldierFireMoraleShellState );
REGISTER_SAVELOAD_CLASS( 0x1108D4AE, CSoldierAttackUnitInBuildingState );
REGISTER_SAVELOAD_CLASS( 0x1108D49D, CSoldierAttackCommonStatObjState );
REGISTER_SAVELOAD_CLASS( 0x1108D49C, CSoldierClearMineRadiusState );
REGISTER_SAVELOAD_CLASS( 0x1108D49B, CSoldierPlaceMineNowState );
REGISTER_SAVELOAD_CLASS( 0x1108D49A, CSoldierParadeState );
REGISTER_SAVELOAD_CLASS( 0x1108D481, CSoldierStatesFactory );
REGISTER_SAVELOAD_CLASS( 0x1108D465, CSoldierEnterTransportNowState );
REGISTER_SAVELOAD_CLASS( 0x1108D466, CSoldierParaDroppingState );
REGISTER_SAVELOAD_CLASS( 0x1108D467, CSoldierUseSpyglassState );
REGISTER_SAVELOAD_CLASS( 0x1108D468, CSoldierAttackFormationState );
REGISTER_SAVELOAD_CLASS( 0x1108D469, CSoldierIdleState );
REGISTER_SAVELOAD_CLASS( 0x1108D46A, CSoldierAttackAviationState );
REGISTER_SAVELOAD_CLASS( 0x1108D482, CSoldierRestState );
REGISTER_SAVELOAD_CLASS( 0x1108D483, CSoldierAttackState );
REGISTER_SAVELOAD_CLASS( 0x1108D484, CSoldierMoveToState );
REGISTER_SAVELOAD_CLASS( 0x1108D485, CSoldierTurnToPointState );
REGISTER_SAVELOAD_CLASS( 0x1108D486, CSoldierMoveByDirState );
REGISTER_SAVELOAD_CLASS( 0x1108D487, CSoldierEnterState );
REGISTER_SAVELOAD_CLASS( 0x1108D488, CSoldierEnterEntrenchmentState );
REGISTER_SAVELOAD_CLASS( 0x1508D486, CSoldierUseState );
REGISTER_SAVELOAD_CLASS( 0x110AB381, CSoldierEntrenchSelfState )
REGISTER_SAVELOAD_CLASS( 0x110AB380, CSoldierLeaveSelfEntrenchState )

