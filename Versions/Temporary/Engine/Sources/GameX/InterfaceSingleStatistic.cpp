#include "stdafx.h"
#include "InterfaceSingleStatistic.h"
#include "GameXClassIDs.h"
#include "ScenarioTracker.h"
#include "Misc/StrProc.h"
#include "InterfaceState.h"
#include "System/Text.h"
#include "InterfaceChapterMapMenuDialogs.h"
#include "SceneB2/Scene.h"
#include "SceneB2/Camera.h"
#include "SceneB2/FullScreenFader.h"
#include "InterfaceMisc.h"
#include "SaveLoadHelper.h"
#include "Sound/MusicSystem.h"
#include "DBGameRoot.h"

#include "GameX_export.h"

#include <fmt/format.h>

const int PAUSE_BETWEEN_MEDALS = 200;
const float UNFADE_TIME = 1.0f;

const wchar_t* DYNAMIC_TAG_PLAYER_NAME = L"player_name";
const wchar_t* DYNAMIC_TAG_PLAYER_RANK = L"player_rank";

// CInterfaceSingleStatistic

CInterfaceSingleStatistic::CInterfaceSingleStatistic() :
	CInterfaceScreenBase( "SingleStatistics2", "single_statistic" ),
	timePrevPopup( 0 )
{
	AddObserver( "message_box_ok", &CInterfaceSingleStatistic::MsgOk );
	AddObserver( "message_box_cancel", &CInterfaceSingleStatistic::MsgCancel );
	AddObserver( "try_exit_windows", &CInterfaceSingleStatistic::MsgTryExitWindows );
}

bool CInterfaceSingleStatistic::Init()
{
	if ( Singleton<IScenarioTracker>()->IsTutorialMission() )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
		NMainLoop::Command( ML_COMMAND_SELECT_TUTORIAL, "" );
		InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );
		return true;
	}

	if ( !CInterfaceScreenBase::Init() )
		return false;
		
	AddScreen( this );

	MakeInterior();

	return true;
}

