#include "Net/stdafx.h"
#include "Net/NetA4.h"

#include <gtest/gtest.h>
#include <deque>
#include <map>

namespace NNet
{
// Only the clock and connection setup are synthetic; tests execute the real
// driver Step and incoming packet parser, with no background thread or sleeps.
struct CNetDriverTestAccess
{
	static void Init( CNetDriver &driver, ILinksManager *pLinks )
	{
		driver.pLinks = pLinks;
		driver.bStopRequested = false;
		driver.bIsClient = false;
		driver.bAcceptNewClients = false;
		driver.nSent = 0;
		driver.state = IDriver::ACTIVE;
		NHPTimer::GetTime( &driver.lastTime );
		// Initialize login retry state without opening a socket.
		driver.login.StartLogin( CNodeAddress(), CMemoryStream() );
	}

	static void AddPeer( CNetDriver &driver, unsigned int id, const CNodeAddress &address )
	{
		CNodeAddressSet localAddresses;
		localAddresses.Clear();
		driver.AddNewP2PClient( CNetDriver::SClientAddressInfo( address, localAddresses ), id );
		driver.ProcessP2PMessages();
		driver.msgQueue.clear();
		driver.GetClient( id )->bTryShortcut = false;
	}

	static void Elapse( CNetDriver &driver, double seconds )
	{
		NHPTimer::GetTime( &driver.lastTime );
		driver.lastTime -= static_cast<NHPTimer::STime>( seconds * NHPTimer::GetClockRate() );
	}
};
}

namespace
{
using namespace NNet;

class CQueuedLinks : public ILinksManager
{
	OBJECT_NOCOPY_METHODS( CQueuedLinks );
public:
	struct Datagram
	{
		CNodeAddress address;
		CMemoryStream data;
	};
	mutable std::deque<Datagram> incoming;
	mutable std::vector<Datagram> outgoing;

	bool MakeBroadcastAddr( CNodeAddress *, int ) const override { return false; }
	bool IsLocalAddr( const CNodeAddress & ) const override { return false; }
	bool GetSelfAddress( CNodeAddressSet *pResult ) const override
	{
		pResult->Clear();
		return true;
	}
	bool Send( const CNodeAddress &destination, CMemoryStream &packet ) const override
	{
		outgoing.push_back( { destination, packet } );
		return true;
	}
	bool Recv( CNodeAddress *pSource, CMemoryStream *pPacket ) const override
	{
		if ( incoming.empty() )
			return false;
		*pSource = incoming.front().address;
		*pPacket = incoming.front().data;
		pPacket->Seek( 0 );
		incoming.pop_front();
		return true;
	}
};

CNodeAddress Address( int port )
{
	CNodeAddress result;
	result.SetInetName( "127.0.0.1", port );
	return result;
}

CMemoryStream Logout( unsigned int clientID )
{
	CMemoryStream packet;
	packet << static_cast<unsigned char>( 6 ); // LOGOUT
	packet << clientID;
	return packet;
}

CMemoryStream Normal( unsigned int clientID, const std::vector<unsigned char> &payloads )
{
	CStreamTracker sender;
	for ( unsigned char payload : payloads )
	{
		CMemoryStream &frame = sender.outList.emplace_back();
		frame << static_cast<unsigned char>( 5 ); // one-byte header: two payload bytes
		frame << static_cast<unsigned char>( 3 ); // PKT_DIRECT_MSG
		frame << payload;
	}
	CMemoryStream packet;
	CBitLocker bits;
	bits.LockWrite( packet, N_MAX_PACKET_SIZE );
	bits.Write( static_cast<unsigned char>( 0 ) ); // NORMAL
	bits.Write( clientID );
	CAckTracker acks;
	const PACKET_ID id = acks.WrtieAcks( &bits, N_MAX_PACKET_SIZE );
	sender.WriteMsg( id, &bits, 500 );
	bits.Free();
	packet.SetSize( packet.GetPosition() );
	return packet;
}

std::vector<unsigned char> ReadDirectMessages( CNetDriver &driver )
{
	std::vector<unsigned char> result;
	IDriver::EMessage type;
	int clientID;
	CMemoryStream packet;
	while ( driver.GetMessage( &type, &clientID, nullptr, &packet ) )
	{
		if ( type == IDriver::DIRECT )
		{
			unsigned char value;
			packet.Seek( 0 );
			packet >> value;
			result.push_back( value );
		}
	}
	return result;
}
}

