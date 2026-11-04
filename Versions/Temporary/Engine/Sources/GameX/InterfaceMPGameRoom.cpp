#include "stdafx.h"

#include "DBMPConsts.h"
#include "GameXClassIDs.h"
#include "GetConsts.h"
#include "Input/GameMessage.h"
#include "InterfaceMisc.h"
#include "InterfaceMPGameRoom.h"
#include "InterfaceState.h"
#include "MessageParts.h"
#include "Misc/2Darray.h"
#include "Misc/StrProc.h"
#include "Misc/Win32Random.h"
#include "MPCoopRules.h"
#include "MultiplayerCommandManager.h"
#include "port/time.h"
#include "SceneB2/Scene.h"
#include "Stats_B2_M1/IconsSet.h"
#include "System/Commands.h"
#include "System/Text.h"
#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "UI/SceneClassIDs.h"
#include "UI/UI.h"
#include "UI/UIFactory.h"
#include "UISpecificB2/DBUISpecificB2.h"

#include "GameX_export.h"

#include <algorithm>
#include <cstdint>

#include <fmt/format.h>
#include <zconf.h>

static bool s_bMPAllowStartGame;

namespace
{
std::wstring GetLobbyMessageFormat( const ITextView *pTextView )
{
	// Replacing a fixed message must retain its DB font, colour and alignment.
	const auto *pDesc = checked_cast<const NDb::SWindowTextView *>( pTextView->GetDesc() );
	if ( pDesc->pTextFormat && CHECK_TEXT_NOT_EMPTY_PRE( pDesc->pTextFormat->, FormatString ) )
	{
		return GET_TEXT_PRE( pDesc->pTextFormat->, FormatString );
	}
	if ( pDesc->pTextString && pDesc->pTextString->pShared &&
	     CHECK_TEXT_NOT_EMPTY_PRE( pDesc->pTextString->pShared->, FormatString ) )
	{
		return GET_TEXT_PRE( pDesc->pTextString->pShared->, FormatString );
	}
	return std::wstring();
}
} // namespace

// CInterfaceMPGameRoom

CInterfaceMPGameRoom::CInterfaceMPGameRoom()
	: CInterfaceMPScreenBase( "MPGameRoom", "game_room" )
{
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_CHAT_MESSAGE, SMPUIChatMessage, &CInterfaceMPGameRoom::OnChatMessage );
	REGISTER_MPUI_MESSAGE_HANDLER(
			EMUI_GAME_ROOM_INIT, SMPUIGameRoomInitMessage, &CInterfaceMPGameRoom::OnGameRoomInitMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_UPDATE_SLOT, SMPUIUpdateSlotMessage, &CInterfaceMPGameRoom::OnUpdateSlotMessage );
}

CInterfaceMPGameRoom::~CInterfaceMPGameRoom()
{
	if ( pScreen )
	{
		Scene()->RemoveScreen( pScreen );
	}
}

bool CInterfaceMPGameRoom::Init()
{
	if ( CInterfaceScreenBase::Init() == false )
	{
		return false;
	}

	RegisterObservers();

	// load screen
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	if ( AddUIScreen( pScreen, "MPGameRoom", this ) == false )
	{
		return false;
	}

	pMain = GetChildChecked<IWindow>( pScreen, "Main", true );
	NI_VERIFY( pMain, "No main window found", return false );
	SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );

	InitControls();
	SetControls();
	bConnected = false;
	pWaitMessage->ShowWindow( true );
	NWin32Random::Seed( GetCurrentTimeMilliseconds() );
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
	{
		pCantStartAccept->ShowWindow( false );
	}
	pCantStartColour = GetChildChecked<ITextView>( pMain, "CantStartColour", true );
	if ( pCantStartColour )
	{
		pCantStartColour->ShowWindow( false );
	}
	pCantStartSide = GetChildChecked<ITextView>( pMain, "CantStartSide", true );
	if ( pCantStartSide )
	{
		pCantStartSide->ShowWindow( false );
	}
}

