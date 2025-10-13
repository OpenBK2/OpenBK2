#include "stdafx.h"

#include "Misc/win32helper.h"
#include "System/Time.h"
#include "NetA4.h"

#include <cstdint>

// for testing

//#define LOG
#ifdef LOG
#include <iostream>
#endif

const int PACKET_SIZE = 500;

namespace NNet
{
using namespace NWin32Helper;
static NWin32Helper::CCriticalSection netDriverCriticalSection;
const float MIN_TIME_BETWEEN_PACKETS_PER_PEER = 0.050f;

// interaction with master server is accomplished with different object
// if so then it should be possible to start

enum EPacket
{
	NORMAL,
	LOGIN,
	REQUEST_SERVER_INFO,
	SERVER_INFO,
	ACCEPTED,
	REJECTED,
	LOGOUT,
	TRY_SHORTCUT,
	NOP,
	KICK,
};

void GetNOPStream( CMemoryStream *pNOP )
{
	pNOP->Clear();
	pNOP->Seek( 0 );
	EPacket ePacket = NOP;
	pNOP->Write( &ePacket, 1 );
}

const int N_APPLICATIONID = 0x45143100;
class CSendPacket
{
	ILinksManager *pLinks;
	const CNodeAddress &addr;
	static CMemoryStream pkt;
	static CBitLocker bits;
	static bool bLastPacket;
public:
	CSendPacket( const CNodeAddress &_addr, EPacket packet, ILinksManager *_pLinks ) : addr( _addr ), pLinks( _pLinks )
	{
		pkt.Seek(0);
		bits.LockWrite( pkt, N_MAX_PACKET_SIZE + 1024 );
		bits.Write( &packet, 1 );
	}
	CSendPacket( const CNodeAddress &_addr, EPacket packet, CP2PTracker::UCID clientID, ILinksManager *_pLinks ): addr(_addr), pLinks(_pLinks)
	{
		pkt.Seek(0);
		bits.LockWrite( pkt, N_MAX_PACKET_SIZE + 1024 );
		bits.Write( &packet, 1 );
		bits.Write( &clientID, sizeof(clientID) );
	}
	~CSendPacket()
	{
		bits.Free();
		pkt.SetSize( pkt.GetPosition() );
		bLastPacket = pLinks->Send( addr, pkt );
	}
	CBitStream* GetBits() { return &bits; }
	static bool GetResult() { return bLastPacket; }
};
CMemoryStream CSendPacket::pkt;
CBitLocker CSendPacket::bits;
bool CSendPacket::bLastPacket;

// packet to/from stream

static bool CanReadPacket( CRingBuffer<N_STREAM_BUFFER> &buf )
{
	if ( buf.GetSize() < 4 )
		return false;
	int nSize;
	buf.Peek( &nSize, 4 );
	if ( nSize & 1 )
		nSize &= 0xff;
	nSize >>= 1;
	if ( buf.GetSize() >= nSize + (nSize >= 128 ? 4 : 1) )
		return true;
	return false;
}

static void WritePacket( std::list<CMemoryStream> *pDst, CMemoryStream &pkt )
{
	NI_ASSERT( pkt.GetSize() < N_STREAM_BUFFER - 1000, StrFmt( "Wrong memory stream size (%d)", pkt.GetSize() ) );
	CMemoryStream &b = pDst->emplace_back();
	int nSize = pkt.GetSize();
	if ( nSize >= 128 )
	{
		nSize <<= 1;
		b.Write( &nSize, 4 );
	}
	else
	{
		nSize <<= 1;
		nSize |= 1;
		b.Write( &nSize, 1 );
	}
	b.Write( pkt.GetBuffer(), pkt.GetSize() );
}
static void ReadPacket( CRingBuffer<N_STREAM_BUFFER> &src, CMemoryStream *pDst )
{
	NI_ASSERT( CanReadPacket( src ), "Can't read a packet" );
	int nSize = 0;
	src.Read( &nSize, 1 );
	if ( (nSize & 1) == 0 )
		src.Read( ((char*)&nSize) + 1, 3 );
	nSize >>= 1;
	pDst->SetSizeDiscard( nSize );
	src.Read( pDst->GetBufferForWrite(), nSize );
}

// ************************************************************************************************************************ //
// **
// ** net driver
// **
// **
// **
// ************************************************************************************************************************ //

static unsigned long WINAPI TheThreadProc( LPVOID lpParameter )
{
	CNetDriver *pNet = reinterpret_cast<CNetDriver*>(lpParameter);
	// run finction
	pNet->StartThread();
	while ( pNet->CanWork()  )
	{
		Sleep( 1 );
		pNet->Step();
	}
	pNet->FinishThread();
	return 0;
}

IDriver::EState CNetDriver::GetState() const 
{
	// Тут не локаем треды, так как большой погоды это не делает, а производительность ест и мешает работать с сетью
//	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	return state; 
}

void CNetDriver::StartThread()
{
	SetEvent( hThreadReport );
}

bool CNetDriver::CanWork()
{
	return IsValid(this) && WaitForSingleObject( hStopCommand, 0 ) != WAIT_OBJECT_0;
}

void CNetDriver::FinishThread()
{
	SetEvent( hThreadReport );
}

void CNetDriver::CreateEvents()
{
	hThread = 0;
	hThreadReport = CreateEvent( 0, true, false, 0 );
	hStopCommand = CreateEvent( 0, true, false, 0 );
}

CNetDriver::CNetDriver() : serverInfo( 0 ), login( 0 ), state( INACTIVE )
{
	CreateEvents();
	bIsBroadcast = true;
}

CNetDriver::CNetDriver( const SNetDriverConsts &_consts, bool _bIsBroadcast ) 
: serverInfo( 0 ), login( 0 ), state( INACTIVE ), consts(_consts), bIsBroadcast(_bIsBroadcast)
{
	CreateEvents();
}

void CNetDriver::KillThread()
{
	if ( hThread )
	{
		SetEvent( hStopCommand );
		WaitForSingleObject( hThreadReport, INFINITE );

		CloseHandle( hThread );
		hThread = 0;
	}
}

CNetDriver::~CNetDriver()
{
	KillThread();

	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	
	switch ( state )
	{
		case ACTIVE:
			{
				if ( ( !bIsClient ) || ( bIsBroadcast ) )
					for ( CPeerList::iterator i = clients.begin(); i != clients.end(); ++i )
					{
						CSendPacket p( i->second.currentAddr, LOGOUT, i->second.clientID, pLinks );
					}
				else
					//sends message to server only
					for ( CPeerList::iterator i = clients.begin(); i != clients.end(); ++i )
					{
						if ( i->second.clientID == 0 ) 
							CSendPacket p( i->second.currentAddr, LOGOUT, i->second.clientID, pLinks );
					}
					break;
			}
		case CONNECTING:
			{
				CSendPacket p( login.GetLoginTarget(), LOGOUT, -1, pLinks );
			}
			break;
	}

	CloseHandle( hThreadReport );
	CloseHandle( hStopCommand );
}

void CNetDriver::Init( const APPLICATION_ID nApplicationID, const int _nGamePort, bool bClientOnly, ILinksManager *pLinksManager )
{
	NI_ASSERT( pLinksManager != 0, "Can't initialize by 0 linksManager" );
	pLinks = pLinksManager;
	
	serverInfo.Init( nApplicationID );
	login.Init( nApplicationID );
	nGamePort = _nGamePort;
	//
	state = INACTIVE;
	lastReject = NONE;
	bAcceptNewClients = true;
	NHPTimer::GetTime( &lastTime );
	//
	bIsClient = bClientOnly;

#ifdef __TEST_LAGS__
	lastSendTime = 0;
	lastReceiveTime = 0;
	nMsgCanReceive = 0;
	lagPeriod = 0;
	bPaused = false;
	bSendNow = false;
	bReceiveNow = false;
#endif // __TEST_LAGS__

	unsigned long dwThreadId;
	ResetEvent( hStopCommand );
	ResetEvent( hThreadReport );
	hThread = CreateThread( 0, 1024*1024, TheThreadProc, reinterpret_cast<LPVOID>(this), 0, &dwThreadId );
	WaitForSingleObject( hThreadReport, INFINITE );
	ResetEvent( hThreadReport );
}

CNetDriver::SPeer* CNetDriver::GetClientByAddr( const CNodeAddress &addr )
{
	for ( CPeerList::iterator i = clients.begin(); i != clients.end(); ++i )
	{
		if ( i->second.currentAddr == addr )
			return &(i->second);
	}
	return 0;
}

CNetDriver::SPeer* CNetDriver::GetClient( CP2PTracker::UCID nID )
{
	CPeerList::iterator i = clients.find( nID );
	if ( i != clients.end() )
		return &(i->second);
	return 0;
}

void CNetDriver::AddClient( const SClientAddressInfo &addr, CP2PTracker::UCID clientID )
{
	NI_ASSERT( clients.find( clientID ) == clients.end(), "Duplicate peer" );
	SPeer &peer = clients[clientID];
	peer.currentAddr = addr.inetAddress;
	peer.clientID = clientID;
	peer.addrInfo = addr;
	CNodeAddress test;
	addr.localAddress.GetAddress( 0, &test );
	peer.bTryShortcut = !addr.inetAddress.SameIP( test );
	peer.bTryShortcut |= !addr.localAddress.GetAddress( 1, &test );
}

void CNetDriver::AddNewP2PClient( const SClientAddressInfo &addr, CP2PTracker::UCID clientID )
{
	CMemoryStream addrInfo;
	addrInfo.Write( &addr, sizeof( addr ) );
	p2p.AddNewClient( clientID, addrInfo, bIsBroadcast );
}

void CNetDriver::RemoveClient( CP2PTracker::UCID nID )
{
	clients.erase( nID );
}

bool CNetDriver::SendBroadcast( const CMemoryStream &_pkt )
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );

	if ( state != ACTIVE )
		return false;
	//
	CMemoryStream pkt = _pkt;
	// CRAP{ for traffic measurement
	nSent += pkt.GetSize();
	// CRAP}

	p2p.SendBroadcast( pkt );
	return true;
}

