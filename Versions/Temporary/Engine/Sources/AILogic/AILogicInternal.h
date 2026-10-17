#pragma once

#include "B2AI.h"
#include "Common_RTS_AI/Terrain.h"
#include "Scripts.h"
#include "System/det_map.h"

#include <cstdint>

namespace NDb
{
struct SMapInfo;
struct SAIGameConsts;
struct STerrain;
} // namespace NDb

class CCommonUnit;
class CAIUnit;
class CBridgeSpan;
struct SMiniMapUnitInfo;
struct SAIBasicUpdate;
struct ICheckSumLog;
struct ICollisionsCollector;

typedef det_map<int, SMapObjectInfo::SLinkInfo> LinkInfo;

class CAILogic : public IAILogic
{
	OBJECT_NOCOPY_METHODS( CAILogic );

public:
	int operator&( IBinSaver &saver ) override;

private:
	;

	CDBPtr<NDb::SAIGameConsts> pConsts;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	bool bSuspended = {};
	bool bFirstTime = {};

	// мосты
	typedef std::list<std::list<CPtr<CBridgeSpan>>> Bridges;
	Bridges bridges;
	CAITimer timer;

	//
	std::list<CObj<CCommonUnit>> garbage;

	// скрипты
	CScripts scripts;
	bool bMissionLoaded = {};

	std::vector<SAIStartCommand> startCmds;
	std::vector<SBattlePosition> reservePositions;

	NTimer::STime nextCheckSumTime = {};
	NTimer::STime periodToCheckSum = {};
	// Diagnostic only, so deliberately not serialized: a save game reloaded
	// should take the dump again rather than remember that a previous process
	// already did.
	bool bAsyncDumpWritten = false;
	uLong checkSum = {};

	bool bSegment = {};
	bool bNetGameStarted = {};

	CPtr<ICheckSumLog> pCheckSumLog;

	// QUESTIONABLE determinism - iteration order is NOT DETERMINISTIC!
	typedef det_set<CDBPtr<SMechUnitRPGStats>> CAvailTrucks;
	CAvailTrucks availableTrucks;

	CPtr<IProgressHook> pProgress;

	CObj<CAIMap> pAIMap;
	CObj<ICollisionsCollector> pCollisionsCollector;
	CPtr<IAIScenarioTracker> pScenarioTracker;

	NTimer::STime timeLocalPlayerUnitCheck = {};
	bool bLocalPlayerUnitsPresent = {};

	NTimer::STime timeLastMiniMapUpdateUnits = {};
	bool bNeedNewGroupNumber = {};

	// проверить, не является ли object грузовиком, подцеплённым к сценарийной артиллерии
	// если да, ищет подходящий к артиллерии грузовик (в pNewStats) и возвращает true, если артиллерия не найдена - возвращает false
	// если это не является таким грузовиком, возвращает true
	bool
	CheckForScenarioTruck( const SMapObjectInfo &object, LinkInfo *linksInfo, const SMechUnitRPGStats **pNewStats ) const;

	// Loading
	// часть инициализации, общая для игры и редактора
	void CommonInit( const NDb::STerrain *pTerrainInfo );

	void LoadUnits( const struct NDb::SMapInfo *pMapInfo, LinkInfo *linksInfo );
	void UpdateUnitsOnBridges();
	void InitReservePositions();
	void InitStartCommands();
	void LaunchStartCommand( const SAIStartCommand &startCommand, CObjectBase **pUnitsBuffer, const int nSize );
	// bSend - whether to send checksum
	// bFullAI - count all AI data or only object placement
	void UpdateCheckSum( bool bSend );

	// Whether this run computes state checksums, and why. Named rather than
	// spelled out at the use site, because the conditions were a mix of things
	// that are not the question being asked: a lockstep net game is one reason
	// to want checksums, not the definition of wanting them, and "a replay is
	// playing" was being used to mean "do not send them to peers" while also
	// suppressing the computation.
	//
	// Recording. Computes them so they can be compared later, or sent to peers
	// in a net game. count_checksum stays the master switch and still defaults
	// off, so a game that did not compute them before does not start now; what
	// changes is that a single-player game honours it too.
	bool NeedWriteCheckSum() const;
	// Verifying. Computes them while a replay plays back so they can be checked
	// against what the replay recorded.
	bool NeedVerifyCheckSum() const;

	bool CanShowVisibilities() const;

	void LinkArtilleryWithTransport( class CArtillery *pArtillery, class CAITransportUnit *pTransport );

public:
	CAILogic();
	~CAILogic() override;

	int GetScriptID( class CUpdatableObj *pObj ) const
	{
		return scripts.GetScriptID( pObj );
	}

	IAIScenarioTracker *GetScenarioTracker()
	{
		return pScenarioTracker;
	}

	void WriteDetailedChecksumInfo( const char *szFileName ) override;

	void ToGarbage( class CCommonUnit *pUnit );
	void Suspend() override;
	void Resume() override;

	bool IsSuspended() const override
	{
		return bSuspended;
	}

	void SetProgressHook( IProgressHook *pProgress ) override;
	void Init( ICheckSumLog *_pCheckSumLog,
	           const struct NDb::SMapInfo *pMapInfo,
	           const NDb::SAIGameConsts *_pConsts,
	           IAIScenarioTracker *_pScenarioTracker ) override;
	void LogCheckSum( ICheckSumLog *pCheckSumLog ) override;
	void InitAfterMapLoad( const struct NDb::SMapInfo *pMapInfo /*, IProgressHook * pProgress = 0 */ ) override;
	void PostMapLoad() override;
	void ClearAI() override;

