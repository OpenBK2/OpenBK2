
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "CommonStates.h"
#include "StatesFactory.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CInBuildingStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CInBuildingStatesFactory );

	static CPtr<CInBuildingStatesFactory> pFactory;

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
class CSoldierRestInBuildingState : public IUnitState, public CStandartBehaviour
{
	OBJECT_BASIC_METHODS( CSoldierRestInBuildingState );

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
	static IUnitState *Instance( class CSoldier *pSoldier, class CBuilding *pBuilding );

	CSoldierRestInBuildingState()
		: pSoldier( 0 )
	{
	}

	CSoldierRestInBuildingState( class CSoldier *pSoldier );

	void SendUnitTo( class CBuilding *pBuilding );

	void Segment() override;
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	EUnitStateNames GetName() override
	{
		return EUSN_REST_IN_BUILDING;
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
class CSoldierAttackInBuildingState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CSoldierAttackInBuildingState );

	ZDATA
	CPtr<CSoldier> pSoldier;
	CPtr<CAIUnit> pEnemy;

	CPtr<CBasicGun> pGun;
	// стрельба окончена
	bool bFinish = {};
	// обязательно нужно прицелиться
	bool bAim = {};
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
		f.Add( 7, &nEnemyParty );
		f.Add( 8, &damageToEnemyUpdater );
		return 0;
	}

	//
	void AnalyzeCurrentState();

public:
	static IUnitState *Instance( class CSoldier *pSoldier, class CAIUnit *pEnemy );

	CSoldierAttackInBuildingState()
		: pSoldier( 0 )
	{
	}

	CSoldierAttackInBuildingState( class CSoldier *pSoldier, class CAIUnit *pEnemy );
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
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
