#include "stdafx.h"

#include "DBMPConsts.h"
#include "GameRoomData.h"
#include "GameXClassIDs.h"
#include "GetConsts.h"
#include "InterfaceMPStatistics.h"
#include "InterfaceState.h"
#include "Misc/StrProc.h"
#include "MultiplayerCommandManager.h"
#include "SaveLoadHelper.h"
#include "ScenarioTracker.h"
#include "SceneB2/Camera.h"
#include "SceneB2/FullScreenFader.h"
#include "SceneB2/Scene.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/Text.h"

#include "GameX_export.h"

#include <fmt/format.h>

const float UNFADE_TIME = 1.0f;

namespace NGameX
{
SMPStatisticsTeams BuildMultiplayerStatistics( IScenarioTracker *pTracker, const NDb::SMultiplayerConsts *pMPConsts )
{
	SMPStatisticsTeams teams;
	if ( !pTracker )
	{
		return teams;
	}
	const auto *pMPInfo = pTracker->GetMultiplayerInfo();
	if ( !pMPInfo )
	{
		return teams;
	}
	const bool bCooperative = pTracker->IsCooperative();
	const NDb::SMapInfo *pMap = pTracker->GetCurrentMission();
	if ( !pMap )
	{
		// MissionWin/MissionCancel already moved the map and calculated every side's scores.
		pMap = pTracker->GetLastMission();
	}
	int nWinningTeam = -1;
	for ( const auto &player : pMPInfo->players )
	{
		if ( player.nIndex == pTracker->GetLocalPlayer() )
		{
			teams.wszLocalPlayerName = player.wszName;
			nWinningTeam = player.nTeam;
			break;
		}
	}
	if ( bCooperative )
	{
		// The script result is independent of which human views the match or replay.
		nWinningTeam = pTracker->GetScriptOutcome();
		if ( nWinningTeam < 0 )
		{
			nWinningTeam = pTracker->IsMissionWon() ? 0 : 1;
		}
	}
	else if ( !pTracker->IsMissionWon() )
	{
		nWinningTeam = 1 - nWinningTeam;
	}
	if ( nWinningTeam < 0 || nWinningTeam > 1 )
	{
		return teams;
	}

	const auto addPlayer = [&]( int nPlayer, const std::wstring &wszName, int nCountry, int nTeam )
	{
		if ( nPlayer < 0 || nTeam < 0 || nTeam > 1 )
		{
			return;
		}
		SMPStatisticsPlayer item;
		item.wszName = wszName;
		item.nCountry = nCountry;
		item.dwColour = pTracker->GetPlayerColor( nPlayer ).dwColor;
		item.nTeam = nTeam;
		// Do not recalculate after mission end: the active map is already cleared.
		item.nUnitsLost = pTracker->GetStatistics( nPlayer, IScenarioTracker::ESK_UNITS_LOST );
		item.nUnitsKilled = pTracker->GetStatistics( nPlayer, IScenarioTracker::ESK_UNITS_KILLED );
		item.nScoreTactics = pTracker->GetStatistics( nPlayer, IScenarioTracker::ESK_TACTICAL_EFFICIENCY );
		item.nScoreStrategy = pTracker->GetStatistics( nPlayer, IScenarioTracker::ESK_STRATEGIC_EFFICIENCY );
		item.nScore = pTracker->GetStatistics( nPlayer, IScenarioTracker::ESK_SCORE );
		auto &team = nTeam == nWinningTeam ? teams.wonTeam : teams.lostTeam;
		auto position = team.begin();
		while ( position != team.end() && position->nScore > item.nScore )
		{
			++position;
		}
		team.insert( position, item );
	};

	for ( const auto &player : pMPInfo->players )
	{
		if ( bCooperative && pMap &&
		     ( player.nIndex < 0 || player.nIndex >= pMap->players.size() ||
		       pMap->players[player.nIndex].ePlayerType != NDb::HUMAN ||
		       pMap->players[player.nIndex].nDiplomacySide == 2 ) )
		{
			continue;
		}
		addPlayer( player.nIndex, player.wszName, player.nCountry, player.nTeam );
	}
	if ( bCooperative && pMap )
	{
		for ( int nPlayer = 0; nPlayer < pMap->players.size(); ++nPlayer )
		{
			const auto &player = pMap->players[nPlayer];
			if ( player.ePlayerType != NDb::AI_BOT || player.nDiplomacySide == 2 )
			{
				continue;
			}
			std::wstring wszName = NText::GetText( player.szLocalizedPlayerNameFileRef );
			if ( wszName.empty() )
			{
				wszName = L"<val AI_PLAYER_NAME_PREFIX>" + std::to_wstring( nPlayer + 1 );
			}
			int nCountry = -1;
			if ( pMPConsts && player.pPartyInfo )
			{
				for ( int i = 0; i < pMPConsts->sides.size(); ++i )
				{
					if ( pMPConsts->sides[i].pPartyInfo == player.pPartyInfo )
					{
						nCountry = i;
						break;
					}
				}
			}
			addPlayer( nPlayer, wszName, nCountry, pTracker->GetPlayerSide( nPlayer ) );
		}
	}
	return teams;
}
} // namespace NGameX

