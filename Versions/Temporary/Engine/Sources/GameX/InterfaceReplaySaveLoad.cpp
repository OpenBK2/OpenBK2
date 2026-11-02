#include "stdafx.h"

#include "CoopPlayerInfo.h"
#include "DBMPConsts.h"
#include "GameRoomData.h"
#include "GameXClassIDs.h"
#include "GetConsts.h"
#include "InterfaceMisc.h"
#include "InterfaceReplaySaveLoad.h"
#include "Misc/StrProc.h"
#include "MPInterfaceData.h"
#include "SaveLoadHelper.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "Stats_B2_M1/Vis2AI.h"

#include "GameX_export.h"

#include <cmath>

#include <fmt/format.h>

// CInterfaceReplaySaveLoad

CInterfaceReplaySaveLoad::CInterfaceReplaySaveLoad()
	: CInterfaceScreenBase( "ReplaySaveLoad", "intermission" )
{
	AddObserver( "message_box_ok", &CInterfaceReplaySaveLoad::MsgOk );
	AddObserver( "message_box_cancel", &CInterfaceReplaySaveLoad::MsgCancel );
}

bool CInterfaceReplaySaveLoad::Init()
{
	if ( !CInterfaceScreenBase::Init() )
	{
		return false;
	}

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
	{
		pLoadBtn->ShowWindow( !bSaveMode );
	}
	if ( pSaveBtn )
	{
		pSaveBtn->ShowWindow( bSaveMode );
	}
	if ( pHeaderSaveView )
	{
		pHeaderSaveView->ShowWindow( bSaveMode );
	}
	if ( pHeaderLoadView )
	{
		pHeaderLoadView->ShowWindow( !bSaveMode );
	}
	if ( pNameEdit )
	{
		pNameEdit->ShowWindow( bSaveMode );
	}
	if ( pPlayerList )
	{
		pPlayerList->Enable( !bSaveMode );
	}
}

bool CInterfaceReplaySaveLoad::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "react_on_back" )
	{
		return OnBack();
	}
	if ( szReaction == "react_on_save" )
	{
		return OnSave();
	}
	if ( szReaction == "react_on_load" )
	{
		return OnLoad();
	}
	if ( szReaction == "react_on_delete" )
	{
		return OnDelete();
	}
	if ( szReaction == "react_on_select_replay" )
	{
		return OnSelect();
	}
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
	{
		pLoadBtn->Enable( false );
	}
	pSaveBtn = GetChildChecked<IButton>( pMain, "ButtonSave", true );
	pDeleteBtn = GetChildChecked<IButton>( pMain, "ButtonDelete", true );
	pReplayList = GetChildChecked<IScrollableContainer>( pMain, "ReplayList", true );
	pReplayItemTemplate = GetChildChecked<IWindow>( pReplayList, "ItemTemplate", true );
	if ( pReplayItemTemplate )
	{
		pReplayItemTemplate->ShowWindow( false );
	}

	pPlayerList = GetChildChecked<IScrollableContainer>( pMain, "PlayerList", true );
	pPlayerItemTemplate = GetChildChecked<IWindow>( pPlayerList, "ItemTemplate", true );
	if ( pPlayerItemTemplate )
	{
		pPlayerItemTemplate->ShowWindow( false );
	}

	pNameEdit = GetChildChecked<IEditLine>( pMain, "NameEdit", true );
	pHeaderSaveView = GetChildChecked<ITextView>( pMain, "HeaderSave", true );
	pHeaderLoadView = GetChildChecked<ITextView>( pMain, "HeaderLoad", true );

	pMinimapPanel = GetChildChecked<IWindow>( pMain, "MinimapPanel", true );
	pMinimapWnd = GetChildChecked<IMiniMap>( pMinimapPanel, "Minimap", true );
	pMinimapFlagTemplate = GetChildChecked<IWindow>( pMinimapPanel, "PlayerOnMapTeam", true );
	if ( pMinimapFlagTemplate )
	{
		pMinimapFlagTemplate->ShowWindow( false );
	}
	if ( pMinimapPanel )
	{
		// Older UI data keeps the name as a sibling of the marker instead of a child.
		pMinimapNameTemplate = pMinimapPanel->GetChild( "PlayerOnMapNameView", false );
		if ( !pMinimapNameTemplate )
		{
			pMinimapNameTemplate = pMinimapPanel->GetChild( "PlayerOnMapName", false );
		}
		if ( pMinimapNameTemplate )
		{
			pMinimapNameTemplate->ShowWindow( false );
		}
	}

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
	{
		return;
	}

	pReplayList->RemoveItems();
	pReplayList->Select( 0 );
	for ( int i = 0; i < replays.size(); ++i )
	{
		SReplayEntry &entry = replays[i];

		IWindow *pWnd = AddWindowCopy( pReplayList, pReplayItemTemplate );
		if ( !pWnd )
		{
			continue;
		}
		pWnd->ShowWindow( true );

		ITextView *pItemName = GetChildChecked<ITextView>( pWnd, "ItemName", true );
		ITextView *pItemDate = GetChildChecked<ITextView>( pWnd, "ItemDate", true );
		ITextView *pItemPlayers = GetChildChecked<ITextView>( pWnd, "ItemPlayers", true );
		if ( pItemName )
		{
			pItemName->SetText( pItemName->GetDBText() + NStr::ToUnicode( entry.szName ) );
		}
		std::wstring wszDate = NStr::ToUnicode( fmt::format( "{:02d}.{:02d}.{:04d}<br>{:02d}:{:02d}",
		                                                     entry.time.wDay,
		                                                     entry.time.wMonth,
		                                                     entry.time.wYear,
		                                                     entry.time.wHour,
		                                                     entry.time.wMinute ) );
		if ( pItemDate )
		{
			pItemDate->SetText( pItemDate->GetDBText() + wszDate );
		}

		// Keep this as the human count, matching lobby capacity. Authored AI players
		// are shown in the team preview but never occupy recorded network slots.
		int nNumPlayers = 0;
		for ( int nSlot = 0; nSlot < entry.info.slots.size(); ++nSlot )
		{
			if ( entry.info.slots[nSlot].bPresent )
			{
				++nNumPlayers;
			}
		}
		if ( pItemPlayers )
		{
			pItemPlayers->SetText( pItemPlayers->GetDBText() + NStr::ToUnicode( std::to_string( nNumPlayers ) ) );
		}

		entry.pItem = pWnd;
		pReplayList->PushBack( pWnd, true );
	}
}