bool CNetDriver::SendDirect( int nClient, const CMemoryStream &_pkt )
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	CMemoryStream pkt = _pkt;

	// CRAP{ for traffic measurement
	nSent += pkt.GetSize();
	// CRAP}

	SPeer *pDst = GetClient( nClient );
	if ( pDst == 0 || state != ACTIVE )
		return false;

	if ( pDst )
		p2p.SendDirect( pDst->clientID, pkt );

	return true;
}

void CNetDriver::Kick( int nClient )
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	
	SPeer *pDst = GetClient( nClient );
//	NI_ASSERT( pDst != 0, "pDst == 0 " );
	NI_ASSERT( state == ACTIVE, "Wrong state of the game" );
	if ( pDst )
		p2p.KickClient( nClient, bIsBroadcast );
}

#ifdef __TEST_LAGS__
bool CNetDriver::AnalyzeLags()
{
	if ( bPaused )
		return false;

	if ( bReceiveNow )
		return true;
	
	if ( Singleton<IGameTimer>() )
	{
		const NTimer::STime curTime = Singleton<IGameTimer>()->GetAbsTime();
		// all messages are received and it's lag now
		if ( nMsgCanReceive == 0 && lastReceiveTime + lagPeriod > curTime )
			return false;
		// lag finished
		if ( lastReceiveTime + lagPeriod <= curTime )
		{
			lastReceiveTime = curTime;
			if ( bMultiChannel )
			{
				for ( int i = 0; i < channelMsgs.size(); ++i )
					nMsgCanReceive += channelMsgs[i].size();
			}
			else
				nMsgCanReceive += msgQueue.size();
		}

		if ( nMsgCanReceive > 0 )
			--nMsgCanReceive;
	}

	return true;
}
#endif // __TEST_LAGS__

