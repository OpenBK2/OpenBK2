#include "stdafx.h"

#include "InterfaceScreenBase.h"

#include "AILogic/B2AI.h"
#include "Misc/StrProc.h"
#include "Sound/SoundScene.h"
#include "Sound/MusicSystem.h"
#include "SceneB2/Camera.h"
#include "SceneB2/Cursor.h"
#include "SceneB2/Scene.h"
#include "UI/WindowTextView.h"

#include "UI/SceneClassIDs.h"
#include "InterfaceState.h"
#include "GameXClassIDs.h"
#include "ScenarioTracker.h"
#include "Sound/SFX.h"
#include "GetConsts.h"
#include "System/Text.h"
#include "UI/UI.h"
#include "MultiplayerCommandManager.h"

#include <algorithm>

namespace NISB
{
	static CVec2 vCursorStoredPos = VNULL2;
};

bool AddUIScreen( IScreen *pWindowScreen, const std::string &szScreenEntryName, IProgrammedReactionsAndChecks *pReactions )
{
	IScreen *pScr = dynamic_cast<IScreen*>( pWindowScreen );
	if ( pScr == 0 )
		return false;
	//
	const CDBID dbidScreen = InterfaceState()->GetScreenEntryDBID( szScreenEntryName );
	if ( dbidScreen.IsEmpty() )
		return false;
	//
	pScr->SetGView( Scene()->GetG2DView() );
	pScr->Load( NDb::Get<NDb::SWindowScreen>(dbidScreen), pReactions );
	Singleton<IScene>()->AddScreen( pScr );
	pWindowScreen->RegisterObservers();
	//
	return true;
}

CInterfaceScreenBase::CInterfaceScreenBase( const std::string &_szInterfaceType, const std::string &_szBindSection )
: szInterfaceType( _szInterfaceType ), szBindSection( _szBindSection ),
	bShowScreenOnGetFocus( false ), bIsTransparent( true )
{
	bInFocus = false;
	nTime = 0; // 0 - need to set abs time at the appropriate place
	//	
	AddObserver( "win_mouse_move", &CInterfaceScreenBase::MsgMouseMove );
	AddObserver( "win_left_button_down", &CInterfaceScreenBase::MsgLButtonDown );
	AddObserver( "win_left_button_up", &CInterfaceScreenBase::MsgLButtonUp );
	AddObserver( "win_left_button_dblclk", &CInterfaceScreenBase::MsgLButtonDblClk );
	AddObserver( "win_right_button_down", &CInterfaceScreenBase::MsgRButtonDown );
	AddObserver( "win_right_button_up", &CInterfaceScreenBase::MsgRButtonUp );
	AddObserver( "win_right_button_dblclk", &CInterfaceScreenBase::MsgRButtonDblClk );
	AddObserver( "help_screen", &CInterfaceScreenBase::MsgHelpScreen );
	AddObserver( "show_console", &CInterfaceScreenBase::OnShowConsole );

	AddObserver( "mission_win_mouse_move_emit", &CInterfaceScreenBase::MsgMouseMove );
}

bool CInterfaceScreenBase::Init()
{
	//ChangeResolution(); //COMMENTED: it changes UI resolution in editor
	//
	Cursor()->SetMode( NDb::USER_ACTION_UNKNOWN );
	NInput::PurgeUIEvents();
	//
//	if ( IsShowHelpScreenOnInit() )
//		CheckedShowHelpScreen( false );
	vLastScreenSize = Scene()->GetScreenRect();

	return true;
}

CInterfaceScreenBase::~CInterfaceScreenBase()
{
	Cursor()->SetMode( NDb::USER_ACTION_UNKNOWN );
	NInput::PurgeUIEvents();
	if ( pScreen )
	{
		Scene()->RemoveScreen( pScreen );
		pScreen = 0;
	}
	for ( std::unordered_map<int,bool>::iterator it = registeredIDsForMLHandler.begin(); it != registeredIDsForMLHandler.end(); ++it )
	{
		int nID = it->first;
		InterfaceState()->UnregisterIDForMLHandler( nID );
	}
}

