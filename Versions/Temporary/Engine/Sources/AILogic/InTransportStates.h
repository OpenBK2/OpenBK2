#ifndef __IN_TRANSPORT_STATES_H__
#define __IN_TRANSPORT_STATES_H__

#pragma ONCE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "UnitStates.h"
#include "StatesFactory.h"
class CSoldier;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CInTransportStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CInTransportStatesFactory );
	
	static CPtr<CInTransportStatesFactory> pFactory;
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
class CSoldierRestOnBoardState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierRestOnBoardState );
	ZDATA
	CPtr<CSoldier> pSoldier;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSoldier); return 0; }
public:
	static IUnitState* Instance( class CSoldier *pSoldier, class CMilitaryCar* pTransport );

	CSoldierRestOnBoardState() : pSoldier( 0 ) { }
	CSoldierRestOnBoardState( class CSoldier *pSoldier, class CMilitaryCar* pTransport );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_REST_ON_BOARD; }
	virtual bool IsRestState() const { return true; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

	// for Saving/Loading of static members
	friend class CStaticMembers;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __IN_TRANSPORT_STATES_H__