bool CNetDriver::GetMessage( EMessage *pMsg, int *pClientID, std::vector<int> *pReceived, CMemoryStream *pPkt )
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );

#ifdef __TEST_LAGS__
if ( !AnalyzeLags() )
	return false;
#endif // __TEST_LAGS__
	
	if ( msgQueue.empty() )
		return false;
	else
	{
		SMessage &msg = msgQueue.front();
		*pMsg = msg.msg;
		*pClientID = msg.nClientID;
		if ( pReceived )
			*pReceived = msg.received;
		*pPkt = msg.pkt;
		//
		msgQueue.pop_front();
	}

	return true;
}

void CNetDriver::ProcessLogin( const CNodeAddress &addr, CBitStream &bits )
{
	// if can accept login requests only
	CLoginSupport::SLoginInfo info;
	if ( login.ProcessLogin( addr, bits, &info ) )
	{
		bool bAddp2pClient = false;		
		EReject reject = NONE;
		if ( info.bWrongVersion )
			reject = WRONG_VERSION;
		else
		{
			SPeer *pPeer = GetClientByAddr( addr );
			if ( pPeer )
			{
				if ( !login.HasAccepted( addr, info ) )
					return; // ignore obsolete or too new request
			}
			else if ( info.pwd.GetSize() ) // wrong password
				reject = PASSWORD_FAILED;
			else
			{
				if ( !bAcceptNewClients )
					reject = FORBIDDEN;
				else
					bAddp2pClient = true;
			}
		}
		if ( reject != NONE )
		{
			CSendPacket pkt( addr, REJECTED, pLinks );
			login.RejectLogin( addr, pkt.GetBits(), info, (int)reject );
		}
		else
		{
			int nClientID;
#ifdef LOG
			cout << "client " << addr.GetFastName() << " logged in" << endl;
#endif
			CSendPacket pkt( addr, ACCEPTED, pLinks );
			CNodeAddressSet localAddr;
			bool bGetSelf = pLinks->GetSelfAddress( &localAddr );
			ASSERT( bGetSelf );
			login.AcceptLogin( addr, pkt.GetBits(), info, &nClientID, localAddr );

			if ( bAddp2pClient )
				AddNewP2PClient( SClientAddressInfo( addr, info.localAddr ), nClientID );
		}
	}
}