void CInterfaceSingleStatistic::MakeInterior()
{
	bPopup = false;

	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const int nLocalPlayer = pST->GetLocalPlayer();

	pMain = GetChildChecked<IWindow>( GetScreen(), "Main", true );
	pNewRankDlg = GetChildChecked<IWindow>( GetScreen(), "NewRankDlg", true );
	pNewMedalDlg = GetChildChecked<IWindow>( GetScreen(), "MedalDlg", true );
	pChapterReinfDescWnd = GetChildChecked<IWindow>( GetScreen(), "ReinfDescriptionBackground", false );
	pPopupBgWnd = GetChildChecked<IWindow>( GetScreen(), "DlgBg", false );
	pBlackBgWnd = GetChildChecked<IWindow>( GetScreen(), "BlackBg", true );
	if ( pNewRankDlg )
		pNewRankDlg->ShowWindow( false );
	if ( pNewMedalDlg )
		pNewMedalDlg->ShowWindow( false );
	if ( pChapterReinfDescWnd )
		pChapterReinfDescWnd->ShowWindow( false );
	if ( pPopupBgWnd )
		pPopupBgWnd->ShowWindow( false );
	if ( pBlackBgWnd )
		pBlackBgWnd->ShowWindow( false );

	pInfoPanel = GetChildChecked<IWindow>( pMain, "InfoPanel", true );
	pRewardPanel = GetChildChecked<IWindow>( pMain, "RewardPanel", true );
	pBottomPanel = GetChildChecked<IWindow>( pMain, "BottomPanel", true );
	
	pMissionSuccessLabel = GetChildChecked<ITextView>( pInfoPanel, "MissionSuccessLabel", true );
	pMissionFailedLabel = GetChildChecked<ITextView>( pInfoPanel, "MissionFailedLabel", true );
	pCareerProgress = GetChildChecked<IProgressBar>( pInfoPanel, "CareerProgress", true );
	pRankProgress = GetChildChecked<IProgressBar>( pInfoPanel, "ExpProgress", true );
	pNewCareerProgress = GetChildChecked<IProgressBar>( pInfoPanel, "NewCareerProgress", true );
	pNewRankProgress = GetChildChecked<IProgressBar>( pInfoPanel, "NewExpProgress", true );
	pExpView = GetChildChecked<ITextView>( pInfoPanel, "ExpForNextRankView", true );
	pMissionTimeView = GetChildChecked<ITextView>( pInfoPanel, "MissionTimeView", true );
	pCampaignTimeView = GetChildChecked<ITextView>( pInfoPanel, "TotalCampaignTimeView", true );
	if ( pMissionSuccessLabel )
		pMissionSuccessLabel->ShowWindow( false );
	if ( pMissionFailedLabel )
		pMissionFailedLabel->ShowWindow( false );
	
	if ( pInfoPanel )
	{
		for ( int i = 1; ; ++i )
		{
			IWindow *pWnd = pInfoPanel->GetChild( fmt::format( "Line{:02d}", i ), true );
			if ( !pWnd )
				break;
				
			pWnd->ShowWindow( false );
			
			SPlayer player;
			player.pWnd = pWnd;
			player.pIconWnd = GetChildChecked<IWindow>( pWnd, "Flag", true );
			player.pNameView = GetChildChecked<ITextView>( pWnd, "NameView", true );
			player.pLostView = GetChildChecked<ITextView>( pWnd, "UnitsLostView", true );
			player.pKilledView = GetChildChecked<ITextView>( pWnd, "UnitsKilledView", true );
			player.pReinfView = GetChildChecked<ITextView>( pWnd, "ResevedView", true );
			player.bHasStatistics = false;
			players.push_back( player );
		}
	}
	
	if ( pRewardPanel )
	{
		for ( int i = 1; ; ++i )
		{
			IWindow *pWnd = pRewardPanel->GetChild( fmt::format( "Line{:02d}", i ), true );
			if ( !pWnd )
				break;
				
			pWnd->ShowWindow( false );
			
			SReinf reinf;
			reinf.pWnd = pWnd;
			reinf.pBtn = GetChildChecked<IButton>( pWnd, "BlockBtn", true );
			reinf.pIconWnd = GetChildChecked<IWindow>( pWnd, "Icon", true );
			reinf.pNameView = GetChildChecked<ITextView>( pWnd, "NameView", true );
			reinf.szButtonName = fmt::format( "BlockBtn{:02d}", i );
			if ( reinf.pBtn )
				reinf.pBtn->SetName( reinf.szButtonName );
			reinfs.push_back( reinf );
		}
	}

	pNewRankPanel = GetChildChecked<IWindow>( pNewRankDlg, "RankPanel", true );
	pNewRankIconWnd = GetChildChecked<IWindow>( pNewRankPanel, "RankIcon", true );
	pNewRankView = GetChildChecked<ITextView>( pNewRankPanel, "RankView", true );

	pNewMedalPanel = GetChildChecked<IWindow>( pNewMedalDlg, "MedalPanel", true );
	pMedalNameView = GetChildChecked<ITextView>( pNewMedalPanel, "MedalNameView", true );
	pMedalIconWnd = GetChildChecked<IWindow>( pNewMedalPanel, "MedalIcon", true );
	pMedalDescCont = GetChildChecked<IScrollableContainer>( pNewMedalPanel, "MedalDescCont", true );
	pMedalDescView = GetChildChecked<ITextView>( pNewMedalPanel, "MedalDescView", true );
	
	pRestartBtn = GetChildChecked<IButton>( pBottomPanel, "RestartMissionBtn", true );
	pExitToMainMenuBtn = GetChildChecked<IButton>( pBottomPanel, "ExitToMainMenuBtn", true );
	pExitToChapterBtn = GetChildChecked<IButton>( pBottomPanel, "ExitToChapterBtn", true );
	pLoadBtn = GetChildChecked<IButton>( pBottomPanel, "LoadBtn", true );
//	pExitToWindowsBtn = GetChildChecked<IButton>( pBottomPanel, "ExitToWindowsBtn", true );
	pNextBtn = GetChildChecked<IButton>( pBottomPanel, "NextBtn", true );

	wszTime1 = GetScreen()->GetTextEntry( "T_TIME_1" );
	wszTime2 = GetScreen()->GetTextEntry( "T_TIME_2" );
	wszTime3 = GetScreen()->GetTextEntry( "T_TIME_3" );
	
	bool bMissionWon = pST->IsMissionWon();
	if ( pMissionSuccessLabel )
		pMissionSuccessLabel->ShowWindow( bMissionWon );
	if ( pMissionFailedLabel )
		pMissionFailedLabel->ShowWindow( !bMissionWon );

	bool bCustomMission = pST->IsCustomMission();
	if ( pNextBtn )
		pNextBtn->ShowWindow( bMissionWon );
	if ( pRestartBtn )
		pRestartBtn->ShowWindow( !bMissionWon );
	if ( pExitToMainMenuBtn )
		pExitToMainMenuBtn->ShowWindow( !bMissionWon && bCustomMission );
	if ( pExitToChapterBtn )
		pExitToChapterBtn->ShowWindow( !bMissionWon && !bCustomMission );
	if ( pLoadBtn )
		pLoadBtn->ShowWindow( !bMissionWon );
//	if ( pExitToWindowsBtn )
//		pExitToWindowsBtn->ShowWindow( false );
		
	pCareerExpNAView = GetChildChecked<ITextView>( pInfoPanel, "CareerExpNAView", true );
	pMissionExpNAView = GetChildChecked<ITextView>( pInfoPanel, "MissionExpNAView", true );
	pNewReinfLabel = GetChildChecked<ITextView>( pRewardPanel, "NewReinfLabel", true );

	if ( pCareerExpNAView )
		pCareerExpNAView->ShowWindow( bCustomMission );
	if ( pMissionExpNAView )
		pMissionExpNAView->ShowWindow( bCustomMission );
	if ( pCareerProgress )
		pCareerProgress->ShowWindow( !bCustomMission );
	if ( pRankProgress )
		pRankProgress->ShowWindow( !bCustomMission );
	if ( pNewReinfLabel )
		pNewReinfLabel->ShowWindow( !bCustomMission );

	if ( pMain )
		SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );

	pChapterReinfUpgrade = new SChapterReinfUpgrade();
	pChapterReinfUpgrade->InitControls( pChapterReinfDescWnd );

	if ( bCustomMission )
		MakeCustomMissionStatistics();
	else
		MakeCampaignStatistics();

	IFullScreenFader *pFader = Scene()->GetScreenFader();
	if ( pFader )
		pFader->Start( UNFADE_TIME, SCREEN_FADER_CLEAR, SCREEN_FADER_CLEAR, true );

	const NDb::SCampaign *pCampaign = Singleton<IScenarioTracker>()->GetCurrentCampaign();
	if ( pCampaign )
	{
		if ( pCampaign->pIntermissionMusic )
			Singleton<IMusicSystem>()->Init( pCampaign->pIntermissionMusic, 0 );
	}
	else
	{
		const NDb::SGameRoot *pGameRoot = InterfaceState()->GetGameRoot();
		if ( pGameRoot && pGameRoot->pMainMenuMusic )
			Singleton<IMusicSystem>()->Init( pGameRoot->pMainMenuMusic, 0 );
	}

	const bool bDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bDemo )
	{
		if ( pExitToChapterBtn )
			pExitToChapterBtn->ShowWindow( false );
	}
}

