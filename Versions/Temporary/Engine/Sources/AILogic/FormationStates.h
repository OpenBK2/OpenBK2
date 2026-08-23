#pragma once

#include "StatesFactory.h"
#include "CommonStates.h"
#include "StaticObjects.h"
#include "StatusUpdatesHelper.h"
#include "Stats_B2_M1/ActionNotify.h"

#include <cstdint>

class CBuilding;
class CEntrenchment;
class CAIUnit;
class CStaticObject;
class CMilitaryCar;
class CAITransportUnit;
class CTank;
class CCommonStaticObject;
class CArtillery;
class CSoldier;
class CFormation;
class CEntrenchmentPart;
enum EActionNotify : unsigned int;
namespace NDb
{
	enum EUnitSpecialAbility : int;
	enum EMineType : int;
}

struct IEngineerFormationState : public IUnitState
{
	virtual void SetHomeTransport( class CAITransportUnit *pTransport ) = 0;
};

class CFormationStatesFactory : public IStatesFactory
{
	OBJECT_BASIC_METHODS( CFormationStatesFactory );
	
	static CPtr<CFormationStatesFactory> pFactory;
public:
	static CFormationStatesFactory* Instance();
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

class CFormationRestState : public CCommonRestState
{
 	OBJECT_BASIC_METHODS( CFormationRestState );
	ZDATA_(CCommonRestState)
	CPtr<CFormation> pFormation;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CCommonRestState*)this); f.Add(2,&pFormation); return 0; }
protected:
	virtual class CCommonUnit* GetUnit() const;
	
public:
	static IUnitState* Instance( class CFormation *pFormation, const CVec2 &guardPoint, const uint16_t wDir, const float fTimeToWait );
	
	CFormationRestState() : pFormation( 0 ) { }
	CFormationRestState( class CFormation *pFormation, const CVec2 &guardPoint, const uint16_t wDir, const float fTimeToWait );

	void Segment();
	
	virtual EUnitStateNames GetName() { return EUSN_REST; }
	virtual bool IsAttackingState() const { return false; }

	ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
};

class CFormationMoveToState : public IUnitState
{
  OBJECT_BASIC_METHODS( CFormationMoveToState );
	enum { TIME_OF_WAITING = 200 };
	enum EMoveToStates { EMTS_FORMATION_MOVING, EMTS_UNITS_MOVING_TO_FORMATION_POINTS };

	ZDATA
	EMoveToStates eMoveToState;

	CPtr<CFormation> pFormation;

	NTimer::STime startTime;
	bool bWaiting;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eMoveToState); f.Add(3,&pFormation); f.Add(4,&startTime); f.Add(5,&bWaiting); return 0; }
	//
	void FormationMovingState();
	void UnitsMovingToFormationPoints();
public:
	static IUnitState* Instance( class CFormation *pFormation, const CVec2 &point );

	CFormationMoveToState() : pFormation( 0 ) { }
	CFormationMoveToState( class CFormation *pFormation, const CVec2 &point );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }

	virtual EUnitStateNames GetName() { return EUSN_MOVE; }
};

class CFormationParaDropState : public IUnitState
{
  OBJECT_BASIC_METHODS( CFormationParaDropState );
	enum EParadropState
	{
		EPS_WAIT_FOR_PARADROP_BEGIN,
		EPS_WAIT_FOR_PARADROP_END,
	};
	ZDATA
	EParadropState eState;
	CPtr<CFormation> pFormation;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pFormation); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation );

	CFormationParaDropState() : pFormation( 0 ) { }
	CFormationParaDropState( class CFormation *pFormation );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
	virtual EUnitStateNames GetName() { return EUSN_PARTROOP; }
};

class CFormationEnterBuildingState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CFormationEnterBuildingState );

	enum EEnterBuildingStates { EES_START, EES_RUN_UP, EES_FINISHED, EES_WAIT_FOR_UNLOCK, EES_WAITINIG_TO_ENTER };
	ZDATA_( CStatusUpdatesHelper )
	EEnterBuildingStates state;

	CPtr<CFormation> pFormation;
	CPtr<CBuilding> pBuilding;
	int nEntrance;
	NTimer::STime timeSent;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&state); f.Add(3,&pFormation); f.Add(4,&pBuilding); f.Add(5,&nEntrance); f.Add(6,&timeSent); return 0; }
	//
	bool SetPathForRunUp();
	void SendUnitsToBuilding();
	bool IsNotEnoughSpace();
public:
	static IUnitState* Instance( class CFormation *pFormation, class CBuilding *pBuilding );

	CFormationEnterBuildingState() : pFormation( 0 ) { }
	CFormationEnterBuildingState( class CFormation *pFormation, class CBuilding *pBuilding );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

	virtual EUnitStateNames GetName() { return EUSN_ENTER; }
};

class CFormationEnterEntrenchmentState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CFormationEnterEntrenchmentState );
	
	enum EEnterState { EES_START, EES_RUN, EES_WAIT_TO_ENTER, EES_FINISHED };
	ZDATA_( CStatusUpdatesHelper )
	EEnterState state;

	CPtr<CFormation> pFormation;
	CPtr<CEntrenchment> pEntrenchment;
	NTimer::STime timeToWait;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&state); f.Add(3,&pFormation); f.Add(4,&pEntrenchment); f.Add(5,&timeToWait); return 0; }
	//
	bool IsAnyPartCloseToEntrenchment() const;
	bool SetPathForRunIn();
	void EnterToEntrenchment();
public:
	static IUnitState* Instance( class CFormation *pFormation, class CEntrenchment *pEntrenchment );

	CFormationEnterEntrenchmentState() : pFormation( 0 ) { }
	CFormationEnterEntrenchmentState( class CFormation *pFormation, class CEntrenchment *pEntrenchment );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

	virtual EUnitStateNames GetName() { return EUSN_ENTER_ENTRENCHMENT; }
};

class CFormationIdleBuildingState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationIdleBuildingState );
	ZDATA
	CPtr<CFormation> pFormation;
	CPtr<CBuilding> pBuilding;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&pBuilding); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, class CBuilding *pBuilding );

	CFormationIdleBuildingState() : pFormation( 0 ) { }
	CFormationIdleBuildingState( class CFormation *pFormation, class CBuilding *pBuilding );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_REST_IN_BUILDING; }
	virtual bool IsRestState() const { return true; }

	class CBuilding* GetBuilding() const { return pBuilding; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CFormationIdleEntrenchmentState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationIdleEntrenchmentState );
	ZDATA
	CPtr<CFormation> pFormation;
	CPtr<CEntrenchment> pEntrenchment;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&pEntrenchment); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, class CEntrenchment *pEntrenchment );

	CFormationIdleEntrenchmentState() : pFormation( 0 ) { }
	CFormationIdleEntrenchmentState( class CFormation *pFormation, class CEntrenchment *pEntrenchment );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_REST_ENTRENCHMENT; }
	virtual bool IsRestState() const { return true; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

	class CEntrenchment* GetEntrenchment() const { return pEntrenchment; }
};

