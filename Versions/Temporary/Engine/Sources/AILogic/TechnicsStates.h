#pragma once

#include "UnitStates.h"
#include "Common_RTS_AI/Terrain.h"
#include "StatusUpdatesHelper.h"
namespace NDb
{
	struct SMechUnitRPGStats;
}

// выход из танк-пита. 
class CTankPitLeaveState : public IUnitState
{
	OBJECT_BASIC_METHODS( CTankPitLeaveState );

	enum ETankLeaveTankPitState 
	{
		TLTPS_ESTIMATING,
		TLTPS_MOVING,
	};
	ZDATA
	ETankLeaveTankPitState eState;
	CPtr<CAIUnit> pUnit;
	NTimer::STime timeStartLeave;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pUnit); f.Add(4,&timeStartLeave); return 0; }
public:
	static IUnitState* Instance( class CAIUnit *pTank );

	CTankPitLeaveState() { }
	CTankPitLeaveState( class CAIUnit  *pTank );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }
};

// зарыться в землю ( самостоятельно окопаться )
class CMechUnitEntrenchSelfState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CMechUnitEntrenchSelfState );

	enum ESoldierHullDownState 
	{
		ESHD_ESTIMATE,
		ESHD_START_BUILD,
		ESHD_BUILD_PIT,
		ESHD_END,
		ESHD_WAIT_FOR_INSTALL,
	};

	ZDATA_( CStatusUpdatesHelper )
	CPtr<CAIUnit> pUnit;
	list<SObjTileInfo> tiles;											// locked tiles under tank pit that being built
	CVec2 vHalfSize;											// half size of tank pit
	CDBPtr<SMechUnitRPGStats> pStats;		// stats of tank pit
	int nDBIndex;													// db index of tank pit

	ESoldierHullDownState eState;
	NTimer::STime timeEndBuild;
	CVec2 vTankPitCenter;
	NTimer::STime timeStartBuild;
	float fOldProgress;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&pUnit); f.Add(3,&tiles); f.Add(4,&vHalfSize); f.Add(5,&pStats); f.Add(6,&nDBIndex); f.Add(7,&eState); f.Add(8,&timeEndBuild); f.Add(9,&vTankPitCenter); f.Add(10,&timeStartBuild); f.Add(11,&fOldProgress); return 0; }
private:

	bool CheckTrenches( const CAIUnit * pUnit, const SRect &rectToTest ) const;
	bool CheckInfantry( const CAIUnit * pUnit, const SRect &rect ) const;
	void GetTilesNextToRect( const SRect &rect, const WORD wDirExclude );
public:
	static IUnitState* Instance( class CAIUnit * pUnit );
	
	CMechUnitEntrenchSelfState() : pUnit( 0 ) {  }
	CMechUnitEntrenchSelfState( class CAIUnit * pUnit );
	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }
	EUnitStateNames GetName() { return EUSN_ENTRENCH_SELF; }
};

class CSoldierEnterHoldSectorState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierEnterHoldSectorState );
	ZDATA
	CPtr<CAIUnit> pUnit;
	NTimer::STime nTimeStart;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&nTimeStart); return 0; }
public:
	static IUnitState* Instance( class CAIUnit * pUnit );

	CSoldierEnterHoldSectorState() : pUnit( 0 ) {  }
	CSoldierEnterHoldSectorState( class CAIUnit *_pUnit );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }
};

class CMilitaryCar;
class CMechUnitRestOnBoardPath;
class CMechUnitInsideMechUnitState : public IUnitState
{
	OBJECT_BASIC_METHODS( CMechUnitInsideMechUnitState );

	enum EMechUnitEnterMechUnitState
	{
		EMEM_APPROACHING,
		EMEM_REST_INSIDE,
		EMEM_ATTACKING,
		EMEM_EXITTING,
		CHECK_PERIOID = 500,
	};
	class CAIUnit * pUnit;

	ZDATA
		ZSKIP
		ZONSERIALIZE
	EMechUnitEnterMechUnitState eState;
	CPtr<CMilitaryCar> pTransport;
	CPtr<CAIUnit> pEnemy;
	CVec2 vEntrancePoint;
	CPtr<CMechUnitRestOnBoardPath> pPath;
	NTimer::STime timeNextCheck;
	CVec2 vDestination;						// Final destination to send the unit to, after leaving transport
public: 
	ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); f.Add(3,&eState); f.Add(4,&pTransport); f.Add(5,&pEnemy); f.Add(6,&vEntrancePoint); f.Add(7,&pPath); f.Add(8,&timeNextCheck); f.Add(9,&vDestination); return 0; }
	
	void OnSerialize( IBinSaver &saver );
	void SendToEntrance();
	void BoardNow();
	bool WillUnitIntersectOtherUnits( CAIUnit * pUnit, const CVec2 &vCenter, const CVec2 &vDir );
	bool FindAllowedDropPoint( CAIUnit *pUnit, CVec2 *vDropPoint );

	void SendToDestination( const CVec2 &vDropPoint, const CVec2 &vDir );

public:
	CMechUnitInsideMechUnitState() : pUnit( 0 ) {  }
	CMechUnitInsideMechUnitState( class CAIUnit *_pUnit, class CMilitaryCar *pTransport, const bool bImmidiate );

	void AttackTarget( CAIUnit * pEnemy );
	void Unload( const CVec2 &vPoint );
	void Segment();
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	bool IsAttackingState() const { return false; }
	const CVec2 GetPurposePoint() const { return vDestination; }
	EUnitStateNames GetName() { return EUSN_MECHUNIT_REST_ON_BOARD; }
};


