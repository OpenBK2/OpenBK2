#include "StdAfx.h"
#include "ExecutorThrowGrenade.h"
#include "Soldier.h"
#include "Guns.h"
#include "NewUpdater.h"
#include "../Stats_B2_M1/AbilityActions.h"

extern CEventUpdater updater;

REGISTER_SAVELOAD_CLASS( 0x1509C340, CExecutorThrowGrenade )

CExecutorThrowGrenade::CExecutorThrowGrenade( CAIUnit *_pUnit, NDb::EUnitSpecialAbility _eAbility ) 
: CExecutor(TID_THROW_GRENADE, 1000/SConsts::AI_SEGMENT_DURATION), pUnit( _pUnit ), eAbility( _eAbility )
{
	if ( HasGrenades() )
		state.eState = EASS_READY_TO_ON;
	else
		state.eState = EASS_DISABLE;

	state.bAutocast = false;
	secondaryState.eState = EASS_READY_TO_ON;
	secondaryState.bAutocast = false;
	UpdateState( state );
}

const bool CExecutorThrowGrenade::HasGrenades()
{
	const int nGuns = pUnit->GetNGuns();
	bool bPresent = false;
	for ( int i = 0; i < nGuns; ++i )
	{
		CBasicGun *pGun = pUnit->GetGun( i );
		if ( pGun->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE && 
			   pUnit->GetNAmmo( i ) > 0 )
		{
			bPresent = true;
			break;
		}
	}
	return bPresent;
}

int CExecutorThrowGrenade::Segment()
{
	if ( !IsExecutorValid() )
		return -1;

	if ( state.eState != EASS_DISABLE )
	{
		// check if unit has grenades, if not - disable ability.
		if ( !HasGrenades() )
		{
			state.eState = EASS_DISABLE;
			UpdateState( state );
		}
	}
	else if ( state.eState == EASS_DISABLE )
	{
		if ( HasGrenades() )
		{
			state.eState = EASS_READY_TO_ON;
			UpdateState( state );
		}
	}
	return GetNextTime();
}

void CExecutorThrowGrenade::UpdateState( const SAbilitySwitchState &_state )
{
	CPtr< SAISpecialAbilityUpdate > pUpdate = new SAISpecialAbilityUpdate;

	pUpdate->info.nAbilityType = eAbility;
	pUpdate->info.state = _state;
	pUpdate->info.nObjUniqueID = pUnit->GetUniqueId();

	updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pUnit, -1 );
}

bool CExecutorThrowGrenade::NotifyEvent( const CExecutorEvent &event )
{
	const CExecutorEventSpecialAbility *pEv = static_cast<const CExecutorEventSpecialAbility *>( &event );
	if ( pEv == 0 )
		return false;
		
	if ( pEv->GetAbility() != NDb::ABILITY_THROW_GRENADE && pEv->GetAbility() != NDb::ABILITY_THROW_ANTITANK_GRENADE )
		return false;
		
	CSoldier *pSoldier = checked_cast<CSoldier*>( pUnit.GetPtr() );
	if ( pSoldier->GetUniqueId() != event.GetParam().nUnitID )
		return false;
	
	switch ( event.GetParam().eEventID )
	{
		case EID_ABILITY_ACTIVATE_AUTOCAST:
			if ( !state.bAutocast )
			{
				state.bAutocast = true;
				UpdateState( state );
				pSoldier->SetGrenadeAutocast( true );
			}
			return true;
		case EID_ABILITY_DEACTIVATE_AUTOCAST:
			if ( state.bAutocast )
			{
				state.bAutocast = false;
				UpdateState( state );
				pSoldier->SetGrenadeAutocast( false );
			}
			return true;
		case EID_ABILITY_ENABLE:
			if ( secondaryState.eState != EASS_READY_TO_ON )
			{
				secondaryState.eState = EASS_READY_TO_ON;
				UpdateState( secondaryState );
			}
			return true;
		case EID_ABILITY_DISABLE:
			if ( secondaryState.eState != EASS_DISABLE )
			{
				secondaryState.eState = EASS_DISABLE;
				UpdateState( secondaryState );
			}
			return true;
		default:
			return false;
	}
}

void CExecutorThrowGrenade::RegisterOnEvents( IExecutorContainer *pContainer )
{
	SExecutorEventParam par;
	par.nUnitID = pUnit->GetUniqueId();

	par.eEventID = EID_ABILITY_ACTIVATE_AUTOCAST;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_ABILITY_DEACTIVATE_AUTOCAST;
	pContainer->RegisterOnEvent( this, par );
	
	par.eEventID = EID_ABILITY_ENABLE;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_ABILITY_DISABLE;
	pContainer->RegisterOnEvent( this, par );
}


