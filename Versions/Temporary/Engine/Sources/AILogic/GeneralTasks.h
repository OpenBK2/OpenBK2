
#pragma once

#include "GeneralInternalInterfaces.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "Resistance.h"

// ****
// защита патча.
// ****
class CGeneral;
class CGeneralTaskToDefendPatch : public IGeneralTask, public IWorkerEnumerator, public IEnemyEnumerator 
{
	OBJECT_BASIC_METHODS(CGeneralTaskToDefendPatch);

	ZDATA
	CPtr<CGeneral> pOwner;

	NDb::SAIGeneralParcel patchInfo;
	int nCurReinforcePoint;
			
	float fSeverity;										// current severity of this task
	float fEnemyForce, fFriendlyForce, fFriendlyMobileForce;	// 
	float fMaxSeverity;

	bool bFinished;
	bool bWaitForFinish;								// wait for finish the task
	NTimer::STime timeLastUpdate;				

	CommonUnits infantryInTrenches;				// списки юнитов по типам
	CommonUnits infantryFree;
	CommonUnits tanksMobile;
	CommonUnits stationaryUnits;
	CommonUnits enemyForces;
	bool bResistanceRemoved;
	
	CommonUnits newTanks;									// to take tanks not 1 by 1
	
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&patchInfo); f.Add(4,&nCurReinforcePoint); f.Add(5,&fSeverity); f.Add(6,&fEnemyForce); f.Add(7,&fFriendlyForce); f.Add(8,&fFriendlyMobileForce); f.Add(9,&fMaxSeverity); f.Add(10,&bFinished); f.Add(11,&bWaitForFinish); f.Add(12,&timeLastUpdate); f.Add(13,&infantryInTrenches); f.Add(14,&infantryFree); f.Add(15,&tanksMobile); f.Add(16,&stationaryUnits); f.Add(17,&enemyForces); f.Add(18,&bResistanceRemoved); f.Add(19,&newTanks); return 0; }
	void CalcSeverity( const bool bEnemyUpdated, const bool bFriendlyUpdated );
	void InitTanks( class CCommonUnit *pUnit );
	void InitInfantryInTrenches( class CCommonUnit *pUnit );
public:
	CGeneralTaskToDefendPatch();
	void Init( const struct NDb::SAIGeneralParcel &_patchInfo, class CGeneral *pOwner );

	//ITask
	virtual ETaskName GetName() const ;
	virtual void AskForWorker( ICommander *pManager, const float _fMaxSeverity, const bool bInit = false );
	virtual void ReleaseWorker( ICommander *pManager, const float _fMinSeverity );
	virtual float GetSeverity() const;
	virtual bool IsFinished() const;
	virtual void CancelTask( ICommander *pManager ) ;
	virtual void Segment() ;

	//IEnumerator
	virtual bool EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType );
	virtual bool EvaluateWorker( CCommonUnit * pUnit, const enum EForceType eType ) const;
	
	//IEnemyEnumerator
	virtual bool EnumEnemy( class CAIUnit *pEnemy );

	int GetWorkerCount();
};

// ****
// hold reinforcemen
// ****
class CGeneralTaskToHoldReinforcement : public IGeneralTask, public IWorkerEnumerator
{
	OBJECT_BASIC_METHODS(CGeneralTaskToHoldReinforcement);
	typedef det_map<int, CVec2> UnitsPositions;
	ZDATA
	CommonUnits tanksFree;
	NDb::SAIGeneralParcel patchInfo;
	UnitsPositions unitsPositions;

	float fSeverity;										// current severity of this task
	int nCurReinforcePoint;	
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&tanksFree); f.Add(3,&patchInfo); f.Add(4,&unitsPositions); f.Add(5,&fSeverity); f.Add(6,&nCurReinforcePoint); return 0; }
public:
	CGeneralTaskToHoldReinforcement();
	void Init( const struct NDb::SAIGeneralParcel &_patchInfo );
	//empty reinforcement parcell. uses unit's initial positions as reinforcements positions.
	void Init(){ 	fSeverity = 0; }

	//ITask
	virtual ETaskName GetName() const { return ETN_HOLD_REINFORCEMENT; }
	virtual void AskForWorker( ICommander *pManager, const float fMaxSeverity, const bool bInit = false );
	virtual void ReleaseWorker( ICommander *pManager, const float fMinSeverity );
	virtual float GetSeverity() const ;
	virtual bool IsFinished() const { return false; } 
	virtual void CancelTask( ICommander *pManager ) ;
	virtual void Segment() ;
	virtual int NeedNBest( const enum EForceType eType ) const { return true; }


	//IEnumerator
	virtual bool EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType );
	virtual bool EvaluateWorker( CCommonUnit * pUnit, const enum EForceType eType ) const;
	int GetWorkerCount();
};

