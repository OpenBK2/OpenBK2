#pragma once

#include "DBMPConsts.h"
#include "MPManager.h"
#include "Server_Client_Common/LadderStatistics.h"

#include <cstdint>

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

	const int GetOwnClientID() override
	{
		return nMyClientID;
	}

	const bool IsInGameRoom() const override
	{
		return ( eState == EGS_GAME_ROOM || eState == EGS_JOINING || eState == EGS_LADDER_WAIT_CLIENTS );
	}

	const bool IsGameRunning() const override
	{
		return ( eState == EGS_GAME_STARTED );
	}

	const bool IsGameHost() const override
	{
		return ( eState == EGS_GAME_ROOM && bHost );
	}

	void UpdateGameList() override;
	void TryToCreateGame() override;
	void TryToJoinGame( const SNetGameInfo &game ) override;
	void OnSetMySlotNumber() override;
	void OnLeaveGame() override;

	void OnGameRoomClientAdded() override {}

	void OnGameRoomClientRemoved() override {}

	void OnGameSpecificInfo() override;
	void StartGame() override;
	void EndGame() override;
	void KickPlayerFromSlot( const int nSlot ) override;
	void OnSurrender() override;
	void BroadcastGameControlPacket( CNetPacket *pPacket ) override;
	void OnGameControlHostChanged( int nOldHostClientID, int nNewHostClientID, int nRemovedClientID ) override;

	void CheckJoinGameConditions() override;
	void SetServerGameInfo( struct SGameInfo *pInfo );
	bool ShouldSendHeartbeatNow();
	void RefreshServerGameKeepalive( bool bUpdateGameInfo, const char *szReason, int nRemovedClientID );
	void InitLobby();
	void RequestChatChannels( uint32_t dwVersion );
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
	bool Segment() override;

	const ENetMode GetMode() const override
	{
		return ENM_NIVAL_NET;
	}

	void SetLanTester( class CLANTester *_pLANTester ) override
	{
		NI_ASSERT( 0, "Wrong call" )
	}
};
