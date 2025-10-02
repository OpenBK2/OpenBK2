#include "StdAfx.h"
#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "../input/gamemessage.h"
#include "../ui/ui.h"
#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/iconsset.h"
#include "../ui/uifactory.h"
#include "InterfaceMPGameRoom.h"
#include "GameXClassIDs.h"
#include "../UI/SceneClassIDs.h"
#include "../SceneB2/Scene.h"
#include "InterfaceState.h"
#include "../Misc/StrProc.h"
#include "../UISpecificB2/DBUISpecificB2.h"
#include "MultiplayerCommandManager.h"
#include "GetConsts.h"
#include "DBMPConsts.h"
#include "../System/Commands.h"
#include "../Misc/Win32Random.h"
#include "../System/Text.h"
#include "InterfaceMisc.h"

static bool s_bMPAllowStartGame;

// CInterfaceMPGameRoom

CInterfaceMPGameRoom::CInterfaceMPGameRoom() : 
CInterfaceMPScreenBase( "MPGameRoom", "game_room" )
{
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_CHAT_MESSAGE, SMPUIChatMessage, &CInterfaceMPGameRoom::OnChatMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_GAME_ROOM_INIT, SMPUIGameRoomInitMessage, &CInterfaceMPGameRoom::OnGameRoomInitMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_UPDATE_SLOT, SMPUIUpdateSlotMessage, &CInterfaceMPGameRoom::OnUpdateSlotMessage );
}

CInterfaceMPGameRoom::~CInterfaceMPGameRoom()
{
	if ( pScreen ) 
		Scene()->RemoveScreen( pScreen );
}

bool CInterfaceMPGameRoom::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	RegisterObservers();

	// load screen
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	if ( AddUIScreen( pScreen, "MPGameRoom", this ) == false )
		return false;

	pMain = GetChildChecked<IWindow>( pScreen, "Main", true );
	NI_VERIFY( pMain, "No main window found", return false );	
	SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );

	InitControls();
	SetControls();
	bConnected = false;
	pWaitMessage->ShowWindow( true );
	NWin32Random::Seed( GetTickCount() );
	return true;
}

void CInterfaceMPGameRoom::RegisterObservers()
{	
	AddObserver( "message_box_ok", &CInterfaceMPGameRoom::MsgOk );
	AddObserver( "message_box_cancel", &CInterfaceMPGameRoom::MsgCancel );
}

void CInterfaceMPGameRoom::InitControls()
{
	pChatInput = GetChildChecked<IEditLine>( pMain, "ChatInput", true );
	pChatOutput = GetChildChecked<IScrollableContainer>( pMain, "ChatOutput", true );
	pChatWrapper = new CChatControlWrapper( pChatOutput, 100 );
	pList = GetChildChecked<IScrollableContainer>( pMain, "PlayerList", true );
	pSlotTemplate = GetChildChecked<IWindow>( pList, "PlayerListItem", true );
	pSlotTemplate->ShowWindow( false );
	pButtonBeginGame = GetChildChecked<IButton>( pMain, "ButtonBeginGame", true );

	pSessionName = GetChildChecked<ITextView>( pMain, "SessionNameData", true );
	pTechLevel = GetChildChecked<ITextView>( pMain, "TechLevelData", true );
	pNumPlayers = GetChildChecked<ITextView>( pMain, "NumPlayersData", true );	
	pMapName = GetChildChecked<ITextView>( pMain, "MapName", true );		
	pTimeLimit = GetChildChecked<ITextView>( pMain, "TimeLimitData", true );	
	pCaptureTime = GetChildChecked<ITextView>( pMain, "CaptureTimeData", true );	
	pGameSpeed = GetChildChecked<ITextView>( pMain, "GameSpeedData", true );	
	pUnitExperience = GetChildChecked<IButton>( pMain, "UnitExperienceData", true );	
	pWaitMessage = GetChildChecked<IWindow>( pMain, "WaitMessage", true );
	pAdvancedPopup = GetChildChecked<IWindow>( pMain, "AdvancedPopup", true );

	pCantStartAccept = GetChildChecked<ITextView>( pMain, "CantStartAccept", true );
	if ( pCantStartAccept )
		pCantStartAccept->ShowWindow( false );
	pCantStartColour = GetChildChecked<ITextView>( pMain, "CantStartColour", true );
	if ( pCantStartColour )
		pCantStartColour->ShowWindow( false );
	pCantStartSide = GetChildChecked<ITextView>( pMain, "CantStartSide", true );
	if ( pCantStartSide )
		pCantStartSide->ShowWindow( false );
}

