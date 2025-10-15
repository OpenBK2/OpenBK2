#include "stdafx.h"
#include "InterfaceWinLoose.h"
#include "GameXClassIDs.h"
#include "InterfaceState.h"
#include "ScenarioTracker.h"
#include "3Dmotor/ScreenShot.h"
#include "SceneB2/Scene.h"
#include "SceneB2/Camera.h"
#include "SceneB2/FullScreenFader.h"
#include "System/Commands.h"
#include "Sound/MusicSystem.h"

#include "GameX_export.h"

static float END_GAME_FADE_TIME = 2.5f;
static float END_GAME_ROTATE_TIME = 10.0f;
static float END_GAME_CAMERA_YAW_SPEED = 1.0f;
const char* END_GAME_CAMERA_LOCKER = "end_game_locker";
const float SET_YAW_SPEED_DELAY = 0.1f;
const int SET_YAW_SPEED_STEPS = 5;
const float SET_YAW_SPEED_STEP_TIME = 0.1f;

// CInterfaceWinLoose

CInterfaceWinLoose::CInterfaceWinLoose() :
	CInterfaceScreenBase( "WinLooseDialog", "esc_menu" ),
	eUIState( EUIS_NORMAL ),
	timeAbsLast( 0 ),
	bWin( false ),
	nSetYawSpeedStep( 0 ),
	fSetYawSpeedTime( 0.0f )
{
}

bool CInterfaceWinLoose::Init()
{
	if (  Singleton<IScenarioTracker>()->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
	{
		//Make screenshot before anything else
		InterfaceState()->GetScreenShotTexture()->Generate( true );
	}

	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	AddScreen( this );
	
	MakeInterior();

	return true;
}

void CInterfaceWinLoose::MakeInterior()
{
	pMainWnd = GetChildChecked<IWindow>( GetScreen(), "Main", true );
	
	pWinView = GetChildChecked<ITextView>( pMainWnd, "WinView", true );
	pLostView = GetChildChecked<ITextView>( pMainWnd, "LostView", true );
}

bool CInterfaceWinLoose::ProcessEvent( const SGameMessage &msg )
{
	if ( msg.mMessage.cType == NInput::CT_KEY )
	{
		NextScreen();
		return true;
	}

	if ( msg.mMessage.cType != NInput::CT_UNKNOWN )
		return true;

	return CInterfaceScreenBase::ProcessEvent( msg );
}

void CInterfaceWinLoose::SetMode( bool _bWin )
{
	bWin = _bWin;

	if ( pWinView )
		pWinView->ShowWindow( bWin );
	if ( pLostView )
		pLostView->ShowWindow( !bWin );

	EndGameRotation();

	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const NDb::SMapInfo *pMapInfo = pST ? pST->GetLastMission() : 0;
	if ( pMapInfo )
	{
		const NDb::SMapMusic *pMusic = bWin ? pMapInfo->pMusicWin : pMapInfo->pMusicLost;
		if ( pMusic )
			Singleton<IMusicSystem>()->Init( pMusic, 0 );
	}
}

void CInterfaceWinLoose::NextScreen()
{
	Camera()->SwitchManualScrolling( END_GAME_CAMERA_LOCKER, true );
	Camera()->SetYawSpeed( 0.0f );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );

	if (  Singleton<IScenarioTracker>()->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
//		NMainLoop::Command( ML_COMMAND_HIDE_UNFOCUSED_SCREEN, "" );
		NMainLoop::Command( ML_COMMAND_SINGLE_STATISTIC, "" );
	}
	else
	{
		NMainLoop::Command( ML_COMMAND_HIDE_ALL_UP_TO, "Mission" );
		NMainLoop::Command( ML_COMMAND_HIDE_UNFOCUSED_SCREEN, "" );
		NMainLoop::Command( ML_COMMAND_MP_STATISTICS, "" );
	}
}

bool CInterfaceWinLoose::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );

	float fStepAbsTime = 0.0f;
	NTimer::STime timeAbsCur = Singleton<IGameTimer>()->GetAbsTime();
	if ( bAppActive && timeAbsLast > 0 )
		fStepAbsTime = (float)( timeAbsCur - timeAbsLast ) / 1000.0f;
	timeAbsLast = timeAbsCur;

	InterfaceStateStep( fStepAbsTime );

	return bResult;
}

