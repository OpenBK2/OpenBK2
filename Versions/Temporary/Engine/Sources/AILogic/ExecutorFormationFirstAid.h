#pragma once
#include "ExecutorUnitBase.h"

class CFormation;

class CExecutorFormationFirstAid : public CExecutorUnitBase
{
	OBJECT_BASIC_METHODS( CExecutorFormationFirstAid )

	CPtr<CFormation> pFormation;  

	// if 0 then ability must be switched off
	float OnAbilityActive();

	// if 1.0 then ability is ready to on
	float OnAbilityOff();
	void SwitchingOffEnd() { }


	void SwitchingOffStart();

	void SwitchOnEnd();
	void SwitchOnStart( const class CAICommand *pCommand );
	bool ActivateDuringDisable();

	CCommonUnit *GetUnit();

	const bool IsValidInternal() const;
public:
	CExecutorFormationFirstAid( CFormation *_pUnit );
	CExecutorFormationFirstAid() {  }

	int Segment();
	bool NotifyEvent( const CExecutorEvent &event );
	bool IsExecutorValid() const;
	void RegisterOnEvents( IExecutorContainer *pContainer );
	int operator&( IBinSaver &saver );
};