void CInterfaceReplaySaveLoad::CleanSelectionInfo()
{
	if ( pPlayerList )
	{
		pPlayerList->RemoveItems();
	}

	if ( pMinimapPanel )
	{
		while ( IWindow *pWnd = pMinimapPanel->GetChild( "Dynamic_Slot", false ) )
		{
			pMinimapPanel->RemoveChild( pWnd );
		}
	}

	if ( pMinimapWnd )
	{
		pMinimapWnd->SetTexture( 0 );
	}
}

void CInterfaceReplaySaveLoad::MakeInfo( const SReplayEntry &entry )
{
	CleanSelectionInfo();
	MakeTeams( entry.info );
	MakeMinimap( entry.info );
}

void CInterfaceReplaySaveLoad::MakeTeams( const SMultiplayerReplayInfo &info )
{
	if ( !pPlayerItemTemplate || !pPlayerList )
	{
		return;
	}

	const int nWinTeam = info.nWinningSide == 1 ? 1 : 0;
	const bool bCooperative = info.nMultiplayerMode == 1;
	const bool bTeam0Playable = NGameX::FindReplayViewpoint( info, 0 ) >= 0;
	const bool bTeam1Playable = NGameX::FindReplayViewpoint( info, 1 ) >= 0;

	// Team zero is the only human viewpoint in COOP, including a human defeat.
	IWindow *pWnd = AddWindowCopy( pPlayerList, pPlayerItemTemplate );
	if ( !pWnd )
	{
		return;
	}

	AddPlayers( pWnd, ( nWinTeam == 0 ), 0, info );
	pPlayerList->PushBack( pWnd, bTeam0Playable );
	if ( bTeam0Playable && ( bCooperative || nWinTeam == 0 || !bTeam1Playable ) )
	{
		pPlayerList->Select( pWnd );
	}

	// The AI row remains visible but cannot be selected for playback.
	pWnd = AddWindowCopy( pPlayerList, pPlayerItemTemplate );
	if ( !pWnd )
	{
		return;
	}

	AddPlayers( pWnd, ( nWinTeam == 1 ), 1, info );
	pPlayerList->PushBack( pWnd, bTeam1Playable );
	if ( bTeam1Playable && ( nWinTeam == 1 || !bTeam0Playable ) )
	{
		pPlayerList->Select( pWnd );
	}
}

