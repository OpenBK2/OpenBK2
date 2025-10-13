#pragma once

#include "ExecutorUnitCombatBonus.h"

#include <cstdint>

class CExecutorAmbush : public CExecutorUnitCombatBonus
{
	OBJECT_BASIC_METHODS( CExecutorAmbush );

protected:

	CExecutorUnitCombatBonus::EAbilityCombatReaction OnModeChange( const uint16_t oldModeFlags, const uint16_t newModeFlags );

public:
	CExecutorAmbush( CAIUnit *_pUnit );
	CExecutorAmbush() { }

	int operator&( IBinSaver &saver );
};

