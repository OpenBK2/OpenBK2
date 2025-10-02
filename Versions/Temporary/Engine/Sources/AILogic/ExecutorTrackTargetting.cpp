#include "StdAfx.h"

#include "ExecutorTrackTargetting.h"
#include "NewUpdater.h"
#include "Technics.h"
#include "GroupLogic.h"
#include "UnitsIterators2.h"

extern CGroupLogic theGroupLogic;
extern NTimer::STime curTime;
extern CEventUpdater updater;

CExecutorTrackTargetting::CExecutorTrackTargetting( CAIUnit *_pUnit )
: state( EASS_READY_TO_ON ), CExecutor( TID_TRACK_TARGETTING, 1000/SConsts::AI_SEGMENT_DURATION ), 
timeLastUpdate( curTime ), pUnit( _pUnit )
{  
	NI_ASSERT( _pUnit, "Attempt to create TrackTargetting executor for null unit" );
}

int CExecutorTrackTargetting::Segment()
{
	if ( !pUnit || !IsValid( pUnit ) || !pUnit->IsAlive() )
	{
		return -1;
	}

	// Check state: do scan if Autocast and the previous track targetting finished
	if ( GetAutocast() && !pUnit->IsTargetingTrack() )
	{
		bool bFoundUnit = false;
		CAIUnit *pTarget = 0;

		for ( CUnitsIter<1,3> iter( pUnit->GetParty(), EDI_ENEMY, CVec2(pUnit->GetCenter().x,pUnit->GetCenter().y), pUnit->GetSightRadius() );
			!iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pCurUnit = *iter;
			if ( pCurUnit->IsRefValid() && pCurUnit->IsAlive() && 
				pCurUnit->IsVisible( pUnit->GetParty() ) && !pCurUnit->GetStats()->IsAviation() )
			{
				CTank *pTankTarget;

				pTankTarget = dynamic_cast< CTank * >( pCurUnit );
				if ( pTankTarget && !pTankTarget->IsTrackDamaged() )			// It's a tank, and it's mobile
				{
					bFoundUnit = true;
					pTarget = pCurUnit;
					break;
				}
			}
		}
		
		// Found non-track-damaged unit
		if ( bFoundUnit ) 
		{
			//pUnit->SetTrackTargeting( true );
			SAIUnitCmd cmd( ACTION_COMMAND_TRACK_TARGETING, pTarget->GetUniqueId() );
			theGroupLogic.InsertUnitCommand( cmd, pUnit );
		}
	}

	return GetNextTime();
}

void CExecutorTrackTargetting::UpdateProgress( const SAbilitySwitchState _state, const float fParam )
{
	if ( !pUpdate )
		pUpdate = new SAISpecialAbilityUpdate;

	pUpdate->info.nAbilityType = NDb::ABILITY_TRACK_TARGETING;
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

void CExecutorTrackTargetting::SetAutocast( const bool _bAutocast )
{
	state.bAutocast = _bAutocast;
	UpdateProgress( state , 0 );
}

void CExecutorTrackTargetting::RegisterOnEvents( IExecutorContainer *pContainer )
{
	SExecutorEventParam par;
	par.nUnitID = pUnit->GetUniqueId();

	par.eEventID = EID_ABILITY_ACTIVATE_AUTOCAST;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_ABILITY_DEACTIVATE_AUTOCAST;
	pContainer->RegisterOnEvent( this, par );
}

bool CExecutorTrackTargetting::NotifyEvent( const CExecutorEvent &event )
{
	//Filter out wrong calls
	if ( pUnit->GetUniqueId() != event.GetParam().nUnitID )
		return false;

	const CExecutorEventSpecialAbility *pEv = static_cast<const CExecutorEventSpecialAbility *>( &event );
	if ( pEv == 0 )
		return false;

	if ( pEv->GetAbility() != NDb::ABILITY_TRACK_TARGETING )
		return false;
	//end - Filter out wrong calls

	switch( event.GetParam().eEventID )
	{
	case EID_ABILITY_ACTIVATE_AUTOCAST:
		SetAutocast( true );
		pUnit->SetTrackTargeting( false );

		return true;
	case EID_ABILITY_DEACTIVATE_AUTOCAST:
		SetAutocast( false );
		pUnit->SetTrackTargeting( false );

		return true;
	}
	return false;
}

REGISTER_SAVELOAD_CLASS( 0x19152B80, CExecutorTrackTargetting )