void CInterfaceScreenBase::ShowVersionInfo()
{
	if ( pVersionWindow == 0 && pScreen )
	{
		const int nVersionInfoWindow = NGlobal::GetVar( "version_info_text_window", -1 );
		if ( nVersionInfoWindow != -1 )
		{
			CDBPtr<NDb::SUIDesc> pDesc = 0;//NDb::Get<NDb::SWindowTextView>( nVersionInfoWindow );
			if ( pDesc )
			{
				pVersionWindow = new CWindowTextView();
				pVersionWindow->InitByDesc( pDesc );

				pVersionWindow->SetWidth( 500 );
				FillVersionWindow();
				const float fSize = pVersionWindow->GetSize().x;
				pVersionWindow->SetWidth( fSize + 10 );
				pScreen->AddChild( pVersionWindow, true );
			}
		}
	}
}

bool CInterfaceScreenBase::ProcessEvent( const struct SGameMessage &msg )
{
	if ( importantMsgs.ProcessEvent( msg, this ) )
		return true;
#ifdef _DEBUG
	{
		NInput::CBind p("show_console");
		if( p.ProcessEvent(msg) )
		{
			int nFunk;
			nFunk = 0;
		}
	}
#endif	


	if ( pScreen )
	{
		if ( pScreen->ProcessEvent( msg ) )
			return true;
	}

	if ( msg.mMessage.cType != NInput::CT_UNKNOWN && 
		(pScreen && !pScreen->IsEnabled()) )
	{
		return true;
	}

	if ( CGMORegContainer::ProcessEvent( msg, this ) )
		return true;

	if ( msg.mMessage.cType != NInput::CT_UNKNOWN && 
		(pScreen && pScreen->IsModal()) )
	{
		return true;
	}

	return Scene()->ProcessEvent( msg );
}

void CInterfaceScreenBase::OnMouseMove( const CVec2 &vPos  )
{
	//if ( pScreen )
	//	pScreen->OnMouseMove( vPos, 0 );
}

void CInterfaceScreenBase::OnButtonDown( const CVec2 &vPos, int nButton )
{
	//if ( pScreen )
	//	pScreen->OnButtonDown( vPos, nButton );
}

void CInterfaceScreenBase::OnButtonUp( const CVec2 &vPos, int nButton )
{
	//if ( pScreen )
	//	pScreen->OnButtonUp( vPos, nButton );
}

void CInterfaceScreenBase::OnButtonDblClk( const CVec2 &vPos, int nButton )
{
	//if ( pScreen )
	//	pScreen->OnButtonDblClk( vPos, nButton );
}

void CInterfaceScreenBase::StartInterface()
{
}

void CInterfaceScreenBase::Draw( NGScene::CRTPtr *pTexture ) 
{
	Scene()->Draw( pTexture ); 
}