void CInterfaceSingleStatistic::MakeCustomMissionStatistics()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const int nLocalPlayer = pST->GetLocalPlayer();

	int nTime = pST->GetStatistics( nLocalPlayer, IScenarioTracker::ESK_TIME );
	std::wstring wszTime = GetFormattedTime( nTime );
	if ( pMissionTimeView )
		pMissionTimeView->SetText( pMissionTimeView->GetDBText() + wszTime );
	if ( pCampaignTimeView )
		pCampaignTimeView->SetText( pCampaignTimeView->GetDBText() + wszTime );

	MakePlayerStatistics( false );

	CheckNextPopup();
}

void CInterfaceSingleStatistic::MakeCampaignStatistics()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const int nLocalPlayer = pST->GetLocalPlayer();

	expProgressCareer.Init();
	expProgressRank.Init();

	ExpProgressStep();

	std::wstring wszRankHP = NStr::ToUnicode( fmt::format( "{}/{}",
		(int)( expProgressRank.fTarget - expProgressRank.fTargetLevelExp ), 
		(int)( expProgressRank.fTargetNextLevelExp - expProgressRank.fTargetLevelExp ) ) );
	if ( pExpView )
		pExpView->SetText( pExpView->GetDBText() + wszRankHP );

	int nTime = pST->GetStatistics( nLocalPlayer, IScenarioTracker::ESK_TIME );
	std::wstring wszTime = GetFormattedTime( nTime );
	if ( pMissionTimeView )
		pMissionTimeView->SetText( pMissionTimeView->GetDBText() + wszTime );

	int nCampaignTime = pST->GetStatistics( nLocalPlayer, IScenarioTracker::ESK_CAMPAIGN_TIME );
	std::wstring wszCampaignTime = GetFormattedTime( nCampaignTime );
	if ( pCampaignTimeView )
		pCampaignTimeView->SetText( pCampaignTimeView->GetDBText() + wszCampaignTime );

	MakePlayerStatistics( true );
	MakeReinf();

	const NDb::SMapInfo *pMission = pST->GetLastMission();
	const IScenarioTracker::SMissionStats *pMissionStats = pMission ? pST->GetMissionStats( pMission ) : 0;
	if ( pMissionStats )
	{
		pNewPlayerRank = pMissionStats->pNewPlayerRank;
		medals = pMissionStats->medals;
	}

	CheckNextPopup();
}