class CGeneralTaskRecaptureStorage : public IGeneralTask, public IWorkerEnumerator
{
	OBJECT_BASIC_METHODS(CGeneralTaskRecaptureStorage);
	ZDATA
	CommonUnits tanksFree;
	CVec2 vReinforcePoint;
	float fSeverity;										// current severity of this task

	bool bFinished;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&tanksFree); f.Add(3,&vReinforcePoint); f.Add(4,&fSeverity); f.Add(5,&bFinished); return 0; }
public:
	CGeneralTaskRecaptureStorage() {  }
	CGeneralTaskRecaptureStorage( const CVec2 & vReinforcePoint );

	//ITask
	virtual ETaskName GetName() const { return ETN_HOLD_REINFORCEMENT; }
	virtual void AskForWorker( ICommander *pManager, const float fMaxSeverity, const bool bInit = false );
	virtual void ReleaseWorker( ICommander *pManager, const float fMinSeverity );
	virtual float GetSeverity() const { return fSeverity; }
	virtual bool IsFinished() const ; 
	virtual void CancelTask( ICommander *pManager ) ;
	virtual void Segment() ;

	//IEnumerator
	virtual bool EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType );
	virtual bool EvaluateWorker( CCommonUnit * pUnit, const enum EForceType eType ) const;
};

class CGeneralTaskToSwarmToPoint : public IGeneralTask, public IWorkerEnumerator, public IEnemyEnumerator
{
	OBJECT_BASIC_METHODS(CGeneralTaskToSwarmToPoint);
	enum ESwarmState
	{
		ESS_PREPEARING,
		ESS_ATTACKING,
		ESS_REST,
		ESS_WAIT_FOR_WORKERS,
	};
	typedef std::vector< CPtr<CCommonUnit> > CTanks;

	ZDATA
	ESwarmState eState;
	CVec2 vPrepearCenter;

	SResistance curResistanceToAttack;
	float fSeverity;											// severity of this task
	float fMaxSeverity;
	float fMinSeverity;
	int nAdditionalIterations;						// number of additional iterations to try swarm to
	bool bFinished;
	bool bReleaseWorkers;									// this task does not always release workers
	NTimer::STime timeNextCheck;

	CPtr<CGeneral> pOwner;

	CTanks swarmingTanks;
	bool bResistanesBusyByUs;
	
	CVec2 vTanksPosition;									// center of tanks formation
	float fCurDistance;										// distance to nearest resistansce ( during enumeration )
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&vPrepearCenter); f.Add(4,&curResistanceToAttack); f.Add(5,&fSeverity); f.Add(6,&fMaxSeverity); f.Add(7,&fMinSeverity); f.Add(8,&nAdditionalIterations); f.Add(9,&bFinished); f.Add(10,&bReleaseWorkers); f.Add(11,&timeNextCheck); f.Add(12,&pOwner); f.Add(13,&swarmingTanks); f.Add(14,&bResistanesBusyByUs); f.Add(15,&vTanksPosition); f.Add(16,&fCurDistance); return 0; }

	void ClearResistanceToAcceptNewTask();
	bool IsTimeToRun() const;
	void Run();
	void SendToGroupPoint();
public:
	CGeneralTaskToSwarmToPoint();
	CGeneralTaskToSwarmToPoint( class CGeneral *pOwner );
	
	//ITask
	virtual ETaskName GetName() const { return ETN_SWARM_TO_POINT; }
	virtual void AskForWorker( ICommander *pManager, const float _fMaxSeverity, const bool bInit = false );
	virtual void ReleaseWorker( ICommander *pManager, const float fMinSeverity );
	virtual float GetSeverity() const ;
	virtual bool IsFinished() const ; 
	virtual void CancelTask( ICommander *pManager ) ;
	virtual void Segment() ;

	//IEnumerator
	virtual bool EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType );
	virtual bool EvaluateWorker( CCommonUnit * pUnit, const enum EForceType eType ) const;
	virtual int NeedNBest( const enum EForceType eType ) const { return 10000; }
	virtual float EvaluateWorkerRating( CCommonUnit * pUnit, const enum EForceType eType ) const;

	virtual bool EnumResistances( const struct SResistance &resistance );
	virtual bool EnumEnemy( class CAIUnit *pEnemy ) { NI_ASSERT( false, "didn't asked to" ); return false; }

	int GetWorkerCount();

	friend class CGeneralSwarmWaitForReady;
};

class CGeneralSwarmWaitForReady : public IGeneralDelayedTask 
{
	OBJECT_BASIC_METHODS( CGeneralSwarmWaitForReady );

	ZDATA
	CPtr<CGeneralTaskToSwarmToPoint>  pGeneralTask;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pGeneralTask); return 0; }
public:
	CGeneralSwarmWaitForReady() {  }
	CGeneralSwarmWaitForReady( class CGeneralTaskToSwarmToPoint *pTask )
		:pGeneralTask( pTask )	
	{
	}
	virtual bool IsTimeToRun() const { return pGeneralTask->IsTimeToRun(); }
	virtual void Run() { pGeneralTask->Run(); }
};



