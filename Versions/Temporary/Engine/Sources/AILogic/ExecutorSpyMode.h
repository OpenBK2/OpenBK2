#pragma once
#include "ExecutorUnitCombatBonus.h"

class CExecutorSpyMode : public CExecutorUnitCombatBonus
{
	OBJECT_BASIC_METHODS( CExecutorSpyMode );

	CDBPtr<NDb::SHPObjectRPGStats> pEnemyStats;
	bool bMasked;

	void SwitchModel( const NDb::SHPObjectRPGStats *pStats );

protected:

	CExecutorUnitCombatBonus::EAbilityCombatReaction OnModeChange( const WORD oldModeFlags, const WORD newModeFlags );
	void SwitchingOffEnd();
	void SwitchOnEnd();
public:
	CExecutorSpyMode( CAIUnit *_pUnit );
	CExecutorSpyMode() { }

	void RegisterOnEvents( IExecutorContainer *pContainer );
	bool NotifyEvent( const CExecutorEvent &event );

	int operator&( IBinSaver &saver );
};
