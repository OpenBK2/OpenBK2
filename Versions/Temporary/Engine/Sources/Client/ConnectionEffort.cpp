#include "stdafx.h"

#include "ConnectionEffort.h"
#include "ConnectionEffortInternal.h"
#include "../Net/NetDriver.h"

BASIC_REGISTER_CLASS( CConnectionEffort );

void CConnectionEffort::SendBreakThroughPacket( const string &szIP, const int nPort )
{
	if ( CPtr<NNet::ILinksManager> pLinks = NNet::CreateClientLinksManager() )
	{
		CMemoryStream pkt;
		NNet::GetNOPStream( &pkt );

		NNet::CNodeAddress address;
		address.SetInetName( szIP.c_str(), nPort );

		pLinks->Send( address, pkt );
	}
}

namespace NEffortsFactory
{

static string szServerIP = "";
static int nServerPort = 0;
static int nGameNetVersion = 0;

void SetServerInfo( const string &_szServerIP, const int _nServerPort, const int _nGameNetVersion )
{
	szServerIP = _szServerIP;
	nServerPort = _nServerPort;
	nGameNetVersion = _nGameNetVersion;
}

CConnectionEffort* CreateFirstClientEffort( const int nMyServerID, const int nClientServerID, class CNet *pNet, const int nTimeOut )
{
	return new CSimpleClientEffort( nMyServerID, nClientServerID, pNet, szServerIP, nServerPort, nGameNetVersion, nTimeOut );
}

CConnectionEffort* CreateFirstServerEffort( const int nMyServerID, const int nClientServerID, 
	const string &szClientIP, const int nClientPort, CNet *pNet, CNet *pAcceptGamersNet )
{
	return new CSimpleServerEffort( nMyServerID, nClientServerID, szClientIP, nClientPort, pNet, pAcceptGamersNet );
}

}


