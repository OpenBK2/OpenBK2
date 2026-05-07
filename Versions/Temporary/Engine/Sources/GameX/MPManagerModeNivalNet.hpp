#pragma once
#include "MPManager.h"
#include "DBMPConsts.h"
#include "Server_Client_Common/LadderStatistics.h"

class CMPManagerModeNivalNet : public CMPManagerMode
{
	OBJECT_NOCOPY_METHODS( CMPManagerModeNivalNet );

	enum EGameState
	{
		EGS_LOGGING_IN,
		EGS_LOBBY,
		EGS_CUSTOM_GAME,
		EGS_LOBBY_WAIT_LADDER,
		EGS_LADDER_WAIT_CLIENTS,
		EGS_JOINING,
		EGS_GAME_ROOM,
		EGS_GAME_STARTED,
	};

	EGameState eState;
	int nMyClientID;
	bool bHost;
	NTimer::STime timeNextGameHeartbeat;
	SAutoUpdate updateChannels;
	std::unordered_set<std::string> chatChannels;
	int nLadderCountry;
	bool bLadderGame;
	bool bHistoricity;

	SLadderStatistics myLadderStats;
	bool bGotMyLadderStats;
	bool bUIAskedStats;
	bool bAwaitingLadderStatsChange;
	NTimer::STime timeOutLadderGame;

	virtual const int GetOwnClientID() { return nMyClientID; }
	virtual const bool IsInGameRoom() const { return ( eState == EGS_GAME_ROOM || eState == EGS_JOINING || eState == EGS_LADDER_WAIT_CLIENTS ); }
	virtual const bool IsGameRunning() const { return ( eState == EGS_GAME_STARTED ); }
	virtual const bool IsGameHost() const { return ( eState == EGS_GAME_ROOM && bHost ); }

	virtual void UpdateGameList();
	virtual void TryToCreateGame();
	virtual void TryToJoinGame( const SNetGameInfo &game );
	virtual void OnSetMySlotNumber();
	virtual void OnLeaveGame();
	virtual void OnGameRoomClientAdded() {}
	virtual void OnGameRoomClientRemoved() {}
	virtual void OnGameSpecificInfo();
	virtual void StartGame();
	virtual void EndGame();
	virtual void KickPlayerFromSlot( const int nSlot );
	virtual void OnSurrender();
	virtual void OnGameControlHostChanged( int nOldHostClientID, int nNewHostClientID, int nRemovedClientID );

	void CheckJoinGameConditions();
	void SetServerGameInfo( struct SGameInfo *pInfo );
	bool ShouldSendHeartbeatNow();
	void RefreshServerGameKeepalive( bool bUpdateGameInfo, const char *szReason, int nRemovedClientID );
	void InitLobby();
	void RequestChatChannels( DWORD dwVersion );
	void CreateServerClient();
	void SendGameStatistics();
	void ProcessLadderWaitForClients();
	int DetermineLadderCountry( const int nInitial, const NDb::EHistoricalSide eSide );
	void SendLadderGameAftermath( const SLadderStatistics &oldStats, const SLadderStatistics &newStats );
	const int GetRankFromLevel( const int nRace, const int nLevel, std::wstring *pwszOut );
	void ExtractGameInfo( SNetGameInfo *pDst, const struct SGameInfo &src );

	//{ Messages
	bool OnLoginNivalNetMessage( SMPUILoginNivalNetMessage *pMsg );
	bool OnRegisterNivalNetMessage( SMPUIRegisterMessage *pMsg );
	bool OnChatMessage( SMPUIChatMessage *pMsg );
	bool OnCustomGameMessage( SMPUIMessage *pMsg );
	bool OnCancelLadderMessage( SMPUIMessage *pMsg );
	bool OnSetupLadderGameMessage( SMPUILadderGameMessage *pMsg );
	bool OnBackFromGameListMessage( SMPUIMessage *pMsg );
	bool OnJoinChatChannelMessage( SMPUIJoinChannelMessage *pMsg );
	bool OnRequestChatChannelsMessage( SMPUIMessage *pMsg );
	bool OnChangeFriendIgnoreStatusMessage( SMPUIChangeFriendIgnoreStatusMessage *pMsg );
	bool OnLadderInfoRequestMessage( SMPUILadderInfoRequestMessage *pMsg );
	//}

	//{ Packets
	bool OnConnectServerPacket( class CConnectServerPacket *pPacket );
	bool OnSystemBroadcastPacket( class CSystemBroadcastPacket *pPacket );
	bool OnMyIDPacket( class CMyIDPacket *pPacket );
	bool OnLobbyGamesPacket( class CLobbyGamesPacket *pPacket );
	bool OnConnectGameFailed( class CConnectGameFailed *pPacket );
	bool OnConnectedGameIDPacket( class CConnectedGameID *pPacket );
	bool OnNewGameClientPacket( class CNewGameClient *pPacket );
	bool OnEnterLobbyPacket( class CEnterLobbyPacket *pPacket );
	bool OnChatChannelsListPacket( class CChatChannelsListPacket *pPacket );
	bool OnChatChannelClientsPacket( class CChatChannelClientsListPacket *pPacket );
	bool OnChatIgnoreFriendListPacket( class CChatIgnoreFriendListPacket *pPacket );
	bool OnChatFriendNotifyPacket( class CChatFriendNotifyPacket *pPacket );
	bool OnChatAFKResponsePacket( class CChatAFKResponsePacket *pPacket );
	bool OnChatChannelClientNotifyPacket( class CChatClientListChangeNotifyPacket *pPacket );
	bool OnGameClientWasKicked( class CGameClientWasKicked *pPacket );
	bool OnGameClientDeadPacket( class CGameClientDead *pPacket );
	bool OnNetRemoveClientPacket( class CNetRemoveClient *pPacket );
	bool OnLadderStatisticsPacket( class CLadderStatisticsPacket *pPacket );
	bool OnLadderShortStatisticsPacket( class CLadderShortStatisticsPacket *pPacket );
	bool OnLadderInvitePacket( class CLadderInvitePacket *pPacket );
	bool OnLadderInvalidStatisticsPacket( class CLadderInvalidStatisticsPacket *pPacket );
	//}
public:
	CMPManagerModeNivalNet();
	bool Segment();

	virtual const ENetMode GetMode() const { return ENM_NIVAL_NET; }
	virtual void SetLanTester( class CLANTester *_pLANTester ) { NI_ASSERT( 0, "Wrong call" ) }
};