void CInterfaceSingleStatistic::MakePlayerStatistics( bool bChapter )
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const int nLocalPlayer = pST->GetLocalPlayer();

	const NDb::SMapInfo *pMission = pST->GetLastMission();
	if ( !pMission )
		return;

	int nIndex = 0;
	for ( int i = 0; i < pMission->players.size(); ++i )
	{
		const NDb::SMapPlayerInfo &dbPlayer = pMission->players[i];
		if ( dbPlayer.nDiplomacySide == 2 )
			break;
		if ( nIndex >= players.size() )
			break;

		SPlayer &player = players[nIndex];
		nIndex++;
		player.bHasStatistics = true;
		
		const NDb::SPartyDependentInfo *pPartyInfo = pST->GetPlayerParty( i );
		NI_ASSERT( pPartyInfo, "Designers: empty party info detected" );
		if ( !pPartyInfo )
			continue;
		
		if ( player.pWnd )
			player.pWnd->ShowWindow( true );
		
		if ( player.pIconWnd )
			player.pIconWnd->SetTexture( pPartyInfo->pStatisticsIcon );

		MakePlayerName( player, dbPlayer, i == nLocalPlayer, bChapter );

		int nUnitsLost = pST->GetStatistics( i, IScenarioTracker::ESK_UNITS_LOST );
		if ( player.pLostView )
			player.pLostView->SetText( player.pLostView->GetDBText() + 
				NStr::ToUnicode( std::to_string(  nUnitsLost ) ) );

		int nUnitsKilled = pST->GetStatistics( i, IScenarioTracker::ESK_UNITS_KILLED );
		if ( player.pKilledView )
			player.pKilledView->SetText( player.pKilledView->GetDBText() + 
				NStr::ToUnicode( std::to_string(  nUnitsKilled ) ) );

		int nReinfCalled = pST->GetStatistics( i, IScenarioTracker::ESK_REINFORCEMENTS_CALLED );
		if ( player.pReinfView )
			player.pReinfView->SetText( player.pReinfView->GetDBText() + 
				NStr::ToUnicode( std::to_string(  nReinfCalled ) ) );
	}
}