class CFormationLeaveBuildingState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CFormationLeaveBuildingState );
	ZDATA_( CStatusUpdatesHelper )
	CPtr<CFormation> pFormation;
	CPtr<CBuilding> pBuilding;
	CVec2 point;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&pFormation); f.Add(3,&pBuilding); f.Add(4,&point); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, class CBuilding *pBuilding, const enum EActionLeaveParam param, const CVec2 &point );

	CFormationLeaveBuildingState() : pFormation( 0 ) { }
	CFormationLeaveBuildingState( class CFormation *pFormation, class CBuilding *pBuilding, const enum EActionLeaveParam param, const CVec2 &point );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return point; }

	virtual EUnitStateNames GetName() { return EUSN_GO_OUT; }
};

class CFormationLeaveEntrenchmentState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CFormationLeaveEntrenchmentState );

	ZDATA_( CStatusUpdatesHelper )
	CPtr<CFormation> pFormation;
	CPtr<CEntrenchment> pEntrenchment;
	CVec2 point;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&pFormation); f.Add(3,&pEntrenchment); f.Add(4,&point); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, class CEntrenchment *pEntrenchment, const enum EActionLeaveParam param, const CVec2 &point );

	CFormationLeaveEntrenchmentState() : pFormation( 0 ) { }
	CFormationLeaveEntrenchmentState( class CFormation *pFormation, class CEntrenchment *pEntrenchment, const enum EActionLeaveParam param, const CVec2 &point );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return point; }

	virtual EUnitStateNames GetName() { return EUSN_GO_OUT_ENTRENCHMENT; }
};

class CFormationPlaceMine : public IEngineerFormationState
{
	OBJECT_BASIC_METHODS( CFormationPlaceMine );

	enum EPlaceMineStates 
	{ 
		EPM_WAIT_FOR_HOMETRANSPORT,
		EPM_START, 
		EPM_MOVE, 
		EPM_WAITING 
	};
	ZDATA
	EPlaceMineStates eState;
	CPtr<CAITransportUnit> pHomeTransport;

	CPtr<CFormation> pFormation;

	CVec2 point;
	int eType;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pHomeTransport); f.Add(4,&pFormation); f.Add(5,&point); f.Add(6,&eType); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, const CVec2 &point, const EMineType nType );

	CFormationPlaceMine() : pFormation( 0 ) { }
	CFormationPlaceMine( class CFormation *pFormation, const CVec2 &point, const EMineType nType );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return point; }

	virtual void SetHomeTransport( class CAITransportUnit *pTransport );
};

class CFormationClearMine : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationClearMine );

	enum EClearMineStates { EPM_START, EPM_MOVE, EPM_WAIT };
	ZDATA
	EClearMineStates eState;
	CPtr<CFormation> pFormation;
	CVec2 point;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pFormation); f.Add(4,&point); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, const CVec2 &point );

	CFormationClearMine() : pFormation( 0 ) { }
	CFormationClearMine( class CFormation *pFormation, const CVec2 &point );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual EUnitStateNames GetName() { return EUSN_CLEAR_MINE; }

	virtual const CVec2 GetPurposePoint() const { return point; }
};

class CFormationAttackUnitState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CFormationAttackUnitState );
	
	enum EAttackUnitStates { EPM_MOVING, EPM_WAITING };
	ZDATA
	EAttackUnitStates eState;

	CPtr<CFormation> pFormation;
	CPtr<CAIUnit> pEnemy;
	bool bSwarmAttack;
	int nEnemyParty;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pFormation); f.Add(4,&pEnemy); f.Add(5,&bSwarmAttack); f.Add(6,&nEnemyParty); return 0; }

	//
	void SetToMovingState();
	void SetToWaitingState();
public:
	static IUnitState* Instance( class CFormation *pFormation, class CAIUnit *pEnemy, const bool bSwarmAttack );

	CFormationAttackUnitState() { }
	CFormationAttackUnitState( class CFormation *pFormation, class CAIUnit *pEnemy, const bool bSwarmAttack );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	virtual bool IsAttacksUnit() const { return true; }
	virtual class CAIUnit* GetTargetUnit() const { return 0; }
	
	virtual EUnitStateNames GetName();
};

class CFormationAttackCommonStatObjState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CFormationAttackCommonStatObjState );
	
	enum EAttackUnitStates { EPM_START, EPM_MOVING, EPM_WAITING };
	ZDATA
	EAttackUnitStates eState;

	CPtr<CFormation> pFormation;
	CPtr<CStaticObject> pObj;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pFormation); f.Add(4,&pObj); return 0; }

	//
	void SetToWaitingState();
public:
	static IUnitState* Instance( class CFormation *pFormation, class CStaticObject *pObj );

	CFormationAttackCommonStatObjState() { }
	CFormationAttackCommonStatObjState( class CFormation *pFormation, class CStaticObject *pObj );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	virtual bool IsAttacksUnit() const { return false; }
	virtual class CAIUnit* GetTargetUnit() const { return 0; }

	virtual EUnitStateNames GetName() { return EUSN_ATTACK_STAT_OBJECT; }
};

class CFormationRotateState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationRotateState );
	
	ZDATA
	CPtr<CFormation> pFormation;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, const uint16_t wDir );

	CFormationRotateState() { }
	CFormationRotateState( class CFormation *pFormation, const uint16_t wDir );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CFormationEnterTransportState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CFormationEnterTransportState );
	
	enum { CHECK_PERIOD = 500 };
	enum EEnterTransportStates { EETS_START, EETS_MOVING, EETS_WAIT_FOR_TURRETS_RETURN, EETS_WAITING, EETS_FINISHED, EETS_WAIT_TO_UNLOCK_TRANSPORT };
	ZDATA_( CStatusUpdatesHelper )
	EEnterTransportStates eState;

	CPtr<CFormation> pFormation;
	CPtr<CMilitaryCar> pTransport;
	NTimer::STime lastCheck;
	CVec2 lastTransportPos;
	SAIAngle lastTransportDir;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&eState); f.Add(3,&pFormation); f.Add(4,&pTransport); f.Add(5,&lastCheck); f.Add(6,&lastTransportPos); f.Add(7,&lastTransportDir); return 0; }
	//
	bool SetPathToRunUp();
	void SendUnitsToTransport();
	bool IsAllUnitsInside();
	void SetTransportToWaitState();
	// все башни транпорта повёрнуты в default положение
	bool IsAllTransportTurretsReturned() const;
