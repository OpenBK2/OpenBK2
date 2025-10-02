#include "StdAfx.h"
#include "InterfaceMPStatistics.h"
#include "GameXClassIDs.h"
#include "ScenarioTracker.h"
#include "../Misc/StrProc.h"
#include "SaveLoadHelper.h"
#include "InterfaceState.h"
#include "../System/Text.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "MultiplayerCommandManager.h"
#include "GameRoomData.h"
#include "../SceneB2/Scene.h"
#include "../SceneB2/Camera.h"
#include "../SceneB2/FullScreenFader.h"

const float UNFADE_TIME = 1.0f;

// CInterfaceMPStatistics

CInterfaceMPStatistics::CInterfaceMPStatistics() :
	CInterfaceMPScreenBase( "MPStatistics", "mp_statistics" )
{
	AddObserver( "menu_next", &CInterfaceMPStatistics::MsgNext );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_GAME_AFTERMATH, SMPUIGameAftemathMessage, &CInterfaceMPStatistics::OnGameAftermathMessage );
}

bool CInterfaceMPStatistics::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;
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
		pHeaderWinView->ShowWindow( Singleton<IScenarioTracker>()->IsMissionWon() );
	if ( pHeaderLostView )
		pHeaderLostView->ShowWindow( !Singleton<IScenarioTracker>()->IsMissionWon() );

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
		int nTime = Singleton<IScenarioTracker>()->GetStatistics( Singleton<IScenarioTracker>()->GetLocalPlayer(), IScenarioTracker::ESK_TIME );
		string szText = StrFmt( "%02d", nTime % 60 );
		nTime = nTime / 60;
		szText = StrFmt( "%02d:", nTime % 60 ) + szText;
		nTime = nTime / 60;
		szText = StrFmt( "%d:", nTime ) + szText;
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
		pFader->Start( UNFADE_TIME, SCREEN_FADER_CLEAR, SCREEN_FADER_CLEAR, true );
}

void CInterfaceMPStatistics::FillTeams()
{
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
	const IScenarioTracker::SMultiplayerInfo *pMPInfo = pScenarioTracker->GetMultiplayerInfo();
	NI_VERIFY( pMPInfo, "Missing multiplayer info", return );

	const int nLocalPlayer = pScenarioTracker->GetLocalPlayer();
	int nWinningTeam = -1;
	int nLocalIndex = -1;
	for ( int i = 0; i < pMPInfo->players.size(); ++i )
	{
		if ( pMPInfo->players[i].nIndex == nLocalPlayer )
		{
			nWinningTeam = pMPInfo->players[i].nTeam;
			nLocalIndex = i;
			pPlayerNameView->SetText( pPlayerNameView->GetDBText() + pMPInfo->players[i].wszName );
			break;
		}
	}
	if ( !pScenarioTracker->IsMissionWon() )
		nWinningTeam = 1 - nWinningTeam;

	wonTeam.clear();
	lostTeam.clear();

	for ( int i = 0; i < pMPInfo->players.size(); ++i )
	{
		const IScenarioTracker::SMultiplayerInfo::SPlayer &player = pMPInfo->players[i];
		if ( player.nTeam == nWinningTeam )
			AddPlayerToTeam( player, wonTeam );
		else if ( player.nTeam == 1 - nWinningTeam )
			AddPlayerToTeam( player, lostTeam );
		else
			NI_ASSERT( 0, "Neutral player found" );
	}
}

void CInterfaceMPStatistics::AddPlayerToTeam( const IScenarioTracker::SMultiplayerInfo::SPlayer &player, list<SPlayerItemData> &team )
{
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
	SPlayerItemData item;

	// Fill the data
	item.wszName = player.wszName;
	item.nCountry = player.nCountry;
	item.dwColour = pScenarioTracker->GetPlayerColor( player.nIndex ).dwColor;
	item.nTeam = pScenarioTracker->GetPlayerSide( player.nIndex );
	item.nUnitsLost = pScenarioTracker->GetStatistics( player.nIndex, IScenarioTracker::ESK_UNITS_LOST );
	item.nUnitsKilled = pScenarioTracker->GetStatistics( player.nIndex, IScenarioTracker::ESK_UNITS_KILLED );
	item.nScoreTactics = pScenarioTracker->GetStatistics( player.nIndex, IScenarioTracker::ESK_TACTICAL_EFFICIENCY );
	item.nScoreStrategy = pScenarioTracker->GetStatistics( player.nIndex, IScenarioTracker::ESK_STRATEGIC_EFFICIENCY );
	item.nScore = pScenarioTracker->GetStatistics( player.nIndex, IScenarioTracker::ESK_SCORE );

	// Insert it appropriately
	list<SPlayerItemData>::iterator it = team.begin();

	while ( it != team.end() && (*it).nScore > item.nScore )
		++it;

	team.insert( it, item );
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

void CInterfaceMPStatistics::AddPlayerItemsToList( list<SPlayerItemData> &team )
{
	for ( list<SPlayerItemData>::iterator it = team.begin(); it != team.end(); ++it )
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
		pItemCountry->SetTexture( pMPConsts->sides[item.nCountry].pPartyInfo->pMinimapKeyObjectIcon );

		CPtr<CColorBackground> pColourBgr = new CColorBackground();
		pColourBgr->nColor = item.dwColour;
		pItemColour->SetBackground( pColourBgr );

		if ( pItemTeamColour )
			pItemTeamColour->SetState( item.nTeam );

		pItemLost->SetText( pItemLost->GetDBText() + NStr::ToUnicode( StrFmt( "%d", item.nUnitsLost ) ) );
		pItemKilled->SetText( pItemKilled->GetDBText() + NStr::ToUnicode( StrFmt( "%d", item.nUnitsKilled ) ) );
		pItemScore1->SetText( pItemScore1->GetDBText() + NStr::ToUnicode( StrFmt( "%d%%", item.nScoreTactics ) ) );
		pItemScore2->SetText( pItemScore2->GetDBText() + NStr::ToUnicode( StrFmt( "%d%%", item.nScoreStrategy ) ) );
		pItemScoreTotal->SetText( pItemScoreTotal->GetDBText() + NStr::ToUnicode( StrFmt( "%d", item.nScore ) ) );

		pPlayerList->PushBack( pItemWnd, false );
	}
}