void CNetDriver::ProcessNormal( const CNodeAddress &addr, CBitStream &bits )
{
	CP2PTracker::UCID clientID = -1;
	bits.Read( &clientID, sizeof(clientID) );
	if ( !p2p.IsActive( clientID ) )
		return;
	SPeer *pPeer = GetClient( clientID );
	if ( pPeer )
	{
		pPeer->currentAddr = addr;
		pPeer->bTryShortcut = false;
		std::vector<PACKET_ID> acked;
		NI_ASSERT( pPeer->data.CanReadMsg(), "data polling is not perfect" ); // data polling is not perfect
		if ( pPeer->data.CanReadMsg() && pPeer->acks.ReadAcks( &acked, bits ) )
		{
			pPeer->data.ReadMsg( bits );
			pPeer->data.Commit( acked );
			PollMessages( pPeer );			
		}
	}
	else
	{
		//if (pCSLog) (*pCSLog) << "normal packet from non client received from " << addr.GetFastName() << endl;
	}
}

void CNetDriver::ProcessIncomingMessages()
{
	// process incoming packets
//	CNodeAddress addr;
	static CMemoryStream pkts;
	bool bContinue = true;
	while ( bContinue && pLinks->Recv( &addr, &pkts ) )
	{
		if ( pkts.GetSize() == 0 )
		{
			//if (pCSLog) (*pCSLog) << "ZERO length packet received from " << addr.GetFastName() << endl;
			continue;
		}
		EPacket cmd = (EPacket)0;
		pkts.Read( &cmd, 1 );

		CBitStream bits( pkts.GetBufferForWrite() + 1, CBitStream::read, pkts.GetSize() - 1 );
		switch ( cmd )
		{
			case NORMAL:
				ProcessNormal( addr, bits );
				break;
			case LOGIN:
				ProcessLogin( addr, bits );
				break;
			case REQUEST_SERVER_INFO:
				if ( serverInfo.DoReplyRequest() )
				{
					CSendPacket pkt( addr, SERVER_INFO, pLinks );
					serverInfo.ReplyServerInfoRequest( bits, pkt.GetBits() );
				}
				break;
			case SERVER_INFO:
				serverInfo.ProcessServerInfo( addr, bits, consts.fServerListTimeout );
				break;
			case ACCEPTED:
				login.ProcessAccepted( addr, bits );
				bContinue = false;
				break;
			case REJECTED:
				login.ProcessRejected( addr, bits );
				bContinue = false;
				break;
			case LOGOUT:
				{
					CP2PTracker::UCID clientID = -1;
					bits.Read( &clientID, sizeof( clientID ) );
					p2p.KickClient( clientID, bIsBroadcast );
					SPeer *pPeer = GetClientByAddr( addr );
					if ( pPeer )
						p2p.KickClient( pPeer->clientID, bIsBroadcast );
				}
				break;
			case TRY_SHORTCUT:
				{
					CP2PTracker::UCID clientID = -1;
					bits.Read( &clientID, sizeof(clientID) );
					CLoginSupport::TServerID uniqueServerID;
					bits.Read( uniqueServerID );
					if ( uniqueServerID == login.GetUniqueServerID() )
					{
						if ( !p2p.IsActive( clientID ) )
							return;
						SPeer *pPeer = GetClient( clientID );
						if ( pPeer )
						{
							pPeer->currentAddr = addr;
							pPeer->bTryShortcut = false;
						}
					}
				}
				break;
			case NOP:
				break;
			case KICK:
				// was kicked
				{
					CMemoryStream fake;
					std::vector<CP2PTracker::UCID> fake1;

					AddOutputMessage( KICKED, 0, fake, fake1 );
					while ( !clients.empty() )
					{
						CP2PTracker::UCID clientID = clients.begin()->first;
						AddOutputMessage( REMOVE_CLIENT, clientID, fake, fake1 );
						if ( clientID == 0 )
							AddOutputMessage( SERVER_DEAD, 0, fake, fake1 );
						RemoveClient( clientID );
					}
				}
				break;
			default:
				// Unknown UDP packet. Maybe a remote spam. The packet should be ignored.
				//NI_ASSERT( 0 && "Unknown command", "Unknown command" );
				break;
		}
	}
}

