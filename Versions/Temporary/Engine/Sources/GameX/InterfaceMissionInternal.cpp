#include "stdafx.h"

#include "MemoryLib/MemoryLib_export.h"

#include "UI/UI.h"
#include "Misc/StrProc.h"
#include "stats_b2_m1/rpgstatsautomagic.h"
#include "Stats_B2_M1/DBClientConsts.h"
#include "InterfaceMissionInternal.h"
#include "InterfaceMisc.h"

#include "WorldClient.h"
#include "GameXClassIDs.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "ScenarioTracker.h"
#include "Stats_B2_M1/DBCameraConsts.h"
#include "AICmdsAutoMagic.h"
#include "InterfaceState.h"

#include "AILogic/B2AI.h"
#include "Transceiver.h"
#include "SceneB2/Cursor.h"
#include "Sound/SoundScene.h"
#include "Sound/musicsystem.h"
#include "System/Commands.h"
#include "UI/SceneClassIDs.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "UI/Window.h"

#include "SceneB2/WindController.h"
#include "SceneB2/FullScreenFader.h"

#include "Main/MainLoopCommands.h"
#include "3Dmotor/LoadingCounter.h"
#include "InterfaceNotifications.h"
#include "MultiplayerCommandManager.h"
#include "MissionReinforcements.h"
#include "MissionUnitFullInfo.h"
#include "SceneB2/StatSystem.h"
#include "SaveLoadHelper.h"
#include "System/FileUtils.h"
#include "InterfaceMPLoading.h"
#include "GetConsts.h"
#include "DBMPConsts.h"

#include "3Dmotor/FrameTransition.h"
#include "libdb/Db.h"
#include "System/Text.h"
#include "SuperWeapon.hpp"
#include "GetConsts.h"
#include "DBGameRoot.h"
#include "Misc/Win32Random.h"
#include "System/Text.h"

#include "B2_M1_World/ClientAckManager.h"

#include <algorithm>


static int WARFOG_HARD_RECT_WIDTH = 2;
static int WARFOG_MIN_VALUE = 128;
static int WARFOG_UPDATE_PERIOD = 1000;

static int CHAT_MESSAGE_VISIBLE_TIME = 5000;
static int CHAT_MESSAGE_MAX_VISIBLE_TIME = 15000;
static int CHAT_MESSAGE_SPEED = 20; // pixels per second
static int CHAT_MESSAGE_MAX_SPEED = 60; // pixels per second
static int CHAT_MESSAGE_INTERVAL = 5;

static std::wstring MISSION_BUTTONS_BIND_SECTION;

static int s_nTransitionEffectDuration = 800;
static bool s_bEnableScrollTransition = false;

static bool s_bDeepMapLoad = false;

const int TAB_MULTI_SELECT				= 0;
const int TAB_SINGLE_SELECT				= 3;
const int TAB_REINF_SELECT				= 1;
const int TAB_SUPER_WEAPON_SELECT	= 4;

const int ACTION_TAB_SELECT						= 0;
const int ACTION_TAB_CANCEL						= 1;
const int ACTION_TAB_FORMATIONS				= 2;
const int ACTION_TAB_REINF						= 3;
const int ACTION_TAB_SUPER_WEAPON			= 3;
const int ACTION_TAB_RADIO_CONTROLLED	= 5;

const int TAB_APPEARANCE_SINGLE_SELECT	= 0;
const int TAB_APPEARANCE_REINF					= 1;
const int TAB_APPEARANCE_SUPER_WEAPON		= 2;

static std::vector< SMiniMapUnitInfo > s_unitsForMiniMap;

void DoQuickSave();
void DoQuickLoad();

static void SetRawWarfog( BYTE value )
{
	CArray2D<BYTE> warFog( 1, 1 );
	warFog.FillEvery( value );
	Scene()->SetWarFog( warFog, 1.0f );
}

MEMORYLIB_EXPORT void DumpMemoryStats();
static void MsgDumpMemoryStats( const SGameMessage &msg )
{
	DumpMemoryStats();
}

static const NDb::SMapInfo *GetMapInfo( const std::string &_szParam )
{
	NI_VERIFY( !NStr::IsDecNumber(_szParam), "Deprecated way to identify resources! Use DBID instead!", return 0 );
	{
		std::string szParam = _szParam;
		NStr::TrimBoth( szParam, '\"' );
		return NDb::Get<NDb::SMapInfo>( CDBID(szParam) );
	}
}

static DWORD ConvertColor( const CVec3 &vColor )
{
	const int r = vColor.r * 255.0f;
	const int g = vColor.g * 255.0f;
	const int b = vColor.b * 255.0f;

  return 0xFF000000 + (( r & 0xFF ) << 16) + (( g & 0xFF ) << 8) + ( b & 0xFF );
}

class CDrawProgress : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CDrawProgress )
public:
	enum ECounter
	{
		EC_MATERIALS,
		EC_TERRAINS,
		EC_TEXTURES,
	};
private:
	class CLoadingCounter;

	CPtr<IProgressHookB2> pProgress;
	CObj<CLoadingCounter> pMaterials;
	CObj<CLoadingCounter> pTerrains;
	CObj<CLoadingCounter> pTextures;
public:
	CDrawProgress() {}
	CDrawProgress( IProgressHookB2 *pProgress );

	CLoadingCounter* GetTextures() const { return pTextures; }

	void OnTotalCount( ECounter eCounter, int nTotalCount );
	void OnCount( ECounter eCounter, int nCount );
};

class CDrawProgress::CLoadingCounter : public ILoadingCounter
{
	OBJECT_NOCOPY_METHODS( CLoadingCounter );
	CPtr<CDrawProgress> pOwner;
	CDrawProgress::ECounter eCounter;
	int nLeftCount;
	int nTotalCount;
public:
	CLoadingCounter() {}
	CLoadingCounter( CDrawProgress *pOwner, CDrawProgress::ECounter eCounter );

	//{ ILoadingCounter
	void LeftToLoad( int nLeftCount );
	void SetTotalCount( int nTotalCount );
	void Step();
	//}
};

class CICLoadB2 : public CICLoadBase
{
	OBJECT_NOCOPY_METHODS( CICLoadB2 );

	CObj<IProgressHookB2> pProgress;
public:
	CICLoadB2() {}
	CICLoadB2( const std::string &szFileName );

	void OnProgress( EStage eStage );
};

class CICSaveB2 : public CICSaveBase
{
	OBJECT_NOCOPY_METHODS( CICSaveB2 );

	ZDATA_(CICSaveBase)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CICSaveBase*)this); return 0; }
public:
	CICSaveB2() {}
	CICSaveB2( const std::string &szFileName );

	void OnProgress( EStage eStage );
};

// ************************************************************************************************************************ //
// **
// ** observers
// **
// **
// **
// ************************************************************************************************************************ //

void MsgChangeGameSpeed( const SGameMessage &msg, int nAdd );

// CInterfaceMission::CReactions

bool CInterfaceMission::CReactions::Execute( const std::string &szSender, const std::string &szReaction, WORD wKeyboardFlags )
{
	if ( szReaction == "FullInfoMember" )
		return pInterface->OnClickFullInfoMember( szSender );
	if ( szReaction == "FullInfoMemberOverOn" )
		return pInterface->OnFullInfoMemberOverOn( szSender );
	if ( szReaction == "FullInfoMemberOverOff" )
		return pInterface->OnFullInfoMemberOverOff( szSender );
	if ( szReaction == "FullInfoWeaponOverOn" )
		return pInterface->OnFullInfoWeaponOverOn( szSender );
	if ( szReaction == "FullInfoWeaponOverOff" )
		return pInterface->OnFullInfoWeaponOverOff( szSender );

	if ( szReaction == "objective_notify_closed" )
	{
		NInput::PostEvent( "objective_notify_closed", 0, 0 );
		return true;
	}

	if ( szReaction == "ReinfSelect" )
		return pInterface->OnReinfSelect( szSender );
	if ( szReaction == "ReinfSelectDblClick" )
		return pInterface->OnReinfSelectDblClick( szSender );
	if ( szReaction == "ToggleReinf" )
		return pInterface->OnToggleReinf( szSender );
	if ( szReaction == "reinf_unit_info" )
		return pInterface->OnReinfUnitInfo( szSender );
	if ( szReaction == "ReinfFullInfoBack" )
		return pInterface->OnReinfFullInfoBack( szSender );
	if ( szReaction == "ReinfMouseOverForward" )
		return pInterface->OnReinfMouseOverForward( szSender );
	if ( szReaction == "ReinfMouseOverBackward" )
		return pInterface->OnReinfMouseOverBackward( szSender );
	if ( szReaction == "call_reinf_mode" )
		return pInterface->OnReinfCallMode( szSender );
	if ( szReaction == "auto_show_reinf_on" )
		return pInterface->OnReinfAutoShowReinf( szSender, true );
	if ( szReaction == "auto_show_reinf_off" )
		return pInterface->OnReinfAutoShowReinf( szSender, false );

	if ( szReaction == "MissionSelectUnit" )
		return pInterface->OnClickMultiSelectUnit( szSender, wKeyboardFlags );

	if ( szReaction == "MissionSelectSpecialGroup" )
		return pInterface->OnSelectSpecialGroup( szSender, wKeyboardFlags );
	if ( szReaction == "MissionUnselectSpecialGroup" )
		return pInterface->OnUnselectSpecialGroup( szSender, wKeyboardFlags );

	if ( szReaction == "MissionNewActionButtonClick" )
		return pInterface->OnNewActionButtonClick( szSender, wKeyboardFlags );
	if ( szReaction == "MissionNewActionButtonRightClick" )
		return pInterface->OnNewActionButtonRightClick( szSender, wKeyboardFlags );

	if ( szReaction == "NotificationEventLeft" )
		return pInterface->OnNotificationEventBtn( szSender, false );
	if ( szReaction == "NotificationEventRight" )
		return pInterface->OnNotificationEventBtn( szSender, true );

	if ( szReaction == "enter_pressed" )
		return pInterface->OnEnterPressed( wKeyboardFlags );
	if ( szReaction == "chat_input_enter_pressed" )
		return pInterface->OnChatInputEnterPressed();
	if ( szReaction == "chat_input_esc_pressed" )
		return pInterface->OnChatInputEscPressed();
	if ( szReaction == "chat_input_focus_lost" )
		return pInterface->OnChatInputFocusLost();

	return false;
}

// CInterfaceMission

CInterfaceMission::CInterfaceMission()
: CInterfaceScreenBase( "Mission", "mission" ), nCurrentSlot( 0 ), nCurrentIconSlot( 0 ),
	bScreenLoaded( false ), nCurrentReinfPoint( -1 ), bFrozen( false ), bEscMenuPressed( false ),
	nGameSpeed( 0 ), bMultifunctionPanelMinimized( false ),
	eActivePanel( NDb::ACTION_BTN_PANEL_DEFAULT ),
	eActiveAction( NDb::USER_ACTION_UNKNOWN ),
	vPrevCameraLine( VNULL3 ),
	timeMissionLastCheck( 0 ),
	nFrameTransition(0),
	eUIState( EUIS_NORMAL ),
	timeAbsLast( 0 ),
	eSkipState( ESMS_NONE ),
	bScriptMoivie( false ),
	fFormerVolume( 0 ),
	bCheckShowHelpScreen( true ),
	bMovieMode( false ),
	bTryExitWindows( false )
{
	pMission = 0;
	AddObserver( "inc_game_speed", &MsgChangeGameSpeed, +1 );
	AddObserver( "dec_game_speed", &MsgChangeGameSpeed, -1 );
	AddObserver( "toggle_war_fog", &CInterfaceMission::MsgToggleWarFog );

	// new reinf system
	AddObserver( "new_update_reinf_avail", &CInterfaceMission::MsgNewUpdateReinfAvail );
	AddObserver( "new_update_reinf_point", &CInterfaceMission::MsgNewUpdateReinfPoint );
	AddObserver( "forced_action_call_reinf", &CInterfaceMission::MsgForcedActionCallReinf );
	AddObserver( "reset_target", &CInterfaceMission::MsgResetTarget );
	AddObserver( "reinf_update_minimap_pos", &CInterfaceMission::MsgUpdateMinimapPos );
	AddObserver( "reinf_mode", &CInterfaceMission::MsgReinfMode );
	AddObserver( "forced_action_call_no_reinf", &CInterfaceMission::MsgForcedActionCallNoReinf );
	AddObserver( "bad_weather", &CInterfaceMission::MsgBadWeather );
	AddObserver( "avia_returns", &CInterfaceMission::MsgAviaReturns );

	// super weapon
	AddObserver( "forced_action_call_super_weapon", &CInterfaceMission::MsgForcedActionCallSuperWeapon );

	//
	AddObserver( "user_ability_slot_00", &CInterfaceMission::MsgUserAbilitySlot, 0 );
	AddObserver( "user_ability_slot_01", &CInterfaceMission::MsgUserAbilitySlot, 1 );
	AddObserver( "user_ability_slot_02", &CInterfaceMission::MsgUserAbilitySlot, 2 );
	AddObserver( "user_ability_slot_03", &CInterfaceMission::MsgUserAbilitySlot, 3 );

	AddObserver( "mission_multistate_panel_minimize", &CInterfaceMission::MsgMultistatePanelMinimize );
	AddObserver( "mission_multistate_panel_maximize", &CInterfaceMission::MsgMultistatePanelMaximize );
	AddObserver( "mission_toggle_multistate_panel", &CInterfaceMission::MsgMultistatePanelToggle );

	AddObserver( "mission_update_special_select_btn", &CInterfaceMission::MsgUpdateSpecialSelectBtn );

	AddObserver( "new_reset_forced_action", &CInterfaceMission::MsgResetForcedAction );

	AddObserver( "MissionUnitViewClick", &CInterfaceMission::MsgUnitViewClick );

	AddObserver( "mission_update_win_loose_state", &CInterfaceMission::MsgUpdateWinLooseState );

	AddObserver( "multiplayer_win", &CInterfaceMission::MsgMultiplayerWin );
	AddObserver( "multiplayer_loose", &CInterfaceMission::MsgMultiplayerLoose );

	AddObserver( "scroll_map", &CInterfaceMission::MsgScrollMap );

	AddImportantObserver( "win_mouse_move", &CInterfaceMission::MsgOnBeforeMouseMove );

	AddObserver( "message_box_ok", &CInterfaceMission::MsgMessageBoxOk );

	AddObserver( "notification_open_reinf", &CInterfaceMission::MsgNotificationOpenReinf );

	AddObserver( "mission_remove_player", &CInterfaceMission::MsgOnRemovePlayer );

	AddObserver( "notifications_camera_back", &CInterfaceMission::MsgNotificationsCameraBack );

	AddObserver( "multiplayer_pause", &CInterfaceMission::MsgOnMultiplayerPause );

	AddObserver( "script_blink_action_button", &CInterfaceMission::MsgOnScriptBlinkActionButton );
	AddObserver( "blink_objective_button", &CInterfaceMission::MsgBlinkObjectiveBtn );

	scriptMovieMessageProcessor.AddObserver( "skip_movie", &CInterfaceMission::TrySkipMovie );

#ifndef _FINALRELEASE
	AddObserver( "test_command", &CInterfaceMission::MsgTestCommand );
#endif //_FINALRELEASE
	timeLastWarFogUpdate = 0;
	// here we are calling this function to link with module Stats_B2_M1
	StatsB2M1LinkCheatFunction();
	nChatTime = 0;

	s_unitsForMiniMap.clear();
}

CInterfaceMission::~CInterfaceMission()
{
	pReinf = 0;
	pUnitFullInfo = 0;
//	if ( pScreen )
//		Scene()->RemoveScreen( pScreen );
	pReactions = 0;
	pNotifications = 0;
	if ( pWorld )
		pWorld->Clear();
	pWorld = 0;

	if ( pMovieBorder )
	{
		Scene()->RemoveScreen( pMovieBorder );
		pMovieBorder = 0;
	}

	Singleton<IAILogic>()->Clear();
	Singleton<ISoundScene>()->ClearSounds();
	Singleton<IMusicSystem>()->Clear();
	Singleton<IClientAckManager>()->Clear();
	GameTimer()->SetSpeed( 0 );
}

bool CInterfaceMission::Init()
{
	Singleton<IGameTimer>()->Pause( false, PAUSE_TYPE_USER_PAUSE );
	NInput::PostEvent( "show_game_paused", 0, 0 );

	RegisterObservers();
	nLastStepTime = GameTimer()->GetGameTime();
	bNeedWarFogRecalc = false;
	bShowWarFog = true;
	bFrozen = false;

	fBorderScrollX = NGlobal::GetVar( "border_scroll_x", 0.05f );
	fBorderScrollY = NGlobal::GetVar( "border_scroll_y", 0.05f );
	bAllowBorderScroll = false;

	bMultiSelectSubMode = true;
	bPreSelectSubMode = false;

	pReinf = new CMissionReinf();
	pUnitFullInfo = new CMissionUnitFullInfo();
	pSuperWeapon = new CMissionSuperWeapon();

	if ( CInterfaceScreenBase::Init() == false )
		return false;
	//
	return true;
}

void CInterfaceMission::RegisterObservers()
{
	AddObserver( "win_loose", &CInterfaceMission::MsgWinLoose );

	AddObserver( "esc_menu", &CInterfaceMission::MsgEscMenu );
	AddObserver( "message_box_ok", &CInterfaceMission::MsgOk );
	AddObserver( "message_box_cancel", &CInterfaceMission::MsgCancel );

	AddObserver( "try_exit_windows", &CInterfaceMission::MsgTryExitWindows );
	AddObserver( "show_objectives", &CInterfaceMission::MsgShowObjectives );

	AddObserver( "mission_select_mode", &CInterfaceMission::MsgSelectMode );
	AddObserver( "mission_multi_select_mode", &CInterfaceMission::MsgMultiSelectMode );
	AddObserver( "mission_single_select_mode", &CInterfaceMission::MsgSingleSelectMode );
	AddObserver( "mission_preselect_mode", &CInterfaceMission::MsgPreSelectMode );
	AddObserver( "mission_update_single_unit", &CInterfaceMission::MsgUpdateSingleUnit );
	AddObserver( "mission_update_unit_stats", &CInterfaceMission::MsgUpdateUnitStats );
	AddObserver( "mission_update_super_weapon_stats", &CInterfaceMission::MsgUpdateSuperWeaponStats );
	AddObserver( "update_buttons", &CInterfaceMission::MsgUpdateButtons );
	AddObserver( "update_icon", &CInterfaceMission::MsgUnpdateIcon );
	AddObserver( "highlight_units", &CInterfaceMission::MsgHighlightUnits );

	AddObserver( "set_ability_state", &CInterfaceMission::MsgSetAbilityState );
	AddObserver( "set_ability_param", &CInterfaceMission::MsgSetAbilityParam );

	AddObserver( "minimap_show_objectives", &CInterfaceMission::MsgMiniMapShowObjectives );
	AddObserver( "minimap_hide_objectives", &CInterfaceMission::MsgMiniMapHideObjectives );

	AddObserver( "quicksave", &CInterfaceMission::MsgQuickSave );
	AddObserver( "quickload", &CInterfaceMission::MsgQuickLoad );

	AddObserver( "show_game_paused", &CInterfaceMission::MsgShowGamePaused );
	AddObserver( "game_pause", &CInterfaceMission::MsgToggleGamePause );
	AddObserver( "begin_script_movie_sequence", &CInterfaceMission::MsgBeginScriptMovieSequence );

	AddObserver( "dump_mem_stats", &MsgDumpMemoryStats );
}

void CInterfaceMission::MsgToggleGamePause( const SGameMessage &msg )
{
	if ( !pScenarioTracker ||
		pScenarioTracker->GetGameType() == IAIScenarioTracker::EGT_SINGLE ||
		NGlobal::GetVar( "mp_allow_pause", 0 ) == 1 ||
		NGlobal::GetVar( "History.Playing", 0 ) == 1 )
	{
		bool bGamePaused = Singleton<IGameTimer>()->HasPause( PAUSE_TYPE_USER_PAUSE );
		bGamePaused = !bGamePaused;
		//NGlobal::SetVar( "temp.no_visual_updates", bGamePaused, STORAGE_SAVE );
		Singleton<IGameTimer>()->Pause( bGamePaused, PAUSE_TYPE_USER_PAUSE );
		NInput::PostEvent( "show_game_paused", 0, 0 );
	}
}

void CInterfaceMission::MsgNotificationsCameraBack( const SGameMessage &msg )
{
	pNotifications->Notify( EVNT_CAMERA_BACK, -1, VNULL2 );
}

