#include "Client/stdafx.h"

#include "Client/ConnectionEffortInternal.h"
#include "Client/ConnectionInternal.h"
#include "Client/PlayGameProcessor.h"
#include "GameX/MultiplayerNetPackets.h"
#include "Net/NetLowest.h"
#include "port/time.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/NetSaver.h"

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

// Keep the receive converter private in production while testing the actual
// public SendPacket path, including its dispatch through that converter.
#include "../ServerClient.cpp"

REGISTER_SAVELOAD_CLASS( CLIENT, 107, CB2DropPlayerAtSegmentPacket );

struct CPlayGameProcessorTestAccess
{
	static CNet *CreateNet( NNet::IDriver *driver )
	{
		CNet *net = new CNet( 1, 0, 60 );
		net->pNetDriver = driver;
		return net;
	}

	static void InitEffort( CSimpleClientEffort &effort,
	                        CNet *lobby,
	                        CNet *peer,
	                        NNet::CGameLinksManager *links,
	                        int clientID,
	                        bool connectingPeer = false )
	{
		// Replace only socket setup; exercise the production state machine.
		effort.nMyServerID = 42;
		effort.nClientServerID = clientID;
		effort.pNet = lobby;
		effort.pConnet2PlayersNet = peer;
		effort.pLinksManager = links;
		effort.pConnectServerProcessor = new CConnectServerProcessor( peer, false );
		effort.eState = CConnectionEffort::ES_CONNECTING;
		effort.eSubState = connectingPeer ? CSimpleClientEffort::ECONNECT_CLIENT : CSimpleClientEffort::ECONNECT_SERVER;
		effort.dwStartConnectClientTime = GetCurrentTimeMilliseconds();
		effort.dwPeerAttemptStart = effort.dwStartConnectClientTime;
		effort.dwPredBreakThroughSend = effort.dwStartConnectClientTime - 3001;
		effort.szClientIP = "127.0.0.1";
		effort.nClientPort = 43210;
	}

	static void ExpireEffort( CSimpleClientEffort &effort )
	{
		effort.dwStartConnectClientTime = GetCurrentTimeMilliseconds() - uint32_t( 1000 * CNet::GetTimeOut() ) - 1;
	}

	static void RetryPunch( CSimpleServerEffort &effort )
	{
		effort.dwPredBreakThroughSend = GetCurrentTimeMilliseconds() - 3001;
	}

	static void ExpireEffort( CSimpleServerEffort &effort )
	{
		effort.dwStartConnectClientTime = GetCurrentTimeMilliseconds() - uint32_t( 1000 * CNet::GetTimeOut() ) - 1;
	}

	static void AddEffort( CPlayGameProcessor &processor, CConnectionEffort *effort, CNet *accepting = nullptr )
	{
		processor.nOurGameID = 968;
		processor.pAcceptGamersNet = accepting;
		processor.CreateConnectionEffort( effort, effort->GetClientServerID() );
	}

	static void QueueEarlyPacket( CPlayGameProcessor &processor, CNetPacket *packet )
	{
		processor.waitingPackets.emplace_back( packet, GetCurrentTimeMilliseconds() );
	}

	static size_t GetWaitingCount( const CPlayGameProcessor &processor )
	{
		return processor.waitingPackets.size();
	}

	static void MarkJoining( CPlayGameProcessor &processor, int clientID, bool expired = false )
	{
		processor.joiningPeers.insert( clientID );
		processor.dwJoinStartTime = GetCurrentTimeMilliseconds();
		if ( expired )
		{
			processor.dwJoinStartTime -= uint32_t( 1000 * CNet::GetTimeOut() ) + 1;
		}
	}

	static void InitPeer( CServerClient &client, int clientID, NNet::CGameLinksManager *acceptingLinks = nullptr )
	{
		auto &processor = *client.pPlayGameProcessor;
		processor.nMyServerID = clientID;
		processor.nOurGameID = 968;
		if ( acceptingLinks )
		{
			processor.pLinksManager = acceptingLinks;
			processor.pAcceptGamersNet = new CNet( 1, 0, 60 );
			processor.pAcceptGamersNet->InitAsServer( acceptingLinks );
			processor.pAcceptGamersNet->StartGame();
		}
	}

	static void AddRelay( CPlayGameProcessor &processor, int clientID )
	{
		// The handshake is already complete in a running match. Bypass only
		// socket setup; delivery below uses the real connection and processor.
		CPtr<CThroughServerConnection> connection = new CThroughServerConnection();
		connection->nClientServerID = clientID;
		connection->bConnectionTested = true;
		processor.connections[clientID] = connection;
		processor.nOurGameID = 968;
	}

	static bool HasConnection( const CPlayGameProcessor &processor, int clientID )
	{
		return processor.connections.find( clientID ) != processor.connections.end();
	}

	static void InitServerClient( CServerClient &client, NNet::IDriver *driver )
	{
		client.bDebugPaused = false;
		client.pNet = new CNet( 1, 0, 60 );
		client.pNet->pNetDriver = driver;
		client.processors.push_back( new CPacketsConvertor() );
		client.pPlayGameProcessor = new CPlayGameProcessor( client.pNet, "127.0.0.1", 1, 0, 60 );
		client.processors.push_back( client.pPlayGameProcessor.GetPtr() );
	}

	static void AddRelay( CServerClient &client, int clientID )
	{
		AddRelay( *client.pPlayGameProcessor, clientID );
	}

	static void AddConnection( CServerClient &client, int clientID, IConnection *connection )
	{
		client.pPlayGameProcessor->connections[clientID] = connection;
		client.pPlayGameProcessor->nOurGameID = 968;
	}

	static bool HasConnection( const CServerClient &client, int clientID )
	{
		return HasConnection( *client.pPlayGameProcessor, clientID );
	}

	static bool IsAwaitingServerRemoval( const CServerClient &client, int clientID )
	{
		const auto &pending = client.pPlayGameProcessor->pendingServerRemovals;
		return pending.find( clientID ) != pending.end();
	}
};