void CInterfaceScreenBase::Step( bool bAppActive )
{
	if ( bAppActive == false ) 
	{
		if ( !GameTimer()->HasPause(PAUSE_TYPE_INACTIVE) )
		{
			// pause sound

			//
			GameTimer()->Pause( true, PAUSE_TYPE_INACTIVE );
			// return gamma value
			NGfx::SetGamma( false );
			//
			Cursor()->Acquire( false );
			NISB::vCursorStoredPos = Cursor()->GetPos();
		}
		//
		Singleton<IMPToUIManager>()->MPUISegment();
		return;
	}
	else
	{
		if ( GameTimer()->HasPause(PAUSE_TYPE_INACTIVE) )
		{
			// unpause sound

			//
			GameTimer()->Pause( false, PAUSE_TYPE_INACTIVE );
			// set gamma value
			NGfx::SetGamma( true );
			// 
			Cursor()->Acquire( true );
			Cursor()->SetPos( NISB::vCursorStoredPos.x, NISB::vCursorStoredPos.y );
		}

		ChangeResolution();
	}
	//
	ShowVersionInfo();
	// do local step for overloaded interface
	if ( (StepLocal(bAppActive) == false) || (bAppActive == false) /*|| !pGFX->IsActive()*/ )
	{
//		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		return;
	}
	// update sound
	{
		ICamera *pCamera = Camera();

		float fDist, fPitch, fYaw;
		pCamera->GetPlacement( &fDist, &fPitch, &fYaw );
		float fNear, fFar, fFOV;
		pCamera->GetCameraParams( &fNear, &fFar, &fFOV );
		const float fScreenWidth = fDist * tan( ToRadian(fFOV/2.0f) );

		CVec3 vAnchor = pCamera->GetAnchor();
		CVec3 vPosition = pCamera->GetPos();

		CVec3 vDir( vAnchor - vPosition );
		Normalize( &vDir );

		CVec3 vPos = pCamera->GetListener();
		Vis2AI( &vPos );
		
		//DebugTrace( "camera pos : %f, %f, %f", vPos.x, vPos.y, vPos.z );
		SoundScene()->UpdateSound( vPos, vDir, fScreenWidth );
		Singleton<IMusicSystem>()->Update();
	}
	
	// call UI screen segment
	NTimer::STime nCurrTime = Singleton<IGameTimer>()->GetAbsTime();
	int nDeltaTime = (nTime == 0) ? 0 : (std::max<int>)( 0, nCurrTime - nTime );
	if ( pScreen )
		pScreen->Segment( nDeltaTime );
	nTime = nCurrTime;
	
	if ( IsInFocus() )
	{
		// emmit mouse move 
		const CVec2 pos = Cursor()->GetPos();
		NInput::PostWinEvent( "mission_win_mouse_move_emit", PackCoords( CVec2( pos.x, pos.y) ), 0 );
		
		// draw entire scene
		Draw();
	}
}

bool CInterfaceScreenBase::ShouldLimitFrameRate() const
{
	// Keep menus and other GUI-only screens from rendering thousands of frames per second.
	return szInterfaceType != "Mission";
}

bool CInterfaceScreenBase::ChangeResolution()
{
	CVec2 vScreenSize = Scene()->GetScreenRect();
	// check for interface resolution
	if ( fabs2(vScreenSize - vLastScreenSize) > 1 )
	{
		if ( pScreen )
			pScreen->UpdateResolution();
	}
	vLastScreenSize = vScreenSize;
	return false;
}

void CInterfaceScreenBase::OnGetFocus( bool bFocus ) 
{
	if ( bFocus && bShowScreenOnGetFocus )
	{
		if ( pScreen )
			pScreen->ShowWindow( true );

		bShowScreenOnGetFocus = false;
	}
	
	if ( pScreen )
		pScreen->OnGetFocus( bFocus );

	if ( bFocus ) 
		RestoreBindSection();
	//
	bInFocus = bFocus;

	if ( bFocus )
	{
		InterfaceState()->SetSuppressEnableFocus( false );
		nTime = Singleton<IGameTimer>()->GetAbsTime();;
	}
}

void CInterfaceScreenBase::RestoreBindSection()
{
	NInput::SetSection( szBindSection );
}

bool CInterfaceScreenBase::MsgMouseMove( const struct SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	OnMouseMove( vPos );
	return false;
}

bool CInterfaceScreenBase::MsgLButtonDown( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	OnButtonDown( vPos, MSTATE_BUTTON1 );
	return false;
}

bool CInterfaceScreenBase::MsgLButtonUp( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	OnButtonUp( vPos, MSTATE_BUTTON1 );
	return false;
}

bool CInterfaceScreenBase::MsgLButtonDblClk( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	OnButtonDblClk( vPos, MSTATE_BUTTON1 );
	return false;
}

bool CInterfaceScreenBase::MsgRButtonDown( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	OnButtonDown( vPos, MSTATE_BUTTON2 );
	return false;
}