void CInterfaceMPGameRoom::SetControls()
{
	if ( pButtonBeginGame )
	{
		pButtonBeginGame->ShowWindow( false );
		pButtonBeginGame->Enable( false );
	}
	if ( pWaitMessage )
		pWaitMessage->ShowWindow( true );
}

void CInterfaceMPGameRoom::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
}

bool CInterfaceMPGameRoom::Execute( const string &szSender, const string &szReaction )
{
	if ( szReaction == "react_on_back" )
		return OnBackReaction();

	if ( szReaction == "react_on_begin" )
		return OnBeginGameReaction();	

	if ( szReaction == "react_on_accept" )
		return OnAcceptGameReaction();	

	if ( szReaction == "react_on_reject" )
		return OnRejectGameReaction();	

	if ( szReaction == "chat_enter" )
		return OnChatEnter();

	if ( szReaction == "chat_escape" )
		return OnChatEscape();

	if ( szReaction == "react_on_change_player_side" )
		return OnChangeSideReaction();
 
	if ( szReaction == "react_on_change_player_team" )
		return OnChangeTeamReaction();

	if ( szReaction == "react_on_change_player_color" )
		return OnChangeColorReaction();

	if ( szReaction == "reaction_stop_waiting" )
		return OnInterruptReaction();

	if ( szReaction == "react_on_change_slot_status" )
		return OnPlayerCombo( szSender );

	if ( szReaction == "react_on_open_advanced" )
		return OnShowAdvanced( true );

	if ( szReaction == "react_on_close_advanced" )
		return OnShowAdvanced( false );

	return false;
}

bool CInterfaceMPGameRoom::OnPlayerCombo( const string &szSender )
{
	int nIndex = -1;
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( slots[i].pPlayerCombo->GetName() == szSender )
		{
			nIndex = i;
			break;
		}
	}
	NI_VERIFY( nIndex >= 0, StrFmt( "Combo change received from unknown control '%s'", szSender ), return true );
	SUISlot &slot = slots[nIndex];
	bool bWantClosed = ( slot.pPlayerCombo->GetSelectedIndex() == 1 );

	if ( slot.info.bPresent )
	{
		nRequestedSlotChangeIndex = nIndex;
		bRequestedSlotChangeClosed = bWantClosed;

		// Ask user
		NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
			CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
			GetScreen()->GetTextEntry( "ASK_KICK_PLAYER" ) ).c_str() );

		return true;
	}

	if ( slot.info.bAccept != bWantClosed )
		OpenCloseEmptySlot( nIndex, bWantClosed );

	return true;
}

bool CInterfaceMPGameRoom::OnChangeSideReaction()
{
	// Invoked when OWN slot changes
	// Process country change
	SUISlot &slot = slots[nOwnSlot];
	int nIndex = slot.pCountry->GetSelectedIndex();
	if ( nIndex == 0 )		// Random
	{
		slot.info.bRandomCountry = true;
	}
	else
	{
		slot.info.nCountry = nIndex - 1;
		slot.info.bRandomCountry = false;
	}

	SendUpdateSlot( nOwnSlot );
	CheckStartConditions();
	return true;
}

bool CInterfaceMPGameRoom::OnChangeTeamReaction()
{
	// Invoked when OWN slot changes
	// Process team change
	SUISlot &slot = slots[nOwnSlot];
	slot.info.nTeam = slot.pTeam->GetState();

	SendUpdateSlot( nOwnSlot );
	CheckStartConditions();
	return true;
}

