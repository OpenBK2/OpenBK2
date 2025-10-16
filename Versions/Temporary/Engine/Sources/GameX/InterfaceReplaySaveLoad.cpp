#include "stdafx.h"

#include "InterfaceReplaySaveLoad.h"
#include "GameXClassIDs.h"
#include "SaveLoadHelper.h"
#include "MPInterfaceData.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "GameRoomData.h"
#include "Misc/StrProc.h"
#include "InterfaceMisc.h"

#include "GameX_export.h"

#include <fmt/format.h>

// CInterfaceReplaySaveLoad

CInterfaceReplaySaveLoad::CInterfaceReplaySaveLoad() : CInterfaceScreenBase( "ReplaySaveLoad", "intermission" )
{
	AddObserver( "message_box_ok", &CInterfaceReplaySaveLoad::MsgOk );
	AddObserver( "message_box_cancel", &CInterfaceReplaySaveLoad::MsgCancel );
}

bool CInterfaceReplaySaveLoad::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );
	MakeInterior();
	eQuestion = EQM_NONE;
	return true;
}

void CInterfaceReplaySaveLoad::SetMode( const std::string &szMode )
{
	if ( szMode == "save" )
	{
		bSaveMode = true;
		ShowHideControls();
		return;
	}
	else if ( szMode == "load" )
	{
		bSaveMode = false;
		ShowHideControls();
		return;
	}
	NI_ASSERT( 0, fmt::format( "PRG: Wrong parameter ({}) passed to ReplaySaveLoad", szMode ) );
}

void CInterfaceReplaySaveLoad::ShowHideControls()
{
	if ( pLoadBtn )
		pLoadBtn->ShowWindow( !bSaveMode );
	if ( pSaveBtn )
		pSaveBtn->ShowWindow( bSaveMode );
	if ( pHeaderSaveView )
		pHeaderSaveView->ShowWindow( bSaveMode );
	if ( pHeaderLoadView )
		pHeaderLoadView->ShowWindow( !bSaveMode );
	if ( pNameEdit )
		pNameEdit->ShowWindow( bSaveMode );
	if ( pPlayerList )
		pPlayerList->Enable( !bSaveMode );
}

bool CInterfaceReplaySaveLoad::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "react_on_back" )
		return OnBack();
	if ( szReaction == "react_on_save" )
		return OnSave();
	if ( szReaction == "react_on_load" )
		return OnLoad();
	if ( szReaction == "react_on_delete" )
		return OnDelete();
	if ( szReaction == "react_on_select_replay" )
		return OnSelect();
	return false;
}

int CInterfaceReplaySaveLoad::Check( const std::string &szCheckName ) const
{
	return 0;
}

void CInterfaceReplaySaveLoad::MakeInterior()
{
	pMain = GetChildChecked<IWindow>( GetScreen(), "Main", true );
	pLoadBtn = GetChildChecked<IButton>( pMain, "ButtonLoad", true );
	if ( pLoadBtn )
		pLoadBtn->Enable( false );
	pSaveBtn = GetChildChecked<IButton>( pMain, "ButtonSave", true );
	pDeleteBtn = GetChildChecked<IButton>( pMain, "ButtonDelete", true );
	pReplayList = GetChildChecked<IScrollableContainer>( pMain, "ReplayList", true );
	pReplayItemTemplate = GetChildChecked<IWindow>( pReplayList, "ItemTemplate", true );
	if ( pReplayItemTemplate )
		pReplayItemTemplate->ShowWindow( false );

	pPlayerList = GetChildChecked<IScrollableContainer>( pMain, "PlayerList", true );
	pPlayerItemTemplate = GetChildChecked<IWindow>( pPlayerList, "ItemTemplate", true );
	if ( pPlayerItemTemplate )
		pPlayerItemTemplate->ShowWindow( false );

	pNameEdit = GetChildChecked<IEditLine>( pMain, "NameEdit", true );
	pHeaderSaveView = GetChildChecked<ITextView>( pMain, "HeaderSave", true );
	pHeaderLoadView = GetChildChecked<ITextView>( pMain, "HeaderLoad", true );

	pMinimapPanel = GetChildChecked<IWindow>( pMain, "MinimapPanel", true );
	pMinimapWnd = GetChildChecked<IMiniMap>( pMinimapPanel, "Minimap", true );
	pMinimapFlagTemplate = GetChildChecked<IWindow>( pMinimapPanel, "PlayerOnMapTeam", true );
	if ( pMinimapFlagTemplate )
		pMinimapFlagTemplate->ShowWindow( false );

	GetReplays();
	PopulateReplayList();
	nSelected = -1;
}