bool CInterfaceScreenBase::MsgRButtonUp( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	OnButtonUp( vPos, MSTATE_BUTTON2 );
	return false;
}

bool CInterfaceScreenBase::MsgRButtonDblClk( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	OnButtonDblClk( vPos, MSTATE_BUTTON2 );
	return false;
}

bool CInterfaceScreenBase::MsgHelpScreen( const struct SGameMessage &msg )
{
	return CheckedShowHelpScreen( true );
}

bool CInterfaceScreenBase::OnShowConsole( const struct SGameMessage &msg )
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( pST && pST->GetGameType() == IScenarioTracker::EGT_MULTI_FLAG_CONTROL )
	{
		if ( NGlobal::GetVar( "mp_allow_show_console", 0 ) == 0 )
			return false;
	}
	if ( !pScreen || !pScreen->IsVisible() )
		return false;
	IWindow * pConsole = Singleton<IDebugSingleton>()->GetConsole();
	if ( !pConsole )
	{
		NI_ASSERT( 0, "Programmers: no console found" );
		return false;
	}
	NI_VERIFY( IsValid( pConsole->GetParentWindow() ), "Invalid ptr", return false );
	if ( !pConsole->IsVisible() )
	{
		pScreen->RunStateCommandSequience( "ShowConsole", pConsole, 0, true );
		return true;
	}
	//else 
	//	pScreen->RunStateCommandSequience( "ShowConsole", pConsole, 0, false );
	return false;
}

void CInterfaceScreenBase::AfterLoad()
{
	Singleton<IScene>()->SetSceneConsts( NGameX::GetSceneConsts() );
	Singleton<IScene>()->AfterLoad();
	//
	if ( pScreen )
	{
		pScreen->RegisterObservers();
		pScreen->SetGView( Scene()->GetG2DView() );
	}
	nTime = 0; // 0 - need to set abs time at the appropriate place

	SetVersionWindowAfterLoad();
}

void CInterfaceScreenBase::PauseIntermission( bool bPause )
{
	if ( bPause )
	{
		Singleton<IAILogic>()->Suspend();
		Singleton<IGameTimer>()->Pause( true, PAUSE_TYPE_INTERFACE );
	}
	else
	{
		Singleton<IAILogic>()->Resume();
		Singleton<IGameTimer>()->Pause( false, PAUSE_TYPE_INTERFACE );
	}
	NInput::PostEvent( "show_game_paused", 0, 0 );
}

void CInterfaceScreenBase::AddScreen( struct IProgrammedReactionsAndChecks *pReactions )
{
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	if ( AddUIScreen( pScreen, szInterfaceType, pReactions ) == false )
		pScreen = 0;
}

void CInterfaceScreenBase::FillVersionWindow()
{
	std::wstring wszVersionInfo = NGlobal::GetVar( "version.info", "" ).GetString();
	wszVersionInfo = L"<color=green>" + wszVersionInfo;
	pVersionWindow->SetText( wszVersionInfo );
}