void CNetDriver::StepInactive()
{
	std::vector<CNodeAddress> dest;
	CNodeAddress broadcast;
	pLinks->MakeBroadcastAddr( &broadcast, nGamePort );
	if ( serverInfo.CanSendRequest( broadcast, &dest ) )
	{
		for ( int i = 0; i < dest.size(); ++i )
		{
			CSendPacket pkt( dest[i], REQUEST_SERVER_INFO, pLinks );
			serverInfo.WriteRequest( pkt.GetBits() );
		}
	}
}

void CNetDriver::StepConnecting()
{
	switch ( login.GetState() )
	{
		case CLoginSupport::INACTIVE:
			lastReject = TIMEOUT;
			state = INACTIVE;
			break;
		case CLoginSupport::LOGIN:
			if ( login.CanSend() )
			{
				CNodeAddressSet localAddr;
				if ( !pLinks->GetSelfAddress( &localAddr ) )
				{
					CSendPacket pkt( login.GetLoginTarget(), NOP, pLinks );
				}
				bool bGetSelf = pLinks->GetSelfAddress( &localAddr );
				ASSERT( bGetSelf );
				CSendPacket pkt( login.GetLoginTarget(), LOGIN, pLinks );
				login.WriteLogin( pkt.GetBits(), localAddr );
			}
			break;
		case CLoginSupport::ACCEPTED:
			AddNewP2PClient( SClientAddressInfo( login.GetLoginTarget(), login.GetTargetLocalAddr() ), 0 );
			state = ACTIVE;
			StepActive( 0 );
			break;
		case CLoginSupport::REJECTED:
			lastReject = (EReject)login.GetRejectReason();
			state = INACTIVE;
			break;
		default:
			NI_ASSERT( 0, "Unknown message" );
			break;
	}
}

