#pragma once
#include "ExecutorUnitBonus.h"

class CExecutorSmokeShots : public CExecutorUnitBonus
{
	OBJECT_BASIC_METHODS( CExecutorSmokeShots );

protected:

	void SwitchingOffEnd();
	void SwitchOnEnd();

public:
	CExecutorSmokeShots( CAIUnit *_pUnit );
	CExecutorSmokeShots() { }

	int operator&( IBinSaver &saver );
};

