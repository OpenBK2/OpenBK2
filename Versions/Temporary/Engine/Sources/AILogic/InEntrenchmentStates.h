
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "StatesFactory.h"
#include "CommonStates.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIUnit;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CInEntrenchmentStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CInEntrenchmentStatesFactory );
	
	static CPtr<CInEntrenchmentStatesFactory> pFactory;
public:
	static IStatesFactory* Instance();
	int operator&( IBinSaver &saver )
	{
		return 0;
	}

	virtual struct IUnitState* ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand );
	virtual struct IUnitState* ProduceRestState( class CQueueUnit *pUnit );
	virtual bool CanCommandBeExecuted( class CAICommand *pCommand );
	
	// for Saving/Loading of static members
	friend class CStaticMembers;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSoldierRestInEntrenchmentState : public IUnitState, public CStandartBehaviour
{
	OBJECT_BASIC_METHODS( CSoldierRestInEntrenchmentState );

	ZDATA_(CStandartBehaviour)
	CPtr<CSoldier> pSoldier;	
	NTimer::STime startTime;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CStandartBehaviour*)this); f.Add(2,&pSoldier); f.Add(3,&startTime); return 0; }
public:
	static IUnitState* Instance( class CSoldier *pSoldier, class CEntrenchment *pEntrenchment );

	CSoldierRestInEntrenchmentState() : pSoldier( 0 ) { }
	CSoldierRestInEntrenchmentState( class CSoldier *pSoldier );

	void SetUnitTo( class CEntrenchment *pEntrenchment );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_REST_ENTRENCHMENT; }
	virtual bool IsRestState() const { return true; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

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
	bool bFinish;
	bool bAim;
	bool bSwarmAttack;
	int nEnemyParty;

	CDamageToEnemyUpdater damageToEnemyUpdater;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSoldier); f.Add(3,&pEnemy); f.Add(4,&pGun); f.Add(5,&bFinish); f.Add(6,&bAim); f.Add(7,&bSwarmAttack); f.Add(8,&nEnemyParty); f.Add(9,&damageToEnemyUpdater); return 0; }
	//
	void AnalyzeCurrentState();
	void FinishState();
public:
	static IUnitState* Instance( class CSoldier *pSoldier, class CAIUnit *pEnemy, const bool bSwarmAttack );

	CSoldierAttackInEtrenchState() : pSoldier( 0 ) { }
	CSoldierAttackInEtrenchState( class CSoldier *pSoldier, class CAIUnit *pEnemy, const bool bSwarmAttack );
	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	virtual bool IsAttacksUnit() const { return true; }
	virtual class CAIUnit* GetTargetUnit() const;
	virtual EUnitStateNames GetName() { return EUSN_ATTACK_IN_ENTRENCH; }
};