namespace
{
const int hostID = 221376;
const int teammateID = 221421;

CMemoryStream Serialize( CNetPacket *packet )
{
	CMemoryStream result;
	CPtr<CNetPacket> holder = packet;
	CPtr<IBinSaver> saver = CreateNetSaver( &result, SAVER_MODE_WRITE );
	saver->Add( 1, &holder );
	return result;
}

CPtr<CNetPacket> Deserialize( CMemoryStream &stream )
{
	stream.Seek( 0 );
	CPtr<CNetPacket> result;
	CPtr<IBinSaver> saver = CreateNetSaver( &stream, SAVER_MODE_READ );
	saver->Add( 1, &result );
	return result;
}

CPtr<CNetPacket> RoundTrip( CNetPacket *packet )
{
	CMemoryStream stream = Serialize( packet );
	return Deserialize( stream );
}

class CRecordingDriver : public NNet::IDriver
{
	OBJECT_NOCOPY_METHODS( CRecordingDriver );

public:
	std::vector<CMemoryStream> sent;
	std::list<CMemoryStream> received;
	EState state = ACTIVE;
	int expectedRecipient = 0;

	void Init( NNet::APPLICATION_ID, int, bool, NNet::ILinksManager * ) override {}

	EState GetState() const override
	{
		return state;
	}

	EReject GetRejectReason() const override
	{
		return NONE;
	}

	void ConnectGame( const NNet::CNodeAddress &, const CMemoryStream & ) override {}

	void StartGame() override {}

	void StartGameInfoSend( const SGameInfo & ) override {}

	void StopGameInfoSend() override {}

	void StartNewPlayerAccept() override {}

	void StopNewPlayerAccept() override {}

	bool GetGameInfo( int, NNet::CNodeAddress *, bool *, float *, SGameInfo * ) override
	{
		return false;
	}

	void RefreshServersList() override {}

	bool SendBroadcast( const CMemoryStream & ) override
	{
		return false;
	}

	bool SendDirect( int clientID, const CMemoryStream &packet ) override
	{
		EXPECT_EQ( clientID, expectedRecipient );
		sent.push_back( packet );
		return true;
	}

	void Kick( int ) override {}

	bool GetMessage( EMessage *type, int *clientID, std::vector<int> *, CMemoryStream *packet ) override
	{
		if ( received.empty() )
		{
			return false;
		}
		*type = DIRECT;
		*clientID = 0;
		*packet = received.front();
		received.pop_front();
		return true;
	}

	const float GetPing( int ) override
	{
		return 0;
	}

	const float GetTimeSinceLastRecv( int ) override
	{
		return 0;
	}

	void Step() override {}

	int GetSelfClientID() override
	{
		return 0;
	}

	const std::string GetIP( int ) override
	{
		return "127.0.0.1";
	}

	const int GetPort( int ) override
	{
		return 0;
	}
};

// Only the already-established direct transport is simulated. Its notification
// and the lobby's serialized response run through the real client processors.
class CQueuedDirectConnection : public IConnection
{
	OBJECT_NOCOPY_METHODS( CQueuedDirectConnection );
	bool bAlive = true;

public:
	std::list<CPtr<CNetPacket>> packets;

	bool IsAlive() const override
	{
		return bAlive;
	}

	CNet *GetNet() override
	{
		return nullptr;
	}

	void SendGamePacket( CNetPacket * ) override {}

	bool ProcessPacketFromPlayersNet( CNetPacket * ) override
	{
		return false;
	}

	bool ProcessPacketFromServer( CNetPacket * ) override
	{
		return false;
	}

	CNetPacket *GetPacket() override
	{
		if ( packets.empty() )
		{
			return nullptr;
		}
		CPtr<CNetPacket> packet = packets.front();
		packets.pop_front();
		if ( dynamic_cast<CGameClientRemoved *>( packet.GetPtr() ) )
		{
			bAlive = false;
		}
		return packet.Extract();
	}

	void TogglePauseNet( bool ) const override {}
};

void ReceiveDrop( CPlayGameProcessor &processor, int sender, int slot, int segment )
{
	CPtr<CNetPacket> forwarded =
			new CThroughServerGamePacket( 0, sender, new CB2DropPlayerAtSegmentPacket( 0, slot, segment ) );
	CPtr<CNetPacket> received = RoundTrip( forwarded );
	ASSERT_TRUE( processor.ProcessPacket( received ) );
}

void ExpectDrop( CPlayGameProcessor &processor, int sender, int slot, int segment )
{
	CPtr<CNetPacket> packet = processor.GetPacket();
	auto *drop = dynamic_cast<CB2DropPlayerAtSegmentPacket *>( packet.GetPtr() );
	ASSERT_NE( drop, nullptr );
	EXPECT_EQ( drop->nClientID, sender );
	EXPECT_EQ( drop->nSlotToDrop, slot );
	EXPECT_EQ( drop->nSegment, segment );
}

void ReceiveDeath( CPlayGameProcessor &processor, int clientID )
{
	CPtr<CNetPacket> dead = new CGameClientDead( 0, clientID );
	CPtr<CNetPacket> received = RoundTrip( dead );
	ASSERT_TRUE( processor.ProcessPacket( received ) );
}
} // namespace

namespace
{
// Recording the supplied links manager catches accidental use of a new socket.
class CRecordingGameLinks : public NNet::CGameLinksManager
{
	OBJECT_NOCOPY_METHODS( CRecordingGameLinks );

public:
	mutable std::vector<NNet::CNodeAddress> destinations;
	mutable std::vector<CMemoryStream> sent;

	bool Send( const NNet::CNodeAddress &destination, CMemoryStream &packet ) const override
	{
		destinations.push_back( destination );
		sent.push_back( packet );
		return true;
	}
};
} // namespace

