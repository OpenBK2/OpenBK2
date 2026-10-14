
#pragma once

#include "GeneralInternalInterfaces.h"
#include "Resistance.h"
#include "Stats_B2_M1/DBMapInfo.h"

// ****
// защита патча.
// ****
class CGeneral;

class CGeneralTaskToDefendPatch : public IGeneralTask, public IWorkerEnumerator, public IEnemyEnumerator
{
	OBJECT_BASIC_METHODS( CGeneralTaskToDefendPatch );

	ZDATA
	CPtr<CGeneral> pOwner;

	NDb::SAIGeneralParcel patchInfo;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nCurReinforcePoint = {};

	float fSeverity = {};                                                   // current severity of this task
	float fEnemyForce = {}, fFriendlyForce = {}, fFriendlyMobileForce = {}; //
	float fMaxSeverity = {};

	bool bFinished = {};
	bool bWaitForFinish = {}; // wait for finish the task
	NTimer::STime timeLastUpdate = {};

	CommonUnits infantryInTrenches; // списки юнитов по типам
	CommonUnits infantryFree;
	CommonUnits tanksMobile;
	CommonUnits stationaryUnits;
	CommonUnits enemyForces;
	bool bResistanceRemoved = {};

	CommonUnits newTanks; // to take tanks not 1 by 1

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pOwner );
		f.Add( 3, &patchInfo );
		f.Add( 4, &nCurReinforcePoint );
		f.Add( 5, &fSeverity );
		f.Add( 6, &fEnemyForce );
		f.Add( 7, &fFriendlyForce );
		f.Add( 8, &fFriendlyMobileForce );
		f.Add( 9, &fMaxSeverity );
		f.Add( 10, &bFinished );
		f.Add( 11, &bWaitForFinish );
		f.Add( 12, &timeLastUpdate );
		f.Add( 13, &infantryInTrenches );
		f.Add( 14, &infantryFree );
		f.Add( 15, &tanksMobile );
		f.Add( 16, &stationaryUnits );
		f.Add( 17, &enemyForces );
		f.Add( 18, &bResistanceRemoved );
		f.Add( 19, &newTanks );
		return 0;
	}

	void CalcSeverity( const bool bEnemyUpdated, const bool bFriendlyUpdated );
	void InitTanks( class CCommonUnit *pUnit );
	void InitInfantryInTrenches( class CCommonUnit *pUnit );

public:
	CGeneralTaskToDefendPatch();
	void Init( const struct NDb::SAIGeneralParcel &_patchInfo, class CGeneral *pOwner );

	//ITask
	ETaskName GetName() const override;
	void AskForWorker( ICommander *pManager, const float _fMaxSeverity, const bool bInit = false ) override;
	void ReleaseWorker( ICommander *pManager, const float _fMinSeverity ) override;
	float GetSeverity() const override;
	bool IsFinished() const override;
	void CancelTask( ICommander *pManager ) override;
	void Segment() override;

	//IEnumerator
	bool EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType ) override;
	bool EvaluateWorker( CCommonUnit *pUnit, const enum EForceType eType ) const override;

	//IEnemyEnumerator
	bool EnumEnemy( class CAIUnit *pEnemy ) override;

	int GetWorkerCount() override;
};

// ****
// hold reinforcemen
// ****
class CGeneralTaskToHoldReinforcement : public IGeneralTask, public IWorkerEnumerator
{
	OBJECT_BASIC_METHODS( CGeneralTaskToHoldReinforcement );
	typedef det_map<int, CVec2> UnitsPositions;
	ZDATA
	CommonUnits tanksFree;
	NDb::SAIGeneralParcel patchInfo;
	UnitsPositions unitsPositions;

	float fSeverity = {}; // current severity of this task
	int nCurReinforcePoint = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &tanksFree );
		f.Add( 3, &patchInfo );
		f.Add( 4, &unitsPositions );
		f.Add( 5, &fSeverity );
		f.Add( 6, &nCurReinforcePoint );
		return 0;
	}

public:
	CGeneralTaskToHoldReinforcement();
	void Init( const struct NDb::SAIGeneralParcel &_patchInfo );

	//empty reinforcement parcell. uses unit's initial positions as reinforcements positions.
	void Init()
	{
		fSeverity = 0;
	}

	//ITask
	ETaskName GetName() const override
	{
		return ETN_HOLD_REINFORCEMENT;
	}

	void AskForWorker( ICommander *pManager, const float fMaxSeverity, const bool bInit = false ) override;
	void ReleaseWorker( ICommander *pManager, const float fMinSeverity ) override;
	float GetSeverity() const override;

	bool IsFinished() const override
	{
		return false;
	}

	void CancelTask( ICommander *pManager ) override;
	void Segment() override;

	int NeedNBest( const enum EForceType eType ) const override
	{
		return true;
	}

	//IEnumerator
	bool EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType ) override;
	bool EvaluateWorker( CCommonUnit *pUnit, const enum EForceType eType ) const override;
	int GetWorkerCount() override;
};