public:
  static IUnitState* Instance( class CFormation *pFormation, class CMilitaryCar *pTransport );
	
	CFormationEnterTransportState() { }
	CFormationEnterTransportState( class CFormation *pFormation, class CMilitaryCar *pTransport );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_ENTER_TRANSPORT; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CFormationIdleTransportState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationIdleTransportState );
	ZDATA
	CPtr<CFormation> pFormation;
	CPtr<CMilitaryCar> pTransport;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&pTransport); return 0; }

public:
	static IUnitState* Instance( class CFormation *pFormation, class CMilitaryCar *pTransport );

	CFormationIdleTransportState() : pFormation( 0 ) { }
	CFormationIdleTransportState( class CFormation *pFormation, class CMilitaryCar *pTransport );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_REST_ON_BOARD; }
	virtual bool IsRestState() const { return true; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CFormationEnterTransportNowState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationEnterTransportNowState );
	ZDATA
	CPtr<CFormation> pFormation;
	CPtr<CMilitaryCar> pTransport;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&pTransport); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, class CMilitaryCar *pTransport );

	CFormationEnterTransportNowState() : pFormation( 0 ) { }
	CFormationEnterTransportNowState( class CFormation *pFormation, class CMilitaryCar *pTransport );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CFormationEnterTransportByCheatPathState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationEnterTransportByCheatPathState );
	ZDATA
	CPtr<CFormation> pFormation;
	CPtr<CMilitaryCar> pTransport;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&pTransport); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, class CMilitaryCar *pTransport );

	CFormationEnterTransportByCheatPathState() : pFormation( 0 ) { }
	CFormationEnterTransportByCheatPathState( class CFormation *pFormation, class CMilitaryCar *pTransport );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

// общий код для Repear, Resupply
class CFormationServeUnitState: public IEngineerFormationState
{
public:
	//
	class CFindUnitPredicate
	{
	public:
		virtual bool HasUnit() = 0;
		//returns true if we have to finish the search
		virtual bool SetUnit( class CAIUnit *pUnit, const float fMissedHP, const float fDist  ) = 0;
		virtual void SetNotEnoughRu() = 0;
		static float CalcWeight( const float fMissedHP, const float fDist )
		{ return fMissedHP * SConsts::HP_BALANCE_COEFF + 1000 / fDist;}
	};
	//
	class CFindFirstUnitPredicate : public CFindUnitPredicate
	{
		CPtr<CAIUnit> pUnit;
		bool bNotEnoughtRu;
	public:
		CFindFirstUnitPredicate() : bNotEnoughtRu( false ) { }
		virtual bool HasUnit(){ return pUnit; }
		virtual bool SetUnit( class CAIUnit *_pUnit, const float fMissedHP, const float fDist )
			{ pUnit = _pUnit; return true; }
		virtual void SetNotEnoughRu() { bNotEnoughtRu = true; }
		bool IsNotEnoughRu() const { return bNotEnoughtRu; }
	};
	//
	class CFindBestUnitPredicate : public CFindUnitPredicate
	{
		CPtr<CAIUnit> pUnit;
		bool bNotEnoughtRu;
		float fCurWeight;
	public:
		CFindBestUnitPredicate() : bNotEnoughtRu( false ), fCurWeight( 0.0f ) { }
		virtual bool HasUnit(){ return pUnit; }
		virtual bool SetUnit( class CAIUnit *_pUnit, const float fMissedHP, const float fDist )
		{ 
			const float fWeight = CalcWeight( fMissedHP, fDist) ;
			if ( fCurWeight < fWeight )
			{
				pUnit = _pUnit; 
				fCurWeight = fWeight;
			}
			return false; 
		}
		virtual void SetNotEnoughRu() { bNotEnoughtRu = true; }
		bool IsNotEnoughRu() const { return bNotEnoughtRu; }
		CAIUnit * GetUnit() { return pUnit; }
	};
	//
protected:
	enum EFormationServiceUnitState
	{
		EFRUS_WAIT_FOR_HOME_TRANSPORT,
		EFRUS_FIND_UNIT_TO_SERVE,
		EFRUS_START_APPROACH,
		EFRUS_APPROACHING,
		EFRUS_APPROACHING2,
		EFRUS_START_SERVICE,
		EFRUS_SERVICING,
		EFRUS_WAIT_FOR_UNIT_TO_SERVE,
	};
	ZDATA
	EFormationServiceUnitState eState;
	CPtr<CAITransportUnit> pHomeTransport; //транспорт у которого берутся ресурсы на починку
	float fWorkAccumulator;								//накопление работы в сегментах
	float fWorkLeft;											// столько ресурсов взяли с собой солдаты
	CPtr<CAIUnit> pPreferredUnit;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&pHomeTransport); f.Add(4,&fWorkAccumulator); f.Add(5,&fWorkLeft); f.Add(6,&pPreferredUnit); return 0; }
public:
	
	CFormationServeUnitState( CAIUnit *_pPreferredUnit ) 
		: eState( EFRUS_WAIT_FOR_HOME_TRANSPORT ), 
		 fWorkLeft( 0 ), 
		 fWorkAccumulator( 0 ),
		 pPreferredUnit( _pPreferredUnit ){  }
	
	virtual void SetHomeTransport( class CAITransportUnit *pTransport );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }
};

//доходит до юнита и запускает каждому члену отряда данную команду
class CFormationRepairUnitState : public CFormationServeUnitState
{
	OBJECT_BASIC_METHODS( CFormationRepairUnitState );
	
	ZDATA_(CFormationServeUnitState)
	CPtr<CFormation> pUnit;
	CPtr<CAIUnit> pUnitInQuiestion;			//юнит, который нужно обслужить
	CPtr<CTank> pTank;
	CVec2 vPointInQuestion;							//где стоит юнит

	NTimer::STime lastRepearTime;
	float fRepCost;
	bool bNearTruck;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CFormationServeUnitState*)this); f.Add(2,&pUnit); f.Add(3,&pUnitInQuiestion); f.Add(4,&pTank); f.Add(5,&vPointInQuestion); f.Add(6,&lastRepearTime); f.Add(7,&fRepCost); f.Add(8,&bNearTruck); return 0; }

	void Interrupt();
	
	static bool CheckUnit( CAIUnit *pU, CFormationServeUnitState::CFindUnitPredicate * pPred, const float fResurs, const CVec2 &vCenter );