// CInterfaceMPStatistics

CInterfaceMPStatistics::CInterfaceMPStatistics()
	: CInterfaceMPScreenBase( "MPStatistics", "mp_statistics" )
{
	AddObserver( "menu_next", &CInterfaceMPStatistics::MsgNext );
	REGISTER_MPUI_MESSAGE_HANDLER(
			EMUI_GAME_AFTERMATH, SMPUIGameAftemathMessage, &CInterfaceMPStatistics::OnGameAftermathMessage );
}

bool CInterfaceMPStatistics::Init()
{
	if ( !CInterfaceScreenBase::Init() )
	{
		return false;
	}
	pMPConsts = NGameX::GetMPConsts();
	AddScreen( this );
	MakeInterior();
	return true;
}

void CInterfaceMPStatistics::MakeInterior()
{
	nNextMedal = 0;

	pMain = GetChildChecked<IWindow>( pScreen, "Main", true );
	SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );

	pHeaderWinView = GetChildChecked<ITextView>( pMain, "HeaderWin", true );
	pHeaderLostView = GetChildChecked<ITextView>( pMain, "HeaderLose", true );

	pNextBtn = GetChildChecked<IButton>( pMain, "NextBtn", true );
	pSaveReplayBtn = GetChildChecked<IButton>( pMain, "SaveReplayBtn", true );
	if ( pSaveReplayBtn && NGlobal::GetVar( "mp_replays_enabled", 0 ) )
	{
		pSaveReplayBtn->ShowWindow( true );
		pSaveReplayBtn->Enable( true );
	}

	if ( pHeaderWinView )
	{
		pHeaderWinView->ShowWindow( Singleton<IScenarioTracker>()->IsMissionWon() );
	}
	if ( pHeaderLostView )
	{
		pHeaderLostView->ShowWindow( !Singleton<IScenarioTracker>()->IsMissionWon() );
	}

	pPlayerNameView = GetChildChecked<ITextView>( pMain, "PlayerNameView", true );
	pMissionTimeView = GetChildChecked<ITextView>( pMain, "MissionTimeView", true );

	pPlayerList = GetChildChecked<IScrollableContainer>( pMain, "StatsList", true );
	pPlayerListDelimiter = GetChildChecked<IWindow>( pPlayerList, "ItemDelimiter", true );
	pPlayerListDelimiter->ShowWindow( false );
	pPlayerListItemTemplate = GetChildChecked<IWindow>( pPlayerList, "ItemTemplate", true );
	pPlayerListItemTemplate->ShowWindow( false );

	pPlayerRankLabel = GetChildChecked<ITextView>( pMain, "PlayerRankLabel", true );
	pPlayerRankView = GetChildChecked<ITextView>( pMain, "PlayerRankView", true );
	pPlayerLevelLabel = GetChildChecked<ITextView>( pMain, "PlayerLevelLabel", true );
	pPlayerLevelView = GetChildChecked<ITextView>( pMain, "PlayerLevelView", true );
	pExpEarnedLabel = GetChildChecked<ITextView>( pMain, "ExpEarnedLabel", true );
	pExpEarnedView = GetChildChecked<ITextView>( pMain, "ExpEarnedView", true );
	pExpTotalLabel = GetChildChecked<ITextView>( pMain, "TotalExpLabel", true );
	pExpTotalView = GetChildChecked<ITextView>( pMain, "TotalExpView", true );
	pExpTotalProgress = GetChildChecked<IProgressBar>( pMain, "TotalExpProgress", true );

	if ( pMissionTimeView )
	{
		int nTime = Singleton<IScenarioTracker>()->GetStatistics( Singleton<IScenarioTracker>()->GetLocalPlayer(),
		                                                          IScenarioTracker::ESK_TIME );
		std::string szText = fmt::format( "{:02d}", nTime % 60 );
		nTime = nTime / 60;
		szText = fmt::format( "{:02d}:", nTime % 60 ) + szText;
		nTime = nTime / 60;
		szText = std::to_string( nTime ) + szText;
		pMissionTimeView->SetText( pMissionTimeView->GetDBText() + NStr::ToUnicode( szText ) );
	}

	FillTeams();
	PopulateList();

	pWaiting = GetChildChecked<IWindow>( pMain, "WaitingWindow", true );
	pWaiting->ShowWindow( true );

	pMedalPopup = GetChildChecked<IWindow>( pMain, "MedalPopup", true );
	pRankPopup = GetChildChecked<IWindow>( pMain, "RankPopup", true );
	pMedalPopup->ShowWindow( false );
	pRankPopup->ShowWindow( false );

	IFullScreenFader *pFader = Scene()->GetScreenFader();
	if ( pFader )
	{
		pFader->Start( UNFADE_TIME, SCREEN_FADER_CLEAR, SCREEN_FADER_CLEAR, true );
	}
}