void CInterfaceSingleStatistic::MakePlayerName( const SPlayer &player, const NDb::SMapPlayerInfo &dbPlayer, 
	bool bPlayer, bool bChapter )
{
	std::wstring wszPlayerName;
	if ( CHECK_TEXT_NOT_EMPTY_PRE(dbPlayer.,LocalizedPlayerName) )
		wszPlayerName = GET_TEXT_PRE(dbPlayer.,LocalizedPlayerName);
	if ( bPlayer && wszPlayerName.empty() )
		wszPlayerName = NGlobal::GetVar( "profile_name", L"" );

	std::wstring wszRankTag;
	std::wstring wszPlayerRank;
	if ( bPlayer && bChapter )
	{
		IScenarioTracker *pST = Singleton<IScenarioTracker>();
		const NDb::SPlayerRank *pRank = pST->GetPlayerRank();
		if ( pRank )
		{
			if ( CHECK_TEXT_NOT_EMPTY_PRE(pRank->,RankName) )
				wszPlayerRank = GET_TEXT_PRE(pRank->,RankName);
		}
		if ( !wszPlayerRank.empty() )
		{
			if ( IScreen *pScreen = GetScreen() )
				wszRankTag = pScreen->GetTextEntry( "T_PLAYER_NAME_AND_RANK_TAG" );
		}
	}
	if ( !wszRankTag.empty() )
	{
		std::vector< std::pair<std::wstring, std::wstring> > params;
		params.push_back( std::pair<std::wstring, std::wstring>( DYNAMIC_TAG_PLAYER_NAME, wszPlayerName ) );
		params.push_back( std::pair<std::wstring, std::wstring>( DYNAMIC_TAG_PLAYER_RANK, wszPlayerRank ) );
		SetDynamicTextView( player.pNameView, params );
		if ( player.pNameView )
			player.pNameView->SetText( wszRankTag );
	}
	else
	{
		if ( player.pNameView )
			player.pNameView->SetText( player.pNameView->GetDBText() + wszPlayerName );
	}
}

void CInterfaceSingleStatistic::MakeReinf()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const int nLocalPlayer = pST->GetLocalPlayer();

	const NDb::SMapInfo *pMission = pST->GetLastMission();
	if ( !pMission )
		return;
	const IScenarioTracker::SMissionStats *pMissionStats = pST->GetMissionStats( pMission );
	if ( !pMissionStats )
		return;

	for ( int i = 0; i < reinfs.size(); ++i )
	{
		SReinf &reinf = reinfs[i];
		if ( i >= pMissionStats->bonusReinforcements.size() )
			break;
		const NDb::SReinforcement *pReinf = pMissionStats->bonusReinforcements[i];
		if ( !pReinf )
			continue;

		bool bUpgrade = false;
		for ( int j = 0; j < pMissionStats->oldReinfs.size(); ++j )
		{
			const IScenarioTracker::SMissionStats::SOldReinf &oldReinf = pMissionStats->oldReinfs[j];
			if ( !oldReinf.pDBReinf )
				continue;

			if ( oldReinf.pDBReinf->eType != pReinf->eType || oldReinf.eState != IScenarioTracker::ERS_ENABLED )
				continue;

			bUpgrade = true;
			break;
		}
			
		reinf.pReinf = pReinf;
		if ( reinf.pWnd )
			reinf.pWnd->ShowWindow( true );

		if ( reinf.pIconWnd )
			reinf.pIconWnd->SetTexture( pReinf->pIconTexture );
		if ( reinf.pBtn )
		{
			std::wstring wszTooltip;
			if ( bUpgrade )
				wszTooltip = GetScreen()->GetTextEntry( "TOOLTIP_PREFIX_UPGRADE" );
			else
				wszTooltip = GetScreen()->GetTextEntry( "TOOLTIP_PREFIX_NEW_REINF" );

			if ( CHECK_TEXT_NOT_EMPTY_PRE(pReinf->,Tooltip) )
				wszTooltip += GET_TEXT_PRE(pReinf->,Tooltip);
			reinf.pBtn->SetTooltip( wszTooltip );
		}

		if ( reinf.pNameView )
		{
			std::wstring wszName;
			if ( bUpgrade )
				wszName = GetScreen()->GetTextEntry( "T_PREFIX_UPGRADE" );
			else
				wszName = GetScreen()->GetTextEntry( "T_PREFIX_NEW_REINF" );

			if ( CHECK_TEXT_NOT_EMPTY_PRE(pReinf->,LocalizedName) )
				wszName += GET_TEXT_PRE(pReinf->,LocalizedName);

			if ( reinf.pNameView )
				reinf.pNameView->SetText( reinf.pNameView->GetDBText() + wszName );
		}
	}
}