public:
	// первое попавшееся наше хранилище для починки
	class CFindFirstStorageToRepearPredicate : public CStaticObjects::IEnumStoragesPredicate
	{
		bool bHasStor;
		bool bNotEnoughRu;
		const float fMaxRu;									// такой запас ресурсов
	public:
		CFindFirstStorageToRepearPredicate( const float fMaxRu ) : fMaxRu( fMaxRu ), bNotEnoughRu( false ), bHasStor( false ) { }
		virtual bool OnlyConnected() const { return false; }
		virtual bool AddStorage( class CBuilding * pStorage, const float fPathLenght );
		bool HasStorage() const { return bHasStor; }
		bool IsNotEnoughRU() const { return bNotEnoughRu; }
	};
	static void FindUnitToServe( const CVec2 &vCenter, 
																int nPlayer, 
																const float fResurs, 
																CCommonUnit * pLoaderSquad, 
																CFormationServeUnitState::CFindUnitPredicate * pPred,
																CAIUnit *_pPreferredUnit );

	static IUnitState* Instance( class CFormation *pUnit, CAIUnit *_pPreferredUnit );
	
	CFormationRepairUnitState() : CFormationServeUnitState( 0 ), pUnit( 0 ) { }
	CFormationRepairUnitState( class CFormation *pUnit, CAIUnit *_pPreferredUnit );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }

	virtual EUnitStateNames GetName() { return EUSN_REPAIR_UNIT; }
};

class CFormationResupplyUnitState : public CFormationServeUnitState
{
	OBJECT_BASIC_METHODS( CFormationResupplyUnitState );
	ZDATA_(CFormationServeUnitState)
	CPtr<CFormation> pUnit;
	CPtr<CAIUnit> pUnitInQuiestion;			//юнит, который нужно обслужить
	CVec2 vPointInQuestion;							//где стоит юнит
	NTimer::STime lastResupplyTime;

	CPtr<CFormation> pSquadInQuestion; // если юнит, который нужно обслужить - формация, то это она
	int iCurUnitInFormation; // в данный момент обслуживаем этого солдата
	bool bSayAck;							// unit being resupplied must say ack when being resupplied
	bool bNearTruck;

	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CFormationServeUnitState*)this); f.Add(2,&pUnit); f.Add(3,&pUnitInQuiestion); f.Add(4,&vPointInQuestion); f.Add(5,&lastResupplyTime); f.Add(6,&pSquadInQuestion); f.Add(7,&iCurUnitInFormation); f.Add(8,&bSayAck); f.Add(9,&bNearTruck); return 0; }
	void Interrupt();
	static bool CheckUnit( CAIUnit *pU, CFormationServeUnitState::CFindUnitPredicate * pPred, const float fResurs, const CVec2 &vCenter );
public:
	static IUnitState* Instance( class CFormation *pUnit, CAIUnit *pPreferredUnit );
	
	CFormationResupplyUnitState() : CFormationServeUnitState( 0 ), pUnit( 0 ) { }
	CFormationResupplyUnitState( class CFormation *pUnit, CAIUnit *pPreferredUnit );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }

	static void FindUnitToServe( const CVec2 &vCenter, int nPlayer, const float fResurs, 
		CCommonUnit * pLoaderSquad, CFormationServeUnitState::CFindUnitPredicate * pPred, CAIUnit *_pPreferredUnit );

	virtual EUnitStateNames GetName() { return EUSN_RESUPPLY_UNIT; }
};

// загрузка грузовичка ресурсами 
// подходят к складу, делают Use, при этом в грузовик поступают ресурсы
class CBuilding;
class CFormationLoadRuState: public CFormationServeUnitState
{
	OBJECT_BASIC_METHODS( CFormationLoadRuState );
	
	ZDATA_(CFormationServeUnitState)
	CPtr<CFormation> pUnit;
	CPtr<CBuilding> pStorage;			//из этого хранилища берем ресурсы
	NTimer::STime lastResupplyTime;
	int nEntrance;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CFormationServeUnitState*)this); f.Add(2,&pUnit); f.Add(3,&pStorage); f.Add(4,&lastResupplyTime); f.Add(5,&nEntrance); return 0; }
	void Interrupt();
public:
	static IUnitState* Instance( class CFormation *pUnit, class CBuilding *pStorage);
	
	CFormationLoadRuState() : CFormationServeUnitState( 0 ),pUnit( 0 ) { }
	CFormationLoadRuState( class CFormation *pUnit, class CBuilding *pStorage);

	virtual void Segment();
	
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }
};

// бегут за транспортом и садятся на ходу.
class CFormationCatchTransportState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationCatchTransportState );
	
	enum ECatchState 
	{
		E_SENDING,
		E_CHECKING,
	};
	ZDATA
	CPtr<CFormation> pUnit;
	CPtr<CAITransportUnit> pTransportToCatch; //cюда будем запрыгивать

	std::list< CPtr<CSoldier> > deleted; // это не сериализовать, заполняется и чистится на 1 сегменте.

	NTimer::STime timeLastUpdate;
	CVec2 vEnterPoint;
	float fResursPerSoldier;							// солдаты, забегая в транспорт могут принести ресурсы
	ECatchState eState;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&pTransportToCatch); f.Add(4,&deleted); f.Add(5,&timeLastUpdate); f.Add(6,&vEnterPoint); f.Add(7,&fResursPerSoldier); f.Add(8,&eState); return 0; }
	void UpdatePath( CSoldier * pSold, const bool bForce = false );
	void Interrupt();
public:
	static IUnitState* Instance( class CFormation *pUnit, class CAITransportUnit *pTransport, float fResursPerSoldier );
	
	CFormationCatchTransportState() : pUnit( 0 ), fResursPerSoldier( 0 ) {  }
	CFormationCatchTransportState( class CFormation *pUnit, class CAITransportUnit *pTransport, float fResursPerSoldier );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

// кладут противотанковый еж
class CFormationPlaceAntitankState : public IEngineerFormationState
{
	OBJECT_BASIC_METHODS( CFormationPlaceAntitankState );

	enum EFormationPlaceAntitankState
	{
		FPAS_WAIT_FOR_HOMETRANSPORT,
		FPAS_ESITMATING,
		FPAS_APPROACHING,
		FPAS_APPROACHING_2,
		FPAS_START_BUILD,
		FPAS_START_BUILD_2,
		FPAS_BUILDING,
	};

	ZDATA
	CPtr<CFormation> pUnit;
	EFormationPlaceAntitankState eState;

	CPtr<CCommonStaticObject> pAntitank;
	CPtr<CAITransportUnit> pHomeTransport;
	CVec2 vDesiredPoint; //here antitank is going to be built
	float fWorkAccumulator;
	NTimer::STime timeBuild;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&pAntitank); f.Add(5,&pHomeTransport); f.Add(6,&vDesiredPoint); f.Add(7,&fWorkAccumulator); f.Add(8,&timeBuild); return 0; }
public:
	static IUnitState* Instance( class CFormation *pUnit, const CVec2 &vDesiredPoint );
	
	CFormationPlaceAntitankState() : pUnit( 0 ) { }
	CFormationPlaceAntitankState( class CFormation *pUnit, const CVec2 &vDesiredPoint );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return vDesiredPoint; }
	virtual void SetHomeTransport( class CAITransportUnit *pTransport );
};

class CLongObjectCreation;

