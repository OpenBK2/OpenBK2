#pragma once
#include "ExecutorUnitCombatBonus.h"

class CExecutorCaution : public CExecutorUnitCombatBonus
{
	OBJECT_BASIC_METHODS( CExecutorCaution );

	ZDATA_(CExecutorUnitCombatBonus)

public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutorUnitCombatBonus*)this); return 0; }

protected:

	CExecutorUnitCombatBonus::EAbilityCombatReaction OnModeChange( const WORD oldModeFlags, const WORD newModeFlags );

	void SwitchingOffEnd();		// Used to change formations
	void SwitchOnEnd();				//

public:
	CExecutorCaution( CAIUnit *_pUnit );
	CExecutorCaution() { }

	void RegisterOnEvents( IExecutorContainer *pContainer );
	bool NotifyEvent( const CExecutorEvent &event );
};
