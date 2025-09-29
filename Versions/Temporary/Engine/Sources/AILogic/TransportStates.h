#ifndef __TRANSPORT_STATES__
#define __TRANSPORT_STATES__

#pragma ONCE
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "UnitStates.h"
#include "StatusUpdatesHelper.h"
#include "StatesFactory.h"
#include "..\Stats_B2_M1\RPGStats.h"
#include "..\Stats_B2_M1\StatusUpdates.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFormation;
class CAIUnit;
class CBuilding;
class CArtillery;
interface IStaticPath;
class CMilitaryCar;
class CAITransportUnit;
namespace NDb
{
	enum EMineType;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CTransportStatesFactory );
	
	static CPtr<CTransportStatesFactory> pFactory;
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportWaitPassengerState : public IUnitState
{
	OBJECT_BASIC_METHODS( CTransportWaitPassengerState );
	ZDATA
	CPtr<CMilitaryCar> pTransport;
	list< CPtr<CFormation> > formationsToWait;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTransport); f.Add(3,&formationsToWait); return 0; }
public:
	static IUnitState* Instance( class CMilitaryCar *pTransport, class CFormation *pFormation );

	CTransportWaitPassengerState() : pTransport( 0 ) { }
	CTransportWaitPassengerState( class CMilitaryCar *pTransport, class CFormation *pFormation );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_WAIT_FOR_PASSENGER; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

	void AddFormationToWait( class CFormation *pFormation );
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//состояние выгрузки солдат из транспорта (& mech units )
//
class CTransportLandState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CTransportLandState );

	enum ELandStates 
	{ 
		ELS_STARTING, 
		ELS_MOVING, 
		ELS_LANDING, 
		ELS_LANDING_MECH 
	};

	ZDATA_( CStatusUpdatesHelper )
	ELandStates state;

	CPtr<CMilitaryCar> pTransport;

	CVec2 vLandPoint;
	CPtr<CFormation> pUnload;					// if unloading single formation
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&state); f.Add(3,&pTransport); f.Add(4,&vLandPoint); f.Add(5,&pUnload); return 0; }
private:
	//
	void LandPassenger( class CSoldier *pLandUnit, const CVec3 &vPos );
	const SVector GetLandingPoint();
	bool FindAllowedDropPoint( CFormation *pUnit, CVec2 *vDropPoint );
public:
	CTransportLandState() : pTransport( 0 ) { }
	CTransportLandState( class CMilitaryCar *pTransport, const enum EActionLeaveParam param, const CVec2 &vLandPoint, CFormation *pUnload );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_LAND; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportLoadRuState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CTransportLoadRuState );
	enum ETransportLoadRuState
	{
		ETLRS_SEARCH_FOR_STORAGE,
		ETLRS_APPROACHING_STORAGE,
		ETLRS_START_LOADING_RU,
		ETLRS_LOADING_RU,
		ETLRS_WAIT_FOR_LOADERS,
		ETLRS_SEND_LOADERS,
		ETLRS_SUBSTATE_FINISHED,
	};

	ZDATA_( CStatusUpdatesHelper )
	ETransportLoadRuState eState;

	CPtr<CBuilding> pStorage;
	CPtr<CFormation> pLoaderSquad; // толпа грузчиков
	CPtr<CAITransportUnit> pTransport;
	int nEntrance;
	bool bSubState;												// является ли этот стейт сабстейтом
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&eState); f.Add(3,&pStorage); f.Add(4,&pLoaderSquad); f.Add(5,&pTransport); f.Add(6,&nEntrance); f.Add(7,&bSubState); return 0; }
private:
	void CreateSquad();

	CBuilding * FindNearestSource();
	void Interrupt();
