#pragma once

#include "StatesFactory.h"
#include "CommonStates.h"
#include "..\Stats_B2_M1\ActionNotify.h"
#include "StatusUpdatesHelper.h"

class CBuilding;
class CEntrenchment;
class CMineStaticObject;
class CMilitaryCar;
class CFormation;
enum EActionNotify;
namespace NDb
{
	struct SInfantryRPGStats;
	enum EMineType;
}

class CSoldierStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CSoldierStatesFactory );
	
	static CPtr<CSoldierStatesFactory> pFactory;
public:
	int operator&( IBinSaver &saver )
	{
		return 0;
	}

	static IStatesFactory* Instance();
	virtual interface IUnitState* ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand );
	virtual interface IUnitState* ProduceRestState( class CQueueUnit *pUnit );

	virtual bool CanCommandBeExecuted( class CAICommand *pCommand );
	// for Saving/Loading of static members
	friend class CStaticMembers;
};

class CSoldierRestState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierRestState );	
	
	ZDATA
	CPtr<CAIUnit> pUnit;

	NTimer::STime nextMove;
	bool bScanned;
	CVec2 guardPoint;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&nextMove); f.Add(4,&bScanned); f.Add(5,&guardPoint); return 0; }
	//float fDistToGuardPoint; // не используется
	const bool CheckGuardPoint() const;
public:
	static IUnitState* Instance( class CAIUnit *pUnit );
	
	CSoldierRestState() : pUnit( 0 ) { }
	CSoldierRestState( class CAIUnit *pUnit );
	
	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_REST; }
	virtual bool IsRestState() const { return true; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return guardPoint; }

	class CAIUnit* GetTarget() const { return 0; }

	// в 1 - некоторое малое время, чтобы произошло обновление, не 0 - т.к. это говорит о первом запуске сегмента
	void SetNullLastMoveTime() { nextMove = 1; }
};

class CSoldierAttackState : public IUnitAttackingState, public CStandartBehaviour
{
	OBJECT_BASIC_METHODS( CSoldierAttackState );
	
	enum { SHOOTING_CHECK = 3000, ENEMY_DIR_TOLERANCE = 10000 };
	enum ESoldierAttackStates { ESAS_BRUTE_MOVING, ESAS_MOVING, ESAS_MOVING_TO_SIDE };


	ZDATA_(CStandartBehaviour)
	CPtr<CAIUnit> pUnit;
	ESoldierAttackStates state;	

	NTimer::STime nextShootCheck;
	SVector lastEnemyTile;
	SAIAngle wLastEnemyDir;

	CVec2 lastEnemyCenter;

	CPtr<CAIUnit> pEnemy;

	CPtr<CBasicGun> pGun;
	bool bAim;
	bool bFinish;
	bool bSwarmAttack;
	bool bPreferGrenade;
	int nEnemyParty;

	CDamageToEnemyUpdater damageToEnemyUpdater;
	CRndRunUpToEnemy runUpToEnemy;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CStandartBehaviour*)this); f.Add(2,&pUnit); f.Add(3,&state); f.Add(4,&nextShootCheck); f.Add(5,&lastEnemyTile); f.Add(6,&wLastEnemyDir); f.Add(7,&lastEnemyCenter); f.Add(8,&pEnemy); f.Add(9,&pGun); f.Add(10,&bAim); f.Add(11,&bFinish); f.Add(12,&bSwarmAttack); f.Add(13,&bPreferGrenade); f.Add(14,&nEnemyParty); f.Add(15,&damageToEnemyUpdater); f.Add(16,&runUpToEnemy); return 0; }
	//
	bool IsBruteMoving();
	interface IStaticPath* BestSidePath();

	// атака в лоб - ехать на юнит и, когда можно стрелять, стрелять
	void AnalyzeBruteMovingPosition();
	// идти на юнит до какого-то до него расстояния, а потом зайти со стороны
	void AnalyzeMovingPosition();
	// зайти на юнит со стороны
	void AnalyzeMovingToSidePosition();

	void FireNow();
	void StopFire();
	void StartAgain();

	void FinishState();
