#include "stdafx.h"

#include "ConnectionEffort.h"
#include "ConnectionEffortInternal.h"
#include "Net/NetDriver.h"

#include "Client_export.h"

BASIC_REGISTER_CLASS( CLIENT, CConnectionEffort );

void CConnectionEffort::SendBreakThroughPacket( NNet::ILinksManager *pLinks, const std::string &szIP, const int nPort )
{
	// Open the NAT mapping for the retained peer socket, not a throwaway port.
	if ( pLinks )
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

static std::string szServerIP = "";
static int nServerPort = 0;
static int nGameNetVersion = 0;

void SetServerInfo( const std::string &_szServerIP, const int _nServerPort, const int _nGameNetVersion )
{
	szServerIP = _szServerIP;
	nServerPort = _nServerPort;
	nGameNetVersion = _nGameNetVersion;
}

CConnectionEffort *
CreateFirstClientEffort( const int nMyServerID, const int nClientServerID, class CNet *pNet, const int nTimeOut )
{
	return new CSimpleClientEffort(
			nMyServerID, nClientServerID, pNet, szServerIP, nServerPort, nGameNetVersion, nTimeOut );
}

CConnectionEffort *CreateFirstServerEffort( const int nMyServerID,
                                            const int nClientServerID,
                                            const std::string &szClientIP,
                                            const int nClientPort,
                                            CNet *pNet,
                                            CNet *pAcceptGamersNet,
                                            NNet::ILinksManager *pLinksManager )
{
	return new CSimpleServerEffort(
			nMyServerID, nClientServerID, szClientIP, nClientPort, pNet, pAcceptGamersNet, pLinksManager );
}

} // namespace NEffortsFactory
