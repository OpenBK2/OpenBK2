#pragma once
#include "AIUnit.h"
#include "ExecutorUnitBase.h"
#include "Stats_B2_M1/StatusUpdates.h"

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

	ZDATA_( CExecutorUnitBase )
protected:
	CPtr<CAIUnit> pUnit;

private:
	CDBPtr<NDb::SUnitStatsModifier> pModifier;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	NTimer::STime timeLastUpdate = {};
	bool bBonusApplied = {};
	EUnitStatus eStatus = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExecutorUnitBase *)this );
		f.Add( 2, &pUnit );
		f.Add( 3, &pModifier );
		f.Add( 4, &timeLastUpdate );
		f.Add( 5, &bBonusApplied );
		f.Add( 6, &eStatus );
		return 0;
	}

protected:
	float OnAbilityActive() override;

	float OnAbilityOff() override;

	void SwitchingOffEnd() override;
	void SwitchingOffStart() override;

	void SwitchOnEnd() override;
	void SwitchOnStart( const class CAICommand *pCommand ) override;

	void ForceActivate() override;

	// return true if activation is allowed
	bool ActivateDuringDisable() override
	{
		return false;
	}

	CCommonUnit *GetUnit() override
	{
		return pUnit.GetPtr();
	}

	CAIUnit *GetAIUnit()
	{
		return pUnit.GetPtr();
	}

public:
	CExecutorUnitBonus( EUnitSpecialAbility eAbility, CAIUnit *_pUnit, EExecutorTypeID eTID = TID_BONUS );

	CExecutorUnitBonus() {}

	void RegisterOnEvents( IExecutorContainer *pContainer ) override;
	bool NotifyEvent( const CExecutorEvent &event ) override;

	bool IsExecutorValid() const override
	{
		return IsValid( pUnit );
	}
};
