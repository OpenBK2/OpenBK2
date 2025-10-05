#include "stdafx.h"
#include "InterfaceEscMenu.h"
#include "InterfaceMisc.h"
#include "GameXClassIDs.h"
#include "InterfaceState.h"
#include "3Dmotor/ScreenShot.h"
#include "ScenarioTracker.h"
#include "MultiplayerCommandManager.h"
#include "SaveLoadHelper.h"

// CInterfaceEscMenu::CReactions

bool CInterfaceEscMenu::CReactions::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "react_on_save" )
	{
		pInterface->OnSave( szSender );
		return true;
	}
	if ( szReaction == "react_on_load" )
	{
		pInterface->OnLoad( szSender );
		return true;
	}
	if ( szReaction == "restart_mission" )
		return pInterface->OnRestartMission( szSender );
	if ( szReaction == "end_mission_submenu" )
		return pInterface->OnEndMissionSubMenu( szSender );
		
	if ( szReaction == "reaction_on_end_mission_return" )
		return pInterface->OnEndMissionReturnToGame( szSender );
	if ( szReaction == "menu_back" )
		return pInterface->OnMenuBack( szSender );
	if ( szReaction == "mp_menu_back" )
		return pInterface->OnMPMenuBack( szSender );

	if ( szReaction == "react_on_finish_replay" )
		return pInterface->OnFinishReplay();

	return false;
}

// CInterfaceEscMenu

CInterfaceEscMenu::CInterfaceEscMenu() : 
	CInterfaceScreenBase( "MissionEscMenu", "esc_menu" ), bClosed( false )
{
	AddObserver( "try_exit_main_menu", &CInterfaceEscMenu::MsgTryExitMainMenu );
	AddObserver( "try_exit_windows", &CInterfaceEscMenu::MsgTryExitWindows );
	AddObserver( "go_main_menu", &CInterfaceEscMenu::MsgGoMainMenu );
	AddObserver( "options_menu", &CInterfaceEscMenu::MsgOptionsMenu );
	AddObserver( "message_box_ok", &CInterfaceEscMenu::MsgOk );
	AddObserver( "message_box_cancel", &CInterfaceEscMenu::MsgCancel );
	AddObserver( "menu_winned", &CInterfaceEscMenu::MsgWinned );

	AddObserver( "multiplayer_end_mission", &CInterfaceEscMenu::MsgMultiplayerEndMission );
	AddObserver( "menu_help_button", &CInterfaceEscMenu::MsgHelpButton );
}

CInterfaceEscMenu::~CInterfaceEscMenu()
{
}

int CInterfaceEscMenu::CReactions::Check( const std::string &szCheckName ) const
{
	if ( szCheckName == "IsMultiplayer" )
	{
		return Singleton<IScenarioTracker>()->GetGameType() != IAIScenarioTracker::EGT_SINGLE;
	}
	return 0;
}

