#pragma once

#include "StatesFactory.h"
#include "UnitStates.h"
#include "DamageToEnemyUpdater.h"

#include <cstdint>

class CAIUnit;
class CBuilding;
class CAviation;
class CFormation;
class CPlaneShturmovikShootEstimator;

class CPlaneStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CPlaneStatesFactory );
	
	static CPtr<CPlaneStatesFactory> pFactory;
public:
	int operator&( IBinSaver &saver )
	{
		return 0;
	}
	static IStatesFactory* Instance();

	virtual bool CanCommandBeExecuted( class CAICommand *pCommand );

	virtual struct IUnitState* ProduceState( class CQueueUnit *pUnit, class CAICommand *pCommand );
	virtual struct IUnitState* ProduceRestState( class CQueueUnit *pUnit );
	
	// for Saving/Loading of static members
	friend class CStaticMembers;
};
class CPlaneDeffensiveFireShootEstimator;

// стрельба из бортовых стрелковых установок
class CPlaneDeffensiveFire
{
	ZDATA
	CPtr<CAviation> pOwner;
	NTimer::STime timeLastBSUUpdate;			// для поведения бортовых стрелковых установок

	CObj<CPlaneDeffensiveFireShootEstimator> pDefShootEstimator;
	CDamageToEnemyUpdater damageUpdater;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&timeLastBSUUpdate); f.Add(4,&pDefShootEstimator); f.Add(5,&damageUpdater); return 0; }
public:
		// проверить врагов и начать отстреливаться
	void AnalyzeBSU();
	CPlaneDeffensiveFire() : pOwner( 0 ), timeLastBSUUpdate( 0 ) {  }
	CPlaneDeffensiveFire( class CAviation *pPlane );
};

// переход из точти в точку
class CPlanePatrolState : public IUnitAttackingState
{
public: 
	class CEnemyContainer
	{
		ZDATA
		CPtr<CAIUnit> pOwner;

		float fTakenDamage;
		CPtr<CAIUnit> pEnemy;
		CPtr<CBuilding> pBuilding;
		bool bModifApplied;
	public:
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&fTakenDamage); f.Add(4,&pEnemy); f.Add(5,&pBuilding); f.Add(6,&bModifApplied); return 0; }
		CAIUnit *GetEnemy() { return pEnemy; }
	public:
		CEnemyContainer() : fTakenDamage( 0.0f ), bModifApplied( false ) { }
		CEnemyContainer( CAIUnit *pOwner ) : pOwner( pOwner ), fTakenDamage( 0.0f ), bModifApplied( false ) { }

		bool CanBreakTarget( const bool bBombsAllowed );
		CVec2 GetCenter() const;
		float GetZ() const;
		bool CanShootToTarget( class CBasicGun * pGun ) const;
		void StartBurst( class CBasicGun *pGun );

		void SetUnitEnemy( CAIUnit * pNewEnemy );
		void SetBuildingEnemy( CBuilding * pBuilding );

		bool IsValidBuilding() const;
		bool IsValidUnit() const;
		bool IsValid() const ; // is alive and is valid

		CAIUnit *GetGroundEnemy();
		CAviation *GetAviationEnemy();
		CBuilding *GetBuilding();
	};
protected:
	

	ZDATA
	CPtr<CAviation> pPlane;
	CObj<CPlaneShturmovikShootEstimator> pShootEstimator;
	CEnemyContainer enemie;								// enemie that we attack (ground target)

  std::vector<CVec2> vPatrolPoints;				// набор точек патрулирования
	int	 nCurPointIndex;								// текущая точка патрулирования
	bool bEconomyMode;									// if plane use smaller amount of fuel
	NTimer::STime timeNextScan;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pPlane); f.Add(3,&pShootEstimator); f.Add(4,&enemie); f.Add(5,&vPatrolPoints); f.Add(6,&nCurPointIndex); f.Add(7,&bEconomyMode); f.Add(8,&timeNextScan); return 0; }