bool CInterfaceScreenBase::CheckedShowHelpScreen( bool bForced )
{
	if ( NGlobal::GetVar("game_mode_editor", 0) != 0 )
		return false;
	if ( szInterfaceType != "no_interface" )
	{
		const NDb::SUIScreenEntry *pEntry = InterfaceState()->GetScreenEntry( szInterfaceType );
		if ( pEntry )
		{
			bool bVisited = NGlobal::GetVar( std::string( "HelpScreen." ) + szInterfaceType, 0 ) != 0;
			if ( (CHECK_TEXT_NOT_EMPTY_PRE(pEntry->,HelpHeader) || CHECK_TEXT_NOT_EMPTY_PRE(pEntry->,HelpDesc)) && (bForced || !bVisited) )
			{
				if ( !pEntry->bHelpNoMultiplayer || Singleton<IScenarioTracker>()->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
				{
					NGlobal::SetVar( std::string( "HelpScreen." ) + szInterfaceType, 1, STORAGE_USER );
					NMainLoop::Command( ML_COMMAND_HELP_SCREEN, szInterfaceType.c_str() );
					return true;
				}
			}
		}
	}
	return false;
}

void CInterfaceScreenBase::SetVersionWindowAfterLoad()
{
	if ( pVersionWindow )
	{
		std::wstring wszOldBuildInfo = pVersionWindow->GetText();
		const int nPos = wszOldBuildInfo.find( L"\n\nLoaded from " );
		if ( nPos != std::wstring::npos )
			wszOldBuildInfo.erase( nPos, wszOldBuildInfo.size() );

		FillVersionWindow();
		const std::wstring wszNowBuildInfo = pVersionWindow->GetText();

		const std::wstring wszBuildInfo = wszNowBuildInfo + L"\n\nLoaded from " + wszOldBuildInfo;
		pVersionWindow->SetWidth( 500 );
		pVersionWindow->SetText( wszBuildInfo );
		pVersionWindow->SetWidth( pVersionWindow->GetSize().x + 10 );
		pScreen->RemoveChild( pVersionWindow );
		pScreen->AddChild( pVersionWindow, true );
	}
}

void CInterfaceScreenBase::HideUnfocusedScreen()
{
	if ( pScreen )
	{
		bShowScreenOnGetFocus = pScreen->IsVisible();
		if ( bShowScreenOnGetFocus )
			pScreen->ShowWindow( false );
	}
}

void CInterfaceScreenBase::SetDynamicTextView( ITextView *pView, const std::vector< std::pair<std::wstring, std::wstring> > &params )
{
	if ( pView )
	{
		int nOldID = pView->GetIDForMLHandler();
		if ( nOldID >= 0 )
			InterfaceState()->UnregisterIDForMLHandler( nOldID );
		int nID = InterfaceState()->GetAndRegisterIDForMLHandler( params );
		registeredIDsForMLHandler[nID] = true;
		pView->SetIDForMLHandler( nID );
	}
}

void CInterfaceScreenBase::SetDynamicTooltip( IWindow *pWnd, const std::wstring &wszTooltip, const std::vector< std::pair<std::wstring, std::wstring> > &params )
{
	if ( pWnd )
	{
		int nOldID = pWnd->GetTooltipIDForMLHandler();
		if ( nOldID >= 0 )
			InterfaceState()->UnregisterIDForMLHandler( nOldID );
		int nID = InterfaceState()->GetAndRegisterIDForMLHandler( params );
		registeredIDsForMLHandler[nID] = true;
		pWnd->SetTooltipIDForMLHandler( nID );
		pWnd->SetTooltip( wszTooltip );
	}
}

void CInterfaceScreenBase::SetMainWindowTexture( IWindow *pMainWindow, const NDb::STexture *pTexture )
{
	pMainWindow->SetTexture( pTexture );
	if ( pTexture )
		bIsTransparent = false;
}

int CInterfaceScreenBase::operator&( IBinSaver &saver )
{
	saver.Add( 1, const_cast<std::string*>( &szInterfaceType ) );
	saver.Add( 2, const_cast<std::string*>( &szBindSection ) );
	saver.Add( 3, &pScreen );
	saver.Add( 4, &pVersionWindow );
	saver.Add( 5, &bInFocus );
	saver.Add( 6, &vLastScreenSize );
	saver.Add( 7, &bShowScreenOnGetFocus );
	saver.Add( 8, &registeredIDsForMLHandler );
	saver.Add( 9, &bIsTransparent );
	return 0;
}


namespace NInterface
{

static void CmdStartInterface ( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.empty() ) 
		return;

	std::string res;
	NStr::ToMBCS( &res, paramsSet[0] );

	NGlobal::SetVar( "MainMenuDBID", NStr::ToInt( res ) );
	NGlobal::SetVar( "mainmenu", "1" );
}
}

START_REGISTER(Interfaces)
REGISTER_CMD( "start_interface", NInterface::CmdStartInterface )
FINISH_REGISTER

