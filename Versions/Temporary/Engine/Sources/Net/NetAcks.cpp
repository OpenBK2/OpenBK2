#include "stdafx.h"

#include "NetAcks.h"
#include "port/time.h"

#include <cmath>
//#define LOG
#ifdef LOG
#include <iostream>
#endif

#include <cstdint>

namespace NNet
{
const int CS_PACKET_ID_RANGE = 0x10000;
const float TIME_TO_UPDATE_PING = 4;
const float F_INITIAL_RTT = 0.333f;
const double MAX_ACK_DELAY = 0.025;
const float MIN_WINDOWS_SIZE = 2.0f;
const float MAX_WINDOWS_SIZE = 10.0f;

// CAckTracker

CAckTracker::CAckTracker()
{
	uint32_t dwTick = GetCurrentTimeMilliseconds();
	float fRTT;
	fRTT = F_INITIAL_RTT;
	fAvrgRTT = fRTT;
	fRTTVariation = fAvrgRTT * 0.5f;
	// 1 мало, т.к. в начале первые пакеты с обоих сторон не имеют ack`ов. Как следствие обе стороны приходят в состояние
	// "хочу послать подтверждение о первом приеме, но не могу, т.к. окно занято не подтвержденным первым сообщением"
	// это состояние разрешается по таймауту. Чтобы это происходило сразу нужно окно как минимум 2
	fWindow = MIN_WINDOWS_SIZE;
	nFlyPackets = 0;
	//fUpdateTimeDelay = 0.04f;// * ( 0.5f + (dwTick & 0xff ) / 255.0 );
	//fUpdateTimeElapsed = 0;
	fTimeSinceLastSend = 0;
	fTimeSinceLastRecv = 0;
	fCurrentTime = 0;
	//nUpdateSize = 220;//512;
	nPktSent = (PACKET_ID)dwTick;
	nPktLastReceived = 0;
	nPktHighCounter = CS_PACKET_ID_RANGE;
	dwAckedBits = 0;
	nAckedLast = 0;

	//
	fPing = 0;
	nPingPacketsReceived = 0;
	fSumPktRTT4Period = 0;
	fLastPingUpdateTime = fCurrentTime;
	//bHasNewPacketToAck = true; // так делать нельзя: не все пакеты содержат данные!
}

/*void CAckTracker::SetRate( int nBytesPerSec )
{
	nBytesPerSec = Max( nBytesPerSec, 500 );
	nBytesPerSec = Min( nBytesPerSec, 50000 );
	fUpdateTimeDelay = 250.0f / nBytesPerSec;
}*/

bool CAckTracker::CanSend( int nUnits ) const
{
	return nFlyPackets + nUnits - 1 < fWindow;
}

bool CAckTracker::NeedSend() const
{
	return fTimeSinceLastSend > 1.0f;
}

void CAckTracker::RequestAck()
{
	if ( !bAckPending )
	{
		bAckPending = true;
		fAckDeadline = fCurrentTime + MAX_ACK_DELAY;
	}
}

bool CAckTracker::AckDue() const
{
	return bAckPending && fCurrentTime >= fAckDeadline;
}

void CAckTracker::RegisterRTT( float _fRTT )
{
	//ASSERT( fRTT >= 0 );
	const float fRTT = Clamp( _fRTT, 0.001f, 20.0f );
	// The first observation seeds the estimate instead of slowly decaying a
	// multi-second guess. Variation keeps delayed/reordered paths conservative.
	if ( !bHaveRTT )
	{
		fAvrgRTT = fRTT;
		fRTTVariation = fRTT * 0.5f;
		bHaveRTT = true;
	}
	else
	{
		fRTTVariation = 0.75f * fRTTVariation + 0.25f * std::fabs( fAvrgRTT - fRTT );
		fAvrgRTT = 0.875f * fAvrgRTT + 0.125f * fRTT;
	}

	// update ping info
	++nPingPacketsReceived;
	fSumPktRTT4Period += fRTT;
	if ( fLastPingUpdateTime + TIME_TO_UPDATE_PING < fCurrentTime )
	{
		fPing = fSumPktRTT4Period / nPingPacketsReceived;

		fLastPingUpdateTime = fCurrentTime;
		fSumPktRTT4Period = 2 * fPing;
		nPingPacketsReceived = 2;
	}
}

void CAckTracker::Step( std::vector<PACKET_ID> *pRolled,
                        std::vector<PACKET_ID> *pErased,
                        double fDeltaTime,
                        float fMaxRTT )
{
	pRolled->resize( 0 );
	pErased->resize( 0 );
	fTimeSinceLastRecv += fDeltaTime;
	fTimeSinceLastSend += fDeltaTime;
	//fUpdateTimeElapsed += fDeltaTime;

	fCurrentTime += fDeltaTime;
	//
	const double timeout =
			(std::min)( static_cast<double>( fMaxRTT ),
	                std::ldexp( (std::max)( 0.1, fAvrgRTT + 4.0 * fRTTVariation + MAX_ACK_DELAY ), nTimeoutBackoff ) );
	// Three distinct newer acknowledgements indicate a gap, but allow a
	// reordering interval before retransmission. Tail loss still has a timer.
	const double reorderDelay = (std::max)( 0.025, 1.125 * fAvrgRTT );
	std::list<CUpdate>::iterator it;
	bool timedOut = false;
	for ( it = sentUpdates.begin(); it != sentUpdates.end(); )
	{
		const double age = fCurrentTime - it->fTimeCreated;
		const bool fastLoss = it->nLaterAcks >= 3 && age >= reorderDelay;
		if ( fastLoss || age >= timeout )
		{
			timedOut = timedOut || !fastLoss;
			nFlyPackets -= it->nUnits;
			// React once to a flight's loss, rather than shrinking again for
			// every missing datagram in the same burst.
			if ( it->fTimeCreated > fRecoveryEnd )
			{
				fWindow = (std::max)( MIN_WINDOWS_SIZE, fWindow * 0.75f );
				fRecoveryEnd = fCurrentTime;
			}
			pRolled->push_back( it->nPktNumber );
			auto lost = it++;
			rolledUpdates.splice( rolledUpdates.end(), sentUpdates, lost );
		}
		else
		{
			++it;
		}
	}

	if ( timedOut )
	{
		nTimeoutBackoff = (std::min)( nTimeoutBackoff + 1, 6u );
	}
	for ( it = rolledUpdates.begin(); it != rolledUpdates.end(); )
	{
		if ( fCurrentTime - it->fTimeCreated > fMaxRTT )
		{
			pErased->push_back( it->nPktNumber );
			it = rolledUpdates.erase( it );
		}
		else
		{
			++it;
		}
	}
	//
}

// receive single ack, search for this packet and ack it
void CAckTracker::ReceiveAck( std::vector<PACKET_ID> *pAcked, PACKET_ID nPkt )
{
	std::list<CUpdate>::iterator i;
	for ( i = sentUpdates.begin(); i != sentUpdates.end(); )
	{
		if ( ( i->nPktNumber ) == nPkt )
		{
			nTimeoutBackoff = 0;
			nFlyPackets -= i->nUnits;
			if ( i->bOnTheWindowEdge )
			{
				fWindow += 0.05f;
				fWindow = (std::min)( fWindow, MAX_WINDOWS_SIZE );
			}
			// / fWindow;
			//fUpdateTimeDelay *= 0.99f;
			//fUpdateTimeDelay = Max( fUpdateTimeDelay, 0.00001f );
#ifdef LOG
			std::cout << "ACCELERATE new window=" << fWindow << "packets" << std::endl;
#endif
			for ( auto &pending : sentUpdates )
			{
				const unsigned distance = static_cast<PACKET_ID>( nPkt - pending.nPktNumber );
				if ( distance > 0 && distance < CS_PACKET_ID_RANGE / 2 )
				{
					++pending.nLaterAcks;
				}
			}
			pAcked->push_back( nPkt );
			// Late ACKs for rolled transmissions do not inflate the RTT estimate.
			RegisterRTT( fCurrentTime - i->fTimeCreated );
			i = sentUpdates.erase( i );
			return;
		}
		else
		{
			++i;
		}
	}
	// this should be very rare case so perfomance is not critical
	for ( i = rolledUpdates.begin(); i != rolledUpdates.end(); )
	{
		if ( i->nPktNumber == nPkt )
		{
			nTimeoutBackoff = 0;
			pAcked->push_back( nPkt );
			i = rolledUpdates.erase( i );
			return;
		}
		else
		{
			++i;
		}
	}
}

// pair to SendPktAcks()
void CAckTracker::ReceivePktAcks( std::vector<PACKET_ID> *pAcked, CBitStream &bits )
{
	uint32_t dwBits, dwNewAckBits;
	PACKET_ID nLast;
	bits.Read( &nLast, sizeof( nLast ) );
	bits.Read( &dwBits, sizeof( dwBits ) );
	//
	int nShift = ( nLast - nAckedLast ) & ( CS_PACKET_ID_RANGE - 1 );
	if ( nShift > CS_PACKET_ID_RANGE / 2 )
	{
		nShift -= CS_PACKET_ID_RANGE;
	}
	if ( nShift <= 0 )
	{
		// A whole bitmap has moved out of range; shifting uint32_t by 32 is
		// undefined and can suppress valid ACKs after packet reordering.
		if ( nShift <= -32 )
		{
			dwAckedBits = 0;
		}
		else
		{
			dwAckedBits >>= -nShift;
		}
	}
	else
	{
		dwAckedBits <<= 1;
		dwAckedBits |= 1;
		nShift--;
		if ( nShift < 32 )
		{
			dwAckedBits <<= nShift;
		}
		else
		{
			dwAckedBits = 0;
		}
	}
	dwNewAckBits = dwBits & ~dwAckedBits;
	dwAckedBits |= dwNewAckBits;
	nAckedLast = nLast;
	//
	ReceiveAck( pAcked, nLast );
	for ( int i = 1; i <= 32; i++ )
	{
		nLast--;
		if ( dwNewAckBits & ( uint32_t( 1 ) << ( i - 1 ) ) )
		{
			ReceiveAck( pAcked, nLast );
		}
	}
}

// check if nPkt can be acked and is not too obsolete
// add it to received list and update pktlastreceived value if needed
bool CAckTracker::CheckRecvPacketNumber( UPDATE_ID nPkt )
{
	// is not update too old?
	if ( nPkt + 32 < nPktLastReceived )
	{
		//if (pCSLog) (*pCSLog) << "SKIP too old update, dif=" << nPktLastReceived - nPkt << endl;
		return false;
	}
	nPktLastReceived = (std::max)( nPktLastReceived, nPkt );
	// check for duplicate (for economy on rcvList mostly, because ack maybe already be sent
	// and duplication will not be detected)
	for ( std::vector<UPDATE_ID>::iterator k = receivedPkts.begin(); k != receivedPkts.end(); ++k )
	{
		if ( *k == nPkt )
		{
			//if (pCSLog) (*pCSLog) << "DUPLICATE update received from " << peer.GetFastName() << endl;
			return false;
		}
	}
	// add to receivedPkts set
	receivedPkts.push_back( nPkt );
	return true;
}

bool CAckTracker::ReadAcks( std::vector<PACKET_ID> *pAcked, CBitStream &bits, bool bAcceptData )
{
	PACKET_ID nReceived;
	UPDATE_ID nCurUpdate;

	pAcked->resize( 0 );
	fTimeSinceLastRecv = 0;
	//
	bits.Read( &nReceived, sizeof( PACKET_ID ) );
	// calcing sequential number of arrived update (accounting for wrapping of pkt numbers)
	nCurUpdate = nPktHighCounter + nReceived;
	if ( nPktLastReceived == 0 )
	{
		nPktLastReceived = nCurUpdate;
	}
	int nDif = nCurUpdate - nPktLastReceived;
	if ( nDif > CS_PACKET_ID_RANGE / 2 )
	{
		nCurUpdate -= CS_PACKET_ID_RANGE;
	}
	else if ( nDif < -CS_PACKET_ID_RANGE / 2 )
	{
		nCurUpdate += CS_PACKET_ID_RANGE;
		nPktHighCounter += CS_PACKET_ID_RANGE;
	}
	// ACKs still free outgoing data when the receive ring is full or this
	// datagram is a duplicate. Do not acknowledge payload we cannot retain.
	ReceivePktAcks( pAcked, bits );
	return bAcceptData && CheckRecvPacketNumber( nCurUpdate );
}

// use receivedPkts
void CAckTracker::SendPktAcks( CBitStream *pBits )
{
	int i;
	uint32_t dwBits = 0;
	PACKET_ID nLast = (PACKET_ID)nPktLastReceived;
	UPDATE_ID nTest = nPktLastReceived;
	for ( i = 1; i <= 32; i++ )
	{
		nTest--;
		std::vector<UPDATE_ID>::iterator k;
		k = find( receivedPkts.begin(), receivedPkts.end(), nTest );
		if ( k != receivedPkts.end() )
		{
			dwBits |= uint32_t( 1 ) << ( i - 1 );
		}
	}
	( *pBits ).Write( &nLast, sizeof( nLast ) );
	( *pBits ).Write( &dwBits, 4 );
	// removing too old ack Pkts
	for ( i = receivedPkts.size() - 1; i >= 0; i-- )
	{
		if ( receivedPkts[i] + 32 < nPktLastReceived )
		{
			receivedPkts.erase( receivedPkts.begin() + i );
		}
	}
}

PACKET_ID CAckTracker::WrtieAcks( CBitStream *pBits, int nPktSize, int nUnits )
{
	nFlyPackets += nUnits;
	bAckPending = false;
	fTimeSinceLastSend = 0;
#ifdef LOG
	std::cout << "SEND, pkts left=" << fWindow - nFlyPackets << std::endl;
#endif
	// current packet number
	// const unsigned char *pStart = bits.GetCurrentPtr();
	PACKET_ID nPktCurrent = nPktSent++;
	( *pBits ).Write( &nPktCurrent, sizeof( PACKET_ID ) );

	// acks on received packets
	SendPktAcks( pBits );

	// Pure acknowledgements/keepalives do not occupy the data window and
	// never require an acknowledgement of their own.
	if ( nUnits > 0 )
	{
		CUpdate &update = sentUpdates.emplace_back();
		update.fTimeCreated = fCurrentTime;
		update.nPktNumber = nPktCurrent;
		update.nUnits = nUnits;
		update.bOnTheWindowEdge = nFlyPackets + 1 > fWindow;
	}

	return nPktCurrent;
}

void CAckTracker::PacketLost( PACKET_ID pktID )
{
	bool bFound = false;
	std::list<CUpdate>::iterator it;
	for ( it = sentUpdates.begin(); it != sentUpdates.end(); )
	{
		if ( it->nPktNumber == pktID )
		{
			bFound = true;
			nFlyPackets -= it->nUnits;
			fWindow -= 1.0f;
			fWindow = (std::max)( MIN_WINDOWS_SIZE, fWindow );
#ifdef LOG
			std::cout << "SEND FAILURE, new window=" << fWindow << "packets" << std::endl;
#endif
			it = sentUpdates.erase( it );
		}
		else
		{
			++it;
		}
	}
	ASSERT( bFound );
}

} // namespace NNet