bool CInterfaceMPGameRoom::OnChangeColorReaction()
{
	// Invoked when OWN slot changes
	// Process colour change
	SUISlot &slot = slots[nOwnSlot];
	slot.info.nColour = slot.pColour->GetSelectedIndex();

	SendUpdateSlot( nOwnSlot );
	CheckStartConditions();
	return true;	
}

int CInterfaceMPGameRoom::Check( const string &szCheckName ) const
{
	return 0;
}

bool CInterfaceMPGameRoom::OnBackReaction()
{
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_INTERRUPT );

	ReturnToGamesList();
	return true;
}

bool CInterfaceMPGameRoom::OnBeginGameReaction()
{
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_START_GAME );
	IWindow *pPopup = GetChildChecked<IWindow>( pMain, "Initializing", true );
	if ( pPopup )
	{
		pPopup->ShowWindow( true );
		Draw( 0 );
	}

	return true;
}

bool CInterfaceMPGameRoom::OnShowAdvanced( const bool bShow )
{
	pAdvancedPopup->ShowWindow( bShow );
	return true;
}

bool CInterfaceMPGameRoom::OnAcceptGameReaction()
{	
	// Invoked when OWN slot changes
	// Process Accept change
	SUISlot &slot = slots[nOwnSlot];
	slot.info.bAccept = true;
	slot.pColour->Enable( false );
	slot.pCountry->Enable( false );
	slot.pTeam->Enable( false );
	if ( slot.pControlDisabler )
		slot.pControlDisabler->ShowWindow( true );

	SendUpdateSlot( nOwnSlot );
	CheckStartConditions();
	return true;
}

bool CInterfaceMPGameRoom::OnRejectGameReaction()
{
	// Invoked when OWN slot changes
	// Process Reject change
	SUISlot &slot = slots[nOwnSlot];
	slot.info.bAccept = false;
	slot.pColour->Enable( true );
	slot.pCountry->Enable( true );
	slot.pTeam->Enable( true );
	if ( slot.pControlDisabler )
		slot.pControlDisabler->ShowWindow( false );

	SendUpdateSlot( nOwnSlot );
	CheckStartConditions();
	return true;
}

void CInterfaceMPGameRoom::SendUpdateSlot( const int nSlot )
{
	CPtr<SMPUIUpdateSlotMessage> pMsg = new SMPUIUpdateSlotMessage;
	if ( slots[nSlot].info.bRandomCountry )
	{
		const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();
		slots[nSlot].info.nCountry = NWin32Random::Random( pMPConsts->sides.size() );
	}
	pMsg->info = slots[nSlot].info;
	pMsg->nSlot = nSlot;
	Singleton<IMPToUIManager>()->AddUIMessage( pMsg );					
}

bool CInterfaceMPGameRoom::ProcessEvent( const SGameMessage &msg )
{
	return CInterfaceScreenBase::ProcessEvent( msg );
}

void CInterfaceMPGameRoom::AfterLoad()
{
	CInterfaceScreenBase::AfterLoad();

	RegisterObservers();
}

bool CInterfaceMPGameRoom::OnChatEnter()
{
	if ( pChatInput )
	{
		wstring wszText = pChatInput->GetText();
		if ( !wszText.empty() )
		{
			Singleton<IMPToUIManager>()->AddUIMessage( new SMPUIChatMessage( wszText ) );					
			pChatInput->SetText( L"" );			
		}
	}
	return true;
	
};

bool CInterfaceMPGameRoom::OnChatEscape()
{
	return true;
};

bool CInterfaceMPGameRoom::OnChatMessage( SMPUIChatMessage *pMsg )
{
	wstring wszText;
	if ( pMsg->szName.empty() )
		wszText = pMsg->wszText;
	else
		wszText = NStr::ToUnicode( pMsg->szName ) + L": " + pMsg->wszText;
	pChatWrapper->AddItem( wszText );
	return true;
};

void CInterfaceMPGameRoom::SetTimeLimit( int nTimeLimit )
{
	if ( !pTimeLimit )
		return;	  	
	wstring wszText = NStr::ToUnicode( StrFmt( "%d min.", nTimeLimit ) );
	pTimeLimit->SetText( pTimeLimit->GetDBText() + wszText );	
}

