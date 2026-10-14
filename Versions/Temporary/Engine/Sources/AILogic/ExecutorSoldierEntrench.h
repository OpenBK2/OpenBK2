#pragma once

#include "ExecutorUnitBase.h"

class CAIUnit;

class CExecutorSoldierEntrench : public CExecutorUnitBase
{
	OBJECT_NOCOPY_METHODS( CExecutorSoldierEntrench )

	ZDATA_( CExecutorUnitBase )
	CPtr<CAIUnit> pUnit;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	float fEntrenchTimeCoeff = {}; //Used for Mobile Fortress ability
public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExecutorUnitBase *)this );
		f.Add( 2, &pUnit );
		f.Add( 3, &fEntrenchTimeCoeff );
		return 0;
	}

	// if 0 then ability must be switched off
	float OnAbilityActive() override;

	// if 1.0 then ability is ready to on
	float OnAbilityOff() override;

	void SwitchingOffEnd() override;
	void SwitchingOffStart() override;

	void SwitchOnEnd() override;
	void SwitchOnStart( const class CAICommand *pCommand ) override;
	bool ActivateDuringDisable() override;

	CCommonUnit *GetUnit() override;

	const bool IsEntrenchAllowed() const;
	const bool IsValidInternal() const;

public:
	CExecutorSoldierEntrench( CAIUnit *_pUnit );

	CExecutorSoldierEntrench()
		: fEntrenchTimeCoeff( 1.0f )
	{
	}

	~CExecutorSoldierEntrench() override;

	int Segment() override;
	bool NotifyEvent( const CExecutorEvent &event ) override;
	bool IsExecutorValid() const override;
	void RegisterOnEvents( IExecutorContainer *pContainer ) override;
};
