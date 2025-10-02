#pragma once
#include "ExecutorUnitBonus.h"

class CExecutorLinkedGrenades : public CExecutorUnitBonus
{
	OBJECT_BASIC_METHODS( CExecutorLinkedGrenades );

protected:

	void SwitchingOffEnd();
	void SwitchOnEnd();

public:
	CExecutorLinkedGrenades( CAIUnit *_pUnit );
	CExecutorLinkedGrenades() { }

	int operator&( IBinSaver &saver );
};

