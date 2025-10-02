
#pragma once

#include "UnitStates.h"
#include "Behaviour.h"
#include "FreeFireManager.h"
#include "RndRunUpToEnemy.h"
#include "DamageToEnemyUpdater.h"

#include "StatusUpdatesHelper.h"

class CAIUnit;
class CSoldier;
class CStaticObject;

class CMechAttackUnitState : public IUnitAttackingState, public CFreeFireManager, public CStandartBehaviour
{
	OBJECT_BASIC_METHODS( CMechAttackUnitState );
	
	enum { SHOOTING_CHECK = 3000, ENEMY_DIR_TOLERANCE = 10000 };
	enum ESoldierAttackStates { ESAS_ESTIMATING, ESAS_BRUTE_MOVING, ESAS_MOVING, ESAS_MOVING_TO_SIDE, ESAS_SIMPLE_FIRING };
	enum EAttackType { EAT_GOOD_PROB, EAT_POOR_PROB };

	ZDATA_(CFreeFireManager)
	ZPARENT( CStandartBehaviour )
	ESoldierAttackStates state;	

	NTimer::STime lastShootCheck;
	SVector lastEnemyTile;
	SAIAngle wLastEnemyDir;

	vector<float> fProb;
	EAttackType eAttackType;
	BYTE nBestSide;
	
	bool bTurningToBest;
	int nBestAngle;
	CVec2 lastEnemyCenter;
	bool bSwarmAttack;
	CPtr<CAIUnit> pUnit;
	int nEnemyParty;

	CDamageToEnemyUpdater damageToEnemyUpdater;

	CVec2 vEnemyCenter;
	SAIAngle wEnemyDir;
protected:
	CPtr<CAIUnit> pEnemy;

	CPtr<CBasicGun> pGun;
	bool bAim;
	bool bFinish;
	bool bAllowMove;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CFreeFireManager*)this); f.Add(2,( CStandartBehaviour *)this); f.Add(3,&state); f.Add(4,&lastShootCheck); f.Add(5,&lastEnemyTile); f.Add(6,&wLastEnemyDir); f.Add(7,&fProb); f.Add(8,&eAttackType); f.Add(9,&nBestSide); f.Add(10,&bTurningToBest); f.Add(11,&nBestAngle); f.Add(12,&lastEnemyCenter); f.Add(13,&bSwarmAttack); f.Add(14,&pUnit); f.Add(15,&nEnemyParty); f.Add(16,&damageToEnemyUpdater); f.Add(17,&vEnemyCenter); f.Add(18,&wEnemyDir); f.Add(19,&pEnemy); f.Add(20,&pGun); f.Add(21,&bAim); f.Add(22,&bFinish); f.Add( 23, &bAllowMove ); return 0; }
	void OnSerialize( IBinSaver &f );
	//
	bool IsBruteMoving();
	struct IStaticPath* BestSidePath();
	void CalculateProbabilitites();

	void AnalyzeBruteMovingPosition();
	void AnalyzeMovingPosition();
	void AnalyzeMovingToSidePosition();
	
	void TraceAim();
	// пытается развернуться лбом к противнику за время перезарядки, если вернула false, то сейчас не время для поворота
	bool TurnToBestPos();
	// выстрелить и произвести все необходимы updates в state
	void FireToEnemy();
	// можно ли прямо сейчас стрельнуть в enemy без вращений
	bool CanShootToEnemyNow() const;
	// проинициализировать, всё что нужно, если для стрельбы выбран pGun
	void StartStateWithGun( CBasicGun *pGun );
	void FinishState();
protected:


	//
	virtual void FireNow();
	virtual void StopFire();
	virtual void StartAgain();
