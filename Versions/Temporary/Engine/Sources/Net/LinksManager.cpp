#include "stdafx.h"

#include "LinksManager.h"
#include "NetLowest.h"

#include <cstdint>

namespace NNet
{

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

CLinksManagerCommon::CLinksManagerCommon()
{
	s = INVALID_SOCKET;
	// get host for broadcast addresses formation
	char szHost[1024];
	if ( gethostname( szHost, 1000 ) )
	{
		ASSERT(0);
		return;
	}
	hostent *he;
	he = gethostbyname( szHost ); // m.b. it is string comp.domain
	if ( he == NULL )
	{
		ASSERT(0);
		return;
	}
	// form addresses
	CNodeAddress addr;
	sockaddr_in &name = *(sockaddr_in*)&addr;
	name.sin_family = AF_INET;
	// hostent are broken for some unknown reason, only one address is valid
	unsigned long *pAddr = (unsigned long*)( he->h_addr_list[0] );
	//for ( ; *pAddr; pAddr++ )
	{
		name.sin_addr.S_un.S_addr = pAddr[0];
		unsigned char bClass = name.sin_addr.S_un.S_un_b.s_b1;
		if ( bClass >= 1 && bClass <= 126 )
		{
			name.sin_addr.S_un.S_un_b.s_b2 = 255;
			name.sin_addr.S_un.S_un_b.s_b3 = 255;
			name.sin_addr.S_un.S_un_b.s_b4 = 255;
		}
		if ( bClass >= 128 && bClass <= 191 )
		{
			name.sin_addr.S_un.S_un_b.s_b3 = 255;
			name.sin_addr.S_un.S_un_b.s_b4 = 255;
		}
		if ( bClass >= 192 && bClass <= 223 )
			name.sin_addr.S_un.S_un_b.s_b4 = 255;
		broadcastAddr = addr;
	}
}

CLinksManagerCommon::~CLinksManagerCommon()
{
	if ( s != INVALID_SOCKET )
		closesocket( s );
	s = INVALID_SOCKET;
}

bool CLinksManagerCommon::Init( const int nPort )
{
	s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( s == INVALID_SOCKET )
		return false;
	sockaddr_in name;
	memset( &name, 0, sizeof(name) );
	name.sin_family = AF_INET;
	name.sin_addr.S_un.S_addr = INADDR_ANY;
	name.sin_port = htons( nPort );
	if ( nPort > 0 )
	{
		if ( bind( s, (sockaddr*)&name, sizeof( name ) ) != 0 )
		{
			closesocket( s );
			s = INVALID_SOCKET;
			return false;
		}
	}
	unsigned long dwOpt = 1;
	ioctlsocket( s, FIONBIO, &dwOpt ); // no block
	setsockopt( s, SOL_SOCKET, SO_BROADCAST, (const char*)&dwOpt, 4 );
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

bool CLinksManagerCommon::MakeBroadcastAddr( CNodeAddress *pRes, int nPort ) const
{
	*pRes = broadcastAddr;
	sockaddr_in &name = *(sockaddr_in*)&pRes->addr;
	name.sin_port = htons( nPort );

	return true;
}

bool CLinksManagerCommon::IsLocalAddr( const CNodeAddress &test ) const
{
	const sockaddr_in &nt = *(sockaddr_in*)&test.addr;
	if ( nt.sin_addr.S_un.S_addr == 0x0100007f )
		return true;
	//for ( int i = 0; i < broadcastAddr.size(); ++i )
	{
		const CNodeAddress &broad = broadcastAddr;//[ i ];
		const sockaddr_in &nb = *(sockaddr_in*)&broad.addr;
		uint32_t dwB = nb.sin_addr.S_un.S_addr;
		uint32_t dwT = nt.sin_addr.S_un.S_addr;
		uint32_t dwMask = 0;
		for ( int k = 3; k >= 0; k-- )
		{
			uint32_t dwTestMask = 0xFF << k*8;
			if ( (dwB & dwTestMask) == dwTestMask )
				dwMask |= dwTestMask;
			else
				break;
		}
		dwMask = ~dwMask;
		const bool bTest = ( dwB & dwMask ) == ( dwT & dwMask );
		if ( bTest )
			return true;
	}

	return false;
}

bool CLinksManagerCommon::GetSelfAddress( CNodeAddressSet *pRes ) const
{
	pRes->Clear();
	sockaddr_in addr;
	int nBufLeng = sizeof(sockaddr_in);
	if ( getsockname( s, (sockaddr*)&addr, &nBufLeng ) != 0 )
		return false;
	pRes->nPort = addr.sin_port;
	char szHostName[10000];
	gethostname( szHostName, 9999 );
	hostent *p = gethostbyname( szHostName );
	if ( !p || p->h_addrtype != AF_INET || p->h_length != 4 )
		return false;
	for ( int k = 0; k < N_MAX_HOST_HOMES; ++k )
	{
		if ( !p->h_addr_list[k] )
			break;
		pRes->ips[k] = *(int*)p->h_addr_list[k];
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
		return true;

#ifdef NET_TEST_APPLICATION
	static std::vector<SPacket> pktQueue[10];
	if ( bEmulateWeakNetwork )
	{
		if ( rand() <= RAND_MAX * fLostRate )
			return true;
		pktQueue.push_back();
		pktQueue.back().addr = dst;
		pktQueue.back().pkt = pkt;
		while ( pktQueue.size() > 3 )
		{
			int nPkt = rand() % pktQueue.size();
			SPacket &p = pktQueue[nPkt];
			int nSize = p.pkt.GetSize();
			int nRv = sendto( s, (const char*)p.pkt.GetBuffer(), nSize, 0, &p.addr.addr, sizeof( p.addr.addr ) );

			pktQueue.erase( pktQueue.begin() + nPkt );
		}
		return true;
	}
#endif

	const int nSize = pkt.GetSize();
	const int nRv = sendto( s, (const char*)pkt.GetBuffer(), nSize, 0, &dst.addr, sizeof( dst.addr ) );

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

	int nAddrSize;
	pPkt->Seek( 2048 );
	nAddrSize = sizeof( pSrc->addr );
	int nRes = recvfrom( s, (char*)pPkt->GetBufferForWrite(), 2048, 0, &pSrc->addr, &nAddrSize );

	const bool bGoodAddress = IsGoodAddress( pSrc->addr );
	if ( !bGoodAddress )
		nRes = 0;

	if ( nRes >= 0 )
	{
		pSrc->addr.sa_family = AF_INET;       // somehow this gets spoiled on win2k
		memset( pSrc->addr.sa_data + 6, 0, 8 );
		pPkt->SetSize( nRes );
	}

	pPkt->Seek( 0 );

	// CRAP{
	if ( nRes >=0 )
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

ILinksManager* CreateClientLinksManager()
{
	CPtr<CLinksManager> pManager = new CLinksManager();
	const bool bSuccess = pManager->Init( 0 );

	return bSuccess ? pManager.Extract() : 0;
}

ILinksManager* CreateServerLinksManager(  const int nPort )
{
	CPtr<CLinksManager> pManager = new CLinksManager();
	const bool bSuccess = pManager->Init( nPort );

	return bSuccess ? pManager.Extract() : 0;
}

}

BASIC_REGISTER_CLASS( NNet::ILinksManager )
BASIC_REGISTER_CLASS( NNet::CLinksManager )