void CNetDriver::AddOutputMessage( EMessage msg, const CP2PTracker::UCID _from, 
		CMemoryStream &data, const std::vector<CP2PTracker::UCID> &received )
{
	if ( msg == KICKED )
	{
		SMessage &res = msgQueue.emplace_back();
		res.msg = msg;
		return;
	}
	SPeer *pPeer = GetClient( _from );
	NI_ASSERT( pPeer != 0, "NULL peer" );
	if ( !pPeer )
		return;
	SMessage &res = msgQueue.emplace_back();
	res.msg = msg;
	res.pkt = data;
	res.nClientID = pPeer->clientID;
	for ( int i = 0; i < received.size(); ++i )
	{
		SPeer *pTest = GetClient( received[i] );
		NI_ASSERT( pTest != 0, "NULL pTest" );
		if ( pTest )
			res.received.push_back( pTest->clientID );
	}
}

void CNetDriver::PollMessages( SPeer *pPeer )
{
	// seek for packets through incoming traffic
	if ( CanReadPacket( pPeer->data.channelInBuf ) )
	{
		CMemoryStream pkt;
		ReadPacket( pPeer->data.channelInBuf, &pkt );
#ifdef LOG
		cout << "receive packet from " << pPeer->addr.GetFastName() << " size=" << pkt.GetSize() << endl;
#endif
		p2p.ProcessPacket( pPeer->clientID, pkt, bIsBroadcast );
	}
}

void CNetDriver::ProcessP2PMessages()
{
	CP2PTracker::SMessage msg;
	while ( p2p.GetMessage( &msg ) )
	{
		switch ( msg.msg )
		{
		case CP2PTracker::NEW_CIENT:
			{
				SClientAddressInfo addr;
				msg.pkt.Seek(0);
				msg.pkt.Read( &addr, sizeof(addr) );
				AddClient( addr, msg.from );
				AddOutputMessage( NEW_CLIENT, msg.from, msg.pkt, msg.received );
			}

			break;
		case CP2PTracker::REMOVE_CLIENT:
			AddOutputMessage( REMOVE_CLIENT, msg.from, msg.pkt, msg.received );

			if ( msg.from == 0 )
				AddOutputMessage( SERVER_DEAD, msg.from, msg.pkt, msg.received );

			RemoveClient( msg.from );

			break;
		case CP2PTracker::DIRECT:
			AddOutputMessage( DIRECT, msg.from, msg.pkt, msg.received );
			break;
		case CP2PTracker::BROADCAST:
			AddOutputMessage( BROADCAST, msg.from, msg.pkt, msg.received );
			break;
		}
	}
}

