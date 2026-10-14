#pragma once

#include "ExecutorUnitCombatBonus.h"

#include <cstdint>

class CExecutorCamouflage : public CExecutorUnitCombatBonus
{
	OBJECT_BASIC_METHODS( CExecutorCamouflage );

	ZDATA_( CExecutorUnitCombatBonus )
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	NTimer::STime nextCheckTime = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExecutorUnitCombatBonus *)this );
		f.Add( 2, &nextCheckTime );
		return 0;
	}

protected:
	CExecutorUnitCombatBonus::EAbilityCombatReaction OnModeChange( const uint16_t oldModeFlags,
	                                                               const uint16_t newModeFlags ) override;
	void SwitchingOffEnd() override;
	void SwitchOnEnd() override;

public:
	CExecutorCamouflage( CAIUnit *_pUnit );

	CExecutorCamouflage() {}

	int Segment() override;
};