public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, const bool bSubState = false, CBuilding *_pPreferredStorage = 0 );

	CTransportLoadRuState () : pTransport( 0 ) { }
	CTransportLoadRuState ( class CAITransportUnit *pTransport, const bool bSubState, CBuilding *_pPreferredStorage );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }
	
	bool IsSubStateFinished() const { return eState == ETLRS_SUBSTATE_FINISHED; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportServeState : public IUnitState, public CStatusUpdatesHelper
{
	enum ETransportServeState
	{
		ETRS_WAIT_FOR_UNLOCK,
		ETRS_INIT,

		ETRS_START_APPROACH,
		ETRS_APPROACHING,
		ETRS_CREATE_SQUAD,
		ETRS_FINDING_UNIT_TO_SERVE,
		ETRS_LOADERS_INROUTE,
		
		ETRS_GOING_TO_STORAGE,
		
		ETRS_WAIT_FOR_LOADERS,
		ETRS_WAIT_FOR_UNIT_TO_SERVE,
	};
	void CreateSquad();

	ZDATA_( CStatusUpdatesHelper )
	ETransportServeState eState;
	CVec2 vServePoint; //senter of serving circle
	CPtr<CAIUnit> pResupplyUnit;	//юнит, который перезаряжают
	NTimer::STime timeLastUpdate ;//время последнего апдейта поведения.

	CPtr<IStaticPath> pStaticPath ;
	bool bWaitForPath;
protected:
	bool bUpdatedActionsBegin;
	CPtr<CFormation> pLoaderSquad; // толпа грузчиков
	CPtr<CAIUnit> pPreferredUnit;			// unit that is served first
	CPtr<CAITransportUnit> pTransport;
	bool bSendFinishFeedback;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&eState); f.Add(3,&vServePoint); f.Add(4,&pResupplyUnit); f.Add(5,&timeLastUpdate); f.Add(6,&pStaticPath); f.Add(7,&bWaitForPath); f.Add(8,&bUpdatedActionsBegin); f.Add(9,&pLoaderSquad); f.Add(10,&pPreferredUnit); f.Add(11,&pTransport); f.Add(12,&bSendFinishFeedback); return 0; }
	virtual bool FindUnitToServe( bool *pIsNotEnoughRU ) = 0;

	virtual void SendLoaders() = 0;
	
	virtual void UpdateActionBegin() = 0;

public:
	CTransportServeState() : pTransport( 0 ), bUpdatedActionsBegin( false ) { }
	CTransportServeState( class CAITransportUnit *pTransport, const CVec2 &_vServePoint, CAIUnit *_pPreferredUnit );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return vServePoint; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportResupplyState: public CTransportServeState
{
	OBJECT_BASIC_METHODS( CTransportResupplyState );
	ZDATA_(CTransportServeState)
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CTransportServeState*)this); return 0; }
protected:
	bool FindUnitToServe( bool *pIsNotEnoughRU );
	virtual void SendLoaders();
	virtual void UpdateActionBegin();

public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, const CVec2 &_vServePoint, CAIUnit *_pPreferredUnit );

	CTransportResupplyState() { }
	CTransportResupplyState( class CAITransportUnit *pTransport, const CVec2 &_vServePoint, CAIUnit *_pPreferredUnit );

	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }

	virtual EUnitStateNames GetName() { return EUSN_RESUPPLY_UNIT; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportRepairState: public CTransportServeState
{
	OBJECT_BASIC_METHODS( CTransportRepairState );
	ZDATA_(CTransportServeState)
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CTransportServeState*)this); return 0; }
protected:
	virtual bool FindUnitToServe( bool *pIsNotEnoughRU );
	void SendLoaders();
	void UpdateActionBegin();
