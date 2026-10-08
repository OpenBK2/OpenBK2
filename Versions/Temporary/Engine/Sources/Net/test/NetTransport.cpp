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

CMemoryStream NormalBlock( unsigned int clientID, PACKET_ID packetID, unsigned int offset,
	const unsigned char *data, unsigned char size )
{
	CMemoryStream packet;
	packet << static_cast<unsigned char>( 0 ) << clientID; // NORMAL
	packet << packetID << PACKET_ID( 0 ) << uint32_t( 0 ); // packet ID and ACK header
	packet << offset << size;
	packet.Write( data, size );
	return packet;
}

std::vector<unsigned char> Payload( int id, int size )
{
	std::vector<unsigned char> result( size );
	for ( int i = 0; i < size; ++i )
		result[i] = static_cast<unsigned char>( id + i * 37 );
	return result;
}

std::vector<unsigned char> LargeFrames( int count, int payloadSize )
{
	CMemoryStream stream;
	for ( int i = 0; i < count; ++i )
	{
		stream << int( ( payloadSize + 1 ) << 1 );
		stream << static_cast<unsigned char>( 3 ); // PKT_DIRECT_MSG
		const auto payload = Payload( i + 1, payloadSize );
		stream.Write( payload.data(), payload.size() );
	}
	const auto *bytes = reinterpret_cast<const unsigned char*>( stream.GetBuffer() );
	return { bytes, bytes + stream.GetSize() };
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

using DirectMessage = std::pair<int, std::vector<unsigned char>>;
std::vector<DirectMessage> ReadFullDirectMessages( CNetDriver &driver )
{
	std::vector<DirectMessage> result;
	IDriver::EMessage type;
	int clientID;
	CMemoryStream packet;
	while ( driver.GetMessage( &type, &clientID, nullptr, &packet ) )
	{
		if ( type == IDriver::DIRECT )
		{
			const auto *bytes = reinterpret_cast<const unsigned char*>( packet.GetBuffer() );
			result.emplace_back( clientID, std::vector<unsigned char>( bytes, bytes + packet.GetSize() ) );
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

TEST( NetTransport, BufferedBurstLargerThanRingDrainsWithoutAnotherDatagram )
{
	CPtr<CQueuedLinks> links = new CQueuedLinks();
	CNetDriver driver( SNetDriverConsts( 60 ), false );
	CNetDriverTestAccess::Init( driver, links );
	const CNodeAddress peer = Address( 12100 );
	CNetDriverTestAccess::AddPeer( driver, 17, peer );
	const CNodeAddress otherPeer = Address( 12102 );
	CNetDriverTestAccess::AddPeer( driver, 23, otherPeer );
	// Two large frames can be queued within the game's two-tick send window.
	const auto data = LargeFrames( 2, 20000 );
	PACKET_ID packetID = 100;
	// A missing first block leaves later, acknowledged stream data buffered.
	for ( size_t offset = 255; offset < data.size(); offset += 255 )
	{
		const unsigned char size = static_cast<unsigned char>( (std::min)( size_t( 255 ), data.size() - offset ) );
		links->incoming.push_back( { peer, NormalBlock( 17, packetID++, offset, data.data() + offset, size ) } );
	}
	links->incoming.push_back( { otherPeer, Normal( 23, { 42 } ) } );
	// Retransmitting the missing block closes the gap. No heartbeat or new data
	// follows: the ring must be pumped again as complete frames are consumed.
	links->incoming.push_back( { peer, NormalBlock( 17, packetID++, 0, data.data(), 255 ) } );
	driver.Step();
	const auto received = ReadFullDirectMessages( driver );
	ASSERT_EQ( received.size(), 3u );
	EXPECT_EQ( received[0], (DirectMessage{ 23, { 42 } }) );
	EXPECT_EQ( received[1], (DirectMessage{ 17, Payload( 1, 20000 ) }) );
	EXPECT_EQ( received[2], (DirectMessage{ 17, Payload( 2, 20000 ) }) );
}

TEST( NetTransport, MaximumLegalFrameAcceptsItsLastFragment )
{
	CPtr<CQueuedLinks> links = new CQueuedLinks();
	CNetDriver driver( SNetDriverConsts( 60 ), false );
	CNetDriverTestAccess::Init( driver, links );
	const CNodeAddress peer = Address( 12101 );
	CNetDriverTestAccess::AddPeer( driver, 17, peer );
	// WritePacket permits a P2P payload below N_STREAM_BUFFER - 1000 bytes.
	const auto data = LargeFrames( 1, N_STREAM_BUFFER - 1002 );
	const size_t partialSize = 31750;
	PACKET_ID packetID = 100;
	for ( size_t offset = 0; offset < partialSize; offset += 255 )
	{
		const unsigned char size = static_cast<unsigned char>( (std::min)( size_t( 255 ), partialSize - offset ) );
		links->incoming.push_back( { peer, NormalBlock( 17, packetID++, offset, data.data() + offset, size ) } );
	}
	driver.Step();
	EXPECT_TRUE( ReadDirectMessages( driver ).empty() );
	// Only 1017 ring bytes remain free, but this final block is just 21 bytes.
	links->incoming.push_back( { peer, NormalBlock( 17, packetID++, partialSize,
		data.data() + partialSize, static_cast<unsigned char>( data.size() - partialSize ) ) } );
	driver.Step();
	const auto received = ReadFullDirectMessages( driver );
	ASSERT_EQ( received.size(), 1u );
	EXPECT_EQ( received[0], (DirectMessage{ 17, Payload( 1, N_STREAM_BUFFER - 1002 ) }) );
}

TEST( NetTransport, ReliableStreamSurvivesLossReorderingAndSequenceWrap )
{
	struct Endpoint
	{
		CAckTracker acks;
		CStreamTracker stream;

		CMemoryStream Send()
		{
			CMemoryStream packet;
			CBitLocker bits;
			bits.LockWrite( packet, N_MAX_PACKET_SIZE );
			const PACKET_ID id = acks.WrtieAcks( &bits, 500 );
			stream.WriteMsg( id, &bits, 500 );
			bits.Free();
			packet.SetSize( packet.GetPosition() );
			return packet;
		}
		bool Receive( CMemoryStream &packet )
		{
			CBitStream bits( packet.GetBufferForWrite(), CBitStream::read, packet.GetSize() );
			std::vector<PACKET_ID> acknowledged;
			if ( !acks.ReadAcks( &acknowledged, bits ) )
				return false;
			stream.ReadMsg( bits );
			stream.Commit( acknowledged );
			return true;
		}
		int Tick()
		{
			std::vector<PACKET_ID> rolled, erased;
			acks.Step( &rolled, &erased, 0.05, 20 );
			stream.Rollback( rolled );
			stream.Erase( erased );
			return static_cast<int>( rolled.size() );
		}
	};
	Endpoint sender, receiver;
	// Exercise a complete 16-bit sequence cycle regardless of the clock seed.
	// Both directions exchange real ACKs, keeping their send windows bounded.
	for ( int i = 0; i < 65540; ++i )
	{
		auto packet = sender.Send();
		ASSERT_TRUE( receiver.Receive( packet ) );
		packet = receiver.Send();
		ASSERT_TRUE( sender.Receive( packet ) );
	}

	const int frameSize = 20005;
	const auto expected = LargeFrames( 10, frameSize - 5 );
	for ( size_t offset = 0; offset < expected.size(); offset += frameSize )
		sender.stream.outList.emplace_back().Write( expected.data() + offset, frameSize );
	std::vector<std::pair<int, CMemoryStream>> delayed;
	std::vector<unsigned char> received;
	unsigned int highestOffset = 0;
	int sent = 0, acknowledgements = 0, rolledBack = 0, dropped = 0;
	for ( int round = 0; round < 10000; ++round )
	{
		rolledBack += sender.Tick();
		receiver.Tick();
		if ( sender.acks.CanSend() && ( sender.stream.HasOutData() || sender.acks.NeedSend() ) )
		{
			auto packet = sender.Send();
			packet.Seek( 8 ); // ACK header precedes the stream block.
			unsigned int offset;
			unsigned char size;
			packet >> offset >> size;
			highestOffset = (std::max)( highestOffset, offset );
			++sent;
			// Keep one gap open until more than a ring has arrived, and also
			// lose ordinary packets so real timers must roll back their blocks.
			if ( ( size && offset == 0 && highestOffset < 40000 ) || sent % 11 == 0 )
				++dropped;
			else
			{
				delayed.emplace_back( round + ( sent % 7 == 0 ? 3 : 0 ), packet );
				if ( sent % 13 == 0 )
					delayed.emplace_back( round + 1, packet ); // duplicate delivery
			}
		}
		// Reverse delivery order for packets whose delays expire together.
		for ( int i = static_cast<int>( delayed.size() ) - 1; i >= 0; --i )
		{
			if ( delayed[i].first <= round )
			{
				receiver.Receive( delayed[i].second );
				delayed.erase( delayed.begin() + i );
			}
		}
		for (;;)
		{
			receiver.stream.PumpIncoming();
			unsigned char bytes[1537];
			const int size = receiver.stream.channelInBuf.Read( bytes, sizeof(bytes) );
			if ( !size )
				break;
			received.insert( received.end(), bytes, bytes + size );
		}
		if ( receiver.acks.CanSend() )
		{
			auto packet = receiver.Send();
			if ( ++acknowledgements % 5 != 0 )
				sender.Receive( packet ); // lose some ACK datagrams as well
		}
		// HasOutData excludes blocks still in flight; a dropped final block
		// must get its timeout and retransmission before the test can finish.
		if ( received.size() == expected.size() && !sender.stream.HasOutData() && delayed.empty() )
			break;
	}
	EXPECT_GT( dropped, 0 );
	EXPECT_GT( rolledBack, 0 );
	EXPECT_FALSE( sender.stream.HasOutData() );
	EXPECT_EQ( received, expected );
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
