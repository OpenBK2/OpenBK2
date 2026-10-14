#pragma once

#include "DamageToEnemyUpdater.h"
#include "StatesFactory.h"
#include "UnitStates.h"

class CHelicopter;
class CBasicGun;
class CFormation;
class CSoldier;

class CHelicopterStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CHelicopterStatesFactory );

	static CPtr<CHelicopterStatesFactory> pFactory;

public:
	int operator&( IBinSaver &saver ) override
	{
		return 0;
	}

	static IStatesFactory *Instance();

	bool CanCommandBeExecuted( class CAICommand *pCommand ) override;
	struct IUnitState *ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand ) override;
	struct IUnitState *ProduceRestState( class CQueueUnit *pUnit ) override;

	friend class CStaticMembers;
};

class CHelicopterBaseState : public IUnitAttackingState
{
protected:
	ZDATA
	CPtr<CHelicopter> pHelicopter;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pHelicopter );
		return 0;
	}

	CHelicopterBaseState()
		: pHelicopter( 0 )
	{
	}

	CHelicopterBaseState( CHelicopter *pUnit )
		: pHelicopter( pUnit )
	{
	}

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	bool IsAttackingState() const override
	{
		return false;
	}

	bool IsAttacksUnit() const override
	{
		return false;
	}

	CAIUnit *GetTargetUnit() const override
	{
		return 0;
	}

	const CVec2 GetPurposePoint() const override;

protected:
	bool ShouldLeaveMap() const;
	void Finish();

	virtual void OnFinish() {}
};

class CHelicopterRestState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterRestState );
	ZDATA_( CHelicopterBaseState )

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CHelicopterBaseState *)this );
		return 0;
	}

public:
	CHelicopterRestState() {}

	CHelicopterRestState( CHelicopter *pUnit )
		: CHelicopterBaseState( pUnit )
	{
	}

	void Segment() override;

	bool IsRestState() const override
	{
		return true;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_REST;
	}
};

class CHelicopterMoveState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterMoveState );
	ZDATA_( CHelicopterBaseState )
	CVec2 vTarget;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	bool bScanTargets = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CHelicopterBaseState *)this );
		f.Add( 2, &vTarget );
		f.Add( 3, &bScanTargets );
		return 0;
	}

public:
	CHelicopterMoveState()
		: vTarget( VNULL2 )
		, bScanTargets( false )
	{
	}

	CHelicopterMoveState( CHelicopter *pUnit, const CVec2 &_vTarget, const bool _bScanTargets );
	void Segment() override;

	const CVec2 GetPurposePoint() const override
	{
		return vTarget;
	}

	EUnitStateNames GetName() override
	{
		return bScanTargets ? EUSN_SWARM : EUSN_MOVE;
	}
};

// Release one soldier at a time; finish the active squad before obeying a new order.
class CHelicopterUnloadState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterUnloadState );
	ZDATA_( CHelicopterBaseState )
	CVec2 vTarget;
	CPtr<CFormation> pUnload;
	bool bUnloadOneSquad = {};
	NTimer::STime timeNextDrop = {};
	std::vector<CPtr<CSoldier>> dropPassengers;
	std::vector<CVec3> dropPoints;
	int nNextPassenger = {};
	bool bFinishAfterSquad = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CHelicopterBaseState *)this );
		f.Add( 2, &vTarget );
		f.Add( 3, &pUnload );
		f.Add( 4, &bUnloadOneSquad );
		f.Add( 5, &timeNextDrop );
		f.Add( 6, &dropPassengers );
		f.Add( 7, &dropPoints );
		f.Add( 8, &nNextPassenger );
		f.Add( 9, &bFinishAfterSquad );
		return 0;
	}

	bool IsPassengerAboard( CSoldier *pSoldier ) const;
	CFormation *GetNextSquad() const;
	bool FindDropPoint( const CVec2 &vPreferred, CVec3 *pDropPoint ) const;
	bool PrepareSquad( CFormation *pSquad );

