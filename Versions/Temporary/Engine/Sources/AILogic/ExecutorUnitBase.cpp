#include "stdafx.h"

#include "ExecutorUnitBase.h"
#include "NewUpdater.h"
#include "Soldier.h"

extern NTimer::STime curTime;
extern CEventUpdater updater;

CExecutorUnitBase::CExecutorUnitBase( const EExecutorTypeID _eTypeID, const int _nNextTime, const EUnitSpecialAbility _eAbility )
: timeLastUpdate( curTime ), timeDisableGroup( curTime ), eAbility( _eAbility ), state( EASS_READY_TO_ON ), stateBeforeDisable( EASS_DISABLE ),
	CExecutor( _eTypeID, _nNextTime ), fSpeedCoeff (1.0f), pAbilityDesc(0), fOldProgress( -1.0f )
{  

}

int CExecutorUnitBase::Segment()
{
	while ( UpdateAbilityState() )
	{
	}

	return GetNextTime();
}

void CExecutorUnitBase::UpdateProgress( const SAbilitySwitchState _state, const float fParam )
{
	if ( !pUpdate )
		pUpdate = new SAISpecialAbilityUpdate;

	pUpdate->info.nAbilityType = eAbility;
	pUpdate->info.state = state;
	pUpdate->info.fCurValue = fParam;
	pUpdate->info.nObjUniqueID = GetUnit()->GetUniqueId();

	bool bNearlySameProgress = false;
	if ( state.dwStateValue == lastSent.state.dwStateValue )
	{
		if ( fabs( fOldProgress - fParam ) < 0.1f )
			bNearlySameProgress = true;
	}

	if ( !bNearlySameProgress )
		fOldProgress = fParam;

	if ( !bNearlySameProgress && 0 != memcmp( &lastSent, &pUpdate->info, sizeof(lastSent) ) )
	{
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, GetUnit(), -1 );
		lastSent = pUpdate->info;
		pUpdate = 0;
	}
}

void CExecutorUnitBase::SetAutocast( const bool _bAutocast )
{
	state.bAutocast = _bAutocast;
	UpdateProgress( state , 0 );
}

bool CExecutorUnitBase::UpdateAbilityState()
{
	switch( state.eState )
	{
	case EASS_ACTIVE:
		{
			const float fProgress = OnAbilityActive();
			if ( 1.0f == fProgress )
			{
				SwitchingOffStart();
				timeLastUpdate = curTime;
				state.eState = EASS_SWITCHING_OFF;
				fOldProgress = -1.0f;
				return true;
			}
			else
				UpdateProgress( state, fProgress );
		}
		break;
	case EASS_DISABLE:
		{
			const int nTimePassed = curTime - timeLastUpdate;
			const int nTimeNeeded = timeDisableGroup;

			if ( timeDisableGroup == 0 ) // never enable
				UpdateProgress( state, 0.0f );
			else if ( nTimePassed >= nTimeNeeded )
			{
				state = stateBeforeDisable;
				fOldProgress = -1.0f;
				return true;
			}
			else
				UpdateProgress( state, float(nTimePassed) / float(nTimeNeeded) );
		}	

		break;
	case EASS_OFF:
		{
			const float fProgress = OnAbilityOff();
			if ( 1.0f == fProgress )
			{
				state.eState = EASS_READY_TO_ON;
				fOldProgress = -1.0f;
				return true;
			}
			else
			{
				const int nTimePassed = curTime - timeLastUpdate;
				if ( const NDb::SUnitSpecialAblityDesc *pSA = GetUnit()->GetUnitAbilityDesc(eAbility) )
				{
					const int nTimeNeeded = pSA->nRefreshTime;
					if ( nTimePassed >= nTimeNeeded )
					{
						state.eState = EASS_READY_TO_ON;
						fOldProgress = -1.0f;
						return true;
					}
					else
						UpdateProgress( state, float(nTimePassed) / float(nTimeNeeded) );
				}
			}
		}	

		break;
	case EASS_SWITCHING_ON:
		{
			const int nTimePassed = curTime - timeLastUpdate;
			if ( const NDb::SUnitSpecialAblityDesc *pSA = GetAbilityDesc() )
			{
				const int nTimeNeeded = pSA->nSwitchOnTime * fSpeedCoeff;
				if ( nTimePassed >= nTimeNeeded )
				{
					state.eState = EASS_ACTIVE;
					fOldProgress = -1.0f;
					SwitchOnEnd();
					return true;
				}
				else
					UpdateProgress( state, float(nTimePassed) / float(nTimeNeeded) );
			}
		}

		break;
	case EASS_SWITCHING_OFF:
		{
			const int nTimePassed = curTime - timeLastUpdate;
			if ( const NDb::SUnitSpecialAblityDesc *pSA = GetAbilityDesc() )
			{
				const int nTimeNeeded = pSA->nSwitchOffTime * fSpeedCoeff;
				if ( nTimePassed >= nTimeNeeded )
				{
					SwitchingOffEnd();
					fOldProgress = -1.0f;
					timeLastUpdate = curTime;
					state.eState = EASS_OFF;
					return true;
				}
				else
					UpdateProgress( state, float(nTimePassed) / float(nTimeNeeded) );
			}
		}

		break;
	case EASS_READY_TO_ON:
		UpdateProgress( state, 0 );
		break;
	}
	return false;
}

