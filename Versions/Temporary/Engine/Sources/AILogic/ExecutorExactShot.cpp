#include "StdAfx.h"

#include "ExecutorExactShot.h"
#include "NewUpdater.h"

extern CGroupLogic theGroupLogic;
extern NTimer::STime curTime;
extern CEventUpdater updater;

CExecutorExactShot::CExecutorExactShot( CAIUnit *_pUnit )
: state( EASS_READY_TO_ON ), CExecutor( TID_EXACT_SHOT, 1000/SConsts::AI_SEGMENT_DURATION ), 
timeLastUpdate( curTime ), pUnit( _pUnit )
{  
}

int CExecutorExactShot::Segment()
{
	if ( !IsExecutorValid() )
		return -1;

	// CRAP: turn it on again after each shot
	if ( state.bAutocast && !pUnit->IsIgnoreAABBCoeff() )
		pUnit->SetIgnoreAABBCoeff( true );

	return GetNextTime();
}

void CExecutorExactShot::UpdateProgress( const SAbilitySwitchState _state, const float fParam )
{
	if ( !pUpdate )
		pUpdate = new SAISpecialAbilityUpdate;

	pUpdate->info.nAbilityType = NDb::ABILITY_EXACT_SHOT;
	pUpdate->info.state = state;
	pUpdate->info.fCurValue = fParam;
	pUpdate->info.nObjUniqueID = pUnit->GetUniqueId();

	if ( 0 != memcmp( &lastSent, &pUpdate->info, sizeof(lastSent) ) )
	{
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pUnit, -1 );
		lastSent = pUpdate->info;
		pUpdate = 0;
	}

}

void CExecutorExactShot::SetAutocast( const bool _bAutocast )
{
	if ( ( state.bAutocast != 0 ) != _bAutocast )
	{
		state.bAutocast = _bAutocast;

		pUnit->SetIgnoreAABBCoeff( _bAutocast );

		UpdateProgress( state , 0 );
	}
}

void CExecutorExactShot::RegisterOnEvents( IExecutorContainer *pContainer )
{
	SExecutorEventParam par;
	par.nUnitID = pUnit->GetUniqueId();

	par.eEventID = EID_ABILITY_ACTIVATE_AUTOCAST;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_ABILITY_DEACTIVATE_AUTOCAST;
	pContainer->RegisterOnEvent( this, par );
}

bool CExecutorExactShot::NotifyEvent( const CExecutorEvent &event )
{
	//Filter out wrong calls
	if ( pUnit->GetUniqueId() != event.GetParam().nUnitID )
		return false;

	const CExecutorEventSpecialAbility *pEv = static_cast<const CExecutorEventSpecialAbility *>( &event );
	if ( pEv == 0 )
		return false;

	if ( pEv->GetAbility() != NDb::ABILITY_EXACT_SHOT )
		return false;
	//end - Filter out wrong calls

	switch( event.GetParam().eEventID )
	{
	case EID_ABILITY_ACTIVATE_AUTOCAST:
		SetAutocast( true );

		return true;
	case EID_ABILITY_DEACTIVATE_AUTOCAST:
		SetAutocast( false );

		return true;
	}
	return false;
}

REGISTER_SAVELOAD_CLASS( 0x1112D400, CExecutorExactShot )


