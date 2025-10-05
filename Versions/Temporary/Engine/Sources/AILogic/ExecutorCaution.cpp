#include "stdafx.h"

#include "ExecutorCaution.h"

CExecutorCaution::CExecutorCaution( CAIUnit *_pUnit	) :
CExecutorUnitCombatBonus ( NDb::ABILITY_CAUTION, _pUnit, TID_CAUTION )
{
}

void CExecutorCaution::RegisterOnEvents( IExecutorContainer *pContainer )
{
	std::vector<EExecutorEventID> unitEvents;

	unitEvents.push_back( EID_ABILITY_ACTIVATE_AUTOCAST );
	unitEvents.push_back( EID_ABILITY_DEACTIVATE_AUTOCAST );

	RegisterOnUnitEvents( pContainer, unitEvents, GetAIUnit()->GetUniqueId() );

	CExecutorUnitCombatBonus::RegisterOnEvents( pContainer );
}

bool CExecutorCaution::NotifyEvent( const CExecutorEvent &event )
{
	//Filter out wrong calls
	if ( pUnit->GetUniqueId() != event.GetParam().nUnitID )
		return false;

	const CExecutorEventSpecialAbility *pEv = static_cast<const CExecutorEventSpecialAbility *>( &event );
	if ( pEv == 0 )
		return false;

	if ( pEv->GetAbility() != GetAbility() )
		return false;

	return CExecutorUnitCombatBonus::NotifyEvent( event );
}

CExecutorUnitCombatBonus::EAbilityCombatReaction CExecutorCaution::OnModeChange( const WORD oldModeFlags, const WORD newModeFlags )
{
	if ( GetAutocast() && GetState() == EASS_READY_TO_ON )	
	{
		if ( !( oldModeFlags & EUM_FIGHTING ) && ( newModeFlags & EUM_FIGHTING ) )		//Started fighting
			return EACR_FORCE_ACTIVATE;
	}

	return EACR_IGNORE;
}

void CExecutorCaution::SwitchOnEnd()
{
	//TODO? Go to "defensive" pose

	CExecutorUnitCombatBonus::SwitchOnEnd();
}

void CExecutorCaution::SwitchingOffEnd()
{
	//TODO? Leave "defensive" pose

	CExecutorUnitCombatBonus::SwitchingOffEnd();
}

REGISTER_SAVELOAD_CLASS( 0x19129440, CExecutorCaution )


