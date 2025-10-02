#pragma once

#include "../Server_Client_Common/GameInfo.h"
#include "../Server_Client_Common/CustomLobbyClientInfo.h"
#include "../Server_Client_Common/PacketProcessor.h"

class CTestClientProcessor : public CPacketProcessor
{
	OBJECT_NOCOPY_METHODS( CTestClientProcessor )

	CPtr<class CServerClient> pServerClient;
	int nMyID;

	DWORD dwClientsVersion;
	hash_map<int, SCustomLobbyClientInfo> lobbyClients;
	
	int nGameID;
	DWORD dwLastGameUpdate;
	DWORD dwGamesVersion;
	DWORD dwHeartBeatPeriod;
	hash_map<int, SGameInfo> lobbyGames;

	hash_set<int> gameClients;
public:
	CTestClientProcessor() { }
	CTestClientProcessor( const string &szCfgFile );
	void SetServerClient( CServerClient *pServerClient );

	virtual bool Segment();

	int GetMyID() const { return nMyID; }
	// processors
	bool ProcessConnectServerResult( class CConnectServerPacket *pPacket );
	//bool ProcessLobbyChatPacket( class CReceiveLobbyChatPacket *pPacket );
	//bool ProcessPrivateChatPacket( class CPrivateChatPacket *pPacket );
	bool ProcessChatPacket( class CChatPacket * pPacket );
	bool ProcessMyIDPacket( class CMyIDPacket *pPacket );
	bool ProcessRemoveClient( class CNetRemoveClient *pPacket );
	bool ProcessEnteredLobby( class CEnteredLobby *pPacket );
	bool ProcessGetLobbyClientsPacket( class CGetLobbyClientsPacket *pPacket );
	bool ProcessCustomLobbyClients( class CCustomLobbyClientsPacket *pPacket );
	bool ProcessCreateGame( class CCreateGamePacket *pPacket );
	bool ProcessKillGame( class CKillGamePacket *pPacket );
	bool ProcessUpdateGameInfo( class CUpdateGameInfo *pPacket );
	bool ProcessGetLobbyGamesPacket( class CGetLobbyGamesPacket *pPacket );
	bool ProcessCustomLobbyGamesPacket( class CLobbyGamesPacket *pPacket );
	bool ProcessLeaveGame( class CLeaveGamePacket *pPacket );
	bool ProcessConnectGameFailed( class CConnectGameFailed *pPacket );
	bool ProcessGameClientRemoved( class CGameClientRemoved *pPacket );
	bool ProcessNewGameClient( class CNewGameClient *pPacket );
	bool ProcessGameKilled( class CGameKilled *pPacket );
	bool ProcessConnectGame( class CConnectGamePacket *pPacket );
	bool ProcessGameTestBroadcastMsg( class CGameTestBroadcastMsg *pPacket );
	bool ProcessGameTestDirectMsg( class CGameTestDirectMsg *pPacket );
	bool ProcessConnectedGameID( class CConnectedGameID *pPacket );
	bool ProcessClientWasKicked ( class CGameClientWasKicked *pPacket );
	bool ProcessGameKickClient( class CGameKickClient *pPacket );
	bool ProcessTestDirectPacket( class CTestDirectPacket *pPacket );
	bool ProcessShowGameClients( class CTestShowGameClients *pPacket );
	bool ProcessSendSpecGameInfo( class CTestSpecGameInfo *pPacket );
	bool ProcessReceiveSpecificGameInfo( class CSpecificGameInfo *pPacket );
	
	bool ProcessLadderInvitePacket( class CLadderInvitePacket *pPacket );
	bool ProcessLadderStatisticsPacket( class CLadderStatisticsPacket *pPacket ); 
	bool ProcessForgottenPasswordAnswerPacket( class CForgottenPasswordAnswerPacket *pPacket );

	bool ProcessChatChannelClientsListPacket( class CChatChannelClientsListPacket *pPacket );
	bool ProcessChatClientListChangeNotifyPacket( class CChatClientListChangeNotifyPacket *pPacket );
	bool ProcessChatChannelsListPacket( class CChatChannelsListPacket *pPacket );
	bool ProcessChatIgnoreListPacket( class CChatIgnoreFriendListPacket *pPacket );
	bool ProcessChatChannelByNickPacket( class CChatChannelByNickPacket *pPacket );
	bool ProcessChatFriendNotifyPacket( class CChatFriendNotifyPacket *pPacket );

	bool ProcessWelcomePacket( class CSystemBroadcastPacket *pPacket );
	bool ProcessPingPacket( class CPingPacket *pPacket );
};

