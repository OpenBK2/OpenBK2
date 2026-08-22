#pragma once

#include "ClientAckManager.h"
#include "Misc/HashFuncs.h"

#include <cstdint>

namespace NDb
{
	enum EAckClass;
	enum EAckPosition;
	struct SAckParameter;
	struct SClientGameConsts;
	struct SComplexSoundDesc;
}

typedef std::unordered_map< int/*AckType*/, int/*index in consts*/ > CUnitAcksInfo;

class CClientAckManager : public IClientAckManager
{
	OBJECT_BASIC_METHODS(CClientAckManager);

	//
	//
	class CBoredUnitsContainer
	{
		typedef std::unordered_map<CPtr<IMOUnit>, bool, SPtrHash> CBoredUnits;
		CBoredUnits boredUnits;							
		int nCounter;												// для оптимизации - размер boredUnits
		NTimer::STime timeLastBored;				// time for last bored sound
		void Copy( const CBoredUnitsContainer &cp );
	public:
		int operator&( IBinSaver &save );
		CBoredUnitsContainer();
		CBoredUnitsContainer operator=( const CBoredUnitsContainer &cp ) { Copy(cp); return *this; }
		CBoredUnitsContainer( const CBoredUnitsContainer&cp ) { Copy(cp); }

		int GetCount() const { return nCounter; }

		void AddUnit( struct IMOUnit *pUnit );
		void DelUnit( struct IMOUnit *pUnit );
		// если время подошло, то заставляет рандомного юнита послать Ack. 
		//Если ack послали, то true;
		bool SendAck( const NTimer::STime curTime, 
			const NDb::EUnitAckType eBored, 
			IClientAckManager *pAckManager,
			const NTimer::STime timeInterval );
		void Clear();
	};

	// 
	struct SAck
	{
		NDb::EUnitAckType eAck;
		CDBPtr<NDb::SComplexSoundDesc> pSound;
		enum ESoundMixType eMixType;

		int operator&( IBinSaver &saver );
		bool operator==( const SAck & ack ) const { return eAck == ack.eAck; }
	};
	typedef std::list< SAck > CAcks;
	//	
	struct SUnitAck
	{
		int operator&( IBinSaver &saver );
		CAcks acks;													// очередь Ack, которые еще проигрывать
		uint16_t wSoundID;											// этот звук играет
		int /*EUnitAckType*/ eCurrentAck;		// этот  Ack сейчас играет
		NTimer::STime timeRun;							// время когда можно проиграть AckPisitive
		SUnitAck()
			:wSoundID( 0 ), eCurrentAck( -1 ), timeRun( 0 ) { }
	};
	// acknowledgement of dead unit
	struct SDeathAck
	{
		int operator&( IBinSaver &saver );
		CDBPtr<NDb::SComplexSoundDesc> pSound;
		CVec3 vPos;
		NTimer::STime timeSinceStart; 
		SDeathAck() {  }
		SDeathAck( const CVec3 &_vPos, const NDb::SComplexSoundDesc *_pSound, const unsigned int nTimeSinceStart )
			: vPos( _vPos ), pSound( _pSound ), timeSinceStart( nTimeSinceStart ) {  }
	};
	//
	typedef std::unordered_map< int, NTimer::STime > CUnitAcksPresence;
	typedef std::unordered_map< CPtr<IMOUnit>, SUnitAck > CUnitsAcks;
	typedef std::list< SDeathAck > CDeathAcks;

	//для выбора заданного типа асков
	class CAckPredicate
	{
		const NDb::EAckClass eType;
		const CUnitAcksInfo &acksInfo;
		const NDb::SClientGameConsts * pConsts;
	public:
		CAckPredicate( const NDb::EAckClass _eType, const CUnitAcksInfo &_acksInfo, const NDb::SClientGameConsts * _pConsts ) 
			: acksInfo( _acksInfo ), eType( _eType ), pConsts( _pConsts ) {  }
		bool operator()( const SAck & a );
	};

	IConsoleBuffer *pConsoleBuffer;
	struct IGameTimer *pGameTimer;
	static CDBPtr<NDb::SClientGameConsts> pConsts;

	CPtr<IMOUnit> pLastSelected;
	int nSelectionCounter;

	// тут инфо о юнитах, зарегистрированных в bored состояниях
	typedef std::unordered_map<int, CBoredUnitsContainer> BoredUnits;
	BoredUnits boredUnits;	

	CUnitAcksPresence acksPresence;				// наличие в даный момент в звучании данного Ack'а 
	CUnitsAcks				unitAcks;						// для хранения всех текущих Ack'ов
	CDeathAcks				deathAcks;					// for death acknowledgements;
	NTimer::STime timeLastDeath;

	// не сериалайзится.
	CUnitAcksInfo			acksInfo;						// данные об Ack'ах
	std::unordered_map<std::string,int> loadHelper;
	// константы
	int TIME_ACK_WAIT;
	int NUM_SELECTIONS_BEFORE_F_OFF;
	int ACK_BORED_INTERVAL;
	int ACK_BORED_INTERVAL_RANDOM;

	void InitConsts();

	void RegisterAck( SUnitAck *ack, const NTimer::STime curTime  );
	void UnregisterAck( SUnitAck *ack );
	const NDb::SAckParameter * GetParam( const NDb::EUnitAckType eAck ) const;
public:
	int operator&( IBinSaver &saver );
	CClientAckManager();

	void Init();
	void Clear();
	bool IsNegative( const NDb::EUnitAckType eAck );

	void SetClientConsts( const NDb::SClientGameConsts *pConsts );
	void AddDeathAcknowledgement( const CVec3 &vPos, const NDb::SComplexSoundDesc *pSound, const unsigned int nTimeSinceStart );
	void AddAcknowledgement( struct IMOUnit *pUnit, const NDb::EUnitAckType eAck, const NDb::SComplexSoundDesc *pSound, const int nSet, const unsigned int nTimeSinceStart );
	void UnitDead( struct IMOUnit *pUnit, struct ISoundScene *pSoundScene );
	void Update( struct ISoundScene *pSoundScene );
	void RegisterAsBored( const NDb::EUnitAckType eBored, struct IMOUnit *pObject );
	void UnRegisterAsBored( const NDb::EUnitAckType eBored, struct IMOUnit *pObject );
};