void CInterfaceMPGameRoom::ReturnToGamesList()
{
	// Return to Games list
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MP_CUSTOM_GAME_MENU, "" );

	// Resume updating games list
	SMPUIGameListMessage *pMsg = new SMPUIGameListMessage( true );
	Singleton<IMPToUIManager>()->AddUIMessage( pMsg );
}

bool CInterfaceMPGameRoom::OnGameRoomInitMessage( const SMPUIGameRoomInitMessage *pMsg )
{
	if ( pMsg->eResult != SMPUIGameRoomInitMessage::ERR_SUCCESS )
	{
		ReturnToGamesList();
		string szReasonCode = "DISCONNECT_UNKNOWN";
		switch ( pMsg->eResult )
		{
		case SMPUIGameRoomInitMessage::ERR_GAME_FULL:
			szReasonCode = "DISCONNECT_GAME_FULL";
			break;
		case SMPUIGameRoomInitMessage::ERR_CHECKSUM:
			szReasonCode = "DISCONNECT_CHECKSUM";
			break;
		case SMPUIGameRoomInitMessage::ERR_KICKED:
			szReasonCode = "DISCONNECT_KICKED";
			break;
		case SMPUIGameRoomInitMessage::ERR_GAME_KILLED:
			szReasonCode = "DISCONNECT_GAME_KILLED";
			break;
		case SMPUIGameRoomInitMessage::ERR_CONNECT_FAILED:
			szReasonCode = "DISCONNECT_CONNECT_FAIL";
			break;
		}
		wstring wszReason = GetScreen()->GetTextEntry( szReasonCode );
		if ( wszReason.length() > 0 )
		{
			NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, CICMessageBox::MakeConfigString( "MessageBoxWindowOk", wszReason ).c_str() );
		}
		return true;
	}

	if ( bConnected )
		return true;

	bHost = pMsg->bHost;

	const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();

	// Store info
	pSessionName->SetText( pSessionName->GetDBText() + NStr::ToUnicode( pMsg->szSessionName ) );
	gameDesc = pMsg->specificInfo;
	const int nPlayers = gameDesc.nPlayers;

	// Prepare info for slots (country list, colour list)
	pTechLevel->SetText( pTechLevel->GetDBText() + GET_TEXT_PRE(pMPConsts->techLevels[gameDesc.nTechLevel].,Name) );
	pNumPlayers->SetText( pNumPlayers->GetDBText() + NStr::ToUnicode( StrFmt( "%d", nPlayers ) ) );
	pMapName->SetText( pMapName->GetDBText() + GET_TEXT_PRE( gameDesc.pMPMap->, MapName ) );
	pTimeLimit->SetText( pTimeLimit->GetDBText() + NStr::ToUnicode( StrFmt( "%d min", gameDesc.nTimeLimit ) ) );
	pCaptureTime->SetText( pCaptureTime->GetDBText() + NStr::ToUnicode( StrFmt( "%d sec", gameDesc.nCaptureTime ) ) );
	pGameSpeed->SetText( pGameSpeed->GetDBText() + NStr::ToUnicode( StrFmt( "%d", gameDesc.nGameSpeed ) ) );
	pUnitExperience->SetState( gameDesc.bUnitExp ? 1 : 0 );

	IButton *pRandomPlacement = GetChildChecked<IButton>( pAdvancedPopup, "RandomPlacement", true );
	if ( pRandomPlacement )
		pRandomPlacement->SetState( gameDesc.bRandomPlacement ? 1 : 0 );

	// Set up slots
	slots.resize( nPlayers );
	nOwnSlot = pMsg->nOwnSlot;
	pList->RemoveItems();
	for ( int i = 0; i < nPlayers; ++i )
	{
		SUISlot &slot = slots[i];

		// Set up list items
		IWindow *pWnd = AddWindowCopy( pList, pSlotTemplate );
		pWnd->ShowWindow( true );
		pList->PushBack( pWnd, true );
		slot.pPlayerName = GetChildChecked<IWindow>( pWnd, "PlayerName", true );
		slot.pPlayerNameText = GetChildChecked<ITextView>( slot.pPlayerName, "PlayerNameText", true );
		slot.pPlayerName->SetName( StrFmt( "Name%d", i ) );
		slot.pCountry = GetChildChecked<IComboBox>( pWnd, "Country", true );
		slot.pCountry->SetName( StrFmt( "Country%d", i ) );

		//Set viewer
		slot.pCountry->SetViewer( new CTextureViewer() );	
		CPtr<CTextureData> pRandomItem = new CTextureData( pMPConsts->pRandomCountryIcon, GetScreen()->GetTextEntry( "TT_RANDOM_COUNTRY" ) );
		slot.pCountry->AddItem( pRandomItem );
		for ( int j = 0; j < pMPConsts->sides.size(); ++j )
		{
			if ( pMPConsts->sides[j].pPartyInfo && CHECK_TEXT_NOT_EMPTY_PRE(pMPConsts->sides[j].,Name) )
			{
				CPtr<CTextureData> pData = new CTextureData( pMPConsts->sides[j].pListItemIcon, GET_TEXT_PRE(pMPConsts->sides[j].,Name) );
				slot.pCountry->AddItem( pData );
			}
		}

		slot.pTeam = GetChildChecked<IButton>( pWnd, "Team", true );
		slot.pTeam->SetName( StrFmt( "Team%d", i ) );
		slot.pColour = GetChildChecked<IComboBox>( pWnd, "Colour", true );
		slot.pColour->SetName( StrFmt( "Colour%d", i ) );
		slot.pColour->SetViewer( new CColorViewer() );	
		for ( int j = 0; j < pMPConsts->playerColorInfos.size(); ++j )
			slot.pColour->AddItem( new CColorData( pMPConsts->playerColorInfos[j].nColor | 0xff000000 ) );

		slot.pAccept = GetChildChecked<IButton>( pWnd, "Status", true );
		slot.pAccept->SetName( StrFmt( "Status%d", i ) );

		slot.pPlayerCombo = GetChildChecked<IComboBox>( pWnd, "PlayerCombo", true );
		slot.pPlayerCombo->ShowWindow( false );
		slot.pPlayerCombo->SetName( StrFmt( "PlayerCombo%d", i ) );

		slot.pPing = GetChildChecked<IButton>( pWnd, "Ping", true );

		slot.pControlDisabler = GetChildChecked<IWindow>( pWnd, "ControlDisabler", true );
		if ( slot.pControlDisabler )
			slot.pControlDisabler->ShowWindow( false );

		if ( i != nOwnSlot ) 
		{
			CPtr<IWindow> pShade = GetChildChecked<IWindow>( pWnd, "SlotShade", true );
			if ( pShade )
				pShade->ShowWindow( true );

			slot.pPlayerName->ShowWindow( true );
			slot.pPlayerNameText->SetText( slot.pPlayerNameText->GetDBText() + pScreen->GetTextEntry( "COMBO_OPEN" ) );
			slot.pCountry->Enable( false );
			slot.pCountry->ShowWindow( false );
			slot.pTeam->Enable( false );
			slot.pTeam->ShowWindow( false );
			slot.pColour->Enable( false );
			slot.pColour->ShowWindow( false );
			slot.pAccept->Enable( false );
			slot.pAccept->ShowWindow( false );

			if ( bHost )
			{
				slot.pPlayerName->ShowWindow( false );
				slot.pPlayerCombo->ShowWindow( true );
				slot.pPlayerCombo->SetViewer( new CTextDataViewer() );
				slot.pPlayerCombo->AddItem( new CTextData( pScreen->GetTextEntry( "COMBO_OPEN" ) ) );
				slot.pPlayerCombo->AddItem( new CTextData( pScreen->GetTextEntry( "COMBO_CLOSE" ) ) );
				slot.pPlayerCombo->Select( 0 );
				slot.pPlayerCombo->Enable( true );
			}
		}
	}

	// Hide "waiting"
	pWaitMessage->ShowWindow( false );
	pChatInput->SetFocus( true );
	bConnected = true;

	if ( pMsg->bHost )
		pButtonBeginGame->ShowWindow( true );

	return true;
}

