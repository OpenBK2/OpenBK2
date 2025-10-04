#include "stdafx.h"

#include "ExecutorAmbush.h"

CExecutorAmbush::CExecutorAmbush( CAIUnit *_pUnit	) :
CExecutorUnitCombatBonus ( NDb::ABILITY_AMBUSH, _pUnit, TID_AMBUSH )
{
}

CExecutorUnitCombatBonus::EAbilityCombatReaction CExecutorAmbush::OnModeChange( const WORD oldModeFlags, const WORD newModeFlags )
{
	if ( GetState() == EASS_ACTIVE && newModeFlags )
	{
		return EACR_FORCE_DEACTIVATE;
	}
	else if ( GetState() == EASS_SWITCHING_ON && newModeFlags )
	{
		return EACR_FORCE_RECHARGE;
	}
	return EACR_IGNORE;
}

int CExecutorAmbush::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CExecutorUnitCombatBonus*>( this ) );

	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x19124C00, CExecutorAmbush )


