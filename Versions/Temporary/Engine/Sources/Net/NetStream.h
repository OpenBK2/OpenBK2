#pragma once

#include "NetAcks.h"
#include "port/socket.h"

/////////////////////////////////////////////////////////////////////////////////////
namespace NNet
{
/////////////////////////////////////////////////////////////////////////////////////
template <int N_SIZE>
class CRingBuffer
{
	unsigned int nReadPtr, nWritePtr;
	char szBuffer[N_SIZE];

public:
	CRingBuffer()
	{
		nReadPtr = 0;
		nWritePtr = 0;
	}

	int GetSize() const
	{
		return ( nWritePtr - nReadPtr + N_SIZE ) % N_SIZE;
	}

	int GetBufSize() const
	{
		return N_SIZE - 1 - GetSize();
	}

	int Peek( void *pBuf, int nBufSize )
	{
		// Clamped against a negative request as well as against what is in the
		// buffer. The test that splits the copy was "nReadPtr + nRes > N_SIZE",
		// and nReadPtr is unsigned, so a negative size was converted and passed
		// it, leaving the second length negative and its memcpy enormous. The
		// one caller whose size comes off the wire, CanReadPacket in NetA4.cpp,
		// rejects a negative one before getting here, and the ASSERT in Write
		// below checks nothing in this tree, so the bound belongs here.
		const int nRes = (std::min)( (std::max)( nBufSize, 0 ), GetSize() );
		// Read is the only thing that moves nReadPtr, and does so modulo
		// N_SIZE, so it is already in range. Saying it again here is what lets
		// a compiler see that neither copy leaves szBuffer, which it cannot
		// work out across calls: GCC reads the subtraction below as possibly
		// huge and reports both copies as overflowing their destination.
		const int nRead = static_cast<int>( nReadPtr % N_SIZE );
		const int nRight = N_SIZE - nRead;
		if ( nRes > nRight )
		{
			memcpy( pBuf, szBuffer + nRead, nRight );
			memcpy( ( (char *)pBuf ) + nRight, szBuffer, nRes - nRight );
		}
		else
		{
			memcpy( pBuf, szBuffer + nRead, nRes );
		}
		return nRes;
	}

	int Read( void *pBuf, int nBufSize )
	{
		int nRes = Peek( pBuf, nBufSize );
		nReadPtr = ( nReadPtr + nRes ) % N_SIZE;
		return nRes;
	}

	int Write( const void *pBuf, int nLength )
	{
		ASSERT( GetSize() + nLength < N_SIZE );
		// The same two points as Peek. nWritePtr differs from nReadPtr in that
		// the straight branch leaves it equal to N_SIZE rather than below it,
		// which the modulo here absorbs as the old wrapping branch did.
		const int nRes = (std::min)( (std::max)( nLength, 0 ), GetBufSize() );
		const int nWrite = static_cast<int>( nWritePtr % N_SIZE );
		const int nRight = N_SIZE - nWrite;
		if ( nRes > nRight )
		{
			memcpy( szBuffer + nWrite, pBuf, nRight );
			memcpy( szBuffer, ( (const char *)pBuf ) + nRight, nRes - nRight );
			nWritePtr = ( nWrite + nRes ) % N_SIZE;
		}
		else
		{
			memcpy( szBuffer + nWrite, pBuf, nRes );
			nWritePtr = nWrite + nRes;
		}
		return nRes;
	}
};

/////////////////////////////////////////////////////////////////////////////////////
// streamed connection, input/output buffers
const int N_STREAM_BUFFER = 32768;
//const int N_STREAM_BUFFER = 128000;
const int N_MAX_PACKET_SIZE = 1024;      // low level packet size
const int N_MAX_STREAM_BLOCK_SIZE = 255; // one-byte length in SChannelBlock

class CStreamTracker
{
public:
	// channel data
	CRingBuffer<N_STREAM_BUFFER> channelInBuf;
	std::list<CMemoryStream> outList;

	CStreamTracker();

	bool HasOutData()
	{
		return !outList.empty() || channelOutBuf.GetSize() != 0 || !channelOutList.empty();
	}

	// A datagram carries at most 255 stream bytes, even with a larger UDP limit.
	bool CanReadMsg() const
	{
		return channelInBuf.GetBufSize() >= N_MAX_STREAM_BLOCK_SIZE;
	}

	void WriteMsg( PACKET_ID nPkt, CBitStream *pBits, int nSizeLimit );
	void ReadMsg( CBitStream &bits );
	void PumpIncoming();
	void Rollback( const std::vector<PACKET_ID> &pkts );
	void Erase( const std::vector<PACKET_ID> &pkts );
	void Commit( const std::vector<PACKET_ID> &pkts );

private:
	CRingBuffer<N_STREAM_BUFFER> channelOutBuf;
	typedef unsigned int CHANNEL_DATA_OFFSET;
#pragma pack( push, 1 )

	struct SChannelBlock
	{
		CHANNEL_DATA_OFFSET nOffset;
		unsigned char nLength;
		char cData[256];
		PACKET_ID nPkt;

		int GetSendSize() const
		{
			return sizeof( nOffset ) + sizeof( nLength ) + nLength;
		}

		static int GetHeaderSize()
		{
			return sizeof( CHANNEL_DATA_OFFSET ) + sizeof( unsigned char );
		}
	};

#pragma pack( pop )

	// streaming data control structures
	// текущее смещение отсылаемых и принимаемых данных
	CHANNEL_DATA_OFFSET nChannelOutputOffset, nChannelInputOffset;
	typedef std::list<SChannelBlock> SChannelBlockList;
	SChannelBlockList channelOutFlyList, channelOutList, channelInList;
	std::unordered_map<PACKET_ID, PACKET_ID> reassign;

	static bool IsBefore( CHANNEL_DATA_OFFSET border, CHANNEL_DATA_OFFSET test );
};

/////////////////////////////////////////////////////////////////////////////////////
} // namespace NNet