bool CInterfaceMPGameRoom::OnUpdateSlotMessage( SMPUIUpdateSlotMessage *pMsg )
{
	NI_VERIFY( pMsg->nSlot < slots.size(), "PRG: nSlot passed exceeds max", return true );

	slots[pMsg->nSlot].info = pMsg->info;

	UpdateInterior();
	CheckStartConditions();

	return true;
}

#define PING_GOOD 200
#define PING_AVG 400
void CInterfaceMPGameRoom::UpdateInterior()
{	
	// Rebuild player list
	for ( int i = 0; i < slots.size(); ++i )
	{
		SUISlot &slot = slots[i];

		slot.pPlayerNameText->SetText( slot.pPlayerNameText->GetDBText() + NStr::ToUnicode( slot.info.szName ) );
		if ( slot.info.bRandomCountry )
			slot.pCountry->Select( 0 );
		else
			slot.pCountry->Select( slot.info.nCountry + 1 );
		slot.pTeam->SetState( slot.info.nTeam );
		slot.pColour->Select( slot.info.nColour );
		slot.pAccept->SetState( slot.info.bAccept ? 1 : 0 );

		// Hide unused slots
		//slot.pPlayerName->ShowWindow( slot.info.bPresent );
		slot.pCountry->ShowWindow( slot.info.bPresent );
		slot.pTeam->ShowWindow( slot.info.bPresent );
		slot.pColour->ShowWindow( slot.info.bPresent );
		slot.pAccept->ShowWindow( slot.info.bPresent );

		slot.pPing->ShowWindow( i != nOwnSlot );
		wstring wszPingTooltip = GetScreen()->GetTextEntry( "PING_PREFIX" );

		if ( !slot.info.bPresent )
		{
			if ( slot.info.bAccept )
				slot.pPlayerNameText->SetText( slot.pPlayerNameText->GetDBText() + pScreen->GetTextEntry( "COMBO_CLOSE" ) );
			else
				slot.pPlayerNameText->SetText( slot.pPlayerNameText->GetDBText() + pScreen->GetTextEntry( "COMBO_OPEN" ) );

			if ( bHost )
				slot.pPlayerCombo->Select( ( slot.info.bAccept ) ? 1 : 0 );

			slot.pPing->SetState( 0 );
			wszPingTooltip += GetScreen()->GetTextEntry( "PING_NA" );
		}
		else
		{
			slot.pPlayerCombo->SetLine( new CTextData( NStr::ToUnicode( slot.info.szName ) ) );

			if ( slot.info.nPing < 0 )
			{
				slot.pPing->SetState( 1 );
				wszPingTooltip += GetScreen()->GetTextEntry( "PING_NO_REPLY" );
			}
			else 
			{
				wszPingTooltip += NStr::ToUnicode( StrFmt( " %d ", slot.info.nPing ) );
				if ( slot.info.nPing < PING_GOOD )
				{
					slot.pPing->SetState( 4 );
					wszPingTooltip += GetScreen()->GetTextEntry( "PING_GOOD" );
				}
				else if ( slot.info.nPing < PING_AVG )
				{
					slot.pPing->SetState( 3 );
					wszPingTooltip += GetScreen()->GetTextEntry( "PING_AVG" );
				}
				else
				{
					slot.pPing->SetState( 2 );
					wszPingTooltip += GetScreen()->GetTextEntry( "PING_BAD" );
				}
			}
		}
		slot.pPing->SetTooltip( wszPingTooltip );
	}

	const NDb::SMapInfo *pMapInfo = gameDesc.pMPMap->pMap;
	if ( pMapInfo )
	{
		const NDb::SMaterial *pMaterial = pMapInfo->pMiniMap;
		CPtr<IMiniMap> pWindowMiniMap = GetChildChecked<IMiniMap>( pMain, "Minimap", true );
		if ( pWindowMiniMap )
		{
			pWindowMiniMap->SetLoadingMapParams( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE,
				pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE );
			pWindowMiniMap->SetMaterial( pMapInfo->pMiniMap );
			pWindowMiniMap->ShowWindow( true );
		}
	}
}