TEST( ClientDirectConnections, RegistrationAdvertisesTheRetainedPeerSocket )
{
	CPtr<CRecordingDriver> lobbyDriver = new CRecordingDriver();
	CPtr<CRecordingDriver> peerDriver = new CRecordingDriver();
	CPtr<CNet> lobby = CPlayGameProcessorTestAccess::CreateNet( lobbyDriver );
	CPtr<CNet> peer = CPlayGameProcessorTestAccess::CreateNet( peerDriver );
	CSimpleClientEffort effort;
	CPlayGameProcessorTestAccess::InitEffort( effort, lobby, peer, nullptr, hostID );
	CPtr<CConnectServerPacket> connected =
			new CConnectServerPacket( CConnectServerPacket::ECS_SUCCESS, CConnectServerPacket::ERR_UNKNOWN );
	effort.ProcessConnectServerPacket( connected );
	ASSERT_EQ( peerDriver->sent.size(), 1u );
	EXPECT_TRUE( lobbyDriver->sent.empty() );
	CPtr<CNetPacket> packet = Deserialize( peerDriver->sent.front() );
	auto *registration = dynamic_cast<CNewGameConnectingClient *>( packet.GetPtr() );
	ASSERT_NE( registration, nullptr );
	EXPECT_EQ( registration->nServerID, 42 );
	EXPECT_EQ( registration->nConnection, hostID );

	// The existing server replies on the lobby link, which still requests the introduction.
	CPtr<CGameConnectingClientAccepted> accepted = new CGameConnectingClientAccepted( 0, hostID );
	EXPECT_TRUE( effort.ProcessClientAccepted( accepted ) );
	ASSERT_EQ( lobbyDriver->sent.size(), 1u );
	packet = Deserialize( lobbyDriver->sent.front() );
	ASSERT_NE( dynamic_cast<CWant2Connect2Client *>( packet.GetPtr() ), nullptr );
}

TEST( ClientDirectConnections, ProbeFailureWithoutIncomingPacketReportsRemovalWithoutRelay )
{
	CPtr<CRecordingDriver> lobbyDriver = new CRecordingDriver();
	CPtr<CRecordingDriver> peerDriver = new CRecordingDriver();
	peerDriver->state = NNet::IDriver::INACTIVE;
	CPtr<CNet> lobby = CPlayGameProcessorTestAccess::CreateNet( lobbyDriver );
	CPtr<CNet> peer = CPlayGameProcessorTestAccess::CreateNet( peerDriver );
	CPtr<CSimpleClientEffort> effort = new CSimpleClientEffort();
	CPlayGameProcessorTestAccess::InitEffort( *effort, lobby, peer, nullptr, hostID );
	CPlayGameProcessor processor( lobby, "127.0.0.1", 1, 0, 60 );
	CPlayGameProcessorTestAccess::AddEffort( processor, effort );
	processor.Segment();
	EXPECT_EQ( effort->GetState(), CConnectionEffort::ES_FAILED );
	CPtr<CNetPacket> packet = processor.GetPacket();
	ASSERT_NE( dynamic_cast<CGameClientRemoved *>( packet.GetPtr() ), nullptr );
	EXPECT_EQ( packet->nClientID, hostID );
	processor.Segment();
	EXPECT_EQ( processor.GetPacket(), nullptr );
	EXPECT_FALSE( CPlayGameProcessorTestAccess::HasConnection( processor, hostID ) );
	EXPECT_TRUE( lobbyDriver->sent.empty() );
}

TEST( ClientDirectConnections, UnansweredRegistrationAndPeerHandshakeBothTimeOut )
{
	for ( bool connectingPeer : { false, true } )
	{
		CPtr<CRecordingDriver> driver = new CRecordingDriver();
		CPtr<CNet> peer = CPlayGameProcessorTestAccess::CreateNet( driver );
		CSimpleClientEffort effort;
		CPlayGameProcessorTestAccess::InitEffort( effort, peer, peer, nullptr, hostID, connectingPeer );
		CPlayGameProcessorTestAccess::ExpireEffort( effort );
		effort.Segment();
		EXPECT_EQ( effort.GetState(), CConnectionEffort::ES_FAILED );
		CPtr<CConnectionEffort> fallback = effort.CreateNextEffort( 968 );
		EXPECT_EQ( fallback.GetPtr(), nullptr );
		EXPECT_TRUE( driver->sent.empty() );
	}
}

TEST( ClientDirectConnections, BothEndpointsPunchFromTheirRetainedSockets )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CPtr<CNet> peer = CPlayGameProcessorTestAccess::CreateNet( driver );
	CPtr<CRecordingGameLinks> outgoingLinks = new CRecordingGameLinks();
	CPtr<CRecordingGameLinks> acceptingLinks = new CRecordingGameLinks();
	CSimpleClientEffort outgoing;
	CPlayGameProcessorTestAccess::InitEffort( outgoing, peer, peer, outgoingLinks, hostID, true );
	outgoing.Segment();
	CSimpleServerEffort accepting( 42, hostID, "127.0.0.1", 43210, peer, peer, acceptingLinks );
	CPlayGameProcessorTestAccess::RetryPunch( accepting );
	accepting.Segment();
	ASSERT_EQ( outgoingLinks->sent.size(), 1u );
	ASSERT_EQ( acceptingLinks->sent.size(), 2u );
	CMemoryStream nop;
	NNet::GetNOPStream( &nop );
	NNet::CNodeAddress destination;
	ASSERT_TRUE( destination.SetInetName( "127.0.0.1", 43210 ) );
	for ( const auto *links : { outgoingLinks.GetPtr(), acceptingLinks.GetPtr() } )
	{
		for ( size_t i = 0; i < links->sent.size(); ++i )
		{
			EXPECT_EQ( links->destinations[i], destination );
			ASSERT_EQ( links->sent[i].GetSize(), nop.GetSize() );
			EXPECT_EQ( std::memcmp( links->sent[i].GetBuffer(), nop.GetBuffer(), nop.GetSize() ), 0 );
		}
	}
}

TEST( ClientDirectConnections, AcceptingTimeoutDoesNotCreateRelay )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CPtr<CNet> peer = CPlayGameProcessorTestAccess::CreateNet( driver );
	CPtr<CRecordingGameLinks> links = new CRecordingGameLinks();
	CPtr<CSimpleServerEffort> effort = new CSimpleServerEffort( 42, hostID, "127.0.0.1", 43210, peer, peer, links );
	CPlayGameProcessorTestAccess::ExpireEffort( *effort );
	CPlayGameProcessor processor( peer, "127.0.0.1", 1, 0, 60 );
	CPlayGameProcessorTestAccess::AddEffort( processor, effort );
	processor.Segment();
	EXPECT_EQ( effort->GetState(), CConnectionEffort::ES_FAILED );
	CPtr<CNetPacket> packet = processor.GetPacket();
	ASSERT_NE( dynamic_cast<CGameClientRemoved *>( packet.GetPtr() ), nullptr );
	EXPECT_EQ( packet->nClientID, hostID );
	EXPECT_FALSE( CPlayGameProcessorTestAccess::HasConnection( processor, hostID ) );
	EXPECT_TRUE( driver->sent.empty() );
}

