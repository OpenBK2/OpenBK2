#include "stdafx.h"
#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "Input/gamemessage.h"
#include "UI/UI.h"
#include "UI/uifactory.h"
#include "InterfaceMPLobby.h"
#include "MPLobbyData.h"
#include "GameXClassIDs.h"
#include "Misc/StrProc.h"
#include "UI/windoweditline.h"
#include "UI/windowListctrl.h"
#include "MultiplayerCommandManager.h"
#include "InterfaceState.h"
#include "InterfaceMisc.h"

#include "GameX_export.h"

#include <fmt/format.h>

CInterfaceMPLobby::CInterfaceMPLobby() : 
CInterfaceMPScreenBase( "MPGameLobby", "game_lobby" )
{
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_CHAT_MESSAGE, SMPUIChatMessage, &CInterfaceMPLobby::OnChatMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_CHAT_CHANNELS, SMPUIChatChannelListMessage, &CInterfaceMPLobby::OnChatChannelsListMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_CHAT_NICKS, SMPUIChatChannelNicksMessage, &CInterfaceMPLobby::OnChatChannelNicksMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_CHAT_NICKS_CHANGE, SMPUIChatChannelNicksChangeMessage, &CInterfaceMPLobby::OnChatChannelNicksChangeMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_SHORT_INFO, SMPUIShortInfoMessage, &CInterfaceMPLobby::OnShortInfoMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_NIVAL_NET_LADDER, SMPUILadderStatusChangeMessage, &CInterfaceMPLobby::OnLadderMessage );
}

bool CInterfaceMPLobby::Init()
{
	if ( CInterfaceScreenBase::	Init() == false ) 
		return false;
	AddScreen( this );
	pMain = GetChildChecked<IWindow>( pScreen, "Main", true );
	if ( !pMain )
		return false;	
	SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );
	InitControls();
	TryToJoinChatChannel( "Common" );
	bShowingFriends = false;
	return true;
}

void CInterfaceMPLobby::InitControls()
{
	pChatInput = GetChildChecked<IEditLine>( pMain, "ChatInput", true );
	IScrollableContainer *pChatOutputWnd = GetChildChecked<IScrollableContainer>( pMain, "ChatOutput", true );
	pChatOutput = new CChatControlWrapper( pChatOutputWnd, 100 );
	pChannelsButtonDisabler = GetChildChecked<IWindow>( pMain, "ChannelsButtonDisabler", true );
	pChannelsButtonDisabler->ShowWindow( false );
	pChannelsList = GetChildChecked<IListControl>( pMain, "ChannelsList", true );
	pChannelsButton = GetChildChecked<IButton>( pMain, "ChannelsButton", true );
	CPtr<CChannelListViewer> pViewer = new CChannelListViewer();
	pChannelsList->SetViewer( pViewer );		
	pChannelsList->Update();
	pChannelsList->ShowWindow( false );
	pChannelNameEdit = GetChildChecked<IEditLine>( pMain, "ChatChannelEdit", true );
	pNicksList = GetChildChecked<IListControl>( pMain, "ClientsList", true );
	CPtr<CClientListViewer> pClientsViewer = new CClientListViewer( this );
	pNicksList->SetViewer( pClientsViewer );		
	pNicksList->Update();
	pClientInfoWindow = GetChildChecked<IWindow>( pMain, "ClientInfo", true );
	pLadderButton = GetChildChecked<IButton>( pMain, "LadderGame", true );
	pCustomButton = GetChildChecked<IButton>( pMain, "CustomGame", true );
	pCancelButton = GetChildChecked<IButton>( pMain, "CancelButton", true );
	pCancelButton->ShowWindow( false );
	pWaitingForLadder = GetChildChecked<ITextView>( pMain, "WaitForLadder", true );;
	pWaitingForLadder->ShowWindow( false );
	pStatsButton = GetChildChecked<IButton>( pMain, "MyLadderStats", true );
	pBackButton = GetChildChecked<IButton>( pMain, "ButtonBack", true );

	const bool bMPDemo = NGlobal::GetVar( "MP_DEMO", 0 ) != 0;
	const bool bMPLadderDemo = NGlobal::GetVar( "MP_LADDER_DEMO", 0 ) != 0;
	if ( bMPDemo && !bMPLadderDemo )
	{
		pLadderButton->Enable( false );
		pStatsButton->Enable( false );
	}
	pPostMessage = GetChildChecked<IButton>( pMain, "ButtonPostMessage", true );
}