protected:
	void InternalSetPoint( const CVec2 &_vPoint, const bool _bAttackState );


	const CVec2 &GetPoint() const { return vPatrolPoints[nCurPointIndex%vPatrolPoints.size()]; }
	const void ToNextPoint() { ++nCurPointIndex; }

	// return true if some enemy found
	bool TargetScan();

	bool IsBombsPresent() const;
	void DisableBombAbility();

  bool CanAttackEnemy( CAIUnit *pEnemy ) const;
	CAviation* FindBetterEnemiyPlane( CAviation * pEnemie, const CVec2 &vPoint, const float fRadius ) const;
	CAviation* FindNewEnemyPlane( const CVec2 &vPoint, const float fRadius ) const;
	bool FindNewGroundEnemie( const CVec2 &vPoint, const float fRadius );

	void AdvancePlane();
	bool PathSegment();
	void InitPathToPoint( const CVec3 &vPoint, const bool bEconomyMode, const bool bToHorisontal );
	void InitPathToEnemyPlane( class CPlanesFormation *_pEnemy );
	// was command to attack or drop. not MOVE
public:
	CPlanePatrolState() : nCurPointIndex( 0 ), pPlane( 0 ), bEconomyMode( true ), timeNextScan( 0 ) {  }
	CPlanePatrolState( CAviation *pPlane );
	int GetNPoints() const { return vPatrolPoints.size(); }
	// для добавления точек патрулирования
	void AddPoint( const CVec2 &vAddPoint );
	// force plane to move to provided point (patrolling)
	void SetPoint( const CVec2 &vPoint );
	void Escape();

	virtual bool IsAttacksUnit() const { return false; }
	virtual class CAIUnit* GetTargetUnit() const { return 0; }
	virtual EUnitStateNames GetName() { return EUSN_PLANE_PATROL; }
};

class CPlaneRestState : public CPlanePatrolState, public CPlaneDeffensiveFire
{
	OBJECT_BASIC_METHODS( CPlaneRestState )
		ZDATA_(CPlanePatrolState)
		ZPARENT(CPlaneDeffensiveFire)
		ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPlanePatrolState*)this); f.Add(2,(CPlaneDeffensiveFire*)this); return 0; }

public:

	CPlaneRestState() { }
	CPlaneRestState( CAviation *pUnit );

	virtual void Segment();

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return GetPoint(); }
	virtual EUnitStateNames GetName() { return EUSN_REST; }
	virtual bool IsRestState() const { return true; }
};

class CPlaneSwarmToState : public CPlanePatrolState, public CPlaneDeffensiveFire
{
	OBJECT_BASIC_METHODS( CPlaneSwarmToState )
	enum EPlaneSwarmToState
	{
		PSTS_ESTIMATING,
		PSTS_MOVING,
	};
	ZDATA_(CPlanePatrolState)
		ZPARENT(CPlaneDeffensiveFire)
	NTimer::STime timeNextScan;
	EPlaneSwarmToState eState;
	bool bScanForTarget;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPlanePatrolState*)this); f.Add(2,(CPlaneDeffensiveFire*)this); f.Add(3,&timeNextScan); f.Add(4,&eState); f.Add(5,&bScanForTarget); return 0; }

public:
	
	CPlaneSwarmToState() { }
	CPlaneSwarmToState( CAviation *pUnit, const CVec2 &_vPoint, const bool bScanForTarget );

	virtual void Segment();

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return GetPoint(); }
	virtual EUnitStateNames GetName() { return EUSN_SWARM; }
};

class CPlaneBombState : public CPlanePatrolState, public CPlaneDeffensiveFire
{
	OBJECT_BASIC_METHODS( CPlaneBombState );
	
	enum ECurBombState
	{
		ECBS_ESTIMATE,
		ECBS_GAIN_DISTANCE,

		ECBS_APPROACH,
		ECBS_WAIT_BOMBPOINT_REACH,
		ECBS_ATTACK_HORISONTAL,

		ECBS_AIM_TO_NEXT_POINT,
		ECBS_AIM_TO_NEXT_POINT_2,
		ECBS_START_ESACPE,
	};
	ZDATA_(CPlanePatrolState)
		ZPARENT(CPlaneDeffensiveFire)
	ECurBombState eState;

	float fInitialHeight ;
	float fStartAttackDist;
	bool bHaveBombs;
	bool bSingleSalvo;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPlanePatrolState*)this); f.Add(2,(CPlaneDeffensiveFire*)this); f.Add(3,&eState); f.Add(4,&fInitialHeight); f.Add(5,&fStartAttackDist); f.Add(6,&bHaveBombs); f.Add(7,&bSingleSalvo); return 0; }
	
	float RecalcStartAttack() const;
