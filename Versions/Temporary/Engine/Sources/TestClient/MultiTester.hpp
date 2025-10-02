#pragma once

#include "MultiTester.h"
#include "../Server_Client_Common/PacketProcessor.h"

class CServerClient;

class CMultiTester : public CPacketProcessor, public IMultiTester
{
	OBJECT_NOCOPY_METHODS( CMultiTester )
private:
	CObj<CServerClient> pServerClient;
	string szServerAddress;
	int nServerPort;
	int nNetVersion;
	int nTimeOut;
	string szName;
	string szPassword;
	string szCDKey;
	int nTestMode;
	int nStage;
	int nStatus;
	bool bCancelled;
	int nMyID;

	DWORD dwLastChatChannelChangeTime;
	DWORD dwLastChatMessageTime;
	DWORD dwLastSegmentTime;
	DWORD dwGameStartTime;
	DWORD dwLastGameHeartBeatTime;

	bool bLadderInfoSend;
	bool bIsInGame;

	hash_set<int> playersToWait;
	int nGameID;
	hash_set<int> winnersSet;
	hash_map<int,int> playersRaces;

	void LoginStage();
	void EnterLobbyStage();
	void EnterChatStage();
	void MainStage();

	void TestChat();
	void TestLadder();

	void ChangeChatChannel( const string &szChannelName );
	void SendChatMessage( const string &szMessage );

	void WaitForResponse();
	void Activate();

public:
	CMultiTester();

	void Init( const string& _szServerAddress, const int _nNetVersion, const int _nServerPort, const int _nTimeOut,
		const string &_szName, const string &_szPassword, const string &_szCDKey, const int _cTestMode );
	virtual bool Segment();
	bool IsActive() const;
	bool IsCancelled() const { return bCancelled; }

	// Processors
	bool ProcessConnectServerResult( class CConnectServerPacket *pPacket );
	bool ProcessMyIDPacket( class CMyIDPacket *pPacket );
	bool ProcessChatChannelClientsListPacket( class CChatChannelClientsListPacket *pPacket );
	bool ProcessChatPacket( class CChatPacket *pPacket );
	bool ProcessEnteredLobby( class CEnterLobbyPacket *pPacket );
	bool ProcessClientRemoved( class CNetRemoveClient *pPacket );
	bool ProcessLadderInvitePacket( class CLadderInvitePacket *pPacket );
	bool ProcessNewGameClient( class CNewGameClient *pPacket );
	bool ProcessGameKilled( class CGameKilled *pPacket );
};


