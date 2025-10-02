#include "stdafx.h"

#include "ConnectionInternal.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/Net.h"
#include "Server_Client_Common/PacketProcessor.h"

//*******************************************************************
//*                    CFinishConnectionFilter                       *
//*******************************************************************

class CFinishConnectionFilter : public CPacketProcessor
{
	OBJECT_NOCOPY_METHODS( CFinishConnectionFilter );

	bool bConnectionAlive;
public:
	CFinishConnectionFilter();
	virtual bool Segment();

	//
	bool ProcessRemoveClient( class CNetRemoveClient *pPacket );
	bool ProcessConnectServerPacket( class CConnectServerPacket *pPacket );
};

CFinishConnectionFilter::CFinishConnectionFilter()
: bConnectionAlive( true )
{
	REGISTER_PACKET_PROCESSOR( &CFinishConnectionFilter::ProcessRemoveClient );
	REGISTER_PACKET_PROCESSOR( &CFinishConnectionFilter::ProcessConnectServerPacket );
}

bool CFinishConnectionFilter::ProcessRemoveClient( CNetRemoveClient *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	if ( bConnectionAlive )
	{
		PushPacket( new CGameClientRemoved( pPacket->nClientID ) );
		bConnectionAlive = false;
	}

	return true;
}

bool CFinishConnectionFilter::ProcessConnectServerPacket( CConnectServerPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	if ( pPacket->eConnectionState == CConnectServerPacket::ECS_FAILED && bConnectionAlive )
	{
		PushPacket( new CGameClientRemoved( pPacket->nClientID ) );
		bConnectionAlive = false;
	}

	return true;
}

bool CFinishConnectionFilter::Segment()
{
	return bConnectionAlive;
}

//*******************************************************************
//*                 CTestConnectionPacket                           *
//*******************************************************************

class CTestConnectionPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CTestConnectionPacket );
public:
	ZDATA
		int nClientServerID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nClientServerID); return 0; }

	CTestConnectionPacket() { }
	CTestConnectionPacket( const int nClientID, const int _nClientServerID )
		: CNetPacket( nClientID ), nClientServerID( _nClientServerID ) { }
};

REGISTER_SAVELOAD_CLASS( 92, CTestConnectionPacket );

//*******************************************************************
//*                     CSimpleConnection                           *
//*******************************************************************

CSimpleConnection::CSimpleConnection( CNet *_pNet, bool _bAsClient, const int _nClientServerID, const int _nClientLocalID, const int _nMyServerID )
: pNet( _pNet ), bAsClient( _bAsClient ), nClientServerID( _nClientServerID ), nClientLocalID( _nClientLocalID ),
	nMyServerID( _nMyServerID ), bConnectionTested( false )
{
	pFinishConnection = new CFinishConnectionFilter();
	pNet->SendPacket( new CTestConnectionPacket( nClientLocalID, nMyServerID ) );
}

void CSimpleConnection::SendGamePacket( CNetPacket *pPacket )
{
	pPacket->nClientID = nClientLocalID;
	pNet->SendPacket( pPacket );
}

int CSimpleConnection::CheckTestConnection( CNetPacket *pPacket )
{
	if ( CTestConnectionPacket *pTestPacket = dynamic_cast<CTestConnectionPacket*>( pPacket ) )
	{
		CPtr<CNetPacket> pPacketToDelete = pTestPacket;
		if ( pTestPacket->nClientServerID == nClientServerID )
		{
			if ( !bConnectionTested )
			{
				bConnectionTested = true;
				pNet->SendPacket( new CTestConnectionPacket( nClientLocalID, nMyServerID ) );
				packets.push_back( new CNewGameClient( nClientServerID ) );
			}

			return 1;
		}

		return -1;
	}

	return 0;
}

bool CSimpleConnection::ProcessPacketFromPlayersNet( CNetPacket *pPacket )
{
	if ( bAsClient )
		return false;

	const int nCheckTestConnectionResult = CheckTestConnection( pPacket );
	if ( nCheckTestConnectionResult != 0 )
		return nCheckTestConnectionResult == 1;
	
	if ( pPacket->nClientID == nClientLocalID )
	{
		pPacket->nClientID = nClientServerID;

		if ( !pFinishConnection->ProcessPacket( pPacket ) )
			packets.push_back( pPacket );
		else
		{
			while ( CNetPacket *pFinishConnectionPacket = pFinishConnection->GetPacket() )
				packets.push_back( pFinishConnectionPacket );
		}

		return true;
	}
	else
		return false;
}

CNetPacket* CSimpleConnection::GetPacket()
{
	if ( !packets.empty() )
	{
		CPtr<CNetPacket> pPacket = packets.front();
		packets.pop_front();
		return pPacket.Extract();
	}

	if ( !bAsClient )
		return 0;

	if ( CNetPacket* pPacket = pFinishConnection->GetPacket() )
		return pPacket;

	CNetPacket *pPacket = pNet->ReceivePacket();
	if ( !pPacket )
		return 0;

	const int nCheckTestConnectionResult = CheckTestConnection( pPacket );
	if ( nCheckTestConnectionResult != 0 )
		return 0;

	pPacket->nClientID = nClientServerID;
	if ( !pFinishConnection->ProcessPacket( pPacket ) )
		return pPacket;
	else
	{
		CNetPacket *pFinishConnectionPacket = pFinishConnection->GetPacket();
		return pFinishConnectionPacket;
	}
}

bool CSimpleConnection::IsAlive() const
{
	return pFinishConnection->Segment();
}

void CSimpleConnection::TogglePauseNet( const bool bPause ) const
{
	if ( bAsClient )
		pNet->DebugTogglePause( bPause );
}

//*******************************************************************
//*                    CThroughServerConnection                     *
//*******************************************************************

CThroughServerConnection::CThroughServerConnection( const int _nClientServerID, CNet *_pNet )
: nClientServerID( _nClientServerID ), pNet( _pNet ), bConnectionTested( false )
{
	pNet->SendPacket( new CThroughServerGamePacket( 0, nClientServerID, new CTestConnectionPacket( nClientServerID, nClientServerID ) ) );
}

void CThroughServerConnection::SendGamePacket( CNetPacket *pPacket )
{
	pNet->SendPacket( new CThroughServerGamePacket( 0, nClientServerID, pPacket ) );
}

bool CThroughServerConnection::ProcessPacketFromServer( CNetPacket *pRawPacket )
{
	CPtr<CThroughServerGamePacket> pPacket = dynamic_cast<CThroughServerGamePacket*>( pRawPacket );
	if ( pPacket && pPacket->nClient == nClientServerID )
	{
		if ( CTestConnectionPacket* pTestPacket = dynamic_cast_ptr<CTestConnectionPacket*>( pPacket->pGamePacket ) )
		{
			if ( !bConnectionTested )
			{
				bConnectionTested = true;
				pNet->SendPacket( new CThroughServerGamePacket( 0, nClientServerID, new CTestConnectionPacket( nClientServerID, nClientServerID ) ) );

				packets.push_back( new CNewGameClient( nClientServerID ) );
			}
		}
		else
		{
			pPacket->pGamePacket->nClientID = pPacket->nClient;
			packets.push_back( pPacket->pGamePacket );
		}

		return true;
	}

	return false;
}

CNetPacket* CThroughServerConnection::GetPacket()
{
	if ( !packets.empty() )
	{
		CPtr<CNetPacket> pPacket = packets.front();
		packets.pop_front();
		return pPacket.Extract();
	}
	else
		return 0;
}

void CThroughServerConnection::TogglePauseNet( const bool bPause ) const
{
	pNet->DebugTogglePause( bPause );
}