void CInterfaceMPStatistics::FillTeams()
{
	const auto teams = NGameX::BuildMultiplayerStatistics( Singleton<IScenarioTracker>(), pMPConsts );
	wonTeam = teams.wonTeam;
	lostTeam = teams.lostTeam;
	if ( pPlayerNameView )
	{
		pPlayerNameView->SetText( pPlayerNameView->GetDBText() + teams.wszLocalPlayerName );
	}
}

void CInterfaceMPStatistics::PopulateList()
{
	// Winning team
	IWindow *pDelim = AddWindowCopy( pPlayerList, pPlayerListDelimiter );
	pDelim->ShowWindow( true );
	ITextView *pDelimTextWon = GetChildChecked<ITextView>( pDelim, "DelimiterWin", true );
	ITextView *pDelimTextLost = GetChildChecked<ITextView>( pDelim, "DelimiterLose", true );
	pDelimTextWon->ShowWindow( true );
	pDelimTextLost->ShowWindow( false );
	pPlayerList->PushBack( pDelim, false );

	AddPlayerItemsToList( wonTeam );

	// Losing team
	pDelim = AddWindowCopy( pPlayerList, pPlayerListDelimiter );
	pDelim->ShowWindow( true );
	pDelimTextWon = GetChildChecked<ITextView>( pDelim, "DelimiterWin", true );
	pDelimTextLost = GetChildChecked<ITextView>( pDelim, "DelimiterLose", true );
	pDelimTextWon->ShowWindow( false );
	pDelimTextLost->ShowWindow( true );
	pPlayerList->PushBack( pDelim, false );

	AddPlayerItemsToList( lostTeam );
}

void CInterfaceMPStatistics::AddPlayerItemsToList( std::list<SPlayerItemData> &team )
{
	for ( std::list<SPlayerItemData>::iterator it = team.begin(); it != team.end(); ++it )
	{
		SPlayerItemData &item = *it;

		IWindow *pItemWnd = AddWindowCopy( pPlayerList, pPlayerListItemTemplate );
		pItemWnd->ShowWindow( true );

		ITextView *pItemName = GetChildChecked<ITextView>( pItemWnd, "ItemName", true );
		IWindow *pItemCountry = GetChildChecked<IWindow>( pItemWnd, "ItemCountry", true );
		IWindow *pItemColour = GetChildChecked<IWindow>( pItemWnd, "ItemColour", true );
		IButton *pItemTeamColour = GetChildChecked<IButton>( pItemWnd, "ItemTeamColour", true );
		ITextView *pItemLost = GetChildChecked<ITextView>( pItemWnd, "ItemLosses", true );
		ITextView *pItemKilled = GetChildChecked<ITextView>( pItemWnd, "ItemKills", true );
		ITextView *pItemScore1 = GetChildChecked<ITextView>( pItemWnd, "ItemScore1", true );
		ITextView *pItemScore2 = GetChildChecked<ITextView>( pItemWnd, "ItemScore2", true );
		ITextView *pItemScoreTotal = GetChildChecked<ITextView>( pItemWnd, "ItemScoreTotal", true );

		pItemName->SetText( pItemName->GetDBText() + item.wszName );
		// nCountry travels with the player info and indexes a database array, so
		// nothing has checked it against that array's size. The party reference is
		// a second way to get null here: it stays null if it did not resolve.
		const NDb::SMultiplayerSide *pSide = NGameX::GetMultiplayerSide( pMPConsts, item.nCountry );
		if ( pSide && pSide->pPartyInfo )
		{
			pItemCountry->SetTexture( pSide->pPartyInfo->pMinimapKeyObjectIcon );
		}

		CPtr<CColorBackground> pColourBgr = new CColorBackground();
		pColourBgr->nColor = item.dwColour;
		pItemColour->SetBackground( pColourBgr );

		if ( pItemTeamColour )
		{
			pItemTeamColour->SetState( item.nTeam );
		}

		pItemLost->SetText( pItemLost->GetDBText() + NStr::ToUnicode( std::to_string( item.nUnitsLost ) ) );
		pItemKilled->SetText( pItemKilled->GetDBText() + NStr::ToUnicode( std::to_string( item.nUnitsKilled ) ) );
		pItemScore1->SetText( pItemScore1->GetDBText() + NStr::ToUnicode( fmt::format( "{}%", item.nScoreTactics ) ) );
		pItemScore2->SetText( pItemScore2->GetDBText() + NStr::ToUnicode( fmt::format( "{}%", item.nScoreStrategy ) ) );
		pItemScoreTotal->SetText( pItemScoreTotal->GetDBText() + NStr::ToUnicode( std::to_string( item.nScore ) ) );

		pPlayerList->PushBack( pItemWnd, false );
	}
}

