
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "StatesFactory.h"
#include "UnitStates.h"
class CArtillery;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtRocketStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CArtRocketStatesFactory );

	static CPtr<CArtRocketStatesFactory> pFactory;
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
class CArtRocketAttackGroundState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CArtRocketAttackGroundState );
	enum EAttackGroundState { EAGS_ROTATING, EAGS_FIRING };
	
	ZDATA
	CPtr<CArtillery> pArtillery;
	EAttackGroundState eState;
	CVec2 point;
	bool bFired;
	bool bFinished;
	WORD wDirToRotate;
	bool bSaidNoAmmo;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pArtillery); f.Add(3,&eState); f.Add(4,&point); f.Add(5,&bFired); f.Add(6,&bFinished); f.Add(7,&wDirToRotate); f.Add(8,&bSaidNoAmmo); return 0; }

public:
	static IUnitState* Instance( class CArtillery *pArtillery, const CVec2 &point );

	CArtRocketAttackGroundState() : pArtillery( 0 ) { }
	CArtRocketAttackGroundState( class CArtillery *pArtillery, const CVec2 &point );

	virtual void Segment();
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return point; }

	virtual bool IsAttacksUnit() const { return false; }
	virtual class CAIUnit* GetTargetUnit() const { return 0; }
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