public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, const CVec2 &_vServePoint, CAIUnit *_pPreferredUnit );

	CTransportRepairState() { }
	CTransportRepairState( class CAITransportUnit *pTransport, const CVec2 &_vServePoint, CAIUnit *_pPreferredUnit );

	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }

	virtual EUnitStateNames GetName() { return EUSN_REPAIR_UNIT; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// погрузка пушки
class CTransportHookArtilleryState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CTransportHookArtilleryState );
	
	enum ETransportTakeGunState
	{
		TTGS_ESTIMATING,                    //0
		TTGS_APPROACHING,	                  //1
		TTGS_START_UNINSTALL,	              //2
		TTGS_START_APPROACH_BY_MOVE_BACK,		//3
		TTGS_APPROACH_BY_MOVE_BACK,				  //4
		TTGS_START_APPROACH_BY_CHEAT_PATH,	//5
		TTGS_APPROACH_BY_CHEAT_PATH,	      //6
		TTGS_WAIT_FOR_UNINSTALL,	          //7
		TTGS_WAIT_FOR_TURN,	                //8
		TTGS_WAIT_FOR_CREW,	                //9
		TTGS_SEND_CREW_TO_TRANSPORT,	      //10
		TTGS_WAIT_FOR_LEAVE_TANKPIT,	      //11
	};

	ZDATA_( CStatusUpdatesHelper )
	CPtr<CAITransportUnit> pTransport;
	ETransportTakeGunState eState;

	CPtr<CArtillery> pArtillery;
	CVec2 vArtilleryPoint;
	
	NTimer::STime timeLast;

	SAIAngle wDesiredTransportDir; // куда бдет направлен транспорт при погрузке
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&pTransport); f.Add(3,&eState); f.Add(4,&pArtillery); f.Add(5,&vArtilleryPoint); f.Add(6,&timeLast); f.Add(7,&wDesiredTransportDir); return 0; }
private:
	void OnSerialize( IBinSaver &saver );

	bool bInterrupted;
	bool CanInterrupt();
	void InterruptBecauseOfPath();
public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, class CArtillery *pArtillery, const CVec2 &vHookPoint );

	CTransportHookArtilleryState() { }
	CTransportHookArtilleryState( class CAITransportUnit *pTransport, class CArtillery *pArtillery, const CVec2 &vHookPoint );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
	
	virtual EUnitStateNames GetName() { return EUSN_HOOK_ARTILLERY; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// отцепление ариллерии
class CTransportUnhookArtilleryState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CTransportUnhookArtilleryState );

	enum ETransportUnhookGunState
	{
		TUAS_START_APPROACH,
		TUAS_APPROACHING,
		TUAS_ESTIMATING,
		TUAS_ADVANCE_A_LITTLE,
		TUAS_MOVE_A_LITTLE,
		TUAS_MOVE_ARTILLERY_TO_THIS_POINT,
		TUAS_START_UNHOOK,
	};
	ZDATA_( CStatusUpdatesHelper )
	ETransportUnhookGunState eState;
	CPtr<CAITransportUnit> pTransport;
	CVec2 vDestPoint;
	int nAttempt; // количество попыток поставить артиллерию
	bool bInterrupted;
	bool bNow;														// unhook gun right at the current place
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&eState); f.Add(3,&pTransport); f.Add(4,&vDestPoint); f.Add(5,&nAttempt); f.Add(6,&bInterrupted); f.Add(7,&bNow); return 0; }
private:

	bool CanPlaceUnit( const class CAIUnit * pUnit ) const;
public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, const class CVec2 &vDestPoint, const bool bNow );

	CTransportUnhookArtilleryState () { }
	CTransportUnhookArtilleryState ( class CAITransportUnit *pTransport, const class CVec2 &vDestPoint, const bool bNow );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return vDestPoint; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportBuildState : public IUnitState, public CStatusUpdatesHelper
{
	enum ETransportBuildState
	{
		ETBS_ESTIMATE,

		ETBS_WAIT_FOR_ENDPOINT,
		ETBS_END_POINT_READY,
		
		ETBS_APROACHING_BUILDPOINT,
		
		ETBS_CREATE_SQUAD,
		ETBS_WAIT_FINISH_BUILD,
		ETBS_START_APPROACH,

		ETBS_LOADING_RESOURCES,

		ETBS_WAIT_FOR_LOADERS,
	};

	ZDATA_( CStatusUpdatesHelper )
protected:
	CPtr<CAITransportUnit> pUnit;
	ETransportBuildState eState;
	CPtr<CTransportLoadRuState> pLoadRuSubState;
	ZSKIP
protected:
	CVec2 vStartPoint;
	CVec2 vEndPoint;
	CPtr<CFormation> pEngineers;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&pLoadRuSubState); f.Add(6,&vStartPoint); f.Add(7,&vEndPoint); f.Add(8,&pEngineers); return 0; }
