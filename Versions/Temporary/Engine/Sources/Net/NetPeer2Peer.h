#pragma once

/////////////////////////////////////////////////////////////////////////////////////
namespace nstl
{
	template<> struct hash<__int64> {
		size_t operator()(__int64 __x) const { return __x; }
	};
}

namespace NNet
{
/////////////////////////////////////////////////////////////////////////////////////
class CP2PTracker
{
public:
	typedef unsigned int UCID;
	enum EOutMessage
	{
		NEW_CIENT,
		REMOVE_CLIENT,
		DIRECT,
		BROADCAST,
	};
	struct SMessage
	{
		EOutMessage msg;
		UCID from;
		vector<UCID> received;
		CMemoryStream pkt;
	};
	struct SPacket
	{
		UCID addr;
		CMemoryStream pkt;
		
		SPacket() {}
		SPacket( UCID _addr, CMemoryStream &_pkt ): addr(_addr), pkt(_pkt) {}
	};
	vector<SPacket> packets;
	vector<UCID> kicks;

	void SendBroadcast( CMemoryStream &pkt );
	void SendDirect( UCID addr, CMemoryStream &pkt );
	bool GetMessage( SMessage *pRes );
	void ProcessPacket( UCID addr, CMemoryStream &pkt, bool bIsBroadcast );
	void AddNewClient( UCID addr, CMemoryStream &addrInfo, bool bIsBroadcast );
	void KickClient( UCID addr, bool bIsBroadcast );
	bool IsActive( UCID addr );

	CP2PTracker() : maxPeerID( 0 ) {}
private:
	struct SAck
	{
		UCID addr;
		bool bAcked;
	};
	struct SQMessage
	{
		int nID;
		bool bDirect;
		CMemoryStream msg;
		list<SAck> acks;
	};
	struct SFastAck
	{
		int nID;
		UCID addr; // broadcast sender address
	};
	typedef int64 PEER_ID;
	struct SPeerClient
	{
		UCID addr;
		PEER_ID id;

		SPeerClient() {}
		SPeerClient( UCID _addr, PEER_ID _id ): addr(_addr), id(_id) {}
	};
	struct SPeer
	{
		UCID addr;
		bool bActive;    // not active when peer is being dropped
		list<SPeerClient> clients; // current clients from peer view
		list<SQMessage> messages;
		list<SFastAck> fastacks; // acks received from this peer before message itself
		list<UCID> requireKick;
		PEER_ID id;
		CMemoryStream addrInfo;

		bool IsActive() const { return bActive; }
		bool HasClient( UCID addr ) const
		{
			for ( list<SPeerClient>::const_iterator i = clients.begin(); i != clients.end(); ++i )
				if ( i->addr == addr )
					return true;
			return false;
		}
		const UCID GetAddr( PEER_ID id )
		{
			for ( list<SPeerClient>::iterator i = clients.begin(); i != clients.end(); ++i )
			{
				if ( i->id == id )
					return i->addr;
			}
			ASSERT( 0 );
			return clients.front().addr;
		}
		void RemoveClient( UCID addr )
		{
			for ( list<SPeerClient>::iterator i = clients.begin(); i != clients.end(); )
			{
				if ( i->addr == addr )
					i = clients.erase( i );
				else
					++i;
			}
		}
	};
	list<SMessage> output;
	hash_map<UCID,SPeer> clients;
	hash_map<PEER_ID,UCID> peersByID; 
	PEER_ID maxPeerID;

	SPeer* GetClient( UCID addr );
	SPeer* GetClientByPeerID( PEER_ID id );
	void AddOutputMessage( EOutMessage msg, UCID _from, 
		const CMemoryStream *pData = 0, vector<UCID> *pReceived = 0 );
	void AddKickApprove( UCID victim, UCID kickFrom );
	void ApproveKick( UCID victim, UCID from );
	PEER_ID GetUnusedID();
	void ReceiveBroadcast( const UCID who, CMemoryStream &data, int nID );
	void ReceiveDirect( const UCID who, CMemoryStream &data );
	void ReceiveAddClient( const UCID peer, UCID who, PEER_ID id, CMemoryStream &addrInfo, bool bIsBroadcast );
	void ReceiveRemoveClient( const UCID peer, UCID whom, bool bIsBroadcast );
	void ReceiveAck( const UCID peer, int nID, PEER_ID id );
	void CheckQueuedMessages( const UCID who );
	void CheckCorpses();
	void SendRemoveClient( UCID destAddr, UCID whom );
	void SendAddClient( const UCID dest, UCID whom, PEER_ID id, CMemoryStream &addrInfo );
	void SendBroadcast( const UCID dest, int nID, CMemoryStream &data );
	//void SendDirect( const SPeer &dest, CMemoryStream &pkt );
	void SendAck( const UCID dest, int nID, PEER_ID id );
};	
}

