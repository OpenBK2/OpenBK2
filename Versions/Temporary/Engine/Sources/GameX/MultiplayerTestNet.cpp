#include "StdAfx.h"
#include "MultiplayerTestNet.h"
#include "../Server_Client_Common/LoginPackets.h"
#include "../Server_Client_Common/CommonPackets.h"
#include "../Server_Client_Common/GamePackets.h"
#include "../System/RandomGen.h"
#include "../Misc/StrProc.h"
#include "MultiplayerNetPackets.h"

// CEmptyServerClient

CNetPacket* CEmptyServerClient::GetPacket()
{
	return 0;
}

void CEmptyServerClient::SendPacket( CNetPacket *pPacket )
{
	NI_ASSERT( 0, "Wrong call" );
}

CNetPacket* CEmptyServerClient::GetGamePacket()
{
	return 0;
}

void CEmptyServerClient::SendGamePacket( CNetPacket *pPacket, bool bBroadcast )
{
	NI_ASSERT( 0, "Wrong call" );
}

void CEmptyServerClient::Segment()
{
}

// CMPUITestServerClient

/*CMPUITestServerClient::CMPUITestServerClient() :
	nFreeClientID( 1 ),
	bLobby( false ),
	nFreeGameID( 1 )
{
	REGISTER_PACKET_PROCESSOR( OnRegisterPacket );
	REGISTER_PACKET_PROCESSOR( OnLoginPacket );

	REGISTER_PACKET_PROCESSOR( OnSendRoomChatPacket );

	REGISTER_PACKET_PROCESSOR( OnEnterGameRoomPacket );
	REGISTER_PACKET_PROCESSOR( OnLeaveGameRoomPacket );
	
	REGISTER_PACKET_PROCESSOR( OnGetRoomClientsPacket );
	REGISTER_PACKET_PROCESSOR( OnGetRoomGamesPacket );
	
	REGISTER_PACKET_PROCESSOR( OnCreateGamePacket );
	REGISTER_PACKET_PROCESSOR( OnConnectGamePacket );
	REGISTER_PACKET_PROCESSOR( OnLeaveGamePacket );
	REGISTER_PACKET_PROCESSOR( OnKillGamePacket );

	REGISTER_PACKET_PROCESSOR( OnB2GameRoomPlayerPacket );
}

void CMPUITestServerClient::PushPacket( CNetPacket *pPacket )
{
	serverPackets.push_back( pPacket );
}

void CMPUITestServerClient::PushGamePacket( CNetPacket *pPacket )
{
	gamePackets.push_back( pPacket );
}

CNetPacket* CMPUITestServerClient::GetPacket()
{
	if ( serverPackets.empty() )
		return 0;
		
	CPtr< CNetPacket > pPacket = serverPackets.front();
	serverPackets.pop_front();
	return pPacket.Extract();
}

void CMPUITestServerClient::SendPacket( CNetPacket *_pPacket )
{
	CPtr<CNetPacket> pPacket = _pPacket;
	bool bResult = ProcessPacket( pPacket );
	NI_ASSERT( bResult, "Unhandled packet" );
}

CNetPacket* CMPUITestServerClient::GetGamePacket()
{
	if ( gamePackets.empty() )
		return 0;
		
	CPtr< CNetPacket > pPacket = gamePackets.front();
	gamePackets.pop_front();
	return pPacket.Extract();
}

void CMPUITestServerClient::SendGamePacket( CNetPacket *_pPacket, bool bBroadcast )
{
	CPtr<CNetPacket> pPacket = _pPacket;
	bool bResult = ProcessPacket( pPacket );
	if ( !bResult )
		PushGamePacket( pPacket );
//	NI_ASSERT( bResult, "Unhandled game packet" );
}

bool CMPUITestServerClient::OnRegisterPacket( class CRegisterPacket *pPacket )
{
	for ( list<SPlayer>::iterator it = opponents.begin(); it != opponents.end(); ++it )
	{
		SPlayer &opponent = *it;
		if ( opponent.wszNick == NStr::ToUnicode( pPacket->szNick ) )
		{
			CConnectServerPacket *pConnectPacket = new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, 
				CConnectServerPacket::ERR_ALREADY_REGISTERED );
			PushPacket( pConnectPacket );
			return true;
		}
	}
	opponents.push_back( SPlayer() );
	opponents.back().nID = nFreeClientID++;
	opponents.back().wszNick = NStr::ToUnicode( pPacket->szNick );
	CConnectServerPacket *pConnectPacket = new CConnectServerPacket( CConnectServerPacket::ECS_SUCCESS, 
		CConnectServerPacket::ERR_UNKNOWN );
	PushPacket( pConnectPacket );
	return true;
}

bool CMPUITestServerClient::OnLoginPacket( class CLoginPacket *pPacket )
{
	if ( player.nID != -1 )
	{
		CConnectServerPacket *pConnectPacket = new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, 
			CConnectServerPacket::ERR_ALREADY_ONLINE );
		PushPacket( pConnectPacket );
		return true;
	}
	
	player.nID = nFreeClientID++;
	player.wszNick = NStr::ToUnicode( pPacket->szNick );
	CConnectServerPacket *pConnectPacket = new CConnectServerPacket( CConnectServerPacket::ECS_SUCCESS, 
		CConnectServerPacket::ERR_UNKNOWN );
	PushPacket( pConnectPacket );
	
	CMyIDPacket *pIDPacket = new CMyIDPacket( player.nID );
	PushPacket( pIDPacket );

	return true;
}

bool CMPUITestServerClient::OnSendRoomChatPacket( class CChatPacket *pPacket )
{
	PushPacket( new CChatPacket( 0, pPacket->wszMessage, "Fake Opponent", 0, true ) );
	return true;
}

bool CMPUITestServerClient::OnEnterGameRoomPacket( class CEnterLobbyPacket *pPacket )
{
	bLobby = true;

	//{ DEBUG - добавим противников
	SendPacket( new CRegisterPacket( 0, "ClientB2 01", "Pass", "Key", "email" ) );
	SendPacket( new CRegisterPacket( 0, "ClientB2 02", "Pass", "Key", "email" ) );
	SendPacket( new CRegisterPacket( 0, "ClientB2 03", "Pass", "Key", "email" ) );
	//}
	
	return true;
}

bool CMPUITestServerClient::OnLeaveGameRoomPacket( class CLeaveLobbyPacket *pPacket )
{
	bLobby = false;

	return true;
}

bool CMPUITestServerClient::OnGetRoomClientsPacket( class CGetLobbyClientsPacket *pPacket )
{
	return true;
}

bool CMPUITestServerClient::OnGetRoomGamesPacket( class CGetLobbyGamesPacket *pPacket )
{
	CLobbyGamesPacket *pInfoPacket = new CLobbyGamesPacket( 0 );
//	pPacket->dwVersion = 0;
	pInfoPacket->bFullUpdate = true;
	int nCount = NRandom::Random( 0, 10 );
	for ( int i = 0; i < nCount; ++i )
	{
		SGameInfo info;
		
		info.nID = i;
		info.szName = StrFmt( "MegaBattle %d", i );
		info.nMaxPlayers = 16;
		info.bCanConnect = true;
		info.bHasPassword = false;
		info.szPassword = "";
		
		CB2LobbyGameInfoPacket *pSpecificPacket = new CB2LobbyGameInfoPacket();
		
		pSpecificPacket->nPlayers = 2;
		pSpecificPacket->nPing = 100;
		pSpecificPacket->nMapType = 1;
		pSpecificPacket->nMapSize = 1;
		pSpecificPacket->nObservers = 1;
		switch ( i % 3 )
		{
			case 0:
				pSpecificPacket->nMissionID = 18;
			break;

			case 1:
				pSpecificPacket->nMissionID = 19;
			break;

			case 2:
				pSpecificPacket->nMissionID = 21;
			break;
		};
		GetSystemTime( &(pSpecificPacket->created) );
		
		info.pSpecificInfo = pSpecificPacket;

		pInfoPacket->added.push_back( info );
	}
	
	PushPacket( pInfoPacket );
	
	return true;
}

bool CMPUITestServerClient::OnCreateGamePacket( class CCreateGamePacket *pPacket )
{
	player.nGameID = nFreeGameID;
	player.bCreator = true;
	nFreeGameID++;
	PushPacket( new CConnectedGameID( player.nID, player.nGameID ) );
	
	PushPacket( new CNewGameClient( nFreeClientID++ ) );
	PushPacket( new CNewGameClient( nFreeClientID++ ) );
	PushPacket( new CNewGameClient( nFreeClientID++ ) );
	
	return true;
}

bool CMPUITestServerClient::OnConnectGamePacket( class CConnectGamePacket *pPacket )
{
	player.nGameID = pPacket->nGameID;
	player.bCreator = false;
	PushPacket( new CConnectedGameID( player.nID, player.nGameID ) );

	PushPacket( new CNewGameClient( nFreeClientID++ ) );
	PushPacket( new CNewGameClient( nFreeClientID++ ) );
	PushPacket( new CNewGameClient( nFreeClientID++ ) );

	return true;
}

bool CMPUITestServerClient::OnLeaveGamePacket( class CLeaveGamePacket *pPacket )
{
	player.nGameID = -1;

	return true;
}

bool CMPUITestServerClient::OnKillGamePacket( class CKillGamePacket *pPacket )
{
	player.nGameID = -1;

	return true;
}

bool CMPUITestServerClient::OnB2GameRoomPlayerPacket( class CB2GameRoomPlayerPacket *pPacket )
{
	CB2GameRoomPlayerPacket *pPlayerPacket = new CB2GameRoomPlayerPacket( pPacket->nClientID );
	pPlayerPacket->wszName = NStr::ToUnicode( StrFmt( "%s to %d", NStr::ToMBCS( pPacket->wszName ).c_str(), pPacket->nClientID ) );
	pPlayerPacket->nTeam = pPacket->nTeam;
	pPlayerPacket->nSide = pPacket->nSide;
	pPlayerPacket->nColor = pPacket->nColor;
	PushGamePacket( pPlayerPacket );
	return true;
}*/

REGISTER_SAVELOAD_CLASS( 0x17130C40, CEmptyServerClient );
//REGISTER_SAVELOAD_CLASS( 0x17130B00, CMPUITestServerClient );


