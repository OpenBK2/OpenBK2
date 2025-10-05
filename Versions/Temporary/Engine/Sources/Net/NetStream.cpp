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
CStreamTracker::CStreamTracker(): nChannelOutputOffset(0), nChannelInputOffset(0)
{
}
// return true if test offset is before border
bool CStreamTracker::IsBefore( CHANNEL_DATA_OFFSET border, CHANNEL_DATA_OFFSET test )
{
	return ((CHANNEL_DATA_OFFSET)(test - border)) > 0x80000000;
}
/////////////////////////////////////////////////////////////////////////////////////
void CStreamTracker::WriteMsg( PACKET_ID nPkt, CBitStream *pBits, int nSizeLimit )
{
//	cout << "CHANNEL stats " << channelOutFlyList.size() << " " << channelOutList.size();
//	cout << " " << channelInList.size() << " " << reassign.size() << endl;
	ASSERT( nSizeLimit <= N_MAX_PACKET_SIZE );
	if ( channelOutList.empty() )
	{
		if ( channelOutBuf.GetSize() < N_MAX_PACKET_SIZE )
		{
			while ( !outList.empty() && channelOutBuf.GetBufSize() >= outList.front().GetSize() )
			{
				CMemoryStream &m = outList.front();
				channelOutBuf.Write( m.GetBuffer(), m.GetSize() );
				outList.pop_front();
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
			SChannelBlock *pMSVCSuck = 0;
			int nMaxSize = nSizeLimit - pMSVCSuck->GetHeaderSize();
			nMaxSize = Min( nMaxSize, 255 );
			int nSize = Min( nMaxSize, channelOutBuf.GetSize() );
			
			SChannelBlock &block = channelOutFlyList.emplace_back();
			block.nOffset = nChannelOutputOffset;
			block.nLength = nSize;
			channelOutBuf.Read( block.cData, nSize );
			nChannelOutputOffset += nSize;
#ifdef LOG
			cout << "pkt" << nPkt << "send new block offs=" << (unsigned)block.nOffset << " size=" << (unsigned)block.nLength << endl;
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
		PACKET_ID oldPkt = ibest->nPkt;
		channelOutFlyList.splice( channelOutFlyList.end(), channelOutList, ibest );
		reassign[ oldPkt ] = nPkt;
	}
	// send channel data
	SChannelBlock &block = channelOutFlyList.back();
	ASSERT( block.GetSendSize() <= nSizeLimit );
	block.nPkt = nPkt;
	pBits->Write( &block, block.GetSendSize() );
}
/////////////////////////////////////////////////////////////////////////////////////
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
	// gather input blocks into stream
	for(;;)
	{
		bool bCont = false;
		for ( SChannelBlockList::iterator i = channelInList.begin(); i != channelInList.end(); )
		{
			if ( i->nOffset == nChannelInputOffset )
			{
				if ( channelInBuf.GetBufSize() < i->nLength )
					break; // avoid buffer overflow
				int nRes = channelInBuf.Write( i->cData, i->nLength );
				ASSERT( nRes == i->nLength ); // buffer overflow
				nChannelInputOffset += i->nLength;
				i = channelInList.erase( i );
				bCont = true;
			}
			else
				++i;
		}
		if ( !bCont )
			break;
	}
	// remove redundant blocks
	for ( SChannelBlockList::iterator i = channelInList.begin(); i != channelInList.end(); )
	{
		if ( IsBefore( nChannelInputOffset, i->nOffset ) )
			i = channelInList.erase( i );
		else
			++i;
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
				channelOutList.splice( channelOutList.end(), channelOutFlyList, i );
				break;
			}
			else
				++i;
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////
void CStreamTracker::Erase( const std::vector<PACKET_ID> &pkts )
{
	for ( int i = 0; i < pkts.size(); ++i )
	{
		PACKET_ID nPkt = pkts[i];
		std::unordered_map< PACKET_ID, PACKET_ID >::iterator k = reassign.find( nPkt );
		if ( k != reassign.end() )
			reassign.erase( k );
	}
}
/////////////////////////////////////////////////////////////////////////////////////
void CStreamTracker::Commit( const std::vector<PACKET_ID> &pkts )
{
	for ( int i = 0; i < pkts.size(); ++i )
	{
		PACKET_ID nPkt = pkts[i];
		for(;;)
		{
			std::unordered_map< PACKET_ID, PACKET_ID >::iterator k = reassign.find( nPkt );
			if ( k == reassign.end() )
				break;
#ifdef LOG
			cout << "commit pkt" << nPkt << " translated to commit pkt " << k->second << endl;
#endif
			nPkt = k->second;
			reassign.erase( k );
		}
		SChannelBlockList::iterator k;
		bool isFound = false;
		for ( k = channelOutFlyList.begin(); k != channelOutFlyList.end(); )
		{
			if ( k->nPkt == nPkt )
			{
#ifdef LOG
				cout << "commit pkt" << nPkt << endl;
#endif
				channelOutFlyList.erase( k );
				isFound = true;
				break;
			}
			++k;
		}
		if ( !isFound )
		{
			for ( k = channelOutList.begin(); k != channelOutList.end(); )
			{
				if ( k->nPkt == nPkt )
				{
#ifdef LOG
					cout << "commit pkt" << nPkt << " from rolled set" << endl;
#endif
					channelOutList.erase( k );
					break;
				}
				++k;
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////
}

