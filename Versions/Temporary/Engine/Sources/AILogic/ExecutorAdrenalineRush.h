#pragma once
#include "ExecutorUnitCombatBonus.h"

class CExecutorAdrenalineRush : public CExecutorUnitCombatBonus
{
	OBJECT_BASIC_METHODS( CExecutorAdrenalineRush );

protected:

	CExecutorUnitCombatBonus::EAbilityCombatReaction OnModeChange( const WORD oldModeFlags, const WORD newModeFlags );

public:
	CExecutorAdrenalineRush( CAIUnit *_pUnit );
	CExecutorAdrenalineRush() { }

	int operator&( IBinSaver &saver );
};