public:
	static IUnitState* Instance( class CAIUnit *pUnit, class CAIUnit *pEnemy, bool bAim, const bool bSwarmAttack, const bool bPreferGrenade );

	CSoldierAttackState() : pUnit( 0 ) { }
	CSoldierAttackState( class CAIUnit *pUnit, class CAIUnit *pEnemy, bool bAim, const bool bSwarmAttack, const bool bPreferGrenade );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_ATTACK_UNIT; }
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	class CAIUnit* GetTarget() const { return pEnemy; }

	virtual bool IsAttacksUnit() const { return true; }
	virtual class CAIUnit* GetTargetUnit() const { return GetTarget(); }
};

class CSoldierMoveToState : public IUnitState, public CFreeFireManager
{
	OBJECT_BASIC_METHODS( CSoldierMoveToState );
	
	enum { TIME_OF_WAITING = 200 };

	ZDATA_(CFreeFireManager)
	CPtr<CAIUnit> pUnit;

	NTimer::STime startTime;
	bool bWaiting;
	CVec2 point;
	SAIAngle wDirToPoint;
	bool bLongMove;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CFreeFireManager*)this); f.Add(2,&pUnit); f.Add(3,&startTime); f.Add(4,&bWaiting); f.Add(5,&point); f.Add(6,&wDirToPoint); f.Add(7,&bLongMove); return 0; }
public:
	static IUnitState* Instance( CAIUnit *pUnit, const CVec2 &point );

	CSoldierMoveToState() : pUnit( 0 ) { }
	CSoldierMoveToState( class CAIUnit *pUnit, const CVec2 &point );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
	
	virtual EUnitStateNames GetName() { return EUSN_MOVE; }
};

class CSoldierTurnToPointState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CSoldierTurnToPointState );
	
	ZDATA_( CStatusUpdatesHelper )
	CPtr<CAIUnit> pUnit;

	NTimer::STime lastCheck;
	CVec2 targCenter;
	NTimer::STime timeStart;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&pUnit); f.Add(3,&lastCheck); f.Add(4,&targCenter); f.Add(5,&timeStart); return 0; }

public:
	static IUnitState* Instance( class CAIUnit *pUnit, const CVec2 &targCenter );

	CSoldierTurnToPointState() : pUnit( 0 ) { }
	CSoldierTurnToPointState( class CAIUnit *pUnit, const CVec2 &targCenter );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
	
	virtual EUnitStateNames GetName() { return EUSN_TURN_TO_POINT; }
};

class CSoldierMoveByDirState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierMoveByDirState );

	ZDATA 
	CPtr<CAIUnit> pUnit;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); return 0; }
	//
	void Init( class CAIUnit *pUnit, const CVec2 &vTarget );
public:
	static IUnitState* Instance( class CAIUnit *pUnit, const CVec2 &vTarget );

	CSoldierMoveByDirState() : pUnit( 0 ) { }
	CSoldierMoveByDirState( class CAIUnit *pUnit, const CVec2 &vTarget );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

class CSoldierEnterState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierEnterState );

	enum EEnterStates { EES_START, EES_RUN_UP };
	ZDATA
	EEnterStates state;

	CPtr<CAIUnit> pUnit;

	int nEntrance;
	CPtr<CBuilding> pBuilding;
	int nEfforts;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&state); f.Add(3,&pUnit); f.Add(4,&nEntrance); f.Add(5,&pBuilding); f.Add(6,&nEfforts); return 0; }

	//
	bool SetPathForRunUp();
public:
	static IUnitState* Instance( class CAIUnit *pUnit, class CBuilding *pBuilding );

	CSoldierEnterState() : pUnit( 0 ) { }
	CSoldierEnterState( class CAIUnit *pUnit, class CBuilding *pBuilding );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_ENTER; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CSoldierEnterEntrenchmentState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierEnterEntrenchmentState );
	
	enum EEnterState { EES_START, EES_RUN, EES_FINISHED };
	ZDATA
	EEnterState state;

	CPtr<CAIUnit> pUnit;
	CPtr<CEntrenchment> pEntrenchment;
	int nTries;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&state); f.Add(3,&pUnit); f.Add(4,&pEntrenchment); f.Add(5,&nTries); return 0; }
	//
	bool SetPathForRunIn();
	void EnterToEntrenchment();
