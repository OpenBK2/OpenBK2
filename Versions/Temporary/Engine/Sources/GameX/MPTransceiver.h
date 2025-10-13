#pragma once

#include "Transceiver.h"
#include "Misc/2Darray.h"
#include "Server_Client_Common/PacketProcessor.h"
#include "MultiplayerNetPackets.h"

#include <cstdint>

struct ICommandsHistory;
struct IServerClient;
struct IMPAICommandPacker;
class CNetPacket;
namespace NDb
{
	struct SMapInfo;
}

//#define CHECKSUM_LIST_DEBUG

struct SB2StartGameParams
{
	struct SClient
	{
		ZDATA
		int nClientID;
		int nPlayer; // logic player
		int nTeam; // 0 or 1
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nClientID); f.Add(3,&nPlayer); f.Add(4,&nTeam); return 0; }
	};

	ZDATA
	std::vector< SClient > clients;
	CDBPtr<NDb::SMapInfo> pMapInfo;
	int nGameID;
	int nSpeedAdjustment;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&clients); f.Add(3,&pMapInfo); f.Add(4,&nGameID); f.Add(5,&nSpeedAdjustment); return 0; }
};

class CMPTransceiver : public ITransceiver, public CPacketProcessorBase
{
	OBJECT_BASIC_METHODS( CMPTransceiver );

	CPtr<IServerClient> pClient;
	CPtr<IAICmdsAutoMagic> pCmdsSerializer;
	CPtr<ICommandsHistory> pCmdsHistory;
	bool bIsGameRunning;
	bool bIsGameEnded;

#ifdef CHECKSUM_LIST_DEBUG
	std::vector<int> myHistoryHashes;
#endif

	int nLatency;
	int nSegmentsPackSize;

	int nSegment;													// номер текущего сегмента
	long nCommonSegment;
	int nMyLogicID;												// номер "нашего" игрока
	bool bCommandsFromHistory;
	std::vector<uint16_t> segmFinished;
	uint16_t wMask;
	uint16_t wWaitMask;
	typedef std::list< CPtr<IAILogicCommandB2> > CAILogicCommandsList;		// команды для каждого игрока ( [i][j] i - номер сегмента, j - номер игрока )
	CArray2D<CAILogicCommandsList> cmds;

	CDBPtr<NDb::SNetGameConsts> pConsts;
	CPtr<IAILogic> pAI;
	CPtr<IGameTimer> pTimer;

public:
	struct SRawCommand
	{
		ZDATA
		int nTypeID;
		CMemoryStream cmd;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nTypeID); f.Add(3,&cmd); return 0; }
	};
private:
	std::list< CPtr<IAILogicCommandB2> > aiCommandsToSend;

	struct SPlayer
	{
		int nClientID;
		int nTeam;
		bool bLoaded;
		SPlayer() : nClientID( -1 ), bLoaded( false ) {}
	};
	typedef std::vector<SPlayer> CPlayersList;
	CPlayersList players;

	CDBPtr<NDb::SMapInfo> pMapInfo;
	int nGameID;
	bool bWaiting;
	NTimer::STime timeStartWaiting;

	CArray2D<unsigned long> checkSums;
	int nGameSpeed;

	int nFinalSegment;
	std::vector<int> playerRemovalSegments;
private:
	bool Segment() { return false; }
	bool IsSegmentPackFinished();
	bool IsSegmentFinishedByAll( int nSegment );
	void SetSegmentFinished( int nPlayer, int nSegment, unsigned long ulCheckSum );
	void FinalizeSegmentPack();
	void AdvanceToNextSegment();
	void ExecuteCommands( int nFromSegment );
	bool IsAsyncDetected( int nSegment );
	void SetLagState( int nSegment, bool bOn );

	void ReportAsnycToFile(int segment);

	bool OnTransciverCommonPacket( class CTransciverCommonPacket *pPacket );
	bool OnAISegmentFinishedPacket( class CAISegmentFinishedPacket *pPacket );

	int GetPlayerByClient( int nClient ) const;

	void CheckRunGame();
	void LeaveOutOfSync();
	void ReportLags( uint32_t dwLaggers, bool bInitial );
	bool IsPlayerPresent( int nPlayer ) const;
	bool IsPlayerWaitedFor( int nPlayer ) const;
	int GetSegmentToExecute( const int nSegment ) const;
	void StopWaitingForPlayer( int nPlayer, int nDropSegment );
	void ApplyScheduledPlayerRemovals();
public:
	CMPTransceiver();
	void StartMission( const NDb::SMapInfo *pMap, IAILogic *pAI ) {}
	void StartMission();
	void DoSegments();
	void SendCommand( struct IAILogicCommandB2 *pCommand );
	ICheckSumLog *GetCheckSumLogger();
	const NDb::SMapInfo *GetMap() const;
	void EndGame();
	void PlayerRemoved( int nPlayer );
	void SchedulePlayerRemoval( int nPlayer, int nSegment );
	static const bool IsGamePacket( const CNetPacket *pPacket );
	int operator&( IBinSaver &f ) { return 0; }
	void Init( IServerClient *_pClient, const SB2StartGameParams &params, int nMySlot );

	// client commands
	void CommandClientTogglePause() {}
	void CommandClientSpeed( int nChange ) {}
	void CommandClientDropPlayer( const std::wstring &szPlayerNick ) {}
	void CommandTimeOut( bool bSet );

	bool IsGameEnded() { return bIsGameEnded; }
	bool IsGameRunning() const;
	int GetCurrentCommonSegment() const { return nCommonSegment; }
	unsigned long GetPlayerMask() const { return wWaitMask; }

	int ScheduleGameEnd( const int _nSegment );
	ICommandsHistory *GetCommandsHistory() { return pCmdsHistory; }
};



CMPTransceiver* CreateMPTransceiver( IServerClient *pClient, 
																				const SB2StartGameParams &params, int nMySlot );

