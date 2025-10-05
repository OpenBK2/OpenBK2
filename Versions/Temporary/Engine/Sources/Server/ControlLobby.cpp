#include "stdafx.h"

#include "clients.h"
#include "ControlLobby.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/LoginPackets.h"
#include "Server_Client_Common/Net.h"
#include "Server_Client_Common/PrimeNumbers.h"
#include "System/XmlSaver.h"

#include "Statistics.h"

CControlLobby::CControlLobby( CClients *_pClients, CNet *_pNet, const string &_szCfgFile )
{
	REGISTER_PACKET_PROCESSOR( ProcessNewClient );
	REGISTER_PACKET_PROCESSOR( ProcessCheckConnectAnswer );
	REGISTER_PACKET_PROCESSOR( ProcessRemoveClient );
	REGISTER_PACKET_PROCESSOR( ProcessCommonClientStatePacket );
	REGISTER_PACKET_PROCESSOR( ProcessNewConnectingClient );
	REGISTER_PACKET_PROCESSOR( ProcessDirectClientPacket );
	REGISTER_PACKET_PROCESSOR( ProcessThroughServerGamePacket );
	REGISTER_PACKET_PROCESSOR( ProcessPingPacket );
	REGISTER_PACKET_PROCESSOR( ProcessForgottenPasswordPacket );
	pClients = _pClients;
	pNet = _pNet;
	pPrime = new CPrimeNumbers();
	szCfgFile = _szCfgFile;
	
	ReloadConfig();
	NStatistics::SetGlobalCounter( "TotalLogins", NStatistics::CreateEventsCounter() );
}

void CControlLobby::ReloadConfig()
{
	CFileStream stream( szCfgFile, CFileStream::WIN_READ_ONLY );
	NI_ASSERT( stream.IsOk(), StrFmt( "Could not open cfg file: %s", szCfgFile ) );
	CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
	NI_ASSERT( pSaver.GetPtr(), "Could not create XML saver" );
	pSaver->Add( "CheckCDKeyIsValid", &bCheckCDKeyIsValid );
}

bool CControlLobby::ProcessNewClient( CNetNewClient *pPacket )
{
	connecting.insert( pPacket->nClientID );

	const int nPrimes = pPrime->GetNNumbers();
	const __int64 nPrimeNumber1 = pPrime->GetPrime( rand() % nPrimes );
	const __int64 nPrimeNumber2 = pPrime->GetPrime( rand() % nPrimes );

	const __int64 nAskedNumber = nPrimeNumber1 * nPrimeNumber2;
	askedNumbers[pPacket->nClientID] = nAskedNumber;

	CCheckConnectPacket *pCheckPacket = new CCheckConnectPacket( nAskedNumber );
	pCheckPacket->nClientID = pPacket->nClientID;

	PushPacket( pCheckPacket );

	return true;
}

