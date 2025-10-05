#include "stdafx.h"
#include "InterfaceMainMenu.h"
#include "GameXClassIDs.h"
#include "Misc/StrProc.h"
#include "SceneB2/Cursor.h"
#include "InterfaceState.h"
#include "InterfaceMisc.h"
#include "DBGameRoot.h"
#include "ScenarioTracker.h"
#include "Sound/MusicSystem.h"
#include "SaveLoadHelper.h"
#include "Main/Profiles.h"
#include "Main/MODs.h"

static bool bVisited = false;

const char* PROFILE_MANAGER_VISITED = "ProfileManagerVisited";

CInterfaceMainMenu::CInterfaceMainMenu() :
	CInterfaceScreenBase( "MainMenu", "intermission" )
{
	AddObserver( "options_menu", &CInterfaceMainMenu::MsgOptionsMenu );
	AddObserver( "credits_screen", &CInterfaceMainMenu::MsgCreditsScreen );
	AddObserver( "campaign_selection_menu", &CInterfaceMainMenu::MsgCampaignSelectionMenu );
	AddObserver( "profile_manager_menu", &CInterfaceMainMenu::MsgProfileManagerMenu );
	AddObserver( "try_exit", &CInterfaceMainMenu::MsgExit );
	AddObserver( "message_box_ok", &CInterfaceMainMenu::MsgOk );
	AddObserver( "message_box_cancel", &CInterfaceMainMenu::MsgCancel );
	AddObserver( "menu_load", &CInterfaceMainMenu::MsgLoadMenu );
	AddObserver( "mp_games_list_menu", &CInterfaceMainMenu::MsgMPGamesListMenu );
	AddObserver( "menu_tutorial", &CInterfaceMainMenu::MsgTutorialMenu );

	AddObserver( "menu_custom_mission", &CInterfaceMainMenu::MsgMenuCustomMission );
	AddObserver( "menu_encyclopedia", &CInterfaceMainMenu::MsgEncyclopedia );
	AddObserver( "menu_single_player", &CInterfaceMainMenu::MsgSinglePlayer );
	AddObserver( "quickload", &CInterfaceMainMenu::MsgQuickLoad );

	AddReaction( "reaction_on_multiplayer", &CInterfaceMainMenu::OnMultiplayer );
	AddReaction( "menu_custom_campaign", &CInterfaceMainMenu::OnCustomCampaign );
	AddReaction( "menu_load_mod", &CInterfaceMainMenu::OnLoadMod );
	AddReaction( "reaction_on_hall_of_fame", &CInterfaceMainMenu::OnHallOfFame );
}

bool CInterfaceMainMenu::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;
		
	Singleton<IGameTimer>()->Pause( false, PAUSE_TYPE_USER_PAUSE );
	
	AddScreen( this );

	Cursor()->Show( true );
	Cursor()->SetMode( NDb::USER_ACTION_UNKNOWN );
//	Singleton<ISoundScene>()->SetSoundSceneMode( ESSM_INTERMISSION_INTERFACE );

	// Substitute MPScenarioTracker to ScenarioTracker
/*	NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
	NSingleton::UnRegisterSingleton( IAIScenarioTracker::tidTypeID );

	IScenarioTracker * pScenarioTracker = CreateScenarioTracker();
	NSingleton::RegisterSingleton( pScenarioTracker, IScenarioTracker::tidTypeID );
	NSingleton::RegisterSingleton( pScenarioTracker, IAIScenarioTracker::tidTypeID );*/

	InterfaceState()->VerifyScenarioTracker( IInterfaceState::ESTT_NONE );

	const NDb::SGameRoot *pGameRoot = InterfaceState()->GetGameRoot();

	pMainWnd = GetChildChecked<IWindow>( GetScreen(), "MainMenu", true );

	pSinglePlayerMenuWnd = GetChildChecked<IWindow>( GetScreen(), "SinglePlayerMenu", true );

	pTutorialBtn = GetChildChecked<IButton>( pSinglePlayerMenuWnd, "TutorialBtn", true );
	pLoadGameBtn = GetChildChecked<IButton>( pSinglePlayerMenuWnd, "LoadGameBtn", true );

	if ( pTutorialBtn )
		pTutorialBtn->Enable( pGameRoot && !pGameRoot->tutorialMaps.empty() );