public:
	static IUnitState* Instance( class CAIUnit *pUnit, class CEntrenchment *pEntrenchment );

	CSoldierEnterEntrenchmentState() : pUnit( 0 ) { }
	CSoldierEnterEntrenchmentState( class CAIUnit *pUnit, class CEntrenchment *pEntrenchment );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_ENTER_ENTRENCHMENT; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CSoldierAttackCommonStatObjState : public CCommonAttackCommonStatObjState
{
	OBJECT_BASIC_METHODS( CSoldierAttackCommonStatObjState );
	ZDATA_(CCommonAttackCommonStatObjState)
	CPtr<CAIUnit> pUnit;
	bool bFinishAfterInsidersDead;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CCommonAttackCommonStatObjState*)this); f.Add(2,&pUnit); f.Add(3,&bFinishAfterInsidersDead); return 0; }

protected:
	virtual class CAIUnit* GetUnit() const;
	virtual void FireNow();
public:
	static IUnitState* Instance( class CAIUnit *pUnit, class CStaticObject *pObj, bool bSwarmAttack );

	CSoldierAttackCommonStatObjState() : pUnit( 0 ) { }
	CSoldierAttackCommonStatObjState( class CAIUnit *pUnit, class CStaticObject *pObj, bool bSwarmAttack );

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;
	virtual void Segment();
	virtual EUnitStateNames GetName() { return EUSN_ATTACK_OBJECT; }
};

class CSoldierParadeState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierParadeState );
	ZDATA
	CPtr<CAIUnit> pUnit;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); return 0; }
public:
	static IUnitState* Instance( class CAIUnit *pUnit );

	CSoldierParadeState() : pUnit( 0 ) { }
	CSoldierParadeState( CAIUnit *pUnit );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_PARADE; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

	friend class CStaticMembers;
};

class CSoldierPlaceMineNowState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierPlaceMineNowState );
	ZDATA
	CPtr<CAIUnit> pUnit;

	CVec2 point;
	int nType;
	NTimer::STime beginAnimTime; 
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&point); f.Add(4,&nType); f.Add(5,&beginAnimTime); return 0; }
public:
	static IUnitState* Instance( class CAIUnit *pUnit, const CVec2 &point, const EMineType nType );

	CSoldierPlaceMineNowState() : pUnit( 0 ) { }
	CSoldierPlaceMineNowState( class CAIUnit *pUnit, const CVec2 &point, const EMineType nType );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CSoldierClearMineRadiusState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierClearMineRadiusState );
	
	enum EPutMineStates { EPM_START, EPM_MOVE,	EPM_WAITING };
	ZDATA
	EPutMineStates eState;

	CPtr<CAIUnit> pUnit;
	CPtr<CMineStaticObject> pMine;

	CVec2 clearCenter;
	NTimer::STime beginAnimTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pUnit); f.Add(4,&pMine); f.Add(5,&clearCenter); f.Add(6,&beginAnimTime); return 0; }
	//
	bool FindMineToClear();
public:
	static IUnitState* Instance( class CAIUnit *pUnit, const CVec2 &clearCenter );

	CSoldierClearMineRadiusState() : pUnit( 0 ) { }
	CSoldierClearMineRadiusState( class CAIUnit *pUnit, const CVec2 &clearCenter );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return clearCenter; }
};

class CSoldierAttackUnitInBuildingState : public CCommonAttackUnitInBuildingState
{
	OBJECT_BASIC_METHODS( CSoldierAttackUnitInBuildingState );
	ZDATA_(CCommonAttackUnitInBuildingState)
	CPtr<CAIUnit> pUnit;
	bool bTriedToShootBuilding;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CCommonAttackUnitInBuildingState*)this); f.Add(2,&pUnit); f.Add(3,&bTriedToShootBuilding); return 0; }
protected:	
	virtual class CAIUnit* GetUnit() const;
	virtual void FireNow();
