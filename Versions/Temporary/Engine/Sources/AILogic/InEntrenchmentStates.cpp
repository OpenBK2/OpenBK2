#include "stdafx.h"

#include "InEntrenchmentStates.h"
#include "Commands.h"
#include "Soldier.h"
#include "Entrenchment.h"
#include "NewUpdater.h"
#include "Guns.h"
#include "GroupLogic.h"
#include "UnitCreation.h"
#include "STaticObjects.h"
#include "Diplomacy.h"
#include "FeedbackSystem.h"

extern CFeedBackSystem theFeedBackSystem;
extern CDiplomacy theDipl;
extern CStaticObjects theStatObjs;
extern CEventUpdater updater;
extern CUnitCreation theUnitCreation;
extern NTimer::STime curTime;
extern CGroupLogic theGroupLogic;

//*******************************************************************
//*									  CInEntrenchmentStatesFactory									*
//*******************************************************************

CPtr<CInEntrenchmentStatesFactory> CInEntrenchmentStatesFactory::pFactory = 0;

IStatesFactory* CInEntrenchmentStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CInEntrenchmentStatesFactory();

	return pFactory;
}

bool CInEntrenchmentStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		( cmdType == ACTION_COMMAND_DIE						||
			cmdType == ACTION_COMMAND_ATTACK_UNIT		||
			cmdType == ACTION_COMMAND_IDLE_TRENCH		||
//			cmdType == ACTION_COMMAND_AMBUSH				||
			cmdType == ACTION_COMMAND_SWARM_ATTACK_UNIT ||
			cmdType == ACTION_COMMAND_DISAPPEAR );
}

IUnitState* CInEntrenchmentStatesFactory::ProduceState( class CQueueUnit *pUnit, CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CSoldier*>( pUnit ) != 0, "Wrong unit type" );
	CSoldier *pSoldier = checked_cast<CSoldier*>( pUnit );
	
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	IUnitState* pResult = 0;
	bool bSwarmAttack = false;
	
	switch ( cmd.nCmdType )
	{
		case ACTION_COMMAND_DIE:
			NI_ASSERT( false, "Command to die in the queue" );

			break;
		case ACTION_COMMAND_SWARM_ATTACK_UNIT:
			bSwarmAttack = true;
		case ACTION_COMMAND_ATTACK_UNIT:
			{
				CONVERT_OBJECT( CAIUnit, pUnit, GetObjectByCmd( cmd ), "Wrong unit to attack" );
				pResult = CSoldierAttackInEtrenchState::Instance( pSoldier, pUnit, bSwarmAttack );
			}

			break;
		case ACTION_COMMAND_IDLE_TRENCH:
			{
				CObjectBase *pEnt = GetObjectByCmd( cmd );
				if ( checked_cast<CEntrenchment*>(pEnt) )
					pResult = CSoldierRestInEntrenchmentState::Instance( pSoldier, checked_cast<CEntrenchment*>(pEnt) );
				else
					pResult = CSoldierRestInEntrenchmentState::Instance( pSoldier, 0 );
			}

			break;
/*		case ACTION_COMMAND_AMBUSH:
			pResult = CCommonAmbushState::Instance( pSoldier );

			break;*/
		default:
			NI_ASSERT( false, "Wrong command" );
	}

	return pResult;
}

IUnitState* CInEntrenchmentStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	NI_ASSERT( dynamic_cast<CSoldier*>( pUnit ) != 0, "Wrong unit type" );
	return CSoldierRestInEntrenchmentState::Instance( checked_cast<CSoldier*>( pUnit ), 0 );
}

//*******************************************************************
//*										CSoldierRestInEntrenchmentState								*
//*******************************************************************

IUnitState* CSoldierRestInEntrenchmentState::Instance( CSoldier *pSoldier, CEntrenchment *pEntrenchment )
{
	CSoldierRestInEntrenchmentState *pRest = new CSoldierRestInEntrenchmentState( pSoldier );
	pRest->SetUnitTo( pEntrenchment );

	return pRest;
}

CSoldierRestInEntrenchmentState::CSoldierRestInEntrenchmentState( CSoldier *_pSoldier )
: pSoldier( _pSoldier )
{
	startTime = curTime;
	pSoldier->StartCamouflating();
}

void CSoldierRestInEntrenchmentState::SetUnitTo( CEntrenchment *pEntrenchment )
{
	// только зашёл в окоп
	if ( pEntrenchment != 0 && !pSoldier->IsInEntrenchment())
		pSoldier->SetInEntrenchment( pEntrenchment );		
	// уже там сидит
	else
	{
		//NI_ASSERT( pSoldier->IsInEntrenchment(), "Wrong unit state" );
		if ( pSoldier->IsVisibleByPlayer() )
			updater.AddUpdate( 0, ACTION_NOTIFY_IDLE_TRENCH, pSoldier, -1 );
	}
}

void CSoldierRestInEntrenchmentState::Segment()
{
	if ( pSoldier->IsInFirePlace() )
		pSoldier->AnalyzeTargetScan( 0, false, false );

	if ( pSoldier->IsInFollowState() && fabs( pSoldier->GetCenter() - pSoldier->GetFollowedUnit()->GetCenter() ) >= SConsts::FOLLOW_GO_RADIUS )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_FOLLOW_NOW, pSoldier->GetFollowedUnit()->GetUniqueId() ), pSoldier, false );
	else
		pSoldier->FreezeByState( true );
}

