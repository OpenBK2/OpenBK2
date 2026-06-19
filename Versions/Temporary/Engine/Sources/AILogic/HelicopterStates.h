#pragma once

#include "StatesFactory.h"
#include "UnitStates.h"
#include "DamageToEnemyUpdater.h"

class CHelicopter;
class CBasicGun;

class CHelicopterStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CHelicopterStatesFactory );

	static CPtr<CHelicopterStatesFactory> pFactory;
public:
	int operator&( IBinSaver &saver ) { return 0; }
	static IStatesFactory* Instance();

	virtual bool CanCommandBeExecuted( class CAICommand *pCommand );
	virtual struct IUnitState* ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand );
	virtual struct IUnitState* ProduceRestState( class CQueueUnit *pUnit );

	friend class CStaticMembers;
};

class CHelicopterBaseState : public IUnitAttackingState
{
protected:
	ZDATA
	CPtr<CHelicopter> pHelicopter;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pHelicopter); return 0; }
	CHelicopterBaseState() : pHelicopter( 0 ) { }
	CHelicopterBaseState( CHelicopter *pUnit ) : pHelicopter( pUnit ) { }

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual bool IsAttacksUnit() const { return false; }
	virtual CAIUnit* GetTargetUnit() const { return 0; }
	virtual const CVec2 GetPurposePoint() const;
protected:
	bool ShouldLeaveMap() const;
	void Finish();
	virtual void OnFinish() {}
};

class CHelicopterRestState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterRestState );
	ZDATA_(CHelicopterBaseState)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CHelicopterBaseState*)this); return 0; }
public:
	CHelicopterRestState() { }
	CHelicopterRestState( CHelicopter *pUnit ) : CHelicopterBaseState( pUnit ) { }
	virtual void Segment();
	virtual bool IsRestState() const { return true; }
	virtual EUnitStateNames GetName() { return EUSN_REST; }
};

class CHelicopterMoveState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterMoveState );
	ZDATA_(CHelicopterBaseState)
	CVec2 vTarget;
	bool bScanTargets;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CHelicopterBaseState*)this); f.Add(2,&vTarget); f.Add(3,&bScanTargets); return 0; }
public:
	CHelicopterMoveState() : vTarget( VNULL2 ), bScanTargets( false ) { }
	CHelicopterMoveState( CHelicopter *pUnit, const CVec2 &_vTarget, const bool _bScanTargets );
	virtual void Segment();
	virtual const CVec2 GetPurposePoint() const { return vTarget; }
	virtual EUnitStateNames GetName() { return bScanTargets ? EUSN_SWARM : EUSN_MOVE; }
};

class CHelicopterRotateState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterRotateState );
	ZDATA_(CHelicopterBaseState)
	CVec2 vTarget;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CHelicopterBaseState*)this); f.Add(2,&vTarget); return 0; }
public:
	CHelicopterRotateState() : vTarget( VNULL2 ) { }
	CHelicopterRotateState( CHelicopter *pUnit, const CVec2 &_vTarget );
	virtual void Segment();
	virtual const CVec2 GetPurposePoint() const { return vTarget; }
	virtual EUnitStateNames GetName() { return EUSN_TURN_TO_POINT; }
};

class CHelicopterAttackUnitState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterAttackUnitState );
	ZDATA_(CHelicopterBaseState)
	CPtr<CAIUnit> pTarget;
	CPtr<CBasicGun> pGun;
	bool bSwarmAttack;
	bool bAirModifierApplied;
	CDamageToEnemyUpdater damageUpdater;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CHelicopterBaseState*)this); f.Add(2,&pTarget); f.Add(3,&pGun); f.Add(4,&bSwarmAttack); f.Add(5,&bAirModifierApplied); f.Add(6,&damageUpdater); return 0; }
	void ApplyAirModifier( const bool bApply );
	bool IsGunCompatible( CBasicGun *pCheckGun ) const;
	bool RefreshGun();
	void StartAvailableGuns();
	void StopAllGuns();
public:
	CHelicopterAttackUnitState() : pTarget( 0 ), pGun( 0 ), bSwarmAttack( false ), bAirModifierApplied( false ) { }
	CHelicopterAttackUnitState( CHelicopter *pUnit, CAIUnit *pTarget, const bool _bSwarmAttack );
	
	//virtual ~CHelicopterAttackUnitState();
	void OnFinish() override;

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;
	virtual bool IsAttacksUnit() const { return true; }
	virtual CAIUnit* GetTargetUnit() const { return pTarget; }
	virtual EUnitStateNames GetName() { return EUSN_ATTACK_UNIT; }
};

class CHelicopterLeaveState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterLeaveState );
	ZDATA_(CHelicopterBaseState)
	CVec2 vTarget;
	bool bScanTargets;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CHelicopterBaseState*)this); f.Add(2,&vTarget); f.Add(3,&bScanTargets); return 0; }
public:
	CHelicopterLeaveState() : vTarget( VNULL2 ), bScanTargets( false ) { }
	CHelicopterLeaveState( CHelicopter *pUnit, const CVec2 &_vTarget, const bool _bScanTargets );
	virtual void Segment();
	virtual const CVec2 GetPurposePoint() const { return vTarget; }
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) { return TSIR_NO_COMMAND_INCOMPATIBLE; }
	virtual EUnitStateNames GetName() { return EUSN_PLANE_LEAVE; }
};

class CHelicopterFlyDeadState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterFlyDeadState );
	ZDATA_(CHelicopterBaseState)
	bool bStarted;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CHelicopterBaseState*)this); f.Add(2,&bStarted); return 0; }
public:
	CHelicopterFlyDeadState() : bStarted( false ) { }
	CHelicopterFlyDeadState( CHelicopter *pUnit );
	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) { return TSIR_NO_COMMAND_INCOMPATIBLE; }
	virtual EUnitStateNames GetName() { return EUSN_FLY_DEAD; }
};
