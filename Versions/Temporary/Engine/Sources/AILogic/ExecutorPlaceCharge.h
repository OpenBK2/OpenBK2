#pragma once
#include "Executor.h"
#include "Soldier.h"
#include "..\Stats_B2_M1\SpecialAbilities.h"

class CExecutorPlaceCharge : public CExecutor
{
	OBJECT_BASIC_METHODS(CExecutorPlaceCharge)

	ZDATA_(CExecutor)
	CPtr<CSoldier> pSoldier;  
	EAbilitySwitchState eState;
	NDb::EUnitSpecialAbility eAbility;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&pSoldier); f.Add(3,&eState); f.Add(4,&eAbility); return 0; }
	void UpdateState( const EAbilitySwitchState eState );

public:
	CExecutorPlaceCharge( CAIUnit *_pSoldier, NDb::EUnitSpecialAbility _eAbility );
	CExecutorPlaceCharge() {  }

	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event );
	virtual bool IsExecutorValid() const
	{
		return IsValid( pSoldier );
	}
	virtual void RegisterOnEvents( IExecutorContainer *pContainer );

};