bool CInterfaceMPLobby::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "react_on_back" )
		return OnBackReaction();

	if ( szReaction == "custom_game_enter" )
		return OnCustomGameReaction();

	if ( szReaction == "chat_enter" )
		return OnPostChatMessageReaction();
	if ( szReaction == "chat_escape" )
		return OnChatEscape();

	if ( szReaction == "react_on_begin_game" )
		return OnLadderGameReaction();

	if ( szReaction == "react_on_select_channel" )
		return OnChatSelectChannel();
	if ( szReaction == "react_on_join_channel" )
		return OnChatJoinNamedChannel();
	if ( szReaction == "react_on_cancel_join_channel" )
		return OnChatCancelJoinChannel();

	if ( szReaction == "react_on_channels_open" )
		return OnChatShowChannels();
	if ( szReaction == "react_on_channels_close" )
		return OnChatHideChannels();

	if ( szReaction == "react_on_client_menu" )
		return OnClientMenu( szSender );
	if ( szReaction == "react_client_cancel" )
		return OnClientInfoClose( false );
	if ( szReaction == "react_client_ok" )
		return OnClientInfoClose( true );

	if ( szReaction == "react_client_friend_change" )
		return OnClientInfoFriendChange();
	if ( szReaction == "react_client_ignore_change" )
		return OnClientInfoIgnoreChange();

	if ( szReaction == "react_cancel_ladder" )
		return OnLadderCancelReaction();
	if ( szReaction == "react_on_my_ladder_stats" )
		return OnLadderStatsReaction();

	if ( szReaction == "client_selected" )
		return OnClientSelected();

	if ( szReaction == "react_on_show_friends" )
		return OnFriendsSwitch( true );
	if ( szReaction == "react_on_show_all" )
		return OnFriendsSwitch( false );

	return false;
}

bool CInterfaceMPLobby::OnLadderGameReaction()
{
	NMainLoop::Command( ML_COMMAND_MP_LADDER_GAME, "" ); 
	return true;
}

int CInterfaceMPLobby::Check( const std::string &szCheckName ) const
{
	return 0;
}

bool CInterfaceMPLobby::OnBackReaction()
{
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_NO_NET );
	
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MULTIPLAYER_MENU, "" );
	//Crap
	return true;
}

bool CInterfaceMPLobby::OnCustomGameReaction()
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MP_CUSTOM_GAME_MENU, "" ); 
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_NIVAL_NET_CUSTOM_GAME );
	return true;
}

