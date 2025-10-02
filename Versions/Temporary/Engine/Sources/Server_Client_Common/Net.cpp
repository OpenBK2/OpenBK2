#include "stdafx.h"

#include "..\net\netaddress.h"
#include "CommonPackets.h"
#include "Net.h"
#include "NetSaver.h"
#include "../Net/NetDriver.h"

#include "ZipPacket.h"

const int MIN_PACKET_SIZE_TO_ZIP = 50;

BASIC_REGISTER_CLASS( CNet );

float CNet::fTimeOut = 60;

CNet::CNet( const int _nApplicationID, const int _nServerPort, const int _nTimeOut )
: nApplicationID( _nApplicationID ), nServerPort( _nServerPort ), bDebugNetPaused( false ), nTimeOut( _nTimeOut )
{

}

bool CNet::InitAsServer()
{
	pNetDriver = NNet::CreateNetDriver( NNet::SNetDriverConsts( nTimeOut ), false );
	CPtr<NNet::ILinksManager> pLinksManager = NNet::CreateServerLinksManager( nServerPort );
	if ( pLinksManager )
	{
		pNetDriver->Init( nApplicationID, nServerPort, false, pLinksManager );
		return true;
	}
	else
		return false;
}

bool CNet::InitAsClient()
{
	pNetDriver = NNet::CreateNetDriver( NNet::SNetDriverConsts( nTimeOut ), false );
	CPtr<NNet::ILinksManager> pLinksManager = NNet::CreateClientLinksManager();
	if ( pLinksManager )
	{
		pNetDriver->Init( nApplicationID, nServerPort, true, pLinksManager );
		return true;
	}
	else
		return false;
}

bool CNet::InitAsClient( NNet::ILinksManager *pLinksManager )
{
	if ( pLinksManager )
	{
		pNetDriver = NNet::CreateNetDriver( NNet::SNetDriverConsts( nTimeOut ), false );
		pNetDriver->Init( nApplicationID, nServerPort, true, pLinksManager );
		return true;
	}
	else
		return false;
}

bool CNet::InitAsServer( NNet::ILinksManager *pLinksManager )
{
	if ( pLinksManager )
	{
		pNetDriver = NNet::CreateNetDriver( NNet::SNetDriverConsts( nTimeOut ), false );
		pNetDriver->Init( nApplicationID, nServerPort, false, pLinksManager );
		return true;
	}
	else
		return false;
}

void CNet::StartGame()
{
	pNetDriver->StartGame();
}

void CNet::StartNewPlayerAccept()
{
	pNetDriver->StartNewPlayerAccept();
}

void CNet::Kick( const int nClient )
{
	pNetDriver->Kick( nClient );
}

void CNet::ConnectGame( const NNet::CNodeAddress addr, const CMemoryStream &pwd )
{
	pNetDriver->ConnectGame( addr, pwd );
}

const int CNet::GetState() const
{
	return pNetDriver->GetState();
}

const int CNet::GetRejectReason() const
{
	return pNetDriver->GetRejectReason();
}

CNetPacket* CNet::ReceivePacket()
{
	if ( !bDebugNetPaused )
	{
		CMemoryStream memStream;
		NNet::IDriver::EMessage eMsgID;
		int nClientID;

		while ( pNetDriver && pNetDriver->GetMessage( &eMsgID, &nClientID, 0, &memStream ) )
		{
			CPtr<CNetPacket> pNetPacket;
			{
				switch ( eMsgID )
				{
				case NNet::IDriver::DIRECT:
				case NNet::IDriver::BROADCAST:
					{
						memStream.Seek( 0 );
						{
							CPtr<IBinSaver> pSaver = CreateNetSaver( &memStream, SAVER_MODE_READ );
							pSaver->Add( 1, &pNetPacket );
						}
						pNetPacket = UnzipPacketIfNecessary( pNetPacket );
						break;
					}
				case NNet::IDriver::NEW_CLIENT:
					pNetPacket = new CNetNewClient();

					break;
				case NNet::IDriver::REMOVE_CLIENT:
					pNetPacket = new CNetRemoveClient();

					break;
				case NNet::IDriver::KICKED:
					pNetPacket = new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_KICKED );

					break;
				case NNet::IDriver::SERVER_DEAD:
					pNetPacket = new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_CONNECTION_LOST );

					break;
				default:
					break;
				}
			}
			if ( pNetPacket )
			{
				pNetPacket->nClientID = nClientID;
        return pNetPacket.Extract();			
			}
		}
	}

	return 0;
}

void CNet::SendZipped( const int nClientID, const CMemoryStream &memStream )
{
	CPtr<CZipPacket> pZipPacket = new CZipPacket( nClientID );
	pZipPacket->Zip( memStream );
	CPtr<CNetPacket> pNetPacket = pZipPacket;
	CMemoryStream memStreamToSend;
	{
		CPtr<IBinSaver> pSaver = CreateNetSaver( &memStreamToSend, SAVER_MODE_WRITE );
		pSaver->Add( 1, &pNetPacket );
	}
	if ( memStreamToSend.GetSize() < memStream.GetSize() )
		pNetDriver->SendDirect( nClientID, memStreamToSend );
	else
		pNetDriver->SendDirect( nClientID, memStream );
}

CNetPacket* CNet::UnzipPacketIfNecessary( CNetPacket *pPacket )
{
	CDynamicCast<CZipPacket> pZipPacket = pPacket;
	if ( pZipPacket )
	{
		CPtr<CNetPacket> pNetPacket = pPacket;
		CMemoryStream unzippedStream;
		pZipPacket->UnZip( &unzippedStream );
		unzippedStream.Seek( 0 );
		{
			CPtr<IBinSaver> pSaver = CreateNetSaver( &unzippedStream, SAVER_MODE_READ );
			pSaver->Add( 1, &pNetPacket );
		}
		return pNetPacket.Extract();
	}
	return pPacket;
}

void CNet::SendPacket( CNetPacket *pPacket )
{
	if ( !bDebugNetPaused )
	{
		CPtr<CNetPacket> pNetPacket = pPacket;

		CMemoryStream memStream;
		{
			memStream.Seek( 0 );
			CPtr<IBinSaver> pSaver = CreateNetSaver( &memStream, SAVER_MODE_WRITE );
			pSaver->Add( 1, &pNetPacket );
		}
		
		if ( memStream.GetSize() > MIN_PACKET_SIZE_TO_ZIP )
			SendZipped( pNetPacket->nClientID, memStream );
		else
			pNetDriver->SendDirect( pNetPacket->nClientID, memStream );
	}
	else
		pausePackets.push_back( pPacket );
}

const string CNet::GetIP( const int nClientID ) const
{
	return pNetDriver->GetIP( nClientID );
}

const int CNet::GetPort( const int nClientID ) const
{
	return pNetDriver->GetPort( nClientID );
}

const float CNet::GetTimeOut()
{
	return fTimeOut;
}

void CNet::SetTimeOut( const float _fTimeOut )
{
	fTimeOut = _fTimeOut;
}

void CNet::DebugTogglePause( bool bPause )
{
	bDebugNetPaused = bPause;

	if ( !bDebugNetPaused )
	{
		for ( list< CPtr<CNetPacket> >::iterator iter = pausePackets.begin(); iter != pausePackets.end(); ++iter )
			SendPacket( *iter );

		pausePackets.clear();
	}
}