void CInterfaceMPGameRoom::CheckStartConditions()
{
	bool bCanBeginGame = bHost;
	int nTeams[2];
	nTeams[0] = 0;
	nTeams[1] = 0;
	int nColourMask = 0;
	enum { E_OK, E_SIDE, E_COLOUR, E_ACCEPT } eState = E_OK;

	for ( int i = 0; i < slots.size(); ++i )
	{
		SUISlot &slot = slots[i];

		if ( slot.info.bPresent )
			nTeams[slot.info.nTeam] += 1;

		if ( slot.info.bPresent && !slot.info.bAccept )
		{
			bCanBeginGame = false;
			if ( eState == E_OK )
				eState = E_ACCEPT;
		}
		
		if ( slot.info.bPresent )
		{
			int nSlotColourMask = 1UL << slot.info.nColour;
			if ( nColourMask & nSlotColourMask )
			{
				bCanBeginGame = false;
				eState = E_COLOUR;
			}
			nColourMask = nColourMask | nSlotColourMask;
		}
	}

	if ( !( nTeams[0] > 0 && nTeams[1] > 0 ) )
	{
		bCanBeginGame = false;
		eState = E_SIDE;
	}

	// CRAP
#ifndef _FINALRELEASE
	bCanBeginGame = bCanBeginGame || s_bMPAllowStartGame;
#endif //_FINALRELEASE
	pButtonBeginGame->Enable( bCanBeginGame );

	if ( pCantStartAccept )
		pCantStartAccept->ShowWindow( eState == E_ACCEPT );
	if ( pCantStartSide )
		pCantStartSide->ShowWindow( eState == E_SIDE );
	if ( pCantStartColour )
		pCantStartColour->ShowWindow( eState == E_COLOUR );
}

