#pragma once

#include "executor.h"
#include "../Stats_B2_M1/AbilityActions.h"

struct SAISpecialAbilityUpdate;
class CUpdatableObj;
class CCommonUnit;

class CExecutorUnitBase :	public CExecutor
{
	ZDATA_(CExecutor)
	EUnitSpecialAbility eAbility;

	NTimer::STime									timeLastUpdate;
	float													fSpeedCoeff;							//Time multiplier (e.g. for MobileFortress ability)
	SSpecialAbilityInfo						lastSent;
	NTimer::STime									timeDisableGroup;					// this executor may be disabled by group run
	CPtr<SAISpecialAbilityUpdate> pUpdate;

	CDBPtr<NDb::SUnitSpecialAblityDesc> pAbilityDesc;

	SAbilitySwitchState stateBeforeDisable;									// to switch to that state after disable
	SAbilitySwitchState state;

	float fOldProgress;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&eAbility); f.Add(3,&timeLastUpdate); f.Add(4,&fSpeedCoeff); f.Add(5,&lastSent); f.Add(6,&timeDisableGroup); f.Add(7,&pUpdate); f.Add(8,&pAbilityDesc); f.Add(9,&stateBeforeDisable); f.Add(10,&state); f.Add(11,&fOldProgress); return 0; }
private:
	void RegisterOnEvents( IExecutorContainer *pContainer, const vector<EExecutorEventID> &events, const SExecutorEventParam &par );
	// return true if it must be called again during current segment
	bool UpdateAbilityState();
	void UpdateProgress( const SAbilitySwitchState _state, const float fParam );

protected:
	bool IsAutocast() const { return state.bAutocast; }

	// if 1.0 then ability must be switched off
	virtual float OnAbilityActive() = 0;

	// if 1.0 then ability is ready to on
	virtual float OnAbilityOff() = 0;

	virtual void SwitchingOffEnd() = 0;
	virtual void SwitchingOffStart() = 0;

	virtual void SwitchOnEnd() = 0;
	virtual void SwitchOnStart( const class CAICommand *pCommand ) = 0;

	// return true if activation is allowed
	virtual bool ActivateDuringDisable() = 0;
	virtual CCommonUnit *GetUnit() = 0;
	const NDb::SUnitSpecialAblityDesc *GetAbilityDesc() const { return pAbilityDesc; }
	void SetAbilityDesc( const NDb::SUnitSpecialAblityDesc *pDesc ) { pAbilityDesc = pDesc; }

	//Used if something happens, e.g. during switch-on (move while switching on Ambush)
	virtual void ForceRecharge();
	virtual void ForceActivate();
	virtual void ForceDeactivate();

	void SetSpeedCoeff( const float _fSpeedCoeff ) { fSpeedCoeff = _fSpeedCoeff; }

	void RegisterOnUnitEvents( IExecutorContainer *pContainer, const vector<EExecutorEventID> &unitEvents, const int nUnitID );
	const EUnitSpecialAbility GetAbility() const { return eAbility; }
	void Disable();
	void SetAutocast( const bool _bAutocast );
	bool GetAutocast() const { return state.bAutocast; }
	CExecutorUnitBase( const EExecutorTypeID _eTypeID, const int _nNextTime, const EUnitSpecialAbility eAbility );
	CExecutorUnitBase() {}
public:
	
	bool NotifyEvent( const CExecutorEvent &event ); // !!!! only events that is for this executor must go here (no checks inside)
	
	int Segment();

	const EAbilitySwitchState GetState() const { return EAbilitySwitchState( state.eState ); }
};

