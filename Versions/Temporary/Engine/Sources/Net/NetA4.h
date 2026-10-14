#pragma once

#include "Misc/HPTimer.h"
#include "NetLogin.h"
#include "NetPeer2Peer.h"
#include "NetServerInfo.h"
#include "NetStream.h"

#include <atomic>
#include <future>
#include <thread>

#if !defined( _FINALRELEASE ) || defined( _DEVVERSION )
//#define __TEST_LAGS__
#endif // !defined(_FINALRELEASE) || defined(_DEVVERSION)

namespace NNet
{
////////////////////////////////////////////////////////////////////////////////////////////////////
class CNetDriver : public IDriver
{
	OBJECT_NOCOPY_METHODS( CNetDriver );
	friend struct CNetDriverTestAccess;

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
		std::vector<int> received;
		CMemoryStream pkt;
	};

private:
	SNetDriverConsts consts;
	bool bIsBroadcast; // used to prevent undesireble broadcast
	bool bIsClient;    //to determine client or server

	//
	struct SClientAddressInfo
	{
		CNodeAddress inetAddress;
		CNodeAddressSet localAddress;

		SClientAddressInfo() {}

		SClientAddressInfo( const CNodeAddress &_inetAddress, const CNodeAddressSet &_localAddress )
			: inetAddress( _inetAddress )
			, localAddress( _localAddress )
		{
		}
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

		SPeer()
			: fTimeToSendData( 0.0f )
		{
		}
	};

	//
	typedef std::unordered_map<CP2PTracker::UCID, SPeer> CPeerList;
	CPeerList clients;
	EState state;
	EReject lastReject;
	NHPTimer::STime lastTime;
	CServerInfoSupport serverInfo;
	CLoginSupport login;
	CP2PTracker p2p;
	std::list<SMessage> msgQueue;
	bool bAcceptNewClients;
	CPtr<ILinksManager> pLinks; // created/destroyed only in Init/Destroy
	int nGamePort;
	CNodeAddress addr;
	CNodeAddress gameHostAddress;

	SPeer *GetClientByAddr( const CNodeAddress &addr );
	SPeer *GetClient( CP2PTracker::UCID nID );
	void AddClient( const SClientAddressInfo &addr, CP2PTracker::UCID clientID );
	void AddNewP2PClient( const SClientAddressInfo &addr, CP2PTracker::UCID clientID );
	void RemoveClient( CP2PTracker::UCID nID );
	void StepInactive();
	void StepConnecting();
	void StepPeerTimers( float fDeltaTime );
	void StepActive( float fDeltaTime );

	void ProcessIncomingMessages();
	void ProcessLogin( const CNodeAddress &addr, CBitStream &bits );
	void ProcessNormal( const CNodeAddress &addr, CBitStream &bits );
	void AddOutputMessage( EMessage msg,
	                       CP2PTracker::UCID _from,
	                       CMemoryStream &data,
	                       const std::vector<CP2PTracker::UCID> &received );

	void PollMessages( SPeer *pPeer );
	void ProcessP2PMessages();

	//
	// thread
	std::thread thread;
	// set once to ask the loop to stop, which is all hStopCommand was used for:
	// it was only ever polled with a zero timeout
	std::atomic<bool> bStopRequested;
	// satisfied when the thread reaches its loop, so Init does not return before
	// the driver is running. The other half of what hThreadReport did, waiting
	// for the thread to finish, is what join does.
	std::promise<void> threadStarted;

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

	std::list<CMemoryStream> msgToSendBroadcast;
	std::list<std::pair<int, CMemoryStream>> msgToSendDirect;

	bool AnalyzeLags();
#endif // __TEST_LAGS__

	//
	void KillThread();

	void StartGameInfoSend( const CMemoryStream &data );

public:
	CNetDriver();
	CNetDriver( const SNetDriverConsts &_consts, bool _bIsBroadcast );
	~CNetDriver() override;
	//
	void Init( const APPLICATION_ID nApplicationID,
	           const int nGamePort,
	           bool bClientOnly,
	           ILinksManager *pLinksManager ) override;
	// get current state (active/inactive/connecting)
	EState GetState() const override;

	// get reject reason (then )
	EReject GetRejectReason() const override
	{
		return lastReject;
	}

	// connect to the game with particular address
	void ConnectGame( const CNodeAddress &addr, const CMemoryStream &pwd ) override;
	// start game (server)
	void StartGame() override;
	// start sending game info (for server), should be compatible with gamespy
	void StartGameInfoSend( const SGameInfo &gameInfo ) override;
	// stop sending game info (for server)
	void StopGameInfoSend() override;
	// start accepting new players (for server)
	void StartNewPlayerAccept() override;
	// stop accepting new players (for server)
	void StopNewPlayerAccept() override;
	// get game info (for client)
	bool GetGameInfo( int nIdx, CNodeAddress *pAddr, bool *pWrongVersion, float *pPing, SGameInfo *pGameInfo ) override;

	// refresh servers list ( for client )
	void RefreshServersList() override {}

	// send broadcast message for all
	bool SendBroadcast( const CMemoryStream &pkt ) override;
	// send direct message for client 'nClient'
	bool SendDirect( int nClient, const CMemoryStream &pkt ) override;
	// kick player 'nClient'
	void Kick( int nClient ) override;
	// get next message
	bool GetMessage( EMessage *pMsg, int *pClientID, std::vector<int> *pReceived, CMemoryStream *pPkt ) override;
	// ping of the client, -1 if client doesn't exist
	const float GetPing( const int nClientID ) override;
	// time since last message was received from this client
	const float GetTimeSinceLastRecv( const int nClientID ) override;
	// step net driver
	void Step() override;

	int GetSelfClientID() override
	{
		return login.GetSelfClientID();
	}

	const std::string GetIP( const int nClientID ) override;
	const int GetPort( const int nClientID ) override;

	void PauseNet() override;
	void UnpauseNet() override;
	void SetLag( const NTimer::STime period ) override;

	// thread functions for internal use only
	void StartThread();
	bool CanWork();
	void FinishThread();

	//
	int operator&( IBinSaver &saver ) override
	{
		return 0;
	}
};
} // namespace NNet