bool CInterfaceEscMenu::Init()
{
	//Make screenshot before anything else
	//InterfaceState()->GetScreenShotTexture()->Generate( true );

	// load screen
	pReactions = new CReactions( pScreen, this );
	
	AddScreen( pReactions );

	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	IWindow *pScreenWnd = dynamic_cast_ptr<IWindow*>( pScreen );
	if ( !pScreenWnd )
		return false;
		
	IWindow *pWinnedMenu = GetChildChecked<IWindow>( pScreenWnd, "WinnedMenu", true );

	IWindow * pEsc = 0;
	if ( NGlobal::GetVar( "History.Playing", 0 ) )
	{
		pEsc = GetChildChecked<IWindow>( pScreenWnd, "ReplayEscapeMenu", true );
	}
	else
	{
		if ( Singleton<IScenarioTracker>() && Singleton<IScenarioTracker>()->GetGameType() != IAIScenarioTracker::EGT_SINGLE )
			pEsc = GetChildChecked<IWindow>( pScreenWnd, "MultiplayerEscapeMenu", true );
		else
			pEsc = GetChildChecked<IWindow>( pScreenWnd, "EscapeMenu", true );
	}
	if ( pEsc )
		pEsc->ShowWindow( true );

	if ( !Singleton<IScenarioTracker>() || Singleton<IScenarioTracker>()->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
	{
		bool bWinned = !Singleton<IScenarioTracker>()->IsMissionActive() && Singleton<IScenarioTracker>()->IsMissionWon();
		IWindow * pWinMissionButton = GetChildChecked<IWindow>( pScreenWnd , "WinMissionButton", true );
		pWinMissionButton->Enable( bWinned );
		if ( !bWinned )
			NInput::PostEvent( "esc_menu_init", 0, 0 );

		if ( bWinned )
		{
			IButton *pRestartBtn = GetChildChecked<IButton>( pScreenWnd, "RestartMissionBtn", true );
			if ( pRestartBtn )
				pRestartBtn->Enable( false );
		}
	}

	pEscMenu = GetChildChecked<IWindow>( pScreenWnd, "EscapeMenu", true );
	pMPEscMenu = GetChildChecked<IWindow>( pScreenWnd, "MultiplayerEscapeMenu", true );
	pEndMissionSubMenu = GetChildChecked<IWindow>( pScreenWnd, "EndMissionMenu", true );
	pMPEndMissionSubMenu = GetChildChecked<IWindow>( pScreenWnd, "MultiplayerEndMissionMenu", true );

	pEndMissionExitToMainMenuBtn = GetChildChecked<IButton>( pEndMissionSubMenu, "ExitToMainMenuBtn", true );
	pEndMissionExitToChapterMapBtn = GetChildChecked<IButton>( pEndMissionSubMenu, "ExitToChapterMapBtn", true );

	pLoadBtn = GetChildChecked<IButton>( pEscMenu, "LoadBtn", true );

	const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bKRIDemo )
	{
//		IButton *pSaveBtn = GetChildChecked<IButton>( pEscMenu, "SaveMission", true );
//		IButton *pLoadBtn = GetChildChecked<IButton>( pEscMenu, "LoadBtn", true );
//		IButton *pOptionsBtn = GetChildChecked<IButton>( pEscMenu, "OptionsBtn", true );

//		IButton *pExitBtn = GetChildChecked<IButton>( pEndMissionSubMenu, "ExitBtn", true );

		IButton *pMPOptionsBtn = GetChildChecked<IButton>( pMPEscMenu, "OptionsBtn", true );

		IButton *pMPExitBtn = GetChildChecked<IButton>( pMPEndMissionSubMenu, "ExitBtn", true );
		
//		if ( pSaveBtn )
//			pSaveBtn->Enable( false );
//		if ( pLoadBtn )
//			pLoadBtn->Enable( false );
//		if ( pOptionsBtn )
//			pOptionsBtn->Enable( false );

//		if ( pExitBtn )
//			pExitBtn->Enable( false );

		if ( pMPOptionsBtn )
			pMPOptionsBtn->Enable( false );

		if ( pMPExitBtn )
			pMPExitBtn->Enable( false );
	}
	
	eMode = EMODE_UNKNOWN;

	return true;
}

void CInterfaceEscMenu::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
	
	if ( bFocus )
	{
		if ( pLoadBtn )
			pLoadBtn->Enable( !NSaveLoad::IsSaveListEmpty() );
	}
}

void CInterfaceEscMenu::MsgMultiplayerEndMission( const SGameMessage &msg )
{
	IWindow * pMEnd = GetChildChecked<IWindow>( pScreen, "MultiplayerEndMissionMenu", true );
	if ( pMEnd )
		pMEnd->ShowWindow( true );
	IWindow * pMEsc = GetChildChecked<IWindow>( pScreen, "MultiplayerEscapeMenu", true );
	if ( pMEsc )
		pMEsc->ShowWindow( false );
}

void CInterfaceEscMenu::MsgHelpButton( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_HELP_SCREEN, "Mission" );
}