bool CInterfaceMPGameRoom::OnInterruptReaction()
{
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_INTERRUPT );

	// Return to games list
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MP_CUSTOM_GAME_MENU, "" );

	// Resume updating games list
	SMPUIGameListMessage *pMsg = new SMPUIGameListMessage( true );
	Singleton<IMPToUIManager>()->AddUIMessage( pMsg );

	return true;
};

void CInterfaceMPGameRoom::OpenCloseEmptySlot( const int nSlot, const bool bClosed )
{
	slots[nSlot].info.Clear();
	slots[nSlot].info.bAccept = bClosed;
	SendUpdateSlot( nSlot );
	CheckStartConditions();
}

void CInterfaceMPGameRoom::MsgOk( const SGameMessage &msg )
{
	// Kick
	OpenCloseEmptySlot( nRequestedSlotChangeIndex, bRequestedSlotChangeClosed );

	nRequestedSlotChangeIndex = -1;
	UpdateInterior();
}

void CInterfaceMPGameRoom::MsgCancel( const SGameMessage &msg )
{
	nRequestedSlotChangeIndex = -1;
	UpdateInterior();
}

static DWORD ConvertColor( const CVec3 &vColor )
{
	const int r = vColor.r * 255.0f;
	const int g = vColor.g * 255.0f;
	const int b = vColor.b * 255.0f;

	return 0xFF000000 + (( r & 0xFF ) << 16) + (( g & 0xFF ) << 8) + ( b & 0xFF );
}

#ifndef _SINGLE_DEMO
// CICMPGameRoom

void CICMPGameRoom::PreCreate()
{
}

void CICMPGameRoom::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMPGameRoom::Configure( const char *pszConfig )
{
	str = pszConfig;	
}

START_REGISTER(MPGameRoomCommands)

REGISTER_VAR_EX( "mp_allow_start_game", NGlobal::VarBoolHandler, &s_bMPAllowStartGame, 0, STORAGE_NONE );

FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x3219DAC0, CInterfaceMPGameRoom );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MP_GAME_ROOM, CICMPGameRoom );

#endif // _SINGLE_DEMO
