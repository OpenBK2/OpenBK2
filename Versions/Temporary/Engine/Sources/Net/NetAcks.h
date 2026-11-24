#pragma once

#include "port/socket.h"

#include <cstdint>
#include <list>
#include <vector>

class CBitStream;

namespace NNet
{

typedef unsigned int UPDATE_ID;
typedef unsigned short PACKET_ID;

// RTT, acks, rate/congestion tracking
class CAckTracker
{
public:
	CAckTracker();

	float GetRTT() const
	{
		return fAvrgRTT;
	}

	float GetRTTDisp() const
	{
		return fRTTVariation;
	}

	float GetTimeSinceLastRecv() const
	{
		return fTimeSinceLastRecv;
	}

	const float GetPing() const
	{
		return fPing;
	}

	// Charge each stream block to the window, including redundant copies.
	// Pure acknowledgements do not consume data-window capacity.
	bool CanSend( int nUnits = 1 ) const;
	bool NeedSend() const;
	bool AckDue() const;
	void RequestAck();

	PACKET_ID GetNextPacketID() const
	{
		return nPktSent;
	}

	void Step( std::vector<PACKET_ID> *pRolled, std::vector<PACKET_ID> *pErased, double fDeltaTime, float fMaxRTT );
	PACKET_ID WrtieAcks( CBitStream *pBits, int nPktSize, int nUnits = 1 );
	bool ReadAcks( std::vector<PACKET_ID> *acked, CBitStream &bits, bool bAcceptData = true );
	void PacketLost( PACKET_ID pktID );

private:
	PACKET_ID nPktSent;         // is used to order update packets
	UPDATE_ID nPktLastReceived; // number of last received pkt
	UPDATE_ID nPktHighCounter;  // high part of packet counter
	//
	// info on received pkts (acks on them should be sent)
	std::vector<UPDATE_ID> receivedPkts;
	// acknowledgements on packets cache
	uint32_t dwAckedBits;
	PACKET_ID nAckedLast;
	//
	float fAvrgRTT, fRTTVariation;
	bool bHaveRTT = false;
	bool bAckPending = false;
	double fAckDeadline = 0;
	unsigned int nTimeoutBackoff = 0;
	double fRecoveryEnd = -1;

	// ping information
	float fPing;
	long nPingPacketsReceived;
	float fSumPktRTT4Period;
	float fLastPingUpdateTime;

	//
	float fWindow;
	int nFlyPackets;
	//float fUpdateTimeDelay;       // recommended delay between updates (in seconds)
	//float fUpdateTimeElapsed;     // time elapsed since last update
	float fTimeSinceLastSend;
	float fTimeSinceLastRecv; // time elapsed since last receive
	double fCurrentTime;

	//int nUpdateSize;               // recommended update size
	// bool bHasNewPacketToAck;
	//
	// пакет, сформированный из CUpdateRequest
	class CUpdate
	{
	public:
		double fTimeCreated;
		PACKET_ID nPktNumber;
		bool bOnTheWindowEdge;
		int nUnits = 1;
		unsigned int nLaterAcks = 0;
	};

	// info on sent update packets
	std::list<CUpdate> sentUpdates, rolledUpdates; // unacknowledged updates

	void RegisterRTT( float fRTT );
	void SendPktAcks( CBitStream *pBits );
	void ReceiveAck( std::vector<PACKET_ID> *pAcked, PACKET_ID nPkt );
	void ReceivePktAcks( std::vector<PACKET_ID> *pAcked, CBitStream &bits );
	bool CheckRecvPacketNumber( UPDATE_ID nPkt );
};

} // namespace NNet
