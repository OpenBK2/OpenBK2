
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "SoldierStates.h"
#include "StatusUpdatesHelper.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIUnit;
class CStaticObject;
class CAITransportUnit;
class CArtillery;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtilleryStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CArtilleryStatesFactory );

	static CPtr<CArtilleryStatesFactory> pFactory;
public:
	static IStatesFactory* Instance();
	int operator&( IBinSaver &saver )
	{
		return 0;
	}

	virtual struct IUnitState* ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand );
	virtual struct IUnitState* ProduceRestState( class CQueueUnit *pUnit );

	virtual bool CanCommandBeExecuted( class CAICommand *pCommand );
	// for Saving/Loading of static members
	friend class CStaticMembers;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtilleryMoveToState : public IUnitState
{
	OBJECT_BASIC_METHODS( CArtilleryMoveToState );

	enum { TIME_OF_WAITING = 200 };
	enum EArtilleryMoveToState { EAMTS_UNINSTALLING, EAMTS_START_MOVING, EAMTS_MOVING, EAMTS_WAIT_FOR_PATH };

	ZDATA
	EArtilleryMoveToState eState;

	bool bToFinish;
	CPtr<CArtillery> pArtillery;

	NTimer::STime startTime;
	CPtr<IStaticPath> pStaticPath;
	bool bForceReverse;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&bToFinish); f.Add(4,&pArtillery); f.Add(5,&startTime); f.Add(6,&pStaticPath); f.Add(7,&bForceReverse); return 0; }

	void SetReversePathMode( const bool bEnable );
public:
	static IUnitState* Instance( class CArtillery *pArtillery, const CVec2 &point, const bool bForceReverse = false );

	CArtilleryMoveToState() : pArtillery( 0 ), bForceReverse( false ) { }
	CArtilleryMoveToState( class CArtillery *pArtillery, const CVec2 &point, const bool bForceReverse );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState(class CAICommand *pCommand);
	
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }
	virtual EUnitStateNames GetName() { return EUSN_MOVE; }
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtilleryTurnToPointState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CArtilleryTurnToPointState );

	enum EArtilleryTurnToPointStates { EATRS_ESTIMATING, EATPS_UNINSTALLING, EATPS_TURNING };
	ZDATA_( CStatusUpdatesHelper )
	EArtilleryTurnToPointStates eState;

	CPtr<CArtillery> pArtillery;

	NTimer::STime lastCheck;
	CVec2 targCenter;
	NTimer::STime timeStart;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&eState); f.Add(3,&pArtillery); f.Add(4,&lastCheck); f.Add(5,&targCenter); f.Add(6,&timeStart); return 0; }
public:
	static IUnitState* Instance( class CArtillery *pArtillery, const CVec2 &targCenter );

	CArtilleryTurnToPointState() : pArtillery( 0 ) { }
	CArtilleryTurnToPointState( class CArtillery *pArtillery, const CVec2 &targCenter );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState(class CAICommand *pCommand);

	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
	
	virtual EUnitStateNames GetName() { return EUSN_TURN_TO_POINT; }
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtilleryBombardmentState : public IUnitAttackingState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CArtilleryBombardmentState );

	enum EArtilleryBombarmentStates { EABS_START, EABS_TURNING, EABS_FIRING };
	ZDATA_( CStatusUpdatesHelper )
	CPtr<CAIUnit> pUnit;
	EArtilleryBombarmentStates eState;
	CVec2 point;
	bool bStop;
	bool bSaidNoAmmo;
	int nShotCount;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&point); f.Add(5,&bStop); f.Add(6,&bSaidNoAmmo); f.Add(7,&nShotCount); return 0; }
