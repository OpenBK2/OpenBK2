#pragma once

#include "MPPlayerDrop.h"
#include "MultiplayerCommandManager.h"
#include "MultiplayerCommandProcessor.h"
#include "Net/NetAddress.h"
#include "Net/NetDriver.h"
#include "Server_Client_Common/PacketProcessor.h"

#include <array>
#include <cstdint>

struct IServerClient;
class CLANTester;

namespace NDb
{
struct SMultiplayerMap;
struct SMultiplayerConsts;
} // namespace NDb
class CMPTransceiver;
class CCommandsHistory;

// General handler for messages and packets
// Particular types (LAN, NivalNet) inherit from it

struct CMPManagerMode : public CMPUIMessageProcessor, public CPacketProcessorBase, public CObjectBase
{
	// The headless integration fixture restores only the state normally set by the lobby.
	friend class CMPPlayerDropTestAccess;

public:
	enum ENetMode
	{
		ENM_NONE,
		ENM_LAN,
		ENM_NIVAL_NET,
	};

protected:
	struct SAutoUpdate
	{
		bool bUpdating;
		NTimer::STime timeNextUpdate;
		NTimer::STime timeUpdatePeriod;
		uint32_t dwVersion;

		SAutoUpdate()
			: bUpdating( false )
			, timeNextUpdate( 0 )
			, timeUpdatePeriod( 1000 )
			, dwVersion( 0 )
		{
		}

		bool CheckNeedUpdate();

		int operator&( IBinSaver &saver )
		{
			NI_ASSERT( 0, "should not be serialized" );
			return 0;
		}
	};

	struct SNetGameInfo
	{
		int nGameID;
		NNet::CNodeAddress lanNode;
		NTimer::STime lastSeen;
		std::string szSessionName;
		std::string szMapName;
		int nPlayers;
		int nMaxPlayers;
		float fPing;
		int nSizeX;
		int nSizeY;
		int nGameType;
		int nTechLevel;
		bool bPasswordRequired;
	};

	typedef std::list<SNetGameInfo> CNetGameList;

	struct SLagInfo
	{
		NTimer::STime timeStartLag; // Start of last lag
		int nLagLeft;               // Remaining lag time
		uint32_t dwHatedBy;         // They want him kicked already
		NTimer::STime timeLastKickVote = 0;
		// Votes are renewable observations, not permanent approvals. The receipt
		// times use the local clock and one entry for each bit of dwHatedBy.
		std::array<NTimer::STime, 32> timeKickVotes = {};

		SLagInfo()
			: timeStartLag( 0 )
			, nLagLeft( 0 )
			, dwHatedBy( 0 )
		{
		}

		void ResetKickVotes()
		{
			dwHatedBy = 0;
			timeLastKickVote = 0;
			timeKickVotes.fill( 0 );
		}
	};

	typedef std::vector<SLagInfo> CLaggerList;

	// General MP state
	std::string szMPName;
	bool bGameRoomInit;

	CObj<IServerClient> pClient;
	CObj<CMPTransceiver> pTransceiver;
	CObj<CCommandsHistory> pCommandsHistory;
	CDBPtr<NDb::SMultiplayerConsts> pMPConsts;

	// Game state
	int nGameID;
	std::string szSessionName;
	std::string szPassword;
	SB2GameSpecificData gameDesc;
	uint32_t ulHostCheckSum;
	uint32_t ulGameCheckSum;
	std::vector<SMPSlot> slots;
	int nHostClientID;
	int nOwnSlot;
	int nSlotsUsed;
	uint32_t dwNetworkLaggers = 0; // Receipt waits reported after the ordinary network timeout
	uint32_t dwLaggers;            // Effective union of receipt waits and explicit pauses
	uint32_t dwLaggersOld;         // Union whose elapsed time is currently being accounted
	uint32_t dwUserPausedPlayers;  // Players that explicitly toggled MP pause
	uint32_t dwInitialPlayers;
	bool bInitialLoadInProgress;
	bool bWaitWindowShown;
	CLaggerList lags;
	SAutoUpdate lagsUpdate;
	std::vector<NMPPlayerDrop::SPlayerDrop> playerDrops;

