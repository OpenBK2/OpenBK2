#include "stdafx.h"
#include "NetAddress.h"

namespace NNet
{

// CNodeAddress

bool CNodeAddress::SetInetName( const char *pszHost, int nDefaultPort )
{
	int nIdx, nPort = nDefaultPort;
	memset( &addr, 0, sizeof( addr ) );
	ASSERT( sizeof(addr) >= sizeof( sockaddr_in) );
  sockaddr_in &nameRemote = *(sockaddr_in*)&addr;
	
	nameRemote.sin_family = AF_INET;
	// extract port number from address
	string szAddr = pszHost;
	nIdx = szAddr.find( ':' );
	if ( nIdx != -1 )
	{
		//nPort = atoi( string( szAddr, nIdx + 1 ).c_str() );
		const string szPort( szAddr.c_str() + nIdx + 1 );
		nPort = atoi( szPort.c_str() );
		//string( szAddr, 0, nIdx );
		szAddr.resize( nIdx );
	}
	// determine host
	nameRemote.sin_addr.S_un.S_addr = inet_addr( szAddr.c_str() ); 
	if( nameRemote.sin_addr.S_un.S_addr == INADDR_NONE )  // not resolved?
	{
		hostent *he;
		he = gethostbyname( szAddr.c_str() ); // m.b. it is string comp.domain
		if( he == NULL )
			return false;
		nameRemote.sin_addr.S_un.S_addr = *( unsigned long* )( he->h_addr_list[0] );
	}
	nameRemote.sin_port = htons( nPort );
	return true;
}

string CNodeAddress::GetName( bool bResolve ) const
{
  sockaddr_in &nameRemote = *(sockaddr_in*)&addr;
	
	hostent *he = 0;
	if ( bResolve )
		he = gethostbyaddr( (const char*)&addr, sizeof(addr), AF_INET ); // m.b. it is string comp.domain
	char szBuf[1024];
	if( he == 0 || he->h_name == 0 )
	{
		in_addr &ia = nameRemote.sin_addr;
		sprintf( szBuf, "%i.%i.%i.%i:%i", 
			(int) ia.S_un.S_un_b.s_b1,
			(int) ia.S_un.S_un_b.s_b2,
			(int) ia.S_un.S_un_b.s_b3,
			(int) ia.S_un.S_un_b.s_b4,
			(int) ntohs( nameRemote.sin_port ) );
	}
	else
	{
		sprintf( szBuf, "%s:%i", 
			(const char*) he->h_name,
			(int) ntohs( nameRemote.sin_port ) );
	}
	return szBuf;
}

}

