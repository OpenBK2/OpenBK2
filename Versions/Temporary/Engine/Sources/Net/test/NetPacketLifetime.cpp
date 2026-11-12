#include "Net/stdafx.h"

#include "Net/NetStream.h"

#include <gtest/gtest.h>

namespace
{
using namespace NNet;

struct SDatagram
{
	CMemoryStream bytes;
	PACKET_ID id;
};

struct SEndpoint
{
	CAckTracker acks;
	CStreamTracker stream;

	void Queue( unsigned char value )
	{
		stream.outList.emplace_back().Write( &value, 1 );
	}

	SDatagram Send( int nInitialPacketID = -1 )
	{
		SDatagram result;
		for ( ;; )
		{
			result.bytes.Seek( 0 );
			CBitLocker bits;
			bits.LockWrite( result.bytes, N_MAX_PACKET_SIZE );
			result.id = acks.WrtieAcks( &bits, 500 );
			if ( nInitialPacketID >= 0 && result.id != nInitialPacketID )
			{
				bits.Free();
				// Seed the first transmission through the public ACK API without
				// depending on the constructor's wall-clock packet number.
				acks.PacketLost( result.id );
				continue;
			}
			stream.WriteMsg( result.id, &bits, 500 );
			bits.Free();
			result.bytes.SetSize( result.bytes.GetPosition() );
			return result;
		}
	}

	void Receive( SDatagram packet )
	{
		CBitStream bits( packet.bytes.GetBufferForWrite(), CBitStream::read, packet.bytes.GetSize() );
		std::vector<PACKET_ID> acked;
		if ( acks.ReadAcks( &acked, bits ) )
		{
			stream.ReadMsg( bits );
			stream.Commit( acked );
		}
	}

	void Tick( double fSeconds = 0.05 )
	{
		std::vector<PACKET_ID> rolled, erased;
		acks.Step( &rolled, &erased, fSeconds, 20 );
		stream.Rollback( rolled );
		stream.Erase( erased );
	}

	void SendFailed( PACKET_ID id )
	{
		// The driver's immediate Send(false) path differs from a timed rollback:
		// the ACK tracker removes this packet instead of retaining it for expiry.
		acks.PacketLost( id );
		stream.Rollback( { id } );
	}

	int Read()
	{
		unsigned char value;
		return stream.channelInBuf.Read( &value, 1 ) ? value : -1;
	}
};

enum ERecovery
{
	IMMEDIATE_SEND_FAILURE,
	REPEATED_SEND_FAILURES,
	TIMED_ROLLBACK,
	ROLLBACK_AND_EXPIRY
};

void CheckRecoveryAfterPacketNumberWrap( ERecovery recovery, bool bEmptyReusedPacket = false )
{
	SEndpoint sender, receiver;
	sender.Queue( 42 );
	const SDatagram original = sender.Send( 100 );
	const int nRetries = recovery == REPEATED_SEND_FAILURES ? 3 : 1;
	if ( recovery == TIMED_ROLLBACK || recovery == ROLLBACK_AND_EXPIRY )
	{
		// A long local stall can both roll back and erase the original ACK
		// record before WriteMsg has even created the reassignment.
		sender.Tick( recovery == TIMED_ROLLBACK ? 4 : 25 );
	}
	else
	{
		sender.SendFailed( original.id );
		for ( int i = 1; i < nRetries; ++i )
		{
			sender.SendFailed( sender.Send().id );
		}
	}
	receiver.Receive( sender.Send() );
	sender.Receive( receiver.Send() );
	ASSERT_EQ( receiver.Read(), 42 );
	sender.Tick( 25 );
	receiver.Tick( 25 );

	// Exercise actual sequential IDs through a full 16-bit cycle. Both peers
	// exchange ACKs so normal send and receive state remains bounded.
	for ( int i = 0; i < 0x10000 - nRetries - 1; ++i )
	{
		receiver.Receive( sender.Send() );
		sender.Receive( receiver.Send() );
		sender.Tick();
		receiver.Tick();
	}
	for ( int i = 0; i < nRetries; ++i )
	{
		if ( !bEmptyReusedPacket || i != 0 )
		{
			sender.Queue( static_cast<unsigned char>( 0xA1 + i ) );
		}
		const SDatagram packet = sender.Send();
		ASSERT_EQ( packet.id, 100 + i );
		receiver.Receive( packet );
	}
	sender.Queue( 0xB2 );
	const SDatagram lost = sender.Send();
	ASSERT_EQ( lost.id, 100 + nRetries );
	// Lose the new transmission that has the old retry's packet number, then
	// ACK only the preceding packets. No old alias may acknowledge its bytes.
	sender.Receive( receiver.Send() );
	for ( int i = 0; i < nRetries; ++i )
	{
		if ( !bEmptyReusedPacket || i != 0 )
		{
			ASSERT_EQ( receiver.Read(), 0xA1 + i );
		}
	}
	sender.Tick( 4 );
	receiver.Tick( 4 );
	sender.Queue( 0xC3 );
	for ( int i = 0; i < 60; ++i )
	{
		receiver.Receive( sender.Send() );
		sender.Receive( receiver.Send() );
		sender.Tick();
		receiver.Tick();
	}
	EXPECT_EQ( receiver.Read(), 0xB2 );
	EXPECT_EQ( receiver.Read(), 0xC3 );
	EXPECT_EQ( receiver.Read(), -1 );
}
} // namespace

TEST( NetPacketLifetime, FailedSendDoesNotCommitAnotherBlockAfterPacketNumberWrap )
{
	CheckRecoveryAfterPacketNumberWrap( IMMEDIATE_SEND_FAILURE );
}

TEST( NetPacketLifetime, EmptyPacketAlsoRetiresThePreviousUseOfItsNumber )
{
	CheckRecoveryAfterPacketNumberWrap( IMMEDIATE_SEND_FAILURE, true );
}

TEST( NetPacketLifetime, ReassignmentCreatedAfterOriginalExpiryCannotSurvivePacketNumberWrap )
{
	CheckRecoveryAfterPacketNumberWrap( ROLLBACK_AND_EXPIRY );
}

TEST( NetPacketLifetime, ChainedFailedRetriesCannotCommitAnotherBlockAfterPacketNumberWrap )
{
	CheckRecoveryAfterPacketNumberWrap( REPEATED_SEND_FAILURES );
}

TEST( NetPacketLifetime, OrdinaryTimedLossStillRecoversAfterPacketNumberWrap )
{
	CheckRecoveryAfterPacketNumberWrap( TIMED_ROLLBACK );
}

TEST( NetPacketLifetime, DelayedAcknowledgementStillCommitsAChainOfCurrentRetries )
{
	SEndpoint sender, receiver;
	sender.Queue( 42 );
	const SDatagram original = sender.Send( 100 );
	sender.Tick( 4 );
	sender.Send();
	sender.Tick( 4 );
	sender.Send();
	// The original datagram arrives after two retransmissions. Its ACK still
	// has to resolve through both aliases within the current packet lifetime.
	receiver.Receive( original );
	sender.Receive( receiver.Send() );
	EXPECT_EQ( receiver.Read(), 42 );
	sender.Tick( 25 );
	EXPECT_FALSE( sender.stream.HasOutData() );
	sender.Queue( 0xC3 );
	receiver.Receive( sender.Send() );
	EXPECT_EQ( receiver.Read(), 0xC3 );
	EXPECT_EQ( receiver.Read(), -1 );
}
