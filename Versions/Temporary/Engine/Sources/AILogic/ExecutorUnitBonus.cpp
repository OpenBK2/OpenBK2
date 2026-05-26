#include "stdafx.h"

#include "ExecutorUnitBonus.h"
#include "NewUpdater.h"

extern CEventUpdater updater;

extern NTimer::STime curTime;

CExecutorUnitBonus::CExecutorUnitBonus( EUnitSpecialAbility eAbility, CAIUnit *_pUnit, EExecutorTypeID eTID ) :
CExecutorUnitBase ( eTID, /*SConsts::AI_SEGMENT_DURATION **/ NRandom::Random( 1, 10 ), eAbility ), pUnit( _pUnit ), timeLastUpdate( curTime ),
bBonusApplied( false ), eStatus( EUS_UNDEFINED )
{
	RecordRandomCall();
	//Init bonus pointer
	SetAbilityDesc( pUnit->GetUnitAbilityDesc( eAbility ) );
	if ( GetAbilityDesc() )
		pModifier = GetAbilityDesc()->pStatsBonus;

	switch( eAbility )
	{
	case NDb::ABILITY_CRITICAL_TARGETING:	eStatus = EUS_CRITICAL_TARGET; break;
	case NDb::ABILITY_SMOKE_SHOTS: eStatus = EUS_SMOKE_SHOT; break;
	case NDb::ABILITY_SPY_MODE: eStatus = EUS_SPY_MODE; break;
	case NDb::ABILITY_AMBUSH: eStatus = EUS_AMBUSH; break;
	case NDb::ABILITY_CAMOFLAGE_MODE: eStatus = EUS_CAMOUFLAGE; break;
	}
}

void CExecutorUnitBonus::RegisterOnEvents( IExecutorContainer *pContainer )
{
	std::vector<EExecutorEventID> unitEvents;

	unitEvents.push_back( EID_ABILITY_ACTIVATE );
	unitEvents.push_back( EID_ABILITY_ENABLE );
	unitEvents.push_back( EID_ABILITY_DISABLE );
	unitEvents.push_back( EID_ABILITY_DEACTIVATE );

	RegisterOnUnitEvents( pContainer, unitEvents, pUnit->GetUniqueId() );
}

bool CExecutorUnitBonus::NotifyEvent( const CExecutorEvent &event )
{
	//Filter out wrong calls
	if ( pUnit->GetUniqueId() != event.GetParam().nUnitID )
		return false;

	const CExecutorEventSpecialAbility *pEv = static_cast<const CExecutorEventSpecialAbility *>( &event );
	if ( pEv == 0 )
		return false;

	if ( pEv->GetAbility() != GetAbility() )
		return false;
	//end - Filter out wrong calls

	return CExecutorUnitBase::NotifyEvent( event );
}

float CExecutorUnitBonus::OnAbilityActive()
{
	//Count until expired
	const int nTimePassed = curTime - timeLastUpdate;
	if ( const NDb::SUnitSpecialAblityDesc *pSA = GetAbilityDesc() )
	{
		const int nTimeNeeded = pSA->nWorkTime;
		if ( nTimeNeeded == 0 )
			return 0.0f;					//Does not expire

		if ( nTimePassed >= nTimeNeeded )
		{
			return 1.0f;
		}
		else
			return float(nTimePassed) / float(nTimeNeeded);
	}

	return 0.0f;
}

float CExecutorUnitBonus::OnAbilityOff()
{
	return 0.0f;		//Wait for recharge timer
}

void CExecutorUnitBonus::SwitchingOffEnd()
{
	if ( pModifier && bBonusApplied )
	{
		//Turn bonus off
		pUnit->ApplyStatsModifier( pModifier, false );
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, "Ability stats bonus OFF" );
		bBonusApplied = false;
		if ( eStatus != EUS_UNDEFINED )
			updater.AddUpdate( CreateStatusUpdate( eStatus, bBonusApplied, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, pUnit, -1 );
	}
}

void CExecutorUnitBonus::SwitchingOffStart()
{
}

void CExecutorUnitBonus::SwitchOnEnd()
{
	if ( pModifier && !bBonusApplied )
	{
		//Turn bonus on
		pUnit->ApplyStatsModifier( pModifier, true );
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, "Ability stats bonus ON" );
		bBonusApplied = true;
		if ( eStatus != EUS_UNDEFINED )
			updater.AddUpdate( CreateStatusUpdate( eStatus, bBonusApplied, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, pUnit, -1 );
	}
	timeLastUpdate = curTime;
}

void CExecutorUnitBonus::SwitchOnStart( const class CAICommand *pCommand )
{
}

void CExecutorUnitBonus::ForceActivate()
{
	if ( GetState() == EASS_ACTIVE )
	{
		timeLastUpdate = curTime;
	}

	return CExecutorUnitBase::ForceActivate();
}

REGISTER_SAVELOAD_CLASS( 0x19126C80, CExecutorUnitBonus )