#ifndef _FINALRELEASE
	ITextView *pVersionView = GetChildChecked<ITextView>( GetScreen(), "VersionNumber", true );
	std::wstring wszVersion = NGlobal::GetVar( "code_version_number", L"" );
	if ( pVersionView )
		pVersionView->SetText( pVersionView->GetDBText() + wszVersion );
#endif

	// music
	if ( pGameRoot && pGameRoot->pMainMenuMusic )
		Singleton<IMusicSystem>()->Init( pGameRoot->pMainMenuMusic, 0 );

	const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bKRIDemo )
	{
		IWindow *pMainWnd = GetChildChecked<IWindow>( GetScreen(), "MainMenu", true );
		IButton *pOptionsBtn = GetChildChecked<IButton>( pMainWnd, "OptionsBtn", true );
		IButton *pLoadMODBtn = GetChildChecked<IButton>( pMainWnd, "LoadMODBtn", true );
		IButton *pCreditsBtn = GetChildChecked<IButton>( pMainWnd, "CreditsBtn", true );
		IButton *pEncyclopediaBtn = GetChildChecked<IButton>( pMainWnd, "EncyclopediaBtn", true );
		IButton *pMultiplayerBtn = GetChildChecked<IButton>( pMainWnd, "MultiplayerBtn", true );
		
		IWindow *pSinglePlayerMenuWnd = GetChildChecked<IWindow>( GetScreen(), "SinglePlayerMenu", true );
		IButton *pTutorialBtn = GetChildChecked<IButton>( pSinglePlayerMenuWnd, "TutorialBtn", true );
		IButton *pLoadGameBtn = GetChildChecked<IButton>( pSinglePlayerMenuWnd, "LoadGameBtn", true );
		IButton *pCustomMissionBtn = GetChildChecked<IButton>( pSinglePlayerMenuWnd, "CustomMissionBtn", true );
		IButton *pCustomCampaignBtn = GetChildChecked<IButton>( pSinglePlayerMenuWnd, "CustomCampaignBtn", true );
		IButton *pProfileManagerBtn = GetChildChecked<IButton>( pSinglePlayerMenuWnd, "ProfileManagerBtn", true );
		
		if ( pMultiplayerBtn )
			pMultiplayerBtn->Enable( false );
//		if ( pOptionsBtn )
//			pOptionsBtn->Enable( false );
		if ( pLoadMODBtn )
			pLoadMODBtn->Enable( false );
//		if ( pCreditsBtn )
//			pCreditsBtn->Enable( false );
		if ( pEncyclopediaBtn )
			pEncyclopediaBtn->Enable( false );

		if ( pTutorialBtn )
			pTutorialBtn->Enable( false );
		if ( pLoadGameBtn )
			pLoadGameBtn->Enable( false );
		if ( pCustomMissionBtn )
			pCustomMissionBtn->Enable( false );
		if ( pCustomCampaignBtn )
			pCustomCampaignBtn->Enable( false );
		if ( pProfileManagerBtn )
			pProfileManagerBtn->Enable( false );
	}
	
	const bool bMPDemo = NGlobal::GetVar( "MP_DEMO", 0 ) != 0;
	if ( bMPDemo )
	{
		IWindow *pMainWnd = GetChildChecked<IWindow>( GetScreen(), "MainMenu", true );
		
		IButton *pSinglePlayerBtn = GetChildChecked<IButton>( pMainWnd, "SinglePlayerBtn", true );
		IButton *pLoadMODBtn = GetChildChecked<IButton>( pMainWnd, "LoadMODBtn", true );
		IButton *pEncyclopediaBtn = GetChildChecked<IButton>( pMainWnd, "EncyclopediaBtn", true );

		if ( pSinglePlayerBtn )
			pSinglePlayerBtn->Enable( false );
		if ( pLoadMODBtn )
			pLoadMODBtn->Enable( false );
		if ( pEncyclopediaBtn )
			pEncyclopediaBtn->Enable( false );
	}

	return true;
}