bool CInterfaceSingleStatistic::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "menu_next" )
		return OnMenuNext();
	if ( szReaction == "medal_dialog_close" )
		return OnMedalDialogClose();
	if ( szReaction == "new_rank_close" )
		return OnNewRankDialogClose();
		
	if ( szReaction == "reinf_click" )
		return OnReinfClick( szSender );
		
	if ( szReaction == "reinf_upgrade_dialog_close" )
		return OnChapterReinfClose();
	if ( szReaction == "reinf_upgrade_unit_btn" )
		return OnChapterReinfUnitBtn( szSender );

	if ( szReaction == "menu_next_on_enter" )
		return OnMenuNextOnEnter();
	if ( szReaction == "restart_mission" )
		return OnRestartMission();
	if ( szReaction == "exit_to_main_menu" )
		return OnExitToMainMenu();
	if ( szReaction == "exit_to_chapter" )
		return OnExitToChapter();
	if ( szReaction == "load" )
		return OnLoad();
	if ( szReaction == "exit_to_windows" )
		return OnExitToWindows();

	return false;
}

int CInterfaceSingleStatistic::Check( const std::string &szCheckName ) const
{
	return 0;
}

bool CInterfaceSingleStatistic::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );

	CheckNextPopup();

	ExpProgressStep();

	return bResult;
}

void CInterfaceSingleStatistic::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );

	if ( bFocus )
	{
		if ( pBlackBgWnd )	
			pBlackBgWnd->ShowWindow( false );
		if ( pLoadBtn )
		{
			if ( pLoadBtn->IsVisible() )
				pLoadBtn->Enable( !NSaveLoad::IsSaveListEmpty() );
		}
	}
}

void CInterfaceSingleStatistic::ShowReinf( const NDb::SReinforcement *pReinf )
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const int nLocalPlayer = pST->GetLocalPlayer();

	if ( pChapterReinfDescWnd )
		pChapterReinfDescWnd->ShowWindow( true );

	IScenarioTracker::EReinforcementState eState = IScenarioTracker::ERS_DISABLED;
	const NDb::SReinforcement *pOldReinf = 0;

	const IScenarioTracker::SMissionStats *pStats = pST->GetMissionStats( pST->GetLastMission() );
	if ( pStats )
	{
		for ( int i = 0; i < pStats->oldReinfs.size(); ++i )
		{
			const IScenarioTracker::SMissionStats::SOldReinf &reinf = pStats->oldReinfs[i];
			if ( reinf.pDBReinf && pReinf && reinf.pDBReinf->eType == pReinf->eType )
			{
				eState = reinf.eState;
				pOldReinf = reinf.pDBReinf;
				break;
			}
		}
	}

	if ( eState == IScenarioTracker::ERS_ENABLED )
		pChapterReinfUpgrade->ShowReinf( pOldReinf, pReinf );
	else
		pChapterReinfUpgrade->ShowReinf( 0, pReinf );
}

bool CInterfaceSingleStatistic::OnMenuNext()
{
	NextMenu();

	return true;
}

bool CInterfaceSingleStatistic::OnMenuNextOnEnter()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( pST->IsMissionWon() )
		NextMenu();
	
	return true;
}

bool CInterfaceSingleStatistic::OnRestartMission()
{
	const NDb::SMapInfo *pMapInfo = Singleton<IScenarioTracker>()->GetCurrentMission();
	if ( !pMapInfo )
		pMapInfo = Singleton<IScenarioTracker>()->GetLastMission();
	NI_VERIFY( pMapInfo, "Wrong mission info", return true );

	Singleton<IScenarioTracker>()->MissionStart( pMapInfo );
		
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	const auto command = fmt::format( "{};normal", pMapInfo->GetDBID().ToString() );
	NMainLoop::Command( ML_COMMAND_MISSION, command.c_str() );

	return true;
}

bool CInterfaceSingleStatistic::OnExitToMainMenu()
{
	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );

	return true;
}

bool CInterfaceSingleStatistic::OnExitToChapter()
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	NMainLoop::Command( ML_COMMAND_CHAPTER_MAP_MENU, "" );

	return true;
}

bool CInterfaceSingleStatistic::OnLoad()
{
	if ( pBlackBgWnd )
		pBlackBgWnd->ShowWindow( true );

	NMainLoop::Command( ML_COMMAND_SAVE_LOAD_MENU, "load_from_single_statistics" );

	return true;
}