class CFormationBuildLongObjectState : public IEngineerFormationState
{
	OBJECT_BASIC_METHODS( CFormationBuildLongObjectState );
	
	enum EFormationBuildEntrenchState
	{
		ETBS_WAITING_FOR_HOMETRANSPORT,


		FBFS_READY_TO_START,
		FBFS_APPROACHING_STARTPOINT,
		FBFS_BUILD_SEGMENT,

		FBFS_START_APPROACH_SEGMENT,
		FBFS_NEXT_SEGMENT,
		FBFS_CANNOT_BUILD_ANYMORE,
		FBFS_CHECK_FOR_UNITS_PREVENTING,
		FBFS_WAIT_FOR_UNITS,
		FBFS_APPROACH_SEGMENT,
	};

	ZDATA
	CPtr<CFormation> pUnit;
	EFormationBuildEntrenchState eState;

	NTimer::STime lastTime;
	std::list<CPtr<CAIUnit> > unitsPreventing;
	float fWorkLeft;
	CPtr<CAITransportUnit> pHomeTransport;
				
	CPtr<CLongObjectCreation> pCreation;
	float fCompletion;										// степень готовности данного сегмента
	int nCurrentSegment;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&lastTime); f.Add(5,&unitsPreventing); f.Add(6,&fWorkLeft); f.Add(7,&pHomeTransport); f.Add(8,&pCreation); f.Add(9,&fCompletion); f.Add(10,&nCurrentSegment); return 0; }
	void SendUnitsAway( std::list<CPtr<CAIUnit> > *pUnitsPreventing );

public:
	static IUnitState * Instance( class CFormation *pUnit, class CLongObjectCreation *pCreation  );

	CFormationBuildLongObjectState() : pUnit ( 0 ) {  }
	CFormationBuildLongObjectState( class CFormation *pUnit, class CLongObjectCreation *pCreation  );

	virtual void Segment();
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2(-1,-1); }

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual void SetHomeTransport( class CAITransportUnit *pTransport );
	virtual EUnitStateNames GetName() { return EUSN_BUILD_LONGOBJECT; }
	
};

class CEntrenchmentCreation;

class CFormationBuildEntrenchmentState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationBuildEntrenchmentState );

	enum EFormationBuildEntrenchmentState
	{
		EBES_START_APPROACHING,
		EBES_APPROACHING_BUILDPOINT,
		EBES_BUILD,
		EBES_CHECK_FOR_UNITS_PREVENTING,
	};

	ZDATA
	
	CPtr<CFormation> pFormation;
	EFormationBuildEntrenchmentState eState;

	NTimer::STime lastTime;
	float fWorkLeft;

	CObj<CEntrenchmentCreation> pCreation;
	bool bEndPointSelected;
	float fCompletion;										// степень готовности
	det_map<int,CVec2> targetPoints;
	CVec2 vStartPoint;
	CVec2 vEndPoint;
	int nMaxIndex;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&eState); f.Add(4,&lastTime); f.Add(5,&fWorkLeft); f.Add(6,&pCreation); f.Add(7,&bEndPointSelected); f.Add(8,&fCompletion); f.Add(9,&targetPoints); f.Add(10,&vStartPoint); f.Add(11,&vEndPoint); f.Add(12,&nMaxIndex); return 0; }
	void SendUnitsAway( std::list<CPtr<CAIUnit> > *pUnitsPreventing );

public:
	static IUnitState * Instance( CFormation *pUnit, CLongObjectCreation *pCreation, const CVec2 &vStartPoint );

	CFormationBuildEntrenchmentState() : pFormation ( 0 ) {  }
	CFormationBuildEntrenchmentState( CFormation *pUnit, CLongObjectCreation *pCreation, const CVec2 &vStartPoint );

	virtual void Segment();
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2(-1,-1); }

	virtual ETryStateInterruptResult TryInterruptState( CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_BUILD_ENTRENCHMENT; }
	void SetEndPoint( const CVec2 &vPos );
};

// отдать себя в качестве обслуживающей команды
class CFormationCaptureArtilleryState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CFormationCaptureArtilleryState );

	enum EFormationCaptureArtilleryState
	{
		FCAS_ESTIMATING,
		FCAS_APPROACHING,
		FCAS_EXITTTING,
	};
	ZDATA_( CStatusUpdatesHelper )
	EFormationCaptureArtilleryState eState;

	CPtr<CFormation> pUnit;
	CPtr<CArtillery> pArtillery;
	std::vector< CPtr<CAIUnit> > usedSoldiers;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&eState); f.Add(3,&pUnit); f.Add(4,&pArtillery); f.Add(5,&usedSoldiers); return 0; }
public:
	static IUnitState* Instance( class CFormation *_pUnit, CArtillery *pArtillery, const bool bUseFormationPart );
	
	CFormationCaptureArtilleryState() { }
	CFormationCaptureArtilleryState( class CFormation *_pUnit, CArtillery *pArtillery, const bool bUseFormationPart );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }
	
	virtual EUnitStateNames GetName() { return EUSN_GUN_CAPTURE; }
};

