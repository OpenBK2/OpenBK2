#include "StdAfx.h"
#include "InterfaceMissionBackground.h"
#include "GameXClassIDs.h"
#include "InterfaceState.h"
#include "DBGameRoot.h"
#include "Transceiver.h"
#include "../AILogic/B2AI.h"
#include "../Sound/SFX.h"
#include "ScenarioTracker.h"
#include "../SceneB2/Camera.h"
#include "../Misc/StrProc.h"

#ifndef _FINALRELEASE
static int nInterfaceMissionBackgroundCount = 0; // sanity check
#endif

// CInterfaceMissionBackground

CInterfaceMissionBackground::CInterfaceMissionBackground() :
	CInterfaceMissionBase( "MissionBackground", "no_section" )
{
#ifndef _FINALRELEASE
	NI_ASSERT( nInterfaceMissionBackgroundCount == 0, "Duplicate background mission" );
	++nInterfaceMissionBackgroundCount;
#endif

	AddObserver( "unload_background_mission", &CInterfaceMissionBackground::MsgUnloadMission );
	AddObserver( "try_exit_windows", &CInterfaceMissionBackground::MsgTryExitWindows );
}

CInterfaceMissionBackground::~CInterfaceMissionBackground()
{
#ifndef _FINALRELEASE
	--nInterfaceMissionBackgroundCount;
#endif
}

bool CInterfaceMissionBackground::Init()
{
	if ( !CInterfaceMissionBase::Init() )
		return false;

	NGlobal::SetVar( "MissionIconsMovieMode", 1.0f );
	
	const NDb::SGameRoot *pGameRoot = InterfaceState()->GetGameRoot();
	const NDb::SMapInfo *pMapInfo = pGameRoot ? pGameRoot->mainMenuBackground.pMap : 0;
	const NDb::STexture *pPicture = pGameRoot ? pGameRoot->mainMenuBackground.pPicture : 0;
	
	pScenarioTracker = CreateScenarioTracker();

	if ( pPicture && !pMapInfo )
	{
		AddScreen( 0 );
		IWindow *pMain = GetChildChecked<IWindow>( GetScreen(), "Main", true );
		if ( pMain )
			SetMainWindowTexture( pMain, pPicture );
	}
	
	Singleton<ISFX>()->Pause( false );

	return true;
}

void CInterfaceMissionBackground::StartMissionMap( const NDb::SMapInfo *pMapInfo )
{
	if ( !pMapInfo || NGlobal::GetVar( "CRAP.MainMenu.BackgroundMap", 0 ) != 0 )
		return;

	CPtr<ITransceiver> pTrans = CreateSinglePlayerTransceiver( SReplayInfo(), Singleton<IAILogic>() );
	StartNewMap( pMapInfo, pTrans, pScenarioTracker );

	// Set camera
	const int nPlayer = 0;
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
		Camera()->SetAnchor( CVec3( 10, 10, 0 ) );
}

void CInterfaceMissionBackground::MsgUnloadMission( const SGameMessage &msg )
{
	Unload();
	pScenarioTracker = 0;

	NInput::PostEvent( "menu_continue_play", 0, 0 );
}

void CInterfaceMissionBackground::MsgTryExitWindows( const SGameMessage &msg )
{
	NInput::PostEvent( "exit", 0, 0 );
}

void CInterfaceMissionBackground::OnSerialize( IBinSaver &saver )
{
	//{ CRAP - for compability with old saves
	if ( saver.IsReading() )
	{
		if ( !pScenarioTracker )
		{
			pScenarioTracker = Singleton<IScenarioTracker>();
		}
	}
	//}
}

bool CInterfaceMissionBackground::StepLocal( bool bAppActive )
{
	bool bNeedToDraw = true;
	for ( IInterfaceBase *pInterface = NMainLoop::GetTopInterface(); pInterface != this, pInterface; pInterface = NMainLoop::GetPrevInterface( pInterface ) )
	{
		CDynamicCast<CInterfaceScreenBase> pScreenBaseInterface = pInterface;
		if ( pScreenBaseInterface && !pScreenBaseInterface->IsTransparent() )
		{
			bNeedToDraw = false;
			break;
		}
	}

	if ( bNeedToDraw )
	{
		if ( !pWorld )
		{
			const NDb::SGameRoot *pGameRoot = InterfaceState()->GetGameRoot();
			const NDb::SMapInfo *pMapInfo = pGameRoot ? pGameRoot->mainMenuBackground.pMap : 0;

			if ( pMapInfo )
			{
				StartMissionMap( pMapInfo );
				PauseIntermission( false );
			}
		}
	}
	else
	{
		if ( pWorld )
			Unload();
	}

	return CInterfaceMissionBase::StepLocal( bAppActive );
}

// CICMissionBackground

void CICMissionBackground::PreCreate()
{
}

void CICMissionBackground::PostCreate( IInterface *pInterface )
{
#ifndef _FINALRELEASE
	// sanity check
	NI_ASSERT( NMainLoop::GetTopInterface() == 0, "Programmers: no interfaces are allowed under the CInterfaceMissionBackground" );
#endif
	NMainLoop::PushInterface( pInterface );
}

void CICMissionBackground::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x17117B81, CInterfaceMissionBackground )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MISSION_BACKGROUND, CICMissionBackground )