public:

	CPlaneBombState() : bSingleSalvo( false ) { }
	CPlaneBombState( CAviation *pUnit, const CVec2 &vPoint, bool bSingleSalvo = false );

	virtual void Segment();

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return GetPoint(); }
	virtual EUnitStateNames GetName() { return EUSN_BOMBING; }
};

// for drop paratroopers in specific point
class CPlaneParaDropState : public CPlanePatrolState, public CPlaneDeffensiveFire
{
	OBJECT_BASIC_METHODS( CPlaneParaDropState );
	enum EPlaneParaDropState
	{
		PPDS_ESTIMATE,
		PPDS_APPROACHNIG,
		PPDS_PREPARE_TO_DROP,
		PPDS_DROPPING,
		PPDS_TO_NEXT_SQUAD,
	};
	ZDATA_(CPlanePatrolState)
		ZPARENT(CPlaneDeffensiveFire)
	EPlaneParaDropState eState;

	bool bDrop1Squad;
	CPtr<CFormation> pSquad; // взвод паращютистов
	int nDroppingSoldier;									// current soldier to drop

	CVec2 vLastDrop;// точка, в которой выброшен последний парашютист
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPlanePatrolState*)this); f.Add(2,(CPlaneDeffensiveFire*)this); f.Add(3,&eState); f.Add(4,&bDrop1Squad); f.Add(5,&pSquad); f.Add(6,&nDroppingSoldier); f.Add(7,&vLastDrop); return 0; }

	//true if some tiles around drop site are unlocked.
	bool CanDrop( const CVec2 & point );

public:

	CPlaneParaDropState () { }
	CPlaneParaDropState ( CAviation *pPlane, const enum EActionLeaveParam param, const CVec2 &vLandPoint, CFormation *_pSquad );

	virtual void Segment();

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

// state for fighters. Duties:
// 1) fight incoming enemie bombers
class CPlaneFighterPatrolState : public CPlanePatrolState, public CPlaneDeffensiveFire
{
	OBJECT_NOCOPY_METHODS( CPlaneFighterPatrolState );
	
  enum ECurFighterOnEnemieState
	{
		ECFS_ESCAPE,
		ECFS_GOTO_GUARDPOINT,
		ECFS_GOING_TO_GUARDPOINT,
		ECFS_ENGAGE_TARGET,
		ECFS_AIM_TO_NEXT_POINT,
		ECFS_FIND_ENEMY_OR_NEXT_POINT,
		ECFS_AVOID_COLLISION,
		
		ECFS_RETURN_TO_MAP,
	};
	ZDATA_(CPlanePatrolState)
		ZPARENT(CPlaneDeffensiveFire)
	ECurFighterOnEnemieState eState;

	float fPartolRadius;									// patrol radius of this state
	ZSKIP
	CPtr<CAviation> pEnemie;							//enemie that we attack (plane)

	NTimer::STime timeOfLastPathUpdate;		//last update of path
	NTimer::STime timeLastCheck;					// последняя проверка на наличие патронов
	bool bAmmoRemains;
	NTimer::STime nextPathUpdate;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPlanePatrolState*)this); f.Add(2,(CPlaneDeffensiveFire*)this); f.Add(3,&eState); f.Add(4,&fPartolRadius); f.Add(6,&pEnemie); f.Add(7,&timeOfLastPathUpdate); f.Add(8,&timeLastCheck); f.Add(9,&bAmmoRemains); f.Add(10,&nextPathUpdate); return 0; }
	void TryInitPathToEnemie( const bool bNewEnemy );

	bool IsEnemyAlive( CAviation * pEnemie ) const;
	void FinishState();
	// возвращает true если pNewEnemy != pCompareEnemy
	bool SetNewEnemy( CAviation *pNewEnemy, CAviation *pCompareEnemy );
public:

	CPlaneFighterPatrolState () { }
	CPlaneFighterPatrolState ( CAviation *_pPlane, const CVec2 &vPoint, CAviation *pTarget );

	virtual void Segment();

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1,-1 ); }

	virtual bool IsAttacksUnit() const ;
	virtual class CAIUnit* GetTargetUnit() const ;

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

