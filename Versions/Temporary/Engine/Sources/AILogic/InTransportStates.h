
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "StatesFactory.h"
#include "UnitStates.h"
class CSoldier;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CInTransportStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CInTransportStatesFactory );

	static CPtr<CInTransportStatesFactory> pFactory;

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
class CSoldierRestOnBoardState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierRestOnBoardState );
	ZDATA
	CPtr<CSoldier> pSoldier;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pSoldier );
		return 0;
	}

public:
	static IUnitState *Instance( class CSoldier *pSoldier, class CMilitaryCar *pTransport );

	CSoldierRestOnBoardState()
		: pSoldier( 0 )
	{
	}

	CSoldierRestOnBoardState( class CSoldier *pSoldier, class CMilitaryCar *pTransport );

	void Segment() override;

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) override;

	EUnitStateNames GetName() override
	{
		return EUSN_REST_ON_BOARD;
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
