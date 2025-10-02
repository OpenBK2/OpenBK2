#pragma once

#include "Scripts.h"
#include "B2AI.h"
#include "Common_RTS_AI/Terrain.h"

namespace NDb
{
	struct SMapInfo;
	struct SAIGameConsts;
	struct STerrain;
}

class CCommonUnit;
class CAIUnit;
class CBridgeSpan;
struct SMiniMapUnitInfo;
struct SAIBasicUpdate;
struct ICheckSumLog;
struct ICollisionsCollector;

typedef hash_map< int, SMapObjectInfo::SLinkInfo> LinkInfo;

class CAILogic : public IAILogic
{
	OBJECT_NOCOPY_METHODS( CAILogic );
	public: int operator&( IBinSaver &saver ); private:;

	CDBPtr<NDb::SAIGameConsts> pConsts;
	bool bSuspended;
	bool bFirstTime;
	
	// мосты
	typedef list< list<CPtr<CBridgeSpan> > > Bridges;
	Bridges bridges;
	CAITimer timer;

	//
	list< CObj<CCommonUnit> > garbage;

	// скрипты
	CScripts scripts;
	bool bMissionLoaded;
	
	vector< SAIStartCommand > startCmds;
	vector< SBattlePosition > reservePositions;

	NTimer::STime nextCheckSumTime;
	NTimer::STime periodToCheckSum;
	uLong checkSum;

	bool bSegment;
	bool bNetGameStarted;

	CPtr<ICheckSumLog> pCheckSumLog;
	
	typedef hash_set<CDBPtr<SMechUnitRPGStats>, SDefaultPtrHash> CAvailTrucks;
	CAvailTrucks availableTrucks;
	
	CPtr<IProgressHook> pProgress;

	CObj<CAIMap> pAIMap;
	CObj<ICollisionsCollector> pCollisionsCollector;
	CPtr<IAIScenarioTracker> pScenarioTracker;

	NTimer::STime timeLocalPlayerUnitCheck;
	bool bLocalPlayerUnitsPresent;

	NTimer::STime timeLastMiniMapUpdateUnits;
	bool bNeedNewGroupNumber;
	
	// проверить, не является ли object грузовиком, подцеплённым к сценарийной артиллерии
	// если да, ищет подходящий к артиллерии грузовик (в pNewStats) и возвращает true, если артиллерия не найдена - возвращает false
	// если это не является таким грузовиком, возвращает true
	bool CheckForScenarioTruck( const SMapObjectInfo &object, LinkInfo *linksInfo, const SMechUnitRPGStats **pNewStats ) const;

	// Loading
	// часть инициализации, общая для игры и редактора
	void CommonInit( const NDb::STerrain *pTerrainInfo );

	void LoadUnits( const struct NDb::SMapInfo* pMapInfo, LinkInfo *linksInfo );
	void UpdateUnitsOnBridges();
	void InitReservePositions();
	void InitStartCommands();
	void LaunchStartCommand( const SAIStartCommand &startCommand, CObjectBase **pUnitsBuffer, const int nSize );
	// bSend - whether to send checksum
	// bFullAI - count all AI data or only object placement
	void UpdateCheckSum( bool bSend );

	bool CanShowVisibilities() const;

	void LinkArtilleryWithTransport( class CArtillery * pArtillery, class CAITransportUnit *pTransport );

public:
	CAILogic();
	~CAILogic();

	int GetScriptID( class CUpdatableObj *pObj ) const { return scripts.GetScriptID( pObj ); }

	IAIScenarioTracker *GetScenarioTracker() { return pScenarioTracker; }

	void ToGarbage( class CCommonUnit *pUnit );
	virtual void Suspend();
	virtual void Resume();
	virtual bool IsSuspended() const { return bSuspended; }

	void SetProgressHook( IProgressHook *pProgress );
	virtual void Init( ICheckSumLog *_pCheckSumLog, const struct NDb::SMapInfo *pMapInfo, const NDb::SAIGameConsts *_pConsts, IAIScenarioTracker *_pScenarioTracker );
	virtual void LogCheckSum( ICheckSumLog *pCheckSumLog );
	virtual void InitAfterMapLoad( const struct NDb::SMapInfo *pMapInfo/*, IProgressHook * pProgress = 0 */);
	virtual void PostMapLoad();
	virtual void ClearAI();

	virtual void UnitCommand( SAIUnitCmd *pCommand, const WORD wGroupID, const int nPlayer );

	virtual const int GenerateGroupNumber();
	virtual void RegisterGroup( const vector<int> &vIDs, const int nGroup );
	virtual void RegisterGroup( CObjectBase **pUnitsBuffer, const int nLen, const WORD wGroup );
	virtual void UnregisterGroup( const int nGroup );
	virtual void GroupCommand( SAIUnitCmd *pCommand, const WORD wGroup, bool bPlaceInQueue );
	