void CInterfaceReplaySaveLoad::AddPlayers( IWindow *pWnd, bool bWon, int nTeam, const SMultiplayerReplayInfo &info )
{
	ITextView *pHeaderWon = GetChildChecked<ITextView>( pWnd, "ItemHeaderWin", true );
	if ( pHeaderWon && !bWon )
	{
		pHeaderWon->ShowWindow( false );
	}
	ITextView *pHeaderLost = GetChildChecked<ITextView>( pWnd, "ItemHeaderLose", true );
	if ( pHeaderLost && bWon )
	{
		pHeaderLost->ShowWindow( false );
	}
	IWindow *pFlag0 = GetChildChecked<IWindow>( pWnd, "ItemFlag0", true );
	if ( pFlag0 )
	{
		pFlag0->ShowWindow( nTeam == 0 );
	}
	IWindow *pFlag1 = GetChildChecked<IWindow>( pWnd, "ItemFlag1", true );
	if ( pFlag1 )
	{
		pFlag1->ShowWindow( nTeam == 1 );
	}

	std::wstring wszNameList;
	const auto appendName = [&]( const std::wstring &name )
	{
		if ( !wszNameList.empty() )
		{
			wszNameList += L", ";
		}
		wszNameList += name;
	};
	for ( int i = 0; i < info.slots.size(); ++i )
	{
		const SMPSlot &slot = info.slots[i];
		if ( slot.nTeam != nTeam || !slot.bPresent )
		{
			continue;
		}

		appendName( NStr::ToUnicode( slot.szName ) );
	}
	if ( nTeam == 1 )
	{
		for ( const auto &player : NGameX::GetCoopReplayPlayers( info ) )
		{
			appendName( player.wszName );
		}
	}

	ITextView *pPlayers = GetChildChecked<ITextView>( pWnd, "ItemPlayers", true );
	if ( pPlayers )
	{
		pPlayers->SetText( pPlayers->GetDBText() + wszNameList );
	}
}