public:
	static IUnitState* Instance( class CAIUnit *pUnit, const CVec2 &point, const int nShotCount );

	CArtilleryBombardmentState() : pUnit( 0 ) { }
	CArtilleryBombardmentState( class CAIUnit *pUnit, const CVec2 &point, const int nShotCount );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return point; }

	virtual bool IsAttacksUnit() const { return false; }
	virtual class CAIUnit* GetTargetUnit() const { return 0; }
	
	virtual EUnitStateNames GetName() { return EUSN_BOMBARDMANET; }
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtilleryRangeAreaState : public IUnitAttackingState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CArtilleryRangeAreaState );

	enum ERangeAreaStates { ERAS_TURNING, ERAS_RANGING, ERAS_WAITING, ERAS_SHOOT_UNIT, ERAS_SHOOT_OBJECT };
	enum { CHECK_TIME = 2000 };


	ZDATA_( CStatusUpdatesHelper )
	CPtr<CAIUnit> pUnit;
	ERangeAreaStates eState;
	CPtr<CAIUnit> pEnemy;
	CPtr<CStaticObject> pObj;
	CPtr<CBasicGun> pGun;

	CVec2 point;
	int nShootsLast;
	NTimer::STime lastCheck;
	float fSearchRadius;
	bool bFinish;
	bool bFired;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&pEnemy); f.Add(5,&pObj); f.Add(6,&pGun); f.Add(7,&point); f.Add(8,&nShootsLast); f.Add(9,&lastCheck); f.Add(10,&fSearchRadius); f.Add(11,&bFinish); f.Add(12,&bFired); return 0; }
	//
	void CheckArea();
	void FinishCommand();

public:
	static IUnitState* Instance( class CAIUnit *pUnit, const CVec2 &point );

	CArtilleryRangeAreaState() : pUnit( 0 ) { }
	CArtilleryRangeAreaState( class CAIUnit *pUnit, const CVec2 &point );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState(class CAICommand *pCommand);
	
	virtual EUnitStateNames GetName() { return EUSN_RANGING; }
	void GetRangeCircle( CCircle *pCircle ) const;

	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return point; }

	virtual bool IsAttacksUnit() const;
	virtual class CAIUnit* GetTargetUnit() const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtilleryInstallTransportState : public IUnitState
{
	OBJECT_BASIC_METHODS( CArtilleryInstallTransportState );

	enum EArtilleryInstallTransportState 
	{
		AITS_WAITING_FOR_CREW,
		AITS_INSTALLING,
	};

	ZDATA
	EArtilleryInstallTransportState eState;

	CPtr<CArtillery> pArtillery;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pArtillery); return 0; }
public:
	static IUnitState* Instance( class CArtillery *pArtillery );

	CArtilleryInstallTransportState() : pArtillery( 0 ) { }
	CArtilleryInstallTransportState( class CArtillery *pArtillery );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState(class CAICommand *pCommand);

	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtilleryUninstallTransportState : public IUnitState
{
	OBJECT_BASIC_METHODS( CArtilleryUninstallTransportState );
	
	enum EArtilleryUninstallTransportState 
	{
		AUTS_WAIT_FOR_UNINSTALL,
		AUTS_INSTALLING,

		AUTS_WAIT_FOR_UNINSTALL_TRANSPORT,
	};

	ZDATA
	EArtilleryUninstallTransportState eState;
	CPtr<CArtillery> pArtillery;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pArtillery); return 0; }
public:
	static IUnitState* Instance( class CArtillery *pArtillery );

	CArtilleryUninstallTransportState() : pArtillery( 0 ) { }
	CArtilleryUninstallTransportState( class CArtillery *pArtillery );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// пушка болтается за грузовиком
class CArtilleryBeingTowedPath;
class CArtilleryBeingTowedState: public IUnitState
{
	OBJECT_BASIC_METHODS( CArtilleryBeingTowedState );

	ZDATA
	CPtr<CArtillery> pArtillery;

	CPtr<CAITransportUnit> pTransport;
	// буксировка
	SAIAngle wLastTagDir;
	CVec2 vLastTagCenter;

	bool bInterrupted;
	CPtr<CArtilleryBeingTowedPath> pPath;
	NTimer::STime timeLastUpdate;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pArtillery); f.Add(3,&pTransport); f.Add(4,&wLastTagDir); f.Add(5,&vLastTagCenter); f.Add(6,&bInterrupted); f.Add(7,&pPath); f.Add(8,&timeLastUpdate); return 0; }
public:
	static IUnitState* Instance( class CArtillery *pArtillery, class CAITransportUnit * pTransport );

	CArtilleryBeingTowedState() : pArtillery( 0 ) { }
	CArtilleryBeingTowedState( class CArtillery *pArtillery, class CAITransportUnit * pTransport );
	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual EUnitStateNames GetName() { return EUSN_BEING_TOWED; }

	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
	
	class CAITransportUnit* GetTowingTransport() const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtilleryAttackState : public IUnitAttackingState, public CFreeFireManager
{
	OBJECT_BASIC_METHODS( CArtilleryAttackState );

