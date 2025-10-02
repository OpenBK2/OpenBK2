#include "StdAfx.h"
#include "..\ui\commandparam.h"
#include "..\ui\dbuserinterface.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "InterfaceMultiplayer.h"
#include "GameXClassIDs.h"
#include "..\UI\SceneClassIDs.h"
#include "..\SceneB2\Scene.h"
#include "InterfaceState.h"


#include "ScenarioTracker.h"
#include "../Misc/StrProc.h"


// CInterfaceMultiplayer

CInterfaceMultiplayer::CInterfaceMultiplayer() : 
	CInterfaceScreenBase( "Multiplayer", "multiplayer" )
{
}

CInterfaceMultiplayer::~CInterfaceMultiplayer()
{
	if ( pScreen ) 
		Scene()->RemoveScreen( pScreen );
}

bool CInterfaceMultiplayer::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	RegisterObservers();

	// load screen
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	AddUIScreen( pScreen, "Multiplayer", this );

	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_NO_NET );

	IButton *pLoadReplay = GetChildChecked<IButton>( pScreen, "ButtonLoadReplay", true );
	if ( pLoadReplay && NGlobal::GetVar( "mp_replays_enabled", 0 ) )
	{
		pLoadReplay->ShowWindow( true );
		pLoadReplay->Enable( true );
	}
	const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bKRIDemo )
	{
		IButton *pNivalNet = GetChildChecked<IButton>( pScreen, "ButtonNivalNet", true );
		if ( pNivalNet )
			pNivalNet->Enable( false );
	}

	const bool bMPDemo = NGlobal::GetVar( "MP_DEMO", 0 ) != 0;
	if ( bMPDemo )
	{
		IButton *pLANBtn = GetChildChecked<IButton>( pScreen, "button_lan", true );

		if ( pLANBtn )
			pLANBtn->Enable( false );
	}

	return true;
}

bool CInterfaceMultiplayer::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );

	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return bResult;
}

void CInterfaceMultiplayer::RegisterObservers()
{
}

void CInterfaceMultiplayer::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
}

bool CInterfaceMultiplayer::Execute( const string &szSender, const string &szReaction )
{
	if ( szReaction == "reaction_on_multiplayer_back" )
		return OnBack( szSender );

	if ( szReaction == "reaction_on_lan" )
		return OnLAN( szSender );

	if ( szReaction == "nival_net" )
		return OnNivalNet( szSender );

	if ( szReaction == "react_on_load_replay" )
		return OnLoadReplay( szSender );

	if ( szReaction == "reaction_on_profile_manager" )
		return OnProfileManagerMenu();

	return false;
}

int CInterfaceMultiplayer::Check( const string &szCheckName ) const
{
	return 0;
}

bool CInterfaceMultiplayer::OnBack( const string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
	return true;
}

bool CInterfaceMultiplayer::OnLAN( const string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_LAN_NET );
	NMainLoop::Command( ML_COMMAND_MP_CUSTOM_GAME_MENU, "" );
	return true;
}

bool CInterfaceMultiplayer::OnNivalNet( const string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_NIVAL_NET );
	NMainLoop::Command( ML_COMMAND_NIVAL_NET_MENU, "" );
	return true;
}

bool CInterfaceMultiplayer::OnLoadReplay( const string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_REPLAY_SAVE_LOAD, "load" );
	return true;
}

bool CInterfaceMultiplayer::OnProfileManagerMenu( )
{
	NMainLoop::Command( ML_COMMAND_PROFILE_MENU, "" );
	return true;
}

bool CInterfaceMultiplayer::ProcessEvent( const SGameMessage &msg )
{
	return CInterfaceScreenBase::ProcessEvent( msg );
}

void CInterfaceMultiplayer::AfterLoad()
{
	CInterfaceScreenBase::AfterLoad();

	RegisterObservers();
}

#ifndef _SINGLE_DEMO
// CICMultiplayer

void CICMultiplayer::PreCreate()
{
}

void CICMultiplayer::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMultiplayer::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x17130C81, CInterfaceMultiplayer );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MULTIPLAYER_MENU, CICMultiplayer );

#endif // _SINGLE_DEMO