void CInterfaceMission::InitMinimapColors( const NDb::SUIConstsB2 *pUIConsts )
{
	//раздача цветов
	int nPlayerCount = pScenarioTracker->GetNPlayers();
	std::vector<CVec4> minimapColors;
	minimapColors.resize( nPlayerCount );
	for ( int nID = 0; nID < nPlayerCount; ++nID )
	{
		const IScenarioTracker::SPlayerColor &color = pScenarioTracker->GetPlayerColor( nID );
		DWORD dwColor = color.dwColor;

		minimapColors[nID] = CVec4( ((dwColor >> 16) & 0xFF) / 255.0f,
			((dwColor >> 8) & 0xFF) / 255.0f, (dwColor & 0xFF) / 255.0f, 1.0f );
		if ( pMiniMap )
			pMiniMap->SetPlayerColor( nID, NGfx::SPixel8888( dwColor ) );
	}
	pWorld->SetMinimapColors( minimapColors );
}

void CInterfaceMission::OnGetFocus( bool bFocus )
{
	if ( bFocus && InterfaceState()->IsTransitEffectFlag() )
	{
		InterfaceState()->SetTransitEffectFlag( false );
		eUIState = EUIS_ON_ENTER_TRANSIT_START;
	}

	if ( !bFocus )
		NInput::PostEvent( "camera_mouse_rotate_finish", 0, 0 );
	//
	bool bIsSuppressEnableFocus = InterfaceState()->IsSuppressEnableFocus();

	CInterfaceScreenBase::OnGetFocus( bFocus );

	if ( bIsSuppressEnableFocus )
		return;

	if ( pWorld )
		pWorld->OnGetFocus( bFocus );
	bAllowBorderScroll = bFocus;
	if ( !bAllowBorderScroll )
		CheckMouseBorder( VNULL2, bAllowBorderScroll );

	UpdateInterfacePause();
}

void CInterfaceMission::RestoreBindSection()
{
	std::vector<std::string> sections;
	sections.push_back( GetBindSection() );
	szButtonsBindSection = NStr::ToMBCS( MISSION_BUTTONS_BIND_SECTION );
	if ( !szButtonsBindSection.empty() )
		sections.push_back( szButtonsBindSection );
	NInput::SetSection( sections );
}

void CInterfaceMission::StartInterface()
{
	CInterfaceScreenBase::StartInterface();
	//
}

bool CInterfaceMission::ProcessEvent( const struct SGameMessage &msg )
{
	// don't allow input messages furhter
	if ( msg.mMessage.cType != NInput::CT_UNKNOWN &&
			 ( NGlobal::GetVar( "temp.script_movie", false ) != 0 || ( pMovieBorder && pMovieBorder->IsVisible() ) ) )
	{
		if ( scriptMovieMessageProcessor.ProcessEvent( msg, this ) )
			return true;
		return false;
	}

	if ( CInterfaceScreenBase::ProcessEvent( msg ) )
		return true;

	if ( !pWorld )
		return false;


	return pWorld->ProcessEvent( msg );
}

void CInterfaceMission::AddActionButton( NDb::EUserAction eUserAction, IWindow *pWnd )
{
	NI_ASSERT( eUserAction != NDb::USER_ACTION_UNKNOWN, "Trying to link button and NDb::USER_ACTION_UNKNOWN" );
	if ( pWnd != 0 && eUserAction != NDb::USER_ACTION_UNKNOWN )
	{
		pWnd->ShowWindow( false );
		actionButtons.push_back( CActionButton( eUserAction, pWnd ) );
	}
}

void CInterfaceMission::SetAbilityState( const NDb::EUserAction eUserAction, const SAbilitySwitchState &state )
{
	CNewActionButtons::iterator it = newActionButtons.find( eUserAction );
	if ( it != newActionButtons.end() )
	{
		SNewActionButton &action = it->second;

		if ( state.eState == EASS_CHANGE_AUTOCAST_ONLY )
			action.curState.bAutocast = state.bAutocast;
		else
			action.curState = state;

		if ( !action.bPassive )
		{
			if ( action.pActiveBorderWnd )
				action.pActiveBorderWnd->ShowWindow( action.curState.eState == EASS_ACTIVE );

			action.Enable( action.curState.eState == EASS_ACTIVE || action.curState.eState == EASS_READY_TO_ON );

			bool bClock = action.curState.eState == EASS_SWITCHING_ON ||
				action.curState.eState == EASS_SWITCHING_OFF ||
				action.curState.eState == EASS_OFF;
			if ( action.pClockWnd )
				action.pClockWnd->ShowWindow( bClock );
			if ( action.pIconFgWnd )
				action.pIconFgWnd->ShowWindow( bClock );

			if ( action.pAutocastWnd )
			{
				action.pAutocastWnd->ShowWindow( action.bAutocast && action.curState.bAutocast );
				action.pAutocastWnd->Run( action.bAutocast && action.curState.bAutocast );
			}
			if ( action.pAutocastBorderWnd )
				action.pAutocastBorderWnd->ShowWindow( action.bAutocast && !action.curState.bAutocast );
		}
	}
}

void CInterfaceMission::NewMission( const NDb::SMapInfo *_pMap, ITransceiver *_pTransceiver, IScenarioTracker *_pScenarioTracker, int nPlayer )
{
	// clear previous chat input
	while ( true )
	{
		std::wstring wszText = InterfaceState()->GetMPChatMessage();
		if ( wszText.empty() )
			break;
	}
	while ( true )
	{
		std::string szReadText;
		DWORD dwColor;
		if ( !ReadFromPipe( PIPE_CHAT, &szReadText, &dwColor ) )
			break;
	}

	// show mission id (for testers and debug purpose)
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "map: %s", _pMap->GetDBID().ToString().c_str() ) );
	NGlobal::SetVar( "Current.Mission", _pMap->GetDBID().ToString(), STORAGE_SAVE );

	pMission = _pMap;

	pScenarioTracker = _pScenarioTracker;

	if ( NGlobal::GetVar( "start_mission_paused", 0 ) )
	{
		Singleton<IGameTimer>()->Pause( true, PAUSE_TYPE_USER_PAUSE );
		NInput::PostEvent( "show_game_paused", 0, 0 );
	}

	szButtonsBindSection = NStr::ToMBCS( MISSION_BUTTONS_BIND_SECTION );

	pMission = _pMap;
	pTransceiver = _pTransceiver;

	NGlobal::SetVar( "MissionIconsMovieMode", 0.0f );

	pScenarioTracker->ClearMissionScriptVars(); // на случай, если запускали не из ScenarioTracker'а

	CObj<IProgressHookB2> pProgress;
	if ( NGlobal::GetVar( "mission_loading_single", 0 ) != 0 )
	{
		if ( const NDb::SMapInfo *pMapInfo = pScenarioTracker->GetCurrentMission() )
		{
			if ( pScenarioTracker->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
			{
				std::wstring wszDesc;
				if ( CHECK_TEXT_NOT_EMPTY_PRE(pMapInfo->,LoadingDescription) )
					wszDesc = GET_TEXT_PRE(pMapInfo->,LoadingDescription);
				std::wstring wszCitation = InterfaceState()->GetRandomCitation();
				pProgress = new CInterfaceLoadingSingle2D( pMapInfo->pLoadingPicture, wszDesc, wszCitation, false );
			}
			else
			{
				CInterfaceMPLoading2D::SParams params;

				params.vMapAISize.x = pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE;
				params.vMapAISize.y = pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE;

				params.pMapPicture = pMapInfo->pLoadingPicture;
				if ( pMapInfo->pMiniMap )
					params.pMinimap = pMapInfo->pMiniMap->pTexture;
				if ( CHECK_TEXT_NOT_EMPTY_PRE(pMapInfo->,LocalizedName) )
					params.wszMapName = GET_TEXT_PRE(pMapInfo->,LocalizedName);

				if ( const IScenarioTracker::SMultiplayerInfo *pInfo = pScenarioTracker->GetMultiplayerInfo() )
				{
					params.wszGameType = pInfo->wszGameType;
					const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();
					for ( int i = 0; i < pInfo->players.size(); ++i )
					{
						const IScenarioTracker::SMultiplayerInfo::SPlayer &scenarioPlayer = pInfo->players[i];

						params.players.push_back( CInterfaceMPLoading2D::SPlayer() );
						CInterfaceMPLoading2D::SPlayer &player = params.players.back();

						player.nPlayerIndex = scenarioPlayer.nIndex;
						player.wszName = scenarioPlayer.wszName;
						player.nLevel = scenarioPlayer.nLevel;
						player.wszRank = scenarioPlayer.wszRank;
						player.nTeam = scenarioPlayer.nTeam;

						if ( pMPConsts && scenarioPlayer.nCountry >= 0 && scenarioPlayer.nCountry < pMPConsts->sides.size() )
							player.pSideListItemIcon = pMPConsts->sides[scenarioPlayer.nCountry].pListItemIcon;
						else
							player.pSideListItemIcon = 0;

						const NDb::SPartyDependentInfo *pPartyInfo = pScenarioTracker->GetPlayerParty( scenarioPlayer.nIndex );
						if ( pPartyInfo )
							player.pSideMinimapIcon = pPartyInfo->pMinimapKeyObjectIcon;
						else
							player.pSideMinimapIcon = 0;

						if ( scenarioPlayer.nIndex >= 0 && scenarioPlayer.nIndex < pMapInfo->players.size() )
						{
							const NDb::SMapPlayerInfo &mapPlayer = pMapInfo->players[scenarioPlayer.nIndex];
							player.vMinimapPos = mapPlayer.vMPStartPos;
						}
						else
							player.vMinimapPos = VNULL2;
					}
				}
				pProgress = new CInterfaceMPLoading2D( params );
			}
		}
	}

	const int nCommonSteps = 2;
	const int nLoadMapSubSteps = 6;
	const int nLoadAISubSteps = 6;
	const int nUpdateAISubSteps = 6;
	const int nDrawSubSteps = 6;
	if ( pProgress )
		pProgress->SetNumSteps( nCommonSteps + nLoadMapSubSteps + nLoadAISubSteps + nUpdateAISubSteps + nDrawSubSteps );
	//
	const NDb::SMapInfo *pMapInfo = pMission;
	NI_ASSERT( pMapInfo, StrFmt( "No map info available for mission %s", pMission->GetDBID().ToString().c_str() ) );
	if ( !pMapInfo )
		return;

	DWORD dwChatColor = 0xFFFFFFFF;
	if ( const NDb::SUIConstsB2 *pUIConsts = InterfaceState()->GetUIConsts() )
	{
		for ( std::vector< NDb::SSeasonColor >::const_iterator it = pUIConsts->chatSeasonColors.begin();
			it != pUIConsts->chatSeasonColors.end(); ++it )
		{
			const NDb::SSeasonColor &seasonColor = *it;
			if ( seasonColor.eSeason == pMapInfo->eSeason )
			{
				dwChatColor = ConvertColor( seasonColor.vColor );
				break;
			}
		}
	}
	InterfaceState()->SetMissionConsoleColor( dwChatColor );

	//camera.Init( pMapInfo );

	Camera()->SetFOV( NGameX::GetClientConsts()->pCamera->fFOV );

	const NDb::SUIConstsB2 *pUIConsts = NGameX::GetUIConsts();
	NI_ASSERT( pUIConsts != 0, "UI constants not defined" );

	// Loading screen
	//////////////////////////////////////////////////////////////////////////
	MakeScreen( pMapInfo, pUIConsts );

	// Loading world
	//////////////////////////////////////////////////////////////////////////
	pNotifications = new CVisualNotifications( GetScreen(), pMiniMap,
		CVec2( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE,
		pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE ), Singleton<IAILogic>() );
	pWorld = new CWorldClient( pTransceiver, pNotifications, Singleton<IAILogic>(), pScenarioTracker,
		pSuperWeapon );
	pWorld->SetMaxUnits( iconSlots.size(), iconSlots.size() );

	Scene()->GetWindController()->SetWindParams( pMapInfo->weather.nWindDirection, pMapInfo->weather.nWindForce );

	// interface reinforcements
	pReinf->Init( pScreen, pWorld, pNotifications, pMapInfo->eSeason );

	pSuperWeapon->Init( pScreen, pWorld, pMapInfo->eSeason );
	//
	if ( s_bDeepMapLoad )
		NDb::SetLoadDepth( 100 );
	//
	if ( /*AI*/ true )
	{
		Singleton<IAILogic>()->Init( pTransceiver->GetCheckSumLogger(), pMapInfo, NGameX::GetAIConsts(), pScenarioTracker );
		if ( pProgress )
			pProgress->Step(); // step 1

		if ( pProgress )
			pProgress->LockRange( nLoadMapSubSteps );
		pWorld->LoadMap( pMapInfo );
		if ( pProgress )
			pProgress->UnlockRange(); // step 2

		if ( pProgress )
			pProgress->LockRange( nLoadAISubSteps );
		Singleton<IAILogic>()->SetProgressHook( pProgress );
		Singleton<IAILogic>()->SetMyDiplomacyInfo( Singleton<IScenarioTracker>()->GetPlayerSide( nPlayer ), nPlayer );
		Singleton<IAILogic>()->InitAfterMapLoad( pMapInfo );

		Singleton<IAILogic>()->SetProgressHook( 0 );
		if ( pProgress )
			pProgress->UnlockRange(); // step 3
	}
	else
		pWorld->LoadMap( pMapInfo );
	//
	if ( s_bDeepMapLoad )
		NDb::SetLoadDepth( 1 );
	//
	// Set camera
	if ( nPlayer < pMapInfo->players.size() )
	{
		Camera()->SetAnchor( pMapInfo->players[nPlayer].camera.vAnchor );
		if ( !pMapInfo->players[nPlayer].camera.bUseAnchorOnly )
		{
			Camera()->SetPlacement( pMapInfo->players[nPlayer].camera.fDist,
				pMapInfo->players[nPlayer].camera.fPitch,
				pMapInfo->players[nPlayer].camera.fYaw );
		}
	}
	else
		Camera()->SetAnchor( CVec3(10, 10, 0) );


	InitMinimapColors( pUIConsts );

	UpdateWarFog( 0, true, false );
	if ( NGlobal::GetVar( "no_warfog", 0 ) != 0 )
		SetWarForVisibility( false );

	if ( pProgress )
		pProgress->Step(); // step 4

	// вызовем первый раз этот апдейт при загрузке, поскольку первоначальное создание
	// юнитов занимает ощутимое время
	if ( pProgress )
		pProgress->LockRange( nUpdateAISubSteps );
	pWorld->Update();
	if ( pProgress )
		pProgress->UnlockRange(); // step 5

	Singleton<IAILogic>()->PostMapLoad();
	pWorld->Update();

	// загрузим всю графику до старта миссии
	if ( pProgress )
		pProgress->LockRange( nDrawSubSteps );
	Scene()->SwitchScene( SCENE_MISSION );
	Scene()->GetGView()->WaitForLoad( true );
	CObj<CDrawProgress> pDrawProgress = new CDrawProgress( pProgress );
	Scene()->GetGView()->SetLoadingCounter( 0, 0, pDrawProgress->GetTextures() );
	Scene()->GetGView()->LoadEverything();
	pDrawProgress = 0;
	Scene()->GetGView()->SetLoadingCounter( 0, 0, 0 );
	if ( pProgress )
		pProgress->UnlockRange(); // step 6

	Singleton<IStatSystem>()->UpdateEntry( "MapDesigner", NStr::ToMBCS( GET_TEXT_PRE(pMapInfo->,MapDesigner) ) );

	nMissionTimeMSec = 0;

	PauseIntermission( true );

	if ( Singleton<IScenarioTracker>()->GetGameType() == IAIScenarioTracker::EGT_SINGLE && !Singleton<IScenarioTracker>()->IsCustomMission() )
	{
		for ( int i = 0; i < NDb::_RT_NONE; ++i )
		{
			if ( Singleton<IScenarioTracker>()->IsNewReinf( (NDb::EReinforcementType)( i ) ) )
			{
				IVisualNotifications::SEventParams params;
				params.eEventType = NDb::NEVT_REINF_NEW_TYPE;

				pNotifications->OnEvent( params );
				break;
			}
		}
	}
	Singleton<IMusicSystem>()->Init( pMapInfo->pMusic, 0 );
}