	std::list<int> pendingClients; // NEW_CLIENT-s that are received when we do not have our slot number yet

	// (Custom) Games List
	SAutoUpdate gamesUpdate;
	CNetGameList games;

	// In-game state
	NTimer::STime timeEndMatch;
	int nWinningSide;
	NTimer::STime timeResolution;
	NTimer::STime timeNextInstaLoseCheck;
	bool bOutcomeKnown;
	bool bWinOnGameEnd;
	bool bWinFromPlayerDrops = false;
	SAutoUpdate pingUpdate;

	CObj<CLANTester> pLanTester;

private:
	//{ Messages
	bool OnCreateGameMessage( SMPUICreateGameMessage *pMsg );

	virtual void TryToCreateGame() {}

	bool OnJoinGameMessage( SMPUIJoinGameMessage *pMsg );

	virtual void TryToJoinGame( const SNetGameInfo &game ) {}

	bool OnLeaveGameMessage( SMPUIMessage *pMsg );
	bool OnInterruptMessage( SMPUIMessage *pMsg );
	bool OnStartGameMessage( SMPUIMessage *pMsg );
	bool OnUpdateGameListMessage( SMPUIGameListMessage *pMsg );
	bool OnUpdateSlotMessage( SMPUIUpdateSlotMessage *pMsg );
	bool OnLagMessage( SMPUILagMessage *pMsg );
	bool OnPauseMessage( SMPUIMessage *pMsg );
	void SetUserPause( bool bPauseOn );
	bool OnInGameChatMessage( SMPUIInGameChatMessage *pMsg );
	//}

	//{ Packets
	bool OnGameClientRemoved( class CGameClientRemoved *pPacket );
	bool OnGameKilled( class CGameKilled *pPacket );
	bool OnConnectGameFailedPacket( class CConnectGameFailed *pPacket );
	bool OnB2SlotInfoPacket( class CB2SlotInfoPacket *pPacket );
	bool OnB2GameRoomStartGamePacket( class CB2GameRoomStartGamePacket *pPacket );
	bool OnB2GameSpecificInfoPacket( class CB2GameSpecificInfoPacket *pPacket );
	bool OnChatPacket( class CChatPacket *pPacket );
	bool OnSlotNumberPacket( class CSlotNumberPacket *pPacket );
	bool OnB2SuggestKickPacket( class CB2SuggestKickPacket *pPacket );
	bool OnB2LagTimeUpdatePacket( class CB2LagTimeUpdatePacket *pPacket );
	bool OnB2UserPausePacket( class CB2UserPausePacket *pPacket );
	bool OnB2DropPlayerAtSegmentPacket( class CB2DropPlayerAtSegmentPacket *pPacket );
	bool OnB2GameLostPacket( class CB2GameLostPacket *pPacket );
	bool OnPingPacket( class CPingPacket *pPacket );
	//}

protected:
	virtual void StartGame();
	void CheckEndGameConditions();
	bool CheckAllLeftWin();
	bool CheckScoreWinLose();
	bool CheckKeyBuildingsWinLose();
	bool CheckInstantLose();
	int GetTeamWithLowestScore();
	void ShowWaitWindow( bool bShow );
	void SendLagInfo();
	void AnalyzeLaggers();
	void UpdateLagState( bool bRefreshUI = false );
	int GetRemainingLagTime( int nPlayer, NTimer::STime curTime ) const;
	void AccountLagTime( int nPlayer, NTimer::STime curTime );
	void PublishLagBudget( int nPlayer );
	int GetLagReserveMilliseconds() const;
	uint32_t GetPresentMask() const;
	bool IsPlayerPresent( int nPlayer );
	bool IsPlayerLagging( int nPlayer );
	int GetSlotByClientID( int nClientID ) const;
	int GetSlotClientID( int nSlot );
	bool ScheduleSynchronizedPlayerDrop( int nSlot, int nSegment );
	void ApplyScheduledSlotDrops();
	void ResolvePendingHostDrops();
	bool IsGameControlHost();
	int GetReplacementHostClientID( int nRemovedClientID );
	void PromoteGameControlHostAfterRemoval( int nRemovedClientID );

