#include "stdafx.h"

#include "LinksManager.h"
#include "NetInterfaces.h"
#include "NetLowest.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>

#include <fmt/format.h>

namespace NNet
{

// What GetLastLinksManagerError reports. Links managers are created on the main
// thread, from the multiplayer screens, so one string serves.
static std::string szLastLinksManagerError;

//! "error <code>, <system message>" for the socket call that just failed.
static std::string DescribeLastSocketError()
{
#if BOOST_OS_WINDOWS
	const int nError = WSAGetLastError();
	char szMessage[512] = "";
	FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, nError, 0, szMessage, sizeof( szMessage ), 0 );
	std::string szText = szMessage;
	// FormatMessage ends the text with a line break the message box would show
	while ( !szText.empty() && ( szText.back() == '\n' || szText.back() == '\r' || szText.back() == '.' ) )
	{
		szText.pop_back();
	}
	return fmt::format( "error {}, {}", nError, szText );
#else
	const int nError = errno;
	return fmt::format( "error {}, {}", nError, std::strerror( nError ) );
#endif
}

std::string GetLastLinksManagerError()
{
	return szLastLinksManagerError;
}

// Winsock alone needs starting before a socket exists, and there is nothing to
// start on POSIX, so this object is Windows-only rather than a pair of no-ops.
// It runs as a static constructor, before main, which is where the requirement
// puts it: the first socket may be created from anywhere.
#if BOOST_OS_WINDOWS
static struct SWSAInit
{
	SWSAInit()
	{
		uint16_t wVersionRequested = MAKEWORD( 1, 1 );
		WSADATA wsaData;

		int bRv = WSAStartup( wVersionRequested, &wsaData ) == 0;
		ASSERT( bRv );
	}

	~SWSAInit()
	{
		WSACleanup();
	}
} wsaInit;
#endif

CLinksManagerCommon::CLinksManagerCommon()
{
	s = INVALID_SOCKET;
}

CLinksManagerCommon::~CLinksManagerCommon()
{
	if ( s != INVALID_SOCKET )
	{
		closesocket( s );
	}
	s = INVALID_SOCKET;
}

bool CLinksManagerCommon::Init( const int nPort )
{
	szLastLinksManagerError.clear();
	s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( s == INVALID_SOCKET )
	{
		szLastLinksManagerError = "socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) failed: " + DescribeLastSocketError();
		return false;
	}
	sockaddr_in name;
	memset( &name, 0, sizeof( name ) );
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons( nPort );
	// Port 0 is bound too, to a port the system picks. Unbound, the socket would
	// get that port only at its first sendto, and until then getsockname, and so
	// GetSelfAddress, reports port 0: the login told the host to answer at :0.
	if ( bind( s, (sockaddr *)&name, sizeof( name ) ) != 0 )
	{
		// Described before closesocket, which may overwrite the error.
		// EADDRINUSE here usually means another game instance holds the port.
		szLastLinksManagerError = fmt::format( "bind(UDP 0.0.0.0:{}) failed: {}", nPort, DescribeLastSocketError() );
		closesocket( s );
		s = INVALID_SOCKET;
		return false;
	}
	unsigned long dwOpt = 1;
	ioctlsocket( s, FIONBIO, &dwOpt ); // no block
	setsockopt( s, SOL_SOCKET, SO_BROADCAST, (const char *)&dwOpt, 4 );
	if ( nPort != 0 )
	{
		int nBuf = 0;
		int nBufSize = sizeof( nBuf );
		nBuf = 500000;
		setsockopt( s, SOL_SOCKET, SO_RCVBUF, (char *)&nBuf, nBufSize );
		nBuf = 500000;
		setsockopt( s, SOL_SOCKET, SO_SNDBUF, (char *)&nBuf, nBufSize );
	}
	return true;
}