	enum EAttackStates { EAS_NONE, EAS_ROTATING, EAS_FIRING };

	ZDATA_(CFreeFireManager)
	CPtr<CArtillery> pArtillery;
	EAttackStates eState;
	SAIAngle wDirToRotate;
	
	CPtr<CAIUnit> pEnemy;
	bool bAim;
	bool bFinish;

	CPtr<CBasicGun> pGun;
	bool bSwarmAttack;

	CDamageToEnemyUpdater damageToEnemyUpdater;
	int nEnemyParty;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CFreeFireManager*)this); f.Add(2,&pArtillery); f.Add(3,&eState); f.Add(4,&wDirToRotate); f.Add(5,&pEnemy); f.Add(6,&bAim); f.Add(7,&bFinish); f.Add(8,&pGun); f.Add(9,&bSwarmAttack); f.Add(10,&damageToEnemyUpdater); f.Add(11,&nEnemyParty); return 0; }
	//
	void FinishState();
public:
	static IUnitState* Instance( class CArtillery *pArtillery, class CAIUnit *pEnemy, bool bAim, const bool bSwarmAttack );

	CArtilleryAttackState() : pArtillery( 0 ) { }
	CArtilleryAttackState( class CArtillery *pArtillery, class CAIUnit *pEnemy, bool bAim, const bool bSwarmAttack );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_ATTACK_UNIT; }
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	virtual bool IsAttacksUnit() const { return true; }
	virtual class CAIUnit* GetTargetUnit() const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtilleryAttackCommonStatObjState : public IUnitAttackingState, public CFreeFireManager
{
	OBJECT_BASIC_METHODS( CArtilleryAttackCommonStatObjState );
	enum EAttackStates { EAS_NONE, EAS_ROTATING, EAS_FIRING };

	ZDATA_( CFreeFireManager )
	CPtr<CArtillery> pArtillery;
	CPtr<CStaticObject> pObj;
	CPtr<CBasicGun> pGun;

	EAttackStates eState;
	SAIAngle wDirToRotate;

	bool bAim;
	bool bFinish;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CFreeFireManager *)this); f.Add(2,&pArtillery); f.Add(3,&pObj); f.Add(4,&pGun); f.Add(5,&eState); f.Add(6,&wDirToRotate); f.Add(7,&bAim); f.Add(8,&bFinish); return 0; }
	//
	void FinishState();
public:
	static IUnitState* Instance( class CArtillery *pArtillery, class CStaticObject *pObj );

	CArtilleryAttackCommonStatObjState() : pArtillery( 0 ) { }
	CArtilleryAttackCommonStatObjState( class CArtillery *pArtillery, class CStaticObject *pObj );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_ATTACK_STAT_OBJECT; }
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	virtual bool IsAttacksUnit() const { return false; }
	virtual class CAIUnit* GetTargetUnit() const { return 0; }
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtilleryRestState : public CMechUnitRestState
{
	OBJECT_BASIC_METHODS( CArtilleryRestState );

	ZDATA_(CMechUnitRestState)
	CPtr<CArtillery> pArtillery;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CMechUnitRestState*)this); f.Add(2,&pArtillery); return 0; }
public:
	static IUnitState* Instance( class CArtillery *pArtillery, const CVec2 &guardPoint, const WORD wDir, const float fTimeToWait );
	
	CArtilleryRestState() : pArtillery( 0 ) { }
	CArtilleryRestState( class CArtillery *pArtillery, const CVec2 &guardPoint, const WORD wDir, const float fTimeToWait );

	virtual void Segment();
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CArtilleryAttackAviationState : public CSoldierAttackAviationState
{
	OBJECT_BASIC_METHODS( CArtilleryAttackAviationState );
	ZDATA_(CSoldierAttackAviationState)
	CPtr<CArtillery> pArtillery;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CSoldierAttackAviationState*)this); f.Add(2,&pArtillery); return 0; }
public:
	static IUnitState* Instance( class CArtillery *pArtillery, class CAviation *pPlane );
	
	CArtilleryAttackAviationState() : pArtillery( 0 ) { }
	CArtilleryAttackAviationState( class CArtillery *pArtillery, class CAviation *pPlane );

	virtual void Segment();
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

