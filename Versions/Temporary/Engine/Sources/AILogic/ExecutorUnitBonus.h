#pragma once
#include "ExecutorUnitBase.h"
#include "AIUnit.h"
#include "../Stats_B2_M1/StatusUpdates.h"

// CExecutorUnitBonus
//
// Provides ability increases/decreases via UnitStatsModifier
// Adds modifier on SwitchOnEnd
// Removes on       SwitchOffEnd
// Is registered on events: ACTIVATE, DEACTIVATE, ENABLE, DISABLE
//
// Can be used to derive another class (e.g. Ambush) with more complex functionality

class CExecutorUnitBonus : public CExecutorUnitBase
{
	OBJECT_BASIC_METHODS( CExecutorUnitBonus );

	ZDATA_(CExecutorUnitBase)
protected:
	CPtr<CAIUnit>										pUnit;
private:
	CDBPtr<NDb::SUnitStatsModifier> pModifier;
	NTimer::STime										timeLastUpdate;
	bool														bBonusApplied;
	EUnitStatus											eStatus;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutorUnitBase*)this); f.Add(2,&pUnit); f.Add(3,&pModifier); f.Add(4,&timeLastUpdate); f.Add(5,&bBonusApplied); f.Add(6,&eStatus); return 0; }
protected:

	float OnAbilityActive();

	float OnAbilityOff();

	void SwitchingOffEnd();
	void SwitchingOffStart();

	void SwitchOnEnd();
	void SwitchOnStart( const class CAICommand *pCommand );

	virtual void ForceActivate();

	// return true if activation is allowed
	bool ActivateDuringDisable() { return false; }
	CCommonUnit *GetUnit() { return pUnit.GetPtr(); }
	CAIUnit *GetAIUnit() { return pUnit.GetPtr(); }

public:
	CExecutorUnitBonus( EUnitSpecialAbility eAbility, CAIUnit *_pUnit, EExecutorTypeID eTID = TID_BONUS );
	CExecutorUnitBonus() { }

	void RegisterOnEvents( IExecutorContainer *pContainer );
	bool NotifyEvent( const CExecutorEvent &event );
	bool IsExecutorValid() const { return IsValid( pUnit ); }
};