// обслуживание пушек артиллеристами. эта команда отдается самой пушкой.
class CFormationGunCrewState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationGunCrewState );

	struct SCrewAnimation
	{
		EActionNotify eAction;
		SAIAngle wDirection;
		//
		SCrewAnimation() : eAction( ACTION_NOTIFY_NONE ), wDirection( 0 ) {  }
		SCrewAnimation( EActionNotify eAction, uint16_t wDirection )
			:eAction( eAction ), wDirection ( wDirection )
		{
		}
	};

	struct SUnit
	{
		ZDATA
		EActionNotify eAction;
		EActionNotify eNewAction;
		bool bForce;
		SAIAngle wDirection;
		NTimer::STime timeNextUpdate;
	public:
		CPtr<CSoldier> pUnit;
		CVec2 vServePoint;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&eAction); f.Add(3,&eNewAction); f.Add(4,&bForce); f.Add(5,&wDirection); f.Add(6,&timeNextUpdate); f.Add(7,&pUnit); f.Add(8,&vServePoint); return 0; }
	public:

		void UpdateAction();
		void SetAction( const struct SCrewAnimation &rNewAnim, bool force = false );
		void ResetAnimation();
		inline bool IsAlive() const;
		SUnit();
		SUnit( class CSoldier *pUnit, const CVec2 &vServePoint, const EActionNotify eAction = ACTION_NOTIFY_IDLE );
	};
	struct SCrewMember : public SUnit
	{
		ZDATA_(SUnit)
	public:
		bool bOnPlace;
		ZEND int operator&( IBinSaver &f ) { f.Add(1,(SUnit*)this); f.Add(2,&bOnPlace); return 0; }
		
		SCrewMember();
		SCrewMember( const CVec2 &vServePoint, class CSoldier *pUnit = 0, const EActionNotify eAction = ACTION_NOTIFY_IDLE );
	};

	enum EGunServeState
	{
		EGSS_OPERATE = 0,
		EGSS_ROTATE = 1,
		EGSS_MOVE = 2,
	};
	
	enum EGunOperateSubState
	{
		EGOSS_AIM,
		EGOSS_AIM_VERTICAL,
		EGOSS_RELAX,
		EGOSS_RELOAD,
	};
	
	typedef std::list< SUnit > CFreeUnits;

	ZDATA
	CPtr<CFormation> pUnit;

	int nReloadPhaze;											// перезагрузка разделена на несколько фаз
	bool b360DegreesRotate;							// gun has no horisontal constraints

	// состояние пушки ( а также инжекс в массиве vGunners статов у пушки)
	EGunServeState eGunState;
	
	// подсостояния пушки в режиме Operate
	EGunOperateSubState eGunOperateSubState;

	std::vector< SCrewMember > crew; // места с меньшим номером более приоритетны

	CPtr<CArtillery> pArtillery;
	CDBPtr<SMechUnitRPGStats> pStats;

	CFreeUnits freeUnits;
	NTimer::STime startTime;
	NTimer::STime timeLastUpdate;

	bool bReloadInProgress;

	float fReloadPrice; // цена одной перезарядки
	float fReloadProgress;	// текущее состояние перезарядки

	SAIAngle wGunTurretDir ; 
	SAIAngle wGunBaseDir;
	SAIAngle wTurretHorDir; //  предыдущее направление ствола
	SAIAngle wTurretVerDir; //  предыдущее направление ствола
	int nFormationSize;
	CVec2 vGunPos;
	
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&nReloadPhaze); f.Add(4,&b360DegreesRotate); f.Add(5,&eGunState); f.Add(6,&eGunOperateSubState); f.Add(7,&crew); f.Add(8,&pArtillery); f.Add(9,&pStats); f.Add(10,&freeUnits); f.Add(11,&startTime); f.Add(12,&timeLastUpdate); f.Add(13,&bReloadInProgress); f.Add(14,&fReloadPrice); f.Add(15,&fReloadProgress); f.Add(16,&wGunTurretDir); f.Add(17,&wGunBaseDir); f.Add(18,&wTurretHorDir); f.Add(19,&wTurretVerDir); f.Add(20,&nFormationSize); f.Add(21,&vGunPos); return 0; }
	// сброс всех распределений - чтобы расставить арттиллеристов как-то по-новому
	// return true - завершить состояние
	bool ClearState();

	// для каждого EGunServeState и по номел=ру юнита выдает нужную анимацию
	SCrewAnimation CalcNeededAnimation( int iUnitNumber ) const;
	SCrewAnimation CalcAniamtionForMG( int iUnitNumber ) const;

	uint16_t CalcDirToAmmoBox( int nCrewNumber ) const;
	uint16_t CalcDirFromTo( int nCrewNumberFrom, int nCrewNumberTo ) const;

	void SetAllAnimation( EActionNotify action, bool force = false );
	void RecountPoints( const CVec2 &gunDir, const CVec2 &vTurretDir );
	void SendThatAreNotOnPlace( const bool bNoAnimation );
	int CheckThatAreOnPlace();
	void RefillCrew();
	void UpdateAnimations();
	void Interrupt();
	bool CanInterrupt() ;
	bool IsGunAttacking() const ;
public:
	static IUnitState* Instance( class CFormation *pUnit, CArtillery *pArtillery);
	
	CFormationGunCrewState() : pUnit( 0 ) { }
	CFormationGunCrewState( class CFormation *pUnit, CArtillery *pArtillery);

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

	virtual EUnitStateNames GetName() { return EUSN_GUN_CREW_STATE; }
	
	CArtillery* GetArtillery() const { return pArtillery; }
};

// 
class CFormationInstallMortarState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationInstallMortarState );
	ZDATA
	CPtr<CFormation> pUnit;
	NTimer::STime timeInstall;
	CPtr<CArtillery> pArt;
	int nStage;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&timeInstall); f.Add(4,&pArt); f.Add(5,&nStage); return 0; }
public:
	static IUnitState* Instance( class CFormation *pUnit );
	CFormationInstallMortarState() : pUnit( 0 ) { }
	CFormationInstallMortarState( class CFormation *pUnit );
	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2(-1,-1);}
};

class CFormationUseSpyglassState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationUseSpyglassState );
	ZDATA	
	CPtr<CFormation> pFormation;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, const CVec2 &point );
	
	CFormationUseSpyglassState() : pFormation( 0 ) { }
	CFormationUseSpyglassState( class CFormation *pFormation, const CVec2 &point );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
	
	virtual EUnitStateNames GetName() { return EUSN_USE_SPYGLASS; }
};

// для атаки взвода взводом
class CFormationAttackFormationState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CFormationAttackFormationState );
	ZDATA	
	CPtr<CFormation> pUnit;
	CPtr<CFormation> pTarget;
	bool bSwarmAttack;
	int nEnemyParty;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&pTarget); f.Add(4,&bSwarmAttack); f.Add(5,&nEnemyParty); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, class CFormation *pTarget, const bool bSwarmAttack );
	
	CFormationAttackFormationState() : pUnit( 0 ) { }
	CFormationAttackFormationState( class CFormation *pFormation, class CFormation *pTarget, const bool bSwarmAttack );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;

	virtual bool IsAttacksUnit() const { return true; }
	virtual class CAIUnit* GetTargetUnit() const { return 0; }
};

class CFormationParadeState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationParadeState );

	ZDATA
	CPtr<CFormation> pFormation;
	NTimer::STime startTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&startTime); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, const int nType );

	CFormationParadeState() : pFormation( 0 ) { }
	CFormationParadeState( class CFormation *pFormation, const int nType );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;

	virtual EUnitStateNames GetName() { return EUSN_PARADE; }
};

class CFormationDisbandState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationDisbandState );
	ZDATA
	CPtr<CFormation> pFormation;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation );

	CFormationDisbandState() : pFormation( 0 ) { }
	CFormationDisbandState( class CFormation *pFormation );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CFormationFormState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationFormState );
	ZDATA
	CPtr<CFormation> pFormation;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation );

	CFormationFormState() : pFormation( 0 ) { }
	CFormationFormState( class CFormation *pFormation );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CFormationWaitToFormState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationWaitToFormState );
	ZDATA
	CPtr<CFormation> pFormation;
	CPtr<CFormation> pFormFormation;
	CPtr<CSoldier> pMainSoldier;
	bool bMain;
	NTimer::STime startTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&pFormFormation); f.Add(4,&pMainSoldier); f.Add(5,&bMain); f.Add(6,&startTime); return 0; }

	//
	void FinishState();
	void FormFormation();