void CNetDriver::StepActive( float fDeltaTime )
{
	// rollback outdated packets
	for ( CPeerList::iterator i = clients.begin(); i != clients.end(); ++i )
	{
		std::vector<PACKET_ID> rolled, erased;
		i->second.acks.Step( &rolled, &erased, fDeltaTime, consts.fServerListTimeout );
		i->second.data.Rollback( rolled );
		i->second.data.Erase( erased );
		PollMessages( &(i->second) );
	}

	// give out kicks
	for ( int k = 0; k < p2p.kicks.size(); ++k )
	{
		SPeer *pPeer = GetClient( p2p.kicks[k] );
		if ( pPeer )
			CSendPacket pkt( pPeer->currentAddr, KICK, login.GetSelfClientID(), pLinks );
	}
	p2p.kicks.clear();

	ProcessP2PMessages();

	// form output packets
	for ( int pi = 0; pi < p2p.packets.size(); ++pi )
	{
		CP2PTracker::SPacket &p = p2p.packets[pi];
		SPeer *pPeer = GetClient( p.addr );
		if ( pPeer )
		{
			WritePacket( &pPeer->data.outList, p.pkt );

#ifdef LOG
			cout << "output packet to " << p.addr.GetFastName() << " size=" << p.pkt.GetSize() << endl;
#endif
		}
		else
		{
#ifdef LOG
			cout << "DISCARD packet to " << p.addr.GetFastName() << " size=" << p.pkt.GetSize() << endl;
#endif
		}
	}
	p2p.packets.resize( 0 );

	int nKicksLeft = 5; // limit the number of timeout kicks per step
	// send updates & check for timeouts
	for ( CPeerList::iterator it = clients.begin(); it != clients.end(); ++it )
	{
		if ( it->second.acks.GetTimeSinceLastRecv() > consts.fTimeout && nKicksLeft > 0 )
		{
			p2p.KickClient( it->second.clientID, bIsBroadcast );
			--nKicksLeft;
			break;
		}
	}
	
	for ( CPeerList::iterator it = clients.begin(); it != clients.end(); ++it )
	{
		if ( p2p.IsActive( it->second.clientID ) )
		{
			if ( it->second.acks.CanSend() && ( it->second.data.HasOutData() || it->second.acks.NeedSend() ) && it->second.fTimeToSendData <= 0 )
			{
				it->second.fTimeToSendData = MIN_TIME_BETWEEN_PACKETS_PER_PEER;
				if ( it->second.bTryShortcut )
				{
					for ( int k = 0; 1; ++k )
					{
						CNodeAddress dest;
						if ( !it->second.addrInfo.localAddress.GetAddress( k, &dest ) )
							break;
						CSendPacket pkt( dest, TRY_SHORTCUT, login.GetSelfClientID(), pLinks );
						pkt.GetBits()->Write( login.GetUniqueServerID() );
					}
				}

				PACKET_ID pktID;
				{
					CSendPacket pkt( it->second.currentAddr, NORMAL, login.GetSelfClientID(), pLinks );
					pktID = it->second.acks.WrtieAcks( pkt.GetBits(), PACKET_SIZE ); // CRAP - packet size limits??
					it->second.data.WriteMsg( pktID, pkt.GetBits(), PACKET_SIZE );
				}
				
				if ( !CSendPacket::GetResult() )
				{
					it->second.acks.PacketLost( pktID );
					std::vector<PACKET_ID> roll;
					roll.push_back( pktID );
					it->second.data.Rollback( roll );
					break;
				}
			}
			else
			{
				it->second.fTimeToSendData -= fDeltaTime;
			}
		}
	}
}

// CRAP{
int nTrafficPackets;
int nTrafficTotalSize;
// CRAP}

void CNetDriver::Step()
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );

	if ( !CanWork() )
		return;

	{
		// CRAP{
		
		// for traffic to winsock measurement
		// use external code to do this
		// doing this here puck ups due to Step() calls from other thread
	 // if ( Singleton<IGameTimer>() != 0  )	
		//{
		//	NTimer::STime curTime = Singleton<IGameTimer>()->GetAbsTime();
		//	if ( lastTrafficCheckTime > curTime )
		//		lastTrafficCheckTime = curTime;

		//	if ( lastTrafficCheckTime + 1000 < curTime )
		//	{
		//		lastTrafficCheckTime = curTime;
		//		if ( Singleton<IConsoleBuffer>() != 0 )
		//			Singleton<IConsoleBuffer>()->WriteASCII
		//				( CONSOLE_STREAM_CONSOLE,
		//					StrFmt( "Packets %d, total size %d, sent to driver %d", nTrafficPackets, nTrafficTotalSize, nSent ),
		//					0xffff0000, true
		//				);

		//		nTrafficPackets = 0;
		//		nTrafficTotalSize = 0;
		//		nSent = 0;
		//	}
		//}
		// CRAP}
		//

		float fSeconds = NHPTimer::GetTimePassed( &lastTime );
		serverInfo.Step( fSeconds );
		login.Step( fSeconds );
		ProcessIncomingMessages();
		//
		switch ( state )
		{
			case INACTIVE:
				StepInactive();
				break;
			case ACTIVE:
				StepActive( fSeconds );
				break;
			case CONNECTING:
				StepConnecting();
				break;
		}
	}
}

void CNetDriver::StartGame()
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );

	NI_ASSERT( state == INACTIVE, "Wrong state of the game" );
	state = ACTIVE;
}