bool CInterfaceMPStatistics::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "react_on_close" )
	{
		return OnClosePopup();
	}

	if ( szReaction == "react_on_save_replay" )
	{
		return OnSaveReplay();
	}

	return false;
}

int CInterfaceMPStatistics::Check( const std::string &szCheckName ) const
{
	return 0;
}

void CInterfaceMPStatistics::MsgNext( const SGameMessage &msg )
{
	NI_VERIFY( Singleton<IScenarioTracker>()->GetGameType() != IAIScenarioTracker::EGT_SINGLE,
	           "Wrong game type (multiplayer only allowed)",
	           return );

	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	if ( bWasLadderGame )
	{
		NMainLoop::Command( ML_COMMAND_MP_GAME_LOBBY, "" );
	}
	else
	{
		NMainLoop::Command( ML_COMMAND_MP_CUSTOM_GAME_MENU, "" );
	}
}

bool CInterfaceMPStatistics::OnSaveReplay()
{
	NMainLoop::Command( ML_COMMAND_REPLAY_SAVE_LOAD, "save" );
	return true;
}

bool CInterfaceMPStatistics::OnGameAftermathMessage( struct SMPUIGameAftemathMessage *pMsg )
{
	pWaiting->ShowWindow( false );
	bWasLadderGame = pMsg->bShowLadderInfo;

	pPlayerRankLabel->ShowWindow( bWasLadderGame );
	pPlayerRankView->ShowWindow( bWasLadderGame );
	pPlayerLevelLabel->ShowWindow( bWasLadderGame );
	pPlayerLevelView->ShowWindow( bWasLadderGame );
	pExpEarnedLabel->ShowWindow( bWasLadderGame );
	pExpEarnedView->ShowWindow( bWasLadderGame );
	pExpTotalLabel->ShowWindow( bWasLadderGame );
	pExpTotalView->ShowWindow( bWasLadderGame );
	pExpTotalProgress->ShowWindow( bWasLadderGame );

	if ( !bWasLadderGame )
	{
		return true;
	}

	// nCountry and nRank arrive from the ladder server and index database arrays.
	// Leave the rank line at its database text when they do not name a rank.
	if ( const NDb::SLadderRank *pRank = NGameX::GetLadderRank( pMPConsts, pMsg->nCountry, pMsg->nRank ) )
	{
		pPlayerRankView->SetText( pPlayerRankView->GetDBText() + GET_TEXT_PRE( pRank->, Name ) );
	}
	pPlayerLevelView->SetText( pPlayerLevelView->GetDBText() + NStr::ToUnicode( std::to_string( pMsg->nLevel ) ) );
	pExpEarnedView->SetText( pExpEarnedView->GetDBText() + NStr::ToUnicode( std::to_string( pMsg->nExpEarned ) ) );
	pExpTotalView->SetText( pExpTotalView->GetDBText() +
	                        NStr::ToUnicode( fmt::format( "{} / {}", pMsg->nExpTotal1, pMsg->nExpTotal2 ) ) );
	if ( pMsg->nExpTotal2 != 0 )
	{
		pExpTotalProgress->SetPosition( float( pMsg->nExpTotal1 ) / pMsg->nExpTotal2 );
	}
	else
	{
		pExpTotalProgress->SetPosition( 0.0f );
	}

	medals = pMsg->medals;
	nNextMedal = 0;

	if ( pMsg->nLevel != pMsg->nOldLevel )
	{
		ShowLevelPopup( pMsg->nLevel, pMsg->nOldLevel, pMsg->nCountry, pMsg->nRank, pMsg->nOldRank );
	}
	else if ( medals.size() > 0 )
	{
		ShowMedalPopup( medals[0] );
		++nNextMedal;
	}

	return true;
}

