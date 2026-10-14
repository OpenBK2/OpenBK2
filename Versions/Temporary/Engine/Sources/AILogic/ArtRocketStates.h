#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "StatesFactory.h"
#include "UnitStates.h"

#include <cstdint>
class CArtillery;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtRocketStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CArtRocketStatesFactory );

	static CPtr<CArtRocketStatesFactory> pFactory;

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
class CArtRocketAttackGroundState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CArtRocketAttackGroundState );

	enum EAttackGroundState
	{
		EAGS_ROTATING,
		EAGS_FIRING
	};

	ZDATA
	CPtr<CArtillery> pArtillery;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	EAttackGroundState eState = {};
	CVec2 point;
	bool bFired = {};
	bool bFinished = {};
	uint16_t wDirToRotate = {};
	bool bSaidNoAmmo = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pArtillery );
		f.Add( 3, &eState );
		f.Add( 4, &point );
		f.Add( 5, &bFired );
		f.Add( 6, &bFinished );
		f.Add( 7, &wDirToRotate );
		f.Add( 8, &bSaidNoAmmo );
		return 0;
	}

public:
	static IUnitState *Instance( class CArtillery *pArtillery, const CVec2 &point );

	CArtRocketAttackGroundState()
		: pArtillery( 0 )
	{
	}

	CArtRocketAttackGroundState( class CArtillery *pArtillery, const CVec2 &point );

	void Segment() override;
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	bool IsAttackingState() const override
	{
		return true;
	}

	const CVec2 GetPurposePoint() const override
	{
		return point;
	}

	bool IsAttacksUnit() const override
	{
		return false;
	}

	class CAIUnit *GetTargetUnit() const override
	{
		return 0;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
