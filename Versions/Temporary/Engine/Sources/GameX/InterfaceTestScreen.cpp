#include "stdafx.h"
#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "InterfaceTestScreen.h"
#include "GameXClassIDs.h"
#include "SceneB2/Cursor.h"
#include "SceneB2/Scene.h"
#include "UI/SceneClassIDs.h"
#include "Misc/StrProc.h"

#include "GameX_export.h"

#include <zconf.h>

// CInterfaceTestScreen

CInterfaceTestScreen::CInterfaceTestScreen() :
	CInterfaceScreenBase( "TestScreen", "default" )
{
}

bool CInterfaceTestScreen::Init()
{
	Cursor()->Show( true );
	Cursor()->SetMode( NDb::USER_ACTION_UNKNOWN );
	
	return true;
}

void CInterfaceTestScreen::SetScreen( const CDBID &_dbid )
{
	dbid = _dbid;
	if ( dbid.IsEmpty() )
		return;
	
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );

	pScreen->SetGView( Scene()->GetG2DView() );
	pScreen->Load( NDb::Get<NDb::SWindowScreen>(dbid), this );
	Singleton<IScene>()->AddScreen( pScreen );
	
	pScreen->RegisterObservers();

}

bool CInterfaceTestScreen::StepLocal( bool bAppActive )
{
	if ( !CInterfaceScreenBase::StepLocal( bAppActive ) )
		return false;

	
	return true;
}

// CICTestScreen

void CICTestScreen::PreCreate( )
{
}

void CICTestScreen::PostCreate( IInterface *pInterface )
{
	pInterface->SetScreen( dbid );
	NMainLoop::PushInterface( pInterface );
}

void CICTestScreen::Configure( const char *pszConfig )
{
	dbid = pszConfig;
}


void StartTestScreen( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
#ifndef _FINALRELEASE
	if ( !paramsSet.empty() ) 
	{
		const std::string szVal = NStr::ToMBCS( paramsSet[0] );
		NMainLoop::Command( ML_COMMAND_TEST_SCREEN, szVal.c_str() );
	}
#endif //_FINALRELEASE
}

START_REGISTER(TestScreenCommands)
REGISTER_CMD( "test_screen", StartTestScreen )
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( GAMEX, 0x171682C1, CInterfaceTestScreen )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_TEST_SCREEN, CICTestScreen )


