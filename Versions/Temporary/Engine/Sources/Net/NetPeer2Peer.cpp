#include "StdAfx.h"
#include "NetPeer2Peer.h"
//#define LOG
#ifdef LOG
#include <iostream>
#endif

#if !defined(_FINALRELEASE) || defined(_DEVVERSION)
//#define __LOG__
#endif // !defined(_FINALRELEASE) || defined(_DEVVERSION)

#ifdef __LOG__
#endif // __LOG__
/////////////////////////////////////////////////////////////////////////////////////
namespace NNet
{
/////////////////////////////////////////////////////////////////////////////////////
// CP2PTracker
/////////////////////////////////////////////////////////////////////////////////////
enum EPacket
{
	PKT_ADD_CLIENT,
	PKT_REMOVE_CLIENT,
	PKT_BROADCAST_MSG,
	PKT_DIRECT_MSG,
	PKT_ACK,
	PKT_KICK_ADDR
};
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::AddOutputMessage( EOutMessage msg, const UCID _from, 
		const CMemoryStream *pData, vector<UCID> *pReceived )
{
#ifdef LOG
	cout << "OUT ";
	switch ( msg )
	{
		case NEW_CIENT: cout << "new client " << _from.GetFastName() << endl; break;
		case REMOVE_CLIENT: cout << "del client " << _from.GetFastName() << endl; break;
		case DIRECT: cout << "direct from " << _from.GetFastName() << endl; break;
		case BROADCAST: cout << "broadcast from " << _from.GetFastName() << endl; break;
		default:
			ASSERT( 0 );
			break;
	}
#endif
	SMessage &res = *output.insert( output.end() );
	res.msg = msg;
	res.from = _from;
	if ( pData )
		res.pkt = *pData;
	if ( pReceived )
		res.received = *pReceived;

#ifdef __LOG__
	switch ( msg )
	{
		case DIRECT:		
			{
				BYTE cMsgID = 0xff;		
				if ( pData )
				{
					CMemoryStream streamCopy( *pData );
					streamCopy.Seek( 0 );
					streamCopy >> cMsgID;
				}
				
				Singleton<IConsoleBuffer>()->WriteASCII(
					CONSOLE_STREAM_CONSOLE, 
					StrFmt( "p2p: direct from %d, msg %d", _from, (int)cMsgID ), 0xffffff00, true );
			}

			break;
		case NEW_CIENT: 
			Singleton<IConsoleBuffer>()->WriteASCII(
				CONSOLE_STREAM_CONSOLE, 
				StrFmt( "p2p: new client from %d", _from ), 0xffffff00, true );

			break;
		case REMOVE_CLIENT: 
			Singleton<IConsoleBuffer>()->WriteASCII(
				CONSOLE_STREAM_CONSOLE, 
				StrFmt( "p2p: del client %d", _from ), 0xffffff00, true );

			break;
	}
#endif // __LOG__
}
/////////////////////////////////////////////////////////////////////////////////////
bool CP2PTracker::GetMessage( SMessage *pRes )
{
	if ( output.empty() )
		return false;
	*pRes = output.front();
	output.pop_front();
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////
CP2PTracker::SPeer* CP2PTracker::GetClient( UCID addr )
{
	hash_map<UCID,SPeer>::iterator it = clients.find( addr );
	if ( it != clients.end() )
		return &(it->second);
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////
CP2PTracker::SPeer* CP2PTracker::GetClientByPeerID( PEER_ID id )
{
	hash_map<PEER_ID,UCID>::iterator it = peersByID.find( id );
	if ( it != peersByID.end() )
	{
		hash_map<UCID,SPeer>::iterator it2 = clients.find( it->second );
		if ( it2 != clients.end() )
			return &(it2->second);
		else
			return 0;
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////
bool CP2PTracker::IsActive( UCID addr )
{
	SPeer *pTest = GetClient( addr );
	if ( !pTest )
		return false;
	return pTest->IsActive();
}
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::CheckQueuedMessages( const UCID who )
{
	SPeer *pWho = GetClient( who );
	while ( pWho && !pWho->messages.empty() )
	{
		SQMessage &b = pWho->messages.front();
		bool bAcked = true;
		for ( list<SAck>::iterator i1 = b.acks.begin(); i1 != b.acks.end(); ++i1 )
			bAcked &= i1->bAcked;
		if ( !bAcked )
			break;
		vector<UCID> res;
		for ( list<SAck>::iterator i2 = b.acks.begin(); i2 != b.acks.end(); ++i2 )
			res.push_back( i2->addr );
		if ( b.bDirect )
			AddOutputMessage( DIRECT, pWho->addr, &b.msg );
		else
			AddOutputMessage( BROADCAST, pWho->addr, &b.msg, &res );
		pWho->messages.pop_front();
	}
	CheckCorpses();
}
/////////////////////////////////////////////////////////////////////////////////////
// check if it is time to remove some inactive clients
void CP2PTracker::CheckCorpses()
{
	list<UCID> idsToDel;
	for ( hash_map<UCID,SPeer>::iterator i = clients.begin(); i != clients.end(); ++i )
	{
		if ( !i->second.IsActive() && i->second.messages.empty() && i->second.requireKick.empty() )
		{
			AddOutputMessage( REMOVE_CLIENT, i->second.addr );
			idsToDel.push_back( i->first );
			peersByID.erase( i->second.id );
		}
	}
	for ( list<UCID>::iterator it = idsToDel.begin(); it != idsToDel.end(); ++it )
	{
		clients.erase( *it );
	}
}
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::AddKickApprove( UCID victim, UCID kickFrom )
{
	SPeer *pV = GetClient( victim );
	if ( pV )
	{
		if ( find( pV->requireKick.begin(), pV->requireKick.end(), kickFrom ) == pV->requireKick.end() )
			pV->requireKick.push_back( kickFrom );
	}
	else
		ASSERT( 0 );
}
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::ApproveKick( UCID victim, UCID from )
{
	SPeer *pV = GetClient( victim );
	if ( pV )
	{
		pV->requireKick.remove( from );
	}
	else
		ASSERT( 0 );

}
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::ReceiveDirect( const UCID peer, CMemoryStream &data )
{
#ifdef LOG
	cout << "RECV direct from " << peer.GetFastName() << endl;
#endif
	// add to pending list
	SPeer *pWho = GetClient( peer );
	SQMessage &b = *pWho->messages.insert( pWho->messages.end() );
	b.msg = data;
	b.nID = -1;
	b.bDirect = true;
	CheckQueuedMessages( peer );	
}
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::ReceiveBroadcast( const UCID peer, CMemoryStream &data, int nID )
{
#ifdef LOG
	cout << "RECV broadcast from " << peer.GetFastName() << " msg " << nID << endl;
#endif
	// add to pending list
	SPeer *pWho = GetClient( peer );
	SQMessage &b = *pWho->messages.insert( pWho->messages.end() );
	b.msg = data;
	b.nID = nID;
	b.bDirect = false;
	// set pending acks to intersection between active hosts from pWho view and current host view
	for ( list<SPeerClient>::iterator i = pWho->clients.begin(); i != pWho->clients.end(); ++i )
	{
		SPeer *pTest = GetClient( i->addr );
		if ( pTest && pTest->IsActive() )
		{
			// add ack request if such client exist
			SAck &ack = *b.acks.insert( b.acks.end() );
			ack.addr = i->addr;
			ack.bAcked = false;
			// seek through fast acks if required ack exists
			for ( list<SFastAck>::iterator k = pTest->fastacks.begin(); k != pTest->fastacks.end(); )
			{
				if ( k->addr == pWho->addr && k->nID == nID )
				{
					ack.bAcked = true;
					k = pTest->fastacks.erase( k );
				}
				else
					++k;
			}
			SendAck( i->addr, nID, pWho->id );
		}
	}
	CheckQueuedMessages( peer ); // can be no acks required
}
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::ReceiveAck( const UCID from, int nID, PEER_ID id )
{
	SPeer *pFrom = GetClient( from );
	if ( !pFrom )
		return;
	const UCID addr = pFrom->GetAddr( id );
#ifdef LOG
	cout << "RECV ack from " << pFrom->addr.GetFastName() << " msg " << nID << " from " << addr.GetFastName() << endl;
#endif
	// find message in question and remove pending ack from xx
	SPeer *pSender = GetClient( addr );
	if ( pSender )
	{
		bool bFound = false;
		for ( list<SQMessage>::iterator i = pSender->messages.begin(); i != pSender->messages.end(); ++i )
		{
			if ( i->nID == nID )
			{
				SQMessage &b = *i;
				bFound = true;
				for ( list<SAck>::iterator k = b.acks.begin(); k != b.acks.end(); ++k )
				{
					if ( k->addr == pFrom->addr )
					{
						ASSERT( !k->bAcked );
						k->bAcked = true;
						break;
					}
				}
				break;
			}
		}
		if ( bFound )
			CheckQueuedMessages( addr );
		else 
		{
			// save ack for the future
			// if sender is inactive no messages from him will be received and ack is useless
			if ( pSender->IsActive() ) 
			{
				SFastAck &b = *pFrom->fastacks.insert( pFrom->fastacks.end() );
				b.addr = addr;
				b.nID = nID;
			}
		}
	}
	else
		ASSERT( !pSender ); // sender was erased too early
}
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::ReceiveAddClient( const UCID peer, const UCID clientAddr, PEER_ID id, CMemoryStream &_addrInfo, bool bIsBroadcast )
{
#ifdef LOG
	cout << "RECV add client " << clientAddr.GetFastName() << " from " << pWho->addr.GetFastName() << endl;
#endif
		//if this is client being kicked then respond with kick addr and do nothing more
	SPeer *pTest = GetClient( clientAddr );
	if ( pTest )
	{
		if ( !pTest->IsActive() )
		{
			// pWho бредит - этого парня мы как раз кикаем
			SendRemoveClient( peer, clientAddr );
			return;
		}
	}
	else
		AddNewClient( clientAddr, _addrInfo, bIsBroadcast ); // это что-то новое, нужно добавить в свой список
	// now to remove buddy we need to receive kick messages from every client pWho talked about him
	AddKickApprove( clientAddr, peer );
	SPeer *pWho = GetClient( peer );
	if ( !pWho )
		return;
	for ( list<SPeerClient>::iterator i = pWho->clients.begin(); i != pWho->clients.end(); ++i )
	{
		SPeer *pT = GetClient( i->addr );
		if ( pT && pT->IsActive() )
			AddKickApprove( clientAddr, i->addr );
		else
			ASSERT( pT );
	}
	// client is added to peer tracking record
	pWho->clients.push_back( SPeerClient( clientAddr, id ) );
}
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::ReceiveRemoveClient( const UCID peer, const UCID corpse, bool bIsBroadcast )
{
#ifdef LOG
	cout << "RECV remove client " << corpse.GetFastName() << " from " << pWho->addr.GetFastName() << endl;
#endif
	SPeer *pWho = GetClient( peer );
	if ( !pWho )
		return;
	bool bHadCorpse = pWho->HasClient( corpse );
	//ASSERT( find( pWho->clients.begin(), pWho->clients.end(), corpse ) != pWho->clients.end() );
	pWho->RemoveClient( corpse );
	KickClient( corpse, bIsBroadcast );

	// every ack for broadcast message from pWho awaiting ack from client corpse 
	// is approved via ack remove
	for ( list<SQMessage>::iterator i = pWho->messages.begin(); i != pWho->messages.end(); ++i )
	{
		SQMessage &b = *i;
		for ( list<SAck>::iterator k = b.acks.begin(); k != b.acks.end(); )
		{
			if ( k->addr == corpse )
				k = b.acks.erase( k );
			else
				++k;
		}
	}
	CheckQueuedMessages( peer );
	SPeer *pCorpse = GetClient( corpse );
	if ( pCorpse )
	{
		ApproveKick( corpse, pWho->addr );
		ASSERT( !pCorpse->IsActive() );
		// every pending message from corpse awaiting ack from pWho is removed
		for ( list<SQMessage>::iterator i = pCorpse->messages.begin(); i != pCorpse->messages.end(); )
		{
			SQMessage &b = *i;
			bool bRemove = false;
			for ( list<SAck>::iterator k = b.acks.begin(); k != b.acks.end(); ++k )
			{
				if ( k->addr == pWho->addr && k->bAcked == false )
					bRemove = true;
			}
			if ( bRemove )
			{
#ifdef LOG
				cout << " remove penging message " << i->nID << " from " << corpse.GetFastName() << endl;
#endif
				i = pCorpse->messages.erase( i );
			}
			else
				++i;
		}
		CheckQueuedMessages( corpse );
	}
	else
		ASSERT( !bHadCorpse ); // was removed too early
}
/////////////////////////////////////////////////////////////////////////////////////
static int nBroadcastID = 1;
void CP2PTracker::SendBroadcast( CMemoryStream &pkt )
{
	for ( hash_map<UCID,SPeer>::iterator i = clients.begin(); i != clients.end(); ++i )
	{
		if ( i->second.IsActive() )
			SendBroadcast( i->second.addr, nBroadcastID, pkt );
	}
	++nBroadcastID;
}
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::SendDirect( UCID addr, CMemoryStream &data )
{
	SPeer *pTest = GetClient( addr );
	//ASSERT( pTest );
	if ( !pTest )
		return;

#ifdef LOG
	cout << "SEND direct to " << addr.GetFastName() << endl;
#endif
	CMemoryStream pkt;
	pkt << (char)PKT_DIRECT_MSG;
	pkt.WriteFrom( data );
	packets.push_back( SPacket( addr, pkt ) );

#ifdef __LOG__
	data.Seek( 0 );

	BYTE cMsgID;
	data >> cMsgID;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "p2p: send direct to %d, msg %d", addr, (int)cMsgID ), 0xffffff00, true );
#endif
}
/////////////////////////////////////////////////////////////////////////////////////
CP2PTracker::PEER_ID CP2PTracker::GetUnusedID()
{
	return ++maxPeerID;
}
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::AddNewClient( const UCID addr, CMemoryStream &_addrInfo, bool bIsBroadcast )
{
#ifdef LOG
	cout << "ADD attempt, client " << addr.GetFastName() << endl;
#endif
	SPeer *pTest = GetClient( addr );
//	ASSERT( !pTest );
	if ( pTest )
		return;

	PEER_ID newID = GetUnusedID();
#ifdef LOG
	cout << "ADD, client " << addr.GetFastName() << " ID=" << newID << endl;
#endif
	AddOutputMessage( NEW_CIENT, addr, &_addrInfo );
	// new client addr is added and for every existing channel info about that is sent
	for ( hash_map<UCID,SPeer>::iterator i1 = clients.begin(); i1 != clients.end(); ++i1 )
	{
		if ( bIsBroadcast && ( i1->second.IsActive() ) )
			SendAddClient( i1->second.addr, addr, newID, _addrInfo );
	}
	// to client addr info sent about every active SPeer on the moment
	SPeer res;
	res.addr = addr;	
	res.bActive = true;
	res.addrInfo = _addrInfo;
	for ( hash_map<UCID,SPeer>::iterator i2 = clients.begin(); i2 != clients.end(); ++i2 )
	{
		if ( bIsBroadcast && ( i2->second.IsActive() ) )
		{
			SendAddClient( addr, i2->second.addr, i2->second.id, i2->second.addrInfo );
			res.requireKick.push_back( i2->second.addr );
			AddKickApprove( i2->second.addr, addr );
		}
	}
	res.id = newID;
	clients[res.addr] = res;
	peersByID[res.id] = res.addr;
}
/////////////////////////////////////////////////////////////////////////////////////
// mark client as corpse and inform everybody about it
// no messages from this address will be received anymore
void CP2PTracker::KickClient( const UCID addr, bool bIsBroadcast )
{
	SPeer *pVictim = GetClient( addr );
	//cout << "KICK attempt, client " << addr.GetFastName() << endl;
	if ( pVictim && pVictim->IsActive() )
	{
#ifdef LOG
		cout << "KICK, client " << addr.GetFastName() << endl;
#endif
		kicks.push_back( addr );

		pVictim->bActive = false;
		for ( hash_map<UCID,SPeer>::iterator i1 = clients.begin(); i1 != clients.end(); ++i1 )
		{
			ApproveKick( (i1->second).addr, addr ); // мертвые не кусаются и вряд ли пришлют подтверждение о kick
			if ( bIsBroadcast && ( i1->second.IsActive() ) )
				SendRemoveClient( i1->second.addr, addr );
		}
		// every fast ack received about messages from victim should be deleted
		for ( hash_map<UCID,SPeer>::iterator i2 = clients.begin(); i2 != clients.end(); ++i2 )
		{
			for ( list<SFastAck>::iterator k = i2->second.fastacks.begin(); k != i2->second.fastacks.end(); )
			{
				if ( k->addr == addr )
					k = i2->second.fastacks.erase( k );
				else
					++k;
			}
		}
	}
	CheckCorpses();
}

void CP2PTracker::SendRemoveClient( UCID destAddr, const UCID whom )
{
#ifdef LOG
	cout << "SEND delclient to " << destAddr.GetFastName() << " client " << whom.GetFastName() << endl;
#endif
	CMemoryStream pkt;
	pkt << (char)PKT_REMOVE_CLIENT;
	pkt << whom;
	packets.push_back( SPacket( destAddr, pkt ) );
}

void CP2PTracker::SendAddClient( const UCID dest, const UCID whom, PEER_ID id, CMemoryStream &addrInfo )
{
#ifdef LOG
	cout << "SEND addclient to " << dest.GetFastName() << " client " << whom.GetFastName() << endl;
#endif
	CMemoryStream pkt;
	pkt << (char)PKT_ADD_CLIENT;
	pkt << whom;
	pkt << id;
	int nSize = addrInfo.GetSize();
	pkt << nSize;
	pkt.Write( addrInfo.GetBuffer(), nSize );
	packets.push_back( SPacket( dest, pkt ) );
}

void CP2PTracker::SendBroadcast( const UCID dest, int nID, CMemoryStream &data )
{
#ifdef LOG
	cout << "SEND broadcast to " << dest.GetFastName() << " msg " << nID << endl;
#endif
	CMemoryStream pkt;
	pkt << (char)PKT_BROADCAST_MSG;
	pkt << nID;
	pkt.WriteFrom( data );
	packets.push_back( SPacket( dest, pkt ) );
	
#ifdef __LOG__
	data.Seek( 0 );
	
	BYTE cMsgID;
	data >> cMsgID;
	if ( cMsgID != 9 )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "p2p: send broadcast, msg %d", (int)cMsgID ), 0xffffff00, true );
#endif
}
/////////////////////////////////////////////////////////////////////////////////////
void CP2PTracker::SendAck( const UCID addr, int nID, PEER_ID from )
{
#ifdef LOG
	SPeer *pShow = GetClient( from );
	ASSERT( pShow );
	cout << "SEND ack to " << addr.GetFastName() << " about msg " << nID << " from " << addr.GetFastName() << endl;
#endif
	CMemoryStream pkt;
	pkt << (char)PKT_ACK;
	pkt << nID;
	pkt << from;
	packets.push_back( SPacket( addr, pkt ) );
}
/////////////////////////////////////////////////////////////////////////////////////
static void ReadRest( CMemoryStream &src, CMemoryStream *pDst )
{
	src.ReadTo( pDst, src.GetSize() - src.GetPosition() );
}
void CP2PTracker::ProcessPacket( const UCID addr, CMemoryStream &pkt, bool bIsBroadcast )
{
	SPeer *pWho = GetClient( addr );
	if ( !pWho || !pWho->IsActive() )
		return;
	pkt.Seek(0);
	EPacket t = (EPacket)0;
	pkt.Read( &t, 1 );
	switch ( t )
	{
		case PKT_ADD_CLIENT:
			{
				UCID clientAddr;
				PEER_ID id;
				CMemoryStream addrInfo;
				pkt >> clientAddr;
				pkt >> id;
				int nSize;
				pkt >> nSize;
				if ( nSize < 1024 )
				{
					addrInfo.SetSize( nSize );
					pkt.Read( addrInfo.GetBufferForWrite(), nSize );
					addrInfo.Seek(0);
				}
				ReceiveAddClient( addr, clientAddr, id, addrInfo, bIsBroadcast );
			}
			break;
		case PKT_REMOVE_CLIENT:
			{
				UCID clientAddr;
				pkt >> clientAddr;
				ReceiveRemoveClient( addr, clientAddr, bIsBroadcast );
			}
			break;
		case PKT_BROADCAST_MSG:
			{
				CMemoryStream data;
				int nID;
				pkt >> nID;
				ReadRest( pkt, &data );
				ReceiveBroadcast( addr, data, nID );
			}
			break;
		case PKT_DIRECT_MSG:
			{
				CMemoryStream data;
				ReadRest( pkt, &data );
				ReceiveDirect( addr, data );
			}
			break;
		case PKT_ACK:
			{
				int nID;
				PEER_ID addr;
				pkt >> nID;
				pkt >> addr;
				ReceiveAck( addr, nID, addr );
			}
			break;
		default:
			ASSERT( 0 );
			break;
	}
}
/////////////////////////////////////////////////////////////////////////////////////
}
