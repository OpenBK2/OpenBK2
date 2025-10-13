#include "stdafx.h"

#include "Net/netaddress.h"
#include "MultiplayerInternal.h"
#include "NetMessages.h"
#include "DBNetConsts.h"

#include "Net/NetDriver.h"
#include "AICmdsAutoMagicInterface.h"

#include <float.h>

#include <cstdint>

const int nNetTimeout = 60;				// As in SNetDriverConsts default constructor.
//const int nNetTimeout = 300;

const int N_PACKET_SIZE = 32768;	// Must be equal with buffer size from Net/NetStream.h
//const int N_PACKET_SIZE = 128000;	// Must be equal with buffer size from Net/NetStream.h


CMultiplayerInternal::CMultiplayerInternal( const NDb::SNetGameConsts *_pConsts, IAICmdsAutoMagic *pCmds ) 
	: pConsts(_pConsts), pCmdsSerialize(pCmds) 
{
}

int CMultiplayerInternal::GetPort() const
{
	return pConsts ? pConsts->nPort : 8888;
}

void CMultiplayerInternal::CreateAsHost()
{
	pNetDriver = NNet::CreateNetDriver( NNet::SNetDriverConsts( nNetTimeout ) );
	CPtr<NNet::ILinksManager> pLinksManager = NNet::CreateServerLinksManager( GetPort() );
	pNetDriver->Init( NGlobal::GetVar("NetGameVersion", 1), GetPort(), false, pLinksManager );

	pNetDriver->StartGame();
	pNetDriver->StartNewPlayerAccept();
}

bool CMultiplayerInternal::CreateAsClient( const char *pszIPAddress )
{
	pNetDriver = NNet::CreateNetDriver( NNet::SNetDriverConsts( nNetTimeout ) );
	CPtr<NNet::ILinksManager> pLinksManager = NNet::CreateClientLinksManager();
	pNetDriver->Init( NGlobal::GetVar("NetGameVersion", 1), GetPort(), true, pLinksManager );

	NNet::CNodeAddress address;
	if ( address.SetInetName( pszIPAddress, GetPort() ) )
	{
		pNetDriver->ConnectGame( address, CMemoryStream() );

		Singleton<IConsoleBuffer>()->WriteASCII(
			CONSOLE_STREAM_CONSOLE, StrFmt( "request for connect is sent" ), 0xffffff00, true
		);

		return true;
	}
	return false;
}

void CMultiplayerInternal::SendAICommand( CObjectBase *pCommand )
{
	CPtr<CObjectBase> pHold = pCommand;
	{
		pktOutgoing << uint8_t(NGM_AI_CMD);
		// add command to packet
		int nID = pCmdsSerialize->GetCommandID( pCommand );
		pktOutgoing.Write( &nID, pCmdsSerialize->GetIDSize() );
		CObj<IBinSaver> pSaver = pCmdsSerialize->MakeCommandSerializer( &pktOutgoing, SAVER_MODE_WRITE );
		pSaver->AddPolymorphicBase( 1, pCommand );
		//pCommand->Store( &pktOutgoing );
	}

	ASSERT( pktOutgoing.GetSize() < N_PACKET_SIZE - 1000 );
}

void CMultiplayerInternal::SendDirect( int nPlayer, CObjectBase *pCommand )
{
	CPtr<CObjectBase> pHold = pCommand;
	CMemoryStream pkt;
	{
		pkt << uint8_t(NGM_AI_CMD);
		// add command to packet
		int nID = pCmdsSerialize->GetCommandID( pCommand );
		pkt.Write( &nID, pCmdsSerialize->GetIDSize() );
		CObj<IBinSaver> pSaver = pCmdsSerialize->MakeCommandSerializer( &pkt, SAVER_MODE_WRITE );
		pSaver->AddPolymorphicBase( 1, pCommand );
	}

	ASSERT( pkt.GetSize() < N_PACKET_SIZE - 1000 );

	pNetDriver->SendDirect( nPlayer, pkt );
}

void CMultiplayerInternal::FinishSegment()
{
	pktOutgoing << uint8_t(NGM_ID_SEGMENT);
	pNetDriver->SendBroadcast( pktOutgoing );
	pktOutgoing.SetSizeDiscard( 0 );
}

int CMultiplayerInternal::GetSelfPlayerNum()
{
	return pNetDriver->GetSelfClientID();
}

