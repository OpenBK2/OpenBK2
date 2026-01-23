#pragma once
#include "MultiplayerCommandManager.h"
#include "MultiplayerCommandProcessor.h"
#include "Server_Client_Common/PacketProcessor.h"
#include "Net/NetDriver.h"
#include "Net/NetAddress.h"


struct IServerClient;
class CLANTester;
namespace NDb
{
	struct SMultiplayerMap;
	struct SMultiplayerConsts;
}
class CMPTransceiver;
class CCommandsHistory;

// General handler for messages and packets
// Particular types (LAN, NivalNet) inherit from it

struct CMPManagerMode : public CMPUIMessageProcessor, public CPacketProcessorBase, public CObjectBase
{
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
		bool					bUpdating;
		NTimer::STime timeNextUpdate;
		NTimer::STime timeUpdatePeriod;
		DWORD					dwVersion;
		SAutoUpdate() : bUpdating(false), timeNextUpdate( 0 ), timeUpdatePeriod( 1000 ), dwVersion( 0 ) {}
		bool CheckNeedUpdate();
		int operator&( IBinSaver &saver ) { NI_ASSERT( 0, "should not be serialized" ); return 0; }
	};

	struct SNetGameInfo
	{
		int									nGameID;
		NNet::CNodeAddress	lanNode;
		NTimer::STime				lastSeen;
		std::string							szSessionName;
		std::string							szMapName;
		int									nPlayers;
		int									nMaxPlayers;
		float								fPing;
		int									nSizeX;
		int									nSizeY;
		int									nGameType;
		int									nTechLevel;
		bool								bPasswordRequired;
	};
	typedef std::list<SNetGameInfo> CNetGameList;

	struct SLagInfo
	{
		NTimer::STime timeStartLag;		// Start of last lag
		int nLagLeft;									// Remaining lag time
		DWORD dwHatedBy;							// They want him kicked already
		SLagInfo() : timeStartLag(0), nLagLeft(0), dwHatedBy(0) {}
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
	unsigned long ulHostCheckSum;
	unsigned long ulGameCheckSum;
	std::vector<SMPSlot> slots;
	int nHostClientID;
	int nOwnSlot;
	int nSlotsUsed;
	DWORD dwLaggers;								// Waiting for these players
	DWORD dwLaggersOld;							// same, previous state, used to find changes in AnalyzeLag();
	DWORD dwInitialPlayers;
	bool bInitialLoadInProgress;
	bool bWaitWindowShown;
	CLaggerList lags;
	SAutoUpdate lagsUpdate;

	std::list<int> pendingClients;				// NEW_CLIENT-s that are received when we do not have our slot number yet

	// (Custom) Games List
	SAutoUpdate						gamesUpdate;
	CNetGameList					games;

	// In-game state
	NTimer::STime timeEndMatch;
	int nWinningSide;
	NTimer::STime timeResolution;
	NTimer::STime timeNextInstaLoseCheck;
	bool bOutcomeKnown;
	bool bWinOnGameEnd;
	SAutoUpdate pingUpdate;

	CObj<CLANTester> pLanTester;

	#define MAX_ALLOWED_TICKS_TO_JOIN 3
	int joiningHeartbeatTicksLeft = MAX_ALLOWED_TICKS_TO_JOIN;	// For skipping the deadlock in case of missing MP map on the client, since only the interrupt from the client itself could end it, which was not good!
	
	bool lastAllowedTickTriggered = false;
	#define CONNECTING_TIMEOUT_TIME (16 * 1000)		// * 1000 because it's in MS
	float connectingTimeoutTimer = CONNECTING_TIMEOUT_TIME;
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
	bool IsPlayerPresent( int nPlayer );
	bool IsPlayerLagging( int nPlayer );
	bool HasPlayerStoppedLagging( int nPlayer );
	bool HasPlayerStartedLagging( int nPlayer );
	void SendStartGamePacket();
	void AddGameInfoForUI( std::list<SUIGameInfo> *pList, const SNetGameInfo &game );
	void UpdateMyConnectivityMask();
	virtual void CheckJoinGameConditions() {}
	
	void WinGame();
	void LoseGame();
	void ScheduleLoseGame();
	virtual void EndGame();

	virtual const int GetOwnClientID() = 0;
	virtual const bool IsInGameRoom() const = 0;
	virtual const bool IsGameRunning() const = 0;
	virtual const bool IsGameHost() const = 0;

	void SendGameRoomInitMessages();
	bool HostAssignNewClient( int nClientID );	// return true if successfully added to slots
	void ClientAssignNewClient( int nClientID );
	void JoiningClientAssignNewClient( int nClientID );
	void RemoveClient( int nClientID, bool bKicked );

	void CreateRehash( std::vector<BYTE> *pOrder );
	void RehashSlots( const std::vector<BYTE> &order );

	virtual void UpdateGameList() = 0;
	virtual void OnLeaveGame() = 0;
	virtual void OnGameRoomClientAdded() = 0;
	virtual void OnGameRoomClientRemoved() = 0;
	virtual void OnSetMySlotNumber() = 0;
	virtual void OnGameSpecificInfo() = 0;
	virtual void KickPlayerFromSlot( const int nSlot ) = 0;
	virtual void OnSurrender() {}

	void OnCriticalNetworkError( const std::wstring &wszMessage );

	unsigned long GetCheckSum( const NDb::SMultiplayerMap *pMPMap );
public:
	CMPManagerMode();
	~CMPManagerMode();
	virtual const ENetMode GetMode() const = 0;

	//{ CPacketProcessor
	bool Segment();
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
	DWORD ulCheckSum;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nSlot); f.Add(3,&ulCheckSum); return 0; }

	CSlotNumberPacket() { }
	CSlotNumberPacket( const int nClientID, const int _nSlot, const unsigned long _ulCheckSum )
		: CNetPacket( nClientID ), nSlot( _nSlot ), ulCheckSum( _ulCheckSum ) { }
};

class CB2GameLostPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CB2GameLostPacket );
public:
	ZDATA
	int nGameID;
	int nSegment;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); f.Add(3,&nSegment); return 0; }

	CB2GameLostPacket() { }
	CB2GameLostPacket( const int nClientID, const int _nGameID, const int _nSegment )
		: CNetPacket( nClientID ), nGameID( _nGameID ), nSegment( _nSegment ) { }
};

