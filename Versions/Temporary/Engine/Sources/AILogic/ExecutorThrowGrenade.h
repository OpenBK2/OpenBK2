#pragma once
#include "Executor.h"
#include "Stats_B2_M1/SpecialAbilities.h"

class CAIUnit;

class CExecutorThrowGrenade : public CExecutor
{
	OBJECT_BASIC_METHODS(CExecutorThrowGrenade)
		ZDATA_(CExecutor)
	CPtr<CAIUnit> pUnit;  
	SAbilitySwitchState state;
	SAbilitySwitchState secondaryState;
	NDb::EUnitSpecialAbility eAbility;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&pUnit); f.Add(3,&state); f.Add(4,&secondaryState); f.Add(5,&eAbility); return 0; }

	void UpdateState( const SAbilitySwitchState &_state );
	const bool HasGrenades();
public:
	CExecutorThrowGrenade( CAIUnit *_pUnit, NDb::EUnitSpecialAbility _eAbility );
	CExecutorThrowGrenade() {  }

	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event );
	virtual bool IsExecutorValid() const
	{
		return IsValid( pUnit );
	}
	virtual void RegisterOnEvents( IExecutorContainer *pContainer );

};