bool CInterfaceMPLobby::OnClientMenu( const std::string &szSender )
{
	// Ignore for myself
	bool bOwnInfo = ( NStr::ToUnicode( szSender ) == NGlobal::GetVar( "Multiplayer.PlayerName", L"Player" ).GetString() );

	CChatClientsList *pCurrentList = bShowingFriends ? &chatFriends : &chatClients;
	CChatClientsList::iterator it = pCurrentList->find( szSender );
	if ( it == pCurrentList->end() )
		return true;
	CClientListData *pClientData = it->second;
	szSelectedClient = pClientData->szName;

	ITextView *pClientName = GetChildChecked<ITextView>( pClientInfoWindow, "Client_Name", true );
	if ( !pClientName )
		return true;
	pClientName->SetText( pClientName->GetDBText() + NStr::ToUnicode( szSelectedClient ) );

	IButton *pStatusButton = GetChildChecked<IButton>( pClientInfoWindow, "Client_Status", true );
	ITextView *pStatusText = GetChildChecked<ITextView>( pClientInfoWindow, "Client_StatusText", true );
	int nStatusCode = GetButtonState( szSelectedClient );
	if ( pStatusButton )
		pStatusButton->SetState( nStatusCode );
	std::string szStatusCode;
	switch ( nStatusCode )
	{
	case 1:
		szStatusCode = "T_STATUS_IGNORE";
		break;
	case 2:
		szStatusCode = "T_STATUS_FRIEND_ONLINE";
		break;
	case 3:
		szStatusCode = "T_STATUS_FRIEND_OFFLINE";
		break;
	case 4:
		szStatusCode = "T_STATUS_FRIEND_BUSY";
		break;
	default:
		szStatusCode = "T_STATUS_ONLINE";
	}
	if ( pStatusText )
		pStatusText->SetText( pStatusText->GetDBText() + GetScreen()->GetTextEntry( szStatusCode ) );

	IWindow *pInfoArea = GetChildChecked<IWindow>( pClientInfoWindow, "Client_InfoArea", true );
	if ( pInfoArea )
		pInfoArea->ShowWindow( false );
	ITextView *pWaitText = GetChildChecked<ITextView>( pClientInfoWindow, "GettingInfo", true );
	if ( pWaitText )
		pWaitText->ShowWindow( true );

	bool bFriendNow = ( chatFriends.find( szSelectedClient ) != chatFriends.end() );
	bool bIgnoreNow = ( chatIgnores.find( szSelectedClient ) != chatIgnores.end() );
	IButton *pFriendCheck = GetChildChecked<IButton>( pClientInfoWindow, "Client_Friend", true );
	if ( pFriendCheck )
	{
		pFriendCheck->SetState( bFriendNow ? 1 : 0 );
		pFriendCheck->ShowWindow( !bOwnInfo );
	}
	IButton *pIgnoreCheck = GetChildChecked<IButton>( pClientInfoWindow, "Client_Ignore", true );
	if ( pIgnoreCheck )
	{
		pIgnoreCheck->SetState( bIgnoreNow ? 1 : 0 );
		pIgnoreCheck->ShowWindow( !bOwnInfo );
	}

	ITextView *pFriendLabel = GetChildChecked<ITextView>( pClientInfoWindow, "Client_FriendLabel", true );
	if ( pFriendLabel )
		pFriendLabel->ShowWindow( !bOwnInfo );
	ITextView *pIgnoreLabel = GetChildChecked<ITextView>( pClientInfoWindow, "Client_IgnoreLabel", true );
	if ( pIgnoreLabel )
		pIgnoreLabel->ShowWindow( !bOwnInfo );

	if ( pClientInfoWindow )
		pClientInfoWindow->ShowWindow( true );

	Singleton<IMPToUIManager>()->AddUIMessage( new SMPUILadderInfoRequestMessage( szSelectedClient, true ) );
	pNicksList->SelectItem( 0 );
	if ( pPostMessage )
		pPostMessage->SetState( 0 );
	
	return true;
}

bool CInterfaceMPLobby::OnClientInfoClose( const bool bApply )
{
	if ( pClientInfoWindow )
		pClientInfoWindow->ShowWindow( false );

	if ( !bApply )
		return true;

	bool bNeedUpdate = false;

	IButton *pFriendCheck = GetChildChecked<IButton>( pClientInfoWindow, "Client_Friend", true );
	IButton *pIgnoreCheck = GetChildChecked<IButton>( pClientInfoWindow, "Client_Ignore", true );
	if ( !pFriendCheck || !pIgnoreCheck )
		return true;
	bool bFriendOld = ( chatFriends.find( szSelectedClient ) != chatFriends.end() );
	bool bIgnoreOld = ( chatIgnores.find( szSelectedClient ) != chatIgnores.end() );
	bool bFriendNew = ( pFriendCheck->GetState() == 1 );
	bool bIgnoreNew = ( pIgnoreCheck->GetState() == 1 );
	if ( bFriendOld != bFriendNew )
	{
		if ( bFriendOld )
		{
			Singleton<IMPToUIManager>()->AddUIMessage( 
				new SMPUIChangeFriendIgnoreStatusMessage( szSelectedClient, SMPUIChangeFriendIgnoreStatusMessage::EA_REMOVE_FRIEND ) );
			chatFriends.erase( szSelectedClient );
			bNeedUpdate = true;
		}
		else
		{
			Singleton<IMPToUIManager>()->AddUIMessage( 
				new SMPUIChangeFriendIgnoreStatusMessage( szSelectedClient, SMPUIChangeFriendIgnoreStatusMessage::EA_ADD_FRIEND ) );
		}
	}
	if ( bIgnoreOld != bIgnoreNew )
	{
		if ( bIgnoreOld )
		{
			Singleton<IMPToUIManager>()->AddUIMessage( 
				new SMPUIChangeFriendIgnoreStatusMessage( szSelectedClient, SMPUIChangeFriendIgnoreStatusMessage::EA_REMOVE_IGNORE ) );
			chatIgnores.erase( szSelectedClient );
			bNeedUpdate = true;
		}
		else
		{
			Singleton<IMPToUIManager>()->AddUIMessage( 
				new SMPUIChangeFriendIgnoreStatusMessage( szSelectedClient, SMPUIChangeFriendIgnoreStatusMessage::EA_ADD_IGNORE ) );
			chatIgnores[szSelectedClient] = new CClientListData( szSelectedClient, EMPS_OFFLINE );
			bNeedUpdate = true;
		}
	}

	if ( bNeedUpdate )
		RebuildClientList();

	return true;
}

