#include "StdAfx.h"

#include "MPManagerModeNivalNet.hpp"
#include "../Server_Client_Common/ChatPackets.h"
#include "../Client/ServerClient.h"
#include "../Misc/StrProc.h"
#include "InterfaceState.h"

void CMPManagerModeNivalNet::RequestChatChannels( DWORD dwVersion )
{
	CChatChannelsListRequestPacket *pPacket = new CChatChannelsListRequestPacket( 0, dwVersion );
	pClient->SendPacket( pPacket );
}

//
//	Chat-related Messages
//

bool CMPManagerModeNivalNet::OnChatMessage( SMPUIChatMessage *pMsg )
{
	wstring wszFilteredText = InterfaceState()->FilterMPChatText( pMsg->wszText );

	if ( eState == EGS_LOBBY || eState == EGS_LADDER_WAIT_CLIENTS || eState == EGS_LOBBY_WAIT_LADDER )
	{
		if ( pMsg->bPrivate )
		{
			pClient->SendPacket( new CChatPacket( 0, wszFilteredText, pMsg->szName, 0, false ) );
			PushMessage( new SMPUIChatMessage( szMPName, wszFilteredText ) );
		}
		else 
			pClient->SendPacket( new CChatPacket( 0, wszFilteredText, "", 0, true ) );
	}
	else if ( eState == EGS_GAME_ROOM )
	{
		CChatPacket *pPacket = new CChatPacket( 0, wszFilteredText, szMPName, 0, true );
		pClient->SendGamePacket( pPacket, true );
		PushMessage( new SMPUIChatMessage( szMPName, wszFilteredText ) );		// Doesn't matter if it's private
	}

	// No need to report it back, the server does it

	return true;
}

bool CMPManagerModeNivalNet::OnJoinChatChannelMessage( SMPUIJoinChannelMessage *pMsg )
{
	CChatChannelPacket *pChannelPkt = new CChatChannelPacket( 0, pMsg->szChannel );
	pClient->SendPacket( pChannelPkt );
	return true;
}

bool CMPManagerModeNivalNet::OnRequestChatChannelsMessage( SMPUIMessage *pMsg )
{
	updateChannels.bUpdating = true;
	RequestChatChannels( updateChannels.dwVersion );
	return true;
}

bool CMPManagerModeNivalNet::OnChangeFriendIgnoreStatusMessage( SMPUIChangeFriendIgnoreStatusMessage *pMsg )
{
	CChatModifyIgnoreFriendListPacket *pPkt = new CChatModifyIgnoreFriendListPacket( 0, pMsg->szNick, CChatModifyIgnoreFriendListPacket::REMOVE_IGNORE );
	switch ( pMsg->eAction )
	{
	case SMPUIChangeFriendIgnoreStatusMessage::EA_ADD_FRIEND:
		pPkt->eChange = CChatModifyIgnoreFriendListPacket::ADD_FRIEND;
		break;
	case SMPUIChangeFriendIgnoreStatusMessage::EA_ADD_IGNORE:
		pPkt->eChange = CChatModifyIgnoreFriendListPacket::ADD_IGNORE;
		break;
	case SMPUIChangeFriendIgnoreStatusMessage::EA_REMOVE_FRIEND:
		pPkt->eChange = CChatModifyIgnoreFriendListPacket::REMOVE_FRIEND;
		break;
	case SMPUIChangeFriendIgnoreStatusMessage::EA_REMOVE_IGNORE:
		pPkt->eChange = CChatModifyIgnoreFriendListPacket::REMOVE_IGNORE;
		break;
	}
	pClient->SendPacket( pPkt );
	return true;
}

//
//	Chat-related Packets
//