bool CControlLobby::ProcessCheckConnectAnswer( CCheckConnectAnswerPacket *pPacket )
{
	if ( connecting.find( pPacket->nClientID ) == connecting.end() )
	{
		pNet->Kick( pPacket->nClientID );
		connecting.erase( pPacket->nClientID );
		askedNumbers.erase( pPacket->nClientID );
		return true;
	}

	if ( !pPrime->IsPrime( pPacket->nPrime1 ) || !pPrime->IsPrime( pPacket->nPrime2 ) ||
				(__int64)pPacket->nPrime1 * (__int64)pPacket->nPrime2 != askedNumbers[pPacket->nClientID] )
	{
		pNet->Kick( pPacket->nClientID );
		connecting.erase( pPacket->nClientID );
		askedNumbers.erase( pPacket->nClientID );
		return true;
	}

	if ( pClients->IsCriticalBusy() )
	{
		pClients->RecalcDBOverload();
		CConnectServerPacket *pAnswerPacket = 
			new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_CRITICAL_BUSY );
		pAnswerPacket->nClientID = pPacket->nClientID;
		PushPacket( pAnswerPacket );
		connecting.erase( pPacket->nClientID );
		askedNumbers.erase( pPacket->nClientID );
		return true;
	}

	// rude nick?
	if ( pClients->IsBadNick( pPacket->szNick ) )
	{
		CConnectServerPacket *pAnswerPacket = 
			new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_BAD_NICK );
		pAnswerPacket->nClientID = pPacket->nClientID;

		PushPacket( pAnswerPacket );
		connecting.erase( pPacket->nClientID );
		askedNumbers.erase( pPacket->nClientID );
		return true;
	}

	// nick isn't registered, trying to login?
	if ( pPacket->eConnectType == CCheckConnectAnswerPacket::ECT_LOGIN && !pClients->IsNickRegistered( pPacket->szNick ) )
	{
		CConnectServerPacket *pAnswerPacket = 
			new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_NOT_REGISTERED );
		pAnswerPacket->nClientID = pPacket->nClientID;

		PushPacket( pAnswerPacket );
		connecting.erase( pPacket->nClientID );
		askedNumbers.erase( pPacket->nClientID );
		return true;
	}

	const string szCDKey = 
		pPacket->eConnectType == CCheckConnectAnswerPacket::ECT_REGISTER ? pPacket->szCDKey : pClients->GetCDKey( pPacket->szNick );

	// already online?
	if ( pClients->IsOnLine( pPacket->szNick ) || pClients->IsCDKeyOnline( szCDKey ) )
	{
		CConnectServerPacket *pAnswerPacket = 
			new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_ALREADY_ONLINE );
		pAnswerPacket->nClientID = pPacket->nClientID;

		PushPacket( pAnswerPacket );
		connecting.erase( pPacket->nClientID );
		askedNumbers.erase( pPacket->nClientID );
		return true;
	}

	// not correct cdkey?
	if ( pPacket->eConnectType == CCheckConnectAnswerPacket::ECT_REGISTER && !pClients->IsCorrectCDKey( pPacket->szCDKey ) && bCheckCDKeyIsValid )
	{
		CConnectServerPacket *pAnswerPacket = 
			new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_WRONG_CD_KEY );
		pAnswerPacket->nClientID = pPacket->nClientID;

		PushPacket( pAnswerPacket );
		connecting.erase( pPacket->nClientID );
		askedNumbers.erase( pPacket->nClientID );
		return true;
	}

	// banned nick?
	if ( pClients->IsBannedNick( pPacket->szNick ) )
	{
		CConnectServerPacket *pAnswerPacket = 
			new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_BANNED_NICK );
		pAnswerPacket->nClientID = pPacket->nClientID;

		PushPacket( pAnswerPacket );
		connecting.erase( pPacket->nClientID );
		askedNumbers.erase( pPacket->nClientID );
		return true;
	}

	// nick is already registered, trying to register?
	if ( pPacket->eConnectType == CCheckConnectAnswerPacket::ECT_REGISTER && pClients->IsNickRegistered( pPacket->szNick ) )
	{
		CConnectServerPacket *pAnswerPacket = 
			new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_ALREADY_REGISTERED );
		pAnswerPacket->nClientID = pPacket->nClientID;

		PushPacket( pAnswerPacket );
		connecting.erase( pPacket->nClientID );
		askedNumbers.erase( pPacket->nClientID );
		return true;
	}

	// banned cdkey?
	if ( szCDKey != "" && pClients->IsBannedCDKey( szCDKey ) )
	{
		CConnectServerPacket *pAnswerPacket = 
			new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_BANNED_CDKEY );
		pAnswerPacket->nClientID = pPacket->nClientID;

		PushPacket( pAnswerPacket );
		connecting.erase( pPacket->nClientID );
		askedNumbers.erase( pPacket->nClientID );
		return true;
	}

	// wrong password?
	if ( pPacket->eConnectType == CCheckConnectAnswerPacket::ECT_LOGIN )
	{
		const string szStr = pClients->GetPassword( pPacket->szNick );
		if ( szStr != pPacket->szPassword )
		{
			CConnectServerPacket *pAnswerPacket = 
				new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_WRONG_PASSWORD );
			pAnswerPacket->nClientID = pPacket->nClientID;

			PushPacket( pAnswerPacket );
			connecting.erase( pPacket->nClientID );
			askedNumbers.erase( pPacket->nClientID );
			return true;
		}
	}

	if ( pPacket->eConnectType == CCheckConnectAnswerPacket::ECT_REGISTER )
		pClients->Register( pPacket->szNick, pPacket->szPassword, pPacket->szCDKey, pPacket->szEmail );

	pClients->SetOnLine( pPacket->szNick, pPacket->nClientID );

  CConnectServerPacket *pAnswerPacket = 
		new CConnectServerPacket( CConnectServerPacket::ECS_SUCCESS, CConnectServerPacket::ERR_UNKNOWN );
	pAnswerPacket->nClientID = pPacket->nClientID;
	PushPacket( pAnswerPacket );

	CMyIDPacket *pClientIDPacket = new CMyIDPacket( pPacket->nClientID );
	PushPacket( pClientIDPacket );

	connecting.erase( pPacket->nClientID );
	askedNumbers.erase( pPacket->nClientID );