public:
	static IUnitState* Instance( class CFormation *pFormation, const float fMain, class CSoldier *pMainSoldier );

	CFormationWaitToFormState() : pFormation( 0 ) { }
	CFormationWaitToFormState( class CFormation *pFormation, const float fMain, class CSoldier *pMainSoldier );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_WAIT_TO_FORM; }
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const;
};

class CCatchFormationState : public IUnitState
{
	OBJECT_BASIC_METHODS( CCatchFormationState );

	enum ECatchFormationState { ECFS_NONE, ECFS_FREE, ECFS_IN_BUILDING, ECFS_IN_ENTRENCHMENT, ECFS_IN_TRANSPORT };
	// формация, которая ловит
	ZDATA
	CPtr<CFormation> pCatchingFormation;

	ECatchFormationState eState;

	CVec2 lastFormationPos;
	CPtr<CObjectBase> pLastFormationObject;

	// формация, которую ловят
	CPtr<CFormation> pFormation;
	// время для периодических проверок состояния формации
	NTimer::STime lastUpdateTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pCatchingFormation); f.Add(3,&eState); f.Add(4,&lastFormationPos); f.Add(5,&pLastFormationObject); f.Add(6,&pFormation); f.Add(7,&lastUpdateTime); return 0; }
	//
	void LeaveCurStaticObject();
	void MoveSoldierToFormation();
	void JoinToFormation();

	void SetToDisbandedState();
	void AnalyzeFreeFormation();
	void AnalyzeFormationInTransport( class CMilitaryCar *pCar );
	void AnalyzeFormationInEntrenchment( class CEntrenchment *pEntrenchment );
	void AnalyzeFormationInBuilding( class CBuilding *pBuilding );
public:
	static IUnitState* Instance( class CFormation *pCatchingFormation, class CFormation *pFormation );

	CCatchFormationState() : pCatchingFormation( 0 ) { }
	CCatchFormationState( class CFormation *pCatchingFormation, class CFormation *pFormation );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2( -1.0f, -1.0f ); }
};

class CFormationSwarmState : public IUnitAttackingState
{
	OBJECT_BASIC_METHODS( CFormationSwarmState );

	enum { TIME_OF_WAITING = 200 };
	enum EFormationSwarmStates { EFSS_START, EFSS_WAIT, EFSS_MOVING };
	ZDATA
	EFormationSwarmStates state;

	CPtr<CFormation> pFormation;

	CVec2 point;
	NTimer::STime startTime;
	bool bContinue;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&state); f.Add(3,&pFormation); f.Add(4,&point); f.Add(5,&startTime); f.Add(6,&bContinue); return 0; }

	//
	void AnalyzeTargetScan();
public:
	static IUnitState* Instance( class CFormation *pFormation, const CVec2 &point, const float fContinue );

	CFormationSwarmState() : pFormation( 0 ) { }
	CFormationSwarmState( class CFormation *pFormation, const CVec2 &point, const float fContinue );

	virtual void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );

	virtual EUnitStateNames GetName() { return EUSN_SWARM; }
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return point; }

	virtual bool IsAttacksUnit() const { return false; }
	virtual class CAIUnit* GetTargetUnit() const { return 0; }
};

class CFullBridge;
class CBridgeSpan;

class CFormationRepairBridgeState : public IEngineerFormationState
{
	OBJECT_BASIC_METHODS( CFormationRepairBridgeState );

	enum EFormationRepearBridgeState
	{
		FRBS_WAIT_FOR_HOMETRANSPORT,
		
		FRBS_START_APPROACH,
		FRBS_APPROACH,
		FRBS_REPEAR,
	};

	ZDATA
	CPtr<CFormation> pUnit;
	EFormationRepearBridgeState eState;
	CPtr<CFullBridge> pBridgeToRepair;
	CPtr<CAITransportUnit> pHomeTransport ;
	std::vector< CObj<CBridgeSpan> > bridgeSpans;
	
	float fWorkLeft;											// RU that engineers have with them
	float fWorkDone;
	NTimer::STime timeLastCheck;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&pBridgeToRepair); f.Add(5,&pHomeTransport); f.Add(6,&bridgeSpans); f.Add(7,&fWorkLeft); f.Add(8,&fWorkDone); f.Add(9,&timeLastCheck); return 0; }

public:
	static IUnitState* Instance( class CFormation *pFormation, class CBridgeSpan *pBridge );

	CFormationRepairBridgeState() : pUnit( 0 ) { }
	CFormationRepairBridgeState ( class CFormation *pFormation, class CBridgeSpan *pBridge );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2(-1,-1); }

	virtual void SetHomeTransport( class CAITransportUnit *pTransport );
	virtual EUnitStateNames GetName() { return EUSN_REPAIR_BRIDGE; }

};

class CFormationRepairBuildingState : public IEngineerFormationState
{
	OBJECT_BASIC_METHODS( CFormationRepairBuildingState );

	enum EFormationRepairBuildingState
	{
		EFRBS_WAIT_FOR_HOME_TRANSPORT,
		EFRBS_START_APPROACH,
		EFRBS_APPROACHING,
		EFRBS_START_SERVICE,
		EFRBS_SERVICING,
	};

	ZDATA
	CPtr<CFormation> pUnit;
	EFormationRepairBuildingState eState;

	CPtr<CAITransportUnit> pHomeTransport;
	CPtr<CBuilding> pBuilding;

	float fWorkAccumulator;
	float fWorkLeft;
	NTimer::STime lastRepearTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&pHomeTransport); f.Add(5,&pBuilding); f.Add(6,&fWorkAccumulator); f.Add(7,&fWorkLeft); f.Add(8,&lastRepearTime); return 0; }

	void Interrupt();
public:
	static IUnitState* Instance( class CFormation *pFormation, class CBuilding *pBuilding );

	CFormationRepairBuildingState() : pUnit( 0 ) { }
	CFormationRepairBuildingState( class CFormation *pFormation, class CBuilding *pBuilding );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2(-1,-1); }

	virtual void SetHomeTransport( class CAITransportUnit *pTransport );
	
	// returns nearest entrance
	static int SendToNearestEntrance( CCommonUnit *pTransport, CBuilding * pStorage );

	virtual EUnitStateNames GetName() { return EUSN_REPAIR_BUILDING; }
};

class CFormationEnterBuildingNowState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationEnterBuildingNowState );
	ZDATA 
	CPtr<CFormation> pFormation;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, class CBuilding *pBuilding );

	CFormationEnterBuildingNowState() : pFormation( 0 ) { }
	CFormationEnterBuildingNowState( class CFormation *pFormation, class CBuilding *pBuilding );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2(-1.0f,-1.0f); }
};

class CFormationEnterEntrenchmentNowState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationEnterEntrenchmentNowState );
	ZDATA 
		CPtr<CFormation> pFormation;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); return 0; }
