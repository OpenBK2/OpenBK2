#pragma once

#include "CommonUnit.h"
#include "Executor.h"

struct SUnitListInfo 
{
	ZDATA
	CPtr<CCommonUnit> pUnit;
	int nCell;

	bool bCanGo;		// юнит может двигаться
	bool bGoAway;		// юнит должен свалить (пропустить задних)
	bool bAtPlace;	// притопал на место, первая часть построения для юнита закончена
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&nCell); f.Add(4,&bCanGo); f.Add(5,&bGoAway); f.Add(6,&bAtPlace); return 0; }
public:
	SUnitListInfo() : pUnit( 0 ), bCanGo( false ), bGoAway( false ), bAtPlace( false ), nCell( -1 ) {}
	SUnitListInfo( CCommonUnit *_pUnit ) : pUnit( _pUnit ), bCanGo( false ), bGoAway( false ), bAtPlace( false ), nCell( -1 ) {}
};

struct SPriorityGroupPosition
{
	ZDATA
	int nCount;
	CVec2 vUnitSize;
	std::vector<CVec2> vCells;
	std::vector< CPtr<CCommonUnit> > vUnits;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&nCount); f.Add(3,&vUnitSize); f.Add(4,&vCells); f.Add(5,&vUnits); return 0; }
public:
	SPriorityGroupPosition() : nCount( 0 ), vUnitSize( VNULL2 ) {}
	SPriorityGroupPosition( const CVec2 &_vUnitSize ) : nCount( 1 ), vUnitSize( _vUnitSize ) {}
	SPriorityGroupPosition( const float x, const float y ) : nCount( 1 ), vUnitSize( x, y ) {}
};

typedef std::unordered_map< int, SPriorityGroupPosition > CPriorityGroupMap;
typedef std::list< SUnitListInfo > CUnitsList;
typedef std::unordered_map< int, std::vector<CVec2> > CCellsMap;

class CGroupMoveExecutorEvent : public CExecutorEvent
{
	ZDATA_(CExecutorEvent)
	int nUnitID;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutorEvent*)this); f.Add(2,&nUnitID); return 0; }

public:
	CGroupMoveExecutorEvent() : nUnitID( -1 ) {}
	CGroupMoveExecutorEvent( const SExecutorEventParam &_param, const int _nUnitID ) : nUnitID( _nUnitID ), CExecutorEvent( _param ) {}

	const int GetUnitID() const { return nUnitID; }
};

class CGroupMoveExecutor : public CExecutor
{
	OBJECT_BASIC_METHODS( CGroupMoveExecutor );

	enum EExecutorState
	{
		ES_BUILD_FORMATION,
		ES_MOVE_TO_DESTINATION,
		ES_BUILD_FIGHT_FORMATION,
		ES_DEFAULT_MOVE,
	};

	ZDATA_( CExecutor )
	CVec2 vFinishPoint;
	CVec2 vCurrentFinishPoint;
	CUnitsList units;
	CVec2 vGroupCenter;
	int nUnitsCount;

	EExecutorState eExecutorState;

	int nAtPlaceCount;

	CVec2 vFinishPathDirection;
	CVec2 vStartPathDirection;

	float fMaxGroupSpeed;

	CCellsMap cells;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,( CExecutor *)this); f.Add(2,&vFinishPoint); f.Add(3,&vCurrentFinishPoint); f.Add(4,&units); f.Add(5,&vGroupCenter); f.Add(6,&nUnitsCount); f.Add(7,&eExecutorState); f.Add(8,&nAtPlaceCount); f.Add(9,&vFinishPathDirection); f.Add(10,&vStartPathDirection); f.Add(11,&fMaxGroupSpeed); f.Add(12,&cells); return 0; }

	void SolvePositions( const CPriorityGroupMap &positions );

	void CalcPositions( const CVec2 &vDirection );
	void RecalcDestination();
	const bool CheckPositions() const;

	bool SendUnit( CCommonUnit *pUnit );
	bool SendUnitToPosition( CCommonUnit *pUnit, const CVec2 &vDestination );

	void PeriodicalCheckForBuildFormation();

	void MoveToDestination();
	void PeriodicalCheckForMove();

	bool TryMoveUnit( const int nUnitID );
	void FinishState( const int nUnitID );
	void DeleteUnitFromList( const int nUnitID );

	const float GetUnitPosition( const CCommonUnit *pUnit, const CVec2 &vStartPoint, const CVec2 &vDirection ) const { return ( pUnit->GetCenterPlain() - vStartPoint ) * vDirection; }

	void DeleteDeadUnits();
public:
	CGroupMoveExecutor() : CExecutor( TID_MOVE_BY_FORMATION, 1 )	{}

	const bool Init( const std::vector<CCommonUnit*> &units, const struct SAIUnitCmd &cmd );

	virtual bool IsExecutorValid() const { return true; }
	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event );
	void RegisterOnEvents( IExecutorContainer *pContainer );
};