void CInterfaceReplaySaveLoad::GetReplays()
{
	NSaveLoad::CReplays srcReplays;
	NSaveLoad::GetReplayList( &srcReplays );

	replays.resize( srcReplays.size() );
	for ( int i = 0; i < srcReplays.size(); ++i )
	{
		NSaveLoad::SReplayInfo &src = srcReplays[i];
		SReplayEntry &dst = replays[i];

		dst.szName = src.szFileName;
		dst.time = src.timeFile;
		dst.info = src.replayInfo;
	}
}

void CInterfaceReplaySaveLoad::PopulateReplayList()
{
	if ( !pReplayItemTemplate || !pReplayList )
		return;

	pReplayList->RemoveItems();
	pReplayList->Select( 0 );
	for ( int i = 0; i < replays.size(); ++i )
	{
		SReplayEntry &entry = replays[i];

		IWindow *pWnd = AddWindowCopy( pReplayList, pReplayItemTemplate );
		if ( !pWnd )
			continue;
		pWnd->ShowWindow( true );

		ITextView *pItemName = GetChildChecked<ITextView>( pWnd, "ItemName", true );
		ITextView *pItemDate = GetChildChecked<ITextView>( pWnd, "ItemDate", true );
		ITextView *pItemPlayers = GetChildChecked<ITextView>( pWnd, "ItemPlayers", true );
		if ( pItemName )
			pItemName->SetText( pItemName->GetDBText() + NStr::ToUnicode( entry.szName ) );
		std::wstring wszDate = NStr::ToUnicode( fmt::format("{:02d}.{:02d}.{:04d}<br>{:02d}:{:02d}",
			entry.time.wDay, entry.time.wMonth, entry.time.wYear,
			entry.time.wHour, entry.time.wMinute ) );
		if ( pItemDate )
			pItemDate->SetText( pItemDate->GetDBText() + wszDate );

		int nNumPlayers = 0;
		for ( int i = 0; i < entry.info.slots.size(); ++i )
		{
			if ( entry.info.slots[i].bPresent )
				++nNumPlayers;
		}
		if ( pItemPlayers )
			pItemPlayers->SetText( pItemPlayers->GetDBText() + NStr::ToUnicode( std::to_string(  nNumPlayers ) ) );

		entry.pItem = pWnd;
		pReplayList->PushBack( pWnd, true );
	}
}

void CInterfaceReplaySaveLoad::CleanSelectionInfo()
{
	if ( pPlayerList )
		pPlayerList->RemoveItems();

	if ( pMinimapPanel )
	{
		while ( IWindow *pWnd = pMinimapPanel->GetChild( "Dynamic_Slot", false ) )
			pMinimapPanel->RemoveChild( pWnd );
	}

	if ( pMinimapWnd )
		pMinimapWnd->SetTexture( 0 );
}

void CInterfaceReplaySaveLoad::MakeInfo( const SReplayEntry &entry )
{
	CleanSelectionInfo();
	MakeTeams( entry.info );
	MakeMinimap( entry.info );
}

void CInterfaceReplaySaveLoad::MakeTeams( const SMultiplayerReplayInfo &info )
{
	if ( !pPlayerItemTemplate )
		return;

	const int nWinTeam = info.nWinningSide == 1 ? 1 : 0;

	// Winners
	IWindow *pWnd = AddWindowCopy( pPlayerList, pPlayerItemTemplate );
	if ( !pWnd )
		return;

	AddPlayers( pWnd, ( nWinTeam == 0 ), 0, info );
	pPlayerList->PushBack( pWnd, true );
	if ( nWinTeam == 0 ) 
		pPlayerList->Select( pWnd );

	// Losers
	pWnd = AddWindowCopy( pPlayerList, pPlayerItemTemplate );
	if ( !pWnd )
		return;

	AddPlayers( pWnd, ( nWinTeam == 1 ), 1, info );
	pPlayerList->PushBack( pWnd, true );
	if ( nWinTeam == 1 ) 
		pPlayerList->Select( pWnd );
}

