#pragma once

#include "ExecutorUnitCombatBonus.h"

#include <cstdint>

class CExecutorSpyMode : public CExecutorUnitCombatBonus
{
	OBJECT_BASIC_METHODS( CExecutorSpyMode );

	CDBPtr<NDb::SHPObjectRPGStats> pEnemyStats;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	bool bMasked = {};

	void SwitchModel( const NDb::SHPObjectRPGStats *pStats );

protected:
	CExecutorUnitCombatBonus::EAbilityCombatReaction OnModeChange( const uint16_t oldModeFlags,
	                                                               const uint16_t newModeFlags ) override;
	void SwitchingOffEnd() override;
	void SwitchOnEnd() override;

public:
	CExecutorSpyMode( CAIUnit *_pUnit );

	CExecutorSpyMode() {}

	void RegisterOnEvents( IExecutorContainer *pContainer ) override;
	bool NotifyEvent( const CExecutorEvent &event ) override;

	int operator&( IBinSaver &saver ) override;
};
