#include "stdafx.h"

#include "InterfaceMPLobby.h"
#include "MPLobbyData.h"
#include "MultiplayerCommandManager.h"
#include "ChatControl.h"
#include "..\Misc\StrProc.h"
#include "GameRoomData.h"
#include "InterfaceState.h"


// Chat-related methods for MP Lobby

void CInterfaceMPLobby::TryToJoinChatChannel( const string &szChannel )
{
	bJoiningChannel = true;
	szCurrentChannel = szChannel;
	pChannelNameEdit->SetText( NStr::ToUnicode( szChannel ).c_str() );
	SMPUIJoinChannelMessage *pJoinMsg = new SMPUIJoinChannelMessage( szChannel );
	Singleton<IMPToUIManager>()->AddUIMessage( pJoinMsg );
	SMPUISimpleMessage *pRequestMsg = new SMPUISimpleMessage( EMUI_REQUEST_CHAT_CHANNELS );
	Singleton<IMPToUIManager>()->AddUIMessage( pRequestMsg );
}

bool CInterfaceMPLobby::OnChatCancelJoinChannel()
{
	pChannelNameEdit->SetText( NStr::ToUnicode( szCurrentChannel ).c_str() );
	return true;
}

bool CInterfaceMPLobby::OnChatJoinNamedChannel()
{
	TryToJoinChatChannel( NStr::ToMBCS( pChannelNameEdit->GetText() ) );
	return true;
}

bool CInterfaceMPLobby::OnChatSelectChannel()
{
	IListControlItem *pItem = pChannelsList->GetSelectedListItem();
	if ( pItem )
	{
		CTextData *pData = dynamic_cast<CTextData*>( pItem->GetUserData() );
		if ( pData )
			pChannelNameEdit->SetText( pData->wszText.c_str() );
	}

	OnChatHideChannels();
	return true;
}

bool CInterfaceMPLobby::OnChatShowChannels()
{
	pChannelsButtonDisabler->ShowWindow( true );
	pChannelsList->ShowWindow( true );
	pChannelsButton->SetState( 1 );

	return true;
}

bool CInterfaceMPLobby::OnChatHideChannels()
{
	pChannelsButtonDisabler->ShowWindow( false );
	pChannelsList->ShowWindow( false );
	pChannelsButton->SetState( 0 );
	return true;
}

bool CInterfaceMPLobby::OnChatEscape()
{
	pChatInput->SetText( L"" );			
	return true;
}

bool CInterfaceMPLobby::OnPostChatMessageReaction()
{
	if ( pChatInput )
	{
		wstring wszText = pChatInput->GetText();
		if ( !wszText.empty() )
		{
			if ( pSelection )
				Singleton<IMPToUIManager>()->AddUIMessage( new SMPUIChatMessage( pSelection->szName, wszText ) );					
			else
				Singleton<IMPToUIManager>()->AddUIMessage( new SMPUIChatMessage( wszText ) );					
		}
		pChatInput->SetText( L"" );
	}
	return true;
}

void CInterfaceMPLobby::RebuildClientList()
{
	for ( CChatClientsList::iterator it = chatClients.begin(); it != chatClients.end(); ++it )
		it->second->pListItem = 0;
	for ( CChatClientsList::iterator it = chatFriends.begin(); it != chatFriends.end(); ++it )
		it->second->pListItem = 0;

	CChatClientsList *pClients = bShowingFriends ? &chatFriends : &chatClients;
	if ( pSelection )
		pSelection->pListItem = 0;
	pNicksList->RemoveAllElements();
	for ( CChatClientsList::iterator it = pClients->begin(); it != pClients->end(); ++it )
	{
		CClientListData *pData = it->second;
		pData->pListItem = 0;
		pData->pStatusIcon = 0;
		pData->pListItem = pNicksList->AddItem( pData );
	}
	pNicksList->Update();
	if ( !pSelection || !pSelection->pListItem )
	{
		pSelection = 0;
		if ( pPostMessage )
			pPostMessage->SetState( 0 );
	}
	pNicksList->SelectItem( pSelection );
}

// MP->UI messages

