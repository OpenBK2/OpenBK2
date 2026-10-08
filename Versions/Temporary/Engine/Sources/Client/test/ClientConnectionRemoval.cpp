#include "Client/stdafx.h"
#include "Client/ConnectionInternal.h"
#include "Client/PlayGameProcessor.h"
#include "GameX/MultiplayerNetPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/NetSaver.h"

#include <gtest/gtest.h>

// Keep the receive converter private in production while testing the actual
// public SendPacket path, including its dispatch through that converter.
#include "../ServerClient.cpp"

REGISTER_SAVELOAD_CLASS( CLIENT, 107, CB2DropPlayerAtSegmentPacket );

struct CPlayGameProcessorTestAccess
{
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
	void Init( NNet::APPLICATION_ID, int, bool, NNet::ILinksManager * ) override {}
	EState GetState() const override { return ACTIVE; }
	EReject GetRejectReason() const override { return NONE; }
	void ConnectGame( const NNet::CNodeAddress &, const CMemoryStream & ) override {}
	void StartGame() override {}
	void StartGameInfoSend( const SGameInfo & ) override {}
	void StopGameInfoSend() override {}
	void StartNewPlayerAccept() override {}
	void StopNewPlayerAccept() override {}
	bool GetGameInfo( int, NNet::CNodeAddress *, bool *, float *, SGameInfo * ) override { return false; }
	void RefreshServersList() override {}
	bool SendBroadcast( const CMemoryStream & ) override { return false; }
	bool SendDirect( int clientID, const CMemoryStream &packet ) override
	{
		EXPECT_EQ( clientID, 0 ); // every control packet uses the persistent server link
		sent.push_back( packet );
		return true;
	}
	void Kick( int ) override {}
	bool GetMessage( EMessage *type, int *clientID, std::vector<int> *, CMemoryStream *packet ) override
	{
		if ( received.empty() )
			return false;
		*type = DIRECT;
		*clientID = 0;
		*packet = received.front();
		received.pop_front();
		return true;
	}
	const float GetPing( int ) override { return 0; }
	const float GetTimeSinceLastRecv( int ) override { return 0; }
	void Step() override {}
	int GetSelfClientID() override { return 0; }
	const std::string GetIP( int ) override { return "127.0.0.1"; }
	const int GetPort( int ) override { return 0; }
};

void ReceiveDrop( CPlayGameProcessor &processor, int sender, int slot, int segment )
{
	CPtr<CNetPacket> forwarded = new CThroughServerGamePacket( 0, sender,
		new CB2DropPlayerAtSegmentPacket( 0, slot, segment ) );
	CPtr<CNetPacket> received = RoundTrip( forwarded );
	ASSERT_TRUE( processor.ProcessPacket( received ) );
}

void ExpectDrop( CPlayGameProcessor &processor, int sender, int slot, int segment )
{
	CPtr<CNetPacket> packet = processor.GetPacket();
	auto *drop = dynamic_cast<CB2DropPlayerAtSegmentPacket*>( packet.GetPtr() );
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
	ASSERT_NE( dynamic_cast<CGameClientRemoved*>( packet.GetPtr() ), nullptr );
	EXPECT_EQ( packet->nClientID, hostID );
	EXPECT_EQ( processor.GetPacket(), nullptr );
}

TEST( ClientConnectionRemoval, FullServerReceiveBatchKeepsFinalDropBeforeDeath )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CServerClient client;
	CPlayGameProcessorTestAccess::InitServerClient( client, driver );
	CPlayGameProcessorTestAccess::AddRelay( client, hostID );
	driver->received.push_back( Serialize( new CThroughServerGamePacket( 0, hostID,
		new CB2DropPlayerAtSegmentPacket( 0, 2, 7645 ) ) ) );
	driver->received.push_back( Serialize( new CGameClientDead( 0, hostID ) ) );
	client.Segment();
	CPtr<CNetPacket> packet = client.GetPacket();
	auto *drop = dynamic_cast<CB2DropPlayerAtSegmentPacket*>( packet.GetPtr() );
	ASSERT_NE( drop, nullptr );
	EXPECT_EQ( drop->nClientID, hostID );
	EXPECT_EQ( drop->nSlotToDrop, 2 );
	EXPECT_EQ( drop->nSegment, 7645 );
	packet = client.GetPacket();
	ASSERT_NE( dynamic_cast<CGameClientRemoved*>( packet.GetPtr() ), nullptr );
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
	EXPECT_NE( dynamic_cast<CGameClientRemoved*>( packet.GetPtr() ), nullptr );
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
	ASSERT_NE( dynamic_cast<CGameClientRemoved*>( removed.GetPtr() ), nullptr );
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
	auto *envelope = dynamic_cast<CDirectPacketToClient*>( forwarded.GetPtr() );
	ASSERT_NE( envelope, nullptr );
	EXPECT_EQ( envelope->nClient, teammateID );
	auto *finalDrop = dynamic_cast<CB2DropPlayerAtSegmentPacket*>( envelope->pPacket.GetPtr() );
	ASSERT_NE( finalDrop, nullptr );
	EXPECT_EQ( finalDrop->nSlotToDrop, 2 );
	EXPECT_EQ( finalDrop->nSegment, 7645 );
	CPtr<CNetPacket> leave = Deserialize( driver->sent[1] );
	ASSERT_NE( dynamic_cast<CLeaveGamePacket*>( leave.GetPtr() ), nullptr );
}

TEST( ClientConnectionRemoval, IncomingPersistentControlNeedsNoPeerConnection )
{
	CPtr<CRecordingDriver> driver = new CRecordingDriver();
	CServerClient client;
	CPlayGameProcessorTestAccess::InitServerClient( client, driver );
	CPtr<CNetPacket> envelope = new CDirectPacketToClient( 0, hostID,
		new CB2DropPlayerAtSegmentPacket( 0, 2, 7645 ) );
	driver->received.push_back( Serialize( envelope ) );
	client.Segment();
	CPtr<CNetPacket> packet = client.GetPacket();
	auto *drop = dynamic_cast<CB2DropPlayerAtSegmentPacket*>( packet.GetPtr() );
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
		auto *envelope = dynamic_cast<CDirectPacketToClient*>( packet.GetPtr() );
		ASSERT_NE( envelope, nullptr );
		EXPECT_EQ( envelope->nClient, teammateID + i );
		EXPECT_NE( dynamic_cast<CB2DropPlayerAtSegmentPacket*>( envelope->pPacket.GetPtr() ), nullptr );
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
	auto *envelope = dynamic_cast<CDirectPacketToClient*>( packet.GetPtr() );
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