void CInterfaceMission::MakeScreen( const NDb::SMapInfo *pMapInfo, const NDb::SUIConstsB2 *pUIConsts )
{
	if ( !bScreenLoaded )
	{

		pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
		pReactions = new CReactions( pScreen, this );
		AddUIScreen( pScreen, "Mission", pReactions );
		pMovieBorder = MakeObjectVirtual<IScreen>( UI_SCREEN );
		if ( AddUIScreen( pMovieBorder, "MissionMovieBorder", pReactions ) == false )
		{
			pMovieBorder = 0;
			return;
		}
		pMovieBorder->RegisterObservers();
		pMovieBorder->ShowWindow( false );

		Cursor()->Show( true );
		Cursor()->SetMode( NDb::USER_ACTION_UNKNOWN );
		Singleton<ISoundScene>()->ClearSounds();

		// get ability slots
		SSlotPosition slot;
		IWindow *pWnd;
		for ( int i = 1; ( pWnd = pScreen->GetElement( StrFmt( "ability_slot_%d", i ), true )) != 0 ; ++i)
		{
			pWnd->GetPlacement( &slot.x, &slot.y, &slot.sizeX, &slot.sizeY );
			vAbilitySlots.push_back( slot );
		}

		IWindow *pMultiselectWnd = GetChildChecked<IWindow>( pScreen, "multiselect", true );
		IWindow *pTooltipSlotWnd = GetChildChecked<IWindow>( pMultiselectWnd, "SlotTooltipHolder", true );
		IWindow *pTooltipSlotUnitWnd = GetChildChecked<IWindow>( pMultiselectWnd, "SlotUnitTooltipHolder", true );
		if ( pTooltipSlotWnd )
			wszTooltipSlot = pTooltipSlotWnd->GetDBTooltipStr();
		if ( pTooltipSlotUnitWnd )
			wszTooltipSlotUnit = pTooltipSlotUnitWnd->GetDBTooltipStr();

		IScenarioTracker *pST = Singleton<IScenarioTracker>();
		const IScenarioTracker::SPlayerColor &color = pST->GetPlayerColor( pST->GetLocalPlayer() );
		CPtr<NDb::SBackgroundSimpleTexture> pBackground = new NDb::SBackgroundSimpleTexture();
		pBackground->PostLoad( false );
		pBackground->nColor = color.dwColor;
		// get icons slots
		for ( int i = 1; ( pWnd = pMultiselectWnd->GetChild( StrFmt( "unit_slot_%d", i ), true )) != 0 ; ++i)
		{
			IButton *pBtn = dynamic_cast<IButton*>( pWnd );
			if ( !pBtn )
				break;
			pBtn->ShowWindow( false );

			SIconSlot slot;
			slot.pBtn = pBtn;
			slot.pIconWnd = GetChildChecked<IWindow>( pBtn, "icon", true );
			slot.pProgressBar = GetChildChecked<IProgressBar>( pBtn, "HPBar", true );
			slot.pCountView = GetChildChecked<ITextView>( pBtn, "UnitSlotCount", true );
			slot.pFlagBgWnd = GetChildChecked<IWindow>( pBtn, "MultiselectUnitFlagBg", true );

			if ( slot.pProgressBar )
			{
				slot.pProgressBar->SetForward( pBackground );
			}
			if ( slot.pFlagBgWnd )
				slot.pFlagBgWnd->ShowWindow( false );

			iconSlots.push_back( slot );
		}

		SetArmyPoints( 0 );

		pChatMessages = pScreen->GetElement( "chat_messages", true );
		if ( pChatMessages )
			pChatMessagesElement = pChatMessages->GetChild( "chat_messages_element", true );

		pMultiFunctionTab = GetChildChecked<ITabControl>( pScreen, "multifunction_tab_control", true );
		pActionTab = GetChildChecked<ITabControl>( pScreen, "action_tab_control", true );
		pAppearanceTab = GetChildChecked<ITabControl>( pScreen, "AppearanceTabControl", true );

		pPause = GetChildChecked<IWindow>( pScreen, "Pause", true );

		//{ get action buttons
		ITabControl *pTabCtrl = GetChildChecked<ITabControl>( pScreen, "action_tab_control", true );

		for ( int i = 0; i <  pUIConsts->actionButtons.size(); ++i )
		{
			NI_ASSERT( pUIConsts->actionButtons[i].pButton != 0, StrFmt("Empty button %d in pUIConsts->actionButtons", i) );
			if ( pUIConsts->actionButtons[i].pButton == 0 )
				continue;
			const NDb::EUserAction eAction = pUIConsts->actionButtons[i].eAction;
			if ( eAction == NDb::USER_ACTION_ABILITY )
				continue;
			IWindow *pButton = pScreen->GetElement( pUIConsts->actionButtons[i].pButton->szName, true );
			if ( pButton )
				pButton->ShowWindow( false );
		}
		//}

		if ( IWindow *pTemplatesWnd = GetChildChecked<IWindow>( pScreen, "MissionTemplates", true ) )
		{
			if ( IWindow *pSlotsWnd = GetChildChecked<IWindow>( pScreen, "ActionButtonSlots", true ) )
			{
				for ( int i = 1; ; ++i )
				{
					IWindow *pWnd = pSlotsWnd->GetChild( StrFmt( "Slot%02d", i ), true );
					if ( !pWnd )
						break;
					int nX;
					int nY;
					pWnd->GetPlacement( &nX, &nY, 0, 0 );
					newActionButtonSlots.push_back( CVec2( nX, nY ) );
				}
			}

			if ( pUIConsts )
			{
				if ( IButton *pTemplateActionBtn = GetChildChecked<IButton>( pTemplatesWnd, "ActionButtonTemplate", true ) )
				{
					for ( std::vector< CDBPtr<NDb::SActionButtonInfo> >::const_iterator it = pUIConsts->actionButtonInfos.begin();
						it != pUIConsts->actionButtonInfos.end(); ++it )
					{
						const NDb::SActionButtonInfo *pDBAction = *it;
						if ( !pDBAction )
							continue;
						if ( pDBAction->bNoButton )
							continue;

						if ( IButton *pBtn = dynamic_cast<IButton*>( AddWindowCopy( pTabCtrl, pTemplateActionBtn ) ) )
						{
							SNewActionButton action;

							action.pBtn = pBtn;
							action.bAbility = pDBAction->bIsAbility;
							action.ePanel = pDBAction->ePanel;
							action.eTargetPanel = pDBAction->eTargetPanel;
							action.bPressEffect = pDBAction->bPressEffect;
							action.pIcon = pDBAction->pIcon;
							action.pForegroundIcon = pDBAction->pForegroundIcon;
							action.pIconDisabled = pDBAction->pIconDisabled;
							action.pForegroundIconDisabled = pDBAction->pForegroundIconDisabled;
							action.bAutocast = pDBAction->bAutocast;
							action.bPassive = pDBAction->bPassive;
							action.nSlot = pDBAction->nSlot - 1;
							action.bCurPresent = false;
							action.szHotkeyCmd = pDBAction->szHotkeyCmd;
							if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBAction->,Tooltip) )
								action.wszTooltip = GET_TEXT_PRE(pDBAction->,Tooltip);

//							action.pIconBgWnd = GetChildChecked<IWindow>( pBtn, "IconBg", true );
							action.pIconFgWnd = GetChildChecked<IWindow>( pBtn, "IconFg", true );
							action.pClockWnd = GetChildChecked<IWindowRoundProgressBar>( pBtn, "Clock", true );
							action.pAutocastWnd = GetChildChecked<IWindowFrameSequence>( pBtn, "Autocast", true );
							action.pStaticBorderWnd = GetChildChecked<IWindow>( pBtn, "StaticBorder", true );
							action.pActiveBorderWnd = GetChildChecked<IWindow>( pBtn, "ActiveBorder", true );
							action.pIconBgDisabledWnd = GetChildChecked<IWindow>( pBtn, "IconBgDisabled", true );
							action.pIconFgDisabledWnd = GetChildChecked<IWindow>( pBtn, "IconFgDisabled", true );
							action.pAutocastBorderWnd = GetChildChecked<IWindow>( pBtn, "AutocastBorder", true );
							action.pBlinkWnd = GetChildChecked<IWindow>( pBtn, "Blink", true );

							if ( 0 <= action.nSlot && action.nSlot < newActionButtonSlots.size() )
							{
								CVec2 vPos = newActionButtonSlots[action.nSlot];
								pBtn->SetPlacement( vPos.x, vPos.y, 0, 0, EWPF_POS_X | EWPF_POS_Y );
							}

							if ( pTabCtrl )
							{
								if ( action.ePanel == NDb::ACTION_BTN_PANEL_DEFAULT )
									pTabCtrl->AddElement( ACTION_TAB_SELECT, pBtn );
								else if ( action.ePanel == NDb::ACTION_BTN_PANEL_FORMATIONS )
									pTabCtrl->AddElement( ACTION_TAB_FORMATIONS, pBtn );
								else if ( action.ePanel == NDb::ACTION_BTN_PANEL_RADIO_CONTROLLED )
									pTabCtrl->AddElement( ACTION_TAB_RADIO_CONTROLLED, pBtn );
							}

							if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBAction->,Tooltip) )
								pBtn->SetTooltip( GET_TEXT_PRE(pDBAction->,Tooltip) );

#ifndef _FINALRELEASE
							if ( !CHECK_TEXT_NOT_EMPTY_PRE(pDBAction->,Tooltip) )
								pBtn->SetTooltip( NStr::ToUnicode( StrFmt( "UserAction id: %d", (int)( pDBAction->eAction ) ) ) ); // CRAP - test only
#endif //_FINALRELEASE

							pBtn->ShowWindow( false );
							pBtn->SetName( StrFmt( "NewActionButton%d", (int)( pDBAction->eAction ) ) );
							action.pIconBgDisabledWnd->SetName( StrFmt( "NewActionDisabledButton%d", (int)( pDBAction->eAction ) ) );
							pBtn->RegisterObservers();

//							if ( action.pIconBgWnd )
//								action.pIconBgWnd->ShowWindow( false );
							if ( action.pIconFgWnd )
								action.pIconFgWnd->ShowWindow( false );
							if ( action.pClockWnd )
								action.pClockWnd->ShowWindow( false );

							if ( action.pAutocastWnd )
								action.pAutocastWnd->ShowWindow( action.bAutocast && false );
							if ( action.pAutocastBorderWnd )
								action.pAutocastBorderWnd->ShowWindow( action.bAutocast && true );

							if ( action.pStaticBorderWnd )
								action.pStaticBorderWnd->ShowWindow( action.bPassive );

							if ( action.pActiveBorderWnd )
								action.pActiveBorderWnd->ShowWindow( false );
							if ( action.pIconBgDisabledWnd )
								action.pIconBgDisabledWnd->ShowWindow( false );
							if ( action.pIconFgDisabledWnd )
								action.pIconFgDisabledWnd->ShowWindow( false );
							if ( action.pBlinkWnd )
								action.pBlinkWnd->ShowWindow( false );

							// textures
							if ( !action.bPassive )
							{
								// !passive

								if ( action.pIcon )
									pBtn->SetTexture( action.pIcon );
								if ( action.pIconFgWnd )
								{
									if ( action.pForegroundIcon )
										action.pIconFgWnd->SetTexture( action.pForegroundIcon );
								}
								if ( action.pIconBgDisabledWnd )
								{
									if ( action.pIconDisabled )
										action.pIconBgDisabledWnd->SetTexture( action.pIconDisabled );
								}
								if ( action.pIconFgDisabledWnd )
								{
									if ( action.pForegroundIconDisabled )
										action.pIconFgDisabledWnd->SetTexture( action.pForegroundIconDisabled );
								}
							}
							else
							{
								// passive

								if ( action.pIcon )
									action.pIconBgDisabledWnd->SetTexture( action.pIcon );
							}

							action.Enable( true );

							newActionButtons[pDBAction->eAction] = action;

							AddActionButton( pDBAction->eAction, pBtn );
						}
					}
				}
			}

			RegisterActionObservers();
		}

		IWindow *pLeftPanel = GetChildChecked<IWindow>( pScreen, "MissionMinimapPanel", true );
		if ( pLeftPanel )
		{
			pLeftPanel->ShowWindow( true );

			pMiniMap = GetChildChecked<IMiniMap>( pScreen, "RotableMinimap", true );
			if ( pMiniMap )
			{
				pMiniMap->LoadMap( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE,
					pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE, Singleton<IAILogic>()->VIS_POWER() + 1 );
				pMiniMap->SetMaterial( pMapInfo->pMiniMap );
				pMiniMap->ShowWindow( true );
			}

			pShowObjectives = GetChildChecked<IWindow>( pLeftPanel, "ObjectivesBtn", true );

			pFlareBtn = GetChildChecked<IButton>( pLeftPanel, "FlareBtn", true );
			if ( pFlareBtn )
				pFlareBtn->Enable( Singleton<IScenarioTracker>()->GetGameType() != IAIScenarioTracker::EGT_SINGLE );

			pObjectivesBlink = GetChildChecked<IWindow>( pLeftPanel, "ObjectivesBlink", true );
			if ( pObjectivesBlink )
				pObjectivesBlink->ShowWindow( false );
		}

		const CVec2 vScreenSize = Scene()->GetScreenRect();
		fViewportBottom = vScreenSize.y;

		pMultifunctionWnd = GetChildChecked<IWindow>( pScreen, "MissionMultifunctionPanel", true );
		if ( pMultifunctionWnd )
		{
			pMultifunctionWnd->ShowWindow( true );

			pMinimizeBtn = GetChildChecked<IButton>( pMultifunctionWnd, "MinimizeBtn", true );

			IWindow *pUnitFullInfoWnd = GetChildChecked<IWindow>( pMultifunctionWnd, "UnitFullInfoTab", true );
			IWindow *pAppearanceWnd = GetChildChecked<IWindow>( pScreen, "AppearancePanel", true );
			IWindow *pAppearanceForSelectWnd = GetChildChecked<IWindow>( pAppearanceWnd, "AppearanceForSelect", true );
			pUnitFullInfo->InitByMission( pUnitFullInfoWnd, pAppearanceForSelectWnd, pMapInfo->eSeason );

			CTRect<float> rect = pMultifunctionWnd->GetWindowRect();
			fViewportBottom = rect.y1;
		}

		IWindow *pRightPanel = GetChildChecked<IWindow>( pScreen, "MissionActionsPanel", true );
		if ( pRightPanel )
		{
			pRightPanel->ShowWindow( true );
		}

//		IWindow *pTopCenterPanel = GetChildChecked<IWindow>( pScr, "MissionTopCenterPanel", true );
		if ( pScreen /*&& pTopCenterPanel*/ )
		{
//			pTopCenterPanel->ShowWindow( true );
			// Buttons 01-06 map directly to special-group indices 0-5.
			for ( int i = 1; i <= 6; ++i )
			{
				CDynamicCast<IButton> pBtn = pScreen->GetChild( StrFmt( "Button%02d", i ), true );
				if ( !pBtn )
					break;
				pBtn->ShowWindow( false );
				specialSelectBtns.push_back( pBtn.GetPtr() );
			}
		}

		if ( pScenarioTracker->GetGameType() != IAIScenarioTracker::EGT_SINGLE )
		{
			IWindow *pScoreBoard = GetChildChecked<IWindow>( pScreen, "MultiplayerScoreBoard", true );
			if ( pScoreBoard )
				pScoreBoard->ShowWindow( true );
		}

		chatInput.pPanel = GetChildChecked<IWindow>( pScreen, "ChatInputPanel", true );
		chatInput.pView = GetChildChecked<ITextView>( chatInput.pPanel, "PrefixView", true );
		chatInput.pEdit = GetChildChecked<IEditLine>( chatInput.pPanel, "ChatEdit", true );
		chatInput.bTeamByDefault = false;
		chatInput.bMultifunctionPanelMinimized = false;

		if ( pScreen )
		{
			chatInput.wszAll = pScreen->GetTextEntry( "T_CHAT_INPUT_ALL" );
			chatInput.wszTeam = pScreen->GetTextEntry( "T_CHAT_INPUT_TEAM" );
		}
		if ( chatInput.pEdit )
		{
			int nX, nWidth;
			chatInput.pEdit->GetPlacement( &nX, 0, &nWidth, 0 );
			chatInput.fEditBaseX = nX;
			chatInput.fEditBaseWidth = nWidth;
		}
		if ( chatInput.pPanel )
			chatInput.pPanel->ShowWindow( false );

		InterfaceState()->ClearMPChatMessages();

		bScreenLoaded = true;
	}
}

void CInterfaceMission::MsgToggleWarFog( const SGameMessage &msg )
{
	SetWarForVisibility( !bShowWarFog );
}

void CInterfaceMission::MsgUpdateMinimapPos( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	bool bOnMinimap = msg.nParam2;

	if ( pReinf )
	{
		if ( bOnMinimap )
		{
			pReinf->SetMousePos( vPos );
		}
		pReinf->SetTrackMousePos( bOnMinimap );
		pWorld->SetOnMinimap( bOnMinimap );
	}
}

void CInterfaceMission::MsgReinfMode( const SGameMessage &msg )
{
	if ( msg.nParam1 == 0 )
		SetActionMode( EAM_SELECT );
	else
		SetActionMode( EAM_REINF );
}

void CInterfaceMission::MsgBadWeather( const SGameMessage &msg )
{
	bool bBadWeather = msg.nParam1;
	if ( pReinf )
		pReinf->BadWeather( bBadWeather );
}

void CInterfaceMission::MsgAviaReturns( const SGameMessage &msg )
{
	const NDb::SComplexSoundDesc *pSound = InterfaceState()->GetSoundEntry( "SOUND_BAD_WEATHER_AVIA_BACK" );
	if ( pSound )
		SoundScene()->AddSound( pSound, VNULL3, SFX_INTERFACE, SAM_ADD_N_FORGET, 0, 2 );

	IVisualNotifications::SEventParams params;
	params.nID = -1;
	params.eEventType = NDb::NEVT_AVIA_BAD_WEATHER_RETREAT;
	std::vector<CMapObj*> objects;
	pWorld->GetOwnAvia( &objects );
	params.objects.resize( objects.size() );
	for ( int i = 0; i < objects.size(); ++i )
	{
		CMapObj *pMO = objects[i];
		params.objects[i] = pMO;
	}
	pNotifications->OnEvent( params );
}

void CInterfaceMission::MsgUserAbilitySlot( const SGameMessage &msg, int nParam )
{
	if ( nParam >= lActiveAbilities.size() )
		return;

	CActionButtons::iterator it = lActiveAbilities.begin();
	advance( it, nParam );
	IWindow *pWnd = it->second;
	if ( pWnd && pWnd->IsVisible() )
	{
		bool bEnabled = false;
		NDb::EUserAction eAction = it->first;
		CNewActionButtons::iterator it = newActionButtons.find( eAction );
		if ( it != newActionButtons.end() )
		{
			SNewActionButton &action = it->second;
			bEnabled = action.bEnabled;
		}

		if ( bEnabled )
			OnNewActionButtonClick( pWnd->GetName(), 0 );
	}
}

void CInterfaceMission::MsgMultistatePanelMinimize( const SGameMessage &msg )
{
	if ( msg.nParam1 == 0 )
	{
		MultifunctionPanelMinimize();
	}
	else
	{
		if ( pMinimizeBtn )
			pMinimizeBtn->Enable( true );
	}
}

void CInterfaceMission::MsgMultistatePanelMaximize( const SGameMessage &msg )
{
	if ( msg.nParam1 == 0 )
	{
		MultifunctionPanelMaximize();
	}
	else
	{
		if ( pMinimizeBtn )
			pMinimizeBtn->Enable( true );
	}
}

void CInterfaceMission::MsgMultistatePanelToggle( const SGameMessage &msg )
{
	if ( !pMinimizeBtn->IsEnabled() )
		return;

	if ( bMultifunctionPanelMinimized )
		MultifunctionPanelMaximize();
	else
		MultifunctionPanelMinimize();
}

void CInterfaceMission::MultifunctionPanelMinimize()
{
	if ( !bMultifunctionPanelMinimized )
	{
		NInput::PostEvent( "mission_multistate_panel_minimize_move", 0, 0 );
		NInput::PostEvent( "mission_appearance_panel_minimize_move", 0, 0 );
		NInput::PostEvent( "mission_action_panel_minimize_move", 0, 0 );
		NInput::PostEvent( "mission_reinf_panel_minimize_move", 0, 0 );
		NInput::PostEvent( "mission_minimap_panel_minimize_move", 0, 0 );
		bMultifunctionPanelMinimized = true;
		if ( pMinimizeBtn )
		{
			pMinimizeBtn->SetState( 1 );
			pMinimizeBtn->Enable( false );
		}
	}
}

void CInterfaceMission::MultifunctionPanelMaximize()
{
	if ( bMultifunctionPanelMinimized )
	{
		NInput::PostEvent( "mission_multistate_panel_maximize_move", 0, 0 );
		NInput::PostEvent( "mission_appearance_panel_maximize_move", 0, 0 );
		NInput::PostEvent( "mission_action_panel_maximize_move", 0, 0 );
		NInput::PostEvent( "mission_reinf_panel_maximize_move", 0, 0 );
		NInput::PostEvent( "mission_minimap_panel_maximize_move", 0, 0 );
		bMultifunctionPanelMinimized = false;
		if ( pMinimizeBtn )
		{
			pMinimizeBtn->SetState( 0 );
			pMinimizeBtn->Enable( false );
		}
	}
}

void CInterfaceMission::BlinkActionButton( int nButton, bool bOn )
{
	CNewActionButtons::iterator it = newActionButtons.find( (NDb::EUserAction)( nButton ) );
	if ( it == newActionButtons.end() )
	{
		NI_ASSERT( 0, StrFmt( "Script: button index (%d) out of range (NDb::EUserAction)", nButton ) );
		return;
	}
	SNewActionButton &button = it->second;
	if ( button.pBlinkWnd )
		button.pBlinkWnd->ShowWindow( bOn );
}

void CInterfaceMission::MsgUpdateSpecialSelectBtn( const SGameMessage &msg )
{
	int nIndex = msg.nParam1;
	bool bEnable = msg.nParam2 != 0;
	if ( 0 <= nIndex && nIndex < specialSelectBtns.size() )
	{
		IButton *pBtn = specialSelectBtns[nIndex];
		if ( pBtn )
			pBtn->ShowWindow( bEnable );
	}
}

void CInterfaceMission::MsgResetForcedAction( const SGameMessage &msg )
{
	eActivePanel = NDb::ACTION_BTN_PANEL_DEFAULT;
	eActiveAction = NDb::USER_ACTION_UNKNOWN;
	UpdateActionPanel();

	NInput::PostEvent( "game_reset_forced_action", 0, 0 );
}

void CInterfaceMission::MsgUnitViewClick( const SGameMessage &msg )
{
	if ( pWorld )
		pWorld->CenterSelectedUnit();
}

void CInterfaceMission::MsgUpdateWinLooseState( const SGameMessage &msg )
{
	UpdateWinLooseState();
}

void CInterfaceMission::UpdateWinLooseState()
{
	// reinforcements
	if ( pReinf )
		pReinf->UpdateWinLooseState();

	SetActionMode( EAM_SELECT );

	// objectives
	if ( pShowObjectives && !pScenarioTracker->IsMissionActive() )
		pShowObjectives->Enable( false );
//	NInput::PostEvent( "mission_objectives_close", 0, 0 );
}

void CInterfaceMission::MsgTestCommand( const SGameMessage &msg )
{
	std::string szTestCommand = NStr::ToMBCS( NGlobal::GetVar( "test_command", "" ) );
	if ( szTestCommand == "chat" )
	{
		WriteToPipe( PIPE_CHAT, "MsgTestCommand 1", 0xffff0000 );
		WriteToPipe( PIPE_CHAT,
			"MsgTestCommand 222222222222222222222 33333333333333333333 4444444444444444444444 55555555555555555555555", 0xff0000ff );
	}
	if ( szTestCommand == "esc_menu" )
	{
		NInput::PostEvent( "esc_menu", 0, 0 );
	}
	if ( szTestCommand == "main_menu" )
	{
		NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
	}
	if ( szTestCommand == "mission_objectives" )
	{
		NMainLoop::Command( ML_COMMAND_MISSION_OBJECTIVES, "" );
	}
}

void CInterfaceMission::SetWarForVisibility( const bool _bShowWarFog )
{
	if ( _bShowWarFog != bShowWarFog )
	{
		bShowWarFog = _bShowWarFog;
		Singleton<IAILogic>()->ToggleWarFog( bShowWarFog );
		if ( _bShowWarFog )
			bNeedWarFogRecalc = true;
		else if ( NGlobal::GetVar("no_warfog", 0) == 0 )
			SetRawWarfog( 255 );
	}
}

void CInterfaceMission::UpdateChat( NTimer::STime nDeltaTime )
{
	NTimer::STime nCurrTime = Singleton<IGameTimer>()->GetAbsTime();
	NTimer::STime nDeltaAbsTime = (nChatTime == 0) ? 0 : max( 0, nCurrTime - nChatTime );
	UpdateChatAbs( nDeltaAbsTime );
	nChatTime = nCurrTime;
}