void CInterfaceEscMenu::MsgTryExitMainMenu( const SGameMessage &msg )
{
	eMode = EMODE_EXIT_MAIN_MENU;

	bool bCustomMission = Singleton<IScenarioTracker>()->IsCustomMission();
	IScenarioTracker::EGameType eType = Singleton<IScenarioTracker>()->GetGameType();
	std::wstring wszMsg = InterfaceState()->GetTextEntry( eType != IScenarioTracker::EGT_SINGLE ?
		"T_MP_ESCAPE_MENU_SURRENDER" : (!bCustomMission ? "T_ESCAPE_MENU_EXIT_CHAPTER_MAP_QUESTION" : 
		"T_ESCAPE_MENU_EXIT_MAIN_MENU_QUESTION") );

	const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bKRIDemo )
	{
		wszMsg = InterfaceState()->GetTextEntry( eType != IScenarioTracker::EGT_SINGLE ? 
			"T_MP_ESCAPE_MENU_SURRENDER" : "T_ESCAPE_MENU_EXIT_MAIN_MENU_QUESTION" );
	}

	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", wszMsg ).c_str() );
}

void CInterfaceEscMenu::MsgTryExitWindows( const SGameMessage &msg )
{
	if ( eMode == EMODE_EXIT_WINDOWS )
		return;

	eMode = EMODE_EXIT_WINDOWS;

	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
			InterfaceState()->GetTextEntry( "T_ESCAPE_MENU_EXIT_WINDOWS_QUESTION" ) ).c_str() );
}

void CInterfaceEscMenu::MsgGoMainMenu( const SGameMessage &msg )
{
	GoMainMenu();
}

void CInterfaceEscMenu::GoMainMenu()
{
	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
}

void CInterfaceEscMenu::GoChapterMap()
{
	Singleton<IScenarioTracker>()->MissionCancel();

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	NMainLoop::Command( ML_COMMAND_CHAPTER_MAP_MENU, "" );
}

void CInterfaceEscMenu::MsgOptionsMenu( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_OPTIONS_MENU, "" );
}

void CInterfaceEscMenu::OnSave( const std::string &szSender )
{
	NMainLoop::Command( ML_COMMAND_SUPPRESS_ENABLE_FOCUS, "" );
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_HIDE_UNFOCUSED_SCREEN, "" );
	NMainLoop::Command( ML_COMMAND_SAVE_LOAD_MENU, "save" );
}

void CInterfaceEscMenu::OnLoad( const std::string &szSender )
{
	NMainLoop::Command( ML_COMMAND_SUPPRESS_ENABLE_FOCUS, "" );
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_HIDE_UNFOCUSED_SCREEN, "" );
	NMainLoop::Command( ML_COMMAND_SAVE_LOAD_MENU, "load" );
}

bool CInterfaceEscMenu::OnRestartMission( const std::string &szSender )
{
	eMode = EMODE_RESTART_MISSION;

	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
			InterfaceState()->GetTextEntry( "T_ESCAPE_MENU_RESTART_MISSION_QUESTION" ) ).c_str() );
			
	return true;
}

bool CInterfaceEscMenu::OnEndMissionSubMenu( const std::string &szSender )
{
	if ( pEscMenu )
		pEscMenu->ShowWindow( false );
	if ( pMPEscMenu )
		pMPEscMenu->ShowWindow( false );

	if ( pEndMissionSubMenu )
		pEndMissionSubMenu->ShowWindow( true );

	bool bCustomMission = Singleton<IScenarioTracker>()->IsCustomMission();

	if ( pEndMissionExitToMainMenuBtn )
		pEndMissionExitToMainMenuBtn->ShowWindow( bCustomMission );
	if ( pEndMissionExitToChapterMapBtn )
		pEndMissionExitToChapterMapBtn->ShowWindow( !bCustomMission );

	const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bKRIDemo )
	{
		if ( pEndMissionExitToMainMenuBtn )
			pEndMissionExitToMainMenuBtn->ShowWindow( true );
		if ( pEndMissionExitToChapterMapBtn )
			pEndMissionExitToChapterMapBtn->ShowWindow( false );
	}

	return true;
}

bool CInterfaceEscMenu::OnEndMissionReturnToGame( const std::string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	
	return true;
}

bool CInterfaceEscMenu::OnMenuBack( const std::string &szSender )
{
	if ( pEscMenu )
		pEscMenu->ShowWindow( true );
	if ( pEndMissionSubMenu )
		pEndMissionSubMenu->ShowWindow( false );
	
	return true;
}