void CInterfaceMPGameRoom::SetControls()
{
	if ( pButtonBeginGame )
	{
		pButtonBeginGame->ShowWindow( false );
		pButtonBeginGame->Enable( false );
	}
	if ( pWaitMessage )
	{
		pWaitMessage->ShowWindow( true );
	}
}

void CInterfaceMPGameRoom::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
}

bool CInterfaceMPGameRoom::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "react_on_back" )
	{
		return OnBackReaction();
	}

	if ( szReaction == "react_on_begin" )
	{
		return OnBeginGameReaction();
	}

	if ( szReaction == "react_on_accept" )
	{
		return OnAcceptGameReaction();
	}

	if ( szReaction == "react_on_reject" )
	{
		return OnRejectGameReaction();
	}

	if ( szReaction == "chat_enter" )
	{
		return OnChatEnter();
	}

	if ( szReaction == "chat_escape" )
	{
		return OnChatEscape();
	}

	if ( szReaction == "react_on_change_player_side" )
	{
		return OnChangeSideReaction();
	}

	if ( szReaction == "react_on_change_player_team" )
	{
		return OnChangeTeamReaction();
	}

	if ( szReaction == "react_on_change_player_color" )
	{
		return OnChangeColorReaction();
	}

	if ( szReaction == "reaction_stop_waiting" )
	{
		return OnInterruptReaction();
	}

	if ( szReaction == "react_on_change_slot_status" )
	{
		return OnPlayerCombo( szSender );
	}

	if ( szReaction == "react_on_open_advanced" )
	{
		return OnShowAdvanced( true );
	}

	if ( szReaction == "react_on_close_advanced" )
	{
		return OnShowAdvanced( false );
	}

	return false;
}

int CInterfaceMPGameRoom::SelectionIndexToNationIndex( int nSelectionIndex )
{
	const auto nations = NGameX::GetAllowedMultiplayerNations( gameDesc );
	return nSelectionIndex > 0 && nSelectionIndex <= nations.size() ? nations[nSelectionIndex - 1] : -1;
}

int CInterfaceMPGameRoom::NationIndexToSelectionIndex( int nCountryIndex )
{
	const auto nations = NGameX::GetAllowedMultiplayerNations( gameDesc );
	const auto pos = std::find( nations.begin(), nations.end(), nCountryIndex );
	return pos == nations.end() ? -1 : int( pos - nations.begin() );
}

bool CInterfaceMPGameRoom::OnPlayerCombo( const std::string &szSender )
{
	int nIndex = -1;
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( slots[i].pPlayerCombo && slots[i].pPlayerCombo->GetName() == szSender )
		{
			nIndex = i;
			break;
		}
	}
	NI_VERIFY( nIndex >= 0, fmt::format( "Combo change received from unknown control '{}'", szSender ), return true );
	SUISlot &slot = slots[nIndex];
	bool bWantClosed = ( slot.pPlayerCombo->GetSelectedIndex() == 1 );

	if ( slot.info.bPresent )
	{
		nRequestedSlotChangeIndex = nIndex;
		bRequestedSlotChangeClosed = bWantClosed;

		// Ask user
		NMainLoop::Command(
				ML_COMMAND_MESSAGE_BOX,
				CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", GetScreen()->GetTextEntry( "ASK_KICK_PLAYER" ) )
						.c_str() );

		return true;
	}

	if ( slot.info.bAccept != bWantClosed )
	{
		OpenCloseEmptySlot( nIndex, bWantClosed );
	}

	return true;
}

