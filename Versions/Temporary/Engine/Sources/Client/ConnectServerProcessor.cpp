#include "stdafx.h"

#include "ConnectServerProcessor.h"

#include "../Net/NetDriver.h"
#include "../Server_Client_Common/CommonPackets.h"
#include "../Server_Client_Common/LoginPackets.h"
#include "../Server_Client_Common/Net.h"
#include "../Server_Client_Common/PrimeNumbers.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CConnectServerProcessor::CConnectServerProcessor( CNet *_pServer, bool _bCheckConnect )
{
	pServer = _pServer;
	bCheckConnect = _bCheckConnect;
	eConnectState = ECS_CONNECTING;

	REGISTER_PACKET_PROCESSOR( &CConnectServerProcessor::ProcessCheckConnect );
	REGISTER_PACKET_PROCESSOR( &CConnectServerProcessor::ProcessLoginPacket );
	REGISTER_PACKET_PROCESSOR( &CConnectServerProcessor::ProcessRegisterPacket );
	REGISTER_PACKET_PROCESSOR( &CConnectServerProcessor::ProcessNewClient );
	REGISTER_PACKET_PROCESSOR( &CConnectServerProcessor::ProcessForgottenPasswordPacket );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CConnectServerProcessor::Segment()
{
	switch( eConnectState )
	{
		case ECS_INACTIVE:
			break;
		case ECS_CONNECTING:
			if ( pServer->GetState() == NNet::IDriver::ACTIVE )
			{
				eConnectState = ECS_ACTIVE;
				return true;
			}
			else if ( pServer->GetState() == NNet::IDriver::INACTIVE )
			{
				CConnectServerPacket::ERejectReason eRejectReason = CConnectServerPacket::ERR_UNKNOWN;
				switch ( pServer->GetRejectReason() )
				{
					case NNet::IDriver::EReject::TIMEOUT:
						eRejectReason = CConnectServerPacket::ERR_LOGIN_TIMEOUT;
						break;
					case NNet::IDriver::EReject::WRONG_VERSION:
						eRejectReason = CConnectServerPacket::ERR_WRONG_NET_VERSION;
						break;
				}

				PushPacket( new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, eRejectReason ) );
				eConnectState = ECS_INACTIVE;
				return false;
			}

			break;
		case ECS_ACTIVE:
			if ( pServer->GetState() == NNet::IDriver::INACTIVE )
			{
				PushPacket( new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_UNKNOWN ) );
				eConnectState = ECS_INACTIVE;
				return false;
			}

			break;
		default:
			NI_ASSERT( false, StrFmt( "Uknown connection state %d", (int)eConnectState ) );
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CConnectServerProcessor::ProcessCheckConnect( CCheckConnectPacket *pCheckConnectPacket )
{
	if ( bCheckConnect )
	{
		CPtr<CPrimeNumbers> pPrime = new CPrimeNumbers();

		int nCnt = 0;
		while ( nCnt < pPrime->GetNNumbers() && pPrime->GetPrime( nCnt ) <=	sqrt( double(pCheckConnectPacket->nNumber )) )
		{
			const __int64 nPrime1 = pPrime->GetPrime( nCnt );
			const __int64 nModule = pCheckConnectPacket->nNumber % nPrime1;
			if ( nModule == 0 )
			{
				const __int64 nPrime2 = pCheckConnectPacket->nNumber / nPrime1;
				NI_ASSERT( pPrime->IsPrime( nPrime2 ), "Wrong prime" );

				CNetPacket *pPacket;
				if ( eConnectType != ECT_FORGOT_PASSWORD )								
				{	
					pPacket = new CCheckConnectAnswerPacket(
						nPrime1, nPrime2,
						eConnectType == ECT_LOGIN ? CCheckConnectAnswerPacket::ECT_LOGIN : CCheckConnectAnswerPacket::ECT_REGISTER,
						szNick, szPassword, szCDKey, szEMail );
				}
				else
				{
					pPacket = new CForgottenPasswordPacket( szNick, szEMail );
				}
				pPacket->nClientID = 0;
				pServer->SendPacket( pPacket );
				return true;
			}

			++nCnt;
		}

		NI_ASSERT( false, StrFmt( "Can't factorize number %d", pCheckConnectPacket->nNumber ) );
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CConnectServerProcessor::ProcessLoginPacket( CLoginPacket *pLoginPacket )
{
	eConnectType = ECT_LOGIN;
	szNick = pLoginPacket->szNick;
	szPassword = pLoginPacket->szPassword;

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CConnectServerProcessor::ProcessRegisterPacket( CRegisterPacket *pRegisterPacket )
{
	eConnectType = ECT_REGISTER;
	szNick = pRegisterPacket->szNick;
	szPassword = pRegisterPacket->szPassword;
	szCDKey = pRegisterPacket->szCDKey;
	szEMail = pRegisterPacket->szEmail;

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CConnectServerProcessor::ProcessForgottenPasswordPacket( CForgottenPasswordPacket *pPacket )
{
	eConnectType = ECT_FORGOT_PASSWORD;
	szNick = pPacket->szNick;
	szEMail = pPacket->szEMail;

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CConnectServerProcessor::ProcessNewClient( CNetNewClient *pPacket )
{
	if ( !bCheckConnect )				
		PushPacket( new CConnectServerPacket( CConnectServerPacket::ECS_SUCCESS, CConnectServerPacket::ERR_UNKNOWN ) );
	
	return pPacket->nClientID == 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
