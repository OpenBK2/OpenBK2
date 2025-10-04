#pragma once

#include "Executor.h"
#include "Stats_B2_M1/AbilityActions.h"

class CSoldier;
class CAIUnit;
struct SAISpecialAbilityUpdate;

class CExecutorSniperCamouflage : public CExecutor
{
	OBJECT_BASIC_METHODS(CExecutorSniperCamouflage)
		ZDATA_(CExecutor)
	CPtr<CAIUnit> pUnit;  
	int nEnemysAround;											// count of enemies around, that can see unit
	bool bWasSomeAction;										// camouflated unit performed some action 
	float fCamoflage;												// 0 - fully visible, 1.0 - fully camoflated.
	EAbilitySwitchState eStateBeforeDisable; // to switch to that state after disable
	EAbilitySwitchState eState;
	NTimer::STime timeDisableGroup;					// this executor may be disabled by group run
	
	NTimer::STime timeLastUpdate;
	SSpecialAbilityInfo lastSent;
	CPtr<SAISpecialAbilityUpdate> pUpdate;
	bool bMoving;
	bool bAdvanced;
	NTimer::STime nBeginActionTime;
	NTimer::STime nLastActionTime;
	bool bInAction;
	bool bStartCounting;
	NDb::EUnitSpecialAbility eAbilityType;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&pUnit); f.Add(3,&nEnemysAround); f.Add(4,&bWasSomeAction); f.Add(5,&fCamoflage); f.Add(6,&eStateBeforeDisable); f.Add(7,&eState); f.Add(8,&timeDisableGroup); f.Add(9,&timeLastUpdate); f.Add(10,&lastSent); f.Add(11,&pUpdate); f.Add(12,&bMoving); f.Add(13,&bAdvanced); f.Add(14,&nBeginActionTime); f.Add(15,&nLastActionTime); f.Add(16,&bInAction); f.Add(17,&bStartCounting); f.Add(18,&eAbilityType); return 0; }
	bool IsExecutorValidInternal() const;
	int UpdateCamoflage( const EUnitSpecialAbility eAbility );
	void UpdateState( const EAbilitySwitchState eState, const float fParam );
	
	void UnCamoUnit();
	void CamoUnit();
	bool IsCamoflageAbility( const EUnitSpecialAbility eAbility );
public:
	CExecutorSniperCamouflage( CAIUnit *_pUnit, bool _bAdvanced );
	CExecutorSniperCamouflage() {  }

	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event );
	virtual bool IsExecutorValid() const { return IsExecutorValidInternal(); }
	virtual void RegisterOnEvents( IExecutorContainer *pContainer );
};