public:
	static IUnitState* Instance( class CAIUnit *pUnit, class CSoldier *pTarget, bool bAim, const bool bSwarmAttack );

	CSoldierAttackUnitInBuildingState() : pUnit( 0 ) { }
	CSoldierAttackUnitInBuildingState( class CAIUnit *pUnit, class CSoldier *pTarget, bool bAim, const bool bSwarmAttack );
	
	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_ATTACK_UNIT_IN_BUILDING; }
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;
};

class CSoldierEnterTransportNowState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierEnterTransportNowState );

	enum EEnterTransportStates { EETS_START, EETS_MOVING, EETS_FINISHED };
	ZDATA
	EEnterTransportStates eState;

	CPtr<CAIUnit> pUnit;
	CPtr<CMilitaryCar> pTransport;
	// последний апдейт траектории (нужно для движущегося транспорта)
	NTimer::STime timeLastTrajectoryUpdate;
	CVec2 vLastTransportCenter;
	SAIAngle wLastTransportDir;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pUnit); f.Add(4,&pTransport); f.Add(5,&timeLastTrajectoryUpdate); f.Add(6,&vLastTransportCenter); f.Add(7,&wLastTransportDir); return 0; }

public:
	static IUnitState* Instance( class CAIUnit *pUnit, class CMilitaryCar *pTransport );
	
	CSoldierEnterTransportNowState() : pUnit( 0 ) { }
	CSoldierEnterTransportNowState( class CAIUnit *pUnit, class CMilitaryCar *pTransport );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

//state of soldiers that are dropped from carrier
class CSoldierParaDroppingState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierParaDroppingState );
	
	enum ESoldierParaDroppingState
	{
		ESPDS_FALLING_W_PARASHUTE,
		ESPDS_CLOSING_PARASHUTE,
		ESPDS_WAIT_FOR_END_UPDATES,
		ESPDS_FINISH_STATE,
		ESPDS_WATING_FOR_ALL,
	};
	ZDATA
	ESoldierParaDroppingState eState;
	ESoldierParaDroppingState eStateToSwitch;
	CDBPtr<NDb::SInfantryRPGStats> pRememberedStats;
	
	
	NTimer::STime timeToCloseParashute, timeToFallWithParashute ;
	
	CPtr<CSoldier> pUnit;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&eStateToSwitch); f.Add(4,&pRememberedStats); f.Add(5,&timeToCloseParashute); f.Add(6,&timeToFallWithParashute); f.Add(7,&pUnit); return 0; }
public:
	static IUnitState* Instance( class CSoldier *pUnit, class CAviation *pPlane );
	
	CSoldierParaDroppingState() : pUnit( 0 ) { }
	CSoldierParaDroppingState( class CSoldier *pUnit, class CAviation *pPlane );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
		virtual EUnitStateNames GetName() { return EUSN_PARTROOP; }
	bool IsLanded() const { return eState == ESPDS_WATING_FOR_ALL; }
};

class CSoldierUseSpyglassState : public IUnitState, public CStandartBehaviour, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CSoldierUseSpyglassState );
	
	ZDATA_(CStandartBehaviour)
	CPtr<CSoldier> pSoldier;
	CVec2 vPoint2Look;
	ZPARENT( CStatusUpdatesHelper )
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CStandartBehaviour*)this); f.Add(2,&pSoldier); f.Add(3,&vPoint2Look); f.Add(4,( CStatusUpdatesHelper *)this); return 0; }
	//
	void SetLookAnimation();
public:
	static IUnitState* Instance( class CSoldier *pSoldier, const CVec2 &point );

	CSoldierUseSpyglassState() : pSoldier( 0 ), vPoint2Look( VNULL2 ) { }
	CSoldierUseSpyglassState( class CSoldier *pSoldier, const CVec2 &point );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

// юнит должен атаковать взвод. по 1 солдату.
class CSoldierAttackFormationState: public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CSoldierAttackFormationState );
	
	ZDATA
	CPtr<CAIUnit> pUnit;
	CPtr<CFormation> pTarget;
	bool bSwarmAttack;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&pTarget); f.Add(4,&bSwarmAttack); return 0; }