void CInterfaceReplaySaveLoad::MakeMinimap( const SMultiplayerReplayInfo &info )
{
	const NDb::SMapInfo *pMap = info.pMap;
	if ( !pMap || !pMap->pMiniMap || !pMap->pMiniMap->pTexture || !pMinimapWnd || !pMinimapPanel ||
	     pMap->nNumPatchesX <= 0 || pMap->nNumPatchesY <= 0 )
	{
		return;
	}
	pMinimapWnd->SetLoadingMapParams( pMap->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE,
	                                  pMap->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE );
	pMinimapWnd->SetTexture( pMap->pMiniMap->pTexture );

	if ( !pMinimapFlagTemplate )
	{
		return;
	}

	int nMinimapX, nMinimapY;
	pMinimapWnd->GetPlacement( &nMinimapX, &nMinimapY, 0, 0 );

	const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();
	const auto addMarker =
			[&]( const CVec2 &vStartPos, const std::wstring &wszName, int nTeam, uint32_t nColor, const NDb::STexture *pIcon )
	{
		if ( !std::isfinite( vStartPos.x ) || !std::isfinite( vStartPos.y ) )
		{
			return;
		}
		const CVec2 vPos = pMinimapWnd->GetAIToScreen( vStartPos );

		IWindow *pWnd = AddWindowCopy( pMinimapPanel, pMinimapFlagTemplate );
		if ( !pWnd )
		{
			return;
		}
		pWnd->ShowWindow( true );
		pWnd->SetName( "Dynamic_Slot" );

		IWindow *pTeam1Wnd = pWnd->GetChild( "Team1", true );
		IWindow *pTeam2Wnd = pWnd->GetChild( "Team2", true );
		IWindow *pColourWnd = pWnd->GetChild( "TeamBlock", true );
		IWindow *pIconWnd = pWnd->GetChild( "Icon", true );
		ITextView *pNameView = dynamic_cast<ITextView *>( pWnd->GetChild( "Name", true ) );

		int nSizeX, nSizeY;
		pWnd->GetPlacement( 0, 0, &nSizeX, &nSizeY );
		pWnd->SetPlacement(
				nMinimapX + vPos.x - nSizeX * 0.5f, nMinimapY + vPos.y - nSizeY * 0.5f, 0, 0, EWPF_POS_X | EWPF_POS_Y );
		if ( !pNameView && pMinimapNameTemplate )
		{
			IWindow *pNameWnd = AddWindowCopy( pMinimapPanel, pMinimapNameTemplate );
			if ( pNameWnd )
			{
				pNameWnd->SetName( "Dynamic_Slot" );
				pNameWnd->ShowWindow( true );
				int nNameX, nNameY, nNameWidth;
				pNameWnd->GetPlacement( &nNameX, &nNameY, &nNameWidth, nullptr );
				pNameWnd->SetPlacement( nMinimapX + vPos.x - nNameWidth * 0.5f + nNameX,
				                        nMinimapY + vPos.y - nSizeY * 0.5f + nNameY,
				                        0,
				                        0,
				                        EWPF_POS_X | EWPF_POS_Y );
				pNameView = dynamic_cast<ITextView *>( pNameWnd );
				if ( !pNameView )
				{
					pNameView = dynamic_cast<ITextView *>( pNameWnd->GetChild( "Name", true ) );
				}
			}
		}

		CPtr<CColorBackground> pColourBgr = new CColorBackground();
		pColourBgr->nColor = nColor;
		if ( pTeam1Wnd )
		{
			pTeam1Wnd->ShowWindow( nTeam == 0 );
		}
		if ( pTeam2Wnd )
		{
			pTeam2Wnd->ShowWindow( nTeam == 1 );
		}
		if ( info.nMultiplayerMode == 1 && !pColourWnd )
		{
			// The legacy sibling-name layout has solid blue/red team blocks and
			// no individual-colour parent. Its human side can safely become green.
			IWindow *pTeamWnd = nTeam == 0 ? pTeam1Wnd : pTeam2Wnd;
			if ( pTeamWnd )
			{
				CPtr<CColorBackground> pTeamColour = new CColorBackground();
				pTeamColour->nColor = nTeam == 0 ? 0xff008040 : NGameX::COOP_BOT_COLOR;
				pTeamWnd->SetBackground( pTeamColour );
			}
		}
		// Team indicator textures retain their alpha mask, exposing the individual
		// colour on TeamBlock underneath instead of covering it with a solid rectangle.
		if ( pColourWnd )
		{
			pColourWnd->SetBackground( pColourBgr );
		}
		if ( pIconWnd )
		{
			pIconWnd->SetTexture( pIcon );
			pIconWnd->ShowWindow( pIcon != nullptr );
		}
		if ( pNameView )
		{
			std::wstring wszText = pNameView->GetDBText();
			if ( info.nMultiplayerMode == 1 && !pColourWnd )
			{
				// Native marker skins have only a team indicator; keep each human's
				// chosen individual colour visible in the sibling name instead.
				wszText += NStr::ToUnicode( fmt::format( "<color=0x{:X}>", nColor ) );
			}
			pNameView->SetText( wszText + wszName );
		}
	};
	for ( int i = 0; i < info.slots.size() && i < pMap->players.size(); ++i )
	{
		const SMPSlot &slot = info.slots[i];
		if ( !slot.bPresent || slot.nTeam < 0 || slot.nTeam > 1 )
		{
			continue;
		}
		uint32_t nColor = 0xffffffff;
		if ( pMPConsts && slot.nColour >= 0 && slot.nColour < pMPConsts->playerColorInfos.size() )
		{
			nColor = pMPConsts->playerColorInfos[slot.nColour].nColor | 0xFF000000;
		}
		const NDb::SMultiplayerSide *pSide = NGameX::GetMultiplayerSide( pMPConsts, slot.nCountry );
		const NDb::STexture *pIcon =
				pSide && pSide->pPartyInfo ? pSide->pPartyInfo->pMinimapKeyObjectIcon.GetPtr() : nullptr;
		addMarker( pMap->players[i].vMPStartPos, NStr::ToUnicode( slot.szName ), slot.nTeam, nColor, pIcon );
	}
	for ( const auto &player : NGameX::GetCoopReplayPlayers( info ) )
	{
		// Bot identity and position come from the recording, independently of later map edits.
		addMarker( player.vMPStartPos, player.wszName, 1, NGameX::COOP_BOT_COLOR, nullptr );
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
	{
		szNewName = NStr::ToMBCS( pNameEdit->GetText() );
	}

	if ( szNewName.length() == 0 )
	{
		szNewName = "Unnamed";
	}

	bool bResult = Singleton<IMPToUIManager>()->SaveReplay( szNewName );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	std::string szErrorCode = bResult ? "T_SAVE_SUCCESSFUL" : "T_SAVE_FAILED";
	NMainLoop::Command(
			ML_COMMAND_MESSAGE_BOX,
			CICMessageBox::MakeConfigString( "MessageBoxWindowOk", GetScreen()->GetTextEntry( szErrorCode ) ).c_str() );
}

bool CInterfaceReplaySaveLoad::OnSave()
{
	std::string szNewName;
	if ( pNameEdit )
	{
		szNewName = NStr::ToMBCS( pNameEdit->GetText() );
	}

	if ( szNewName.length() == 0 )
	{
		szNewName = "Unnamed";
	}

	for ( int i = 0; i < replays.size(); ++i )
	{
		if ( replays[i].szName == szNewName )
		{
			eQuestion = EQM_OVERWRITE;
			NMainLoop::Command( ML_COMMAND_MESSAGE_BOX,
			                    CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel",
			                                                     GetScreen()->GetTextEntry( "T_OVERWRITE_QUESTION" ) )
			                        .c_str() );
			return true;
		}
	}

	DoSaveReplay();
	return true;
}

