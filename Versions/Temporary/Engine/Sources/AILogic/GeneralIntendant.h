
#pragma once
#include "Commander.h"
#include "General.h"
#include "EnemyRememberer.h"
#include "Stats_B2_M1/DBMapInfo.h"

class CResupplyCellInfo : public CAIObjectBase
{
	OBJECT_BASIC_METHODS(CResupplyCellInfo);

	typedef std::unordered_map< int/*Unique ID*/, BYTE > CResupplyInfo;
	ZDATA
	CResupplyInfo resupplyInfo;
	std::vector<float> resupplyCount;
	float fCount;													// whole weight
	BYTE cMarkedUnderSupply;							// indicates that this cell is under supply
	SVector vCell;												// coordinates of the cell ( in GetGeneralCell )
	NTimer::STime timeLastDanger;					// time when last truck was killed near this cell
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&resupplyInfo); f.Add(3,&resupplyCount); f.Add(4,&fCount); f.Add(5,&cMarkedUnderSupply); f.Add(6,&vCell); f.Add(7,&timeLastDanger); return 0; }
	void RemoveUnitResupplyInternal( class CCommonUnit *pUnit, const enum EResupplyType eType );
public:
	CResupplyCellInfo();
	void Init( const SVector &_vPos ) { vCell = _vPos; }
	SVector GetCenter() const { return vCell; }
	void AddUnitResupply( class CCommonUnit * pUnit, const enum EResupplyType eType );
	void RemoveUnitResupply( class CCommonUnit * pUnit, const enum EResupplyType eType );
	
	BYTE RemoveUnit( class CCommonUnit * pUnit );
	void AddUnit( class CCommonUnit * pUnit, const BYTE cRes );
	const bool IsUnitRegistered( CCommonUnit * pUnit ) const;

	// for marking that this cell is under supply.
	void MarkUnderSupply( const enum EResupplyType eType, const bool bSupply = true );
	const bool IsMarkedUnderSupply( const enum EResupplyType eType ) const { return cMarkedUnderSupply & (1<<eType); }
	// dander
	void SetDanger( const NTimer::STime timeDanger ) ;
	const bool IsDangerous() const ;

	float GetNNeeded( const BYTE cTypeMask ) const;
	bool IsEmpty() const;
	float GetNNeeded( const enum EResupplyType eType ) const 
	{ 
		return resupplyCount[eType]; 
	}

	static bool IsUnitSuitable( const class CCommonUnit *pUnit, const enum EResupplyType eType );
	static void MoveUnitToCell( const class CCommonUnit *pUnit, const enum EResupplyType eType );
	static void IssueCommand( class CCommonUnit * pUnit, const enum EResupplyType eType, const CVec2 &vResupplyCenter );

	struct SSortByResupplyMaskPredicate
	{
		BYTE cMask;
		SSortByResupplyMaskPredicate( const BYTE cMask ) : cMask( cMask ) {  }
		bool operator()( const CPtr<CResupplyCellInfo> &s1, const CPtr<CResupplyCellInfo> &s2 ) const
		{
			float s1n = s1->GetNNeeded( cMask );
			float s2n = s2->GetNNeeded( cMask );
			if (s1n != s2n)
				return s1n > s2n; 
			else
			{
				auto p1 = s1->GetCenter();
				auto p2 = s2->GetCenter();
				if (p1.x != p2.x)
					return p1.x > p2.x;
				if (p1.y != p2.y)
					return p1.y > p2.y;
				
				// this should never happen since centers are always unique
				return false;
			}
		}
	};

	// were transport must be to cover all units with resupply.
	CVec2 CalcResupplyPos( const enum EResupplyType eType ) const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CBuilding;
typedef std::list< CPtr<CBuilding> > Storages;

class CGeneralTaskToDefendStorage : public IGeneralTask, public IWorkerEnumerator
{
	OBJECT_BASIC_METHODS(CGeneralTaskToDefendStorage);

public:
	class CWaitForChangePlayer : public IGeneralDelayedTask 
	{
		OBJECT_BASIC_METHODS(CWaitForChangePlayer);
	ZDATA
		CPtr<CGeneralTaskToDefendStorage> pMainTask;
		CPtr<CBuilding> pStorage;
		int nParty;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMainTask); f.Add(3,&pStorage); f.Add(4,&nParty); return 0; }
	public:
		CWaitForChangePlayer() {  }
		CWaitForChangePlayer( CBuilding * pStorage, CGeneralTaskToDefendStorage * pMainTask, const int nParty );
		virtual bool IsTimeToRun() const;
		virtual void Run();
	};