bool CInterfaceMPStatistics::OnClosePopup()
{
	pMedalPopup->ShowWindow( false );
	pRankPopup->ShowWindow( false );

	if ( nNextMedal >= medals.size() )
	{
		return true;
	}

	ShowMedalPopup( medals[nNextMedal] );
	++nNextMedal;

	return true;
}

void CInterfaceMPStatistics::ShowLevelPopup( int nLevel, int nLevelOld, int nCountry, int nRank, int nRankOld )
{
	if ( !pRankPopup )
	{
		return;
	}

	IWindow *pIcon = GetChildChecked<IWindow>( pRankPopup, "PopupPicture", true );
	ITextView *pCongrats = GetChildChecked<ITextView>( pRankPopup, "PopupCongratulations", true );
	ITextView *pBadNews = GetChildChecked<ITextView>( pRankPopup, "PopupBadNews", true );
	ITextView *pLevel = GetChildChecked<ITextView>( pRankPopup, "PopupLevel", true );
	ITextView *pRankUp = GetChildChecked<ITextView>( pRankPopup, "PopupRankUp", true );
	ITextView *pRankDown = GetChildChecked<ITextView>( pRankPopup, "PopupRankDown", true );
	ITextView *pRankSame = GetChildChecked<ITextView>( pRankPopup, "PopupRankSame", true );
	ITextView *pRank = GetChildChecked<ITextView>( pRankPopup, "PopupRank", true );

	pCongrats->ShowWindow( nLevel > nLevelOld );
	pBadNews->ShowWindow( nLevel < nLevelOld );
	pLevel->SetText( pLevel->GetDBText() + NStr::ToUnicode( std::to_string( nLevel ) ) );

	pRankUp->ShowWindow( nRank > nRankOld );
	pRankDown->ShowWindow( nRank < nRankOld );
	pRankSame->ShowWindow( nRank == nRankOld );
	// Same pair of server-supplied indices as OnGameAftermathMessage, which is
	// where this popup's arguments come from.
	if ( const NDb::SLadderRank *pRankInfo = NGameX::GetLadderRank( pMPConsts, nCountry, nRank ) )
	{
		pRank->SetText( pRank->GetDBText() + GET_TEXT_PRE( pRankInfo->, Name ) );
		pIcon->SetTexture( pRankInfo->pTexture );
	}

	pRankPopup->ShowWindow( true );
}

void CInterfaceMPStatistics::ShowMedalPopup( const NDb::SMedal *pMedal )
{
	if ( !pMedalPopup )
	{
		return;
	}
	NI_VERIFY( pMedal, "Null link to medal desc in MPConsts", return );

	IWindow *pIcon = GetChildChecked<IWindow>( pMedalPopup, "PopupPicture", true );
	ITextView *pName = GetChildChecked<ITextView>( pMedalPopup, "MedalName", true );
	ITextView *pDesc = GetChildChecked<ITextView>( pMedalPopup, "MedalDesc", true );
	pName->SetText( pName->GetDBText() + GET_TEXT_PRE( pMedal->, LocalizedName ) );
	pDesc->SetText( pDesc->GetDBText() + GET_TEXT_PRE( pMedal->, LocalizedDesc ) );
	pIcon->SetTexture( pMedal->pPictureTexture );

	pMedalPopup->ShowWindow( true );
}

// CICMPStatistics

void CICMPStatistics::PreCreate() {}

void CICMPStatistics::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMPStatistics::Configure( const char *pszConfig ) {}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x1719C300, CInterfaceMPStatistics )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_MP_STATISTICS, CICMPStatistics )
