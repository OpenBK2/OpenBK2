#pragma once

#include "ExecutorUnitCombatBonus.h"

#include <cstdint>

class CExecutorAdrenalineRush : public CExecutorUnitCombatBonus
{
	OBJECT_BASIC_METHODS( CExecutorAdrenalineRush );

protected:

	CExecutorUnitCombatBonus::EAbilityCombatReaction OnModeChange( const uint16_t oldModeFlags, const uint16_t newModeFlags );

public:
	CExecutorAdrenalineRush( CAIUnit *_pUnit );
	CExecutorAdrenalineRush() { }

	int operator&( IBinSaver &saver );
};

