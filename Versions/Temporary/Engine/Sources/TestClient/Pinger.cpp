#include "stdafx.h"

#include "Pinger.h"
#include "../Server_Client_Common/CommonPackets.h"
#include "../Misc/Time64.h"
#include "../Client/ServerClient.h"
#include "../Server_Client_Common/LoginPackets.h"

const int MAX_ATTEMPTS = 3;

CPinger::CPinger()
{
	REGISTER_PACKET_PROCESSOR( ProcessConnectServerResult )
}

CPinger::CPinger( const int &nPeriod, const string &_szName, const string &_szPassword, const string &_szIP,
									const int &_nPort, const int &_nNetVersion )
	: nConnectPeriod( nPeriod ), szName( _szName ), szPassword( _szPassword ),
	szIP( _szIP ), nPort( _nPort ), nNetVersion( _nNetVersion ), nAttemptsLeft( MAX_ATTEMPTS ), bConnectNow( false )
{
	REGISTER_PACKET_PROCESSOR( ProcessConnectServerResult )
	nLastConnectTime = GetLongTickCount();
}

bool CPinger::Segment()
{
	if ( pServerClient ) 
	{
		pServerClient->Segment();

		CPtr<CNetPacket> pPacket;
		while ( pPacket = pServerClient->GetPacket() )
			ProcessPacket( pPacket );
	}

	if ( GetLongTickCount() > nLastConnectTime + nConnectPeriod  || bConnectNow )
	{
		bConnectNow = false;
		pServerClient = new CServerClient( szIP.c_str(), nNetVersion, nPort, 30 );
		pServerClient->SendPacket( new CLoginPacket( 0, szName, szPassword ) );
		nLastConnectTime = GetLongTickCount();
	}
	return true;
}

bool CPinger::ProcessConnectServerResult( CConnectServerPacket *pPacket )
{
	if ( pPacket->eConnectionState != CConnectServerPacket::ECS_SUCCESS )
	{
		--nAttemptsLeft;
		if ( nAttemptsLeft > 0 )
			bConnectNow = true;
		else
		{
			CFileStream stream( "server_is_down", CFileStream::WIN_CREATE );
			WriteMSG( "Pinger: Server is down!\n" );
			nAttemptsLeft = MAX_ATTEMPTS;
		}
	}
	else
	{
		DeleteFile( "server_is_down" );
		WriteMSG( "Pinger: Server is up!\n" );
		nAttemptsLeft = MAX_ATTEMPTS;
	}
	pServerClient = 0;
	return true;
}