void CInterfaceMission::UpdateChatAbs( NTimer::STime nDeltaTime )
{
	if ( !pChatMessages || !pChatMessagesElement )
		return;

	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( pST->GetGameType() == IScenarioTracker::EGT_MULTI_FLAG_CONTROL )
	{
		while ( true )
		{
			std::wstring wszText = InterfaceState()->GetMPChatMessage();
			if ( wszText.empty() )
				break;
			InterfaceState()->WriteToMissionConsole( wszText );
		}
	}

	int nBaseWidth;
	pChatMessages->GetPlacement( 0, 0, &nBaseWidth, 0 );

	while ( true )
	{
		std::wstring szReadText;
		DWORD dwColor;
		if ( !ReadFromPipe( PIPE_CHAT, &szReadText, &dwColor ) )
			break;

		IWindow *pElement = Singleton<IUIInitialization>()->CreateWindowFromDesc( pChatMessagesElement->GetDesc() );
		pElement->Init();
		pElement->ShowWindow( true );
		pElement->SetPlacement( 0, 0, 100, nBaseWidth, EWPF_ALL );

		IWindow *pViewWindow = pElement->GetChild( "chat_messages_text", true );
		ITextView *pView = dynamic_cast<ITextView*>( pViewWindow );
		NI_ASSERT( pView, "View window for chat messages not found" );
		pView->SetWidth( nBaseWidth );
		DWORD a = (dwColor >> 24) & 0xFF;
		DWORD r = (dwColor >> 16) & 0xFF;
		DWORD g = (dwColor >> 8) & 0xFF;
		DWORD b = (dwColor >> 0) & 0xFF;
		std::wstring szText = NStr::ToUnicode( StrFmt( "<color=%02X%02X%02X%02X>", a, r, g, b ) );
		szText += szReadText;
		pView->SetText( szText.c_str() );
		int nX, nY, nSizeX, nSizeY;
		pViewWindow->GetPlacement( &nX, &nY, &nSizeX, &nSizeY );
		if ( chatMessages.empty() )
			nY = 0;
		else
		{
			int nX2, nY2, nSizeX2, nSizeY2;
			chatMessages.back().pWnd->GetPlacement( &nX2, &nY2, &nSizeX2, &nSizeY2 );
			nY = nY2 + nSizeY2 + CHAT_MESSAGE_INTERVAL;
		}
		pElement->SetPlacement( 0, nY, nSizeX, nSizeY, EWPF_ALL );

		pChatMessages->AddChild( pElement, true );

		chatMessages.push_back( SChatMessage() );
		chatMessages.back().pWnd = pElement;
		chatMessages.back().nVisibleTime = 0;
	}

	// переместим сообщения, долго находившиеся на экране
	for ( std::list<SChatMessage>::iterator it = chatMessages.begin(); it != chatMessages.end(); ++it )
	{
		SChatMessage &el = *it;
		el.nVisibleTime += nDeltaTime;
	}
	if ( !chatMessages.empty() && chatMessages.front().nVisibleTime >= CHAT_MESSAGE_VISIBLE_TIME )
	{
		int nSpeed = (chatMessages.front().nVisibleTime < CHAT_MESSAGE_MAX_VISIBLE_TIME) ?
			CHAT_MESSAGE_SPEED : CHAT_MESSAGE_MAX_SPEED;
		int nDelta = (std::max)( (int)1, (int)(nDeltaTime * nSpeed / 1000) );
		for ( std::list<SChatMessage>::iterator it = chatMessages.begin(); it != chatMessages.end(); )
		{
			SChatMessage &el = *it;
			int nX, nY, nSizeX, nSizeY;
			el.pWnd->GetPlacement( &nX, &nY, &nSizeX, &nSizeY );
			el.pWnd->SetPlacement( nX, nY - nDelta, nSizeX, nSizeY, EWPF_ALL );
			if ( nY - nDelta + nSizeY < 0 )
			{
				pChatMessages->RemoveChild( el.pWnd );
				it = chatMessages.erase( it );
			}
			else
				++it;
		}
	}
}

void CInterfaceMission::UpdateWarFog( NTimer::STime nGameTime, bool bFirst, bool bForced )
{
	const int nMapSizeX = Singleton<IAILogic>()->GetMiniMapWarFogSizeX();
	const int nMapSizeY = Singleton<IAILogic>()->GetMiniMapWarFogSizeY();
	const int nMaxMapSize = (std::max)( nMapSizeX, nMapSizeY );
	const int nWarFogSize = GetNextPow2( nMaxMapSize );
	const float fWarFogCellSize = AI_TILES_IN_VIS_TILE * VIS_TILES_IN_PATCH * VIS_TILE_SIZE / AI_TILES_IN_PATCH;

	CArray2D<BYTE> *pWarFogInfo = 0;
	bool bWarFogUpdated = false;
	if ( bShowWarFog )
	{
		NTimer::STime timeCurrent = GameTimer()->GetGameTime();

		float fBlend = 0;

		if ( bFirst || bForced || timeCurrent > timeLastWarFogUpdate + WARFOG_UPDATE_PERIOD )
		{
			bWarFogUpdated = Singleton<IAILogic>()->GetMiniMapWarForInfo( &pWarFogInfo, bNeedWarFogRecalc || bFirst );
			if ( bWarFogUpdated )
				timeLastWarFogUpdate = timeCurrent;
		}
		else
			bWarFogUpdated = false;

		bNeedWarFogRecalc = false;

		if ( pMiniMap )
		{
			if ( bWarFogUpdated )
				pMiniMap->SetWarFog( pWarFogInfo );
			else if ( bFirst )
			{
				CArray2D<BYTE> warfog( 1, 1 );
				warfog.FillEvery( 0 );
				pMiniMap->SetWarFog( &warfog );
			}
		}

		if ( !bWarFogUpdated && !bFirst )
			fBlend = Clamp( 1.0f * ( timeCurrent - timeLastWarFogUpdate ) / WARFOG_UPDATE_PERIOD, 0.0f, 1.0f );
		else
		{
			const int nWarFogX = 0;
			const int nWarFogY = 0;

			const int nWarFogDefault = WARFOG_MIN_VALUE;
			CArray2D<BYTE> sceneWarFogInfo;
			sceneWarFogInfo.SetSizes( nWarFogSize + 1, nWarFogSize + 1 );
			// зальем всю карту
			sceneWarFogInfo.FillEvery( nWarFogDefault );
			// заполним игровой участок
			if ( bFirst )
			{
				// оставим все серым
			}
			else if ( bWarFogUpdated )
			{
				const int nVisPower = Singleton<IAILogic>()->VIS_POWER();
				for ( int x = 0; x < pWarFogInfo->GetSizeX(); ++x )
					for ( int y = 0; y < pWarFogInfo->GetSizeY(); ++y )
						sceneWarFogInfo[y + nWarFogY][x + nWarFogX] = WARFOG_MIN_VALUE + (255 - WARFOG_MIN_VALUE) * (*pWarFogInfo)[y][x] / nVisPower;
			}
			else
				sceneWarFogInfo.FillEvery( 255 );

			// заполним бордюр
			int nDensity = 0;
			for ( int i = 0; i < WARFOG_HARD_RECT_WIDTH; ++i )
			{
				nDensity = WARFOG_MIN_VALUE * i / ( WARFOG_HARD_RECT_WIDTH - 1 );
				for ( int x = i; x <= nMapSizeX-i; ++x )
				{
					sceneWarFogInfo[i][x] = nDensity;
					sceneWarFogInfo[nMapSizeY - i][x] = nDensity;
				}
				for ( int y = i; y <= nMapSizeY-i; ++y )
				{
					sceneWarFogInfo[y][i] = nDensity;
					sceneWarFogInfo[y][nMapSizeX - i] = nDensity;
				}
			}
			Scene()->SetWarFog( sceneWarFogInfo, 1.0f / fWarFogCellSize );
		}

		Scene()->SetWarFogBlend( fBlend );
	}
}

bool CInterfaceMission::StepLocal( bool bAppActive )
{
	Sleep( 1 );

	if ( IsValid( pTransceiver ) )
		pTransceiver->DoSegments();

	bEscMenuPressed = false;

	UpdateMissionTime( bAppActive );

	if ( !bAppActive )
		return false;

	bool bResult = false;
	//
	pWorld->Update();

	const NTimer::STime nCurrentTime = GameTimer()->GetGameTime();
	const bool bScriptMovieNow = 	NGlobal::GetVar( "temp.script_movie", false );
	if ( bScriptMoivie && !bScriptMovieNow ) // just finished
		EndScriptMovieSequence();
	bScriptMoivie = bScriptMovieNow;

/*	if ( bCheckShowHelpScreen )
	{
		if ( nCurrentTime != 0 && !bMovieMode && eUIState == EUIS_NORMAL )
		{
			CheckedShowHelpScreen( false );
			bCheckShowHelpScreen = false;
		}
	}*/

	const float fFadeTime = 2.0f;

	switch( eSkipState )
	{
	case ESMS_NONE:
		break;
	case ESMS_FADING_OUT:
		{
			Scene()->GetScreenFader()->Start( fFadeTime, SCREEN_FADER_CLEAR, SCREEN_FADER_BLACK );
			eSkipState = ESMS_PREPARE_FOR_FAST_FORWARD;
		}
		break;
	case ESMS_PREPARE_FOR_FAST_FORWARD:
		{
			if ( !Scene()->GetScreenFader()->IsInProgress() )
			{
				fFormerVolume = NGlobal::GetVar( "Sound.SFXVolume", 0.50f );
				NGlobal::SetVar( "Sound.SFXVolume", 0.0f );
				eSkipState = ESMS_FAST_FORWARD;
				GameTimer()->SetSpeed( 100000 );
				timeSkipProgress = nCurrentTime;
			}
			break;
		}
	case ESMS_FAST_FORWARD:
		if ( NGlobal::GetVar( "temp.script_movie", false ) == 0 )
		{
			eSkipState = ESMS_FADING_IN;
			timeSkipProgress = nCurrentTime;
		}

		break;
	case ESMS_FADING_IN:
		{
			Scene()->GetScreenFader()->Start( fFadeTime, SCREEN_FADER_BLACK, SCREEN_FADER_CLEAR );

			eSkipState = ESMS_DONE;
			GameTimer()->SetSpeed( 0 );
			NGlobal::SetVar( "Sound.SFXVolume", fFormerVolume );
		}
		break;
	case ESMS_DONE:
		eSkipState = ESMS_NONE;
		GameTimer()->SetSpeed( 0 );
		NGlobal::SetVar( "Sound.SFXVolume", fFormerVolume );
		break;
	}

	float fStepAbsTime = 0.0f;
	NTimer::STime timeAbsCur = Singleton<IGameTimer>()->GetAbsTime();
	if ( bAppActive && timeAbsLast > 0 )
		fStepAbsTime = (float)( timeAbsCur - timeAbsLast ) / 1000.0f;
	timeAbsLast = timeAbsCur;

	if ( !bFrozen )
	{
		// multi player score
		if ( pScenarioTracker && pScenarioTracker->GetGameType() != IAIScenarioTracker::EGT_SINGLE )
			UpdateMultiplayerScoreBoard();

		if ( pMiniMap )
		{
			std::vector< CVec2 > viewport;
			CVec3 vPos;
			const CVec2 vScreenSize = Scene()->GetScreenRect();
			Scene()->PickZeroHeight( &vPos, CVec2( 0, 0 ) );
			viewport.push_back( CVec2( vPos.x, vPos.y ) );
			Scene()->PickZeroHeight( &vPos, CVec2( vScreenSize.x, 0 ) );
			viewport.push_back( CVec2( vPos.x, vPos.y ) );
			Scene()->PickZeroHeight( &vPos, CVec2( vScreenSize.x, fViewportBottom ) );
			viewport.push_back( CVec2( vPos.x, vPos.y ) );
			Scene()->PickZeroHeight( &vPos, CVec2( 0, fViewportBottom ) );
			viewport.push_back( CVec2( vPos.x, vPos.y ) );
			if ( pMiniMap )
				pMiniMap->SetViewport( viewport );
			//

			CVec3 vCameraLine;
			if ( pWorld->IsCameraUpdated() && vCameraLine != VNULL3 )
			{
				vCameraLine = vPrevCameraLine;
			}
			else
			{
				CVec3 vCameraPos;
				Vis2AI( &vCameraPos, Camera()->GetPos() );
				CVec3 vCameraAnchor;
				Vis2AI( &vCameraAnchor, Camera()->GetAnchor() );
				vCameraLine = vCameraAnchor - vCameraPos;
				vPrevCameraLine = vCameraLine;
			}
			pWorld->ResetCameraUpdated();

			Singleton<IAILogic>()->GetMiniMapUnitsInfo( s_unitsForMiniMap );

			if ( fabs( vCameraLine.z ) < FP_EPSILON )
				pMiniMap->SetUnits( s_unitsForMiniMap );
			else
			{
				std::vector< SMiniMapUnitInfo > units2;
				units2.reserve( s_unitsForMiniMap.size() );
				for ( std::vector< SMiniMapUnitInfo >::iterator it = s_unitsForMiniMap.begin(); it != s_unitsForMiniMap.end(); ++it )
				{
					SMiniMapUnitInfo &info = *it;

					float t = -info.z / vCameraLine.z;
					float x = info.x + vCameraLine.x * t;
					float y = info.y + vCameraLine.y * t;
					if ( !pWorld->IsInsideAIMap( CVec2( x, y ) ) )
						continue;
					units2.push_back( SMiniMapUnitInfo( x, y, 0, info.player, info.radius ) );
				}
				pMiniMap->SetUnits( units2 );
			}

			if ( pWorld->IsAreasShown() )
			{
				// Show areas
				pMiniMap->RemoveAllRangeInfo();
				SShootAreas areas;
				pWorld->GetAreas( &areas );
				pMiniMap->SetRangeInfo( 0, areas );
			}
			else
			{
				pMiniMap->RemoveAllRangeInfo();
			}
		}


		const NTimer::STime nCurrentTime = GameTimer()->GetGameTime();
		const NTimer::STime nDeltaTime = nCurrentTime - nLastStepTime;

		UpdateWarFog( nCurrentTime, false, false );

		UpdateChat( nDeltaTime );

		nLastStepTime = nCurrentTime;

		if ( pNotifications )
			pNotifications->Step( nDeltaTime, bAppActive );

		if ( fStepAbsTime > 0.0f )
			pWorld->UpdateUnitIcons( fStepAbsTime );

		if ( pShowObjectives )
		{
			bool bEnable = pScenarioTracker && pScenarioTracker->IsMissionActive() &&
				pScenarioTracker->GetKnownObjectiveCount() != 0;
			if ( pShowObjectives->IsEnabled() != bEnable )
				pShowObjectives->Enable( bEnable );
		}

		bResult = true;
	}
	else
	{
		nLastStepTime = GameTimer()->GetGameTime();

		bResult = false;
	}

	if ( pReinf )
		pReinf->Step();

	CInterfaceScreenBase::StepLocal( bAppActive );

	Singleton<IMPToUIManager>()->MPUISegment();

	CheckInactiveInput();

	return bResult;
}

void CInterfaceMission::CheckInactiveInput()
{
	// check if input is inactive for N seconds. run command
	const int nSecondsToWait = NGlobal::GetVar( "time_input_inactive", 0 );
	if ( nSecondsToWait )
	{
		if ( GetTickCount() - NInput::GetLastEventTime() >= nSecondsToWait * 1000 )
		{
			if ( NGlobal::GetVar( "inactive_input_reaction_done", 0 ) == 0 )
			{
				NGlobal::SetVar( "inactive_input_reaction_done", 1 );

				std::string szRes = NStr::ToMBCS( NGlobal::GetVar( "input_inactive_command", "" ).GetString() );
				if ( szRes.size() > 2 )
					NStr::TrimBoth( szRes, '\"' );
				WriteToPipe( PIPE_CONSOLE_CMDS, szRes );
			}
		}
		else
		{
			if ( NGlobal::GetVar( "inactive_input_reaction_done", 0 ) == 1 )
			{
				NGlobal::RemoveVar( "inactive_input_reaction_done" );

				std::string szRes = NStr::ToMBCS( NGlobal::GetVar( "input_active_again_command", "" ).GetString() );
				if ( szRes.size() > 2 )
					NStr::TrimBoth( szRes, '\"' );
				WriteToPipe( PIPE_CONSOLE_CMDS, szRes );
			}
		}
	}
}

void CInterfaceMission::UpdateMultiplayerScoreBoard()
{
	if ( !pScenarioTracker )
		return;
	pScenarioTracker->UpdateStatistics();

	const int nPlayer = pScenarioTracker->GetLocalPlayer();
	const int nLocalParty = pScenarioTracker->GetPlayerSide( nPlayer );

	const int nPlayers = pScenarioTracker->GetNPlayers();
	int nScore0 = 0, nScore1 = 0;
	for ( int i = 0; i < nPlayers; ++i )
	{
		const int nPlayerSide = pScenarioTracker->GetPlayerSide( i );
		const int nScore = pScenarioTracker->GetStatistics( i, IScenarioTracker::ESK_SCORE );
		if ( nPlayerSide != 2 )
		{
			if ( nPlayerSide == 0 )
			{
				nScore0 += nScore;
			}
			else
			{
				nScore1 += nScore;
			}
		}
	}
	ITextView *pFriendlyScore = GetChildChecked<ITextView>( pScreen, "FriendlyScore", true );
	ITextView *pEnemyScore = GetChildChecked<ITextView>( pScreen, "EnemyScore", true );
	pEnemyScore->SetText( NStr::ToUnicode( StrFmt( "%i", nLocalParty == 0 ? nScore1 : nScore0 ) ) );
	pFriendlyScore->SetText( NStr::ToUnicode( StrFmt( "%i", nLocalParty == 1 ? nScore1 : nScore0 ) ) );

	std::pair<int,int> buildings = pScenarioTracker->GetKeyBuildingSummary();
	ITextView *pEnemyKeyBuildings = GetChildChecked<ITextView>( pScreen, "EnemyKeyBuildings", true );
	ITextView *pFriendlyKeyBuildings = GetChildChecked<ITextView>( pScreen, "FriendlyKeyBuildings", true );
	pEnemyKeyBuildings->SetText( NStr::ToUnicode( StrFmt( "%i", nLocalParty == 0 ? buildings.second : buildings.first ) ) );
	pFriendlyKeyBuildings->SetText( NStr::ToUnicode( StrFmt( "%i", nLocalParty == 1 ? buildings.second : buildings.first ) ) );

	const int nTimelimit = NGlobal::GetVar( "multiplayer_time_limit", -1 );
	ITextView *pTimeRemaining = GetChildChecked<ITextView>( pScreen, "TimeRemaining", true );
	ITextView *pTimeRemainingTitle = GetChildChecked<ITextView>( pScreen, "TimeRemainingTitle", true );

	pTimeRemaining->ShowWindow( nTimelimit != -1 );
	pTimeRemainingTitle->ShowWindow( nTimelimit != -1 );
	if ( nTimelimit != -1 )
	{
		const int nTimeRemaining =  (std::max)( 0, int(nTimelimit - Singleton<IGameTimer>()->GetGameTime()/1000) );
		int hours = nTimeRemaining / 3600;
		int minutes = (nTimeRemaining % 3600) / 60;
		int seconds = nTimeRemaining % 60;
		pTimeRemaining->SetText( NStr::ToUnicode( StrFmt( "%i:%02i:%02i", hours, minutes, seconds ) ) );
	}

	ITextView *timeForNextReinf = GetChildChecked<ITextView>( pScreen, "RecycleHintTime", true );
	if (timeForNextReinf != nullptr)
	{
		int totalSeconds = pScenarioTracker->GetReinforcementXP( pScenarioTracker->GetLocalPlayer(), NDb::_RT_NONE );

		int minutes = totalSeconds / 60;
		int seconds = totalSeconds % 60;

		timeForNextReinf->SetText(NStr::ToUnicode(StrFmt( "<center><font size = 16pt outlinesize = 1 outlinecolor = black forcefontsize = 1>%d:%02d", minutes, seconds )));

		timeForNextReinf->ShowWindow(true);

		IWindow* parent = timeForNextReinf->GetParentWindow();
		parent->ShowWindow(true);
		parent->SetPlacement(0, 0, 1024, 768, 1);
	}

	ITextView *pTimeToLooseOrWin = GetChildChecked<ITextView>( pScreen, "TimeToLooseOrWin", true );
	ITextView *pWinText = GetChildChecked<ITextView>( pScreen, "WinText", true );
	ITextView *pLooseText = GetChildChecked<ITextView>( pScreen, "LooseText", true );
	int nTimeToLooseOrWin = NGlobal::GetVar( "multiplayer_loss_timeout", -1 );

	if ( nTimeToLooseOrWin != -1 && buildings.first != 0 && buildings.second == 0 )
	{
		pWinText->ShowWindow( nLocalParty == 0 );
		pLooseText->ShowWindow( nLocalParty != 0 );
		pTimeToLooseOrWin->ShowWindow( true );
		pTimeToLooseOrWin->SetText( NStr::ToUnicode( StrFmt( "%i", nTimeToLooseOrWin ) ) );
	}
	else if ( nTimeToLooseOrWin != -1 && buildings.first == 0 && buildings.second != 0 )
	{
		pWinText->ShowWindow( nLocalParty != 0 );
		pLooseText->ShowWindow( nLocalParty == 0 );
		pTimeToLooseOrWin->ShowWindow( true );
		pTimeToLooseOrWin->SetText( NStr::ToUnicode( StrFmt( "%i", nTimeToLooseOrWin ) ) );
	}
	else
	{
		// hide these windows
		pWinText->ShowWindow( false );
		pLooseText->ShowWindow( false );
		pTimeToLooseOrWin->ShowWindow( false );
	}
}

void CInterfaceMission::UpdateInterfacePause()
{
	PauseIntermission( pScenarioTracker &&
		( pScenarioTracker->GetGameType() == IAIScenarioTracker::EGT_SINGLE || NGlobal::GetVar( "History.Playing", 0 ) ) &&
		(!IsInFocus() /*&& !IsMoveSequence() */) );
}