void CInterfaceReplaySaveLoad::AddPlayers( IWindow *pWnd, bool bWon, int nTeam, const SMultiplayerReplayInfo &info )
{
	ITextView *pHeaderWon = GetChildChecked<ITextView>( pWnd, "ItemHeaderWin", true );
	if ( pHeaderWon && !bWon )
		pHeaderWon->ShowWindow( false );
	ITextView *pHeaderLost = GetChildChecked<ITextView>( pWnd, "ItemHeaderLose", true );
	if ( pHeaderLost && bWon )
		pHeaderLost->ShowWindow( false );
	IWindow *pFlag0 = GetChildChecked<IWindow>( pWnd, "ItemFlag0", true );
	if ( pFlag0 )
		pFlag0->ShowWindow( nTeam == 0 );
	IWindow *pFlag1 = GetChildChecked<IWindow>( pWnd, "ItemFlag1", true );
	if ( pFlag1 )
		pFlag1->ShowWindow( nTeam == 1 );

	std::string szNameList;
	for ( int i = 0; i < info.slots.size(); ++i )
	{
		const SMPSlot &slot = info.slots[i];
		if ( slot.nTeam != nTeam || !slot.bPresent )
			continue;

		if ( szNameList.length() > 0 )
			szNameList += ", ";

		szNameList += slot.szName;
	}

	ITextView *pPlayers = GetChildChecked<ITextView>( pWnd, "ItemPlayers", true );
	if ( pPlayers )
		pPlayers->SetText( pPlayers->GetDBText() + NStr::ToUnicode( szNameList ) );
}

void CInterfaceReplaySaveLoad::MakeMinimap( const SMultiplayerReplayInfo &info )
{
	const NDb::SMapInfo *pMap = info.pMap;
	if ( pMinimapWnd )
	{
		if ( pMap )
		{
			pMinimapWnd->SetLoadingMapParams( pMap->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE, pMap->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE );
			pMinimapWnd->SetTexture( pMap->pMiniMap->pTexture );
		}
		else
			return;
	}

	if ( !pMinimapFlagTemplate )
		return;

	int nMinimapX, nMinimapY;
	pMinimapWnd->GetPlacement( &nMinimapX, &nMinimapY, 0, 0 );

	const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();
	if ( !pMPConsts )
		return;

	for ( int i = 0; i < info.slots.size(); ++i )
	{
		const SMPSlot &slot = info.slots[i];

		if ( !slot.bPresent )
			continue;

		// Put on minimap
		CVec2 vPos = pMinimapWnd->GetAIToScreen( pMap->players[i].vMPStartPos );

		IWindow *pWnd = AddWindowCopy( pMinimapPanel, pMinimapFlagTemplate );
		if ( !pWnd )
			continue;
		pWnd->ShowWindow( true );
		pWnd->SetName( "Dynamic_Slot" );

		IWindow *pTeam1Wnd = GetChildChecked<IWindow>( pWnd, "Team1", true );
		IWindow *pTeam2Wnd = GetChildChecked<IWindow>( pWnd, "Team2", true );
		IWindow *pColourWnd = GetChildChecked<IWindow>( pWnd, "TeamBlock", true );
		IWindow *pIconWnd = GetChildChecked<IWindow>( pWnd, "Icon", true );
		ITextView *pNameView = GetChildChecked<ITextView>( pWnd, "Name", true );

		int nSizeX, nSizeY;
		pWnd->GetPlacement( 0, 0, &nSizeX, &nSizeY );
		pWnd->SetPlacement( nMinimapX + vPos.x - nSizeX * 0.5f, nMinimapY + vPos.y - nSizeY * 0.5f, 0, 0, EWPF_POS_X | EWPF_POS_Y );

		CPtr<CColorBackground> pColourBgr = new CColorBackground();
		pColourBgr->nColor = pMPConsts->playerColorInfos[slot.nColour].nColor | 0xFF000000;
		pTeam1Wnd->ShowWindow( slot.nTeam == 0 );
		pTeam2Wnd->ShowWindow( slot.nTeam == 1 );
		if ( pColourWnd )
			pColourWnd->SetBackground( pColourBgr );
		pIconWnd->SetTexture( pMPConsts->sides[slot.nCountry].pPartyInfo->pMinimapKeyObjectIcon );
		pNameView->SetText( pNameView->GetDBText() + NStr::ToUnicode( slot.szName ) );
	}
}

bool CInterfaceReplaySaveLoad::OnBack()
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	if ( !bSaveMode )
	{
		NMainLoop::Command( ML_COMMAND_MULTIPLAYER_MENU, "" );
	}
	return true;
}

