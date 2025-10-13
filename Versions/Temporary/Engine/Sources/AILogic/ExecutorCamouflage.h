#pragma once

#include "ExecutorUnitCombatBonus.h"

#include <cstdint>

class CExecutorCamouflage : public CExecutorUnitCombatBonus
{
	OBJECT_BASIC_METHODS( CExecutorCamouflage );

	ZDATA_(CExecutorUnitCombatBonus)
		NTimer::STime nextCheckTime;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutorUnitCombatBonus*)this); f.Add(2,&nextCheckTime); return 0; }

protected:

	CExecutorUnitCombatBonus::EAbilityCombatReaction OnModeChange( const uint16_t oldModeFlags, const uint16_t newModeFlags );
	void SwitchingOffEnd();
	void SwitchOnEnd();
public:
	CExecutorCamouflage( CAIUnit *_pUnit );
	CExecutorCamouflage() { }

	virtual int Segment();
};