bool CInterfaceReplaySaveLoad::OnLoad()
{
	NI_VERIFY( nSelected >= 0 && nSelected < replays.size(), "PRG: No replay selected to load", return true );

	SReplayEntry &entry = replays[nSelected];
	IWindow *pSelectedTeam = pPlayerList ? pPlayerList->GetSelectedItem() : nullptr;
	const int nSelectedTeamEntry = pSelectedTeam ? pPlayerList->GetItemNumber( pSelectedTeam ) : 0;
	const int nSelectedTeam = entry.info.nMultiplayerMode != 1 && nSelectedTeamEntry == 1 ? 1 : 0;
	const int nPlayFor = NGameX::FindReplayViewpoint( entry.info, nSelectedTeam );
	if ( nPlayFor < 0 )
	{
		return true;
	}

	std::string szCmd = "replay " + entry.szName + fmt::format( " {}", nPlayFor );

	NMainLoop::Command( ML_COMMAND_CLEAR_INTERFACES, "" );
	NGlobal::ProcessCommand( NStr::ToUnicode( szCmd ) );
	return true;
}

bool CInterfaceReplaySaveLoad::OnDelete()
{
	if ( nSelected < 0 || nSelected >= replays.size() )
	{
		return true;
	}

	eQuestion = EQM_DELETE;
	NMainLoop::Command(
			ML_COMMAND_MESSAGE_BOX,
			CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", GetScreen()->GetTextEntry( "T_DELETE_QUESTION" ) )
					.c_str() );
	return true;
}

bool CInterfaceReplaySaveLoad::OnSelect()
{
	if ( !pReplayList )
	{
		return true;
	}
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
				{
					pNameEdit->SetText( NStr::ToUnicode( replays[i].szName ).c_str() );
				}
			}
			else
			{
				if ( pLoadBtn )
				{
					pLoadBtn->Enable( NGameX::FindReplayViewpoint( replays[i].info, 0 ) >= 0 ||
					                  NGameX::FindReplayViewpoint( replays[i].info, 1 ) >= 0 );
				}
			}
			if ( pDeleteBtn )
			{
				pDeleteBtn->Enable( true );
			}
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
	else if ( eQuestion == EQM_DELETE && nSelected >= 0 && nSelected < replays.size() )
	{
		NSaveLoad::DeleteReplay( replays[nSelected].szName );
		GetReplays();
		PopulateReplayList();
		CleanSelectionInfo();
		nSelected = -1;
		if ( pLoadBtn )
		{
			pLoadBtn->Enable( false );
		}
		if ( pDeleteBtn )
		{
			pDeleteBtn->Enable( false );
		}
	}
	eQuestion = EQM_NONE;
}

void CInterfaceReplaySaveLoad::MsgCancel( const SGameMessage &msg )
{
	eQuestion = EQM_NONE;
}

// CICInterfaceReplaySaveLoad

void CICInterfaceReplaySaveLoad::PreCreate() {}

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