bool CInterfaceMPLobby::OnShortInfoMessage( struct SMPUIShortInfoMessage *pMsg )
{
	IWindow *pInfoArea = GetChildChecked<IWindow>( pClientInfoWindow, "Client_InfoArea", true );
	if ( pInfoArea )
		pInfoArea->ShowWindow( true );
	ITextView *pWaitText = GetChildChecked<ITextView>( pClientInfoWindow, "GettingInfo", true );
	if ( pWaitText )
		pWaitText->ShowWindow( false);

	ITextView *pInfoLevel = GetChildChecked<ITextView>( pInfoArea, "Client_Level", true );
	if ( pInfoLevel )
		pInfoLevel->SetText( pInfoLevel->GetDBText() + NStr::ToUnicode( std::to_string(  pMsg->nLevel ) ) );
	ITextView *pInfoRank = GetChildChecked<ITextView>( pInfoArea, "Client_Rank", true );
	if ( pInfoRank )
		pInfoRank->SetText( pInfoRank->GetDBText() + pMsg->wszRank );

	return true;
}

bool CInterfaceMPLobby::OnClientInfoFriendChange()
{
	IButton *pFriendCheck = GetChildChecked<IButton>( pClientInfoWindow, "Client_Friend", true );
	IButton *pIgnoreCheck = GetChildChecked<IButton>( pClientInfoWindow, "Client_Ignore", true );
	if ( !pFriendCheck || !pIgnoreCheck )
		return true;
	if ( pIgnoreCheck->GetState() == 1 && pFriendCheck->GetState() == 1 )
		pIgnoreCheck->SetState( 0 );
	return true;
}

bool CInterfaceMPLobby::OnClientInfoIgnoreChange()
{
	IButton *pFriendCheck = GetChildChecked<IButton>( pClientInfoWindow, "Client_Friend", true );
	IButton *pIgnoreCheck = GetChildChecked<IButton>( pClientInfoWindow, "Client_Ignore", true );
	if ( !pFriendCheck || !pIgnoreCheck )
		return true;
	if ( pIgnoreCheck->GetState() == 1 && pFriendCheck->GetState() == 1 )
		pFriendCheck->SetState( 0 );
	return true;
}