bool CInterfaceSingleStatistic::OnExitToWindows()
{
	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
		InterfaceState()->GetTextEntry( "T_ESCAPE_MENU_EXIT_WINDOWS_QUESTION" ) ).c_str() );

	return true;
}

void CInterfaceSingleStatistic::MsgOk( const SGameMessage &msg )
{
	NInput::PostEvent( "exit", 0, 0 );
}

void CInterfaceSingleStatistic::MsgCancel( const SGameMessage &msg )
{
}

void CInterfaceSingleStatistic::MsgTryExitWindows( const SGameMessage &msg )
{
	OnExitToWindows();
}

bool CInterfaceSingleStatistic::OnMedalDialogClose()
{
	if ( pNewMedalDlg )
		pNewMedalDlg->ShowWindow( false );
	if ( pPopupBgWnd )
		pPopupBgWnd->ShowWindow( true );

	timePrevPopup = Singleton<IGameTimer>()->GetAbsTime();
	
	CheckNextPopup();

	return true;
}

bool CInterfaceSingleStatistic::OnNewRankDialogClose()
{
	if ( pNewRankDlg )
		pNewRankDlg->ShowWindow( false );
	if ( pPopupBgWnd )
		pPopupBgWnd->ShowWindow( true );

	timePrevPopup = Singleton<IGameTimer>()->GetAbsTime();

	CheckNextPopup();

	return true;
}

bool CInterfaceSingleStatistic::OnReinfClick( const std::string &szSender )
{
	for ( int i = 0; i < reinfs.size(); ++i )
	{
		SReinf &reinf = reinfs[i];
		if ( reinf.szButtonName == szSender )
		{
			ShowReinf( reinf.pReinf );
			break;
		}
	}

	return true;
}

bool CInterfaceSingleStatistic::OnChapterReinfClose()
{
	if ( pChapterReinfDescWnd )
		pChapterReinfDescWnd->ShowWindow( false );

	pChapterReinfUpgrade->Hide();
	
	return true;
}

bool CInterfaceSingleStatistic::OnChapterReinfUnitBtn( const std::string &szSender )
{
	pChapterReinfUpgrade->UnitBtnPressed( szSender );
	
	return true;
}

void CInterfaceSingleStatistic::NextMenu()
{
	NI_VERIFY( Singleton<IScenarioTracker>()->GetGameType() == IAIScenarioTracker::EGT_SINGLE, "Wrong game type (single only allowed)", return );

	const bool bDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bDemo )
	{
		InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

		NGlobal::SetVar( "DEMO_MODE_CONTINUE_MOVIE", 1 );

		const std::string szParam = "Movies\\demo_outro.xml;demo_campaign_selection_menu";

		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		NMainLoop::Command( ML_COMMAND_PLAY_MOVIE, szParam.c_str() );
		return;
	}

	const NDb::SMapInfo *pMission = Singleton<IScenarioTracker>()->GetCurrentMission();
	if ( Singleton<IScenarioTracker>()->IsCustomMission() )
	{
//		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );

		if ( Singleton<IScenarioTracker>()->IsTutorialMission() )
			NMainLoop::Command( ML_COMMAND_SELECT_TUTORIAL, "" );
		else
			NMainLoop::Command( ML_COMMAND_CUSTOM_MISSIONS, "" );

		InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );
	}
	else
	{
//		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
		NMainLoop::Command( ML_COMMAND_CHAPTER_MAP_MENU, "" );
	}
}

std::wstring CInterfaceSingleStatistic::GetFormattedTime( int nTime ) const
{
	std::wstring wszTime = NStr::ToUnicode( std::to_string(  nTime % 60 ) ) + wszTime3;
	nTime /= 60;
	if ( nTime > 0 )
	{
		wszTime = NStr::ToUnicode( std::to_string(  nTime % 60 ) ) + wszTime2 + wszTime;
		nTime /= 60;
	}
	if ( nTime > 0 )
	{
		wszTime = NStr::ToUnicode( std::to_string(  nTime % 60 ) ) + wszTime1 + wszTime;
	}
	
	return wszTime;
}

