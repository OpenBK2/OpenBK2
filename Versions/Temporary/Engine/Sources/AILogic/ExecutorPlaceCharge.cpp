#include "StdAfx.h"
#include "ExecutorPlaceCharge.h"
#include "NewUpdater.h"
#include "../Stats_B2_M1/AbilityActions.h"

extern CEventUpdater updater;

REGISTER_SAVELOAD_CLASS( 0x120A4B00, CExecutorPlaceCharge )

CExecutorPlaceCharge::CExecutorPlaceCharge( CAIUnit *_pSoldier, NDb::EUnitSpecialAbility _eAbility ) 
: CExecutor(TID_PLACE_CHARGE, 1000/SConsts::AI_SEGMENT_DURATION), eState( EASS_READY_TO_ON ), eAbility( _eAbility )
{
	pSoldier = checked_cast<CSoldier*>( _pSoldier );
	UpdateState( EASS_READY_TO_ON );
}

int CExecutorPlaceCharge::Segment()
{
	if ( !IsExecutorValid() )
		return -1;

	return 0;
}

void CExecutorPlaceCharge::UpdateState( const EAbilitySwitchState eState )
{
	CPtr< SAISpecialAbilityUpdate > pUpdate = new SAISpecialAbilityUpdate;

	pUpdate->info.nAbilityType = eAbility;
	pUpdate->info.state = eState;
	pUpdate->info.nObjUniqueID = pSoldier->GetUniqueId();

	updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pSoldier, -1 );
}

bool CExecutorPlaceCharge::NotifyEvent( const CExecutorEvent &event )
{
	const CExecutorEventSpecialAbility *pEv = static_cast<const CExecutorEventSpecialAbility *>( &event );
	if ( pEv == 0 )
		return false;

	if ( pEv->GetAbility() != eAbility )
		return false;

	if ( pSoldier->GetUniqueId() != event.GetParam().nUnitID )
		return false;

	switch ( event.GetParam().eEventID )
	{
	case EID_ABILITY_ENABLE:
		if ( eState != EASS_READY_TO_ON )
		{
			eState = EASS_READY_TO_ON;
			UpdateState( EASS_READY_TO_ON );
		}
		return true;
	case EID_ABILITY_DISABLE:
		if ( eState != EASS_DISABLE )
		{
			eState = EASS_DISABLE;
			UpdateState( EASS_DISABLE );
		}
		return true;
	default:
		return false;
	}
}

void CExecutorPlaceCharge::RegisterOnEvents( IExecutorContainer *pContainer )
{
	SExecutorEventParam par;
	par.nUnitID = pSoldier->GetUniqueId();

	par.eEventID = EID_ABILITY_ENABLE;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_ABILITY_DISABLE;
	pContainer->RegisterOnEvent( this, par );
}