bool CInterfaceMPStatistics::Execute( const string &szSender, const string &szReaction )
{
	if ( szReaction == "react_on_close" )
		return OnClosePopup();

	if ( szReaction == "react_on_save_replay" )
		return OnSaveReplay();

	return false;
}

int CInterfaceMPStatistics::Check( const string &szCheckName ) const
{
	return 0;
}

void CInterfaceMPStatistics::MsgNext( const SGameMessage &msg )
{
	NI_VERIFY( Singleton<IScenarioTracker>()->GetGameType() != IAIScenarioTracker::EGT_SINGLE, "Wrong game type (multiplayer only allowed)", return );

	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	if ( bWasLadderGame )
		NMainLoop::Command( ML_COMMAND_MP_GAME_LOBBY, "" );
	else
		NMainLoop::Command( ML_COMMAND_MP_CUSTOM_GAME_MENU, "" );
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
		return true;

	pPlayerRankView->SetText( pPlayerRankView->GetDBText() +
		GET_TEXT_PRE( pMPConsts->sides[pMsg->nCountry].ladderRanks[pMsg->nRank]., Name ) );
	pPlayerLevelView->SetText( pPlayerLevelView->GetDBText() + NStr::ToUnicode( StrFmt( "%d", pMsg->nLevel ) ) );
	pExpEarnedView->SetText( pExpEarnedView->GetDBText() + NStr::ToUnicode( StrFmt( "%d", pMsg->nExpEarned ) ) );
	pExpTotalView->SetText( pExpTotalView->GetDBText() + NStr::ToUnicode( StrFmt( "%d / %d", pMsg->nExpTotal1, pMsg->nExpTotal2 ) ) );
	if ( pMsg->nExpTotal2 != 0 )
		pExpTotalProgress->SetPosition( float( pMsg->nExpTotal1 ) / pMsg->nExpTotal2 );
	else
		pExpTotalProgress->SetPosition( 0.0f );

	medals = pMsg->medals;
	nNextMedal = 0;

	if ( pMsg->nLevel != pMsg->nOldLevel )
		ShowLevelPopup( pMsg->nLevel, pMsg->nOldLevel, pMsg->nCountry, pMsg->nRank, pMsg->nOldRank );
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
		return true;

	ShowMedalPopup( medals[nNextMedal] );
	++nNextMedal;

	return true;
}

void CInterfaceMPStatistics::ShowLevelPopup( int nLevel, int nLevelOld, int nCountry, int nRank, int nRankOld )
{
	if ( !pRankPopup )
		return;

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
	pLevel->SetText( pLevel->GetDBText() + NStr::ToUnicode( StrFmt( "%d", nLevel ) ) );

	pRankUp->ShowWindow( nRank > nRankOld );
	pRankDown->ShowWindow( nRank < nRankOld );
	pRankSame->ShowWindow( nRank == nRankOld );
	const NDb::SLadderRank &rank = pMPConsts->sides[nCountry].ladderRanks[nRank];
	pRank->SetText( pRank->GetDBText() + GET_TEXT_PRE( rank., Name ) );
	pIcon->SetTexture( rank.pTexture );

	pRankPopup->ShowWindow( true );
}

void CInterfaceMPStatistics::ShowMedalPopup( const NDb::SMedal *pMedal )
{
	if ( !pMedalPopup )
		return;
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

void CICMPStatistics::PreCreate()
{
}

void CICMPStatistics::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMPStatistics::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x1719C300, CInterfaceMPStatistics )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MP_STATISTICS, CICMPStatistics )


