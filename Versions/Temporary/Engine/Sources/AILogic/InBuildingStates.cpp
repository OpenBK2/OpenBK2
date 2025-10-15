#include "stdafx.h"

#include "InBuildingStates.h"
#include "Commands.h"
#include "Soldier.h"
#include "Building.h"
#include "NewUpdater.h"
#include "Guns.h"
#include "GroupLogic.h"
#include "Diplomacy.h"
#include "FeedBackSystem.h"

#include "AILogic_export.h"

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D4B8, CSoldierAttackInBuildingState );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D48C, CInBuildingStatesFactory );
REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D48D, CSoldierRestInBuildingState );

extern CDiplomacy theDipl;
extern CFeedBackSystem theFeedBackSystem;
extern CEventUpdater updater;
extern NTimer::STime curTime;
extern CGroupLogic theGroupLogic;

//*******************************************************************
//*										  CInBuildingStatesFactory										*
//*******************************************************************

CPtr<CInBuildingStatesFactory> CInBuildingStatesFactory::pFactory = 0;

IStatesFactory* CInBuildingStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CInBuildingStatesFactory();

	return pFactory;
}

bool CInBuildingStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		( cmdType == ACTION_COMMAND_DIE						||
			cmdType == ACTION_COMMAND_ATTACK_UNIT		||
			cmdType == ACTION_COMMAND_IDLE_BUILDING ||
//			cmdType == ACTION_COMMAND_AMBUSH				||
			cmdType == ACTION_COMMAND_DISAPPEAR			||
			cmdType == ACTION_COMMAND_SWARM_ATTACK_UNIT
		);
}

IUnitState* CInBuildingStatesFactory::ProduceState( class CQueueUnit *pUnit, CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CSoldier*>( pUnit ) != 0, "Wrong unit type" );
	CSoldier *pSoldier = checked_cast<CSoldier*>( pUnit );
	
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	IUnitState* pResult = 0;
	
	switch ( cmd.nCmdType )
	{
		case ACTION_COMMAND_DIE:
			NI_ASSERT( false, "Command to die in the queue" );

			break;
		case ACTION_COMMAND_SWARM_ATTACK_UNIT:
		case ACTION_COMMAND_ATTACK_UNIT:
			{
				CONVERT_OBJECT( CAIUnit, pUnit, GetObjectByCmd( cmd ), "Wrong unit to attack" );
				pResult = CSoldierAttackInBuildingState::Instance( pSoldier, pUnit );
			}

			break;
		case ACTION_COMMAND_IDLE_BUILDING:
			pResult = CSoldierRestInBuildingState::Instance( pSoldier, 0 );

			break;
		/*case ACTION_COMMAND_AMBUSH:
			pResult = CCommonAmbushState::Instance( pSoldier );

			break;*/
		default:
			NI_ASSERT( false, "Wrong command" );
	}

	return pResult;
}

IUnitState* CInBuildingStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	NI_ASSERT( dynamic_cast<CSoldier*>( pUnit ) != 0, "Wrong unit type" );
	return CSoldierRestInBuildingState::Instance( checked_cast<CSoldier*>( pUnit ), 0 );
}

//*******************************************************************
//*										CSoldierRestInBuildingState										*
//*******************************************************************

IUnitState* CSoldierRestInBuildingState::Instance( CSoldier *pSoldier, CBuilding *pBuilding )
{
	CSoldierRestInBuildingState *pRest = new CSoldierRestInBuildingState( pSoldier );
	pRest->SendUnitTo( pBuilding );
	
	return pRest;
}

CSoldierRestInBuildingState::CSoldierRestInBuildingState( CSoldier *_pSoldier )
: pSoldier( _pSoldier )
{
	startTime = curTime;

	pSoldier->StartCamouflating();
	ResetTime( pSoldier );
}

void CSoldierRestInBuildingState::SendUnitTo( CBuilding *pBuilding )
{
	if ( pBuilding != 0 )
	{
		if ( pBuilding->IsRefValid() && pBuilding->IsAlive() && pBuilding->GetNFreePlaces() != 0 )
			pSoldier->SetInBuilding( pBuilding );
		else
			pSoldier->SetCommandFinished();
	}
	else
		NI_ASSERT( pSoldier->IsInBuilding(), "Wrong unit state" );
}

void CSoldierRestInBuildingState::Segment()
{
	if ( pSoldier->IsInFollowState() && fabs( pSoldier->GetCenter() - pSoldier->GetFollowedUnit()->GetCenter() ) >= SConsts::FOLLOW_GO_RADIUS )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_FOLLOW_NOW, pSoldier->GetFollowedUnit()->GetUniqueId() ), pSoldier, false );
	else if ( pSoldier->GetSlot() != -1 )
		pSoldier->AnalyzeTargetScan( 0, false, false );
}

