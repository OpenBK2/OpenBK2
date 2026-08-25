#include "stdafx.h"
#include "NetLowest.h"


namespace NNet
{

bool CNodeAddressSet::GetAddress( int n, CNodeAddress *pRes ) const
{
	pRes->Clear();
	if ( n < 0 || n >= N_MAX_HOST_HOMES || ips[n] == 0 )
		return false;
	sockaddr_in *p = (sockaddr_in*)pRes->GetSockAddr();
	p->sin_family = AF_INET;
	p->sin_port = nPort;
	p->sin_addr.s_addr = ips[n];
	return true;
}

std::string CNodeAddress::GetIP()
{
	sockaddr_in &nameRemote = *(sockaddr_in*)&addr;
	char szBuf[1024];
	const unsigned char *pOctets = AddressOctets( &nameRemote.sin_addr );
	sprintf( szBuf, "%i.%i.%i.%i", 
		(int) pOctets[0],
		(int) pOctets[1],
		(int) pOctets[2],
		(int) pOctets[3] );
	return szBuf;
}

int CNodeAddress::GetPort()
{
	sockaddr_in &nameRemote = *(sockaddr_in*)&addr;
	return (int) ntohs( nameRemote.sin_port );
}

}