void CExecutorUnitBase::RegisterOnEvents( IExecutorContainer *pContainer, const std::vector<EExecutorEventID> &events, const SExecutorEventParam &_par )
{
	SExecutorEventParam par( _par );
	for ( int i = 0; i < events.size(); ++i )
	{
		par.eEventID = events[i];
		pContainer->RegisterOnEvent( this, par );
	}
}

void CExecutorUnitBase::RegisterOnUnitEvents( IExecutorContainer *pContainer, const std::vector<EExecutorEventID> &unitEvents, const int nUnitID )
{
	SExecutorEventParam par;
	par.nUnitID = nUnitID;
	RegisterOnEvents( pContainer, unitEvents, par );
}

void CExecutorUnitBase::Disable()
{
	if ( EASS_DISABLE != state.eState )
		stateBeforeDisable = state;
	state.eState = EASS_DISABLE;
	timeDisableGroup = 0;
}

bool CExecutorUnitBase::NotifyEvent( const CExecutorEvent &event )
{
	switch( event.GetParam().eEventID )
	{
	case EID_ABILITY_ACTIVATE_AUTOCAST:
		SetAutocast( true );

		return true;
	case EID_ABILITY_DEACTIVATE_AUTOCAST:
		SetAutocast( false );

		return true;
	case EID_ABILITY_DEACTIVATE:
		{
			// this ablity finished
			SwitchingOffStart();
			timeLastUpdate = curTime;
			state.eState = EASS_SWITCHING_OFF;
			fOldProgress = -1.0f;
		}

		return true;
	case EID_ABILITY_DISABLE:
		Disable();
		fOldProgress = -1.0f;

		break;
	case EID_ABILITY_ENABLE:
		state = stateBeforeDisable;

		break;
	case EID_NEW_COMMAND_RECIEVED:
		if ( state.eState == EASS_SWITCHING_ON )
		{
			state.eState = EASS_OFF;
			fOldProgress = -1.0f;
			timeLastUpdate = curTime;
			SwitchingOffEnd();
		}

		break;
	case EID_ABILITY_ACTIVATE:
		{
			const CExecutorEventSpecialAbilityActivate *pEv( static_cast<const CExecutorEventSpecialAbilityActivate *>( &event ) );
			if ( state.eState == EASS_DISABLE )
			{
				if ( ActivateDuringDisable() )
				{
					timeLastUpdate = curTime;
					state.eState = EASS_SWITCHING_ON;
					fOldProgress = -1.0f;
				}
			}
			else if ( eAbility == pEv->GetAbility() ) // this ablity started
			{
				if ( state.eState == EASS_READY_TO_ON )
				{								//If not already active, start switching on
					state.eState = EASS_SWITCHING_ON;
					SwitchOnStart( static_cast<const CExecutorEventSpecialAbilityActivate*>( &event )->GetCommand() );
					timeLastUpdate = curTime;
				}
			}
			else if ( GetAbilityDesc() && GetAbilityDesc()->eGroupID == pEv->GetDesc()->eGroupID ) // ability from this group started
			{
				if ( EASS_DISABLE != state.eState )
					stateBeforeDisable = state;
				state.eState = EASS_DISABLE;
				fOldProgress = -1.0f;
				timeDisableGroup = pEv->GetDesc()->nDisableGroupTime;
			}
		}
		return true;
	}
	return false;
}

void CExecutorUnitBase::ForceRecharge( )
{
	state.eState = EASS_OFF;
	fOldProgress = -1.0f;
	timeLastUpdate = curTime;
	UpdateProgress( state, 0 );
}

void CExecutorUnitBase::ForceActivate( )
{
	if ( state.eState == EASS_DISABLE )
	{
		if ( ActivateDuringDisable() )
		{
			SwitchOnStart( 0 );
			timeLastUpdate = curTime;
			state.eState = EASS_SWITCHING_ON;
		}
	}
	else if ( state.eState == EASS_READY_TO_ON ) {
		SwitchOnStart( 0 );
		timeLastUpdate = curTime;
		state.eState = EASS_SWITCHING_ON;
	}
	else if ( state.eState == EASS_SWITCHING_OFF ) {
		timeLastUpdate = curTime;
		state.eState = EASS_ACTIVE;
	}
	fOldProgress = -1.0f;
}

void CExecutorUnitBase::ForceDeactivate( )
{
	fOldProgress = -1.0f;
	if ( state.eState == EASS_ACTIVE ) 
	{
		SwitchingOffStart();
		timeLastUpdate = curTime;
		state.eState = EASS_SWITCHING_OFF;
	}
	else if ( state.eState == EASS_SWITCHING_ON ) 
	{
		timeLastUpdate = curTime;
		state.eState = EASS_READY_TO_ON;
	}
}