bool CInterfaceEscMenu::OnMPMenuBack( const std::string &szSender )
{
	if ( pMPEscMenu )
		pMPEscMenu->ShowWindow( true );
	if ( pMPEndMissionSubMenu )
		pMPEndMissionSubMenu->ShowWindow( false );
	
	return true;
}

bool CInterfaceEscMenu::OnFinishReplay()
{
	eMode = EMODE_FINISH_REPLAY;

	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
		GetScreen()->GetTextEntry( "T_FINISH_REPLAY_QUESTION" ) ).c_str() );

	return true;
}

void CInterfaceEscMenu::MsgOk( const SGameMessage &msg )
{
	if ( eMode == EMODE_EXIT_MAIN_MENU )
	{
		if ( Singleton<IScenarioTracker>()->GetGameType() != IAIScenarioTracker::EGT_SINGLE )
		{
			NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
			Singleton<IMPToUIManager>()->AddUIMessage( EMUI_LEAVE_GAME );
		}
		else
		{
			const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
			if ( bKRIDemo )
			{
				GoMainMenu();
				return;
			}

			if ( Singleton<IScenarioTracker>()->IsCustomMission() )
				GoMainMenu();
			else
				GoChapterMap();
		}
	}
	else if ( eMode == EMODE_EXIT_WINDOWS )
		NInput::PostEvent( "exit", 0, 0 );
	else if ( eMode == EMODE_RESTART_MISSION )
		DoRestartMission();
	else if ( eMode == EMODE_FINISH_REPLAY )
	{
		NGlobal::SetVar( "History.Playing", 0 );
		NMainLoop::Command( ML_COMMAND_CLEAR_INTERFACES, "" );
		NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
		NMainLoop::Command( ML_COMMAND_MULTIPLAYER_MENU, "" );
		NMainLoop::Command( ML_COMMAND_REPLAY_SAVE_LOAD, "load" );
	}
}

void CInterfaceEscMenu::MsgCancel( const SGameMessage &msg )
{
	if ( eMode == EMODE_EXIT_WINDOWS )
		eMode = EMODE_UNKNOWN;
}

void CInterfaceEscMenu::MsgWinned( const SGameMessage &msg )
{
	if ( Singleton<IScenarioTracker>()->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		NMainLoop::Command( ML_COMMAND_HIDE_UNFOCUSED_SCREEN, "" );
		NMainLoop::Command( ML_COMMAND_SINGLE_STATISTIC, "" );
	}
	else
	{
		Singleton<IInterfaceState>()->SendCommandsToCloseAllIncluding( this );
		NMainLoop::Command( ML_COMMAND_HIDE_ALL_UP_TO, "Mission" );
		NMainLoop::Command( ML_COMMAND_HIDE_UNFOCUSED_SCREEN, "" );
		NMainLoop::Command( ML_COMMAND_MP_STATISTICS, "" );
	}
}

void CInterfaceEscMenu::DoRestartMission()
{
	const NDb::SMapInfo *pMapInfo = Singleton<IScenarioTracker>()->GetCurrentMission();
	if ( !pMapInfo )
		pMapInfo = Singleton<IScenarioTracker>()->GetLastMission();
	NI_VERIFY( pMapInfo, "Wrong mission info", return );

	Singleton<IScenarioTracker>()->MissionCancel();
	Singleton<IScenarioTracker>()->MissionStart( pMapInfo );
		
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MISSION, StrFmt( "%s;normal", pMapInfo->GetDBID().ToString().c_str() ) );
}

bool CInterfaceEscMenu::ProcessEvent( const SGameMessage &msg )
{
	return CInterfaceScreenBase::ProcessEvent( msg );
}

bool CInterfaceEscMenu::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return CInterfaceScreenBase::StepLocal( bAppActive );
}

// CICEscMenu

void CICEscMenu::PreCreate()
{
}

void CICEscMenu::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICEscMenu::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x170B6B80, CInterfaceEscMenu )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_ESC_MENU, CICEscMenu )
REGISTER_SAVELOAD_CLASS_NM( 0x170B6B81, CReactions, CInterfaceEscMenu );

