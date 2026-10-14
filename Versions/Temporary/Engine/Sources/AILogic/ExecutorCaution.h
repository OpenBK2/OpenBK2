#pragma once

#include "ExecutorUnitCombatBonus.h"

#include <cstdint>

class CExecutorCaution : public CExecutorUnitCombatBonus
{
	OBJECT_BASIC_METHODS( CExecutorCaution );

	ZDATA_( CExecutorUnitCombatBonus )

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExecutorUnitCombatBonus *)this );
		return 0;
	}

protected:
	CExecutorUnitCombatBonus::EAbilityCombatReaction OnModeChange( const uint16_t oldModeFlags,
	                                                               const uint16_t newModeFlags ) override;

	void SwitchingOffEnd() override; // Used to change formations
	void SwitchOnEnd() override;     //

public:
	CExecutorCaution( CAIUnit *_pUnit );

	CExecutorCaution() {}

	void RegisterOnEvents( IExecutorContainer *pContainer ) override;
	bool NotifyEvent( const CExecutorEvent &event ) override;
};