bool CInterfaceMission::IsScreenControl( const CVec2 &vPos ) const
{
	return bScreenLoaded && ( pScreen->Pick( vPos, false ) != 0 );
}

void CInterfaceMission::OnMouseMove( const CVec2 &vPos )
{
	if ( bIsActiveScreen != IsScreenControl( vPos ) )
	{
		if ( bIsActiveScreen )
			pWorld->OnEnter();
		else
			pWorld->OnLeave();

		bIsActiveScreen = !bIsActiveScreen;
	}

	if ( pReinf )
	{
		if ( !bIsActiveScreen )
		{
			CVec3 vZeroPos;
			Scene()->PickZeroHeight( &vZeroPos, vPos );
			pReinf->SetTrackMousePos( true );
			pReinf->SetMousePos( CVec2( vZeroPos.x, vZeroPos.y ) );

			IDebugSingleton *pDebug = Singleton<IDebugSingleton>();
			if ( pDebug )
			{
				IStatsSystemWindow *pStatsSystemWindow = pDebug->GetStatsWindow();
				if ( pStatsSystemWindow )
				{
					pStatsSystemWindow->UpdateEntry( L"TerrainPos", NStr::ToUnicode(StrFmt( "%.2f,%.2f", vZeroPos.x, vZeroPos.y )), 0xff00ff00 );
					CVec3 vTerrainPos;
					Scene()->PickTerrain( &vTerrainPos, vPos );
					pStatsSystemWindow->UpdateEntry( L"MousePos", NStr::ToUnicode(StrFmt( "%.2f x %.2f x %.2f", vTerrainPos.x, vTerrainPos.y, vTerrainPos.z )), 0xffffff00 );
				}
			}
		}
		else if ( !pWorld->IsOnMinimap() )
			pReinf->SetTrackMousePos( false );
	}

	if ( eActiveControl != AC_SCREEN )
		pWorld->DoMouseMove( vPos );
	//if ( eActiveControl != AC_WORLD )
	//	pScreen->OnMouseMove( vPos, 0 );
#ifndef _FINALRELEASE
	std::list<int> objects;
	if ( pWorld->PickMapObj( vPos, &objects ) )
	{
		if ( objects.empty() )
			Singleton<IAILogic>()->PickEmpty();
		else
		{
			while ( !objects.empty() )
			{
				Singleton<IAILogic>()->PickedObj( objects.front() );
				objects.pop_front();
			}
		}
	}
	else
		Singleton<IAILogic>()->PickEmpty();
#endif

	CheckMouseBorder( vPos, bAllowBorderScroll && !NGlobal::GetVar( "script_movie_on", false ) );
}

void CInterfaceMission::CheckMouseBorder( const CVec2 &vPos, const bool bAllowScroll )
{
	ICamera *pCamera = Camera();
	if ( !bAllowScroll )
	{
		pCamera->ResetScrolling();
		return;
	}
	const CVec2 vSize = Scene()->GetScreenRect();
	const int nX = 0, nY = 0, nSizeX = vSize.x, nSizeY = vSize.y;
//	pScreen->GetPlacement( &nX, &nY, &nSizeX, &nSizeY );
	int nDX = 0;
	int nDY = 0;
	if ( vPos.x - nX == 0 )
		nDX = -1;
	if ( vPos.y - nY == 0 )
		nDY = 1;
	if ( vPos.x - nX == nSizeX - 1 )
		nDX = 1;
	if ( vPos.y - nY == nSizeY - 1 )
		nDY = -1;
	if ( nDX == 0 && nDY == 0 )
		pCamera->ResetScrolling();
	else
	{
		pCamera->SetScrollSpeedX( nDX * fBorderScrollX );
		pCamera->SetScrollSpeedY( nDY * fBorderScrollY );
	}
}

void CInterfaceMission::OnButtonDown( const CVec2 &vPos, int nButton )
{
	if ( IsScreenControl( vPos ) )
	{
		eActiveControl = AC_SCREEN;
		//pScreen->OnButtonDown( vPos, nButton );
	}
	else
	{
		eActiveControl = AC_WORLD;
		if ( nButton == MSTATE_BUTTON1 )
			pWorld->DoLButtonDown( vPos );
		else if ( nButton == MSTATE_BUTTON2 )
			pWorld->DoRButtonDown( vPos );
	}
}

void CInterfaceMission::OnButtonUp( const CVec2 &vPos, int nButton )
{
	if ( eActiveControl != AC_SCREEN ) {
		if ( nButton == MSTATE_BUTTON1 )
			pWorld->DoLButtonUp( vPos );
		else if  ( nButton == MSTATE_BUTTON2 )
			pWorld->DoRButtonUp( vPos );
	}
	//if ( eActiveControl != AC_WORLD )
	//	pScreen->OnButtonUp( vPos, nButton );
	eActiveControl = AC_NONE;
}

void CInterfaceMission::OnButtonDblClk( const CVec2 &vPos, int nButton )
{
	//if ( IsScreenControl( vPos ) )
		//pScreen->OnButtonDblClk( vPos, nButton );
	//else
	{
		if ( nButton == MSTATE_BUTTON1 )
			pWorld->DoLButtonDblClk( vPos );
		else if  ( nButton == MSTATE_BUTTON2 )
			pWorld->DoRButtonDblClk( vPos );
	}
}

void CInterfaceMission::ResetAbilityButtons()
{
	for ( CActionButtons::iterator it = lActiveAbilities.begin(); it != lActiveAbilities.end(); ++it )
	{
		it->second->ShowWindow( false );
	}
//	pAbilityProgressBar->ShowWindow( false );
	lActiveAbilities.clear();
	nCurrentSlot = 0;
}

void CInterfaceMission::AddAbilityButton( NDb::EUserAction eAction, IWindow *pWnd, bool bFixedPlace, const NDb::SUnitSpecialAblityDesc *pAbilityDesc )
{
	if ( !pWnd )
		return;

	CNewActionButtons::iterator it = newActionButtons.find( eAction );
	if ( it != newActionButtons.end() )
	{
		SNewActionButton &action = it->second;
		const NDb::STexture *pNormalIcon = action.pIcon;
		const NDb::STexture *pDisabledIcon = action.pIconDisabled;
		const NDb::STexture *pForegroundNormalIcon = action.pForegroundIcon;
		const NDb::STexture *pForegroundDisabledIcon = action.pForegroundIconDisabled;
		if ( pAbilityDesc )
		{
			if ( pAbilityDesc->pAbilityIconTextureNormal )
				pNormalIcon = pAbilityDesc->pAbilityIconTextureNormal;
			if ( pAbilityDesc->pAbilityIconTextureDisabled )
				pDisabledIcon = pAbilityDesc->pAbilityIconTextureDisabled;
			if ( pAbilityDesc->pAbilityIconTextureForegroundNormal )
				pForegroundNormalIcon = pAbilityDesc->pAbilityIconTextureForegroundNormal;
			if ( pAbilityDesc->pAbilityIconTextureForegroundDisabled )
				pForegroundDisabledIcon = pAbilityDesc->pAbilityIconTextureForegroundDisabled;
		}

		// Restore the action textures when the selected ability does not override them.
		if ( !action.bPassive )
		{
			if ( action.pBtn )
				action.pBtn->SetTexture( pNormalIcon );
			if ( action.pIconFgWnd )
				action.pIconFgWnd->SetTexture( pForegroundNormalIcon );
			if ( action.pIconBgDisabledWnd )
				action.pIconBgDisabledWnd->SetTexture( pDisabledIcon );
			if ( action.pIconFgDisabledWnd )
				action.pIconFgDisabledWnd->SetTexture( pForegroundDisabledIcon );
		}
		else if ( action.pIconBgDisabledWnd )
			action.pIconBgDisabledWnd->SetTexture( pNormalIcon );
	}
	if ( bFixedPlace )
	{
		pWnd->ShowWindow( true );
		MakeAbilityTooltip( eAction, nCurrentSlot, pAbilityDesc, true );
	}
	else if ( nCurrentSlot < vAbilitySlots.size() )
	{
		pWnd->SetPlacement( vAbilitySlots[nCurrentSlot].x, vAbilitySlots[nCurrentSlot].y, vAbilitySlots[nCurrentSlot].sizeX, vAbilitySlots[nCurrentSlot].sizeY, EWPF_ALL );
		pWnd->ShowWindow( true );

/*		IProgressBar *pProgress = dynamic_cast<IProgressBar *>( pWnd->GetChild( "ability_progress", true ) );
		if ( pProgress )
			pProgress->SetPosition( 0 );*/

		MakeAbilityTooltip( eAction, nCurrentSlot, pAbilityDesc, false );

		lActiveAbilities.push_back( CActionButton( eAction, pWnd ) );
		nCurrentSlot++;
	}
}

void CInterfaceMission::MsgMultiplayerWin( const SGameMessage &msg )
{
	Singleton<IInterfaceState>()->SendCommandsToCloseAllIncluding( this );
	NMainLoop::Command( ML_COMMAND_HIDE_ALL_UP_TO, "Mission" );
	NMainLoop::Command( ML_COMMAND_HIDE_UNFOCUSED_SCREEN, "" );
	NMainLoop::Command( ML_COMMAND_MP_STATISTICS, "" );
}

void CInterfaceMission::MsgMultiplayerLoose( const SGameMessage &msg )
{
	Singleton<IInterfaceState>()->SendCommandsToCloseAllIncluding( this );
	NMainLoop::Command( ML_COMMAND_HIDE_ALL_UP_TO, "Mission" );
	NMainLoop::Command( ML_COMMAND_HIDE_UNFOCUSED_SCREEN, "" );
	NMainLoop::Command( ML_COMMAND_MP_STATISTICS, "" );
}

bool CInterfaceMission::MsgOnBeforeMouseMove( const SGameMessage &msg )
{
	pWorld->SetOnMinimap( false );

	return false;
}

void CInterfaceMission::MsgMessageBoxOk( const SGameMessage &msg )
{
	// Пока только уведомления (CVisualNotifications) - не делаем ничего
}

void CInterfaceMission::MsgNotificationOpenReinf( const SGameMessage &msg )
{
//	if ( !pReinf->IsOpen() && pReinf->IsEnabled() && pReinf->IsAviaPresents() )
//		OnReinfShow( "notifications" );
}

void CInterfaceMission::MsgOnMultiplayerPause( const SGameMessage &msg )
{
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_MP_PAUSE );
}

void CInterfaceMission::MsgOnScriptBlinkActionButton( const SGameMessage &msg )
{
	BlinkActionButton( msg.nParam1, (msg.nParam2 != 0) );
}

void CInterfaceMission::MsgBlinkObjectiveBtn( const SGameMessage &msg )
{
	bool bBlink = msg.nParam1;
	if ( pObjectivesBlink )
		pObjectivesBlink->ShowWindow( bBlink );
}

void CInterfaceMission::MsgOnRemovePlayer( const SGameMessage &msg )
{
	int nPlayer = msg.nParam1;

	IVisualNotifications::SEventParams params;
	params.nID = nPlayer;
	params.eEventType = NDb::NEVT_PLAYER_ELIMINATED;
	pNotifications->OnEvent( params );
}

bool CInterfaceMission::MsgScrollMap( const SGameMessage &msg )
{
	if ( s_bEnableScrollTransition )
	{
		vFrameTransitionTo = UnPackCoords( msg.nParam1 );
		nFrameTransition = 1;
	}
	else
	{
		NInput::PostEvent( "scroll_map_true", msg.nParam1, msg.nParam2 );
		nFrameTransition = 0;
	}

	return false;
}

void CInterfaceMission::MsgWinLoose( const SGameMessage &msg )
{
	if ( !Singleton<IScenarioTracker>()->IsMissionActive() )
		return;

	bool bWin = (msg.nParam1 == 1);

	NMainLoop::Command( ML_COMMAND_WIN_LOOSE, bWin ? "win" : "lose" );

	if ( bWin )
		Singleton<IScenarioTracker>()->MissionWin();
	else
		Singleton<IScenarioTracker>()->MissionCancel();
	UpdateWinLooseState();

	IWindow *pScreen = GetScreen();
	if ( pScreen )
		pScreen->ShowWindow( false );
}

void CInterfaceMission::MsgEscMenu( const SGameMessage &msg )
{
	if ( bEscMenuPressed )
		return;
	bEscMenuPressed = true;
	NMainLoop::Command( ML_COMMAND_ESC_MENU, "" );
	//if ( IsMoveSequence() )
		//Cursor()->Show( true );
}

void CInterfaceMission::MsgOk( const SGameMessage &msg )
{
	if ( bTryExitWindows )
		NInput::PostEvent( "exit", 0, 0 );
}

void CInterfaceMission::MsgCancel( const SGameMessage &msg )
{
	bTryExitWindows = false;
}

void CInterfaceMission::MsgTryExitWindows( const SGameMessage &msg )
{
	bTryExitWindows = true;
	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX,
		CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel",
		InterfaceState()->GetTextEntry( "T_ESCAPE_MENU_EXIT_WINDOWS_QUESTION" ) ).c_str() );
}

void CInterfaceMission::MsgShowObjectives( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_MISSION_OBJECTIVES, "" );
}

void CInterfaceMission::MsgSelectMode( const SGameMessage &msg )
{
	SetActionMode( EAM_SELECT );
}

void CInterfaceMission::MsgMultiSelectMode( const SGameMessage &msg )
{
	bMultiSelectSubMode = true;
	bPreSelectSubMode = false;
	UpdateSelectMode();
}

void CInterfaceMission::MsgSingleSelectMode( const SGameMessage &msg )
{
	bMultiSelectSubMode = false;
	bPreSelectSubMode = false;
	UpdateSelectMode();
}

void CInterfaceMission::MsgPreSelectMode( const SGameMessage &msg )
{
	bMultiSelectSubMode = false;
	bPreSelectSubMode = true;
	UpdateSelectMode();
}

void CInterfaceMission::UpdateSelectMode()
{
	if ( pWorld->GetActionMode() != EAM_SELECT )
		return;

	if ( !bMultiSelectSubMode )
	{
		if ( pMultiFunctionTab && TAB_SINGLE_SELECT < pMultiFunctionTab->GetNTabs() )
		{
			pMultiFunctionTab->SetActive( TAB_SINGLE_SELECT );
			if ( pAppearanceTab )
				pAppearanceTab->SetActive( TAB_APPEARANCE_SINGLE_SELECT );
			return;
		}
	}

	if ( pMultiFunctionTab && TAB_MULTI_SELECT < pMultiFunctionTab->GetNTabs() )
	{
		pMultiFunctionTab->SetActive( TAB_MULTI_SELECT );
		if ( pAppearanceTab )
			pAppearanceTab->SetActive( TAB_APPEARANCE_SINGLE_SELECT );
	}

	UpdateActionPanel();
}

void CInterfaceMission::MsgUpdateSingleUnit( const SGameMessage &msg )
{
	if ( pMultiFunctionTab && TAB_SINGLE_SELECT < pMultiFunctionTab->GetNTabs() )
	{
		CMapObj *pMO = pWorld->GetMapObj( msg.nParam1 );
		pUnitFullInfo->SetObject( pMO );
	}
}

void CInterfaceMission::MsgUpdateUnitStats( const SGameMessage &msg )
{
	if ( bMultiSelectSubMode )
	{
		CMapObj *pMO = pWorld->GetMapObj( msg.nParam1 );
		pUnitFullInfo->SetObject( pMO );
	}
	else
	{
		CMapObj *pMO = pWorld->GetMapObj( msg.nParam1 );
		pUnitFullInfo->UpdateObject( pMO );
	}
}

void CInterfaceMission::MsgUpdateSuperWeaponStats( const SGameMessage &msg )
{
	if ( pSuperWeapon )
	{
		CMapObj *pMO = pWorld->GetMapObj( msg.nParam1 );
		pSuperWeapon->UpdateObject( pMO );
	}
}

void CInterfaceMission::MsgUpdateButtons( const SGameMessage &msg )
{
	UpdateActionPanel();
}

void CInterfaceMission::MsgUnpdateIcon( const SGameMessage &msg )
{
	if ( bMultiSelectSubMode )
	{
		if ( msg.nParam1 < 0 && msg.nParam2 < 0 )
		{
			UpdateMultiUnitsInfo( 0, 0 );
		}
		else
		{
			CMapObj *pMO = pWorld->GetMapObj( msg.nParam1 );
			if ( pMO )
				UpdateMultiUnitsInfo( pMO, msg.nParam2 );
		}
	}
	else
	{
		CMapObj *pMO = pWorld->GetMapObj( msg.nParam1 );
		pUnitFullInfo->UpdateObject( pMO );
	}
}

void CInterfaceMission::MsgHighlightUnits( const SGameMessage &msg )
{
	for ( int i = 0; i < iconSlots.size(); ++i )
	{
		IButton *pBtn = iconSlots[i].pBtn;
		if ( !pBtn->IsVisible() )
			return;
		pBtn->SetState( ( msg.nParam1 <= i && i <= msg.nParam2 ) ? 1 : 0 );
	}
}

void CInterfaceMission::MsgSetAbilityState( const SGameMessage &msg )
{
	const NDb::EUserAction eUserAction = GetActionByAbility( static_cast<NDb::EUnitSpecialAbility>(msg.nParam1) );
	SAbilitySwitchState state;
	state.dwStateValue = msg.nParam2;
	SetAbilityState( eUserAction, state );
}

struct SReinfInfo
{
	const NDb::SHPObjectRPGStats *pStats;
	int nCount;

	bool operator==( const SReinfInfo &reinf ) const
	{
		bool bResult = (pStats == reinf.pStats);
		return bResult;
	}
};

void CInterfaceMission::SetArmyPoints( int nPoints )
{
}

void CInterfaceMission::GetObjectHPs( const CMapObj *pMO, std::vector<float> *pHPs, bool *pIsSquad )
{
	if ( !pMO )
		return;

	pHPs->clear();
	bool bSquad = false;
	if ( pMO->GetTypeID() == NDb::SSquadRPGStats::typeID )
	{
		if ( const IMOSquad *pSquad = checked_cast<const IMOSquad*>( pMO ) )
		{
			if ( const NDb::SSquadRPGStats *pSquadStats = dynamic_cast<const NDb::SSquadRPGStats*>( pSquad->GetStats() ) )
			{
				bSquad = true;

				if ( !pSquadStats->members.empty() )
				{
					pHPs->push_back( (float)( pSquad->GetPassangersCount() ) / (float)( pSquadStats->members.size() ) );
				}
			}
		}
	}
	else
	{
		pHPs->push_back( pMO->GetHP() );
	}

//	*pIsSquad = bSquad;
	*pIsSquad = false;
}

void CInterfaceMission::UpdateMultiUnitsInfo( CMapObj *pMO, int nCount )
{
	if ( !pMO )
	{
		for ( int i = 0; i < nCurrentIconSlot; ++i )
		{
			IButton *pBtn = iconSlots[i].pBtn;
			pBtn->ShowWindow( false );
			pBtn->SetState( 0 );
		}
		nCurrentIconSlot = 0;
		pUnitFullInfo->SetObject( 0 );
		return;
	}

	if ( nCurrentIconSlot < iconSlots.size() )
	{
		const NDb::SHPObjectRPGStats *pStats = pMO->GetStats();
		SIconSlot &slot = iconSlots[nCurrentIconSlot];
		if ( slot.pBtn )
		{
			slot.pBtn->ShowWindow( true );
			slot.pBtn->SetTooltip( nCount == 1 ? wszTooltipSlotUnit : wszTooltipSlot );
		}
		if ( slot.pIconWnd )
		{
			slot.pIconWnd->SetTexture( pStats->pIconTexture );
		}
		if ( slot.pFlagBgWnd )
		{
			slot.pFlagBgWnd->ShowWindow( pStats->pIconFlagBackground != 0 );
			slot.pFlagBgWnd->SetTexture( pStats->pIconFlagBackground );
		}
		if ( slot.pCountView )
		{
			slot.pCountView->ShowWindow( nCount != 1 );
			if ( nCount != 1 )
				slot.pCountView->SetText( slot.pCountView->GetDBText() + NStr::ToUnicode( StrFmt( "%d", nCount ) ) );
		}

		bool bSquad;
		std::vector<float> progresses;
		GetObjectHPs( pMO, &progresses, &bSquad );

		if ( !progresses.empty() )
		{
			if ( slot.pProgressBar )
				slot.pProgressBar->SetPosition( progresses.front() );
		}

/*		IWindow *pWndUnit = vIconSlots[nCurrentIconSlot]->GetChild( "multi_bar_unit", true );
		IMultiTextureProgressBar *pWndUnit2 = dynamic_cast<IMultiTextureProgressBar*>( pWndUnit );

		IWindow *pWndSquad = vIconSlots[nCurrentIconSlot]->GetChild( "multi_bar_squad", true );
		IMultiTextureProgressBar *pWndSquad2 = dynamic_cast<IMultiTextureProgressBar*>( pWndSquad );*/

//		bool bSolid = bSquad; // unit/squad
//		bool bSolid = (progresses.size() == 1); // single/multi
//		bool bSolid = true;

//		if ( pWndUnit )
//			pWndUnit->ShowWindow( bSolid );
//		if ( pWndSquad )
//			pWndSquad->ShowWindow( !bSolid );
/*		if ( bSolid )
		{
			if ( pWndUnit2 )
				pWndUnit2->SetPositions( progresses, bSolid );
		}
		else
		{
			if ( pWndSquad2 )
				pWndSquad2->SetPositions( progresses, bSolid );
		}*/

		nCurrentIconSlot++;
	}
}