TEST( ClientDirectConnections, FailedOutgoingJoinLeavesRoomAndReportsJoinFailure )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	driver->state = NNet::IDriver::INACTIVE;
	CPtr<CNet> net = CPlayGameProcessorTestAccess::CreateNet( driver );
	CPtr<CSimpleClientEffort> effort = new CSimpleClientEffort();
	CPlayGameProcessorTestAccess::InitEffort( *effort, net, net, nullptr, hostID );
	CPlayGameProcessor processor( net, "127.0.0.1", 1, 0, 60 );
	CPlayGameProcessorTestAccess::AddEffort( processor, effort );
	CPlayGameProcessorTestAccess::MarkJoining( processor, hostID );
	processor.Segment();
	CPtr<CNetPacket> failure = processor.GetPacket();
	ASSERT_NE( dynamic_cast<CConnectGameFailed *>( failure.GetPtr() ), nullptr );
	ASSERT_EQ( driver->sent.size(), 1u );
	CPtr<CNetPacket> leave = Deserialize( driver->sent.front() );
	ASSERT_NE( dynamic_cast<CLeaveGamePacket *>( leave.GetPtr() ), nullptr );
	processor.Segment();
	EXPECT_EQ( processor.GetPacket(), nullptr );
	EXPECT_EQ( driver->sent.size(), 1u );
}

TEST( ClientDirectConnections, MissingFinalConnectionTestAlsoEndsTheJoin )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CPtr<CNet> net = CPlayGameProcessorTestAccess::CreateNet( driver );
	CPlayGameProcessor processor( net, "127.0.0.1", 1, 0, 60 );
	CPlayGameProcessorTestAccess::AddRelay( processor, hostID );
	CPlayGameProcessorTestAccess::MarkJoining( processor, hostID, true );
	processor.Segment();
	CPtr<CNetPacket> failure = processor.GetPacket();
	ASSERT_NE( dynamic_cast<CConnectGameFailed *>( failure.GetPtr() ), nullptr );
	EXPECT_FALSE( CPlayGameProcessorTestAccess::HasConnection( processor, hostID ) );
	ASSERT_EQ( driver->sent.size(), 1u );
	CPtr<CNetPacket> leave = Deserialize( driver->sent.front() );
	ASSERT_NE( dynamic_cast<CLeaveGamePacket *>( leave.GetPtr() ), nullptr );
}

static void CheckSocketHandshake( bool bPublicReachable, bool bLocalReachable, bool bForwardOffers )
{
	// Real UDP sockets, drivers, registration, identity and connection-test
	// packets. Only the lobby's authenticated introduction/forwarding is modeled.
	CPtr<NNet::CGameLinksManager> serverLinks = new NNet::CGameLinksManager();
	CPtr<NNet::CGameLinksManager> hostLinks = new NNet::CGameLinksManager();
	CPtr<NNet::CGameLinksManager> unreachableLinks = new NNet::CGameLinksManager();
	ASSERT_TRUE( serverLinks->Init( 0 ) );
	ASSERT_TRUE( hostLinks->Init( 0 ) );
	ASSERT_TRUE( unreachableLinks->Init( 0 ) );
	NNet::CNodeAddressSet addresses;
	NNet::CNodeAddress serverAddress, unreachableAddress, hostAddress;
	ASSERT_TRUE( serverLinks->GetSelfAddress( &addresses ) );
	ASSERT_TRUE( addresses.GetAddress( 0, &serverAddress ) );
	ASSERT_TRUE( unreachableLinks->GetSelfAddress( &addresses ) );
	ASSERT_TRUE( addresses.GetAddress( 0, &unreachableAddress ) );
	ASSERT_TRUE( hostLinks->GetSelfAddress( &addresses ) );
	ASSERT_TRUE( addresses.GetAddress( 0, &hostAddress ) );
	if ( hostAddress.GetIP().compare( 0, 4, "127." ) == 0 )
	{
		GTEST_SKIP() << "Local-address offers require a non-loopback interface";
	}
	CPtr<CNet> server = new CNet( 1, 0, 60 );
	ASSERT_TRUE( server->InitAsServer( serverLinks ) );
	server->StartGame();
	CPtr<CRecordingDriver> hostLobby = new CRecordingDriver();
	CPtr<CRecordingDriver> joiningLobby = new CRecordingDriver();
	CServerClient host, joining;
	CPlayGameProcessorTestAccess::InitServerClient( host, hostLobby );
	CPlayGameProcessorTestAccess::InitServerClient( joining, joiningLobby );
	CPlayGameProcessorTestAccess::InitPeer( host, hostID, hostLinks );
	CPlayGameProcessorTestAccess::InitPeer( joining, teammateID );
	NEffortsFactory::SetServerInfo( "127.0.0.1", serverAddress.GetPort(), 1 );
	CPtr<CAnswerConnectGame> answer = new CAnswerConnectGame();
	answer->clients.push_back( hostID );
	joiningLobby->received.push_back( Serialize( answer ) );
	bool hostConnected = false;
	bool joiningConnected = false;
	bool sentPing = false;
	bool receivedEcho = false;
	int offersForwarded = 0;
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 12 );
	while ( !receivedEcho && std::chrono::steady_clock::now() < deadline )
	{
		joining.Segment();
		host.Segment();
		while ( CPtr<CNetPacket> packet = server->ReceivePacket() )
		{
			if ( auto *registration = dynamic_cast<CNewGameConnectingClient *>( packet.GetPtr() ) )
			{
				EXPECT_EQ( registration->nServerID, teammateID );
				EXPECT_EQ( registration->nConnection, hostID );
				joiningLobby->received.push_back( Serialize( new CGameConnectingClientAccepted( 0, hostID ) ) );
			}
		}
		for ( auto &stream : joiningLobby->sent )
		{
			CPtr<CNetPacket> packet = Deserialize( stream );
			ASSERT_NE( dynamic_cast<CWant2Connect2Client *>( packet.GetPtr() ), nullptr );
			// Bound but unanswered sockets model a router that cannot hairpin.
			joiningLobby->received.push_back( Serialize( new CClientGameConnectInfo(
					0, "127.0.0.1", bPublicReachable ? hostAddress.GetPort() : unreachableAddress.GetPort(), hostID ) ) );
			hostLobby->received.push_back(
					Serialize( new CClientWantToConnect( 0, teammateID, "127.0.0.1", unreachableAddress.GetPort() ) ) );
		}
		joiningLobby->sent.clear();
		for ( auto &stream : hostLobby->sent )
		{
			CPtr<CNetPacket> packet = Deserialize( stream );
			auto *envelope = dynamic_cast<CDirectPacketToClient *>( packet.GetPtr() );
			ASSERT_NE( envelope, nullptr );
			auto *offer = dynamic_cast<CClientGameConnectInfo *>( envelope->pPacket.GetPtr() );
			ASSERT_NE( offer, nullptr );
			if ( !bLocalReachable )
			{
				offer->nPort = unreachableAddress.GetPort();
			}
			EXPECT_EQ( envelope->nClient, teammateID );
			// Same sender rewrite performed by the unchanged CControlLobby.
			envelope->nClientID = hostID;
			std::swap( envelope->nClient, envelope->nClientID );
			if ( bForwardOffers )
			{
				joiningLobby->received.push_back( Serialize( envelope ) );
				++offersForwarded;
			}
		}
		hostLobby->sent.clear();
		while ( CPtr<CNetPacket> packet = host.GetPacket() )
		{
			if ( dynamic_cast<CNewGameClient *>( packet.GetPtr() ) )
			{
				EXPECT_EQ( packet->nClientID, teammateID );
				hostConnected = true;
			}
			if ( auto *ping = dynamic_cast<CPingPacket *>( packet.GetPtr() ) )
			{
				host.SendGamePacket( new CPingPacket( packet->nClientID, ping->nFromID, ping->nSendTime ), false );
			}
		}
		while ( CPtr<CNetPacket> packet = joining.GetPacket() )
		{
			if ( dynamic_cast<CNewGameClient *>( packet.GetPtr() ) )
			{
				EXPECT_EQ( packet->nClientID, hostID );
				joiningConnected = true;
			}
			if ( auto *ping = dynamic_cast<CPingPacket *>( packet.GetPtr() ) )
			{
				EXPECT_EQ( packet->nClientID, hostID );
				EXPECT_EQ( ping->nSendTime, 5678 );
				receivedEcho = true;
			}
			ASSERT_EQ( dynamic_cast<CConnectGameFailed *>( packet.GetPtr() ), nullptr );
		}
		if ( hostConnected && joiningConnected && !sentPing )
		{
			joining.SendGamePacket( new CPingPacket( hostID, 2, 5678 ), false );
			sentPing = true;
		}
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}
	EXPECT_EQ( offersForwarded > 0, bForwardOffers );
	EXPECT_TRUE( hostConnected );
	EXPECT_TRUE( joiningConnected );
	EXPECT_TRUE( receivedEcho );
}