protected:
	virtual void SendTransportToBuildPoint() = 0;
	virtual bool HaveToSendEngeneersNow()  = 0;
	virtual void SendEngineers() = 0;
	virtual bool IsEndPointNeeded() const = 0;
	// хватает ли инженерам ресурсов, чтобы строить
	virtual bool IsEnoughResources() const = 0;
	// все ли инженеры построили
	virtual bool IsWorkDone() const = 0;
	virtual bool MustSayNegative() const { return false; }
	virtual void NotifyGoToStorage() { }

	CTransportBuildState() { }
	CTransportBuildState( class CAITransportUnit *pTransport, const class CVec2 & vDestPoint );

public:
	virtual void Segment();

	void SetStartPoint( const CVec2 &_vStartPoint ) { vStartPoint = _vStartPoint; }
	virtual void SetEndPoint( const CVec2& _vEndPoint ) ;
	
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return vStartPoint; }
};
class CLongObjectCreation;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportBuildLongObjectState : public CTransportBuildState
{
protected:
	ZDATA_(CTransportBuildState)
	CObj<CLongObjectCreation> pCreation;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CTransportBuildState*)this); f.Add(2,&pCreation); return 0; }
protected:

	virtual void SendTransportToBuildPoint();
	virtual bool HaveToSendEngeneersNow() ;
	virtual bool IsEnoughResources() const;
	virtual bool IsWorkDone() const;
	virtual bool MustSayNegative() const;
	virtual void PreCreate() = 0;
	void SendEngineers();

	virtual bool IsEndPointNeeded() const { return true; }

	CTransportBuildLongObjectState () {  }
	CTransportBuildLongObjectState ( class CAITransportUnit *pTransport, const class CVec2 & vDestPoint, class CLongObjectCreation *pCreation );
public:

	virtual void SetEndPoint( const CVec2& _vEndPoint );
	
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportBuildFenceState : public CTransportBuildLongObjectState
{
	OBJECT_BASIC_METHODS( CTransportBuildFenceState );
	ZDATA_(CTransportBuildLongObjectState)
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CTransportBuildLongObjectState*)this); return 0; }
public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, const class CVec2 &vStartPoint );

	CTransportBuildFenceState() {  }
	CTransportBuildFenceState ( class CAITransportUnit *pTransport, const class CVec2 &vStartPoint );

	virtual EUnitStateNames GetName() { return EUSN_BUILD_FENCE; }
	void PreCreate();
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportBuildEntrenchmentState : public CTransportBuildLongObjectState
{
	OBJECT_BASIC_METHODS( CTransportBuildEntrenchmentState );
	ZDATA_(CTransportBuildLongObjectState)
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CTransportBuildLongObjectState*)this); return 0; }
public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, const class CVec2 & vStartPoint );
	void PreCreate();

	CTransportBuildEntrenchmentState() {  }
	CTransportBuildEntrenchmentState( class CAITransportUnit *pTransport, const class CVec2 & vStartPoint );

	virtual EUnitStateNames GetName() { return EUSN_BUILD_ENTRENCHMENT; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportClearMineState : public CTransportBuildState
{
	OBJECT_BASIC_METHODS( CTransportClearMineState );

	ZDATA_(CTransportBuildState)
	NTimer::STime timeCheckPeriod, timeNextCheck;
	bool bWorkDone;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CTransportBuildState*)this); f.Add(2,&timeCheckPeriod); f.Add(3,&timeNextCheck); f.Add(4,&bWorkDone); return 0; }
