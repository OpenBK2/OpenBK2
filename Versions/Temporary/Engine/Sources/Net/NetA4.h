#pragma once

#include "NetStream.h"
#include "NetServerInfo.h"
#include "NetLogin.h"
#include "NetPeer2Peer.h"

#include "Misc/HPTimer.h"

#if !defined(_FINALRELEASE) || defined(_DEVVERSION)
//#define __TEST_LAGS__
#endif // !defined(_FINALRELEASE) || defined(_DEVVERSION)

namespace NNet
{
	////////////////////////////////////////////////////////////////////////////////////////////////////
class CNetDriver : public IDriver
{
	OBJECT_NOCOPY_METHODS( CNetDriver );
public:
	struct SServer
	{
		CNodeAddress addr;
		bool bWrongVersion;
		float fPing;
		CMemoryStream info;
	};
	struct SMessage
	{
		EMessage msg;
		int nClientID;
		vector<int> received;
		CMemoryStream pkt;
	};
private:
	SNetDriverConsts consts;
	bool bIsBroadcast; // used to prevent undesireble broadcast
	bool bIsClient; //to determine client or server
	//
	struct SClientAddressInfo
	{
		CNodeAddress inetAddress;
		CNodeAddressSet localAddress;
		
		SClientAddressInfo() {}
		SClientAddressInfo( const CNodeAddress &_inetAddress, const CNodeAddressSet &_localAddress ) 
			: inetAddress(_inetAddress), localAddress(_localAddress) {}
	};
	//
	struct SPeer
	{
		CP2PTracker::UCID clientID;
		CNodeAddress currentAddr;
		SClientAddressInfo addrInfo;
		CAckTracker acks;
		CStreamTracker data;
		bool bTryShortcut;
		float fTimeToSendData;
		SPeer() : fTimeToSendData( 0.0f ) {}
	};

	//
	typedef hash_map<CP2PTracker::UCID,SPeer> CPeerList;
	CPeerList clients;
	EState state;
	EReject lastReject;
	NHPTimer::STime lastTime;
	CServerInfoSupport serverInfo;
	CLoginSupport login;
	CP2PTracker p2p;
	list<SMessage> msgQueue;
	bool bAcceptNewClients;
	CPtr<ILinksManager> pLinks; // created/destroyed only in Init/Destroy
	int nGamePort;
	CNodeAddress addr;
	CNodeAddress gameHostAddress;

	SPeer* GetClientByAddr( const CNodeAddress &addr );
	SPeer* GetClient( CP2PTracker::UCID nID );
	void AddClient( const SClientAddressInfo &addr, CP2PTracker::UCID clientID );
	void AddNewP2PClient( const SClientAddressInfo &addr, CP2PTracker::UCID clientID );
	void RemoveClient( CP2PTracker::UCID nID );
	void StepInactive();
	void StepConnecting();
	void StepActive( float fDeltaTime );

	void ProcessIncomingMessages();
	void ProcessLogin( const CNodeAddress &addr, CBitStream &bits );
	void ProcessNormal( const CNodeAddress &addr, CBitStream &bits );
	void AddOutputMessage( EMessage msg, CP2PTracker::UCID _from, 
		CMemoryStream &data, const vector<CP2PTracker::UCID> &received );

	void PollMessages( SPeer *pPeer );
	void ProcessP2PMessages();

	//
	// thread
	HANDLE hThread;
	HANDLE hThreadReport;
	HANDLE hStopCommand;

	// CRAP{ for traffic to winsock measurement
	NTimer::STime lastTrafficCheckTime;
	int nSent;
	// CRAP}

#ifdef __TEST_LAGS__
	NTimer::STime lastSendTime;
	NTimer::STime lastReceiveTime;
	int nMsgCanReceive;
	NTimer::STime lagPeriod;
	bool bPaused;
	bool bSendNow;
	bool bReceiveNow;

	list<CMemoryStream> msgToSendBroadcast;
	list< pair< int, CMemoryStream > > msgToSendDirect;

	bool AnalyzeLags();
#endif __TEST_LAGS__

	//
	void CreateEvents();
	void KillThread();

	void StartGameInfoSend( const CMemoryStream &data );
public:
	CNetDriver();
	CNetDriver( const SNetDriverConsts &_consts, bool _bIsBroadcast );
	~CNetDriver();
	//
	virtual void Init( const APPLICATION_ID nApplicationID, const int nGamePort, bool bClientOnly, ILinksManager *pLinksManager );
	// get current state (active/inactive/connecting)
	virtual EState GetState() const;
	// get reject reason (then )
	virtual EReject GetRejectReason() const { return lastReject; }
	// connect to the game with particular address
	virtual void ConnectGame( const CNodeAddress &addr, const CMemoryStream &pwd );
	// start game (server)
	virtual void StartGame();
	// start sending game info (for server), should be compatible with gamespy
	virtual void StartGameInfoSend( const SGameInfo &gameInfo );
	// stop sending game info (for server)
	virtual void StopGameInfoSend();
	// start accepting new players (for server)
	virtual void StartNewPlayerAccept();
	// stop accepting new players (for server)
	virtual void StopNewPlayerAccept();
	// get game info (for client)
	virtual bool GetGameInfo( int nIdx, CNodeAddress *pAddr, bool *pWrongVersion, float *pPing, SGameInfo *pGameInfo );
	// refresh servers list ( for client )
	virtual void RefreshServersList() { }
	// send broadcast message for all
	virtual bool SendBroadcast( const CMemoryStream &pkt );
	// send direct message for client 'nClient'
	virtual bool SendDirect( int nClient, const CMemoryStream &pkt );
	// kick player 'nClient'
	virtual void Kick( int nClient );
	// get next message
	virtual bool GetMessage( EMessage *pMsg, int *pClientID, vector<int> *pReceived, CMemoryStream *pPkt );
	// ping of the client, -1 if client doesn't exist
	virtual const float GetPing( const int nClientID );
	// time since last message was received from this client
	virtual const float GetTimeSinceLastRecv( const int nClientID );
	// step net driver
	virtual void Step();
	virtual int GetSelfClientID() { return login.GetSelfClientID(); }

	virtual const string GetIP( const int nClientID );
	virtual const int GetPort( const int nClientID );
	// 
	// for debug
	virtual const char* GetAddressByClientID( const int nClientID ) const;


	virtual void PauseNet();
	virtual void UnpauseNet();
	virtual void SetLag( const NTimer::STime period );
	
	// thread functions for internal use only
	void StartThread();
	bool CanWork();
	void FinishThread();
	//
	int operator&( IBinSaver &saver ) { return 0; }
};
}