TEST( ClientDirectConnections, CompleteSocketHandshakeAndPingUseLocalOfferWhenPublicEndpointCannotReply )
{
	CheckSocketHandshake( false, true, true );
}

TEST( ClientDirectConnections, CompleteSocketHandshakeRetriesPublicEndpointAfterLocalFailure )
{
	CheckSocketHandshake( true, false, true );
}

TEST( ClientDirectConnections, CompleteSocketHandshakeWorksWithoutLocalOffers )
{
	CheckSocketHandshake( true, false, false );
}

TEST( ClientDirectConnections, EarlyPeerPacketsAreDeliveredInOrderOnConnectionInstallation )
{
	CPtr<CRecordingDriver> lobbyDriver = new CRecordingDriver();
	CPtr<CRecordingDriver> peerDriver = new CRecordingDriver();
	peerDriver->expectedRecipient = 17;
	CPtr<CNet> lobby = CPlayGameProcessorTestAccess::CreateNet( lobbyDriver );
	CPtr<CNet> peer = CPlayGameProcessorTestAccess::CreateNet( peerDriver );
	CPtr<CRecordingGameLinks> links = new CRecordingGameLinks();
	CPtr<CSimpleServerEffort> effort = new CSimpleServerEffort( 42, hostID, "127.0.0.1", 43210, lobby, peer, links );
	CPlayGameProcessor processor( lobby, "127.0.0.1", 1, 0, 60 );
	CPlayGameProcessorTestAccess::AddEffort( processor, effort, peer );
	CPtr<CIndentityPacket> identity = new CIndentityPacket( 17, hostID );
	ASSERT_TRUE( effort->ProcessClientIdentity( identity ) );
	CPlayGameProcessorTestAccess::QueueEarlyPacket( processor, new CPingPacket( 18, 3, 900 ) );
	CPlayGameProcessorTestAccess::QueueEarlyPacket( processor, new CPingPacket( 17, 2, 1000 ) );
	CPlayGameProcessorTestAccess::QueueEarlyPacket( processor, new CPingPacket( 17, 2, 2000 ) );
	processor.Segment();
	ASSERT_TRUE( CPlayGameProcessorTestAccess::HasConnection( processor, hostID ) );
	for ( int timestamp : { 1000, 2000 } )
	{
		CPtr<CNetPacket> packet = processor.GetPacket();
		auto *ping = dynamic_cast<CPingPacket *>( packet.GetPtr() );
		ASSERT_NE( ping, nullptr );
		EXPECT_EQ( ping->nClientID, hostID );
		EXPECT_EQ( ping->nSendTime, timestamp );
	}
	EXPECT_EQ( processor.GetPacket(), nullptr );
	EXPECT_EQ( CPlayGameProcessorTestAccess::GetWaitingCount( processor ), 1u );
	EXPECT_TRUE( lobbyDriver->sent.empty() );
}