TEST( NetTransport, ShortFinalMessageDoesNotWaitForAnotherPacket )
{
	CPtr<CQueuedLinks> links = new CQueuedLinks();
	CNetDriver driver( SNetDriverConsts( 60 ), false );
	CNetDriverTestAccess::Init( driver, links );
	const CNodeAddress peer = Address( 12001 );
	CNetDriverTestAccess::AddPeer( driver, 17, peer );
	links->incoming.push_back( { peer, Normal( 17, { 42 } ) } );
	driver.Step();
	EXPECT_EQ( ReadDirectMessages( driver ), (std::vector<unsigned char>{ 42 }) );
}

TEST( NetTransport, CompleteMessagesAreDeliveredBeforeFollowingLogout )
{
	CPtr<CQueuedLinks> links = new CQueuedLinks();
	CNetDriver driver( SNetDriverConsts( 60 ), false );
	CNetDriverTestAccess::Init( driver, links );
	const CNodeAddress peer = Address( 12002 );
	CNetDriverTestAccess::AddPeer( driver, 17, peer );
	links->incoming.push_back( { peer, Normal( 17, { 10, 20, 30 } ) } );
	links->incoming.push_back( { peer, Logout( 17 ) } );
	driver.Step();
	EXPECT_EQ( ReadDirectMessages( driver ), (std::vector<unsigned char>{ 10, 20, 30 }) );
}

TEST( NetTransport, OldLogoutRecipientIDCannotRemoveAnotherConnection )
{
	CPtr<CQueuedLinks> links = new CQueuedLinks();
	CNetDriver driver( SNetDriverConsts( 60 ), false );
	CNetDriverTestAccess::Init( driver, links );
	const CNodeAddress departing = Address( 12003 );
	CNetDriverTestAccess::AddPeer( driver, 17, departing );
	CNetDriverTestAccess::AddPeer( driver, 23, Address( 12004 ) );
	links->incoming.push_back( { departing, Logout( 23 ) } );
	driver.Step();
	EXPECT_LT( driver.GetTimeSinceLastRecv( 17 ), 0 );
	EXPECT_GE( driver.GetTimeSinceLastRecv( 23 ), 0 );
}

TEST( NetTransport, LogoutIdentifiesTheDepartingDriver )
{
	CPtr<CQueuedLinks> links = new CQueuedLinks();
	{
		CNetDriver driver( SNetDriverConsts( 60 ), false );
		CNetDriverTestAccess::Init( driver, links );
		CNetDriverTestAccess::AddPeer( driver, 17, Address( 12005 ) );
		CNetDriverTestAccess::AddPeer( driver, 23, Address( 12006 ) );
	}
	ASSERT_EQ( links->outgoing.size(), 2u );
	for ( auto &datagram : links->outgoing )
	{
		datagram.data.Seek( 0 );
		unsigned char type;
		unsigned int sender;
		datagram.data >> type;
		datagram.data >> sender;
		EXPECT_EQ( type, 6 );
		EXPECT_EQ( sender, 0u ); // the host's ID, independent of either recipient
	}
}

TEST( NetTransport, PacketReceivedAfterLocalStallResetsTimeout )
{
	CPtr<CQueuedLinks> links = new CQueuedLinks();
	CNetDriver driver( SNetDriverConsts( 60 ), false );
	CNetDriverTestAccess::Init( driver, links );
	const CNodeAddress peer = Address( 12007 );
	CNetDriverTestAccess::AddPeer( driver, 17, peer );
	CNetDriverTestAccess::Elapse( driver, 65 );
	links->incoming.push_back( { peer, Normal( 17, {} ) } );
	driver.Step();
	EXPECT_GE( driver.GetTimeSinceLastRecv( 17 ), 0 );
	EXPECT_LT( driver.GetTimeSinceLastRecv( 17 ), 1 );
	driver.Step();
	EXPECT_GE( driver.GetTimeSinceLastRecv( 17 ), 0 );
}

TEST( NetTransport, SilentPeerStillTimesOut )
{
	CPtr<CQueuedLinks> links = new CQueuedLinks();
	CNetDriver driver( SNetDriverConsts( 60 ), false );
	CNetDriverTestAccess::Init( driver, links );
	CNetDriverTestAccess::AddPeer( driver, 17, Address( 12008 ) );
	CNetDriverTestAccess::Elapse( driver, 65 );
	driver.Step();
	driver.Step();
	EXPECT_LT( driver.GetTimeSinceLastRecv( 17 ), 0 );
}