CMultiplayerInternal::EConnectionState CMultiplayerInternal::GetState()
{
	switch ( pNetDriver->GetState() )
	{
	case NNet::IDriver::INACTIVE:
		return INACTIVE;
	case NNet::IDriver::ACTIVE:
		return ACTIVE;
	case NNet::IDriver::CONNECTING:
		return CONNECTING;
	default:
		ASSERT(0);
		break;
	}
	return INACTIVE;
}

float CMultiplayerInternal::GetPing( int nPlayer )
{
	return pNetDriver->GetPing( nPlayer );
}

CMultiplayerCommand* CMultiplayerInternal::GetCommand()
{
	if ( !CheckConnection() )
		return 0;
	if ( pktIncoming.GetPosition() != pktIncoming.GetSize() )
		return ProcessPacket( nIncomingPktClientID, &pktIncoming );

	NNet::IDriver::EMessage eMsgID;
	int nClientID;
	CMemoryStream pkt;
	//
	if ( pNetDriver->GetMessage( &eMsgID, &nClientID, 0, &pkt ) )
	{
		CMultiplayerCommand *pCommand = 0;
		switch ( eMsgID ) 
		{
		case NNet::IDriver::DIRECT:
		case NNet::IDriver::BROADCAST:
			nIncomingPktClientID = nClientID;
			pktIncoming = pkt;
			pktIncoming.Seek(0);
			return GetCommand();

		case NNet::IDriver::NEW_CLIENT:
			pCommand = new CMultiplayerCommand( CMultiplayerCommand::EMC_PLAYER_ADDED, nClientID, -1, 0 );
			Singleton<IConsoleBuffer>()->WriteASCII(
				CONSOLE_STREAM_CONSOLE, StrFmt( "new client" ), 0xffffff00, true
				);
			break;

		case NNet::IDriver::REMOVE_CLIENT:
			pCommand = new CMultiplayerCommand( CMultiplayerCommand::EMC_PLAYER_REMOVED, nClientID, -1, 0 );
			break;
		}
		return pCommand;
	}
	return 0;
}

bool CMultiplayerInternal::CheckConnection()
{
	return GetState() != INACTIVE;
}

CMultiplayerCommand* CMultiplayerInternal::ProcessPacket( const int nClientID, CMemoryStream *pPkt )
{
	CMemoryStream &pkt = *pPkt;
	uint8_t msgID = 0;
	if ( pkt.GetPosition() < pkt.GetSize() )
	{
		pkt.Read( &msgID, sizeof(msgID) );
		switch ( msgID ) 
		{
		case NGM_ID_SEGMENT:
			return new CMultiplayerCommand( CMultiplayerCommand::EMC_SEGMENT_FINISHED, nClientID, 0, 0 );
		case NGM_AI_CMD:
			{
				int nID = 0;
				pkt.Read( &nID, pCmdsSerialize->GetIDSize() );
				CObjectBase *pAICmd = pAICmd = pCmdsSerialize->MakeCommand( nID );
				{
					CObj<IBinSaver> pSaver = pCmdsSerialize->MakeCommandSerializer( &pkt, SAVER_MODE_READ );
					pSaver->AddPolymorphicBase( 1, pAICmd );
				}
				return new CMultiplayerCommand( CMultiplayerCommand::EMC_AI_COMMAND, nClientID, -1, pAICmd );
			}
		default:
			ASSERT( 0 && "wrong network packet format" );
			pkt.Seek( pkt.GetSize() );
			break;
		}
	}
	return 0;
// new unregistered player, kick him
//	else
//		pInGameNetDriver->Kick( nClientID );
}

void CMultiplayerInternal::SendRecv()
{
	pNetDriver->Step();
//	else
//		NI_ASSERT( false, "Can't establish connection!" );
}

IMultiplayer *CreateMultiplayerHost( IAICmdsAutoMagic *pCmds, const NDb::SNetGameConsts *pConsts )
{
	CMultiplayerInternal *pRes = new CMultiplayerInternal( pConsts, pCmds );
	pRes->CreateAsHost();
	return pRes;
}
IMultiplayer *CreateMultiplayerClient( IAICmdsAutoMagic *pCmds, const NDb::SNetGameConsts *pConsts, const char *pszIPAddress )
{
	CPtr<CMultiplayerInternal> pRes = new CMultiplayerInternal( pConsts, pCmds );
	if ( pRes->CreateAsClient( pszIPAddress ) )
		return pRes.Extract();
	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x300A7540, CMultiplayerInternal )

