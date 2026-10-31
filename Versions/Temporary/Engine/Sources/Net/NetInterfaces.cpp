// Built without the precompiled stdafx.h (see CMakeLists.txt): on Windows that
// prelude includes windows.h, which brings in winsock.h, and IP Helper's
// headers need winsock2.h, which refuses to follow it.
#include "NetInterfaces.h"

#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
// After winsock2.h, which IP Helper's headers depend on; the line keeps the
// formatter from sorting them into the wrong order.
#include <iphlpapi.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace NNet
{

//! Whether an address in network byte order is in 127.0.0.0/8.
static bool IsLoopback( uint32_t dwAddress )
{
	return reinterpret_cast<const unsigned char *>( &dwAddress )[0] == 127;
}

#if BOOST_OS_WINDOWS

void GetInterfaceAddresses( std::vector<SInterfaceAddress> *pResult )
{
	pResult->clear();
	// GetIpAddrTable says how large a table it needs when the buffer is too
	// small. An address can appear between the two calls, so ask a few times.
	std::vector<unsigned char> buffer( sizeof( MIB_IPADDRTABLE ) );
	DWORD dwResult = ERROR_INSUFFICIENT_BUFFER;
	for ( int nTry = 0; nTry < 4 && dwResult == ERROR_INSUFFICIENT_BUFFER; ++nTry )
	{
		ULONG nSize = static_cast<ULONG>( buffer.size() );
		dwResult = GetIpAddrTable( reinterpret_cast<MIB_IPADDRTABLE *>( buffer.data() ), &nSize, FALSE );
		if ( dwResult == ERROR_INSUFFICIENT_BUFFER )
		{
			buffer.resize( nSize );
		}
	}
	if ( dwResult != NO_ERROR )
	{
		return;
	}
	const MIB_IPADDRTABLE *pTable = reinterpret_cast<const MIB_IPADDRTABLE *>( buffer.data() );
	for ( DWORD i = 0; i < pTable->dwNumEntries; ++i )
	{
		const MIB_IPADDRROW &row = pTable->table[i];
		// An adapter without a link, or an address being removed, cannot carry anything
		if ( ( row.wType & ( MIB_IPADDR_DISCONNECTED | MIB_IPADDR_DELETED ) ) != 0 || row.dwAddr == 0 ||
		     IsLoopback( row.dwAddr ) )
		{
			continue;
		}
		// IP Helper lists no point-to-point flag; Windows broadcasts on every adapter it lists
		pResult->push_back( SInterfaceAddress{ row.dwAddr, row.dwMask, true } );
	}
}

#else

void GetInterfaceAddresses( std::vector<SInterfaceAddress> *pResult )
{
	pResult->clear();
	ifaddrs *pList = 0;
	if ( getifaddrs( &pList ) != 0 )
	{
		return;
	}
	for ( const ifaddrs *p = pList; p != 0; p = p->ifa_next )
	{
		if ( p->ifa_addr == 0 || p->ifa_addr->sa_family != AF_INET || p->ifa_netmask == 0 )
		{
			continue;
		}
		// IFF_UP alone is not enough: an empty Docker or LXC bridge is up but has
		// no carrier, which IFF_RUNNING tells.
		if ( ( p->ifa_flags & IFF_UP ) == 0 || ( p->ifa_flags & IFF_RUNNING ) == 0 || ( p->ifa_flags & IFF_LOOPBACK ) != 0 )
		{
			continue;
		}
		const uint32_t dwAddress = reinterpret_cast<const sockaddr_in *>( p->ifa_addr )->sin_addr.s_addr;
		if ( IsLoopback( dwAddress ) )
		{
			continue;
		}
		const uint32_t dwMask = reinterpret_cast<const sockaddr_in *>( p->ifa_netmask )->sin_addr.s_addr;
		pResult->push_back( SInterfaceAddress{ dwAddress, dwMask, ( p->ifa_flags & IFF_BROADCAST ) != 0 } );
	}
	freeifaddrs( pList );
}

#endif

} // namespace NNet