void CNetDriver::ConnectGame( const CNodeAddress &addr, const CMemoryStream &pwd )
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	
	state = CONNECTING;
	login.StartLogin( addr, pwd );

	gameHostAddress.Clear();
	gameHostAddress.SetInetName( addr.GetFastName().c_str(), 0 );
}

void CNetDriver::StartGameInfoSend( const CMemoryStream &pkt )
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	serverInfo.StartReply( pkt );
}

void CNetDriver::StartGameInfoSend( const SGameInfo &_gameInfo )
{
	CMemoryStream memStream;
	{
		CObj<IBinSaver> p = CreateBinSaver( &memStream, SAVER_MODE_WRITE );
		SGameInfo gameInfo = _gameInfo;
		gameInfo.gameSettings.Seek( 0 );
		p->Add( 1, &gameInfo );
	}
	StartGameInfoSend( memStream );
}

void CNetDriver::StopGameInfoSend()
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	
	serverInfo.StopReply();
}

bool CNetDriver::GetGameInfo( int nIdx, CNodeAddress *pAddr, bool *pWrongVersion, float *pPing, SGameInfo *pGameInfo )
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	
	CServerInfoSupport::CServerInfoList &servers = serverInfo.GetServers();
	CServerInfoSupport::CServerInfoList::iterator k = servers.begin();
	
	for ( ; k != servers.end(); ++k, --nIdx )
	{
		if ( nIdx <= 0 )
		{
			*pAddr = k->addr;
			*pWrongVersion = k->bWrongVersion;
			*pPing = k->fPing;
			{
				CObj<IBinSaver> p = CreateBinSaver( &k->info, SAVER_MODE_READ );
				p->Add( 1, &(*pGameInfo) );
			}

			if ( pGameInfo->nMaxPlayers * pGameInfo->nCurPlayers == 0 )
			{
				++nIdx;
				continue;
			}

			return true;
		}
	}

	return false;
}

void CNetDriver::StartNewPlayerAccept() 
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );

	bAcceptNewClients = true;
}

void CNetDriver::StopNewPlayerAccept()
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	
	bAcceptNewClients = false;
}

const float CNetDriver::GetPing( int nClientID )
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );

	CPeerList::const_iterator iter = clients.find( nClientID );
	if ( iter == clients.end() )
		return -1.0f;
	else
		return iter->second.acks.GetPing();
}

const float CNetDriver::GetTimeSinceLastRecv( const int nClientID )
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	
	CPeerList::const_iterator iter = clients.find( nClientID );
	if ( iter == clients.end() )
		return -1.0f;
	else
		return iter->second.acks.GetTimeSinceLastRecv();	
}

// for debug
const char* CNetDriver::GetAddressByClientID( const int nClientID ) const
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	CPeerList::const_iterator iter = clients.find( nClientID );
	if ( iter == clients.end() )
		return StrFmt( "Invalid client %d", nClientID );
	else
		return iter->second.currentAddr.GetFastName().c_str();
}

const std::string CNetDriver::GetIP( const int nClientID )
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	CPeerList::iterator iter = clients.find( nClientID );
	return iter == clients.end() ? 0 : iter->second.currentAddr.GetIP();
}

const int CNetDriver::GetPort( const int nClientID )
{
	CCriticalSectionLock criticalSectionLock( netDriverCriticalSection );
	CPeerList::iterator iter = clients.find( nClientID );
	return iter == clients.end() ? 0 : iter->second.currentAddr.GetPort();
}

void CNetDriver::PauseNet()
{
#ifdef __TEST_LAGS__
	bPaused = true;
#endif // __TEST_LAGS__
}

void CNetDriver::UnpauseNet()
{
#ifdef __TEST_LAGS__
	bPaused = false;
#endif // __TEST_LAGS__
}

void CNetDriver::SetLag( const NTimer::STime period )
{
#ifdef __TEST_LAGS__
	lagPeriod = period;
#endif // __TEST_LAGS__
}

IDriver* CreateNetDriver( const SNetDriverConsts &consts, bool bIsBroadcast )
{	
	return new NNet::CNetDriver( consts, bIsBroadcast );
}

}