bool CInterfaceMPLobby::OnChatMessage( SMPUIChatMessage *pMsg )
{
	wstring wszText;
	if ( pMsg->szName.empty() )
		wszText = pMsg->wszText;
	else
	{
		wstring wszPrivate;
		if ( pMsg->bPrivate )
			wszPrivate = InterfaceState()->GetTextEntry( "T_PRIVATE_MESSAGE_PREFIX" );
		wszText = NStr::ToUnicode( pMsg->szName ) + wszPrivate + L" : " + pMsg->wszText;
	}
	pChatOutput->AddItem( wszText );
	return true;
};

bool CInterfaceMPLobby::OnChatChannelsListMessage( struct SMPUIChatChannelListMessage *pMsg )
{
	pChannelsList->RemoveAllElements();

	for ( list<string>::iterator it = pMsg->channels.begin(); it != pMsg->channels.end(); ++it )
	{
		CTextData *pNewData = new CTextData( NStr::ToUnicode( *it ) );
		pChannelsList->AddItem( pNewData );
	}
	pChannelsList->Update();

	return true;
}

bool CInterfaceMPLobby::OnChatChannelNicksMessage( struct SMPUIChatChannelNicksMessage *pMsg )
{
	if ( bJoiningChannel )
	{
		bJoiningChannel = false;
		pChannelNameEdit->SetText( NStr::ToUnicode( szCurrentChannel ).c_str() );
		pChatOutput->AddItem(  GetScreen()->GetTextEntry( "T_ENTER_CHAT_CHANNEL" ) + NStr::ToUnicode( szCurrentChannel )  );
	}

	CChatClientsList *pList = &chatClients;
	EMPChatStatus eDefaultStatus = EMPS_ONLINE;
	if ( pMsg->eType == SMPUIChatChannelNicksMessage::ELT_FRIEND )
	{
		pList = &chatFriends;
		eDefaultStatus = EMPS_OFFLINE;
	}
	else if ( pMsg->eType == SMPUIChatChannelNicksMessage::ELT_IGNORE )
		pList = &chatIgnores;

	// Process clients list
	pList->clear();
	for ( list<string>::iterator it = pMsg->nicks.begin(); it != pMsg->nicks.end(); ++it )
	{
		CPtr<CClientListData> pData = new CClientListData( *it, eDefaultStatus );
		(*pList)[*it] = pData;
	}
	RebuildClientList();
	pChatInput->SetFocus( true );

	return true;
}

bool CInterfaceMPLobby::OnChatChannelNicksChangeMessage( struct SMPUIChatChannelNicksChangeMessage *pMsg )
{
	if ( pMsg->bFriend )
	{
		// Friends list
		CChatClientsList::iterator itFriend = chatFriends.find( pMsg->szNick );
		if ( itFriend == chatFriends.end() )
			chatFriends[pMsg->szNick] = new CClientListData( pMsg->szNick, pMsg->eStatus );
		else
			itFriend->second->eStatus = pMsg->eStatus;
	}
	else
	{
		// Chat clients
		CChatClientsList::iterator itChat = chatClients.find( pMsg->szNick );
		bool bPresentInChat = ( itChat != chatClients.end() );
		if ( bPresentInChat && ( pMsg->eStatus == EMPS_OFFLINE ) )
		{
			chatClients.erase( itChat );
			//pChatOutput->AddItem( NStr::ToUnicode( pMsg->szNick ) + GetScreen()->GetTextEntry( "T_USER_LEFT" ) );
		}
		else if ( !bPresentInChat && ( pMsg->eStatus != EMPS_OFFLINE ) )
		{
			chatClients[pMsg->szNick] = new CClientListData( pMsg->szNick, EMPS_ONLINE );
			//pChatOutput->AddItem( NStr::ToUnicode( pMsg->szNick ) + GetScreen()->GetTextEntry( "T_USER_ARRIVED" ) );
		}
	}
	RebuildClientList();

	return true;
}

int CInterfaceMPLobby::GetButtonState( const string &szNick )
{
	CChatClientsList::iterator itIgnore = chatIgnores.find( szNick );
	if ( itIgnore != chatIgnores.end() )
		return 1;

	CChatClientsList::iterator itFriend = chatFriends.find( szNick );
	if ( itFriend != chatFriends.end() )
	{
		EMPChatStatus eFriendStatus = itFriend->second->eStatus;
		switch ( eFriendStatus )
		{
		case EMPS_ONLINE:
			return 2;
		case EMPS_OFFLINE:
			return 3;
		case EMPS_BUSY:
			return 4;
		}
	}

	return 0;
}