void CInterfaceSingleStatistic::CheckNextPopup()
{
	if ( !expProgressCareer.bFinal && !expProgressRank.bFinal )
	{
		timePrevPopup = Singleton<IGameTimer>()->GetAbsTime();
		return;
	}

	if ( bPopup )
	{
		if ( pNewRankDlg && pNewRankDlg->IsVisible() )
			return;
		if ( pNewMedalDlg && pNewMedalDlg->IsVisible() )
			return;
			
		if ( pNewPlayerRank || !medals.empty() )
		{
			NTimer::STime timeCur = Singleton<IGameTimer>()->GetAbsTime();
			if ( timeCur < timePrevPopup + PAUSE_BETWEEN_MEDALS )
				return;
		}

		if ( pPopupBgWnd )
			pPopupBgWnd->ShowWindow( false );
		bPopup = false;
	}

	if ( pNewPlayerRank )
	{
		ShowNewRank( pNewPlayerRank );
		pNewPlayerRank = 0;
		bPopup = true;
	}
	else
	{
		if ( !medals.empty() )
		{
			const NDb::SMedal *pMedal = medals.front();
			ShowMedal( pMedal );
			medals.pop_front();
			bPopup = true;
		}
	}
}

void CInterfaceSingleStatistic::ShowNewRank( const NDb::SPlayerRank *pRank )
{
	if ( pNewRankDlg )
		pNewRankDlg->ShowWindow( true );

	if ( pNewRankIconWnd )
		pNewRankIconWnd->SetTexture( pRank ? pRank->pStrap : 0 );

	std::wstring wszRank;
	if ( pRank && CHECK_TEXT_NOT_EMPTY_PRE(pRank->,RankName) )
		wszRank = GET_TEXT_PRE(pRank->,RankName);
	if ( pNewRankView )
		pNewRankView->SetText( pNewRankView->GetDBText() + wszRank );
}

void CInterfaceSingleStatistic::ShowMedal( const NDb::SMedal *pMedal )
{
	if ( pNewMedalDlg )
		pNewMedalDlg->ShowWindow( true );

	if ( pMedalIconWnd )
		pMedalIconWnd->SetTexture( pMedal ? pMedal->pPictureTexture : 0 );
		
	std::wstring wszName;
	if ( pMedal && CHECK_TEXT_NOT_EMPTY_PRE(pMedal->,LocalizedName) )
		wszName = GET_TEXT_PRE(pMedal->,LocalizedName);
	if ( pMedalNameView )
		pMedalNameView->SetText( pMedalNameView->GetDBText() + wszName );
		
	std::wstring wszDesc;
	if ( pMedal && CHECK_TEXT_NOT_EMPTY_PRE(pMedal->,LocalizedDesc) )
		wszDesc = GET_TEXT_PRE(pMedal->,LocalizedDesc);
	if ( pMedalDescView )
		pMedalDescView->SetText( pMedalDescView->GetDBText() + wszDesc );
	if ( pMedalDescCont )
		pMedalDescCont->Update();
}

void CInterfaceSingleStatistic::ExpProgressStep()
{
	if ( !expProgressCareer.bFinal )
	{
		expProgressCareer.Step( false );
		if ( pCareerProgress )
			pCareerProgress->SetPosition( expProgressCareer.fProgress );
		if ( pNewCareerProgress )
			pNewCareerProgress->SetPosition( expProgressCareer.fNewProgress );
	}
	
	if ( !expProgressRank.bFinal )
	{
		expProgressRank.Step( false );
		if ( pRankProgress )
			pRankProgress->SetPosition( expProgressRank.fProgress );
		if ( pNewRankProgress )
			pNewRankProgress->SetPosition( expProgressRank.fNewProgress );
	}
}

// CICSingleStatistic

void CICSingleStatistic::PreCreate()
{
}

void CICSingleStatistic::PostCreate( IInterface *pInterface )
{
#ifndef _FINALRELEASE
	// sanity check
	NI_ASSERT( NMainLoop::GetTopInterface() == 0, "Programmers: no interfaces are allowed under the CInterfaceSingleStatistic" );
#endif
	NMainLoop::PushInterface( pInterface );
}

void CICSingleStatistic::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x17194B40, CInterfaceSingleStatistic )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_SINGLE_STATISTIC, CICSingleStatistic )


