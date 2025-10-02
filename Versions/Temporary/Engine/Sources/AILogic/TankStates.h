#pragma once

#include "StatesFactory.h"
#include "UnitStates.h"
#include "AIUnit.h"
#include "../Stats_B2_M1/StatusUpdates.h"
#include "StatusUpdatesHelper.h"

class CTankStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CTankStatesFactory );
	
	static CPtr<CTankStatesFactory> pFactory;
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

class CMechUnitSupportFireState : public IUnitAttackingState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CMechUnitSupportFireState );

	enum ESupportFireStates { ESFS_TURNING, ESFS_WAITING, ESFS_SHOOT_UNIT, ESFS_SHOOT_OBJECT };
	enum { CHECK_TIME = 2000 };


	ZDATA_( CStatusUpdatesHelper )
	CPtr<CAIUnit> pUnit;
	ESupportFireStates eState;

	CPtr<CAIUnit> pFriend;
	CPtr<CAIUnit> pEnemy;
	CPtr<CStaticObject> pObj;
	CPtr<CBasicGun> pGun;

	NTimer::STime lastCheck;
	float fSearchRadius;
	bool bFinish;
	CPtr<CFormation> pFriendFormation;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&pFriend); f.Add(5,&pEnemy); f.Add(6,&pObj); f.Add(7,&pGun); f.Add(8,&lastCheck); f.Add(9,&fSearchRadius); f.Add(10,&bFinish); f.Add(11,&pFriendFormation); return 0; }
private:
	//
	void CheckArea();
	void FinishCommand();

public:
	static IUnitState* Instance( class CAIUnit *pUnit, class CAIUnit *pFriend );

	CMechUnitSupportFireState() : pUnit( 0 ) { }
	CMechUnitSupportFireState( class CAIUnit *_pUnit, class CAIUnit *_pFriend );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState(class CAICommand *pCommand);

	virtual EUnitStateNames GetName() { return EUSN_SUPPORT_FIRE; }

	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return pFriend->GetCenterPlain(); }

	virtual bool IsAttacksUnit() const;
	virtual class CAIUnit* GetTargetUnit() const;

};