ETryStateInterruptResult CSoldierRestInBuildingState::TryInterruptState( class CAICommand *pCommand )
{
	pSoldier->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierRestInBuildingState::GetPurposePoint() const
{
	if ( pSoldier && pSoldier->IsRefValid() && pSoldier->IsAlive() )
		return pSoldier->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										 CSoldierAttackInBuildingState								*
//*******************************************************************

IUnitState* CSoldierAttackInBuildingState::Instance(  CSoldier *pSoldier, CAIUnit *pEnemy )
{
	return new CSoldierAttackInBuildingState( pSoldier, pEnemy );
}

CSoldierAttackInBuildingState::CSoldierAttackInBuildingState( class CSoldier *_pSoldier, CAIUnit *_pEnemy )
: pSoldier( _pSoldier ), pEnemy( _pEnemy ), bFinish( false ), bAim( true ), nEnemyParty( _pEnemy->GetParty() )
{
	pSoldier->GetBuilding()->Alarm();

	if ( !pEnemy->IsAlive() )
		pSoldier->SendAcknowledgement( ACK_INVALID_TARGET, true );
}

void CSoldierAttackInBuildingState::AnalyzeCurrentState()
{
	// можно выстрелить
	if ( pGun->CanShootToUnitWOMove( pEnemy ) )
	{
		// выстрелить
		pSoldier->RegisterAsBored( ACK_BORED_ATTACK );
		pGun->StartEnemyBurst( pEnemy, bAim );
		bAim = false;
	}
	else
	{
		const EUnitAckType eReject = pGun->GetRejectReason();
		pSoldier->SendAcknowledgement( eReject );
		if ( eReject == ACK_NO_AMMO && pSoldier->GetPlayer() == theDipl.GetMyNumber() )
			theFeedBackSystem.AddFeedbackAndForget( pSoldier->GetUniqueID(), pSoldier->GetCenterPlain(), EFB_NO_AMMO, -1 );

		pGun->StopFire();

		pSoldier->UnRegisterAsBored( ACK_BORED_ATTACK );
		damageToEnemyUpdater.UnsetDamageFromEnemy( pEnemy );
		pSoldier->SetCommandFinished();
	}
}

void CSoldierAttackInBuildingState::Segment()
{
	if ( !pSoldier->GetBuilding()->IsAnyAttackers() || bFinish )
	{
		if ( bFinish && ( !IsValid( pGun ) || !pGun->IsFiring() ) )
			pSoldier->SetCommandFinished();
		else if ( !IsValid( pGun ) && !bFinish )
		{
			if ( IsValidObj( pEnemy ) )
			{
				pSoldier->ResetShootEstimator( pEnemy, false, pSoldier->GetForbiddenGuns() );
				pGun = pSoldier->GetBestShootEstimatedGun();
				if ( pGun == 0 )
					pSoldier->SendAcknowledgement( pSoldier->GetGunsRejectReason() );
			}

			if ( pGun == 0 )
			{
				pSoldier->UnRegisterAsBored( ACK_BORED_ATTACK );
				damageToEnemyUpdater.UnsetDamageFromEnemy( pEnemy );
				pSoldier->SetCommandFinished();
			}
		}
		// не момент стрельбы
		else if ( !pGun->IsFiring() )
		{
			damageToEnemyUpdater.SetDamageToEnemy( pSoldier, pEnemy, pGun );
			
			// если враг мёртв или его не видно или стреляем сами по себе или пора заканчивать стрельбу
			if ( !IsValidObj( pEnemy ) || pEnemy.GetPtr() == pSoldier ||
					 !pEnemy->IsNoticableByUnit( pSoldier, pGun->GetFireRange( 0 ) ) || bFinish ||
					 nEnemyParty != pEnemy->GetParty() )
			{
				if ( IsValidObj( pEnemy ) && !pEnemy->IsNoticableByUnit( pSoldier, pGun->GetFireRange( 0 ) ) )
					pSoldier->SendAcknowledgement( ACK_DONT_SEE_THE_ENEMY );

				pSoldier->UnRegisterAsBored( ACK_BORED_ATTACK );
				damageToEnemyUpdater.UnsetDamageFromEnemy( pEnemy );
				pSoldier->SetCommandFinished();
				pGun->StopFire();
			}
			else
				AnalyzeCurrentState();
		}
	}
}

ETryStateInterruptResult CSoldierAttackInBuildingState::TryInterruptState( class CAICommand *pCommand )
{ 
	if ( !pCommand )
	{
		pSoldier->UnRegisterAsBored( ACK_BORED_ATTACK );
		damageToEnemyUpdater.UnsetDamageFromEnemy( pEnemy );
		pSoldier->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else
	{
		const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
		if ( cmd.nCmdType != ACTION_COMMAND_ATTACK_UNIT || GetObjectByCmd( cmd ) != pEnemy )
		{
			if ( IsValid( pGun ) )
				pGun->StopFire();

			bFinish = true;
			return TSIR_YES_WAIT;
		}
	}

	return TSIR_NO_COMMAND_INCOMPATIBLE;
}

const CVec2 CSoldierAttackInBuildingState::GetPurposePoint() const
{
	if ( IsValidObj( pEnemy ) )
		return pEnemy->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

CAIUnit* CSoldierAttackInBuildingState::GetTargetUnit() const
{
	return pEnemy;
}