public:
	static IUnitState* Instance( class CAIUnit *pUnit, class CFormation *pTarget, const bool bSwarmAttack );
	
	CSoldierAttackFormationState() : pUnit( 0 ) { }
	CSoldierAttackFormationState( class CAIUnit *pUnit, class CFormation *pTarget, const bool bSwarmAttack );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	virtual bool IsAttacksUnit() const { return true; }
	virtual class CAIUnit* GetTargetUnit() const { return 0; }
};

class CSoldierIdleState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierIdleState );
	ZDATA
	CPtr<CAIUnit> pUnit;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); return 0; }
public:
	static IUnitState* Instance( class CAIUnit *pUnit );
	
	CSoldierIdleState() : pUnit( 0 ) { }
	CSoldierIdleState( class CAIUnit *pUnit );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

// для атаки самолетов. в этом стейте юнит следит не приближаются ли
// к нему наземные цели. если какая-то цель наземная появилась в радиусе видимости,
// то происходит переключение на нее.
#include "PredictedAntiAviationFire.h"

class CSoldierAttackAviationState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CSoldierAttackAviationState );
	ZDATA
	CPredictedAntiAviationFire predictedFire;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&predictedFire); return 0; }

public:

	CSoldierAttackAviationState () : predictedFire( 0, 0 ) { }
	// if nGun == -1, then state chooses gun. if gun != -1 then state don't set
	CSoldierAttackAviationState ( class CAIUnit *pUnit, class CAviation *pPlane );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }

	virtual bool IsAttacksUnit() const { return true; }
	virtual class CAIUnit* GetTargetUnit() const;
};

class CSoldierFireMoraleShellState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierFireMoraleShellState );

	ZDATA
	CPtr<CAIUnit> pUnit;
	int nMoraleGun;
	CVec2 vTarget;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&nMoraleGun); f.Add(4,&vTarget); return 0; }
public:
	static IUnitState* Instance( class CAIUnit *pUnit, const class CVec2 &vTarget );

	CSoldierFireMoraleShellState() : pUnit( 0 ) { }
	CSoldierFireMoraleShellState( class CAIUnit *pUnit, const class CVec2 &vTarget );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }
};

class CSoldierUseState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierUseState );

	ZDATA
	EActionNotify eState;
	CPtr<CAIUnit> pUnit;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pUnit); return 0; }

public:
	static IUnitState* Instance( class CAIUnit *pUnit, const EActionNotify &eState );

	CSoldierUseState() : eState( ACTION_NOTIFY_NONE ), pUnit( 0 ) { }
	CSoldierUseState( class CAIUnit *pUnit, const EActionNotify &eState );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }

	virtual EUnitStateNames GetName() { return EUSN_USE; }
};

class CSoldierEntrenchSelfState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierEntrenchSelfState )
		ZDATA
	CPtr<CSoldier> pSoldier;
	NTimer::STime timePlaceEntrench;
	NTimer::STime timeStartEntrench;
	float fOldProgress; 
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSoldier); f.Add(3,&timePlaceEntrench); f.Add(4,&timeStartEntrench); f.Add(5,&fOldProgress); return 0; }
public:
	CSoldierEntrenchSelfState() : pSoldier ( 0 ) { }
	CSoldierEntrenchSelfState ( class CSoldier *_pSoldier );
	void Segment();
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	bool IsAttackingState() const { return false; }
	const CVec2 GetPurposePoint() const;
	EUnitStateNames GetName() { return EUSN_ENTRENCH_SELF; }
};

class CSoldierLeaveSelfEntrenchState : public IUnitState
{
	OBJECT_BASIC_METHODS( CSoldierLeaveSelfEntrenchState )
		ZDATA
	CPtr<CAIUnit> pSoldier;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSoldier); return 0; }
public:
	CSoldierLeaveSelfEntrenchState() : pSoldier ( 0 ) { }
	CSoldierLeaveSelfEntrenchState( class CAIUnit *_pSoldier );
	void Segment();
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	bool IsAttackingState() const { return false; }
	const CVec2 GetPurposePoint() const;
	EUnitStateNames GetName() { return EUSN_LEAVE_SELF_ENTRENCH; }
};