public:
	static IUnitState* Instance( class CFormation *pFormation, class CEntrenchment *pEntrenchment );

	CFormationEnterEntrenchmentNowState() : pFormation( 0 ) { }
	CFormationEnterEntrenchmentNowState( class CFormation *pFormation, class CEntrenchment *pEntrenchment );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return CVec2(-1.0f,-1.0f); }
};

class CFormationPlaceChargeState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CFormationPlaceChargeState )

	enum EPlaceChargeState
	{
		EPCS_MOVING_TO,
		EPCS_LAYING_CHARGE,
		EPCS_RETURNING,
	};

	ZDATA_( CStatusUpdatesHelper )
	CPtr<CFormation> pFormation;
	CPtr<CSoldier> pSapper;

	CVec2 vTarget;
	int nOffset;
	EPlaceChargeState eState;
	int nBeginAnimTime;
	NDb::EUnitSpecialAbility eChargeType;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&pFormation); f.Add(3,&pSapper); f.Add(4,&vTarget); f.Add(5,&nOffset); f.Add(6,&eState); f.Add(7,&nBeginAnimTime); f.Add(8,&eChargeType); return 0; }
	void ReturnSapper();
	
public:
	static IUnitState* Instance( class CFormation *pUnit, const CVec2 &vDesiredPoint, NDb::EUnitSpecialAbility _eChargeType, int nTimeOffset );
	
	CFormationPlaceChargeState() : pFormation( 0 ) {}
	CFormationPlaceChargeState( class CFormation *pUnit, const CVec2 &vDesiredPoint, NDb::EUnitSpecialAbility _eChargeType, int nTimeOffset );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const;
};

class CFormationDetonateChargeState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationDetonateChargeState )

	ZDATA
	CPtr<CFormation>pFormation;
	std::list< CPtr<CSoldier> > sappers;
	int nBeginAnimTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&sappers); f.Add(4,&nBeginAnimTime); return 0; }

public:
	static IUnitState* Instance( class CFormation *pUnit );

	CFormationDetonateChargeState() : pFormation( 0 ) {}
	CFormationDetonateChargeState( class CFormation *pUnit );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return CVec2(-1.0f,-1.0f); }
};

class CFormationThrowGrenadeState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationThrowGrenadeState )

	struct SThrowInfo
	{
		ZDATA
		CPtr<CSoldier> pSoilder;
		bool bPassSegment;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSoilder); f.Add(3,&bPassSegment); return 0; }
	public:
		SThrowInfo() : bPassSegment( true ), pSoilder( 0 ) {}
		SThrowInfo( class CSoldier *_pSoilder ) : bPassSegment( true ), pSoilder( _pSoilder ) {}
	};

	ZDATA 
	CPtr<CFormation> pFormation;
	std::vector<SThrowInfo> vSoldiers;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&vSoldiers); return 0; }

public:
	static IUnitState* Instance( class CFormation *pUnit );

	CFormationThrowGrenadeState() : pFormation( 0 ) {}
	CFormationThrowGrenadeState( class CFormation *pUnit );

	virtual void Segment();

	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return true; }
	virtual const CVec2 GetPurposePoint() const { return VNULL2; }
	virtual EUnitStateNames GetName() { return EUSN_FORM_THROW_GRENADE; }

	void AddTarget( CAIUnit *pEnemy, const CVec2 &vTarget, const int nParam );
};

class CFormationEntrenchSelfState : public IUnitState, public CStatusUpdatesHelper
{
	OBJECT_BASIC_METHODS( CFormationEntrenchSelfState )
	ZDATA_( CStatusUpdatesHelper )
	CPtr<CFormation> pFormation;
	NTimer::STime timeStart;
	bool bWaitForSoldiers;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStatusUpdatesHelper *)this); f.Add(2,&pFormation); f.Add(3,&timeStart); f.Add(4,&bWaitForSoldiers); return 0; }
public:
	CFormationEntrenchSelfState() : pFormation( 0 ) {  }
	CFormationEntrenchSelfState( class CFormation *_pFormation );

	void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return VNULL2; }
};

class CFormationLeaveSelfEntrenchState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationLeaveSelfEntrenchState );
	ZDATA
	CPtr<CFormation> pFormation;
	NTimer::STime timeStart;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&timeStart); return 0; }
public:
	CFormationLeaveSelfEntrenchState() : pFormation( 0 ) { }
	CFormationLeaveSelfEntrenchState( class CFormation *_pFormation );
	void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return VNULL2; }

};

class CFormationFirstAidState : public IUnitState
{
	OBJECT_BASIC_METHODS( CFormationFirstAidState );
	struct SHealingPair
	{
		enum EPairState
		{
			EPS_JUST_CREATED,
			EPS_HEADING_TO_PATIENT,
			EPS_DOC_NEAR_PATIENT,
			EPS_HEAL_STARTED,
			EPS_HEAL_COMPLETE,
		};
		ZDATA
		CPtr<CSoldier> pPatient;
		CPtr<CSoldier> pDoctor;
		NTimer::STime timeHealed;
		EPairState  eState;										// doc is near patient
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pPatient); f.Add(3,&pDoctor); f.Add(4,&timeHealed); f.Add(5,&eState); return 0; }
	public:
		~SHealingPair();
		SHealingPair() {  }
		SHealingPair( CSoldier *_pPatient, CSoldier *_pDoctor ) ;
		void SetNewPatient( CSoldier *_pPatient );
		void Heal();
	};

	ZDATA
	CPtr<CFormation> pFormation;								// current formation
	CPtr<CFormation> pHealeadFormation;		// heal only this formation
	std::vector<SHealingPair> healingPairs;
	NTimer::STime timeNextCheck;
	CVec2 vStartPoint;										// point to heal around

	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFormation); f.Add(3,&pHealeadFormation); f.Add(4,&healingPairs); f.Add(5,&timeNextCheck); f.Add(6,&vStartPoint); return 0; }
	// return true if found patient
	bool FindNearestPatient( CSoldier *pDoctor, CSoldier *pFormerPatient, CSoldier **pNewPatient );
	bool IsBeingHeales( CSoldier *pSold );
	void SetBeingHealed( CSoldier *pSold, const bool bHealed );
	static bool IsSoldierNeedHealing( CAIUnit *pUnit );
public:

	CFormationFirstAidState() : pFormation( 0 ) { }
	CFormationFirstAidState( class CFormation *_pFormation, CSoldier *pPriorityUnit );

	static bool IsAnyNeedHealing( const int nParty, const CVec3 &vCenter );

	void Segment();
	virtual ETryStateInterruptResult TryInterruptState( class CAICommand *pCommand );
	virtual bool IsAttackingState() const { return false; }
	virtual const CVec2 GetPurposePoint() const { return VNULL2; }
};