//! An IPv4 address and port as the CNodeAddress the driver passes around.
static CNodeAddress MakeNodeAddress( uint32_t dwAddress, int nPort )
{
	CNodeAddress addr;
	sockaddr_in *pName = reinterpret_cast<sockaddr_in *>( addr.GetSockAddr() );
	pName->sin_family = AF_INET;
	pName->sin_addr.s_addr = dwAddress;
	pName->sin_port = htons( nPort );
	return addr;
}

void CLinksManagerCommon::MakeBroadcastAddrs( std::vector<CNodeAddress> *pRes, int nPort ) const
{
	pRes->clear();
	// Each interface's own directed broadcast, address | ~mask, which reaches
	// the hosts on that network and this machine as well. The old way took the
	// first address the host name resolved to and guessed its network from the
	// address class, which on Debian's 127.0.1.1 matched no class at all and
	// broadcast to 127.0.1.1 itself, and on a /24 inside 10.0.0.0/8 broadcast
	// to the whole /8.
	std::vector<SInterfaceAddress> interfaces;
	GetInterfaceAddresses( &interfaces );
	std::vector<uint32_t> sent;
	for ( const SInterfaceAddress &item : interfaces )
	{
		const uint32_t dwBroadcast = item.dwAddress | ~item.dwMask;
		// Two addresses on one network share a broadcast address
		if ( item.bBroadcast && std::find( sent.begin(), sent.end(), dwBroadcast ) == sent.end() )
		{
			sent.push_back( dwBroadcast );
			pRes->push_back( MakeNodeAddress( dwBroadcast, nPort ) );
		}
	}
	if ( pRes->empty() )
	{
		// No network at all, but a game on this machine can still be found.
		// Systems differ in which of these two reaches it, so send to both;
		// one that the system refuses fails quietly in Send.
		pRes->push_back( MakeNodeAddress( htonl( INADDR_BROADCAST ), nPort ) );
		pRes->push_back( MakeNodeAddress( htonl( 0x7FFFFFFF ), nPort ) ); // 127.255.255.255
	}
}

bool CLinksManagerCommon::IsLocalAddr( const CNodeAddress &test ) const
{
	const uint32_t dwTest = reinterpret_cast<const sockaddr_in *>( &test.addr )->sin_addr.s_addr;
	if ( AddressOctets( &reinterpret_cast<const sockaddr_in *>( &test.addr )->sin_addr )[0] == 127 )
	{
		return true;
	}
	// On the network of one of this machine's interfaces
	std::vector<SInterfaceAddress> interfaces;
	GetInterfaceAddresses( &interfaces );
	for ( const SInterfaceAddress &item : interfaces )
	{
		if ( ( item.dwAddress & item.dwMask ) == ( dwTest & item.dwMask ) )
		{
			return true;
		}
	}
	return false;
}

bool CLinksManagerCommon::GetSelfAddress( CNodeAddressSet *pRes ) const
{
	pRes->Clear();
	sockaddr_in addr;
	socket_length_t nBufLeng = sizeof( sockaddr_in );
	if ( getsockname( s, (sockaddr *)&addr, &nBufLeng ) != 0 )
	{
		return false;
	}
	pRes->nPort = addr.sin_port;
	// The addresses the host and the other players may reach this machine at,
	// sent with the login. They used to come from resolving the host name too,
	// which on Debian gave only 127.0.1.1. Interfaces that carry broadcasts go
	// first, since those are the local networks a LAN game is played over; a
	// tunnel's address comes after them.
	std::vector<SInterfaceAddress> interfaces;
	GetInterfaceAddresses( &interfaces );
	std::stable_partition(
			interfaces.begin(), interfaces.end(), []( const SInterfaceAddress &item ) { return item.bBroadcast; } );
	int nCount = 0;
	for ( const SInterfaceAddress &item : interfaces )
	{
		if ( nCount == N_MAX_HOST_HOMES )
		{
			break;
		}
		pRes->ips[nCount++] = static_cast<int>( item.dwAddress );
	}
	if ( nCount == 0 )
	{
		// No network: this machine is reachable only by itself
		pRes->ips[0] = static_cast<int>( htonl( INADDR_LOOPBACK ) );
	}
	return true;
}