bool CInterfaceMission::OnClickFullInfoMember( const std::string &szSender )
{
	if ( pReinf->IsOpen() )
		pReinf->OnClickFullInfoMember( szSender );
	else
		pUnitFullInfo->OnClickMember( szSender );
	return true;
}

bool CInterfaceMission::OnFullInfoMemberOverOn( const std::string &szSender )
{
	if ( pReinf->IsOpen() )
		pReinf->OnFullInfoMemberOverOn( szSender );
	else
		pUnitFullInfo->OnMemberOverOn( szSender );
	return true;
}

bool CInterfaceMission::OnFullInfoMemberOverOff( const std::string &szSender )
{
	if ( pReinf->IsOpen() )
		pReinf->OnFullInfoMemberOverOff( szSender );
	else
		pUnitFullInfo->OnMemberOverOff( szSender );
	return true;
}

bool CInterfaceMission::OnFullInfoWeaponOverOn( const std::string &szSender )
{
	if ( pReinf->IsOpen() )
		pReinf->OnFullInfoWeaponOverOn( szSender );
	else
		pUnitFullInfo->OnWeaponOverOn( szSender );
	return true;
}

bool CInterfaceMission::OnFullInfoWeaponOverOff( const std::string &szSender )
{
	if ( pReinf->IsOpen() )
		pReinf->OnFullInfoWeaponOverOff( szSender );
	else
		pUnitFullInfo->OnWeaponOverOff( szSender );
	return true;
}

/*void CInterfaceMission::MakeActionTooltip( IWindow *pWnd, const string &szCommand, bool bHotkey )
{
	list<NInput::SBind> binds;
	NInput::GetBind( szCommand, &binds );
	if ( CDynamicCast<const NDb::SWindow> pDesc = pWnd->GetDesc() )
	{
		if ( pDesc->pTooltip )
		{
			wstring wszTooltip = pDesc->pTooltip->wszText;

			if ( bHotkey && !szButtonsBindSection.empty() )
			{
				for ( list<NInput::SBind>::iterator it = binds.begin(); it != binds.end(); ++it )
				{
					NInput::SBind &bind = *it;
					if ( bind.szSection == szButtonsBindSection && !bind.controlsSet.empty() )
					{
						wszTooltip += L"<br><val hotkey>" + NStr::ToUnicode( "'" + bind.controlsSet.front() + "'" );
						break;
					}
				}
			}
			pWnd->SetTooltip( wszTooltip );
		}
	}
}

void CInterfaceMission::MakeAbilityTooltip( IWindow *pWnd, int nSlot )
{
	MakeActionTooltip( pWnd, StrFmt( "user_ability_slot_%02d", nSlot ), !GetActionOwnReaction( pWnd ).empty() );
}

void CInterfaceMission::MakeCommandTooltip( IWindow *pWnd )
{
	MakeActionTooltip( pWnd, GetActionOwnReaction( pWnd ), true );
}

string CInterfaceMission::GetActionOwnReaction( IWindow *pWnd ) const
{
	if ( CDynamicCast<IButton> pBtn = pWnd )
	{
		if ( CDynamicCast<const NDb::SWindowMSButton> pDesc = pBtn->GetDesc() )
		{
			int nState = pBtn->GetState();
			if ( nState < pDesc->gameMessageReactions.size() )
			{
				const NDb::SGameMessageReaction &reaction = pDesc->gameMessageReactions[nState];
				return reaction.szGameMessage;
			}
		}
	}
	return string();
}*/

void CInterfaceMission::MakeActionTooltip( NDb::EUserAction eUserAction, const std::string &szCommand, bool bHotkey, const std::wstring *pTooltipOverride )
{
	CNewActionButtons::iterator it = newActionButtons.find( eUserAction );
	if ( it != newActionButtons.end() )
	{
		SNewActionButton &action = it->second;
		const std::wstring &wszBaseTooltip = pTooltipOverride ? *pTooltipOverride : action.wszTooltip;
		std::wstring wszTooltip = wszBaseTooltip;

		if ( !wszBaseTooltip.empty() )
		{
			std::list<NInput::SBind> binds;
			NInput::GetBind( szCommand, &binds );

			if ( bHotkey && !szButtonsBindSection.empty() )
			{
				for ( std::list<NInput::SBind>::iterator it = binds.begin(); it != binds.end(); ++it )
				{
					NInput::SBind &bind = *it;
					if ( bind.szSection == szButtonsBindSection && !bind.controlsSet.empty() )
					{
						std::string szControlLocalName = NInput::GetControlLocalName( NInput::GetControlID(bind.controlsSet.front()) );
						if ( szControlLocalName.empty() )
							szControlLocalName = bind.controlsSet.front();
						wszTooltip += L"<br><val hotkey>" + NStr::ToUnicode( "'" + szControlLocalName + "'" );
						break;
					}
				}
			}
		}

#ifndef _FINALRELEASE
		if ( wszTooltip.empty() )
			wszTooltip = NStr::ToUnicode( StrFmt( "UserAction id: %d", (int)( eUserAction ) ) );
#endif //_FINALRELEASE

		// Set even an empty tooltip so a previous ability-specific description is cleared.
		if ( action.pBtn )
			action.pBtn->SetTooltip( wszTooltip );
		if ( action.pIconBgDisabledWnd )
			action.pIconBgDisabledWnd->SetTooltip( wszTooltip );
	}
}

void CInterfaceMission::MakeAbilityTooltip( NDb::EUserAction eUserAction, int nSlot, const NDb::SUnitSpecialAblityDesc *pAbilityDesc, bool bFixedPlace )
{
	CNewActionButtons::iterator it = newActionButtons.find( eUserAction );
	if ( it != newActionButtons.end() )
	{
		SNewActionButton &action = it->second;
		std::wstring wszAbilityTooltip;
		const std::wstring *pTooltipOverride = 0;
		if ( pAbilityDesc && CHECK_TEXT_NOT_EMPTY_PRE(pAbilityDesc->,LocalizedDesc) )
		{
			wszAbilityTooltip = GET_TEXT_PRE(pAbilityDesc->,LocalizedDesc);
			if ( !wszAbilityTooltip.empty() )
				pTooltipOverride = &wszAbilityTooltip;
		}

		const std::string szCommand = bFixedPlace ? action.szHotkeyCmd : StrFmt( "user_ability_slot_%02d", nSlot );
		MakeActionTooltip( eUserAction, szCommand, bFixedPlace || !action.bPassive, pTooltipOverride );
	}
}

void CInterfaceMission::MakeCommandTooltip( NDb::EUserAction eUserAction )
{
	CNewActionButtons::iterator it = newActionButtons.find( eUserAction );
	if ( it != newActionButtons.end() )
	{
		SNewActionButton &action = it->second;

		MakeActionTooltip( eUserAction, action.szHotkeyCmd, true );
	}
}

bool CInterfaceMission::IsAbility( NDb::EUserAction eAction ) const
{
	// Reverse was added after the ability range, but it is a fixed movement command like Move.
	if ( eAction == NDb::USER_ACTION_REVERSE )
		return false;
//	if ( eAction == NDb::USER_ACTION_RADIO_CONTROLLED_MODE )
//		return false;
	return eAction > NDb::USER_ACTION_ABILITY ||
		eAction == NDb::USER_ACTION_AMBUSH ||
		eAction == NDb::USER_ACTION_ENGINEER_BUILD_ENTRENCHMENT ||
		eAction == NDb::USER_ACTION_ENGINEER_CLEAR_MINES;// CRAP
//		eAction == NDb::USER_ACTION_CAPTURE_ARTILLERY; // CRAP
}

NDb::EUserAction CInterfaceMission::GetActionByButtonName( const std::string &szName ) const
{
	int nLen = strlen( "NewActionButton" );
	if ( szName.substr( 0, nLen ) != "NewActionButton" )
		nLen = strlen( "NewActionDisabledButton" );

	NDb::EUserAction eAction = (NDb::EUserAction)( NStr::ToInt( szName.substr( nLen ) ) );
	return eAction;
}

void CInterfaceMission::UpdateActionPanel()
{
	if ( pWorld->GetActionMode() != EAM_SELECT )
		return;

	int nActionTab = ACTION_TAB_SELECT;
	switch ( eActivePanel )
	{
		case NDb::ACTION_BTN_PANEL_DEFAULT:
			nActionTab = ACTION_TAB_SELECT;
		break;

		case NDb::ACTION_BTN_PANEL_ESC:
			nActionTab = ACTION_TAB_CANCEL;
		break;

		case NDb::ACTION_BTN_PANEL_FORMATIONS:
			nActionTab = ACTION_TAB_FORMATIONS;
		break;

		case NDb::ACTION_BTN_PANEL_RADIO_CONTROLLED:
			nActionTab = ACTION_TAB_RADIO_CONTROLLED;
		break;
	}

	if ( pActionTab )
		pActionTab->SetActive( nActionTab );

	CUserActions actions;
	pWorld->GetSelectionEnabledActions( &actions );
	if ( eActiveAction != NDb::USER_ACTION_UNKNOWN && !actions.HasAction( eActiveAction ) )
	{
		eActivePanel = NDb::ACTION_BTN_PANEL_DEFAULT;
		if ( pActionTab )
			pActionTab->SetActive( ACTION_TAB_SELECT );
	}

	UpdateActionButtons();
}

struct SAbility
{
	int nTier;
	NDb::EUserAction eAction;
	IWindow *pWnd;
	const NDb::SUnitSpecialAblityDesc *pDesc;
	bool bFixedPlace;
	bool bEnabled;
};

struct SAbilitySort
{
	bool operator()( const SAbility &ability1, const SAbility &ability2 ) const
	{
		return ability1.nTier < ability2.nTier;
	}
};

void CInterfaceMission::UpdateActionButtons()
{
	std::vector<SAbility> abilities;

	ResetAbilityButtons();
	CUserActions actions, enabledActions;
	pWorld->GetSelectionActions( &actions );
	pWorld->GetSelectionEnabledActions( &enabledActions );
	for ( CActionButtons::iterator it = actionButtons.begin(); it != actionButtons.end(); ++it )
	{
		NDb::EUserAction eAction = it->first;
		IWindow *pWnd = it->second;
		bool bHasAction = actions.HasAction( eAction );
		bool bIsEnabled = enabledActions.HasAction( eAction );
		if ( !IsAbility( eAction ) )
		{
			pWnd->ShowWindow( bHasAction );
//			pWnd->Enable( !bIsDisabled );

			CNewActionButtons::iterator it2 = newActionButtons.find( eAction );
			if ( it2 != newActionButtons.end() )
			{
				SNewActionButton &action = it2->second;

				action.Enable( bIsEnabled );
			}

			MakeCommandTooltip( eAction );
		}
		else
		{
			if ( bHasAction )
			{
				SAbility ability;
				ability.nTier = pWorld->GetAbilityTier( eAction );
				ability.eAction = eAction;
				ability.pWnd = pWnd;
				ability.pDesc = pWorld->GetAbilityDesc( eAction );
				ability.bFixedPlace = false;
				ability.bEnabled = false;

				CNewActionButtons::iterator it2 = newActionButtons.find( eAction );
				if ( it2 != newActionButtons.end() )
				{
					SNewActionButton &action = it2->second;

					if ( !action.bAbility )
						ability.bFixedPlace = true;
					ability.bEnabled = action.bEnabled;
				}

				NI_ASSERT( ability.bFixedPlace || ability.nTier >= 0, StrFmt( "Wrong ability (%d) tier", eAction ) );

				abilities.push_back( ability );
			}
			else
				pWnd->ShowWindow( false );
		}
	}

	sort( abilities.begin(), abilities.end(), SAbilitySort() );
	for ( std::vector<SAbility>::iterator it = abilities.begin(); it != abilities.end(); ++it )
	{
		SAbility &ability = *it;
		AddAbilityButton( ability.eAction, ability.pWnd, ability.bFixedPlace, ability.pDesc );
	}

	// update unit full info buttons
	bool bCanLeave = enabledActions.HasAction( NDb::USER_ACTION_LEAVE );
	pUnitFullInfo->UpdateMembers( bCanLeave );
}

void CInterfaceMission::RegisterActionObservers()
{
	for ( CNewActionButtons::iterator it = newActionButtons.begin(); it != newActionButtons.end(); ++it )
	{
		SNewActionButton &action = it->second;
		NDb::EUserAction eAction = it->first;

		if ( !action.szHotkeyCmd.empty() )
			AddObserver( action.szHotkeyCmd, &CInterfaceMission::MsgActionCmd, (int)(eAction) );
	}
}

void CInterfaceMission::TrySkipMovie( const SGameMessage &msg )
{
	if ( NGlobal::GetVar( "temp.script_movie", false ) != 0 )
	{
		if ( eSkipState == ESMS_NONE )
		{
			eSkipState = ESMS_FADING_OUT;
			timeSkipProgress = GameTimer()->GetGameTime();
			//fInitialBrightness = NGlobal::GetVar( "scene_brightness", 0.5f );
		}
		//return true;
	}
	//return false;
}

bool CInterfaceMission::MsgActionCmd( const SGameMessage &msg, int nAction )
{
	CNewActionButtons::iterator it = newActionButtons.find( (NDb::EUserAction)( nAction ) );
	if ( it != newActionButtons.end() )
	{
		SNewActionButton &action = it->second;
		NDb::EUserAction eAction = it->first;

		if ( action.pBtn && action.pBtn->IsVisible() && action.bEnabled )
		{
			NewActionButtonClick( eAction );

			if ( action.bPressEffect )
				pScreen->RunStateCommandSequience( "PressActionButtonTemplate", action.pBtn, 0, true );

			return true;
		}
	}

	return false;
}

void CInterfaceMission::UpdateMissionTime( bool bAppActive )
{
	NTimer::STime timeCurrent = Singleton<IGameTimer>()->GetAbsTime();
	if ( timeCurrent > timeMissionLastCheck && timeMissionLastCheck > 0 )
		nMissionTimeMSec += timeCurrent - timeMissionLastCheck;

	if ( bAppActive )
		timeMissionLastCheck = timeCurrent;
	else
		timeMissionLastCheck = 0;

	if ( pScenarioTracker )
	{
		const int nLocalPlayer = pScenarioTracker->GetLocalPlayer();
		pScenarioTracker->SetStatistics( nLocalPlayer, IScenarioTracker::ESK_TIME, nMissionTimeMSec / 1000 );
	}
}

void CInterfaceMission::MsgNewUpdateReinfAvail( const SGameMessage &msg )
{
	pReinf->UpdateNewAvail();
}

void CInterfaceMission::MsgNewUpdateReinfPoint( const SGameMessage &msg )
{
	pReinf->UpdateNewPoint( msg.nParam1 != 0 );
}

void CInterfaceMission::MsgForcedActionCallReinf( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	pReinf->Call( vPos );

	SetActionMode( EAM_SELECT );
}

void CInterfaceMission::MsgForcedActionCallNoReinf( const SGameMessage &msg )
{
	pReinf->CallNoReinf();
}

void CInterfaceMission::MsgForcedActionCallSuperWeapon( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	pSuperWeapon->Call( vPos );

	SetActionMode( EAM_SELECT );
}

void CInterfaceMission::MsgResetTarget( const SGameMessage &msg )
{
	if ( pWorld->GetActionMode() == EAM_SELECT )
		NInput::PostEvent( "reset_target2", 0, 0 );
	else if ( pWorld->GetActionMode() == EAM_REINF )
	{
		if ( pReinf->ResetReinfMode() )
			SetActionMode( EAM_SELECT );
	}
}

void CInterfaceMission::MsgSetAbilityParam( const SGameMessage &msg )
{
	const NDb::EUserAction eUserAction = GetActionByAbility( static_cast<NDb::EUnitSpecialAbility>(msg.nParam1) );
	float fProgress = msg.nParam2 / 1024.0f;

	CNewActionButtons::iterator it = newActionButtons.find( eUserAction );
	if ( it != newActionButtons.end() )
	{
		SNewActionButton &action = it->second;

		action.SetProgress( fProgress );
	}

/*	const NDb::EUserAction eUserAction = GetActionByAbility( static_cast<NDb::EUnitSpecialAbility>(msg.nParam1) );
	for ( CActionButtons::iterator it = lActiveAbilities.begin(); it != lActiveAbilities.end(); ++it )
		if ( it->first == eUserAction )
		{
			IProgressBar *pProgress = dynamic_cast<IProgressBar *>( it->second.GetPtr()->GetChild( "ability_progress", true ) );
			if ( pProgress )
				pProgress->SetPosition( msg.nParam2 / 1024.0f );
			return;
		}*/
}

void CInterfaceMission::MsgMiniMapShowObjectives( const SGameMessage &msg )
{
	const int nSelectedID = msg.nParam1;

	pNotifications->Notify( EVNT_SELECT_OBJECTIVE, nSelectedID, VNULL2 );
	pNotifications->Notify( EVNT_SHOW_OBJECTIVES, -1, VNULL2 );
	pNotifications->Notify( EVNT_CHECK_OBJECTIVES, -1, VNULL2 );
}

void CInterfaceMission::MsgMiniMapHideObjectives( const SGameMessage &msg )
{
	pNotifications->Notify( EVNT_HIDE_OBJECTIVES, -1, VNULL2 );
}

void CInterfaceMission::MsgQuickSave( const SGameMessage &msg )
{
	if ( pMovieBorder && pMovieBorder->IsVisible() )
		return;
	if ( !NGlobal::GetVar( "Multiplayer.Host" ) && NGlobal::GetVar( "Multiplayer.Client", "" ).GetString() == L"" )
		DoQuickSave();
}

void CInterfaceMission::MsgQuickLoad( const SGameMessage &msg )
{
	if ( pMovieBorder && pMovieBorder->IsVisible() )
		return;
	DoQuickLoad();
}

void CInterfaceMission::MsgShowGamePaused( const SGameMessage &msg )
{
	bool bUserPaused = Singleton<IGameTimer>()->HasPause( PAUSE_TYPE_USER_PAUSE );
	bool bInterfacePaused = Singleton<IGameTimer>()->HasPause( PAUSE_TYPE_INTERFACE );
	if ( pPause )
		pPause->ShowWindow( bUserPaused && !bInterfacePaused );

	Singleton<ISFX>()->Pause( bUserPaused || bInterfacePaused ); // на паузе выключаются только эффекты, музыка остается
}

bool CInterfaceMission::MsgBeginScriptMovieSequence( const SGameMessage &msg )
{
	bMovieMode = true;

	NI_ASSERT( msg.nParam1 != 0, "Unsupported mode" );

	NGlobal::SetVar( "MissionIconsMovieMode", 1.0f );
	if ( pWorld )
		pWorld->UpdateIcons();

	nGameSpeed = Singleton<IGameTimer>()->GetSpeed();
	Singleton<IGameTimer>()->SetSpeed( 0 );

	pScreen->ShowWindow( false );
	Cursor()->Show( false );

	if ( msg.nParam1 == 1 )
	{
		return false;
	}
	if ( pMovieBorder )
		pMovieBorder->ShowWindow( true );
	//NMainLoop::Command( ML_COMMAND_MISSION_BORDER, "" );

	return false;
}

void CInterfaceMission::EndScriptMovieSequence()
{
	bMovieMode = false;

	NGlobal::SetVar( "MissionIconsMovieMode", 0.0f );
	if ( pWorld )
		pWorld->UpdateIcons();

	Singleton<IGameTimer>()->SetSpeed( nGameSpeed );
	pScreen->ShowWindow( true );
	Cursor()->Show( true );

//	CheckedShowHelpScreen( false );

	if ( pMovieBorder )
		pMovieBorder->ShowWindow( false );
	/*
	if ( eSkipState != ESMS_NONE )
		eSkipState = ESMS_DONE;
		*/
}

bool CInterfaceMission::OnReinfSelect( const std::string &szSender )
{
	pReinf->Select( szSender );

	return true;
}

bool CInterfaceMission::OnReinfSelectDblClick( const std::string &szSender )
{
 // CRAP - temporary disable, uncomment it at designer's request

/*	pReinf->SelectDblClick( szSender );
	SetActionMode( EAM_SELECT );*/

	return true;
}

bool CInterfaceMission::OnToggleReinf( const std::string &szSender )
{
	if ( !pReinf->IsOpen() )
		pReinf->Show();
	else
		pReinf->Close( true );

	return true;
}

bool CInterfaceMission::OnReinfUnitInfo( const std::string &szSender )
{
	pReinf->ShowUnitInfo( szSender );
	return true;
}

bool CInterfaceMission::OnReinfFullInfoBack( const std::string &szSender )
{
	pReinf->UnitFullInfoBack();
	return true;
}