	virtual void ShowAreas( const vector<int> &units, enum EActionNotify &eType, bool bShow );
	virtual void GetShootAreas( int nUnitID, SShootAreas *pAreas );
	
	virtual const bool GetMiniMapUnitsInfo( vector< SMiniMapUnitInfo > &vUnits );
	virtual bool GetMiniMapWarForInfo( CArray2D<BYTE> **pWarFogInfo, bool bFirstTime );
	virtual int GetMiniMapWarFogSizeX() const;
	virtual int GetMiniMapWarFogSizeY() const;
	virtual void CallScriptFunction( const char *pszCommand );
	
	virtual int GetUniqueIDOfObject( CObjectBase *pObj );
	virtual CObjectBase* GetObjByUniqueID( const int id );

	virtual void Segment();

	CObjectBase* AddObject( const int nUniqueID, const SMapObjectInfo &object, LinkInfo *linksInfo, bool bInitialization, const SHPObjectRPGStats *pPassedStats, NDb::EReinforcementType eType = _RT_NONE );

	void InitLinks( LinkInfo &linksInfo );
	void LoadEntrenchments( const vector<struct SEntrenchmentInfo> &entrenchments );
	void LoadBridges( const vector< NDb::SIntArray > &bridgesInfo );

	virtual void SetMyDiplomacyInfo( const int nParty, const int nNumber );
	virtual void SetNPlayers( const int nPlayers );
	virtual void SetNetGame( const bool bNetGame );
	
	virtual bool SubstituteUniqueIDs( const vector<int> &vIDs );
	virtual bool SubstituteUniqueIDs( CObjectBase **pUnitsBuffer, const int nLen );

	bool UpdateAcknowledgment( SAIAcknowledgment &pAck );
	bool UpdateAcknowledgment( SAIBoredAcknowledgement &pAck );
	
	virtual float GetZ( const CVec2 &vPoint ) const;
	virtual const DWORD GetNormal( const CVec2 &vPoint ) const;
	virtual const bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;
	
	virtual bool ToggleShow( const int nShowType );

	virtual bool IsCombatSituation();
	void InitStartCommands( const LinkInfo &linksInfo, hash_map<int, int> &old2NewLinks );
	void InitReservePositions( hash_map<int, int> &old2NewLinks );
	
	bool IsSegment() const { return bSegment; }
	
		// difficuly levels
	virtual void SetDifficultyLevel( const int nLevel );
	virtual void SetCheatDifficultyLevel( const int nCheatLevel );
	
	const bool IsFirstTime() const { return bFirstTime; }

	virtual void SendAcknowlegdementForced( CObjectBase *pObj, const EUnitAckType eAck );	

	// for debug
	virtual int GetUniqueID( CObjectBase *pObj );
	
	// при игре в multiplayer: все игроки загрузились и игра стартовала
	virtual void NetGameStarted();
	virtual bool IsNetGameStarted() const;

	virtual const class CDifficultyLevel* GetDifficultyLevel() const;
	
	virtual void NeutralizePlayer( const int nPlayer );

	virtual CObjectBase* GetUnitState( CObjectBase *pObj );
	virtual bool IsFrozen( CObjectBase *pObj ) const;
	virtual bool IsFrozenByState( CObjectBase *pObj ) const;
	
	virtual void GetGridUnitsCoordinates( const int nGroup, const CVec2 &vGridCenter, CVec2 **pCoord, int *pnLen );
	
	virtual CObjectBase* GetUpdate();
	virtual void PrepareUpdates();
	virtual void ToggleWarFog( const bool bWarFog );
	struct ITerraAIObserver * CreateTerraAIObserver( const int nSizeX, const int nSizeY );

	virtual void PickedObj( const int nObjID );
	virtual void PickEmpty();

	virtual const int WAR_FOG_FULL_UPDATE() const;
	virtual const int VIS_POWER() const;

	void RequestBuildPreview( const EActionCommand eBuildCommand, const CVec2 &vStart, const CVec2 &vEnd, bool bFinished = false );

	virtual bool IsMissionLoaded() const { return bMissionLoaded; }

	virtual void SetGlobeScriptHandler( IGlobeScriptHandler *pHandler );

	class CAIMap* GetAIMap() const { return pAIMap; }
	virtual class CStaticMapHeights* GetHeights() const;

	virtual const int GetPlayerDiplomacy( const int nPlayer ) const;

	void SetNeedNewGroupNumber() { bNeedNewGroupNumber = true; }
	const bool NeedNewGroupNumber() const { return bNeedNewGroupNumber; }
	void ResetNeedNewGroupNumber() { bNeedNewGroupNumber = false; }

	virtual const NDb::SAIGameConsts *GetAIConsts() const { return pConsts; }
	virtual const int GetUnitCount( const int nPlayer ) const;
	virtual const bool HasPlayerNoUnits( const int nPlayer ) const;
	virtual void DumpAfterAssinc() const;
};


