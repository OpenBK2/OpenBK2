#pragma once

#include "General.h"
#include "Commander.h"
#include "Resistance.h"
#include "System/FreeIDs.h"
#include "Stats_B2_M1/DBMapInfo.h"



class CArtillery;
class CAIUnit;
class CFormation;
class CCommonUnit;
class CGeneralAirForce;
class CGeneralArtillery;
class CGeneralIntendant;

class CGeneral : public CCommander, public IEnemyContainer
{
	OBJECT_BASIC_METHODS(CGeneral);

	typedef std::unordered_map< int/*request ID*/, CPtr<IGeneralTask> > RequestedTasks;
	typedef std::pair< CPtr<CAIUnit>, NTimer::STime> CUnitTimeSeen;
	typedef std::unordered_map< int/* unit unique ID*/, CUnitTimeSeen > CEnemyVisibility;

	//{ do not save these, it is only for IN-Segment use
	CEnemyVisibility::iterator curProcessed;	// cannot be saved, so there will be some tricks
	std::list<int> erased;
	//}

	ZDATA_(CCommander)
		ZONSERIALIZE
	CFreeIds requestIDs;
	int nParty;													// general is for this player

	CEnemyVisibility enemys;
	CEnemyVisibility antiAviation;

	CommonUnits infantryInTrenches;				//commander strores only unasigned units
	CommonUnits infantryFree;
	CommonUnits tanksFree;
	CommonUnits stationaryTanks;
	CommonUnits transportsFree;

	NTimer::STime timeNextUpdate;					// next update of this general
	std::unordered_set<int> mobileReinforcementGroupIDs;

	//Distribution of availability of own units in reinforcements.
	std::vector<float>	enemyByRType;					// Current balance of forces (enemy's distribution of reinfs minus own forces)
	int						nAirReinfTurnCounter;		// counter to give reinforcement to AirGeneral every n-th turn

	CObj<CGeneralAirForce> pAirForce;
	CObj<CGeneralArtillery> pGeneralArtillery;
	CObj<CGeneralIntendant> pIntendant;

	RequestedTasks requestedTasks;
	
	CResistancesContainer resContainer;

	NTimer::STime lastBombardmentCheck;
	// 0 - артиллерия, 1 - бомберы
	BYTE cBombardmentType;
	bool bSendReserves;										// send tanks to swarm
	int nMaxAllowedMobileTanks;

	public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CCommander*)this); OnSerialize( f ); f.Add(2,&requestIDs); f.Add(3,&nParty); f.Add(4,&enemys); f.Add(5,&antiAviation); f.Add(6,&infantryInTrenches); f.Add(7,&infantryFree); f.Add(8,&tanksFree); f.Add(9,&stationaryTanks); f.Add(10,&transportsFree); f.Add(11,&timeNextUpdate); f.Add(12,&mobileReinforcementGroupIDs); f.Add(13,&enemyByRType); f.Add(14,&nAirReinfTurnCounter); f.Add(15,&pAirForce); f.Add(16,&pGeneralArtillery); f.Add(17,&pIntendant); f.Add(18,&requestedTasks); f.Add(19,&resContainer); f.Add(20,&lastBombardmentCheck); f.Add(21,&cBombardmentType); f.Add(22,&bSendReserves); f.Add(23,&nMaxAllowedMobileTanks); return 0; }
		void OnSerialize( IBinSaver &saver );
	// сегмент принятия решения - начинать арт. обстрел / бомбардировку или нет
	void BombardmentSegment();
	// дать команду начать арт. обстрел
	void GiveCommandToBombardment();

	//Checks for reinforcement and calls if available
	void CheckAvailableReinforcement();
	void InitRearManager();								//do map-dependent calculations
	enum EBalanceAction
	{
		BA_ADD_ENEMY,
		BA_REMOVE_ENEMY,
		BA_ADD_OWN,
		BA_REMOVE_OWN
	};
	void BalanceUpdate( EBalanceAction eAction, CCommonUnit *_pUnit );

	void EraseLastSeen();
	
	void ArtilleryBombardment();
	void AviationBombardment();
	void Bombardment();
public:
	CGeneral()
		: nParty( -1 ), 
			lastBombardmentCheck( 0 ), timeNextUpdate( 1000 ), 
			nAirReinfTurnCounter( 0 ), nMaxAllowedMobileTanks( 0 )
	{ 
		curProcessed = enemys.end();
	}
	CGeneral( const int _nParty ) : nParty( _nParty ), 
		lastBombardmentCheck( 0 ), timeNextUpdate( 1000 ), 
		nAirReinfTurnCounter( 0 ) 
	{ 
		curProcessed = enemys.end();
	}

	// сервисные функции
	void Init( const struct NDb::SAIGeneralSide &mapInfo );
	void Init();
	// появились новые юниты
	void GiveNewUnits( const std::list<CCommonUnit*> &pUnits,  bool bFromReinforcement = false );

	// для манипулирования мобильными резервами
	bool IsMobileReinforcement( int nGroupID ) const;

	// для того, чтобы следить за видимыми врагами
	void SetUnitVisible( class CAIUnit *pUnit, const bool bVisible );
	void SetAAVisible( class CAIUnit *pUnit, const bool bVisible );

	//IEnemyContainer
	void GiveEnemies( IEnemyEnumerator *pEnumerator );
	virtual void AddResistance( const CVec2 &vCenter, const float fRadius );
	virtual void RemoveResistance( const CVec2 &vCenter );

	//ICommander
	virtual float GetMeanSeverity() const { return 0; }
	virtual void EnumWorkers( const EForceType eType, IWorkerEnumerator *pEnumerator );
	virtual void GiveResistances( IEnemyEnumerator *pEnmumerator );

	// при получении подкрепления его нужно отдать в управление генералу.
	// забирает работника назад
	void Give( CCommonUnit *pWorker, bool bFromReinforcement );
	void Give( CCommonUnit *pWorker ) { Give( pWorker, true ); }

	void Segment();

	virtual void CancelRequest( int nRequestID, enum EForceType eType  );
	virtual int /*request ID*/CGeneral::RequestForSupport( const CVec2 &vSupportCenter, enum EForceType eType );

	
	// для очагов сопротивления
	void UpdateEnemyUnitInfo( class CAIUnitInfoForGeneral *pInfo,
		const NTimer::STime lastVisibleTimeDelta, const CVec2 &vLastVisiblePos,
		const NTimer::STime lastAntiArtTimeDelta, const CVec2 &vLastVisibleAntiArtCenter, const float fDistToLastVisibleAntiArt );
	void UnitDied( class CAIUnitInfoForGeneral *pInfo );
	void UnitDied( class CCommonUnit * pUnit );
	void UnitChangedParty( CAIUnit *pUnit, const int nNewParty );

	// to allow Intendant tracking registered units 
	void UnitChangedPosition( class CCommonUnit * pUnit, const CVec2 &vNewPos );
	void UnitAskedForResupply( class CCommonUnit * pUnit, const EResupplyType eType, const bool bSet );

	void SetCellInUse( const int nResistanceCellNumber, bool bInUse );
	bool IsInResistanceCircle( const CVec2 &vPoint ) const;
	int GetParty() const { return nParty; }
};