public:
	static IUnitState* Instance( class CAIUnit *pOwner, class CAIUnit *pEnemy, bool bAim, const bool bSwamAttack );

	CMechAttackUnitState() { }
	CMechAttackUnitState( class CAIUnit *pOwner, class CAIUnit *pEnemy, bool bAim, const bool bSwarmAttack );

	virtual void Segment();

	class CAIUnit* GetTarget() const { return pEnemy; }
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual bool IsAttacksUnit() const { return true; }
	class CAIUnit* GetTargetUnit() const { return GetTarget(); }

	virtual EUnitStateNames GetName() { return EUSN_ATTACK_UNIT; }
};

class CCommonAttackUnitInBuildingState : public IUnitAttackingState, public CFreeFireManager
{
	enum EAttackUnitInBuildingStates { EAUBS_START, EAUBS_MOVING_SECTOR, EAUBS_MOVING_UNIT };
	ZDATA_(CFreeFireManager)
	EAttackUnitInBuildingStates eState;

	CDamageToEnemyUpdater damageToEnemyUpdater;
	CRndRunUpToEnemy runUpToEnemy;
	
	int nSlot;

	//
protected:
	CPtr<CSoldier> pTarget;
	CVec2 targetCenter;
	CPtr<CBasicGun> pGun;
	bool bAim;
	bool bSwarmAttack;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CFreeFireManager*)this); f.Add(2,&eState); f.Add(3,&damageToEnemyUpdater); f.Add(4,&runUpToEnemy); f.Add(5,&nSlot); f.Add(6,&pTarget); f.Add(7,&targetCenter); f.Add(8,&pGun); f.Add(9,&bAim); f.Add(10,&bSwarmAttack); return 0; }
protected:
	//
	virtual class CAIUnit* GetUnit() const = 0;
	virtual void FireNow() = 0;
	virtual bool IsInTargetSector() const;
	
	void FinishState();
private:
	bool FindPathToUnit();
	bool FindPathToSector();
	void StartState( class CAIUnit *pOwner );

public:
	CCommonAttackUnitInBuildingState() { }
	CCommonAttackUnitInBuildingState( class CAIUnit *pOwner, class CSoldier *pTarget, bool bAim, const bool bSwarmAttack );

	virtual void Segment();

	CSoldier* GetTarget() const { return pTarget; }
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	virtual bool IsAttacksUnit() const { return true; }
	CAIUnit* GetTargetUnit() const;
};

class CCommonAttackCommonStatObjState : public IUnitAttackingState, public CFreeFireManager
{
	//
	bool AttackUnits( class CStaticObject *pObj );
	void AnalyzePosition();
	void AnalyzeShootingObj();

	ZDATA_(CFreeFireManager)
	int nStartObjParty;
protected:
	CPtr<CStaticObject> pObj;
	CPtr<CBasicGun> pGun;

	bool bAim;
	bool bFinish;
	bool bSwarmAttack;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CFreeFireManager*)this); f.Add(2,&nStartObjParty); f.Add(3,&pObj); f.Add(4,&pGun); f.Add(5,&bAim); f.Add(6,&bFinish); f.Add(7,&bSwarmAttack); return 0; }
protected:

	//
	virtual class CAIUnit* GetUnit() const = 0;
	virtual void FireNow() = 0;
	virtual void StartAgain();
	void FinishState();
public:
	CCommonAttackCommonStatObjState() { }
	CCommonAttackCommonStatObjState( class CAIUnit *pOwner, class CStaticObject *pObj, bool bSwarmAttack );

	virtual void Segment();

	virtual EUnitStateNames GetName() { return EUSN_ATTACK_STAT_OBJECT; }
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	class CStaticObject* GetTarget() const { return pObj; }

	virtual bool IsAttacksUnit() const { return true; }
	virtual class CAIUnit* GetTargetUnit() const { return 0; }
};

class CCommonRestState : public IUnitState, public CStandartBehaviour
{
	enum { TIME_OF_CHECK = 2000 };

	ZDATA_(CStandartBehaviour)
	CPtr<CCommonUnit> pUnit;
	CVec2 guardPoint;
	SAIAngle wDir;
	NTimer::STime nextMove;
	NTimer::STime startMoveTime;

	bool bScanned;
	