// CRAP{
extern int nTrafficPackets;
extern int nTrafficTotalSize;
// CRAP}

#ifdef NET_TEST_APPLICATION
bool bEmulateWeakNetwork = false;
float fLostRate = 0.7f;

struct SPacket
{
	CNodeAddress addr;
	CMemoryStream pkt;
};
#endif
bool CLinksManagerCommon::Send( const CNodeAddress &dst, CMemoryStream &pkt ) const
{
	if ( !IsGoodAddress( dst.addr ) )
	{
		return true;
	}

#ifdef NET_TEST_APPLICATION
	static std::vector<SPacket> pktQueue[10];
	if ( bEmulateWeakNetwork )
	{
		if ( rand() <= RAND_MAX * fLostRate )
		{
			return true;
		}
		pktQueue.push_back();
		pktQueue.back().addr = dst;
		pktQueue.back().pkt = pkt;
		while ( pktQueue.size() > 3 )
		{
			int nPkt = rand() % pktQueue.size();
			SPacket &p = pktQueue[nPkt];
			int nSize = p.pkt.GetSize();
			int nRv = sendto( s, (const char *)p.pkt.GetBuffer(), nSize, 0, &p.addr.addr, sizeof( p.addr.addr ) );

			pktQueue.erase( pktQueue.begin() + nPkt );
		}
		return true;
	}
#endif

	const int nSize = pkt.GetSize();
	const int nRv = sendto( s, (const char *)pkt.GetBuffer(), nSize, 0, &dst.addr, sizeof( dst.addr ) );

	// CRAP{
	if ( nRv >= 0 )
	{
		++nTrafficPackets;
		nTrafficTotalSize += nRv;
	}
	// CRAP}

	return nRv == nSize;
}

bool CLinksManagerCommon::Recv( CNodeAddress *pSrc, CMemoryStream *pPkt ) const
{
	ASSERT( pSrc );
	ASSERT( pPkt );

	socket_length_t nAddrSize;
	pPkt->Seek( 2048 );
	nAddrSize = sizeof( pSrc->addr );
	int nRes = recvfrom( s, (char *)pPkt->GetBufferForWrite(), 2048, 0, &pSrc->addr, &nAddrSize );

	const bool bGoodAddress = IsGoodAddress( pSrc->addr );
	if ( !bGoodAddress )
	{
		nRes = 0;
	}

	if ( nRes >= 0 )
	{
		pSrc->addr.sa_family = AF_INET; // somehow this gets spoiled on win2k
		memset( pSrc->addr.sa_data + 6, 0, 8 );
		pPkt->SetSize( nRes );
	}

	pPkt->Seek( 0 );

	// CRAP{
	if ( nRes >= 0 )
	{
		++nTrafficPackets;
		nTrafficTotalSize += nRes;
	}
	// CRAP}

	return bGoodAddress && nRes >= 0;
}

class CLinksManager : public CLinksManagerCommon
{
	OBJECT_NOCOPY_METHODS( CLinksManager );

public:
};

ILinksManager *CreateClientLinksManager()
{
	CPtr<CLinksManager> pManager = new CLinksManager();
	const bool bSuccess = pManager->Init( 0 );

	return bSuccess ? pManager.Extract() : 0;
}

ILinksManager *CreateServerLinksManager( const int nPort )
{
	CPtr<CLinksManager> pManager = new CLinksManager();
	const bool bSuccess = pManager->Init( nPort );

	return bSuccess ? pManager.Extract() : 0;
}

} // namespace NNet

BASIC_REGISTER_CLASS( NET, NNet::ILinksManager )
BASIC_REGISTER_CLASS( NET, NNet::CLinksManager )
