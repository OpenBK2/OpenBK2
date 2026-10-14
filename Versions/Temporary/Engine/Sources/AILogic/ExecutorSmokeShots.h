#pragma once
#include "ExecutorUnitBonus.h"

class CExecutorSmokeShots : public CExecutorUnitBonus
{
	OBJECT_BASIC_METHODS( CExecutorSmokeShots );

protected:
	void SwitchingOffEnd() override;
	void SwitchOnEnd() override;

public:
	CExecutorSmokeShots( CAIUnit *_pUnit );

	CExecutorSmokeShots() {}

	int operator&( IBinSaver &saver ) override;
};