bool CInterfaceMission::OnReinfMouseOverForward( const std::string &szSender )
{
	pReinf->MouseOverForward( szSender );
	return true;
}

bool CInterfaceMission::OnReinfMouseOverBackward( const std::string &szSender )
{
	pReinf->MouseOverBackward( szSender );
	return true;
}

bool CInterfaceMission::OnReinfCallMode( const std::string &szSender )
{
	pReinf->OnReinfCallMode();

	return true;
}

bool CInterfaceMission::OnReinfAutoShowReinf( const std::string &szSender, bool bOn )
{
	pReinf->OnReinfAutoShowReinf( bOn );
	return true;
}

bool CInterfaceMission::OnClickMultiSelectUnit( const std::string &szSender, WORD wKeyboardFlags )
{
	const int nSize = strlen( "unit_slot_" );
	int nSlot = NStr::ToInt( szSender.substr( nSize ) ) - 1;

	pWorld->OnClickMultiSelectUnit( nSlot, wKeyboardFlags );

	return true;
}

bool CInterfaceMission::OnSelectSpecialGroup( const std::string &szSender, WORD wKeyboardFlags )
{
	if ( szSender == "Button01" )
		pWorld->OnSelectSpecialGroup( 0 );
	else if ( szSender == "Button02" )
		pWorld->OnSelectSpecialGroup( 1 );
	else if ( szSender == "Button03" )
		pWorld->OnSelectSpecialGroup( 2 );
	// Keep superweapons on Button04 for compatibility with vanilla UI data.
	else if ( szSender == "Button04" )
	{
		if ( pSuperWeapon->CanActivate() )
		{
			SetActionMode( EAM_SUPER_WEAPON );
			pSuperWeapon->Activate();
		}
	}
	else if ( szSender == "Button05" )
		pWorld->OnSelectSpecialGroup( 4 );
	else if ( szSender == "Button06" )
		pWorld->OnSelectSpecialGroup( 5 );

	return true;
}

bool CInterfaceMission::OnUnselectSpecialGroup( const std::string &szSender, WORD wKeyboardFlags )
{
	if ( szSender == "Button04" )
	{
		pSuperWeapon->Deactivate();
		SetActionMode( EAM_SELECT );
	}
	return true;
}

bool CInterfaceMission::OnNewActionButtonClick( const std::string &szSender, WORD wKeyboardFlags )
{
	NDb::EUserAction eAction = GetActionByButtonName( szSender );

	NewActionButtonClick( eAction );

	return true;
}

void CInterfaceMission::NewActionButtonClick( NDb::EUserAction eAction )
{
	CNewActionButtons::iterator it = newActionButtons.find( eAction );
	if ( it != newActionButtons.end() )
	{
		SNewActionButton &action = it->second;
		eActivePanel = action.eTargetPanel;
		eActiveAction = eAction;
		UpdateActionPanel();
		if ( !action.bAbility )
		{
			if ( action.eTargetPanel != NDb::ACTION_BTN_PANEL_FORMATIONS &&
				action.eTargetPanel != NDb::ACTION_BTN_PANEL_RADIO_CONTROLLED )
				NInput::PostEvent( "set_forced_action", eAction, 0 );
		}
		else
		{
			if ( action.eTargetPanel == NDb::ACTION_BTN_PANEL_ESC )
			{
				NInput::PostEvent( "set_forced_action", eAction, 0 );
			}
			else
			{
				if ( action.curState.eState == EASS_READY_TO_ON )
					NInput::PostEvent( "set_special_ability", eAction, NDb::PARAM_ABILITY_ON );
				else if ( action.curState.eState == EASS_ACTIVE )
					NInput::PostEvent( "set_special_ability", eAction, NDb::PARAM_ABILITY_OFF );
			}
		}
	}
}

bool CInterfaceMission::OnNewActionButtonRightClick( const std::string &szSender, WORD wKeyboardFlags )
{
	NDb::EUserAction eAction = GetActionByButtonName( szSender );
	CNewActionButtons::iterator it = newActionButtons.find( eAction );
	if ( it != newActionButtons.end() )
	{
		SNewActionButton &action = it->second;

		if ( action.bAutocast )
		{
			action.curState.bAutocast = !action.curState.bAutocast;
			NInput::PostEvent( "set_special_ability", eAction,
				action.curState.bAutocast ? NDb::PARAM_ABILITY_AUTOCAST_ON : NDb::PARAM_ABILITY_AUTOCAST_OFF );
		}
	}
	return true;
}

bool CInterfaceMission::OnNotificationEventBtn( const std::string &szSender, bool bRightBtn )
{
	pNotifications->OnBtn( szSender, bRightBtn );

	return true;
}

bool CInterfaceMission::OnEnterPressed( WORD wKeyboardFlags )
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( pST->GetGameType() != IScenarioTracker::EGT_MULTI_FLAG_CONTROL )
		return true;

	bool bTeam = chatInput.bTeamByDefault;
	if ( (wKeyboardFlags & EKF_CTRL) != 0 )
		bTeam = true;
	else if ( (wKeyboardFlags & EKF_SHIFT) != 0 )
		bTeam = false;
	StartChatInput( bTeam );

	return true;
}

bool CInterfaceMission::OnChatInputEnterPressed()
{
	if ( chatInput.pEdit )
		SendChat( chatInput.pEdit->GetText(), chatInput.bTeam );

	CloseChatInput();

	return true;
}

bool CInterfaceMission::OnChatInputEscPressed()
{
	CloseChatInput();

	return true;
}

bool CInterfaceMission::OnChatInputFocusLost()
{
	CloseChatInput();

	return true;
}

void CInterfaceMission::StartChatInput( bool _bTeam )
{
	chatInput.bTeam = _bTeam;

	if ( !pMinimizeBtn || !pMinimizeBtn->IsEnabled() )
		return;

	chatInput.bMultifunctionPanelMinimized = bMultifunctionPanelMinimized;
	if ( !chatInput.bMultifunctionPanelMinimized )
		FastMinimizePanels();

	if ( chatInput.pPanel && !chatInput.pPanel->IsVisible() )
	{
		if ( chatInput.pPanel )
			chatInput.pPanel->ShowWindow( true );
		int nViewWidth = 0;
		if ( chatInput.pView )
		{
			std::wstring wszText = chatInput.bTeam ? chatInput.wszTeam : chatInput.wszAll;
			chatInput.pView->SetText( chatInput.pView->GetDBText() + wszText );
			const CTPoint<int> &size = chatInput.pView->GetSize();
			nViewWidth = size.x;
		}
		if ( chatInput.pEdit )
		{
			chatInput.pEdit->SetText( L"" );
			chatInput.pEdit->SetPlacement( chatInput.fEditBaseX + nViewWidth, 0, chatInput.fEditBaseWidth - nViewWidth, 0, EWPF_POS_X | EWPF_SIZE_X );
			chatInput.pEdit->SetFocus( true );
		}
	}
}

void CInterfaceMission::SendChat( const std::wstring &wszText, bool bTeam )
{
	if ( wszText.empty() )
		return;

	CPtr<SMPUIInGameChatMessage> pChatMsg = new SMPUIInGameChatMessage( wszText, bTeam );
	Singleton<IMPToUIManager>()->AddUIMessage( pChatMsg );

	// CRAP - test echo
//	InterfaceState()->AddMPChatMessage( wszText );
}

void CInterfaceMission::FastMinimizePanels()
{
	NInput::PostEvent( "mission_multistate_panel_minimize_move_fast", 0, 0 );
	NInput::PostEvent( "mission_appearance_panel_minimize_move_fast", 0, 0 );
	NInput::PostEvent( "mission_action_panel_minimize_move_fast", 0, 0 );
	NInput::PostEvent( "mission_reinf_panel_minimize_move_fast", 0, 0 );
	NInput::PostEvent( "mission_minimap_panel_minimize_move_fast", 0, 0 );
}

void CInterfaceMission::FastMaximizePanels()
{
	NInput::PostEvent( "mission_multistate_panel_maximize_move_fast", 0, 0 );
	NInput::PostEvent( "mission_appearance_panel_maximize_move_fast", 0, 0 );
	NInput::PostEvent( "mission_action_panel_maximize_move_fast", 0, 0 );
	NInput::PostEvent( "mission_reinf_panel_maximize_move_fast", 0, 0 );
	NInput::PostEvent( "mission_minimap_panel_maximize_move_fast", 0, 0 );
}

void CInterfaceMission::CloseChatInput()
{
	if ( chatInput.pPanel && chatInput.pPanel->IsVisible() )
	{
		chatInput.pPanel->ShowWindow( false );
		chatInput.pEdit->SetText( L"" );
		chatInput.pEdit->SetFocus( false );

		if ( !chatInput.bMultifunctionPanelMinimized )
			FastMaximizePanels();
	}
}

void CInterfaceMission::AfterLoad()
{
	if ( pMission == 0 )
	{
		NGlobal::ProcessCommand( L"main_menu" );
		return;
	}
/*	Scene()->SetSceneConsts( NGameX::GetSceneConsts() );
	Scene()->AfterLoad();*/

	NGlobal::SetVar( "MissionIconsMovieMode", 0.0f );

	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const IScenarioTracker::SPlayerColor &color = pST->GetPlayerColor( pST->GetLocalPlayer() );
	CPtr<NDb::SBackgroundSimpleTexture> pBackground = new NDb::SBackgroundSimpleTexture();
	pBackground->PostLoad( false );
	pBackground->nColor = color.dwColor;
	for ( int i = 0; i < iconSlots.size(); ++i )
	{
		SIconSlot &slot = iconSlots[i];
		if ( slot.pProgressBar )
			slot.pProgressBar->SetForward( pBackground );
	}

	CInterfaceScreenBase::AfterLoad();
	RegisterObservers();
	RegisterActionObservers();
	pWorld->AfterLoad( pMission );
	// update warfog
	const NTimer::STime nCurrentTime = GameTimer()->GetGameTime();
	UpdateWarFog( nCurrentTime, false, true );
	pReinf->AfterLoad();
}

void CInterfaceMission::Freeze( const bool bFreeze )
{
	bFrozen = bFreeze;
}

void CInterfaceMission::Draw( NGScene::CRTPtr *pTexture )
{
	if ( eUIState == EUIS_ON_ENTER_TRANSIT_START )
	{
		eUIState = EUIS_ON_ENTER_TRANSIT_START_DONE;

		CInterfaceScreenBase::Draw( NGScene::GetFrameTransitionCapture2() );
		return;
	}

	if ( eUIState == EUIS_ON_ENTER_TRANSIT_START_DONE )
	{
		if ( !NGScene::IsFrameTransitionComplete() )
		{
			NGScene::RenderFrameTransition();
			NGScene::Flip();
		}
		else
		{
			eUIState = EUIS_NORMAL;

			UpdateInterfacePause();
		}
		return;
	}

	if ( eUIState != EUIS_NORMAL )
	{
		CInterfaceScreenBase::Draw();
		return;
	}

	if ( nFrameTransition == 0 )
		CInterfaceScreenBase::Draw();
	else if ( nFrameTransition == 1 )
	{
		const CVec3 &vFrom = Camera()->GetAnchor();
		CVec2 vTo;
		AI2Vis( &vTo, vFrameTransitionTo );
		vTo.Set( vTo.x - vFrom.x, -vTo.y + vFrom.y );
		float fDistance = fabs( vTo );
		NGScene::SFrameTransitionInfo ftInfo;
		if ( fDistance > 0 )
			vTo /= fDistance;

		SHMatrix matr = Camera()->GetViewMatrix();
		matr._14 = matr._24 = matr._31 = matr._32 = matr._34 = 0.0f;
		matr._33 = 1.0f;
		CVec3 vTo3;
		matr.RotateVector( &vTo3, CVec3( vTo.x, vTo.y, 0.0f ) );
		ftInfo.vTransitionDir.Set( vTo3.x, vTo3.y );

		fDistance /= 50.0f;
		if ( fDistance > 3.0f )
			fDistance = 3.0f;
		ftInfo.fTransitionLength = fDistance;
		ftInfo.bRandomDir = false;
		ftInfo.nEffectDuration = s_nTransitionEffectDuration;
		ftInfo.fQuadsGroup1MinZ = 1.0f;
		ftInfo.fQuadsGroup1MaxZ = 1.1f;
		ftInfo.fQuadsGroup2MinZ = 1.0f;
		ftInfo.fQuadsGroup2MaxZ = 0.9f;
		CInterfaceScreenBase::Draw( NGScene::GetFrameTransitionCapture1( ftInfo ) );
		nFrameTransition = 2;
		NInput::PostEvent( "scroll_map_true", PackCoords( vFrameTransitionTo ), 0 );
	}
	else if ( nFrameTransition == 2 )
	{
		CInterfaceScreenBase::Draw( NGScene::GetFrameTransitionCapture2() );
		nFrameTransition = 3;
	}

	if ( nFrameTransition == 3 && NGScene::IsFrameTransitionComplete() )
	{
		nFrameTransition = 0;
		CInterfaceScreenBase::Draw();
	}
	else if ( nFrameTransition == 3 )
	{
		NGScene::RenderFrameTransition();
		NGScene::Flip();
	}
}

void CInterfaceMission::SetActionMode( EActionMode eActionMode )
{
	pWorld->SetActionMode( eActionMode );

	switch ( eActionMode )
	{
		case EAM_SELECT:
		{
			if ( pActionTab )
				pActionTab->SetActive( ACTION_TAB_SELECT );

			UpdateSelectMode();

			pWorld->GameResetForcedAction();
			break;
		}

		case EAM_REINF:
		{
			if ( pMultiFunctionTab )
				pMultiFunctionTab->SetActive( TAB_REINF_SELECT );
			if ( pActionTab )
				pActionTab->SetActive( ACTION_TAB_REINF );
			if ( pAppearanceTab )
				pAppearanceTab->SetActive( TAB_APPEARANCE_REINF );

			eActivePanel = NDb::ACTION_BTN_PANEL_DEFAULT;
			eActiveAction = NDb::USER_ACTION_UNKNOWN;

			pWorld->GameResetForcedAction();
			break;
		}

		case EAM_SUPER_WEAPON:
		{
			if ( pMultiFunctionTab )
				pMultiFunctionTab->SetActive( TAB_SUPER_WEAPON_SELECT );
			if ( pActionTab )
				pActionTab->SetActive( ACTION_TAB_SUPER_WEAPON );
			if ( pAppearanceTab )
				pAppearanceTab->SetActive( TAB_APPEARANCE_SUPER_WEAPON );

			eActivePanel = NDb::ACTION_BTN_PANEL_DEFAULT;
			eActiveAction = NDb::USER_ACTION_UNKNOWN;

			pWorld->GameResetForcedAction();
			break;
		}
	}
}

int CInterfaceMission::operator&( IBinSaver &saver )
{
	saver.Add( 1, (CInterfaceScreenBase*)this );

	saver.Add( 2, &pWorld );
	saver.Add( 3, &vAbilitySlots );
	saver.Add( 4, &nCurrentSlot );
	saver.Add( 5, &lActiveAbilities );
//	saver.Add( 6, &vIconSlots );
	saver.Add( 7, &actionButtons );
	saver.Add( 8, &nCurrentIconSlot );
	saver.Add( 9, &bScreenLoaded );
	saver.Add( 10, &pMiniMap );
	saver.Add( 15, &nCurrentReinfPoint );
	saver.Add( 16, &nLastStepTime );
	saver.Add( 17, &bShowWarFog );
	saver.Add( 18, &bNeedWarFogRecalc );

	saver.Add( 23, &pChatMessages );
	saver.Add( 24, &pChatMessagesElement );
	saver.Add( 25, &bAllowBorderScroll );

	//saver.Add( 27, &dbidMission );
	//saver.Add( 28, &camera );

	if ( saver.IsReading() )
	{
		fBorderScrollX = NGlobal::GetVar( "border_scroll_x", 0.05f );
		fBorderScrollY = NGlobal::GetVar( "border_scroll_y", 0.05f );
		nChatTime = 0;
	}

	saver.Add( 29, &bFrozen );

//	saver.Add( 30, &unitFullInfo );
//	saver.Add( 31, &pMultiFunctionTabWnd );
//	saver.Add( 32, &reinf );
	saver.Add( 33, &timeLastWarFogUpdate );
	saver.Add( 34, &bMultiSelectSubMode );
	saver.Add( 35, &pNotifications );
	saver.Add( 36, &chatMessages );
//	saver.Add( 37, &newObjectiveNotify );
	saver.Add( 38, &pShowObjectives );
	saver.Add( 39, &pPause );
	saver.Add( 41, &pTransceiver );
	saver.Add( 43, &nGameSpeed );
	saver.Add( 44, &pMultiFunctionTab );
	saver.Add( 45, &pReinf );
	saver.Add( 46, &pActionTab );
	saver.Add( 47, &szButtonsBindSection );
//	saver.Add( 48, &pReinfLight );
	saver.Add( 49, &pFlareBtn );
	saver.Add( 50, &pMinimizeBtn );
	saver.Add( 51, &pMultifunctionWnd );
	saver.Add( 52, &bMultifunctionPanelMinimized );
	saver.Add( 53, &pUnitFullInfo );
	saver.Add( 54, &specialSelectBtns );
	saver.Add( 55, &newActionButtons );
	saver.Add( 56, &eActivePanel );
	saver.Add( 57, &newActionButtonSlots );
	saver.Add( 58, &eActiveAction );
	saver.Add( 59, &fViewportBottom );
//	saver.Add( 60, &nMissionTime );
	saver.Add( 61, &nMissionTimeMSec );
	saver.Add( 62, &pScenarioTracker );

	saver.Add( 63, &nFrameTransition );
	saver.Add( 64, &vFrameTransitionTo );

//	saver.Add( 65, &ownAviaBackUnits );

	//{ CRAP - for compability with old saves
	if ( saver.IsReading() )
	{
		if ( !pScenarioTracker )
		{
			pScenarioTracker = Singleton<IScenarioTracker>();
		}
	}
	//}

	saver.Add( 66, &eSkipState );
	saver.Add( 67, &timeSkipProgress );
	saver.Add( 68, &fInitialBrightness );
	saver.Add( 69, &pMovieBorder );
	saver.Add( 70, &bScriptMoivie );
	saver.Add( 71, &fFormerVolume	);
	saver.Add( 72, &pMission );
	saver.Add( 73, &bCheckShowHelpScreen );
	saver.Add( 74, &bMovieMode );
	saver.Add( 75, &iconSlots );
	//saver.Add( 76, &pSuperWeaponBtn );
	//saver.Add( 77, &pSuperWeaponProgress );
	saver.Add( 78, &pSuperWeapon );
	saver.Add( 79, &pAppearanceTab );
	saver.Add( 80, &wszTooltipSlot );
	saver.Add( 81, &wszTooltipSlotUnit );
	saver.Add( 82, &chatInput );
	saver.Add( 83, &pObjectivesBlink );

	return 0;
}

// CDrawProgress

CDrawProgress::CDrawProgress( IProgressHookB2 *_pProgress ) :
	pProgress( _pProgress )
{
	pMaterials = new CLoadingCounter( this, EC_MATERIALS );
	pTerrains = new CLoadingCounter( this, EC_TERRAINS );
	pTextures = new CLoadingCounter( this, EC_TEXTURES );
}

void CDrawProgress::OnTotalCount( ECounter eCounter, int nTotalCount )
{
	switch ( eCounter )
	{
		case EC_TEXTURES:
		{
			if ( pProgress )
				pProgress->SetNumSteps( nTotalCount );
		}
		break;
	};
}

void CDrawProgress::OnCount( ECounter eCounter, int nCount )
{
	switch ( eCounter )
	{
		case EC_TEXTURES:
		{
			if ( pProgress )
				pProgress->SetCurrentStep( nCount );
		}
		break;
	};
}

// CDrawProgress::CLoadingCounter

CDrawProgress::CLoadingCounter::CLoadingCounter( CDrawProgress *_pOwner, ECounter _eCounter ) :
	pOwner( _pOwner ),
	eCounter( _eCounter )
{
}

void CDrawProgress::CLoadingCounter::LeftToLoad( int _nLeftCount )
{
	if ( pOwner )
	{
		nLeftCount = _nLeftCount;
		pOwner->OnCount( eCounter, nTotalCount - nLeftCount );
	}
}

void CDrawProgress::CLoadingCounter::SetTotalCount( int _nTotalCount )
{
	if ( pOwner )
	{
		nTotalCount = _nTotalCount;
		nLeftCount = nTotalCount;
		pOwner->OnTotalCount( eCounter, nTotalCount );
	}
}

void CDrawProgress::CLoadingCounter::Step()
{
	LeftToLoad( nLeftCount - 1 );
}

// CICLoadB2

CICLoadB2::CICLoadB2( const std::string &szFileName ) :
	CICLoadBase( szFileName )
{
}