	void UnitCommand( SAIUnitCmd *pCommand, const uint16_t wGroupID, const int nPlayer ) override;
	void TransferUnits( const std::vector<int> &unitIDs, int nSourcePlayer, int nTargetPlayer ) override;

	const int GenerateGroupNumber() override;
	void RegisterGroup( const std::vector<int> &vIDs, const int nGroup ) override;
	virtual void RegisterGroup( CObjectBase **pUnitsBuffer, const int nLen, const uint16_t wGroup );
	void UnregisterGroup( const int nGroup ) override;
	void GroupCommand( SAIUnitCmd *pCommand, const uint16_t wGroup, bool bPlaceInQueue ) override;

	void ShowAreas( const std::vector<int> &units, enum EActionNotify &eType, bool bShow ) override;
	void GetShootAreas( int nUnitID, SShootAreas *pAreas ) override;

	const bool GetMiniMapUnitsInfo( std::vector<SMiniMapUnitInfo> &vUnits ) override;
	bool GetMiniMapWarForInfo( CArray2D<uint8_t> **pWarFogInfo, bool bFirstTime ) override;
	int GetMiniMapWarFogSizeX() const override;
	int GetMiniMapWarFogSizeY() const override;
	virtual void CallScriptFunction( const char *pszCommand );

	virtual int GetUniqueIDOfObject( CObjectBase *pObj );
	virtual CObjectBase *GetObjByUniqueID( const int id );

	void Segment() override;

	CObjectBase *AddObject( const int nUniqueID,
	                        const SMapObjectInfo &object,
	                        LinkInfo *linksInfo,
	                        bool bInitialization,
	                        const SHPObjectRPGStats *pPassedStats,
	                        NDb::EReinforcementType eType = _RT_NONE );

	void InitLinks( LinkInfo &linksInfo );
	void LoadEntrenchments( const std::vector<struct SEntrenchmentInfo> &entrenchments );
	void LoadBridges( const std::vector<NDb::SIntArray> &bridgesInfo );

	void SetMyDiplomacyInfo( const int nParty, const int nNumber ) override;
	void SetNPlayers( const int nPlayers ) override;
	void SetNetGame( const bool bNetGame ) override;

	bool UpdateAcknowledgment( SAIAcknowledgment &pAck ) override;
	bool UpdateAcknowledgment( SAIBoredAcknowledgement &pAck ) override;

	float GetZ( const CVec2 &vPoint ) const override;
	const uint32_t GetNormal( const CVec2 &vPoint ) const override;
	const bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const override;

	virtual bool ToggleShow( const int nShowType );

	bool IsCombatSituation() override;
	void InitStartCommands( const LinkInfo &linksInfo, det_map<int, int> &old2NewLinks );
	void InitReservePositions( det_map<int, int> &old2NewLinks );

	bool IsSegment() const
	{
		return bSegment;
	}

	// difficuly levels
	virtual void SetDifficultyLevel( const int nLevel );
	virtual void SetCheatDifficultyLevel( const int nCheatLevel );

	const bool IsFirstTime() const
	{
		return bFirstTime;
	}

	virtual void SendAcknowlegdementForced( CObjectBase *pObj, const EUnitAckType eAck );

	// for debug
	virtual int GetUniqueID( CObjectBase *pObj );

	// при игре в multiplayer: все игроки загрузились и игра стартовала
	void NetGameStarted() override;
	bool IsNetGameStarted() const override;

	virtual const class CDifficultyLevel *GetDifficultyLevel() const;

	void NeutralizePlayer( const int nPlayer ) override;

	virtual CObjectBase *GetUnitState( CObjectBase *pObj );
	virtual bool IsFrozen( CObjectBase *pObj ) const;
	virtual bool IsFrozenByState( CObjectBase *pObj ) const;

	virtual void GetGridUnitsCoordinates( const int nGroup, const CVec2 &vGridCenter, CVec2 **pCoord, int *pnLen );

	CObjectBase *GetUpdate() override;
	void PrepareUpdates() override;
	void ToggleWarFog( const bool bWarFog ) override;
	struct ITerraAIObserver *CreateTerraAIObserver( const int nSizeX, const int nSizeY ) override;

	void PickedObj( const int nObjID ) override;
	void PickEmpty() override;

	const int WAR_FOG_FULL_UPDATE() const override;
	const int VIS_POWER() const override;

	void RequestBuildPreview( const EActionCommand eBuildCommand,
	                          const CVec2 &vStart,
	                          const CVec2 &vEnd,
	                          bool bFinished = false ) override;

	bool IsMissionLoaded() const override
	{
		return bMissionLoaded;
	}

	void SetGlobeScriptHandler( IGlobeScriptHandler *pHandler ) override;

	class CAIMap *GetAIMap() const
	{
		return pAIMap;
	}
	class CStaticMapHeights *GetHeights() const override;

	const int GetPlayerDiplomacy( const int nPlayer ) const override;

	void SetNeedNewGroupNumber() override
	{
		bNeedNewGroupNumber = true;
	}

	const bool NeedNewGroupNumber() const override
	{
		return bNeedNewGroupNumber;
	}

	void ResetNeedNewGroupNumber() override
	{
		bNeedNewGroupNumber = false;
	}

	const NDb::SAIGameConsts *GetAIConsts() const override
	{
		return pConsts;
	}

	const int GetUnitCount( const int nPlayer ) const override;
	const bool HasPlayerNoUnits( const int nPlayer ) const override;
	void DumpAfterAssinc() const override;
};