bool CMPManagerModeNivalNet::OnChatChannelsListPacket( class CChatChannelsListPacket *pPacket )
{
	//DebugTrace( "+++ ChatChannelsList" );
	if ( pPacket->bIsFullUpdate )
		chatChannels.clear();

	for ( list<string>::iterator it = pPacket->added.begin(); it != pPacket->added.end(); ++it )
		chatChannels.insert( *it );

	for ( list<string>::iterator it = pPacket->removed.begin(); it != pPacket->removed.end(); ++it )
		chatChannels.erase( *it );

	SMPUIChatChannelListMessage *pChannelsMsg = new SMPUIChatChannelListMessage;
	for ( hash_set<string>::iterator it = chatChannels.begin(); it != chatChannels.end(); ++it )
		pChannelsMsg->channels.push_back( *it );

	PushMessage( pChannelsMsg );
	updateChannels.dwVersion = pPacket->dwVersion;
	return true;
}

bool CMPManagerModeNivalNet::OnChatChannelClientsPacket( class CChatChannelClientsListPacket *pPacket )
{
	//DebugTrace( "+++ ChatClientsList" );
	SMPUIChatChannelNicksMessage *pMsg = new SMPUIChatChannelNicksMessage;
	pMsg->eType = SMPUIChatChannelNicksMessage::ELT_CHAT;

	for ( list<SIDNickPair>::iterator it = pPacket->clientsList.begin(); it != pPacket->clientsList.end(); ++it )
	{
		SIDNickPair &nick = *it;
		pMsg->nicks.push_back( nick.szNick );
	}
	PushMessage( pMsg );
	return true;
}

bool CMPManagerModeNivalNet::OnChatChannelClientNotifyPacket( class CChatClientListChangeNotifyPacket *pPacket )
{
	//DebugTrace( "+++ ChatClientsListChange" );
	SMPUIChatChannelNicksChangeMessage *pMsg = new SMPUIChatChannelNicksChangeMessage( pPacket->szNick, 
		( pPacket->bJoined ? EMPS_ONLINE : EMPS_OFFLINE ), false );
	PushMessage( pMsg );
	return true;
}

bool CMPManagerModeNivalNet::OnChatIgnoreFriendListPacket( class CChatIgnoreFriendListPacket *pPacket )
{
	SMPUIChatChannelNicksMessage *pMsg = new SMPUIChatChannelNicksMessage;
	pMsg->eType = SMPUIChatChannelNicksMessage::ELT_FRIEND;
	for ( list<string>::iterator it = pPacket->friendList.begin(); it != pPacket->friendList.end(); ++it )
	{
		string &nick = *it;
		pMsg->nicks.push_back( nick );
	}
	PushMessage( pMsg );

	pMsg = new SMPUIChatChannelNicksMessage;
	pMsg->eType = SMPUIChatChannelNicksMessage::ELT_IGNORE;
	for ( list<string>::iterator it = pPacket->ignoreList.begin(); it != pPacket->ignoreList.end(); ++it )
	{
		string &nick = *it;
		pMsg->nicks.push_back( nick );
	}
	PushMessage( pMsg );

	return true;
}

bool CMPManagerModeNivalNet::OnChatFriendNotifyPacket( class CChatFriendNotifyPacket *pPacket )
{
	SMPUIChatChannelNicksChangeMessage *pMsg = new SMPUIChatChannelNicksChangeMessage( pPacket->szNick, EMPS_OFFLINE, true );
	switch ( pPacket->cChatStatus )
	{
	case 0:
		pMsg->eStatus = EMPS_OFFLINE;
		break;
	case 1:
		pMsg->eStatus = EMPS_ONLINE;
		break;
	case 2:
		pMsg->eStatus = EMPS_BUSY;
		break;
	}
	PushMessage( pMsg );
	return true;
}

bool CMPManagerModeNivalNet::OnChatAFKResponsePacket( class CChatAFKResponsePacket *pPacket )
{
	PushMessage( new SMPUIChatMessage( NStr::ToUnicode( pPacket->szAFKNick ) + L" - " + InterfaceState()->GetTextEntry( "T_CHAT_AFK_RESPONSE" ) ) );
	return true;
}


