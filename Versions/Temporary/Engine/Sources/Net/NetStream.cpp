#include "stdafx.h"

#include "NetStream.h"
//#define LOG
#ifdef LOG
#include <iostream>
#endif
namespace NNet
{
/////////////////////////////////////////////////////////////////////////////////////
// CStreamTracker
/////////////////////////////////////////////////////////////////////////////////////
CStreamTracker::CStreamTracker()
	: nOutListOffset( 0 )
	, nChannelOutputOffset( 0 )
	, nChannelInputOffset( 0 )
{
}

// return true if test offset is before border
bool CStreamTracker::IsBefore( CHANNEL_DATA_OFFSET border, CHANNEL_DATA_OFFSET test )
{
	return ( (CHANNEL_DATA_OFFSET)( test - border ) ) > 0x80000000;
}

/////////////////////////////////////////////////////////////////////////////////////
void CStreamTracker::WriteMsg( PACKET_ID nPkt, CBitStream *pBits, int nSizeLimit )
{
	// Packet IDs wrap. A mapping left by an expired or immediately failed send
	// belongs to the old transmission, even when this new packet carries no data.
	// Retire it before an ACK for the reused ID can commit an unrelated block.
	transmissions.erase( nPkt );
	//	cout << "CHANNEL stats " << channelOutFlyList.size() << " " << channelOutList.size();
	ASSERT( nSizeLimit <= N_MAX_PACKET_SIZE );
	if ( channelOutList.empty() )
	{
		if ( channelOutBuf.GetSize() < N_MAX_PACKET_SIZE )
		{
			while ( !outList.empty() && channelOutBuf.GetBufSize() > 0 )
			{
				CMemoryStream &m = outList.front();
				// Frames may exceed the ring. Preserve their order while copying only
				// the next available portion, instead of waiting for impossible space.
				nOutListOffset += channelOutBuf.Write( m.GetBuffer() + nOutListOffset, m.GetSize() - nOutListOffset );
				if ( nOutListOffset == m.GetSize() )
				{
					outList.pop_front();
					nOutListOffset = 0;
				}
			}
		}
		if ( channelOutBuf.GetSize() == 0 )
		{
#ifdef LOG
			cout << "pkt" << nPkt << "send zero size block" << endl;
#endif
			SChannelBlock block;
			block.nOffset = nChannelOutputOffset;
			block.nLength = 0;
			pBits->Write( &block, block.GetSendSize() );
			return;
		}
		else
		{
			int nMaxSize = nSizeLimit - SChannelBlock::GetHeaderSize();
			nMaxSize = (std::min)( nMaxSize, N_MAX_STREAM_BLOCK_SIZE );
			int nSize = (std::min)( nMaxSize, channelOutBuf.GetSize() );

			SChannelBlock &block = channelOutFlyList.emplace_back();
			block.nOffset = nChannelOutputOffset;
			block.nLength = nSize;
			channelOutBuf.Read( block.cData, nSize );
			nChannelOutputOffset += nSize;
#ifdef LOG
			cout << "pkt" << nPkt << "send new block offs=" << (unsigned)block.nOffset << " size=" << (unsigned)block.nLength
					 << endl;
#endif
		}
	}
	else
	{
		// find first block
		SChannelBlockList::iterator i = channelOutList.begin(), ibest = i;
		CHANNEL_DATA_OFFSET bestOffset = i->nOffset;
		for ( ; i != channelOutList.end(); ++i )
		{
			if ( IsBefore( bestOffset, i->nOffset ) )
			{
				bestOffset = i->nOffset;
				ibest = i;
			}
		}
#ifdef LOG
		cout << "reassign out pkt" << ibest->nPkt << " into " << nPkt << endl;
		cout << "send reassigned block offs=" << (unsigned)ibest->nOffset << " size=" << (unsigned)ibest->nLength << endl;
#endif
		channelOutFlyList.splice( channelOutFlyList.end(), channelOutList, ibest );
	}
	// send channel data
	SChannelBlock &block = channelOutFlyList.back();
	ASSERT( block.GetSendSize() <= nSizeLimit );
	block.nPkt = nPkt;
	transmissions[nPkt].push_back( block.nOffset );
	pBits->Write( &block, block.GetSendSize() );
}

/////////////////////////////////////////////////////////////////////////////////////
bool CStreamTracker::HasRedundantData() const
{
	for ( const auto &block : channelOutFlyList )
	{
		if ( !block.bRepeated )
		{
			return true;
		}
	}
	return false;
}

int CStreamTracker::WriteBundle( PACKET_ID nPkt, CBitStream *pBits, bool bAllowExtraBlock )
{
	const bool primary = HasOutData();
	auto extra = channelOutFlyList.end();
	if ( !primary || bAllowExtraBlock )
	{
		for ( auto it = channelOutFlyList.begin(); it != channelOutFlyList.end(); ++it )
		{
			if ( !it->bRepeated && ( extra == channelOutFlyList.end() || IsBefore( extra->nOffset, it->nOffset ) ) )
			{
				extra = it;
			}
		}
	}
	const unsigned char count = ( primary ? 1 : 0 ) + ( extra != channelOutFlyList.end() ? 1 : 0 );
	pBits->Write( count );
	transmissions.erase( nPkt );
	if ( primary )
	{
		WriteMsg( nPkt, pBits, N_MAX_STREAM_BLOCK_SIZE + SChannelBlock::GetHeaderSize() );
	}
	if ( extra != channelOutFlyList.end() )
	{
		// One proactive copy per block, sent in a different paced datagram.
		// Ordinary retransmission remains available if both copies are lost.
		extra->bRepeated = true;
		extra->nPkt = nPkt;
		transmissions[nPkt].push_back( extra->nOffset );
		pBits->Write( &*extra, extra->GetSendSize() );
	}
	return count;
}

void CStreamTracker::ReadMsg( CBitStream &bits )
{
	// receive channel data
	SChannelBlock block;
	bits.Read( &block, block.GetHeaderSize() );
	bits.Read( block.cData, block.nLength );

#ifdef LOG
	cout << "recv pkt offs=" << (unsigned)block.nOffset << " size=" << (unsigned)block.nLength << endl;
#endif
	// is not this data already received?
	if ( IsBefore( nChannelInputOffset, block.nOffset ) || block.nLength == 0 )
	{
#ifdef LOG
		cout << "SKIP recv pkt" << endl;
#endif
		return;
	}
	channelInList.push_back( block );
	PumpIncoming();
}

//
void CStreamTracker::PumpIncoming()
{
	// Received blocks have already been acknowledged. Resume assembly whenever
	// the consumer frees ring space; another datagram may never arrive.
	// gather input blocks into stream
	for ( ;; )
	{
		bool bCont = false;
		for ( SChannelBlockList::iterator i = channelInList.begin(); i != channelInList.end(); )
		{
			if ( i->nOffset == nChannelInputOffset )
			{
				if ( channelInBuf.GetBufSize() < i->nLength )
				{
					break; // avoid buffer overflow
				}
				int nRes = channelInBuf.Write( i->cData, i->nLength );
				ASSERT( nRes == i->nLength ); // buffer overflow
				nChannelInputOffset += i->nLength;
				i = channelInList.erase( i );
				bCont = true;
			}
			else
			{
				++i;
			}
		}
		if ( !bCont )
		{
			break;
		}
	}
	// remove redundant blocks
	for ( SChannelBlockList::iterator i = channelInList.begin(); i != channelInList.end(); )
	{
		if ( IsBefore( nChannelInputOffset, i->nOffset ) )
		{
			i = channelInList.erase( i );
		}
		else
		{
			++i;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
void CStreamTracker::Rollback( const std::vector<PACKET_ID> &pkts )
{
	for ( int it = 0; it < pkts.size(); ++it )
	{
		PACKET_ID nPkt = pkts[it];
		SChannelBlockList::iterator i;
		for ( i = channelOutFlyList.begin(); i != channelOutFlyList.end(); )
		{
			if ( i->nPkt == nPkt )
			{
#ifdef LOG
				cout << "ROLLBACK pkt" << nPkt << endl;
#endif
				auto lost = i++;
				channelOutList.splice( channelOutList.end(), channelOutFlyList, lost );
			}
			else
			{
				++i;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
void CStreamTracker::Erase( const std::vector<PACKET_ID> &pkts )
{
	for ( PACKET_ID id : pkts )
	{
		transmissions.erase( id );
	}
}

void CStreamTracker::Commit( const std::vector<PACKET_ID> &pkts )
{
	for ( PACKET_ID id : pkts )
	{
		const auto sent = transmissions.find( id );
		if ( sent == transmissions.end() )
		{
			continue;
		}
		for ( CHANNEL_DATA_OFFSET offset : sent->second )
		{
			for ( auto *blocks : { &channelOutFlyList, &channelOutList } )
			{
				for ( auto it = blocks->begin(); it != blocks->end(); )
				{
					if ( it->nOffset == offset )
					{
						it = blocks->erase( it );
					}
					else
					{
						++it;
					}
				}
			}
		}
		transmissions.erase( sent );
	}
}

} // namespace NNet
