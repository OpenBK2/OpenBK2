
#pragma once

#include "GeneralInternalInterfaces.h"

class CAntiArtillery;
class CAIUnit;
class CGeneralArtillery;
class CGeneral;

class CGeneralArtilleryGoToPosition : public CAIObjectBase
{
	OBJECT_BASIC_METHODS( CGeneralArtilleryGoToPosition );
	
	enum EBombardmentState { EBS_START, EBS_MOVING, EBS_WAIT_FOR_TRUCK, EBS_MOVING_WITH_TRUCK, EBS_FINISHING };

	ZDATA
	CPtr<CAIUnit> pUnit;
	EBombardmentState eState;

	CVec2 vPos;
	bool bToReservePosition;
	bool bFinished;
	NTimer::STime timeOfFinish;
	NTimer::STime startTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&eState); f.Add(4,&vPos); f.Add(5,&bToReservePosition); f.Add(6,&bFinished); f.Add(7,&timeOfFinish); f.Add(8,&startTime); return 0; }
	// 
	void StartState();
	void WaitForTruck();
	void MovingWithTruck();
	void Finishing();
public:
	CGeneralArtilleryGoToPosition() { }
	CGeneralArtilleryGoToPosition( CAIUnit *pUnit, const CVec2 &vPos, bool bToReservePosition );

	void Segment();
	bool IsFinished() { return bFinished; }

	bool DoesGoToReservePosition() const { return bToReservePosition; }
};

class CGeneralArtilleryTask
{

	struct SBombardmentUnitState
	{
	public:
		ZDATA
		CVec2 vReservePosition;
		CVec2 vAttackPos;

		CPtr<CAIUnit> pUnit;
		CPtr<CGeneralArtilleryGoToPosition> pGoToPosition;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&vReservePosition); f.Add(3,&vAttackPos); f.Add(4,&pUnit); f.Add(5,&pGoToPosition); return 0; }
	public:
		SBombardmentUnitState() { }
		explicit SBombardmentUnitState( CAIUnit *pUnit );
	};

	enum EBombardmentState { EBS_START, EBS_ROTATING, EBS_GOING_TO_BATTLE, EBS_FIRING, EBS_ESCAPING };

	ZDATA
	CPtr<CGeneralArtillery> pOwner;

	bool bBombardmentFinished;

	CVec2 vBombardmentCenter;
	float fBombardmentRadius;
	NTimer::STime timeToFinishBombardment;
	NTimer::STime timeToSendAntiArtilleryAck;
	CVec2 vAntiArtilleryAckCenter;
	bool bIsAntiArtilleryFight;
	NTimer::STime startRotatingTime;

	EBombardmentState eState;

	std::list<SBombardmentUnitState> bombardmentUnits;

	int nCellNumber;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&bBombardmentFinished); f.Add(4,&vBombardmentCenter); f.Add(5,&fBombardmentRadius); f.Add(6,&timeToFinishBombardment); f.Add(7,&timeToSendAntiArtilleryAck); f.Add(8,&vAntiArtilleryAckCenter); f.Add(9,&bIsAntiArtilleryFight); f.Add(10,&startRotatingTime); f.Add(11,&eState); f.Add(12,&bombardmentUnits); f.Add(13,&nCellNumber); return 0; }

	//
	// начало обстрела - дать команды подцепиться к грузовикам и ехать
	void StartBombardment();
	// как только приехали, дать команду развернуться к врагу
	void GoingToBattle();
	// как только развернулись, дать команду стрелять
	void Rotating();
	// когда отстрелялись, дать команду уезжать на резервные позиции
	void Firing();
	// когда приехали на резервные позиции, закончить артобстрел
	void Escaping();
	// если часть юнитов по какой-то причине не смогла провести артобстрел, проследить, 
	// чтобы они вернулись на резервные позиции
	void CheckEscapingUnits();

	void CalculateTimeToSendAntiArtilleryAck();

	void SetBombardmentFinished();
public:
	CGeneralArtilleryTask() : bBombardmentFinished( true ), bIsAntiArtilleryFight( false ) { }
	CGeneralArtilleryTask( CGeneralArtillery *pOwner, std::list<CAIUnit*> &givenUnits, bool bAntiArtilleryFight, const CVec2 &vCenter, const float fRadius, const int nCellNumber );

	void Segment();

	bool IsTaskFinished() const { return bBombardmentFinished; }
};

class CGeneralArtillery : public CAIObjectBase, public IEnemyEnumerator
{
	OBJECT_BASIC_METHODS( CGeneralArtillery );
	
	typedef std::list< CPtr<CAIUnit> > CUnitsList;

	ZDATA
	CPtr<CGeneral> pOwner;
	CUnitsList freeUnits;
	CUnitsList trucks;

	std::list<CGeneralArtilleryTask> tasks;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&freeUnits); f.Add(4,&trucks); f.Add(5,&tasks); return 0; }
	void OnSerialize( IBinSaver &saver );
public:
	CGeneralArtillery() { }
	CGeneralArtillery( CGeneral *pGeneralOwner );
	
	// дать генералу юнит для использования в качестве артиллерии
	void TakeArtillery( CAIUnit *pUnit );
	// the same for trucks
	void TakeTruck( CAIUnit *pUnit );

	void Segment();
	bool CanBombardRegion( const CVec2 &vRegionCenter );

	int RequestForSupport( const CVec2 &vCenter, const float fRadius, bool bIsAntiArtilleryFight, const int nCellNumber );
	void CancelRequest( int nRequestID, enum EForceType eType );

	void SetEnemyContainer( IEnemyContainer *pEnemyContainer );

	//IEnemyEnumerator
	virtual bool EnumEnemy( class CAIUnit *pEnemy );

	const int GetNFreeUnits() const { return freeUnits.size(); }

	void SetCellInUse( const int nResistanceCell, bool bInUse );
};