bool CInterfaceMPGameRoom::OnChangeSideReaction()
{
	// Invoked when OWN slot changes
	// Process country change
	SUISlot &slot = slots[nOwnSlot];
	int nIndex = slot.pCountry->GetSelectedIndex();
	if ( nIndex == 0 ) // Random
	{
		slot.info.bRandomCountry = true;
	}
	else
	{
		slot.info.nCountry = SelectionIndexToNationIndex( nIndex );
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
	slot.info.nTeam = NGameX::IsCoopGame( gameDesc ) ? 0 : slot.pTeam->GetState();

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

int CInterfaceMPGameRoom::Check( const std::string &szCheckName ) const
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
	{
		slot.pControlDisabler->ShowWindow( true );
	}

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
	slot.pCountry->Enable( NGameX::GetAllowedMultiplayerNations( gameDesc ).size() > 1 );
	slot.pTeam->Enable( !NGameX::IsCoopGame( gameDesc ) );
	if ( slot.pControlDisabler )
	{
		slot.pControlDisabler->ShowWindow( false );
	}

	SendUpdateSlot( nOwnSlot );
	CheckStartConditions();
	return true;
}

void CInterfaceMPGameRoom::SendUpdateSlot( const int nSlot )
{
	CPtr<SMPUIUpdateSlotMessage> pMsg = new SMPUIUpdateSlotMessage;
	if ( slots[nSlot].info.bRandomCountry )
	{
		// Display, Random and backend validation use the same map-filtered IDs.
		const auto validIndexes = NGameX::GetAllowedMultiplayerNations( gameDesc );
		if ( !validIndexes.empty() )
		{
			slots[nSlot].info.nCountry = validIndexes[NWin32Random::Random( validIndexes.size() )];
		}
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
		std::wstring wszText = pChatInput->GetText();
		if ( !wszText.empty() )
		{
			Singleton<IMPToUIManager>()->AddUIMessage( new SMPUIChatMessage( wszText ) );
			pChatInput->SetText( L"" );
		}
	}
	return true;
}

bool CInterfaceMPGameRoom::OnChatEscape()
{
	return true;
}

bool CInterfaceMPGameRoom::OnChatMessage( SMPUIChatMessage *pMsg )
{
	std::wstring wszText;
	if ( pMsg->szName.empty() )
	{
		wszText = pMsg->wszText;
	}
	else
	{
		wszText = L"<font size = 16pt outlinesize = 1 outlinecolor = black forcefontsize = 1>" +
		          NStr::ToUnicode( pMsg->szName ) + L": " + pMsg->wszText;
	}
	pChatWrapper->AddItem( wszText );

	// notify the user if the app is minimized or unfocused
	NWinFrame::FlashTaskbarIfInactive();

	return true;
}

void CInterfaceMPGameRoom::SetTimeLimit( int nTimeLimit )
{
	if ( !pTimeLimit )
	{
		return;
	}
	// COOP has no clock limit; keep the same DB styling as the CLASSIC value.
	const std::wstring wszText = NGameX::IsCoopGame( gameDesc )
	                                 ? L"<val MP_TIME_LIMIT_NONE>"
	                                 : std::to_wstring( nTimeLimit ) + L"<val TIME_MINUTES_SUFFIX>";
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
		std::string szReasonCode = "DISCONNECT_UNKNOWN";
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
			default:
				break;
		}
		std::wstring wszReason = GetScreen()->GetTextEntry( szReasonCode );
		// The reason texts are fixed sentences from the game data; the detail
		// says what actually failed. <br> is how UI text breaks lines.
		if ( !pMsg->szDetail.empty() )
		{
			wszReason += ( wszReason.empty() ? L"" : L"<br>" ) + NStr::ToUnicode( pMsg->szDetail );
		}
		if ( wszReason.length() > 0 )
		{
			NMainLoop::Command( ML_COMMAND_MESSAGE_BOX,
			                    CICMessageBox::MakeConfigString( "MessageBoxWindowOk", wszReason ).c_str() );
		}
		return true;
	}

	if ( bConnected )
	{
		return true;
	}

	bHost = pMsg->bHost;

	const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();

	// Store info
	pSessionName->SetText( pSessionName->GetDBText() + NStr::ToUnicode( pMsg->szSessionName ) );
	gameDesc = pMsg->specificInfo;
	const int nPlayers = NGameX::GetMultiplayerSlotCount( gameDesc );
	const auto nations = NGameX::GetAllowedMultiplayerNations( gameDesc );

	// Prepare info for slots (country list, colour list)
	//
	// Everything read out of gameDesc here arrived in the host's description of
	// the game a moment ago. This tech level indexes MultiplayerConsts' own list
	// rather than a side's, so IsSideAvailable does not cover it, and the map
	// reference is null if it named a record this client does not have.
	if ( gameDesc.nTechLevel >= 0 && gameDesc.nTechLevel < pMPConsts->techLevels.size() )
	{
		pTechLevel->SetText( pTechLevel->GetDBText() + GET_TEXT_PRE( pMPConsts->techLevels[gameDesc.nTechLevel]., Name ) );
	}
	pNumPlayers->SetText( pNumPlayers->GetDBText() + NStr::ToUnicode( std::to_string( gameDesc.nPlayers ) ) );
	if ( gameDesc.pMPMap )
	{
		pMapName->SetText( pMapName->GetDBText() + GET_TEXT_PRE( gameDesc.pMPMap->, MapName ) );
	}
	SetTimeLimit( gameDesc.nTimeLimit );
	pCaptureTime->SetText( pCaptureTime->GetDBText() +
	                       NStr::ToUnicode( fmt::format( "{} sec", gameDesc.nCaptureTime ) ) );
	pGameSpeed->SetText( pGameSpeed->GetDBText() + NStr::ToUnicode( std::to_string( gameDesc.nGameSpeed ) ) );
	pUnitExperience->SetState( gameDesc.bUnitExp ? 1 : 0 );

	IButton *pRandomPlacement = GetChildChecked<IButton>( pAdvancedPopup, "RandomPlacement", true );
	if ( pRandomPlacement )
	{
		pRandomPlacement->SetState( gameDesc.bRandomPlacement ? 1 : 0 );
	}

	// Set up slots
	slots.resize( nPlayers );
	nOwnSlot = pMsg->nOwnSlot;
	pList->RemoveItems();
	for ( int i = 0; i < nPlayers; ++i )
	{
		SUISlot &slot = slots[i];
		if ( !NGameX::IsHumanMultiplayerSlot( gameDesc, i ) )
		{
			slot.info.bAccept = true;
			continue;
		}

		// Set up list items
		IWindow *pWnd = AddWindowCopy( pList, pSlotTemplate );
		pWnd->ShowWindow( true );
		pList->PushBack( pWnd, true );
		slot.pPlayerName = GetChildChecked<IWindow>( pWnd, "PlayerName", true );
		slot.pPlayerNameText = GetChildChecked<ITextView>( slot.pPlayerName, "PlayerNameText", true );
		slot.pPlayerName->SetName( fmt::format( "Name{}", i ) );
		slot.pCountry = GetChildChecked<IComboBox>( pWnd, "Country", true );
		slot.pCountry->SetName( fmt::format( "Country{}", i ) );

		//Set viewer
		slot.pCountry->SetViewer( new CTextureViewer() );
		CPtr<CTextureData> pRandomItem = new CTextureData( pMPConsts->pRandomCountryIcon,
		                                                   GetScreen()->GetTextEntry( "TT_RANDOM_COUNTRY" ),
		                                                   NGameX::GetRandomCountryImage( pMPConsts ) );
		slot.pCountry->AddItem( pRandomItem );
		for ( int j : nations )
		{
			CPtr<CTextureData> pData = new CTextureData( pMPConsts->sides[j].pListItemIcon,
			                                             GET_TEXT_PRE( pMPConsts->sides[j]., Name ),
			                                             NGameX::GetSideFlagImage( pMPConsts, j, gameDesc.nTechLevel ) );
			slot.pCountry->AddItem( pData );
		}

		slot.pTeam = GetChildChecked<IButton>( pWnd, "Team", true );
		slot.pTeam->SetName( fmt::format( "Team{}", i ) );
		slot.pColour = GetChildChecked<IComboBox>( pWnd, "Colour", true );
		slot.pColour->SetName( fmt::format( "Colour{}", i ) );
		slot.pColour->SetViewer( new CColorViewer() );
		for ( int j = 0; j < pMPConsts->playerColorInfos.size(); ++j )
		{
			slot.pColour->AddItem( new CColorData( pMPConsts->playerColorInfos[j].nColor | 0xff000000 ) );
		}

		slot.pAccept = GetChildChecked<IButton>( pWnd, "Status", true );
		slot.pAccept->SetName( fmt::format( "Status{}", i ) );

		slot.pPlayerCombo = GetChildChecked<IComboBox>( pWnd, "PlayerCombo", true );
		slot.pPlayerCombo->ShowWindow( false );
		slot.pPlayerCombo->SetName( fmt::format( "PlayerCombo{}", i ) );

		slot.pPing = GetChildChecked<IButton>( pWnd, "Ping", true );

		slot.pControlDisabler = GetChildChecked<IWindow>( pWnd, "ControlDisabler", true );
		if ( slot.pControlDisabler )
		{
			slot.pControlDisabler->ShowWindow( false );
		}

		if ( i != nOwnSlot )
		{
			CPtr<IWindow> pShade = GetChildChecked<IWindow>( pWnd, "SlotShade", true );
			if ( pShade )
			{
				pShade->ShowWindow( true );
			}

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
	{
		pButtonBeginGame->ShowWindow( true );
	}

	return true;
}

bool CInterfaceMPGameRoom::OnUpdateSlotMessage( SMPUIUpdateSlotMessage *pMsg )
{
	NI_VERIFY( pMsg->nSlot >= 0 && pMsg->nSlot < slots.size(), "PRG: nSlot passed exceeds max", return true );
	// An authoritative update can reject a start request while peers change state.
	if ( IWindow *pPopup = GetChildChecked<IWindow>( pMain, "Initializing", false ) )
	{
		pPopup->ShowWindow( false );
	}

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
		if ( !slot.pPlayerNameText )
		{
			continue;
		}

		slot.pPlayerNameText->SetText( slot.pPlayerNameText->GetDBText() + NStr::ToUnicode( slot.info.szName ) );
		const auto nations = NGameX::GetAllowedMultiplayerNations( gameDesc );
		if ( nations.size() == 1 )
		{
			slot.info.nCountry = nations.front();
			slot.info.bRandomCountry = false;
			slot.pCountry->Enable( false );
		}
		if ( slot.info.bRandomCountry )
		{
			slot.pCountry->Select( 0 );
		}
		else
		{
			slot.pCountry->Select( NationIndexToSelectionIndex( slot.info.nCountry ) + 1 );
		}
		slot.pTeam->SetState( slot.info.nTeam );
		slot.pColour->Select( slot.info.nColour );
		slot.pAccept->SetState( slot.info.bAccept ? 1 : 0 );

		// Hide unused slots
		//slot.pPlayerName->ShowWindow( slot.info.bPresent );
		slot.pCountry->ShowWindow( slot.info.bPresent );
		slot.pTeam->ShowWindow( slot.info.bPresent && !NGameX::IsCoopGame( gameDesc ) );
		slot.pColour->ShowWindow( slot.info.bPresent );
		slot.pAccept->ShowWindow( slot.info.bPresent );

		slot.pPing->ShowWindow( i != nOwnSlot );
		std::wstring wszPingTooltip = GetScreen()->GetTextEntry( "PING_PREFIX" );

		if ( !slot.info.bPresent )
		{
			if ( slot.info.bAccept )
			{
				slot.pPlayerNameText->SetText( slot.pPlayerNameText->GetDBText() + pScreen->GetTextEntry( "COMBO_CLOSE" ) );
			}
			else
			{
				slot.pPlayerNameText->SetText( slot.pPlayerNameText->GetDBText() + pScreen->GetTextEntry( "COMBO_OPEN" ) );
			}

			if ( bHost )
			{
				slot.pPlayerCombo->Select( ( slot.info.bAccept ) ? 1 : 0 );
			}

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
				wszPingTooltip += NStr::ToUnicode( fmt::format( " {} ", slot.info.nPing ) );
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
	int nColourMask = 0;
	int nPresentPlayers = 0;
	int nTeamMask = 0;

	enum
	{
		E_OK,
		E_SIDE,
		E_COLOUR,
		E_ACCEPT
	} eState = E_OK;

	for ( int i = 0; i < slots.size(); ++i )
	{
		SUISlot &slot = slots[i];

		if ( slot.info.bPresent && !slot.info.bAccept )
		{
			bCanBeginGame = false;
			if ( eState == E_OK )
			{
				eState = E_ACCEPT;
			}
		}

		if ( slot.info.bPresent )
		{
			++nPresentPlayers;
			if ( slot.info.nTeam >= 0 && slot.info.nTeam < 2 )
			{
				nTeamMask |= 1 << slot.info.nTeam;
			}
			if ( slot.info.nColour < 0 || slot.info.nColour >= 32 )
			{
				bCanBeginGame = false;
				eState = E_COLOUR;
				continue;
			}
			int nSlotColourMask = 1UL << slot.info.nColour;
			if ( nColourMask & nSlotColourMask )
			{
				bCanBeginGame = false;
				eState = E_COLOUR;
			}
			nColourMask = nColourMask | nSlotColourMask;
		}
	}

	std::vector<SMPSlot> lobbySlots;
	for ( const auto &slot : slots )
	{
		lobbySlots.push_back( slot.info );
	}
	std::string szRulesError;
	if ( !NGameX::ValidateMultiplayerLobby( gameDesc, lobbySlots, &szRulesError ) )
	{
		bCanBeginGame = false;
		if ( eState == E_OK )
		{
			eState = E_SIDE;
			if ( pCantStartSide )
			{
				const bool bCoop = NGameX::IsCoopGame( gameDesc );
				const bool bMissingPlayers = nPresentPlayers < 2 || ( !bCoop && nTeamMask != 3 );
				if ( bMissingPlayers && !bCoop )
				{
					// Keep the original screen translation as a backup for older data.
					const std::wstring &wszLegacy = pCantStartSide->GetDBText();
					if ( !NGameX::FindMessagePart( NGameX::GetUIConsts(), L"CANT_START_SAME_TEAM" ).empty() || wszLegacy.empty() )
					{
						pCantStartSide->SetText( GetLobbyMessageFormat( pCantStartSide ) + L"<val CANT_START_SAME_TEAM>" );
					}
					else
					{
						pCantStartSide->SetText( wszLegacy );
					}
				}
				else
				{
					std::wstring wszMessage = NStr::ToUnicode( szRulesError );
					if ( bMissingPlayers )
					{
						wszMessage = GetScreen()->GetTextEntry( "CANT_START_COOP_PLAYERS" );
						if ( wszMessage.empty() ||
						     !NGameX::FindMessagePart( NGameX::GetUIConsts(), L"CANT_START_COOP_PLAYERS" ).empty() )
						{
							wszMessage = L"<val CANT_START_COOP_PLAYERS>";
						}
					}
					pCantStartSide->SetText( GetLobbyMessageFormat( pCantStartSide ) + wszMessage );
				}
			}
		}
	}

	// CRAP
#ifndef _FINALRELEASE
	bCanBeginGame = bCanBeginGame || s_bMPAllowStartGame;
#endif //_FINALRELEASE
	pButtonBeginGame->Enable( bCanBeginGame );

	if ( pCantStartAccept )
	{
		pCantStartAccept->ShowWindow( eState == E_ACCEPT );
	}
	if ( pCantStartSide )
	{
		pCantStartSide->ShowWindow( eState == E_SIDE );
	}
	if ( pCantStartColour )
	{
		pCantStartColour->ShowWindow( eState == E_COLOUR );
	}
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
}

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

static uint32_t ConvertColor( const CVec3 &vColor )
{
	const int r = vColor.r * 255.0f;
	const int g = vColor.g * 255.0f;
	const int b = vColor.b * 255.0f;

	return 0xFF000000 + ( ( r & 0xFF ) << 16 ) + ( ( g & 0xFF ) << 8 ) + ( b & 0xFF );
}

#ifndef _SINGLE_DEMO
// CICMPGameRoom

void CICMPGameRoom::PreCreate() {}

void CICMPGameRoom::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMPGameRoom::Configure( const char *pszConfig )
{
	str = pszConfig;
}

START_REGISTER( MPGameRoomCommands )

REGISTER_VAR_EX( "mp_allow_start_game", NGlobal::VarBoolHandler, &s_bMPAllowStartGame, 0, STORAGE_NONE );

FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( GAMEX, 0x3219DAC0, CInterfaceMPGameRoom );
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_MP_GAME_ROOM, CICMPGameRoom );

#endif // _SINGLE_DEMO