void CInterfaceMainMenu::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
	
	PauseIntermission( false );
	
	if ( bFocus )
	{
		if ( GetScreen() )
			GetScreen()->ShowWindow( true );
		if ( pLoadGameBtn )
			pLoadGameBtn->Enable( !NSaveLoad::IsSaveListEmpty() );
	}
}

bool CInterfaceMainMenu::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );
	
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return bResult;
}

bool CInterfaceMainMenu::IsModal()
{
	return false;
}


void CInterfaceMainMenu::MsgCreditsScreen( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "options_menu_end" );
	NMainLoop::Command( ML_COMMAND_CREDITS, "" );
}

void CInterfaceMainMenu::OnHallOfFame( const std::string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "options_menu_end" );
	NMainLoop::Command( ML_COMMAND_SHOW_HALL_OF_FAME, "" );
}

void CInterfaceMainMenu::MsgOptionsMenu( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_OPTIONS_MENU, "" );
}

void CInterfaceMainMenu::MsgCampaignSelectionMenu( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	if ( NMOD::DoesAnyMODAttached() )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );

		NMOD::DetachAllMODs();

		NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	}
	NMainLoop::Command( ML_COMMAND_CAMPAIGN_SELECTION_MENU, "" );
}

void CInterfaceMainMenu::MsgProfileManagerMenu( const SGameMessage &msg )
{
	if ( GetScreen() )
		GetScreen()->ShowWindow( false ); 
	NMainLoop::Command( ML_COMMAND_PROFILE_MENU, "" );
}

void CInterfaceMainMenu::MsgMPGamesListMenu( const SGameMessage &msg )
{
	NI_ASSERT( 0, "obsolete" );
}

void CInterfaceMainMenu::MsgExit( const SGameMessage &msg )
{
//	Singleton<IMainLoop>()->Command( ML_COMMAND_MESSAGE_BOX, "type:MessageBoxWindowOkCancel;text:1234" );
	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
			InterfaceState()->GetTextEntry( "T_MAIN_MENU_EXIT_QUESTION" ) ).c_str() );
}

void CInterfaceMainMenu::MsgOk( const SGameMessage &msg )
{
	NInput::PostEvent( "exit", 0, 0 );
}

void CInterfaceMainMenu::MsgCancel( const SGameMessage &msg )
{
}

void CInterfaceMainMenu::MsgLoadMenu( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_SAVE_LOAD_MENU, "load_from_main_menu" );
}

void CInterfaceMainMenu::MsgMenuCustomMission( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_CUSTOM_MISSIONS, "" );
}

void CInterfaceMainMenu::MsgEncyclopedia( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_ENCYCLOPEDIA_WAIT, "" );
}

void CInterfaceMainMenu::MsgTutorialMenu( const SGameMessage &msg )
{
/*	InterfaceState()->VerifyScenarioTracker( IInterfaceState::ESTT_NONE );
	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_SINGLE );

	const NDb::SGameRoot *pGameRoot = InterfaceState()->GetGameRoot();
	const int nDifficulty = 0;
	const NDb::SCampaign *pCampaign = pGameRoot->pTutorial;
	NI_VERIFY( pCampaign, "Designers: tutorial campaign not found (GameRoot/Tutorial)", return );
	Singleton<IScenarioTracker>()->CampaignStart( pCampaign, nDifficulty, true );
	
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_CHAPTER_MAP_MENU, "" );*/

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	if ( NMOD::DoesAnyMODAttached() )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );

		NMOD::DetachAllMODs();

		NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	}
	NMainLoop::Command( ML_COMMAND_SELECT_TUTORIAL, "" );
}