	virtual void OnGameControlHostChanged( int nOldHostClientID, int nNewHostClientID, int nRemovedClientID ) {}

	bool IsAuthoritativeDropPacket( const class CB2DropPlayerAtSegmentPacket *pPacket );
	virtual void BroadcastGameControlPacket( CNetPacket *pPacket );
	void BroadcastSynchronizedPlayerDrop( int nSlot, int nSegment, const char *szReason );
	void SendStartGamePacket();
	void AddGameInfoForUI( std::list<SUIGameInfo> *pList, const SNetGameInfo &game );
	void UpdateMyConnectivityMask();

	virtual void CheckJoinGameConditions() {}

	void WinGame();
	void LoseGame();
	void ScheduleWinGame();
	void ScheduleLoseGame();
	virtual void EndGame();
	void AbortRunningGame();

	// Called by AbortRunningGame once the match is released: the mode leaves its
	// "game started" state without the network traffic its EndGame would send.
	virtual void OnGameAborted() {}

	virtual const int GetOwnClientID() = 0;
	virtual const bool IsInGameRoom() const = 0;
	virtual const bool IsGameRunning() const = 0;
	virtual const bool IsGameHost() const = 0;

	void SendGameRoomInitMessages();
	bool HostAssignNewClient( int nClientID ); // return true if successfully added to slots
	void ClientAssignNewClient( int nClientID );
	void JoiningClientAssignNewClient( int nClientID );
	void RemoveClient( int nClientID, bool bKicked );

	void CreateRehash( std::vector<uint8_t> *pOrder );
	void RehashSlots( const std::vector<uint8_t> &order );

	virtual void UpdateGameList() = 0;
	virtual void OnLeaveGame() = 0;
	virtual void OnGameRoomClientAdded() = 0;
	virtual void OnGameRoomClientRemoved() = 0;
	virtual void OnSetMySlotNumber() = 0;
	virtual void OnGameSpecificInfo() = 0;
	virtual void KickPlayerFromSlot( const int nSlot ) = 0;

	virtual void OnSurrender() {}

	void OnCriticalNetworkError( const std::wstring &wszMessage );

	uint32_t GetCheckSum( const NDb::SMultiplayerMap *pMPMap );

public:
	CMPManagerMode();
	~CMPManagerMode() override;
	virtual const ENetMode GetMode() const = 0;

	//{ CPacketProcessor
	bool Segment() override;
	//}

	const bool SaveReplay( const std::string &szFileName );
	//void CMPManagerMode::UpdateInfoForReplay( SMultiplayerReplayInfo *pReplayInfo );
	virtual void SetLanTester( class CLANTester *_pLANTester ) = 0;
};

class CSlotNumberPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CSlotNumberPacket );

public:
	ZDATA
	int nSlot;
	uint32_t ulCheckSum;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &nSlot );
		f.Add( 3, &ulCheckSum );
		return 0;
	}

	CSlotNumberPacket() {}

	CSlotNumberPacket( const int nClientID, const int _nSlot, const uint32_t _ulCheckSum )
		: CNetPacket( nClientID )
		, nSlot( _nSlot )
		, ulCheckSum( _ulCheckSum )
	{
	}
};

class CB2GameLostPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CB2GameLostPacket );

public:
	ZDATA
	int nGameID;
	int nSegment;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &nGameID );
		f.Add( 3, &nSegment );
		return 0;
	}

	CB2GameLostPacket() {}

	CB2GameLostPacket( const int nClientID, const int _nGameID, const int _nSegment )
		: CNetPacket( nClientID )
		, nGameID( _nGameID )
		, nSegment( _nSegment )
	{
	}
};
