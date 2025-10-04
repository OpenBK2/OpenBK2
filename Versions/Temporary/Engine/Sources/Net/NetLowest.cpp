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
	p->sin_addr.S_un.S_addr = ips[n];
	return true;
}

string CNodeAddress::GetIP()
{
	sockaddr_in &nameRemote = *(sockaddr_in*)&addr;
	char szBuf[1024];
	in_addr &ia = nameRemote.sin_addr;
	sprintf( szBuf, "%i.%i.%i.%i", 
		(int) ia.S_un.S_un_b.s_b1,
		(int) ia.S_un.S_un_b.s_b2,
		(int) ia.S_un.S_un_b.s_b3,
		(int) ia.S_un.S_un_b.s_b4 );
	return szBuf;
}

int CNodeAddress::GetPort()
{
	sockaddr_in &nameRemote = *(sockaddr_in*)&addr;
	return (int) ntohs( nameRemote.sin_port );
}

}