TEST( NetTransport, KickNotificationHasNoRemoteSender )
{
	CPtr<CQueuedLinks> links = new CQueuedLinks();
	CNetDriver driver( SNetDriverConsts( 60 ), false );
	CNetDriverTestAccess::Init( driver, links );
	const CNodeAddress peer = Address( 12009 );
	CNetDriverTestAccess::AddPeer( driver, 17, peer );
	CMemoryStream kick;
	kick << static_cast<unsigned char>( 9 ); // KICK
	kick << static_cast<unsigned int>( 17 );
	links->incoming.push_back( { peer, kick } );
	driver.Step();
	IDriver::EMessage type;
	int sender = 12345;
	CMemoryStream packet;
	ASSERT_TRUE( driver.GetMessage( &type, &sender, nullptr, &packet ) );
	EXPECT_EQ( type, IDriver::KICKED );
	EXPECT_EQ( sender, -1 );
}

TEST( NetTransport, ReorderedAcknowledgementsAtBitmapBoundaryAreNotLost )
{
	// ACK windows can move backwards even while the containing UDP packets
	// arrive in order. Test both sides of the exact 32-bit shift boundary.
	for ( int shift : { 31, 32, 33 } )
	{
		SCOPED_TRACE( shift );
		CAckTracker sender;
		std::vector<PACKET_ID> sent;
		// The first outgoing ID is clock-seeded. Keep this case away from the
		// initial zero ACK and sequence wrap so it isolates bitmap shifting.
		while ( sent.size() < static_cast<size_t>( shift + 2 ) )
		{
			unsigned char header[8];
			CBitStream bits( header, CBitStream::write, sizeof(header) );
			const PACKET_ID id = sender.WrtieAcks( &bits, 500 );
			if ( !sent.empty() || ( id >= 64 && id < 65500 ) )
				sent.push_back( id );
		}
		const auto receive = [&sender]( PACKET_ID incomingID, PACKET_ID lastAcknowledged )
		{
			unsigned char header[8];
			CBitStream output( header, CBitStream::write, sizeof(header) );
			output.Write( incomingID );
			output.Write( lastAcknowledged );
			output.Write( uint32_t( 1 ) ); // also acknowledge the previous packet
			CBitStream input( header, CBitStream::read, sizeof(header) );
			std::vector<PACKET_ID> acknowledged;
			EXPECT_TRUE( sender.ReadAcks( &acknowledged, input ) );
			return acknowledged;
		};
		const auto newest = receive( 100, sent.back() );
		EXPECT_EQ( newest.size(), 2u );
		const auto older = receive( 101, sent[1] );
		EXPECT_NE( std::find( older.begin(), older.end(), sent[0] ), older.end() );
		EXPECT_NE( std::find( older.begin(), older.end(), sent[1] ), older.end() );
	}
}

TEST( NetTransport, ThreePeersAcknowledgeBroadcastWithDifferentLocalPeerIDs )
{
	std::map<unsigned int, CP2PTracker> peers;
	peers.try_emplace( 17 );
	peers.try_emplace( 23 );
	peers.try_emplace( 41 );
	CMemoryStream addressInfo;
	for ( auto &sender : peers )
		for ( const auto &receiver : peers )
			if ( sender.first != receiver.first )
				sender.second.AddNewClient( receiver.first, addressInfo, true );

	const auto exchange = [&peers]()
	{
		for ( int round = 0; round < 10; ++round )
		{
			bool delivered = false;
			for ( auto &sender : peers )
			{
				std::vector<CP2PTracker::SPacket> packets;
				packets.swap( sender.second.packets );
				for ( auto &packet : packets )
				{
					peers.at( packet.addr ).ProcessPacket( sender.first, packet.pkt, true );
					delivered = true;
				}
			}
			if ( !delivered )
				return;
		}
		FAIL() << "Peer protocol did not settle";
	};
	exchange();
	for ( auto &peer : peers )
	{
		CP2PTracker::SMessage message;
		while ( peer.second.GetMessage( &message ) ) {}
	}

	CMemoryStream payload;
	payload << static_cast<unsigned char>( 42 );
	peers.at( 17 ).SendBroadcast( payload );
	exchange();
	for ( unsigned int receiver : { 23u, 41u } )
	{
		CP2PTracker::SMessage message;
		ASSERT_TRUE( peers.at( receiver ).GetMessage( &message ) );
		EXPECT_EQ( message.msg, CP2PTracker::BROADCAST );
		EXPECT_EQ( message.from, 17u );
		ASSERT_EQ( message.received.size(), 1u );
		EXPECT_EQ( message.received.front(), receiver == 23 ? 41u : 23u );
		EXPECT_FALSE( peers.at( receiver ).GetMessage( &message ) );
	}
}
