#include "stdafx.h"

#include "ExecutorAdrenalineRush.h"

CExecutorAdrenalineRush::CExecutorAdrenalineRush( CAIUnit *_pUnit	) :
CExecutorUnitCombatBonus ( NDb::ABILITY_ADRENALINE_RUSH, _pUnit, TID_ADRENALINE_RUSH )
{
}

CExecutorUnitCombatBonus::EAbilityCombatReaction CExecutorAdrenalineRush::OnModeChange( const WORD oldModeFlags, const WORD newModeFlags )
{
	if ( ( GetState() == EASS_READY_TO_ON || GetState() == EASS_SWITCHING_OFF ) && ( newModeFlags & EUM_FIGHTING ) ) 
		return EACR_FORCE_ACTIVATE;
	else if ( ( GetState() == EASS_ACTIVE || GetState() == EASS_SWITCHING_ON ) && !( newModeFlags & EUM_FIGHTING ) ) 
		return EACR_FORCE_DEACTIVATE;
	return EACR_IGNORE;
}

int CExecutorAdrenalineRush::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CExecutorUnitCombatBonus*>( this ) );

	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x19129441, CExecutorAdrenalineRush )


