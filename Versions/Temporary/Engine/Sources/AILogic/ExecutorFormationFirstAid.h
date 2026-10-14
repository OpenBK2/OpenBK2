#pragma once
#include "ExecutorUnitBase.h"

class CFormation;

class CExecutorFormationFirstAid : public CExecutorUnitBase
{
	OBJECT_BASIC_METHODS( CExecutorFormationFirstAid )

	CPtr<CFormation> pFormation;

	// if 0 then ability must be switched off
	float OnAbilityActive() override;

	// if 1.0 then ability is ready to on
	float OnAbilityOff() override;

	void SwitchingOffEnd() override {}

	void SwitchingOffStart() override;

	void SwitchOnEnd() override;
	void SwitchOnStart( const class CAICommand *pCommand ) override;
	bool ActivateDuringDisable() override;

	CCommonUnit *GetUnit() override;

	const bool IsValidInternal() const;

public:
	CExecutorFormationFirstAid( CFormation *_pUnit );

	CExecutorFormationFirstAid() {}

	int Segment() override;
	bool NotifyEvent( const CExecutorEvent &event ) override;
	bool IsExecutorValid() const override;
	void RegisterOnEvents( IExecutorContainer *pContainer ) override;
	int operator&( IBinSaver &saver ) override;
};