TEST( ClientDirectConnections, RoomPingBroadcastAndEchoUseOnlyPeerConnections )
{
	CPtr<CRecordingDriver> lobbyDriver = new CRecordingDriver();
	CPtr<CRecordingDriver> peerDriver = new CRecordingDriver();
	CPtr<CRecordingDriver> secondPeerDriver = new CRecordingDriver();
	CPtr<CNet> peer = CPlayGameProcessorTestAccess::CreateNet( peerDriver );
	CPtr<CNet> secondPeer = CPlayGameProcessorTestAccess::CreateNet( secondPeerDriver );
	CServerClient client;
	CPlayGameProcessorTestAccess::InitServerClient( client, lobbyDriver );
	CPlayGameProcessorTestAccess::AddConnection( client, hostID, new CSimpleConnection( peer, true, hostID, 0, 42 ) );
	CPlayGameProcessorTestAccess::AddConnection(
			client, teammateID, new CSimpleConnection( secondPeer, true, teammateID, 0, 42 ) );
	peerDriver->sent.clear();
	secondPeerDriver->sent.clear();
	client.SendGamePacket( new CPingPacket( 0, 2, 1234 ), true );
	ASSERT_EQ( peerDriver->sent.size(), 1u );
	ASSERT_EQ( secondPeerDriver->sent.size(), 1u );
	for ( auto *driver : { peerDriver.GetPtr(), secondPeerDriver.GetPtr() } )
	{
		CPtr<CNetPacket> packet = Deserialize( driver->sent.front() );
		ASSERT_NE( dynamic_cast<CPingPacket *>( packet.GetPtr() ), nullptr );
	}

	// Same unicast path used by CMPManagerMode::OnPingPacket to return an echo.
	peerDriver->received.push_back( Serialize( new CPingPacket( 0, 1, 5678 ) ) );
	client.Segment();
	CPtr<CNetPacket> packet = client.GetPacket();
	auto *incoming = dynamic_cast<CPingPacket *>( packet.GetPtr() );
	ASSERT_NE( incoming, nullptr );
	EXPECT_EQ( incoming->nClientID, hostID );
	client.SendGamePacket( new CPingPacket( incoming->nClientID, incoming->nFromID, incoming->nSendTime ), false );
	ASSERT_EQ( peerDriver->sent.size(), 2u );
	EXPECT_EQ( secondPeerDriver->sent.size(), 1u );
	packet = Deserialize( peerDriver->sent.back() );
	auto *echo = dynamic_cast<CPingPacket *>( packet.GetPtr() );
	ASSERT_NE( echo, nullptr );
	EXPECT_EQ( echo->nFromID, 1 );
	EXPECT_EQ( echo->nSendTime, 5678 );
	EXPECT_TRUE( lobbyDriver->sent.empty() );
}

TEST( ClientConnectionRemoval, HostFinalDropSurvivesDeathInSameServerBatch )
{
	CPlayGameProcessor processor( nullptr, "127.0.0.1", 1, 0, 60 );
	CPlayGameProcessorTestAccess::AddRelay( processor, hostID );
	ReceiveDrop( processor, hostID, 2, 7645 );
	// CServerClient drains the server batch before calling processor.Segment().
	ReceiveDeath( processor, hostID );
	EXPECT_FALSE( CPlayGameProcessorTestAccess::HasConnection( processor, hostID ) );
	ExpectDrop( processor, hostID, 2, 7645 );
	CPtr<CNetPacket> packet = processor.GetPacket();
	ASSERT_NE( dynamic_cast<CGameClientRemoved *>( packet.GetPtr() ), nullptr );
	EXPECT_EQ( packet->nClientID, hostID );
	EXPECT_EQ( processor.GetPacket(), nullptr );
}

TEST( ClientConnectionRemoval, FullServerReceiveBatchKeepsFinalDropBeforeDeath )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CServerClient client;
	CPlayGameProcessorTestAccess::InitServerClient( client, driver );
	CPlayGameProcessorTestAccess::AddRelay( client, hostID );
	driver->received.push_back(
			Serialize( new CThroughServerGamePacket( 0, hostID, new CB2DropPlayerAtSegmentPacket( 0, 2, 7645 ) ) ) );
	driver->received.push_back( Serialize( new CGameClientDead( 0, hostID ) ) );
	client.Segment();
	CPtr<CNetPacket> packet = client.GetPacket();
	auto *drop = dynamic_cast<CB2DropPlayerAtSegmentPacket *>( packet.GetPtr() );
	ASSERT_NE( drop, nullptr );
	EXPECT_EQ( drop->nClientID, hostID );
	EXPECT_EQ( drop->nSlotToDrop, 2 );
	EXPECT_EQ( drop->nSegment, 7645 );
	packet = client.GetPacket();
	ASSERT_NE( dynamic_cast<CGameClientRemoved *>( packet.GetPtr() ), nullptr );
	EXPECT_EQ( packet->nClientID, hostID );
	EXPECT_EQ( client.GetPacket(), nullptr );
}

TEST( ClientConnectionRemoval, AllQueuedPacketsKeepTheirOrderBeforeRemoval )
{
	CPlayGameProcessor processor( nullptr, "127.0.0.1", 1, 0, 60 );
	CPlayGameProcessorTestAccess::AddRelay( processor, hostID );
	ReceiveDrop( processor, hostID, 0, 7295 );
	ReceiveDrop( processor, hostID, 1, 7638 );
	ReceiveDrop( processor, hostID, 2, 7645 );
	ReceiveDeath( processor, hostID );
	ExpectDrop( processor, hostID, 0, 7295 );
	ExpectDrop( processor, hostID, 1, 7638 );
	ExpectDrop( processor, hostID, 2, 7645 );
	CPtr<CNetPacket> packet = processor.GetPacket();
	EXPECT_NE( dynamic_cast<CGameClientRemoved *>( packet.GetPtr() ), nullptr );
	EXPECT_EQ( processor.GetPacket(), nullptr );
}

TEST( ClientConnectionRemoval, KickNotificationFollowsQueuedControlPacket )
{
	CPlayGameProcessor processor( nullptr, "127.0.0.1", 1, 0, 60 );
	CPlayGameProcessorTestAccess::AddRelay( processor, hostID );
	ReceiveDrop( processor, hostID, 2, 7645 );
	CPtr<CNetPacket> kicked = new CGameClientWasKicked( 0, hostID );
	ASSERT_TRUE( processor.ProcessPacket( kicked ) );
	ExpectDrop( processor, hostID, 2, 7645 );
	CPtr<CNetPacket> packet = processor.GetPacket();
	EXPECT_EQ( packet.GetPtr(), kicked.GetPtr() );
	EXPECT_FALSE( CPlayGameProcessorTestAccess::HasConnection( processor, hostID ) );
	EXPECT_EQ( processor.GetPacket(), nullptr );
}