class CPlaneShturmovikPatrolState : public CPlanePatrolState, public CPlaneDeffensiveFire 
{
	OBJECT_BASIC_METHODS( CPlaneShturmovikPatrolState );
public: 

  enum EPlaneShturmovikPatrolState
	{
		PSPS_ESCAPE,
		PSPS_GOTO_GUARDPOINT,
		PSPS_GOING_TO_GUARDPOINT,
		PSPS_AIM_TO_NEXT_POINT,
		PSPS_FIND_ENEMY_OR_NEXT_POINT,

		PSPS_APPROACH_TARGET,
		PSPS_APPROACHING_TARGET,
		PSPS_ENGAGING_TARGET,
		PSPS_FIRE_TO_WORLD,

		PSPS_TURN_TO_TARGET,
		PSPS_TURNING_TO_TARGET,
		PSPS_GAIN_HEIGHT,
		PSPS_GAINING_HEIGHT,
	};


private:
	
	ZDATA_(CPlanePatrolState)
	ZPARENT(CPlaneDeffensiveFire)
	CPtr<CAviation> pPlane;
	EPlaneShturmovikPatrolState eState;
	CVec2 vCurTargetPoint;								// точка, куда направляется самолет

	ZSKIP
	
	NTimer::STime timeOfLastPathUpdate;		// last update of path
	NTimer::STime timeLastCheck ;					// проверка на наличие патронов

	float fStartAttackDist;								// дистанция для начала пикирования
	float fFinishAttckDist;								// дистанция выхода из атаки
	float fTurnRadius;										// радиус поворота штурмовика
	bool bAmmoRemains;
	bool bMustDrop1Bomb;
	bool bBombsDropped;
	bool bShootedToEnemie;								// performed any shooting during dive
	bool bFirstApproach;
	bool bDiveInProgress;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPlanePatrolState*)this); f.Add(2,(CPlaneDeffensiveFire*)this); f.Add(3,&pPlane); f.Add(4,&eState); f.Add(5,&vCurTargetPoint); f.Add(7,&timeOfLastPathUpdate); f.Add(8,&timeLastCheck); f.Add(9,&fStartAttackDist); f.Add(10,&fFinishAttckDist); f.Add(11,&fTurnRadius); f.Add(12,&bAmmoRemains); f.Add(13,&bMustDrop1Bomb); f.Add(14,&bBombsDropped); f.Add(15,&bShootedToEnemie); f.Add(16,&bFirstApproach); f.Add( 17, &bDiveInProgress ); return 0; }
	void TryInitPathToEnemie( const bool bForceNewPath );
	void TryInitPathToPoint( const CVec3 &vPos, const bool bNewPoint, const bool bToHorisontal );

	// выбери лучшее
	CAIUnit* FindEnemyInPossibleDiveSector();
	CAIUnit* FindEnemyInFiringSector();

	void TryBurstAllGuns();
	void TryDropBombs();
	void TryBurstAllGunsToPoints();
	bool IsTargetBehind( const CVec2 &vTarget ) const;

	void OnFindEnemy();
	void OnTurnToTarget();

	void FinishState();
public:

	CPlaneShturmovikPatrolState () : pPlane( 0 ) { }
	CPlaneShturmovikPatrolState ( CAviation *_pPlane, const CVec2 &vPoint, CAIUnit *pTarget, CBuilding *pBuilding, const bool bMustDropBombs );

	virtual void Segment();

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return GetPoint(); }
};

class CPlaneScoutState : public CPlanePatrolState, public CPlaneDeffensiveFire
{
	OBJECT_BASIC_METHODS( CPlaneScoutState );
	
  enum EPlaneScoutState 
	{
		EPSS_GOTO_GUARDPOINT,
		EPSS_GOING_TO_GUARDPOINT,
		EPSS_AIM_TO_NEXT_POINT,
		EPSS_ESCAPE,
	};
	ZDATA_(CPlanePatrolState)
		ZPARENT(CPlaneDeffensiveFire)
	EPlaneScoutState eState;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPlanePatrolState*)this); f.Add(2,(CPlaneDeffensiveFire*)this); f.Add(3,&eState); return 0; }