#ifndef CONSOLE_LOG_SILENCE
	WriteMSG( "new client %s\n", pPacket->szNick );
#endif	
	NStatistics::GetGlobal( "TotalLogins" )->Add( 1.0f );

	return true;
}

bool CControlLobby::ProcessRemoveClient( CNetRemoveClient *pNetRemoveClient )
{
	string szNick;
	if ( pClients->GetNick( pNetRemoveClient->nClientID, &szNick ) )
	{
#ifndef CONSOLE_LOG_SILENCE	
		WriteMSG( "client %s removed\n", szNick );
#endif
	}
	
	pClients->SetOffLine( pNetRemoveClient->nClientID );
	connecting.erase( pNetRemoveClient->nClientID );
	askedNumbers.erase( pNetRemoveClient->nClientID );

	return false;
}

bool CControlLobby::ProcessCommonClientStatePacket( CCommonClientStatePacket *pPacket )
{
	SCommonClientInfo clientInfo;
	if ( pClients->GetCommonClientInfo( pPacket->nClientID, &clientInfo ) )
	{
		clientInfo.eState = pPacket->eState;
		pClients->SetCommonClientInfo( pPacket->nClientID, clientInfo );
	}

	return false;
}

bool CControlLobby::ProcessNewConnectingClient( CNewGameConnectingClient *pPacket )
{
	const int nClientID = pPacket->nClientID;
	pClients->SetGameConnectInfo( pPacket->nServerID, pPacket->nConnection, pNet->GetIP( nClientID ), pNet->GetPort( nClientID ) );

  CPtr<CGameConnectingClientAccepted> pAnswerPacket = new CGameConnectingClientAccepted( pPacket->nServerID, pPacket->nConnection );
	PushPacket( pAnswerPacket );

	connecting.erase( pPacket->nClientID );

	return true;
}

bool CControlLobby::ProcessDirectClientPacket( CDirectPacketToClient *pPacket )
{
	swap( pPacket->nClient, pPacket->nClientID );
	PushPacket( pPacket );

	return true;
}

bool CControlLobby::ProcessThroughServerGamePacket( CThroughServerGamePacket *pPacket )
{
	swap( pPacket->nClient, pPacket->nClientID );
	PushPacket( pPacket );

	return true;
}

bool CControlLobby::ProcessPingPacket( CPingPacket *pPacket )
{
	PushPacket( pPacket );

	return true;
}

bool CControlLobby::ProcessForgottenPasswordPacket( CForgottenPasswordPacket *pPacket )
{
	if ( ! pClients->IsNickRegistered( pPacket->szNick ) )
	{
		CPtr<CForgottenPasswordAnswerPacket> pAnswerPacket = new CForgottenPasswordAnswerPacket( pPacket->nClientID, 
			CForgottenPasswordAnswerPacket::NICK_NOT_REGISTERED );
		PushPacket( pAnswerPacket );
		return true;
	}
	string szEmail = pClients->GetEmail( pPacket->szNick );

	if ( pPacket->szEMail != szEmail )
	{
		CPtr<CForgottenPasswordAnswerPacket> pAnswerPacket = new CForgottenPasswordAnswerPacket( pPacket->nClientID, 
			CForgottenPasswordAnswerPacket::INVALID_EMAIL );
		PushPacket( pAnswerPacket );
		return true;
	}

	
	// TODO: implement the procedure

	CPtr<CForgottenPasswordAnswerPacket> pAnswerPacket = new CForgottenPasswordAnswerPacket( pPacket->nClientID, 
		CForgottenPasswordAnswerPacket::SORRY_SERVICE_IS_NOT_IMPLEMENTED_YET );
  PushPacket( pAnswerPacket );
	return true;
}