TEST( ClientConnectionRemoval, RemovingOneRelayPreservesOtherRelayQueue )
{
	CPlayGameProcessor processor( nullptr, "127.0.0.1", 1, 0, 60 );
	CPlayGameProcessorTestAccess::AddRelay( processor, hostID );
	CPlayGameProcessorTestAccess::AddRelay( processor, teammateID );
	ReceiveDrop( processor, hostID, 2, 7645 );
	ReceiveDrop( processor, teammateID, 4, 7646 );
	ReceiveDeath( processor, hostID );
	ExpectDrop( processor, hostID, 2, 7645 );
	CPtr<CNetPacket> removed = processor.GetPacket();
	ASSERT_NE( dynamic_cast<CGameClientRemoved *>( removed.GetPtr() ), nullptr );
	EXPECT_EQ( processor.GetPacket(), nullptr );
	EXPECT_TRUE( CPlayGameProcessorTestAccess::HasConnection( processor, teammateID ) );
	processor.Segment();
	ExpectDrop( processor, teammateID, 4, 7646 );
	EXPECT_EQ( processor.GetPacket(), nullptr );
}

TEST( ClientConnectionRemoval, GameKilledPreservesQueuedFinalDrop )
{
	CPlayGameProcessor processor( nullptr, "127.0.0.1", 1, 0, 60 );
	CPlayGameProcessorTestAccess::AddRelay( processor, hostID );
	ReceiveDrop( processor, hostID, 2, 7645 );
	CPtr<CNetPacket> killed = new CGameKilled( 0, 968 );
	ASSERT_TRUE( processor.ProcessPacket( killed ) );
	ExpectDrop( processor, hostID, 2, 7645 );
	CPtr<CNetPacket> packet = processor.GetPacket();
	EXPECT_EQ( packet.GetPtr(), killed.GetPtr() );
	EXPECT_FALSE( CPlayGameProcessorTestAccess::HasConnection( processor, hostID ) );
	EXPECT_EQ( processor.GetPacket(), nullptr );
}

TEST( ClientConnectionRemoval, PersistentControlEnvelopeIsSentAndSurvivesLocalLeave )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CServerClient client;
	CPlayGameProcessorTestAccess::InitServerClient( client, driver );
	CPlayGameProcessorTestAccess::AddRelay( client, teammateID );
	CPtr<CNetPacket> drop = new CB2DropPlayerAtSegmentPacket( 0, 2, 7645 );
	client.SendPacket( new CDirectPacketToClient( 0, teammateID, drop ) );
	client.SendPacket( new CLeaveGamePacket( 0, 968 ) );
	client.Segment();
	EXPECT_EQ( client.GetPacket(), nullptr ); // outgoing envelopes must never echo locally
	ASSERT_EQ( driver->sent.size(), 2u );
	CPtr<CNetPacket> forwarded = Deserialize( driver->sent[0] );
	auto *envelope = dynamic_cast<CDirectPacketToClient *>( forwarded.GetPtr() );
	ASSERT_NE( envelope, nullptr );
	EXPECT_EQ( envelope->nClient, teammateID );
	auto *finalDrop = dynamic_cast<CB2DropPlayerAtSegmentPacket *>( envelope->pPacket.GetPtr() );
	ASSERT_NE( finalDrop, nullptr );
	EXPECT_EQ( finalDrop->nSlotToDrop, 2 );
	EXPECT_EQ( finalDrop->nSegment, 7645 );
	CPtr<CNetPacket> leave = Deserialize( driver->sent[1] );
	ASSERT_NE( dynamic_cast<CLeaveGamePacket *>( leave.GetPtr() ), nullptr );
}

TEST( ClientConnectionRemoval, IncomingPersistentControlNeedsNoPeerConnection )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CServerClient client;
	CPlayGameProcessorTestAccess::InitServerClient( client, driver );
	CPtr<CNetPacket> envelope = new CDirectPacketToClient( 0, hostID, new CB2DropPlayerAtSegmentPacket( 0, 2, 7645 ) );
	driver->received.push_back( Serialize( envelope ) );
	client.Segment();
	CPtr<CNetPacket> packet = client.GetPacket();
	auto *drop = dynamic_cast<CB2DropPlayerAtSegmentPacket *>( packet.GetPtr() );
	ASSERT_NE( drop, nullptr );
	EXPECT_EQ( drop->nClientID, hostID );
	EXPECT_EQ( drop->nSlotToDrop, 2 );
	EXPECT_EQ( drop->nSegment, 7645 );
	EXPECT_EQ( client.GetPacket(), nullptr );
}

TEST( ClientConnectionRemoval, PausedPersistentEnvelopesRetainSeparateRecipients )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CServerClient client;
	CPlayGameProcessorTestAccess::InitServerClient( client, driver );
	client.TogglePause( true );
	CPtr<CNetPacket> drop = new CB2DropPlayerAtSegmentPacket( 0, 2, 7645 );
	client.SendPacket( new CDirectPacketToClient( 0, teammateID, drop ) );
	client.SendPacket( new CDirectPacketToClient( 0, teammateID + 1, drop ) );
	client.SendPacket( new CLeaveGamePacket( 0, 968 ) );
	EXPECT_TRUE( driver->sent.empty() );
	client.TogglePause( false );
	client.Segment();
	EXPECT_EQ( client.GetPacket(), nullptr );
	ASSERT_EQ( driver->sent.size(), 3u );
	for ( int i = 0; i < 2; ++i )
	{
		CPtr<CNetPacket> packet = Deserialize( driver->sent[i] );
		auto *envelope = dynamic_cast<CDirectPacketToClient *>( packet.GetPtr() );
		ASSERT_NE( envelope, nullptr );
		EXPECT_EQ( envelope->nClient, teammateID + i );
		EXPECT_NE( dynamic_cast<CB2DropPlayerAtSegmentPacket *>( envelope->pPacket.GetPtr() ), nullptr );
	}
	EXPECT_EQ( drop->nClientID, 0 );
}