void CInterfaceMainMenu::OnMultiplayer( const std::string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MULTIPLAYER_MENU, "" );
}

void CInterfaceMainMenu::OnCustomCampaign( const std::string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_CUSTOM_CAMPAIGN, "" );
}

void CInterfaceMainMenu::OnLoadMod( const std::string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_LOAD_MOD, "" );
}

void CInterfaceMainMenu::MsgSinglePlayer( const SGameMessage &msg )
{
	if ( bVisited )
		return;

	std::wstring wszProfileName = NProfile::GetCurrentProfileName();
	std::wstring wszDefaultProfileName;
	if ( IScreen *pScreen = GetScreen() )
		wszDefaultProfileName = pScreen->GetTextEntry( "T_DEFAULT_PROFILE_NAME" );
	if ( wszProfileName != wszDefaultProfileName )
	{
		bVisited = true;
		return;
	}

	bVisited = NGlobal::GetVar( PROFILE_MANAGER_VISITED, 0 ) != 0;
	if ( bVisited )
		return;

	bVisited = true;
	NGlobal::SetVar( PROFILE_MANAGER_VISITED, 1, STORAGE_USER );
	if ( GetScreen() )
		GetScreen()->ShowWindow( false ); 
	NMainLoop::Command( ML_COMMAND_PROFILE_MENU, "" );
}

void CInterfaceMainMenu::MsgQuickLoad( const SGameMessage &msg )
{
	NGlobal::ProcessCommand( L"quick_load" );
}

// CICMainMenu

void CICMainMenu::PreCreate()
{
}

void CICMainMenu::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
	if ( !szConfig.empty() )
		NInput::PostEvent( szConfig, 0, 0 );
	else
		NInput::PostEvent( "main_menu_init", 0, 0 );
}

void CICMainMenu::Configure( const char *pszConfig )
{
	szConfig = pszConfig;
}


void StartMainMenu( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
#ifndef _FINALRELEASE
	if ( !paramsSet.empty() ) 
	{
		const std::string szVal = NStr::ToMBCS( paramsSet[0] );
		NGlobal::SetVar( "CRAP.MainMenu.BackgroundMap", szVal.c_str()  );
	}
	if ( paramsSet.size() >= 2 )
	{
		const std::string szVal = NStr::ToMBCS( paramsSet[1] );
		NGlobal::SetVar( "CRAP.MainMenu.CampaignMap", szVal.c_str()  );
	}
#endif

	NGlobal::RemoveVar( "Multiplayer.Host" );
	NGlobal::RemoveVar( "Multiplayer.Client" );

	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

	NMainLoop::Command( ML_COMMAND_CLEAR_INTERFACES, "" );
	NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
}

void StartMainMenuIntro( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
#ifndef _FINALRELEASE
	NGlobal::SetVar( "CRAP.MainMenu.BackgroundMap", "" );
	NGlobal::SetVar( "CRAP.MainMenu.CampaignMap", "" );
#endif

	if ( const NDb::SGameRoot *pRoot = InterfaceState()->GetGameRoot() )
	{
		InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

		std::string szText = pRoot->szIntroMovie;
		const bool bDemo = ( NGlobal::GetVar( "DEMO_MODE", 0 ) != 0 ) || ( NGlobal::GetVar( "MP_DEMO", 0 ) != 0 );
		if ( bDemo )
		{
			szText += ";demo_screen";
			NGlobal::SetVar( "main_loop_on_exit_command", L"demo_screen_final" );
		}
		else
			szText += ";menu";
		NMainLoop::Command( ML_COMMAND_CLEAR_INTERFACES, "" );
		NMainLoop::Command( ML_COMMAND_PLAY_MOVIE, szText.c_str() );
	}
}

START_REGISTER(MainMenuCommands)
REGISTER_CMD( "menu", StartMainMenu );
REGISTER_CMD( "main_menu", StartMainMenuIntro );
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x1005C445, CInterfaceMainMenu )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MAIN_MENU, CICMainMenu )


