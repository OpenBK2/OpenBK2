
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "CommonStates.h"
#include "StatesFactory.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIUnit;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CInEntrenchmentStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CInEntrenchmentStatesFactory );

	static CPtr<CInEntrenchmentStatesFactory> pFactory;

public:
	static IStatesFactory *Instance();

	int operator&( IBinSaver &saver ) override
	{
		return 0;
	}

	struct IUnitState *ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand ) override;
	struct IUnitState *ProduceRestState( class CQueueUnit *pUnit ) override;
	bool CanCommandBeExecuted( class CAICommand *pCommand ) override;

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSoldierRestInEntrenchmentState : public IUnitState, public CStandartBehaviour
{
	OBJECT_BASIC_METHODS( CSoldierRestInEntrenchmentState );

	ZDATA_( CStandartBehaviour )
	CPtr<CSoldier> pSoldier;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	NTimer::STime startTime = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CStandartBehaviour *)this );
		f.Add( 2, &pSoldier );
		f.Add( 3, &startTime );
		return 0;
	}

public:
	static IUnitState *Instance( class CSoldier *pSoldier, class CEntrenchment *pEntrenchment );

	CSoldierRestInEntrenchmentState()
		: pSoldier( 0 )
	{
	}

	CSoldierRestInEntrenchmentState( class CSoldier *pSoldier );

	void SetUnitTo( class CEntrenchment *pEntrenchment );

	void Segment() override;
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	EUnitStateNames GetName() override
	{
		return EUSN_REST_ENTRENCHMENT;
	}

	bool IsRestState() const override
	{
		return true;
	}

	bool IsAttackingState() const override
	{
		return false;
	}

	const CVec2 GetPurposePoint() const override;

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSoldierAttackInEtrenchState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CSoldierAttackInEtrenchState );

	ZDATA
	CPtr<CSoldier> pSoldier;
	CPtr<CAIUnit> pEnemy;

	CPtr<CBasicGun> pGun;
	bool bFinish = {};
	bool bAim = {};
	bool bSwarmAttack = {};
	int nEnemyParty = {};

	CDamageToEnemyUpdater damageToEnemyUpdater;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pSoldier );
		f.Add( 3, &pEnemy );
		f.Add( 4, &pGun );
		f.Add( 5, &bFinish );
		f.Add( 6, &bAim );
		f.Add( 7, &bSwarmAttack );
		f.Add( 8, &nEnemyParty );
		f.Add( 9, &damageToEnemyUpdater );
		return 0;
	}

	//
	void AnalyzeCurrentState();
	void FinishState();

public:
	static IUnitState *Instance( class CSoldier *pSoldier, class CAIUnit *pEnemy, const bool bSwarmAttack );

	CSoldierAttackInEtrenchState()
		: pSoldier( 0 )
	{
	}

	CSoldierAttackInEtrenchState( class CSoldier *pSoldier, class CAIUnit *pEnemy, const bool bSwarmAttack );
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
	class CAIUnit *GetTargetUnit() const override;

	EUnitStateNames GetName() override
	{
		return EUSN_ATTACK_IN_ENTRENCH;
	}
};