	bool bWait;
	NTimer::STime timeToFinishState;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CStandartBehaviour*)this); f.Add(2,&pUnit); f.Add(3,&guardPoint); f.Add(4,&wDir); f.Add(5,&nextMove); f.Add(6,&startMoveTime); f.Add(7,&bScanned); f.Add(8,&bWait); f.Add(9,&timeToFinishState); return 0; }
public:
	CCommonRestState() : pUnit( 0 ) { }
	CCommonRestState( const CVec2 &guardPoint, const WORD wDir, CCommonUnit *pUnit, const float fTimeToWait );
	
	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_REST; }
	virtual bool IsRestState() const;
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return guardPoint; }

	const CVec2& GetGuardPoint() const { return guardPoint; }
	const WORD GetGuardDir() const { return wDir; }

	// в 1 - некоторое малое время, чтобы произошло обновление, не 0 - т.к. это говорит о первом запуске сегмента
	void SetNullLastMoveTime() { nextMove = 1; }
};

class CMechUnitRestState : public CCommonRestState
{
	OBJECT_BASIC_METHODS( CMechUnitRestState );

	ZDATA_(CCommonRestState)
	CPtr<CAIUnit> pUnit;

	bool bFinishWhenCanMove;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CCommonRestState*)this); f.Add(2,&pUnit); f.Add(3,&bFinishWhenCanMove); return 0; }
	void OnSerialize( IBinSaver &saver );
public:
	static IUnitState* Instance( class CAIUnit *pUnit, const CVec2 &guardPoint, const WORD wDir, const bool bFinishWnenCanMove, const float fTimeToWait );

	CMechUnitRestState() : pUnit( 0 ) { }
	CMechUnitRestState( CAIUnit *pUnit, const CVec2 &guardPoint, const WORD wDir, const bool bFinishWnenCanMove, const float fTimeToWait );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_REST; }
	virtual bool IsAttackingState() const { return false; }
};

class CCommonAmbushState : public IUnitState, public CStandartBehaviour
{
	OBJECT_BASIC_METHODS( CCommonAmbushState );
	
	enum { AMBUSH_CHECK = 3000, VISIBLE_CHECK = 500 };
	enum EAmbushStates { EAS_COMMON, EAS_FIRING };
	ZDATA_(CStandartBehaviour)
protected:
	CPtr<CCommonUnit> pUnit;
private:
	EAmbushStates eState;
	
	NTimer::STime startTime;
	NTimer::STime lastCheckTime;
	NTimer::STime lastVisibleCheck;

	CPtr<CBasicGun> pGun;
	CPtr<CAIUnit> pTarget;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CStandartBehaviour*)this); f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&startTime); f.Add(5,&lastCheckTime); f.Add(6,&lastVisibleCheck); f.Add(7,&pGun); f.Add(8,&pTarget); return 0; }

	//
	void CommonState();
	void FiringState();

public:
	static IUnitState* Instance( class CCommonUnit *pUnit );
	
	CCommonAmbushState() { }
	CCommonAmbushState( class CCommonUnit *pUnit );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_AMBUSH; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

	CAIUnit* GetTarget() const { return pTarget; }
};

class CFollowState : public IUnitState, public CStandartBehaviour
{
	OBJECT_BASIC_METHODS( CFollowState );

	enum { CHECK_HEAD = 2000 };
	ZDATA_(CStandartBehaviour)
	CPtr<CCommonUnit> pUnit;
	CPtr<CCommonUnit> pHeadUnit;
	CVec2 lastHeadUnitPos;
	NTimer::STime lastCheck;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CStandartBehaviour*)this); f.Add(2,&pUnit); f.Add(3,&pHeadUnit); f.Add(4,&lastHeadUnitPos); f.Add(5,&lastCheck); return 0; }