private:
	enum ETaskState
	{
		TS_OPERATE,
		TS_START_RECAPTURE,
		TS_RECAPTURE,
		TS_FINISH_RECAPTURE,
		TS_START_REPAIR,
		TS_REPAIR,
		TS_FINISHED,
	};
	ZDATA
	ETaskState eState;

	float fSeverity;
	int nParty;
	CPtr<CBuilding> pStorage;
	CPtr<CCommonUnit> pRepairTransport;
	SAIAngle wRequestID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&fSeverity); f.Add(4,&nParty); f.Add(5,&pStorage); f.Add(6,&pRepairTransport); f.Add(7,&wRequestID); return 0; }

	void Recaptured();
public:
	CGeneralTaskToDefendStorage () { }
	CGeneralTaskToDefendStorage ( CBuilding * pStorage, const int nParty );

	virtual ETaskName GetName() const { return ENT_DEFEND_ESTORAGE; }
	virtual void AskForWorker( ICommander *pManager, const float fMaxSeverity, const bool bInit = false );
	virtual void ReleaseWorker( ICommander *pManager, const float fMinSeverity );
	virtual void CancelTask( ICommander *pManager ) ;
	virtual void Segment() ;
	

	virtual float GetSeverity() const { return fSeverity; }
	virtual bool IsFinished() const { return eState == TS_FINISHED; } 

	//IWorkerEnumerator
	virtual bool EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType );
	virtual bool EvaluateWorker( CCommonUnit * pUnit, const enum EForceType eType ) const;
	virtual int NeedNBest( const enum EForceType eType ) const ;
	virtual float EvaluateWorkerRating( CCommonUnit * pUnit, const enum EForceType eType ) const ;

	friend class CWaitForChangePlayer;
};

class CGeneralTaskToResupplyCell : public IGeneralTask, public IWorkerEnumerator
{
	OBJECT_BASIC_METHODS(CGeneralTaskToResupplyCell);
	class CGeneralIntendant *pCells;

	ZDATA
		ZSKIP
		ZONSERIALIZE
		CPtr<CResupplyCellInfo> pCell;
		CVec2 vResupplyCenter;
		int nParty;
		CPtr<CCommonUnit> pResupplyTransport;
		bool bFinished;
		float fSeverity;
		NTimer::STime timeNextCheck;
		EResupplyType eResupplyType;
	ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); f.Add(3,&pCell); f.Add(4,&vResupplyCenter); f.Add(5,&nParty); f.Add(6,&pResupplyTransport); f.Add(7,&bFinished); f.Add(8,&fSeverity); f.Add(9,&timeNextCheck); f.Add(10,&eResupplyType); return 0; }
	void OnSerialize( IBinSaver &saver );
public:
	CGeneralTaskToResupplyCell() : pCells( 0 ) { }
	CGeneralTaskToResupplyCell( CResupplyCellInfo * pCell, const int nParty, const enum EResupplyType eType, class CGeneralIntendant *pCells  );

	virtual ETaskName GetName() const { return ETN_RESUPPLYCELL; }
	virtual void AskForWorker( ICommander *pManager, const float fMaxSeverity, const bool bInit = false );
	virtual void ReleaseWorker( ICommander *pManager, const float fMinSeverity );
	virtual void CancelTask( ICommander *pManager ) ;
	virtual void Segment() ;
	

	virtual float GetSeverity() const { return fSeverity; }
	virtual bool IsFinished() const { return bFinished; }

	//IWorkerEnumerator
	virtual bool EnumWorker( CCommonUnit *pUnit, const enum EForceType eType );
	virtual bool EvaluateWorker( CCommonUnit * pUnit, const enum EForceType eType ) const;
	virtual int NeedNBest( const enum EForceType eType ) const ;
	virtual float EvaluateWorkerRating( CCommonUnit * pUnit, const enum EForceType eType ) const ;
};

class CGeneralTaskCheckCellDanger : public IGeneralDelayedTask
{
	OBJECT_BASIC_METHODS( CGeneralTaskCheckCellDanger );
	ZDATA
	CPtr<IGeneralTask> pTask;
	CPtr<CResupplyCellInfo> pCell;
	CPtr<ICommander> pCommander;
	EResupplyType eResupplyType;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTask); f.Add(3,&pCell); f.Add(4,&pCommander); f.Add(5,&eResupplyType); return 0; }
public:
	CGeneralTaskCheckCellDanger() {  }
	CGeneralTaskCheckCellDanger( struct IGeneralTask *_pTask, class CResupplyCellInfo *_pCell, enum EResupplyType _eResupplyType, struct ICommander *_pCommander )
		: pTask( _pTask ), pCell( _pCell ), eResupplyType( _eResupplyType ), pCommander( _pCommander )
	{
	}
	
	virtual bool IsTimeToRun() const
	{
		return !pTask || pTask->IsFinished() || pCell->IsDangerous() || pCell->GetNNeeded( eResupplyType ) == 0;
	}
	virtual void Run()
	{
		if ( pTask )
			if ( !pTask->IsFinished() )
				pTask->CancelTask( pCommander );
	}
};