void CICLoadB2::OnProgress( EStage eStage )
{
	switch ( eStage )
	{
		case STG_START:
		{
			InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_SINGLE );

			std::wstring wszText = InterfaceState()->GetTextEntry( "T_LOADING" );
			WriteToPipe( PIPE_CHAT, wszText, 0xffff0000 );

			if ( NGlobal::GetVar( "mission_loading_single", 0 ) != 0 )
			{
				pProgress = MakeObjectVirtual<IProgressHookB2>( ML_COMMAND_LOADING_SINGLE_2D );
				NSaveLoad::g_WaitLoadData.Reset();
			}

			if ( pProgress )
				pProgress->SetNumSteps( 2 + 3 );
		}
		break;
		case STG_SERIALIZE_DONE:
		{
			if ( pProgress )
				pProgress->Step();
		}
		break;
		case STG_AFTER_LOAD_DONE:
		{
			if ( pProgress )
				pProgress->Step();

			if ( pProgress )
				pProgress->LockRange( 3 );
			Scene()->SwitchScene( SCENE_MISSION );
			Scene()->GetGView()->WaitForLoad( true );
			Scene()->ToggleShow( SCENE_SHOW_NO_FLIP ); // no flip
			CObj<CDrawProgress> pDrawProgress = new CDrawProgress( pProgress );
			Scene()->GetGView()->SetLoadingCounter( 0, 0, pDrawProgress->GetTextures() );
			Scene()->Draw( 0 );
			pDrawProgress = 0;
			Scene()->GetGView()->SetLoadingCounter( 0, 0, 0 );
			Scene()->ToggleShow( SCENE_SHOW_NO_FLIP ); // flip
			if ( pProgress )
				pProgress->UnlockRange();

			pProgress = 0;

			Singleton<ISFX>()->Pause( false );
			Singleton<IMusicSystem>()->PauseMusic( EMS_MASTER, false );

			std::wstring wszText = InterfaceState()->GetTextEntry( "T_LOAD_DONE" );
			WriteToPipe( PIPE_CHAT, wszText, 0xffff0000 );

			// set pause after load
			if ( NGlobal::GetVar( "FPS_TEST_DURATION", 0 ) == 0 )
			{
				Singleton<IGameTimer>()->Pause( true, PAUSE_TYPE_USER_PAUSE );
				NInput::PostEvent( "show_game_paused", 0, 0 );
			}

			//{ CRAP - for compability with old saves (is really needed?)
//			if ( !Singleton<IScenarioTracker>() )
//				InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_SINGLE );
			//}
		}
		break;
	};
}

// CICSaveB2

CICSaveB2::CICSaveB2( const std::string &szFileName ) : CICSaveBase( szFileName )
{
}

void CICSaveB2::OnProgress( EStage eStage )
{
	switch ( eStage )
	{
		case STG_START:
		{
			std::wstring wszText = InterfaceState()->GetTextEntry( "T_SAVING" );
#ifndef _FINALRELEASE
//			wszText += L" (" + NStr::ToUnicode( GetPathName().c_str() ) + L")";
#endif //_FINALRELEASE
			WriteToPipe( PIPE_CHAT, wszText, 0xffff0000 );
		}
		break;

		case STG_AFTER_SAVE_DONE:
		{
			std::wstring wszText = InterfaceState()->GetTextEntry( "T_SAVE_DONE" );
			WriteToPipe( PIPE_CHAT, wszText, 0xffff0000 );
		}
		break;
	}
}

void MsgChangeGameSpeed( const SGameMessage &msg, int nAdd )
{
	if ( Singleton<IScenarioTracker>()->GetGameType() != IAIScenarioTracker::EGT_SINGLE && NGlobal::GetVar( "History.Playing", 0 ) == 0 )
		return;

	int nPrevSpeed = GameTimer()->GetSpeed();
	GameTimer()->SetSpeed( nPrevSpeed + nAdd );
	if ( GameTimer()->GetSpeed() == nPrevSpeed )
		return;

	// report about game speed changed to chat
	std::wstring wszText = InterfaceState()->GetTextEntry( "T_GAME_SPEED_CHANGED" ) +
			NStr::ToUnicode( StrFmt( "%d", GameTimer()->GetSpeed() ) );
	if ( GameTimer()->GetSpeed() == GameTimer()->GetMinSpeed() )
	{
		wszText += InterfaceState()->GetTextEntry( "T_GAME_SPEED_CHANGED_MIN" );
	}
	else if ( GameTimer()->GetSpeed() == GameTimer()->GetMaxSpeed() )
	{
		wszText += InterfaceState()->GetTextEntry( "T_GAME_SPEED_CHANGED_MAX" );
	}
	InterfaceState()->WriteToMissionConsole( wszText );
}

static void CmdQuickLoad( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	DoQuickLoad();
}

static void CmdLoadGame( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() != 1 )
		csSystem << "usage: " << szID << "game_name" << endl;
	else
	{
		std::string szParam;
		NStr::ToMBCS( &szParam, paramsSet[0] );

		NSaveLoad::SSaveInfo info;
		info.Read( NSaveLoad::GetSavePath() + szParam + NSaveLoad::INFO_FILE_EXTENSION );
		info.SetLoadInfo();

		NMainLoop::Command( ML_COMMAND_LOAD_GAME, szParam.c_str() );
	}
}

void DoQuickSave()
{
	if ( Singleton<IScenarioTracker>()->GetGameType() != IAIScenarioTracker::EGT_SINGLE )
		return;
	if ( !Singleton<IScenarioTracker>()->IsMissionActive() )
		return;

	std::wstring wszName01 = InterfaceState()->GetTextEntry( "T_QUICK_SAVE_NAME_01" );
	std::wstring wszName02 = InterfaceState()->GetTextEntry( "T_QUICK_SAVE_NAME_02" );

	if ( !wszName01.empty() )
	{
		std::string szName01 = std::string( NSaveLoad::SAVE_NAME_PREFIX ) + "quick01";
		std::string szName02 = std::string( NSaveLoad::SAVE_NAME_PREFIX ) + "quick02";

		NI_ASSERT( NFile::IsValidDirName( szName01 ), "Wrong file name" );
		NI_ASSERT( NFile::IsValidDirName( szName02 ), "Wrong file name" );

		std::string szFullSaveName01 = NSaveLoad::GetSavePath() + szName01 + NSaveLoad::SAVE_FILE_EXTENSION;
		std::string szFullSaveName02 = NSaveLoad::GetSavePath() + szName02 + NSaveLoad::SAVE_FILE_EXTENSION;
		std::string szFullInfoName01 = NSaveLoad::GetSavePath() + szName01 + NSaveLoad::INFO_FILE_EXTENSION;
		std::string szFullInfoName02 = NSaveLoad::GetSavePath() + szName02 + NSaveLoad::INFO_FILE_EXTENSION;

		if ( !wszName02.empty() )
		{
			if ( NFile::DoesFileExist( szFullSaveName01 ) )
			{
				remove( szFullSaveName02.c_str() );
				rename( szFullSaveName01.c_str(), szFullSaveName02.c_str() );
			}
			if ( NFile::DoesFileExist( szFullInfoName01 ) )
			{
				remove( szFullInfoName02.c_str() );
				rename( szFullInfoName01.c_str(), szFullInfoName02.c_str() );

				NSaveLoad::SSaveInfo info;
				info.Rename( szFullInfoName02, wszName02 );
			}
		}

		NSaveLoad::SSaveInfo info;
		info.Write( szFullInfoName01, wszName01, true, false, false );

		NMainLoop::Command( ML_COMMAND_SAVE_GAME, szName01.c_str() );
	}
}

void DoQuickLoad()
{
	std::string szName01 = std::string( NSaveLoad::SAVE_NAME_PREFIX ) + "quick01";

	NSaveLoad::SSaveInfo info;
	info.Read( NSaveLoad::GetSavePath() + szName01 + NSaveLoad::INFO_FILE_EXTENSION );
	info.SetLoadInfo();

	if ( !szName01.empty() )
		NMainLoop::Command( ML_COMMAND_LOAD_GAME, szName01.c_str() );
}

void CmdQuickSave( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	DoQuickSave();
}

void DoAutosaveInMission( const bool bOnWin )
{
	const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bKRIDemo )
		return;

	if ( Singleton<IScenarioTracker>()->GetGameType() != IAIScenarioTracker::EGT_SINGLE )
		return;

	if ( const NDb::SMapInfo *pMission = Singleton<IScenarioTracker>()->GetCurrentMission() )
	{
		std::wstring wszMissionName;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pMission->,LocalizedName) )
			wszMissionName = GET_TEXT_PRE(pMission->,LocalizedName);

		std::wstring wszCampaignName;
		const NDb::SCampaign *pCampaign = Singleton<IScenarioTracker>()->GetCurrentCampaign();
		if ( pCampaign )
			wszCampaignName = GET_TEXT_PRE(pCampaign->,LocalizedName) + L" - ";

		std::wstring wszName = InterfaceState()->GetTextEntry( "T_AUTO_SAVE_NAME" );
		if ( !wszName.empty() )
			NSaveLoad::MakeUniqueSave( wszName + wszCampaignName + wszMissionName + ( bOnWin ? L" 1" : L" 2" ), true, false, false );
	}
}

//*******************************************************************
//*                     CICMission                                  *
//*******************************************************************

CICMission::CICMission( ITransceiver *pTransceiver )
: nPlayerForWarFog( -1 )
{
	pTrans = pTransceiver;
	pMap = pTrans->GetMap();
}

void CICMission::PreCreate()
{
//	NI_ASSERT( !NMainLoop::GetTopInterface(), "Non-empty interface stack at mission start" ); // sanity check
	CInterfaceCommandBase<CInterfaceMission>::PreCreate();
}

void CICMission::PostCreate( CICMission::IInterface *pInterface )
{
#ifndef _FINALRELEASE
	// sanity check
	NI_ASSERT( NMainLoop::GetTopInterface() == 0, "Programmers: no interfaces are allowed under the CInterfaceMission" );
#endif

	if ( !pTrans )
	{
		SReplayInfo replay;
		if ( bReplay )
			replay = SReplayInfo( szReplayFileName );

		pTrans = CreateSinglePlayerTransceiver( replay, Singleton<IAILogic>() );

		if ( bReplay )
			pMap = pTrans->GetMap();
	}

	if ( nPlayerForWarFog < 0 )
		nPlayerForWarFog = Singleton<IAIScenarioTracker>()->GetLocalPlayer();
	else
		Singleton<IAIScenarioTracker>()->SetLocalPlayer( nPlayerForWarFog );
	pTrans->StartMission( pMap, Singleton<IAILogic>() );
	pInterface->NewMission( pMap, pTrans, Singleton<IScenarioTracker>(), nPlayerForWarFog  );

	NMainLoop::PushInterface( pInterface );
}

void CICMission::Configure( const char *pszConfig )
{
	if ( pTrans )
		return;

	std::vector<std::string> szStrings;
	for ( NStr::CStringIterator<char, std::string> it(pszConfig, ';'); !it.IsEnd(); it.Next() )
	{
		std::string szString;
		it.Get( &szString );
		szStrings.push_back( szString );
	}
	if ( szStrings.size() == 0 || szStrings.size() > 2 )
		NI_ASSERT( szStrings.size() >= 1, "Illegal usage of \"map\" command (i.e. map <map_id>|\"replay\";<file_name>;" );

	nPlayerForWarFog = -1;
	if ( szStrings[0] == "replay" )
	{
		pMap = 0;
		bReplay = true;
		szReplayFileName = szStrings[1];
		nPlayerForWarFog = NStr::ToInt( szStrings[2] );
	}
	else
	{
		pMap = GetMapInfo( szStrings[0] );
		bReplay = false;
		szReplayFileName = "";
		bFromInterface = szStrings.size() > 1;
		nPlayerForWarFog = Singleton<IAIScenarioTracker>()->GetLocalPlayer();
	}
}

// CInterfaceMission::SNewActionButton

void CInterfaceMission::SNewActionButton::Enable( bool _bEnable )
{
	bEnabled = _bEnable;

	if ( pBtn )
//		pBtn->Enable( !bPassive && bEnable );
		pBtn->Enable( !bPassive );

	if ( pIconBgDisabledWnd )
		pIconBgDisabledWnd->ShowWindow( bPassive || !bEnabled );
	if ( pIconFgDisabledWnd )
		pIconFgDisabledWnd->ShowWindow( !bPassive && !bEnabled );
}

void CInterfaceMission::SNewActionButton::SetProgress( float fProgress )
{
	if ( pClockWnd )
		pClockWnd->SetAngles( FP_PI2, FP_2PI + FP_PI2 - fProgress * FP_2PI );
}


void StartNewMap( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.empty() )
		return;
	const std::string szVal = NStr::ToMBCS( paramsSet[0] );

	NGlobal::RemoveVar( "Multiplayer.Host" );
	NGlobal::RemoveVar( "Multiplayer.Client" );

	std::vector<std::string> szStrings;
	for ( NStr::CStringIterator<char, std::string> it(szVal, ';'); !it.IsEnd(); it.Next() )
	{
		std::string szString;
		it.Get( &szString );
		szStrings.push_back( szString );
	}
	if ( szStrings.size() == 0 || szStrings.size() > 3 )
		NI_ASSERT( szStrings.size() >= 1, "Illegal usage of \"map\" command (i.e. map <map_id> <player>|\"replay\";<file_name>;" );
	if ( szStrings[0] == "replay" )
	{
	}
	else
	{
		//WARNING{ change this only with CCommandsHistory::LoadHistory
		// don't brake replay syncronization :)
		int nLocalPlayer = 0;
		if ( paramsSet.size() >= 2 )
			nLocalPlayer = NStr::ToInt( NStr::ToMBCS( paramsSet[1] ) );

		InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_SINGLE );
		Singleton<IAIScenarioTracker>()->SetLocalPlayer( nLocalPlayer );
		const NDb::SMapInfo *pInfo = GetMapInfo( szStrings[0] );
		if ( !pInfo )
		{
			Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "Map (%s) not found", szStrings[0].c_str() ) );
			return;
		}
		Singleton<IScenarioTracker>()->Clear();
		const int nDifficulty = 0;
		Singleton<IScenarioTracker>()->CustomMissionStart( pInfo, nDifficulty, false );
		//WARNING}
	}

	NMainLoop::Command( ML_COMMAND_MISSION, szVal.c_str() );
}

void BalanceTest( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	NGlobal::SetVar( "balance_test", NStr::ToInt( NStr::ToMBCS( paramsSet[0] ) ) );
	if ( paramsSet.size() == 1 )
		NGlobal::SetVar( "balance_test_n_iteration", 0 );
	StartNewMap( szID, paramsSet, pContext );
}

void ReplayHistory( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.empty() )
		return;

	const std::string szVal = NStr::ToMBCS( paramsSet[0] );
	std::string szPlayerForWarFog = "0";
	if ( paramsSet.size() >= 2 )
	{
		szPlayerForWarFog = NStr::ToMBCS( paramsSet[1] );
	}
	NMainLoop::Command( ML_COMMAND_MISSION, StrFmt( "replay;%s;%s", szVal.c_str(), szPlayerForWarFog .c_str() ) );
}
#ifndef _FINALRELEASE
#include "System/CheckSumLog.h"
#include "GameX/SaveLoadHelper.h"
#include "Common_RTS_AI/Checksums.h"
#include "Main/Profiles.h"

#include <zlib.h>

class CSimpleChecksumLog : public ICheckSumLog
{
	OBJECT_BASIC_METHODS( CSimpleChecksumLog );
	std::unordered_map<int, unsigned long> entries1;
	bool bEntries2;
public:
	CSimpleChecksumLog() : bEntries2( false ) {  }
	void SetEntries2()
	{
		bEntries2 = true;
	}
	virtual bool AddChecksumLog( const int nGameTime, const unsigned long ulChecksum, const int nEntry )
	{
		if ( bEntries2 )
		{
			const unsigned long entry = entries1[nEntry];
			if ( ulChecksum == entry )
			{

			}
			else
			{
				NI_ASSERT( ulChecksum == entry, "differ" );
				return false;
			}
		}
		else
			entries1[nEntry] = ulChecksum;
		return true;
	}
};

void CompareSaves( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() != 2 )
		csSystem << "usage: " << szID << "save_1" << "save_2" << endl;
	else
	{
OMG_I_USE_GOTO_AGAIN:

		try
		{
			NMainLoop::ResetStack();
			InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_MULTI );
			CPtr<CSimpleChecksumLog> pSumLog = new CSimpleChecksumLog;
			std::string szParam;

			// read first save
			{
				NStr::ToMBCS( &szParam, paramsSet[0] );
				const std::string szPathName = NProfile::GetCurrentProfileDir() + "Saves\\" + szParam + ".sav";

				CFileStream stream( szPathName, CFileStream::WIN_READ_ONLY );
				CPtr<IBinSaver> pSaver = CreateSaveLoadSaver( &stream, SAVER_MODE_READ_64 );
				if ( pSaver == 0 )
					return;
				NMainLoop::Serialize( *pSaver );
			}

			// pSaver checksum
			Singleton<IAILogic>()->LogCheckSum( pSumLog );
			Singleton<IAILogic>()->ClearAI();
			NMainLoop::ResetStack();
			InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_MULTI );

			// read next save
			{
				NStr::ToMBCS( &szParam, paramsSet[1] );
				const std::string szPathName = NProfile::GetCurrentProfileDir() + "Saves\\" + szParam + ".sav";

				CFileStream stream( szPathName, CFileStream::WIN_READ_ONLY );
				CPtr<IBinSaver> pSaver = CreateSaveLoadSaver( &stream, SAVER_MODE_READ_64 );
				if ( pSaver == 0 )
					return;
				NMainLoop::Serialize( *pSaver );
			}

			// save checksum to compare
			pSumLog->SetEntries2();
			Singleton<IAILogic>()->LogCheckSum( pSumLog );
		}
		catch( ... )
		{
			goto OMG_I_USE_GOTO_AGAIN;
		}
		NI_ASSERT( false, "saves identical" );
	}
}
#endif

void DemoExit( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	const std::string szParam = "Movies\\demo_final_outro.xml;final_exit";

	NMainLoop::ResetStack();
	NMainLoop::Command( ML_COMMAND_PLAY_MOVIE, szParam.c_str() );
}

START_REGISTER(MissionCommands)

REGISTER_VAR_EX( "chat_message_visible_time", NGlobal::VarIntHandler, &CHAT_MESSAGE_VISIBLE_TIME, 5000, STORAGE_NONE );
REGISTER_VAR_EX( "chat_message_max_visible_time", NGlobal::VarIntHandler, &CHAT_MESSAGE_MAX_VISIBLE_TIME, 15000, STORAGE_NONE );
REGISTER_VAR_EX( "chat_message_speed", NGlobal::VarIntHandler, &CHAT_MESSAGE_SPEED, 20, STORAGE_NONE );
REGISTER_VAR_EX( "chat_message_max_speed", NGlobal::VarIntHandler, &CHAT_MESSAGE_MAX_SPEED, 60, STORAGE_NONE );
REGISTER_VAR_EX( "chat_message_interval", NGlobal::VarIntHandler, &CHAT_MESSAGE_INTERVAL, 5, STORAGE_NONE );

REGISTER_VAR_EX( "warfog_update_period", NGlobal::VarIntHandler, &WARFOG_UPDATE_PERIOD, 1000, STORAGE_NONE );
REGISTER_VAR_EX( "warfog_border_width", NGlobal::VarIntHandler, &WARFOG_HARD_RECT_WIDTH, 2, STORAGE_NONE );
REGISTER_VAR_EX( "warfog_min_value", NGlobal::VarIntHandler, &WARFOG_MIN_VALUE, 128, STORAGE_NONE );

REGISTER_VAR_EX( "mission_buttons_bind_section", NGlobal::VarWStrHandler, &MISSION_BUTTONS_BIND_SECTION, L"mission_buttons_original", STORAGE_USER );

REGISTER_VAR_EX( "transition_effect_duration", NGlobal::VarIntHandler, &s_nTransitionEffectDuration, 800, STORAGE_NONE );
REGISTER_VAR_EX( "enable_scroll_transition", NGlobal::VarBoolHandler, &s_bEnableScrollTransition, false, STORAGE_NONE );

REGISTER_VAR_EX( "deep_map_load", NGlobal::VarBoolHandler, &s_bDeepMapLoad, false, STORAGE_USER );

REGISTER_CMD( "quick_load", CmdQuickLoad );
REGISTER_CMD( "quick_save", CmdQuickSave );
REGISTER_CMD( "load_game", CmdLoadGame );
REGISTER_CMD( "map", StartNewMap );
REGISTER_CMD( "balance_test", BalanceTest );

REGISTER_CMD( "replay", ReplayHistory );
REGISTER_CMD( "demo_exit", DemoExit );

#ifndef _FINALRELEASE
REGISTER_CMD( "compare_saves", CompareSaves );
#endif

FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( MISSION_INTERFACE, CInterfaceMission );
REGISTER_SAVELOAD_CLASS_NM( 0x1009DCC2, CReactions, CInterfaceMission );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MISSION, CICMission )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_LOAD_GAME, CICLoadB2 );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_SAVE_GAME, CICSaveB2 );