public:
	static IUnitState* Instance( class CCommonUnit *pUnit, class CCommonUnit *pHeadUnit );

	CFollowState() { }
	CFollowState( class CCommonUnit *pUnit, class CCommonUnit *pHeadUnit );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CCommonSwarmState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CCommonSwarmState );
	
	enum { TIME_OF_WAITING = 200 };
	enum ECommonSwarmStates 
	{ 
		ESSS_WAIT, 
		ESSS_MOVING,
		ESS_WAIT_UNTIL_TRACK_REPAIR,
	};
	ZDATA_( CStatusUpdatesHelper )
	ECommonSwarmStates state;

	CPtr<CAIUnit> pUnit;

	CVec2 point;
	NTimer::STime startTime;
	bool bContinue;
	SAIAngle wDirToPoint;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&state); f.Add(3,&pUnit); f.Add(4,&point); f.Add(5,&startTime); f.Add(6,&bContinue); f.Add(7,&wDirToPoint); return 0; }
public:
	static IUnitState* Instance( class CAIUnit *pUnit, const CVec2 &point, const float fContinue );

	CCommonSwarmState() : pUnit( 0 ) { }
	CCommonSwarmState( class CAIUnit *pUnit, const CVec2 &point, const float fContinue );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_SWARM; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return point; }
};

class CCommonMoveToGridState : public IUnitState
{
	OBJECT_BASIC_METHODS( CCommonMoveToGridState );

	enum EStates
	{
		ES_MOVE,
		ES_WAIT
	};

	ZDATA
	CPtr<CCommonUnit> pUnit;
	CVec2 vPoint;
	CVec2 vDir;
	NTimer::STime startMoveTime;

	EStates eState;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&vPoint); f.Add(4,&vDir); f.Add(5,&startMoveTime); f.Add(6,&eState); return 0; }
public:
	static IUnitState* Instance( class CCommonUnit *pUnit, const CVec2 &vPoint, const CVec2 &vDir );

	CCommonMoveToGridState() : pUnit( 0 ), eState( ES_WAIT ) { }
	CCommonMoveToGridState( class CCommonUnit *pUnit, const CVec2 &vPoint, const CVec2 &vDir );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_MOVE_TO_GRID; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return vPoint; }
};

class CMoveByFormationState : public IUnitState
{
	OBJECT_BASIC_METHODS( CMoveByFormationState );

	ZDATA
	CPtr<CCommonUnit> pUnit;
	int nExecutorID;
	bool bNeedIdleNotification;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&nExecutorID); f.Add(4,&bNeedIdleNotification); return 0; }
public:
	static IUnitState *Instance( CCommonUnit *pUnit, const int nExecutorID );
	CMoveByFormationState() : pUnit( 0 ), nExecutorID( -1 ), bNeedIdleNotification( false ) {}
	CMoveByFormationState( CCommonUnit *pUnit, const int nExecutorID );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_MOVE_BY_FORMATION; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return VNULL2; }

	const bool SendUnit( const CVec2 &vGroupFinishPoint );
	const bool SendUnitToPosition( const CVec2 &vDestination );
	void FinishState();

};

struct SPatrolWaypoints
{
	typedef vector<CVec2> CPatrolPoints;
	typedef hash_map<int /*Unit ID*/, CPatrolPoints> CPatrolPointsMap;

	CPatrolPointsMap points;

	vector<CVec2> &operator[]( const int nUnitID ) { return points[ nUnitID ]; }
	const bool HasPoints( const int nUnitID ) { return ( points.find( nUnitID ) != points.end() ); }

	void Clear() { points.clear(); }

	int operator&( IBinSaver &saver ) { saver.Add( 1, &points ); return 0; }
};

// CCommonPatrolState - never gets created
class CCommonPatrolState : public IUnitState
{
	OBJECT_BASIC_METHODS( CCommonPatrolState );

public:
	static IUnitState *Instance( CCommonUnit *pUnit, CVec2 &vTarget );
	CCommonPatrolState() {}

	virtual void Segment() { }

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand ) { return TSIR_YES_IMMIDIATELY; }

	virtual EUnitStateNames GetName() { return EUSN_PATROL; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return VNULL2; }

	int operator&( IBinSaver &saver ) { return 0; }
};