class CArtillery;
class CFormation;
class CFreeArtilleryHolder : public CAIObjectBase
{
	struct SArtilleryInfo
	{
		ZDATA
			CPtr<CArtillery> pArtillery;
			CPtr<CFormation> pCatchingFormation;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pArtillery); f.Add(3,&pCatchingFormation); return 0; }

		SArtilleryInfo() : pArtillery( 0 ), pCatchingFormation( 0 ) {}
		SArtilleryInfo( CArtillery * _pArtillery ) : pArtillery( _pArtillery ), pCatchingFormation( 0 ) {}
	};
	typedef std::unordered_map<int, SArtilleryInfo> TArtilleries;
	OBJECT_NOCOPY_METHODS( CFreeArtilleryHolder );
	TArtilleries::iterator itCurrent;
	NTimer::STime timeFromReset;
	ZDATA
		TArtilleries artilleries;
		ZONSERIALIZE
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&artilleries); OnSerialize( f ); return 0; }
private:
	const bool IsExists( const int nUnitID ) const { return artilleries.find( nUnitID ) != artilleries.end(); }
	const bool IsValidCurrent() const;
	const bool IsFreeCurrent() const;
	void ResetIterator();
	void OnSerialize( IBinSaver &f );
public:
	CFreeArtilleryHolder() : timeFromReset( 0 ) { itCurrent = artilleries.end(); }
	// add artillery to holder
	void AddArtillery( CArtillery *pArtillery );
	// remove artillery from holder
	void RemoveArtillery( CArtillery *pArtillery );
	// switch to next artillery or to null (if no artillry in holder or too early)
	void Segment();

	// get artillry for searching
	CArtillery *Get() const;
	// set 'catching' flag for current artillery
	void TryCatch( CFormation *pFormation );
};

// manipulate with storages and resupply trucks
class CGeneralIntendant : public CCommander
{
	OBJECT_BASIC_METHODS(CGeneralIntendant);
public:
	struct SVectorHash
	{
		int operator()( const SVector & v ) const { return (v.x<<16) & v.y; }
	};
private:
	
	typedef std::unordered_map< SVector, CPtr<CResupplyCellInfo>, SVectorHash > ResupplyCells;
	typedef std::unordered_map< /*Unique ID*/ int, CPtr<CEnemyRememberer> > CFreeArtillery;
	typedef std::pair<CVec2, WORD> CPosition;

	ZDATA_(CCommander)
	CPtr<CCommander> pGeneral;
	CArray2D<CPtr<CResupplyCellInfo> > cells;

	ResupplyCells cellsWithRequests;

	// artillery without crew.
	ZSKIP //CFreeArtillery freeArtillery;

	// storages (tasks to defend storages)
	CommonUnits resupplyTrucks;
	std::vector<CPosition> vPositions;
	int nCurPosition;
	bool bInitedByParcel;

	int nParty;
	CFreeArtilleryHolder freeArtilleryHolder;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CCommander*)this); f.Add(2,&pGeneral); f.Add(3,&cells); f.Add(4,&cellsWithRequests); f.Add(6,&resupplyTrucks); f.Add(7,&vPositions); f.Add(8,&nCurPosition); f.Add(9,&bInitedByParcel); f.Add(10,&nParty); f.Add(11,&freeArtilleryHolder); return 0; }

	const bool IsUnitRegistered( CCommonUnit * pUnit ) const;
	CGeneralIntendant::ResupplyCells::iterator GetCell( const CVec2 &vPos );
	void DeleteForgottenArtillery();
	void SearchCrewForArtillery();
public:
	CGeneralIntendant() : pGeneral( 0 ) {  }
	CGeneralIntendant( const int nPlayer, CCommander * pGeneral );

	void Init();
	void AddReiforcePositions( const struct NDb::SAIGeneralParcel &_patchInfo );
	void AddReiforcePosition( const CVec2 & vPos, const WORD wDirection );
	
	virtual void Give( CCommonUnit *pWorker );
	virtual float GetMeanSeverity() const { return 0; }
	virtual void EnumWorkers( const EForceType eType, IWorkerEnumerator *pEnumerator );
	virtual void Segment() ;
	
	virtual int RequestForSupport( const CVec2 &vSupportCenter, enum EForceType eType );
	virtual void CancelRequest( int nRequestID, enum EForceType eType );

	void UnitDead( class CCommonUnit * pUnit );
	void UnitChangedPosition( class CCommonUnit * pUnit, const CVec2 &vNewPos );
	void UnitAskedForResupply( class CCommonUnit * pUnit, const enum EResupplyType eType, const bool bSet );
	void SetArtilleryVisible( const CAIUnit *pArtillery, const bool bVisible );

	void MarkCellsDangerous( const SVector &vCell );		
};