ETryStateInterruptResult CSoldierRestInEntrenchmentState::TryInterruptState( class CAICommand *pCommand )
{
	pSoldier->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CSoldierRestInEntrenchmentState::GetPurposePoint() const
{
	if ( pSoldier && pSoldier->IsRefValid() && pSoldier->IsAlive() )	
		return pSoldier->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										  CSoldierAttackInEtrenchState								*
//*******************************************************************

IUnitState* CSoldierAttackInEtrenchState::Instance( CSoldier *pSoldier, CAIUnit *pEnemy, const bool bSwarmAttack )
{
	return new CSoldierAttackInEtrenchState( pSoldier, pEnemy, bSwarmAttack );
}

CSoldierAttackInEtrenchState::CSoldierAttackInEtrenchState( CSoldier *_pSoldier, CAIUnit *_pEnemy, const bool _bSwarmAttack )
: pSoldier( _pSoldier ), pEnemy( _pEnemy ), bFinish( false ), bAim( true ), bSwarmAttack( _bSwarmAttack ),
	nEnemyParty( _pEnemy->GetParty() )
{ 
	if ( !pEnemy->IsAlive() )
		pSoldier->SendAcknowledgement( ACK_INVALID_TARGET, true );
}

void CSoldierAttackInEtrenchState::AnalyzeCurrentState()
{
	// можно выстрелить и пробить броню
	if ( pGun->InFireRange( pEnemy ) )
	{
		if ( pGun->CanShootToUnit( pEnemy ) )
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
	else
	{
		pSoldier->SendAcknowledgement( ACK_NOT_IN_FIRE_RANGE );
		pGun->StopFire();
		pSoldier->UnRegisterAsBored( ACK_BORED_ATTACK );
		damageToEnemyUpdater.UnsetDamageFromEnemy( pEnemy );
		pSoldier->SetCommandFinished();
	}
}

void CSoldierAttackInEtrenchState::FinishState()
{
	pSoldier->UnRegisterAsBored( ACK_BORED_ATTACK );
	damageToEnemyUpdater.UnsetDamageFromEnemy( pEnemy );
	pSoldier->SetCommandFinished();
}

void CSoldierAttackInEtrenchState::Segment()
{
	if ( bFinish || !IsValidObj( pEnemy ) )
		FinishState();
	else if ( !pSoldier->GetEntrenchment()->IsAnyAttackers() )
	{
		if ( pGun == 0 )
		{
			pSoldier->ResetShootEstimator( pEnemy, false, pSoldier->GetForbiddenGuns() );
			pGun = pSoldier->GetBestShootEstimatedGun();

			if ( pGun == 0 )
			{
				pSoldier->SendAcknowledgement( pSoldier->GetGunsRejectReason() );				
				FinishState();
			}
		}
		// не момент стрельбы
		else if ( !pGun->IsFiring() )
		{
			damageToEnemyUpdater.SetDamageToEnemy( pSoldier, pEnemy, pGun );
//			if ( bSwarmAttack )
//				pSoldier->AnalyzeTargetScan( pEnemy, damageToEnemyUpdater.IsDamageUpdated(), false );
			// если враг мёртв или его не видно или стреляем сами по себе или пора заканчивать стрельбу
			if ( !IsValidObj( pEnemy ) ||
					 !pEnemy->IsVisible( pSoldier->GetParty() ) || pEnemy.GetPtr() == pSoldier || bFinish ||
					 pEnemy->GetParty() != nEnemyParty )
			{
				if ( IsValidObj( pEnemy ) && !pEnemy->IsVisible( pSoldier->GetParty() ) )
					pSoldier->SendAcknowledgement( ACK_DONT_SEE_THE_ENEMY );
				
				pGun->StopFire();
				damageToEnemyUpdater.UnsetDamageFromEnemy( pEnemy );
				pSoldier->UnRegisterAsBored( ACK_BORED_ATTACK );
				pSoldier->SetCommandFinished();
			}
			else
				AnalyzeCurrentState();
		}
	}
}

ETryStateInterruptResult CSoldierAttackInEtrenchState::TryInterruptState( class CAICommand *pCommand )
{ 
	if ( !pCommand )
	{
		damageToEnemyUpdater.UnsetDamageFromEnemy( pEnemy );
		pSoldier->UnRegisterAsBored( ACK_BORED_ATTACK );
		pSoldier->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}

	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	if ( cmd.nCmdType != ACTION_COMMAND_ATTACK_UNIT || GetObjectByCmd( cmd ) != pEnemy )
	{
		if ( IsValid( pGun ) )
			pGun->StopFire();

		bFinish = true;
		return TSIR_YES_WAIT;
	}
	else
		return TSIR_NO_COMMAND_INCOMPATIBLE;
}

const CVec2 CSoldierAttackInEtrenchState::GetPurposePoint() const
{
	if ( IsValidObj( pEnemy ) )
		return pEnemy->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

CAIUnit* CSoldierAttackInEtrenchState::GetTargetUnit() const
{
	return pEnemy;
}

REGISTER_SAVELOAD_CLASS( 0x1108D48E, CInEntrenchmentStatesFactory );
REGISTER_SAVELOAD_CLASS( 0x1108D48F, CSoldierRestInEntrenchmentState );
REGISTER_SAVELOAD_CLASS( 0x1108D490, CSoldierAttackInEtrenchState );