protected:	
	virtual void SendTransportToBuildPoint();
	virtual bool HaveToSendEngeneersNow() ;
	virtual bool IsEnoughResources() const;
	virtual bool IsWorkDone() const;
	void SendEngineers();
	virtual bool IsEndPointNeeded() const ;

public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, const class CVec2 & vStartPoint );

	CTransportClearMineState() {  }
	CTransportClearMineState( class CAITransportUnit *pTransport, const class CVec2 & vDestPoint );

	virtual EUnitStateNames GetName() { return EUSN_CLEAR_MINE; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportPlaceMineState : public CTransportBuildState
{
	OBJECT_BASIC_METHODS( CTransportPlaceMineState );

	ZDATA_(CTransportBuildState)
	bool bWorkDone;
	bool bTransportSent;
	NDb::EMineType eCurrType;
	int nSqI, nSqJ;
	CVec2 vCorner;
	int nSquareSize;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CTransportBuildState*)this); f.Add(2,&bWorkDone); f.Add(3,&bTransportSent); f.Add(4,&eCurrType); f.Add(5,&nSqI); f.Add(6,&nSqJ); f.Add(7,&vCorner); f.Add(8,&nSquareSize); return 0; }
protected:	
	virtual void SendTransportToBuildPoint();
	virtual bool HaveToSendEngeneersNow();
	virtual bool IsEnoughResources() const;
	virtual bool IsWorkDone() const;
	void SendEngineers();

	virtual bool IsEndPointNeeded() const;

public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, const class CVec2 & vStartPoint );

	CTransportPlaceMineState() {  }
	CTransportPlaceMineState( class CAITransportUnit *pTransport, const class CVec2 & vDestPoint );

	virtual EUnitStateNames GetName() { return EUSN_PLACE_MINE; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportPlaceAntitankState : public CTransportBuildState 
{
	OBJECT_BASIC_METHODS( CTransportPlaceAntitankState );

	ZDATA_(CTransportBuildState)
	bool bWorkFinished;
	bool bSent;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CTransportBuildState*)this); f.Add(2,&bWorkFinished); f.Add(3,&bSent); return 0; }
protected:	
	virtual void SendTransportToBuildPoint();
	virtual bool HaveToSendEngeneersNow() ;
	virtual bool IsEnoughResources() const;
	virtual bool IsWorkDone() const;
	void SendEngineers();
	virtual bool IsEndPointNeeded() const ;

public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, const class CVec2 & vStartPoint);

	CTransportPlaceAntitankState() {  }
	CTransportPlaceAntitankState( class CAITransportUnit *pTransport, const class CVec2 & vDestPoint );

	virtual EUnitStateNames GetName() { return EUSN_PLACE_ANTITANK; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFullBridge;
class CBridgeSpan;
class CTransportRepairBridgeState : public CTransportBuildState
{
	OBJECT_BASIC_METHODS( CTransportRepairBridgeState );

	ZDATA_(CTransportBuildState)
	ZSKIP
	CPtr<CBridgeSpan> pBridgeToRepair;
	bool bSentToBuildPoint;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CTransportBuildState*)this); f.Add(3,&pBridgeToRepair); f.Add(4,&bSentToBuildPoint); return 0; }
private:
protected:	
	virtual void SendTransportToBuildPoint();
	virtual bool HaveToSendEngeneersNow() ;
	virtual bool IsEnoughResources() const;
	virtual bool IsWorkDone() const;
	void SendEngineers();
	virtual bool IsEndPointNeeded() const ;
	virtual void NotifyGoToStorage() { bSentToBuildPoint = false; }