void CInterfaceReplaySaveLoad::DoSaveReplay()
{
	std::string szNewName;
	if ( pNameEdit )
		szNewName = NStr::ToMBCS( pNameEdit->GetText() );

	if ( szNewName.length() == 0 ) 
		szNewName = "Unnamed";

	bool bResult = Singleton<IMPToUIManager>()->SaveReplay( szNewName );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	std::string szErrorCode = bResult ? "T_SAVE_SUCCESSFUL" : "T_SAVE_FAILED";
	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOk", GetScreen()->GetTextEntry( szErrorCode ) ).c_str() );

}

bool CInterfaceReplaySaveLoad::OnSave()
{
	std::string szNewName;
	if ( pNameEdit )
		szNewName = NStr::ToMBCS( pNameEdit->GetText() );

	if ( szNewName.length() == 0 ) 
		szNewName = "Unnamed";

	for ( int i = 0; i < replays.size(); ++i )
	{
		if ( replays[i].szName == szNewName )
		{
			eQuestion = EQM_OVERWRITE;
			NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
				CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
				GetScreen()->GetTextEntry( "T_OVERWRITE_QUESTION" ) ).c_str() );
			return true;
		}
	}

	DoSaveReplay();
	return true;
}

bool CInterfaceReplaySaveLoad::OnLoad()
{
	NI_VERIFY( nSelected > -1, "PRG: No replay selected to load", return true );

	SReplayEntry &entry = replays[nSelected];
	const int nSelectedTeamEntry = pPlayerList->GetItemNumber( pPlayerList->GetSelectedItem() );
	const int nSelectedTeam = ( nSelectedTeamEntry == 1 ) ? 1 : 0;
	int nPlayFor = 0;
	for ( int i = 0; i < entry.info.slots.size(); ++i )
	{
		if ( entry.info.slots[i].bPresent && entry.info.slots[i].nTeam == nSelectedTeam )
		{
			nPlayFor = i;
			break;
		}
	}

	std::string szCmd = "replay " + entry.szName + fmt::format( " {}", nPlayFor );

	NMainLoop::Command( ML_COMMAND_CLEAR_INTERFACES, "" );
	NGlobal::ProcessCommand( NStr::ToUnicode( szCmd ) );
	return true;
}

bool CInterfaceReplaySaveLoad::OnDelete()
{
	if ( nSelected < 0 )
		return true;

	eQuestion = EQM_DELETE;
	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
		GetScreen()->GetTextEntry( "T_DELETE_QUESTION" ) ).c_str() );
	return true;
}

bool CInterfaceReplaySaveLoad::OnSelect()
{
	IWindow *pSelected = pReplayList->GetSelectedItem();
	for ( int i = 0; i < replays.size(); ++i )
	{
		if ( replays[i].pItem == pSelected )
		{
			MakeInfo( replays[i] );
			nSelected = i;
			if ( bSaveMode )
			{
				if ( pNameEdit )
					pNameEdit->SetText( NStr::ToUnicode( replays[i].szName ).c_str() );
			}
			else
			{
				if ( pLoadBtn )
					pLoadBtn->Enable( true );
			}
			if ( pDeleteBtn )
				pDeleteBtn->Enable( true );
			break;
		}
	}
	return true;
}

void CInterfaceReplaySaveLoad::MsgOk( const SGameMessage &msg )
{
	if ( eQuestion == EQM_OVERWRITE )
	{
		DoSaveReplay();
	}
	else if ( eQuestion == EQM_DELETE )
	{
		NSaveLoad::DeleteReplay( replays[nSelected].szName );
		GetReplays();
		PopulateReplayList();
		CleanSelectionInfo();
		if ( pDeleteBtn )
			pDeleteBtn->Enable( false );
	}
	eQuestion = EQM_NONE;
}

void CInterfaceReplaySaveLoad::MsgCancel( const SGameMessage &msg )
{
	eQuestion = EQM_NONE;
}

// CICInterfaceReplaySaveLoad

void CICInterfaceReplaySaveLoad::PreCreate()
{
}

void CICInterfaceReplaySaveLoad::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
	pInterface->SetMode( szMode );
}

void CICInterfaceReplaySaveLoad::Configure( const char *pszConfig )
{
	szMode = pszConfig;
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x19288C40, CInterfaceReplaySaveLoad );
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_REPLAY_SAVE_LOAD, CICInterfaceReplaySaveLoad );