TEST( ClientConnectionRemoval, ServerForwardsUnknownInnerControlWithoutChangingItsBytes )
{
	// Model an application packet unknown to the server, whose executable has
	// no GameX classes. The routing envelope is the only type it needs to know.
	const int unknownType = 0x6F001107;
	ASSERT_FALSE( NObjectFactory::IsRegistered( unknownType ) );
	CMemoryStream incoming;
	incoming << int( 40 ) << teammateID << unknownType << uint8_t( 2 ) << int( 7645 );
	CPtr<CNetPacket> packet = Deserialize( incoming );
	auto *envelope = dynamic_cast<CDirectPacketToClient *>( packet.GetPtr() );
	ASSERT_NE( envelope, nullptr );
	ASSERT_NE( envelope->pPacket.GetPtr(), nullptr );
	EXPECT_EQ( NObjectFactory::GetObjectTypeID( envelope->pPacket.GetPtr() ), UNKNOWN_PACKET_TYPE_ID );
	// The real CControlLobby performs this swap before serializing the reply.
	envelope->nClientID = hostID;
	std::swap( envelope->nClient, envelope->nClientID );
	CMemoryStream forwarded = Serialize( envelope );
	CMemoryStream expected;
	expected << int( 40 ) << hostID << unknownType << uint8_t( 2 ) << int( 7645 );
	ASSERT_EQ( forwarded.GetSize(), expected.GetSize() );
	EXPECT_EQ( std::memcmp( forwarded.GetBuffer(), expected.GetBuffer(), expected.GetSize() ), 0 );
}

TEST( ClientConnectionRemoval, DirectLogoutWaitsForFinalServerForwardAndOrderedConfirmation )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CServerClient client;
	CPlayGameProcessorTestAccess::InitServerClient( client, driver );
	CPtr<CQueuedDirectConnection> connection = new CQueuedDirectConnection();
	connection->packets.push_back( new CGameClientRemoved( hostID ) );
	connection->packets.push_back( new CGameClientRemoved( hostID ) );
	CPlayGameProcessorTestAccess::AddConnection( client, hostID, connection );
	client.Segment();

	EXPECT_EQ( client.GetPacket(), nullptr );
	EXPECT_FALSE( CPlayGameProcessorTestAccess::HasConnection( client, hostID ) );
	EXPECT_TRUE( CPlayGameProcessorTestAccess::IsAwaitingServerRemoval( client, hostID ) );
	ASSERT_EQ( driver->sent.size(), 1u );
	CPtr<CNetPacket> query = Deserialize( driver->sent.front() );
	auto *subscription = dynamic_cast<CThroughServerConnectionPacket *>( query.GetPtr() );
	ASSERT_NE( subscription, nullptr );
	EXPECT_EQ( subscription->nClientWith, hostID );
	EXPECT_EQ( subscription->nGameID, 968 );

	// The retired direct channel is no longer needed: the server stream carries
	// the final self-departure before confirming that the player left the room.
	driver->received.push_back(
			Serialize( new CDirectPacketToClient( 0, hostID, new CB2DropPlayerAtSegmentPacket( 0, 2, 7645 ) ) ) );
	driver->received.push_back( Serialize( new CGameClientDead( 0, hostID ) ) );
	client.Segment();
	CPtr<CNetPacket> packet = client.GetPacket();
	auto *drop = dynamic_cast<CB2DropPlayerAtSegmentPacket *>( packet.GetPtr() );
	ASSERT_NE( drop, nullptr );
	EXPECT_EQ( drop->nClientID, hostID );
	EXPECT_EQ( drop->nSegment, 7645 );
	packet = client.GetPacket();
	ASSERT_NE( dynamic_cast<CGameClientRemoved *>( packet.GetPtr() ), nullptr );
	EXPECT_EQ( packet->nClientID, hostID );
	EXPECT_EQ( client.GetPacket(), nullptr );
	EXPECT_FALSE( CPlayGameProcessorTestAccess::IsAwaitingServerRemoval( client, hostID ) );
	EXPECT_EQ( driver->sent.size(), 1u );
}

TEST( ClientConnectionRemoval, ServerConfirmationDrainsAStillConnectedDirectQueueWithoutAnotherQuery )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CServerClient client;
	CPlayGameProcessorTestAccess::InitServerClient( client, driver );
	CPtr<CQueuedDirectConnection> connection = new CQueuedDirectConnection();
	connection->packets.push_back( new CB2DropPlayerAtSegmentPacket( hostID, 2, 7645 ) );
	connection->packets.push_back( new CGameClientRemoved( hostID ) );
	CPlayGameProcessorTestAccess::AddConnection( client, hostID, connection );
	driver->received.push_back( Serialize( new CGameClientDead( 0, hostID ) ) );
	client.Segment();
	CPtr<CNetPacket> packet = client.GetPacket();
	ASSERT_NE( dynamic_cast<CB2DropPlayerAtSegmentPacket *>( packet.GetPtr() ), nullptr );
	packet = client.GetPacket();
	ASSERT_NE( dynamic_cast<CGameClientRemoved *>( packet.GetPtr() ), nullptr );
	EXPECT_EQ( client.GetPacket(), nullptr );
	EXPECT_TRUE( driver->sent.empty() );
	EXPECT_FALSE( CPlayGameProcessorTestAccess::HasConnection( client, hostID ) );
	EXPECT_FALSE( CPlayGameProcessorTestAccess::IsAwaitingServerRemoval( client, hostID ) );
}

TEST( ClientConnectionRemoval, BrokenDirectPathDoesNotRemoveAPlayerStillInTheServerRoom )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CServerClient client;
	CPlayGameProcessorTestAccess::InitServerClient( client, driver );
	CPtr<CQueuedDirectConnection> connection = new CQueuedDirectConnection();
	connection->packets.push_back( new CGameClientRemoved( hostID ) );
	CPlayGameProcessorTestAccess::AddConnection( client, hostID, connection );
	client.Segment();
	// A still-present player produces no immediate server death notification.
	// Ordinary lockstep timeout policy handles the broken path in the meantime.
	for ( int nFrame = 0; nFrame < 10; ++nFrame )
	{
		client.Segment();
		EXPECT_EQ( client.GetPacket(), nullptr );
	}
	EXPECT_TRUE( CPlayGameProcessorTestAccess::IsAwaitingServerRemoval( client, hostID ) );
	EXPECT_EQ( driver->sent.size(), 1u );
	client.SendPacket( new CLeaveGamePacket( 0, 968 ) );
	EXPECT_FALSE( CPlayGameProcessorTestAccess::IsAwaitingServerRemoval( client, hostID ) );
}

TEST( ClientConnectionRemoval, UnknownServerDeathDoesNotInventARemoval )
{
	CPlayGameProcessor processor( nullptr, "127.0.0.1", 1, 0, 60 );
	CPtr<CNetPacket> packet = new CGameClientDead( 0, hostID );
	EXPECT_FALSE( processor.ProcessPacket( packet ) );
	EXPECT_EQ( processor.GetPacket(), nullptr );
}
