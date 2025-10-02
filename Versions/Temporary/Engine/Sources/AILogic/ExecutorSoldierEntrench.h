#pragma once

#include "ExecutorUnitBase.h"

class CAIUnit;

class CExecutorSoldierEntrench : public CExecutorUnitBase
{
	OBJECT_NOCOPY_METHODS( CExecutorSoldierEntrench )

	ZDATA_(CExecutorUnitBase)
	CPtr<CAIUnit> pUnit;  
	float					fEntrenchTimeCoeff;					//Used for Mobile Fortress ability
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutorUnitBase*)this); f.Add(2,&pUnit); f.Add(3,&fEntrenchTimeCoeff); return 0; }
	// if 0 then ability must be switched off
	float OnAbilityActive();

	// if 1.0 then ability is ready to on
	float OnAbilityOff();

	void SwitchingOffEnd();
	void SwitchingOffStart();

	void SwitchOnEnd();
	void SwitchOnStart( const class CAICommand *pCommand );
	bool ActivateDuringDisable();

	CCommonUnit *GetUnit();

	const bool IsEntrenchAllowed() const;
	const bool IsValidInternal() const;
public:
	CExecutorSoldierEntrench( CAIUnit *_pUnit );
	CExecutorSoldierEntrench() {  }
	~CExecutorSoldierEntrench();

	int Segment();
	bool NotifyEvent( const CExecutorEvent &event );
	bool IsExecutorValid() const;
	void RegisterOnEvents( IExecutorContainer *pContainer );
};

