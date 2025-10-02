#pragma once
#include "ExecutorUnitCombatBonus.h"

class CExecutorAmbush : public CExecutorUnitCombatBonus
{
	OBJECT_BASIC_METHODS( CExecutorAmbush );

protected:

	CExecutorUnitCombatBonus::EAbilityCombatReaction OnModeChange( const WORD oldModeFlags, const WORD newModeFlags );

public:
	CExecutorAmbush( CAIUnit *_pUnit );
	CExecutorAmbush() { }

	int operator&( IBinSaver &saver );
};