class CGeneralTaskRecaptureStorage : public IGeneralTask, public IWorkerEnumerator
{
	OBJECT_BASIC_METHODS( CGeneralTaskRecaptureStorage );
	ZDATA
	CommonUnits tanksFree;
	CVec2 vReinforcePoint;
	float fSeverity = {}; // current severity of this task

	bool bFinished = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &tanksFree );
		f.Add( 3, &vReinforcePoint );
		f.Add( 4, &fSeverity );
		f.Add( 5, &bFinished );
		return 0;
	}

public:
	CGeneralTaskRecaptureStorage() {}

	CGeneralTaskRecaptureStorage( const CVec2 &vReinforcePoint );

	//ITask
	ETaskName GetName() const override
	{
		return ETN_HOLD_REINFORCEMENT;
	}

	void AskForWorker( ICommander *pManager, const float fMaxSeverity, const bool bInit = false ) override;
	void ReleaseWorker( ICommander *pManager, const float fMinSeverity ) override;

	float GetSeverity() const override
	{
		return fSeverity;
	}

	bool IsFinished() const override;
	void CancelTask( ICommander *pManager ) override;
	void Segment() override;

	//IEnumerator
	bool EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType ) override;
	bool EvaluateWorker( CCommonUnit *pUnit, const enum EForceType eType ) const override;
};

class CGeneralTaskToSwarmToPoint : public IGeneralTask, public IWorkerEnumerator, public IEnemyEnumerator
{
	OBJECT_BASIC_METHODS( CGeneralTaskToSwarmToPoint );

	enum ESwarmState
	{
		ESS_PREPEARING,
		ESS_ATTACKING,
		ESS_REST,
		ESS_WAIT_FOR_WORKERS,
	};

	typedef std::vector<CPtr<CCommonUnit>> CTanks;

	ZDATA
	ESwarmState eState = {};
	CVec2 vPrepearCenter;

	SResistance curResistanceToAttack;
	float fSeverity = {}; // severity of this task
	float fMaxSeverity = {};
	float fMinSeverity = {};
	int nAdditionalIterations = {}; // number of additional iterations to try swarm to
	bool bFinished = {};
	bool bReleaseWorkers = {}; // this task does not always release workers
	NTimer::STime timeNextCheck = {};

	CPtr<CGeneral> pOwner;

	CTanks swarmingTanks;
	bool bResistanesBusyByUs = {};

	CVec2 vTanksPosition;    // center of tanks formation
	float fCurDistance = {}; // distance to nearest resistansce ( during enumeration )

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &eState );
		f.Add( 3, &vPrepearCenter );
		f.Add( 4, &curResistanceToAttack );
		f.Add( 5, &fSeverity );
		f.Add( 6, &fMaxSeverity );
		f.Add( 7, &fMinSeverity );
		f.Add( 8, &nAdditionalIterations );
		f.Add( 9, &bFinished );
		f.Add( 10, &bReleaseWorkers );
		f.Add( 11, &timeNextCheck );
		f.Add( 12, &pOwner );
		f.Add( 13, &swarmingTanks );
		f.Add( 14, &bResistanesBusyByUs );
		f.Add( 15, &vTanksPosition );
		f.Add( 16, &fCurDistance );
		return 0;
	}

	void ClearResistanceToAcceptNewTask();
	bool IsTimeToRun() const;
	void Run();
	void SendToGroupPoint();

public:
	CGeneralTaskToSwarmToPoint();
	CGeneralTaskToSwarmToPoint( class CGeneral *pOwner );

	//ITask
	ETaskName GetName() const override
	{
		return ETN_SWARM_TO_POINT;
	}

	void AskForWorker( ICommander *pManager, const float _fMaxSeverity, const bool bInit = false ) override;
	void ReleaseWorker( ICommander *pManager, const float fMinSeverity ) override;
	float GetSeverity() const override;
	bool IsFinished() const override;
	void CancelTask( ICommander *pManager ) override;
	void Segment() override;

	//IEnumerator
	bool EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType ) override;
	bool EvaluateWorker( CCommonUnit *pUnit, const enum EForceType eType ) const override;

	int NeedNBest( const enum EForceType eType ) const override
	{
		return 10000;
	}

	float EvaluateWorkerRating( CCommonUnit *pUnit, const enum EForceType eType ) const override;

	bool EnumResistances( const struct SResistance &resistance ) override;

	bool EnumEnemy( class CAIUnit *pEnemy ) override
	{
		NI_ASSERT( false, "didn't asked to" );
		return false;
	}

	int GetWorkerCount() override;

	friend class CGeneralSwarmWaitForReady;
};

class CGeneralSwarmWaitForReady : public IGeneralDelayedTask
{
	OBJECT_BASIC_METHODS( CGeneralSwarmWaitForReady );

	ZDATA
	CPtr<CGeneralTaskToSwarmToPoint> pGeneralTask;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pGeneralTask );
		return 0;
	}

public:
	CGeneralSwarmWaitForReady() {}

	CGeneralSwarmWaitForReady( class CGeneralTaskToSwarmToPoint *pTask )
		: pGeneralTask( pTask )
	{
	}

	bool IsTimeToRun() const override
	{
		return pGeneralTask->IsTimeToRun();
	}

	void Run() override
	{
		pGeneralTask->Run();
	}
};
