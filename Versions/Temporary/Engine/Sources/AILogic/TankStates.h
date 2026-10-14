#pragma once

#include "AIUnit.h"
#include "StatesFactory.h"
#include "Stats_B2_M1/StatusUpdates.h"
#include "StatusUpdatesHelper.h"
#include "UnitStates.h"

class CTankStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CTankStatesFactory );

	static CPtr<CTankStatesFactory> pFactory;

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

class CMechUnitSupportFireState : public IUnitAttackingState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CMechUnitSupportFireState );

	enum ESupportFireStates
	{
		ESFS_TURNING,
		ESFS_WAITING,
		ESFS_SHOOT_UNIT,
		ESFS_SHOOT_OBJECT
	};

	enum
	{
		CHECK_TIME = 2000
	};

	ZDATA_( CStatusUpdatesHelper )
	CPtr<CAIUnit> pUnit;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	ESupportFireStates eState = {};

	CPtr<CAIUnit> pFriend;
	CPtr<CAIUnit> pEnemy;
	CPtr<CStaticObject> pObj;
	CPtr<CBasicGun> pGun;

	NTimer::STime lastCheck = {};
	float fSearchRadius = {};
	bool bFinish = {};
	CPtr<CFormation> pFriendFormation;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CStatusUpdatesHelper *)this );
		f.Add( 2, &pUnit );
		f.Add( 3, &eState );
		f.Add( 4, &pFriend );
		f.Add( 5, &pEnemy );
		f.Add( 6, &pObj );
		f.Add( 7, &pGun );
		f.Add( 8, &lastCheck );
		f.Add( 9, &fSearchRadius );
		f.Add( 10, &bFinish );
		f.Add( 11, &pFriendFormation );
		return 0;
	}

private:
	//
	void CheckArea();
	void FinishCommand();

public:
	static IUnitState *Instance( class CAIUnit *pUnit, class CAIUnit *pFriend );

	CMechUnitSupportFireState()
		: pUnit( 0 )
	{
	}

	CMechUnitSupportFireState( class CAIUnit *_pUnit, class CAIUnit *_pFriend );

	void Segment() override;
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	EUnitStateNames GetName() override
	{
		return EUSN_SUPPORT_FIRE;
	}

	bool IsAttackingState() const override
	{
		return true;
	}

	const CVec2 GetPurposePoint() const override
	{
		return pFriend->GetCenterPlain();
	}

	bool IsAttacksUnit() const override;
	class CAIUnit *GetTargetUnit() const override;
};
