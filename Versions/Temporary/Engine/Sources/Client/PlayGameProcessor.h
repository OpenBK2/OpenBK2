#pragma once

#include "LinksManager.h"
#include "PeerConnectionRoute.h"
#include "PeerSetupPacket.h"
#include "Server_Client_Common/PacketProcessor.h"

#include <cstdint>
#include <map>
#include <set>
#include <vector>

class CNet;
class CNetPacket;
struct IConnection;
class CConnectionEffort;
class CConnectServerProcessor;

class CPlayGameProcessor : public CPacketProcessor
{
	OBJECT_NOCOPY_METHODS( CPlayGameProcessor )
	friend struct CPlayGameProcessorTestAccess;

	CPtr<CNet> pNet;
	CPtr<CNet> pAcceptGamersNet;

	typedef std::unordered_map<int, CPtr<IConnection>> TConnections;
	TConnections connections;
	// A direct transport can close before the departing player's final lobby
	// relays arrive. Only the ordered server notification confirms membership.
	std::set<int> pendingServerRemovals;
	std::set<int> joiningPeers;
	uint32_t dwJoinStartTime = 0;

	struct SPeerSetup
	{
		bool bInitiator = false;
		bool bRecovery = false;
		uint32_t nSetupID = 0;
		int nAttempt = 0;
		uint32_t dwStartTime = 0;
		EPeerConnectionRoute route = EPeerConnectionRoute::Unknown;
		bool bTested = false;
		bool bConfirmed = false;
		std::string szIP;
		int nPort = 0;
		CPtr<class CSimpleClientEffort> pClientEffort;
		std::list<CPtr<CNetPacket>> packets;
	};

	// Tentative connections cannot carry room/game traffic until route agreement.
	std::map<int, SPeerSetup> peerSetups;
	void SendPeerSetup( int nClientID, CPeerSetupPacket::EAction action );
	void ProcessPeerSetup( int nClientID, const CPeerSetupPacket &packet );
	void ProcessPeerSetups();
	void SelectRelay( int nClientID );
	void FinishPeerSetup( int nClientID );

	struct SPeerSession
	{
		bool bInitiator = false;
		uint32_t nSetupID = 0;
		int nAttempt = 0;
		std::string szIP;
		int nPort = 0;
		CPtr<class CSimpleClientEffort> pClientEffort;
		EPeerConnectionRoute route = EPeerConnectionRoute::Unknown;
		uint64_t nNextSend = 1;
		uint64_t nNextReceive = 1;
		uint64_t nLastAcknowledged = 0;
		std::map<uint64_t, std::vector<char>> pendingSend;
		std::map<uint64_t, std::vector<char>> pendingReceive;
		uint32_t dwLastReceive = 0;
		uint32_t dwLastAckProgress = 0;
		uint32_t dwLastHeartbeat = 0;
		bool bAckPending = false;
		bool bRecoveryRequested = false;
	};

	std::map<int, SPeerSession> peerSessions;
	std::set<int> failedDirectPeers;
	void ProcessPeerSessions();
	void BeginPeerRecovery( int nClientID );
	void PreparePeerRecovery( int nClientID );
	void SendSessionPacket( int nClientID, uint64_t nSequence, const std::vector<char> &data, bool bRequestAck = false );
	void QueueSessionPacket( int nClientID, CNetPacket *pPacket );
	void ReceiveSessionPacket( CNetPacket *pPacket );
	void FlushSessionPackets( int nClientID );

	typedef std::unordered_map<int, CPtr<CConnectionEffort>> TConnectionEfforts;
	TConnectionEfforts connectionEfforts;

	int nMyServerID;

	std::string szServerIP;
	int nNetGameVersion;
	int nServerPort;

	CPtr<CNetPacket> pConnectGamePacket;
	CPtr<CConnectServerProcessor> pConnectServerProcessor;

	struct SWaitingPacket
	{
		CPtr<CNetPacket> pPacket;
		uint32_t dwStartTime;

		SWaitingPacket()
			: dwStartTime( 0 )
		{
		}

		SWaitingPacket( CNetPacket *_pPacket, const uint32_t _dwStartTime )
			: pPacket( _pPacket )
			, dwStartTime( _dwStartTime )
		{
		}
	};

	std::list<SWaitingPacket> waitingPackets;

	CPtr<NNet::CGameLinksManager> pLinksManager;
	NNet::CNodeAddress serverAddress;
	int nOurGameID;
	int nTimeOut;

	//
	void StartServerConnect();
	void ProcessConnectServerProcessor();
	void ProcessAcceptingGamersPackets();
	void DrainConnectionPackets( IConnection *pConnection, bool bServerConfirmed = false );
	void ProcessEfforts();
	void Clear();
	// The id is passed in rather than read back out of pEffort: this is called
	// with a null effort on purpose, to report that the connection attempt
	// chain for that client has ended, and the removal path needs the id that
	// the now absent effort cannot supply.
	bool CreateConnectionEffort( CConnectionEffort *pEffort, const int nClientServerID );

public:
	CPlayGameProcessor() {}

	CPlayGameProcessor(
			CNet *pNet, const char *pszServerIP, const int nNetGameVersion, const int nServerPort, const int nTimeOut );

	bool Segment() override;
	void SendGamePacket( CNetPacket *pPacket, bool bBroadcast );
	EPeerConnectionRoute GetPeerRoute( int nClientID ) const;
	void ProcessConnectServerPacket( class CConnectServerPacket *pPacket );

	bool ProcessPacket( CNetPacket *pPacket ) override;
	bool ProcessPeerSetupEnvelope( CNetPacket *pPacket );

	//
	bool ProcessCreateGame( class CCreateGamePacket *pPacket );
	bool ProcessConnectGame( class CConnectGamePacket *pPacket );
	bool ProcessMyIDPacket( class CMyIDPacket *pPacket );
	bool ProcessGameConnectingClientAccepted( class CGameConnectingClientAccepted *pPacket );
	bool ProcessAnswerConnectGame( class CAnswerConnectGame *pPacket );
	bool ProcessClientWantToConnect( class CClientWantToConnect *pPacket );
	bool ProcessLeaveGame( class CLeaveGamePacket *pPacket );
	bool ProcessGameKilled( class CGameKilled *pPacket );
	bool ProcessKickClient( class CGameKickClient *pPacket );
	bool ProcessClientWasKicked( class CGameClientWasKicked *pPacket );
	bool ProcessConnectedGameID( class CConnectedGameID *pPacket );
	bool ProcessConnectGameFailed( class CConnectGameFailed *pPacket );
	bool ProcessGameClientDead( class CGameClientDead *pPacket );

	// debug functions
	void TogglePauseAcceptGamersNet( bool bPause );
	void TogglePauseConnectGamer( const int nGamer, bool bPause );
};