public:
	CPlaneScoutState () { }
	CPlaneScoutState ( CAviation *_pPlane ) ;
	virtual void Segment();
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return GetPoint(); }
};

//ACTION_MOVE_PLANE_LEAVE
//plane must leave the map trough the some point
class CPlaneLeaveState : public CPlanePatrolState, public CPlaneDeffensiveFire
{
	OBJECT_BASIC_METHODS( CPlaneLeaveState );
	enum EPlaneLeaveState
	{
		EPLS_STARTING,
		EPLS_IN_ROUTE,
	};

	ZDATA_(CPlanePatrolState)
		ZPARENT(CPlaneDeffensiveFire)
	CPtr<IUnitState> pMoveToExitPoint; 
	EPlaneLeaveState eState;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPlanePatrolState*)this); f.Add(2,(CPlaneDeffensiveFire*)this); f.Add(3,&pMoveToExitPoint); f.Add(4,&eState); return 0; }


public:

	CPlaneLeaveState() { }
	CPlaneLeaveState( CAviation *pPlane );

	virtual void Segment();

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
	virtual EUnitStateNames GetName() { return EUSN_PLANE_LEAVE; }

	// for Saving/Loading of static members
	friend class CStaticMembers;
};

// смерть самолета.
class CPlaneFlyDeadState : public CPlanePatrolState
{
	OBJECT_BASIC_METHODS( CPlaneFlyDeadState );
	enum EPlaneDeadState 
	{
		EPDS_START_DIVE,
		EPDS_DIVE,
		EPDS_ESTIMATE,
		EPDS_WAIT_FINISH_PATH,
	};

	class CDeadZone
	{
		float fMaxX, fMinX;
		float fMinY, fMaxY;
	public:
		CDeadZone() : fMaxX( 0.0f ), fMinX( 0.0f ), fMaxY( 0.0f ), fMinY( 0.0f ) { }
		void Init();
		bool IsInZone( const CVec2 &vPoint );
		void AdjustEscapePoint( CVec2 * pPoint );
	};

	ZDATA_(CPlanePatrolState)
	EPlaneDeadState eState;
	CDeadZone deadZone;											// вне зтого rect умирают самолеты

	float fHeight;
	bool bFatality;
	bool bExplodeInstantly;									// if false plane will explode after finish dive.
	NTimer::STime timeStart;								// start death time
	bool bGroundCrash;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPlanePatrolState*)this); f.Add(2,&eState); f.Add(3,&deadZone); f.Add(4,&fHeight); f.Add(5,&bFatality); f.Add(6,&bExplodeInstantly); f.Add(7,&timeStart); f.Add(8,&bGroundCrash); return 0; }
	void InitPathToNearestPoint();
	float CalcPath( const uint16_t wCurDir, const uint8_t nDesiredDir, const bool bRight, const float fTurnRadius, CVec2 *vDestPoint );
public:

	CPlaneFlyDeadState () { }
	CPlaneFlyDeadState ( CAviation *_pPlane );

	virtual void Segment();

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const ;
	virtual EUnitStateNames GetName() { return EUSN_FLY_DEAD; }
};

// самолет самоубийца (V-2)
class CPlaneSuicideState : public IUnitState
{
	OBJECT_NOCOPY_METHODS( CPlaneSuicideState )
	enum EPlaneSuicideState
	{
		EPSS_START,	//стэйт только начался
		EPSS_FLY,	//летим к точке самоубийства
		EPSS_DIVE,	//самоубиваемся
	};
	ZDATA
		CPtr<CAviation> pPlane;
		EPlaneSuicideState eState;
		CVec2 vTarget;
		CDBPtr<SWeaponRPGStats> pWeapon;
		float fDistToDive2;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pPlane); f.Add(3,&eState); f.Add(4,&vTarget); f.Add(5,&pWeapon); f.Add(6,&fDistToDive2); return 0; }
private:
public:
	CPlaneSuicideState() : pPlane( 0 ) {}
	CPlaneSuicideState( CAviation *pPlane, const CVec2 &vTarget );

	void Segment();
	
	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return vTarget; }
	virtual EUnitStateNames GetName() { return EUSN_PLANE_SUICIDE; }
};