void CInterfaceWinLoose::InterfaceStateStep( float fTime )
{
	if ( eUIState == EUIS_END_GAME_ROTATE_CAMERA )
	{
		if ( nSetYawSpeedStep > 0 )
		{
			fSetYawSpeedTime -= fTime;
			if ( fSetYawSpeedTime < 0.0f )
			{
				if ( fSetYawSpeedTime < -SET_YAW_SPEED_STEP_TIME )
				{
					fSetYawSpeedTime += SET_YAW_SPEED_STEP_TIME;
					Camera()->SetYawSpeed( END_GAME_CAMERA_YAW_SPEED / nSetYawSpeedStep );
					nSetYawSpeedStep--;
				}
			}
		}

		fEndGameRestTime -= fTime;
		if ( fEndGameRestTime <= 0.0f )
		{
			EndGameFade();
		}
	}
	else if ( eUIState == EUIS_END_GAME_FADE_SCREEN )
	{
		IFullScreenFader *pFader = Scene()->GetScreenFader();
		if ( !pFader || !pFader->IsInProgress() )
		{
			EndGameStatistics();
		}
	}
}

bool CInterfaceWinLoose::Execute( const std::string &szSender, const std::string &szReaction )
{
	return false;
}

int CInterfaceWinLoose::Check( const std::string &szCheckName ) const
{
	return 0;
}

void CInterfaceWinLoose::EndGameRotation()
{
	eUIState = EUIS_END_GAME_ROTATE_CAMERA;

	fEndGameRestTime = END_GAME_ROTATE_TIME;
	fSetYawSpeedTime = SET_YAW_SPEED_DELAY;
	
	CVec3 vPos = GetAnchor();
	if ( vPos != VNULL3 )
	{
		Camera()->SetAnchor( vPos );
		Camera()->SetYaw( 0.0f );
	}
	else
		Camera()->SetAnchor( Camera()->GetAnchor() );
	Camera()->SwitchManualScrolling( END_GAME_CAMERA_LOCKER, false );
	nSetYawSpeedStep = SET_YAW_SPEED_STEPS;
}

void CInterfaceWinLoose::EndGameFade()
{
	eUIState = EUIS_END_GAME_FADE_SCREEN;

	fEndGameRestTime = 0.0f;
	IFullScreenFader *pFader = Scene()->GetScreenFader();
	if ( pFader )
	{
		fEndGameRestTime = END_GAME_FADE_TIME;
		pFader->Start( END_GAME_FADE_TIME, SCREEN_FADER_CLEAR, SCREEN_FADER_BLACK, true );
	}
}

void CInterfaceWinLoose::EndGameStatistics()
{
	eUIState = EUIS_END_GAME_STATISTICS;
	
	NextScreen();
}

CVec3 CInterfaceWinLoose::GetAnchor() const
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();

	const NDb::SMapInfo *pMapInfo = pST->GetLastMission();
	if ( !pMapInfo )
		return VNULL3;

	if ( !pMapInfo->finalPositions.empty() )
	{
		int nIndex = 0;
		if ( !bWin && pMapInfo->finalPositions.size() >= 2 )
			nIndex = 1;
		CVec2 vPosVis2;
		AI2Vis( &vPosVis2, pMapInfo->finalPositions[nIndex] );
		return CVec3( vPosVis2.x, vPosVis2.y, 0.0f );
	}
	else
		return VNULL3;
}

// CICWinLooseDialog

void CICWinLooseDialog::PreCreate()
{
}

void CICWinLooseDialog::PostCreate( IInterface *pInterface )
{
	pInterface->SetMode( bWin );
	NMainLoop::PushInterface( pInterface );
}

void CICWinLooseDialog::Configure( const char *pszConfig )
{
	bWin = false;

	if ( !pszConfig )
		return;

	if ( strcmp( pszConfig, "win" ) == 0 )
		bWin = true;
}

START_REGISTER(WinLooseCommands)

REGISTER_VAR_EX( "win_loose_rotate_speed", NGlobal::VarFloatHandler, &END_GAME_CAMERA_YAW_SPEED, 1.0f, STORAGE_NONE );
REGISTER_VAR_EX( "win_loose_rotate_time", NGlobal::VarFloatHandler, &END_GAME_ROTATE_TIME, 10.0f, STORAGE_NONE );
REGISTER_VAR_EX( "win_loose_fade_time", NGlobal::VarFloatHandler, &END_GAME_FADE_TIME, 2.5f, STORAGE_NONE );

FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( GAMEX, 0x11119CC0, CInterfaceWinLoose )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_WIN_LOOSE, CICWinLooseDialog )