public:
	CHelicopterUnloadState()
		: vTarget( VNULL2 )
		, pUnload( 0 )
		, bUnloadOneSquad( false )
		, timeNextDrop( 0 )
		, nNextPassenger( 0 )
		, bFinishAfterSquad( false )
	{
	}

	CHelicopterUnloadState( CHelicopter *pUnit, const CVec2 &_vTarget, CFormation *_pUnload );
	void Segment() override;
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	const CVec2 GetPurposePoint() const override
	{
		return vTarget;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_LAND;
	}
};

class CHelicopterRotateState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterRotateState );
	ZDATA_( CHelicopterBaseState )
	CVec2 vTarget;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CHelicopterBaseState *)this );
		f.Add( 2, &vTarget );
		return 0;
	}

public:
	CHelicopterRotateState()
		: vTarget( VNULL2 )
	{
	}

	CHelicopterRotateState( CHelicopter *pUnit, const CVec2 &_vTarget );
	void Segment() override;

	const CVec2 GetPurposePoint() const override
	{
		return vTarget;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_TURN_TO_POINT;
	}
};

class CHelicopterAttackUnitState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterAttackUnitState );
	ZDATA_( CHelicopterBaseState )
	CPtr<CAIUnit> pTarget;
	CPtr<CBasicGun> pGun;
	bool bSwarmAttack = {};
	bool bAirModifierApplied = {};
	CDamageToEnemyUpdater damageUpdater;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CHelicopterBaseState *)this );
		f.Add( 2, &pTarget );
		f.Add( 3, &pGun );
		f.Add( 4, &bSwarmAttack );
		f.Add( 5, &bAirModifierApplied );
		f.Add( 6, &damageUpdater );
		return 0;
	}

	void ApplyAirModifier( const bool bApply );
	bool IsGunCompatible( CBasicGun *pCheckGun ) const;
	bool RefreshGun();
	void StartAvailableGuns();
	void StopAllGuns();

public:
	CHelicopterAttackUnitState()
		: pTarget( 0 )
		, pGun( 0 )
		, bSwarmAttack( false )
		, bAirModifierApplied( false )
	{
	}

	CHelicopterAttackUnitState( CHelicopter *pUnit, CAIUnit *pTarget, const bool _bSwarmAttack );

	//virtual ~CHelicopterAttackUnitState();
	void OnFinish() override;

	void Segment() override;
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	bool IsAttackingState() const override
	{
		return true;
	}

	const CVec2 GetPurposePoint() const override;

	bool IsAttacksUnit() const override
	{
		return true;
	}

	CAIUnit *GetTargetUnit() const override
	{
		return pTarget;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_ATTACK_UNIT;
	}
};

class CHelicopterLeaveState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterLeaveState );
	ZDATA_( CHelicopterBaseState )
	CVec2 vTarget;
	bool bScanTargets = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CHelicopterBaseState *)this );
		f.Add( 2, &vTarget );
		f.Add( 3, &bScanTargets );
		return 0;
	}

public:
	CHelicopterLeaveState()
		: vTarget( VNULL2 )
		, bScanTargets( false )
	{
	}

	CHelicopterLeaveState( CHelicopter *pUnit, const CVec2 &_vTarget, const bool _bScanTargets );
	void Segment() override;

	const CVec2 GetPurposePoint() const override
	{
		return vTarget;
	}

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override
	{
		return TSIR_NO_COMMAND_INCOMPATIBLE;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_PLANE_LEAVE;
	}
};

class CHelicopterFlyDeadState : public CHelicopterBaseState
{
	OBJECT_BASIC_METHODS( CHelicopterFlyDeadState );
	ZDATA_( CHelicopterBaseState )
	bool bStarted = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CHelicopterBaseState *)this );
		f.Add( 2, &bStarted );
		return 0;
	}

public:
	CHelicopterFlyDeadState()
		: bStarted( false )
	{
	}

	CHelicopterFlyDeadState( CHelicopter *pUnit );
	void Segment() override;

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override
	{
		return TSIR_NO_COMMAND_INCOMPATIBLE;
	}

	EUnitStateNames GetName() override
	{
		return EUSN_FLY_DEAD;
	}
};