public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, class CBridgeSpan *pFullBridge );

	CTransportRepairBridgeState() {  }
	CTransportRepairBridgeState( class CAITransportUnit *pTransport, class CBridgeSpan *pFullBridge );
	
	virtual EUnitStateNames GetName() { return EUSN_REPAIR_BRIDGE; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CBridgeCreation;
class CTransportBuildBridgeState : public CTransportBuildState
{
	OBJECT_BASIC_METHODS( CTransportBuildBridgeState );

	ZDATA_(CTransportBuildState)
	CPtr<CFullBridge> pFullBridge;
	CPtr<CBridgeCreation> pCreation;
	bool bTransportSent;									// transport saw sent to build point
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CTransportBuildState*)this); f.Add(2,&pFullBridge); f.Add(3,&pCreation); f.Add(4,&bTransportSent); return 0; }
private:
protected:
	virtual void SendTransportToBuildPoint();
	virtual bool HaveToSendEngeneersNow() ;
	virtual bool IsEnoughResources() const;
	virtual bool IsWorkDone() const;
	void SendEngineers();
	virtual bool IsEndPointNeeded() const ;
	virtual void NotifyGoToStorage() { bTransportSent = false; }
public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, class CFullBridge *pFullBridge );

	CTransportBuildBridgeState() {  }
	CTransportBuildBridgeState( class CAITransportUnit *pTransport, class CFullBridge *pFullBridge );

	virtual EUnitStateNames GetName() { return EUSN_BUILD_BRIDGE; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CBuilding;
class CTransportRepairBuildingState : public CTransportBuildState
{
	OBJECT_BASIC_METHODS( CTransportRepairBuildingState );

	ZDATA_(CTransportBuildState)
	CPtr<CBuilding> pBuilding;
	bool bSentToBuildPoint;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CTransportBuildState*)this); f.Add(2,&pBuilding); f.Add(3,&bSentToBuildPoint); return 0; }
private:

protected:
	virtual void SendTransportToBuildPoint();
	virtual bool HaveToSendEngeneersNow() ;
	virtual bool IsEnoughResources() const;
	virtual bool IsWorkDone() const;
	void SendEngineers();
	virtual bool IsEndPointNeeded() const ;
	virtual void NotifyGoToStorage() { bSentToBuildPoint = false; }
public:
	static IUnitState* Instance( class CAITransportUnit *pTransport, class CBuilding *pBuilding );

	CTransportRepairBuildingState() {  }
	CTransportRepairBuildingState( class CAITransportUnit *pTransport, class CBuilding *pBuilding );

	virtual EUnitStateNames GetName() { return EUSN_REPAIR_BUILDING; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CMoveToPointNotPresize : public IUnitState
{
	OBJECT_BASIC_METHODS( CMoveToPointNotPresize );
	
	ZDATA
	CPtr<CAIUnit> pTransport;
	CVec2 vPurposePoint;
	float fRadius;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTransport); f.Add(3,&vPurposePoint); f.Add(4,&fRadius); return 0; }
private:

	void SendToPurposePoint();
public:
	static IUnitState* Instance( class CAIUnit *pTransport, const CVec2 &vGeneralCell, const float fRadius );

	CMoveToPointNotPresize() {  }
	CMoveToPointNotPresize( class CAIUnit *pTransport, const CVec2 &vGeneralCell, const float fRadius );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return vPurposePoint; }
	virtual EUnitStateNames GetName() { return EUSN_MOVE_TO_RESUPPLY_CELL; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportWaitForUnload : public IUnitState
{
	OBJECT_BASIC_METHODS( CTransportWaitForUnload );

	ZDATA
	CPtr<CMilitaryCar> pTransport;
	CVec2 vPurposePoint;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTransport); f.Add(3,&vPurposePoint); return 0; }

public:
	static IUnitState* Instance( class CAIUnit *_pTransport, const CVec2 &vTarget );

	CTransportWaitForUnload() {  }
	CTransportWaitForUnload( class CMilitaryCar *_pTransport, const CVec2 &vTarget );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return vPurposePoint; }
	virtual EUnitStateNames GetName() { return EUSN_LAND; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __TRANSPORT_STATES__
