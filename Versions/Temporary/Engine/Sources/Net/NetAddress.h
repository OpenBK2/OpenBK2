#pragma once

#include "Net_export.h"


#include <winsock.h>

namespace NNet
{

class NET_EXPORT CNodeAddress
{
	sockaddr addr;
public:
	CNodeAddress() { memset( &addr, 0, sizeof(addr) ); }

	void Clear() { memset( &addr, 0, sizeof(addr) ); }
	//
	bool SetInetName( const char *pszHost, int nDefaultPort );
	string GetName( bool bResolve = true ) const;
	string GetFastName() const { return GetName( false ); }
	//
	string GetIP();
	int GetPort();
	//
	bool SameIP( const CNodeAddress &a ) const { return memcmp( ((char*)&a.addr) + 4, ((char*)&addr) + 4, 4 ) == 0; }
	unsigned int GetIP() const { return ((unsigned int*)(&addr))[1]; }

	bool operator == ( const CNodeAddress &a ) const { return memcmp( &addr, &a.addr, sizeof(addr) ) == 0; }
	bool operator != ( const CNodeAddress &a ) const { return memcmp( &addr, &a.addr, sizeof(addr) ) != 0; }

	sockaddr *GetSockAddr() { return &addr; }

	friend class CLinksManagerCommon;
};

}


