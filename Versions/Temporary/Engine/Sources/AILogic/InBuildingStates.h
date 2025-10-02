
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "StatesFactory.h"
#include "CommonStates.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CInBuildingStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CInBuildingStatesFactory );
	
	static CPtr<CInBuildingStatesFactory> pFactory;
public:
	static IStatesFactory* Instance();
	int operator&( IBinSaver &saver )
	{
		return 0;
	}

	virtual interface IUnitState* ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand );
	virtual interface IUnitState* ProduceRestState( class CQueueUnit *pUnit );
	virtual bool CanCommandBeExecuted( class CAICommand *pCommand );
	
	// for Saving/Loading of static members
	friend class CStaticMembers;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSoldierRestInBuildingState : public IUnitState, public CStandartBehaviour
{
	OBJECT_BASIC_METHODS( CSoldierRestInBuildingState );
	
	ZDATA_(CStandartBehaviour)
	CPtr<CSoldier> pSoldier;
	NTimer::STime startTime;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CStandartBehaviour*)this); f.Add(2,&pSoldier); f.Add(3,&startTime); return 0; }

public:
	static IUnitState* Instance( class CSoldier *pSoldier, class CBuilding *pBuilding );

	CSoldierRestInBuildingState() : pSoldier( 0 ) { }
	CSoldierRestInBuildingState( class CSoldier *pSoldier );
	
	void SendUnitTo( class CBuilding *pBuilding );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_REST_IN_BUILDING; }
	virtual bool IsRestState() const { return true; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

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
	bool bFinish;
	// обязательно нужно прицелиться
	bool bAim;
	int nEnemyParty;
	CDamageToEnemyUpdater damageToEnemyUpdater;

	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSoldier); f.Add(3,&pEnemy); f.Add(4,&pGun); f.Add(5,&bFinish); f.Add(6,&bAim); f.Add(7,&nEnemyParty); f.Add(8,&damageToEnemyUpdater); return 0; }

	//
	void AnalyzeCurrentState();
public:
	static IUnitState* Instance( class CSoldier *pSoldier, class CAIUnit *pEnemy );

	CSoldierAttackInBuildingState() : pSoldier( 0 ) { }
	CSoldierAttackInBuildingState( class CSoldier *pSoldier, class CAIUnit *pEnemy );
	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	virtual bool IsAttacksUnit() const { return true; }
	virtual class CAIUnit* GetTargetUnit() const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