bool CInterfaceMPLobby::OnLadderMessage( struct SMPUILadderStatusChangeMessage *pMsg )
{
	switch ( pMsg->eState )
	{
	case SMPUILadderStatusChangeMessage::ELS_SEARCH_STARTED:
		{
			pLadderButton->ShowWindow( false );
			pCustomButton->ShowWindow( false );
			pStatsButton->ShowWindow( false );
			pBackButton->ShowWindow( false );
			pCancelButton->ShowWindow( true );
			pWaitingForLadder->ShowWindow( true );
			timeStartWaiting = GameTimer()->GetAbsTime();
			bLadderGameFound = false;
		}
		break;
	case SMPUILadderStatusChangeMessage::ELS_GAME_FOUND:
		{
			pLadderButton->ShowWindow( false );
			pCustomButton->ShowWindow( false );
			pStatsButton->ShowWindow( false );
			pCancelButton->ShowWindow( false );
			pBackButton->ShowWindow( false );
			pWaitingForLadder->ShowWindow( true );
			bLadderGameFound = true;
		}
		break;
	case SMPUILadderStatusChangeMessage::ELS_CANCELLED:
		{
			pLadderButton->ShowWindow( true );
			pCustomButton->ShowWindow( true );
			pStatsButton->ShowWindow( true );
			pBackButton->ShowWindow( true );
			pCancelButton->ShowWindow( false );
			pWaitingForLadder->ShowWindow( false );
			timeStartWaiting = 0;
			bLadderGameFound = false;
			std::wstring wszReason = GetScreen()->GetTextEntry( "LADDER_GAME_CANCELLED" );
			if ( wszReason.length() > 0 )
				NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, CICMessageBox::MakeConfigString( "MessageBoxWindowOk", wszReason ).c_str() );
		}
		break;
	}
	return true;
}

bool CInterfaceMPLobby::StepLocal( bool bAppActive )
{
	if ( bAppActive )
	{
		if ( pWaitingForLadder->IsVisible() )
		{
			int nElapsedSec = ( GameTimer()->GetAbsTime() - timeStartWaiting ) / 1000;
			int nElapsedMin = nElapsedSec / 60;
			nElapsedSec %= 60;

			std::wstring wszText = pWaitingForLadder->GetDBText() + NStr::ToUnicode( fmt::format( " {}:{:02d}", nElapsedMin, nElapsedSec ) );
			if ( bLadderGameFound )
				wszText = wszText + GetScreen()->GetTextEntry( "T_LADDER_GAME_FOUND" );
			pWaitingForLadder->SetText( wszText );
		}
	}

	return CInterfaceMPScreenBase::StepLocal( bAppActive );
}

bool CInterfaceMPLobby::OnLadderCancelReaction()
{
	pLadderButton->ShowWindow( true );
	pCustomButton->ShowWindow( true );
	pStatsButton->ShowWindow( true );
	pBackButton->ShowWindow( true );
	pCancelButton->ShowWindow( false );
	pWaitingForLadder->ShowWindow( false );
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_NIVAL_NET_CANCEL_LADDER );

	return true;
}

bool CInterfaceMPLobby::OnLadderStatsReaction()
{
	NMainLoop::Command( ML_COMMAND_MP_LADDER_STATISTICS, NStr::ToMBCS( NGlobal::GetVar( "Multiplayer.PlayerName", L"Player" ).GetString() ).c_str() );
	return true;
}

bool CInterfaceMPLobby::OnClientSelected()
{
	IListControlItem *pNewSelection = pNicksList->GetSelectedListItem();
	if ( pSelection && pSelection->pListItem == pNewSelection )
	{
		pNicksList->SelectItem( 0 );
		if ( pPostMessage )
			pPostMessage->SetState( 0 );
		pSelection = 0;
		return true;
	}

	pSelection = 0;
	CChatClientsList *pClients = bShowingFriends ? &chatFriends : &chatClients;
	for ( CChatClientsList::iterator it = pClients->begin(); it != pClients->end(); ++it )
	{
		CClientListData *pItem = it->second;
		if ( pItem->pListItem == pNewSelection )
		{
			pSelection = pItem;
			if ( pPostMessage )
				pPostMessage->SetState( 1 );
			break;
		}
	}
	if ( !pSelection )
	{
		pNicksList->SelectItem( 0 );
		if ( pPostMessage )
			pPostMessage->SetState( 0 );
		return true;
	}
	
	return true;
}

bool CInterfaceMPLobby::OnFriendsSwitch( bool bShowFriends )
{
	bShowingFriends = bShowFriends;
	pSelection = 0;
	if ( pPostMessage )
		pPostMessage->SetState( 0 );
	RebuildClientList();
	return true;
}

#ifndef _SINGLE_DEMO
// CICMPLobby

void CICMPLobby::PreCreate()
{
}

void CICMPLobby::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMPLobby::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x321AB300, CInterfaceMPLobby );
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_MP_GAME_LOBBY, CICMPLobby );

#endif // _SINGLE_DEMO
