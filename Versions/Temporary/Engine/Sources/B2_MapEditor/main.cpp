#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "SceneB2/Scene.h"

#include "ED_B2_M1_export.h"
#include "ED_B2_export.h"
#include "GameX_export.h"

//
#include "MapEditor/MapEditorApp.h"
#include "MapEditorLib/MapEditorModule.h"
#include "MapEditorLib/Interface_MainFrame.h"

#include "ED_B2_M1/MapInfoEditor.h"
#include "ED_B2_M1/ModelEditor.h"
#include "VFSDbObserver.h"
#include "libdb/EditorDb.h"
#include "port/crashpad.h"
#include "WxHost.h"

#include <zconf.h>

// ************************************************************************************************************************ //
// **
// ** map editor module registration
// **
// **
// **
// ************************************************************************************************************************ //

namespace NGameX
{
	bool Initialize();
	GAMEX_EXPORT void PostStorageInitialize();
};
ED_B2_M1_EXPORT IEditorModule* GetEditorModule1();
ED_B2_EXPORT IEditorModule* GetEditorModule2();
ED_B2_EXPORT IEditorModule* GetEditorModule3();
ED_B2_EXPORT IEditorModule* GetEditorModule4();


class CEditorAppSpecific : public CEditorApp
{
	std::vector<IEditorModule*> extModules;
public:
	bool Initialize( const std::vector<std::string> &rArgs ) override;

	void LoadMapEditorModule( const std::string &szModuleName );
	void UnloadMapEditorModule();
	const std::vector<IEditorModule*>& GetEditorModules();
	bool GameXInitialize() { return NGameX::Initialize(); }
	void GameXPostStorageInitialize() 
	{ 
		NGameX::PostStorageInitialize(); 
		if ( NGlobal::GetVar("delete_removed_object", 0) != 0 )
			NDb::AddDbObserver( NDb::CreateVFSDbObserver() );
	}
	void CreateMenus( IMainFrame *pMainFrame ) const;
};


void CEditorAppSpecific::LoadMapEditorModule( const std::string &szModuleName ) // "c:\\b2\\system\\b2.dle"
{
	// Module 0 was ED_RTS, whose nine hooks were all empty and which registered
	// no editor types; the numbering of the rest is left alone because those
	// names are exported from ED_B2_M1 and ED_B2.
	if ( IEditorModule *pModule = GetEditorModule1() )
		extModules.push_back( pModule );
	if ( IEditorModule *pModule = GetEditorModule2() )
		extModules.push_back( pModule );
	if ( IEditorModule *pModule = GetEditorModule3() )
		extModules.push_back( pModule );
	if ( IEditorModule *pModule = GetEditorModule4() )
		extModules.push_back( pModule );
	//
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	for ( int i = 0; i < extModules.size(); ++i )
	{
		extModules[i]->ModuleStartup();
	}
	//
	DebugTrace( "EditorApp() ModuleStartup(): %g", NHPTimer::GetTimePassed( &time ) );
}


const std::vector<IEditorModule*>& CEditorAppSpecific::GetEditorModules()
{
	return extModules;
}


void CEditorAppSpecific::UnloadMapEditorModule()
{
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	for ( int i = 0; i < extModules.size(); ++i )
	{
		extModules[i]->ModuleShutdown();
	}
	//
	DebugTrace( "EditorApp() ModuleShutdown(): %g", NHPTimer::GetTimePassed( &time ) );
}


bool CEditorAppSpecific::Initialize( const std::vector<std::string> &rArgs )
{
	// Before anything else, because everything after it is what wants watching.
	// The editor had no crash handler until now: a fault left a truncated
	// stingray trace and nothing to read, which is not enough to tell a crash
	// from a close. Ignore the result deliberately -- an editor with no handler
	// behind it is still an editor, and refusing to start would be a worse
	// answer than starting unwatched. Dumps land in bin/crashpad_db/reports.
	InitCrashpad();
	//
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
	//
	SetMapFileName( "CMapEditorSingletonBase_B2MapEditor_1.0" );
	// wx is up by now: this runs from the wxApp's OnInit (WxHost.cpp).
	return CEditorApp::Initialize( rArgs );
}


void CEditorAppSpecific::CreateMenus( IMainFrame *pMainFrame ) const
{
	std::vector<unsigned> nIDs;
	nIDs.push_back( IDM_MAIN );
	nIDs.push_back( IDM_MAPINFO );
	nIDs.push_back( IDM_MODEL );
	pMainFrame->AddMenuResources( nIDs );
}

// A global, as the CWinApp was, so that it exists -- and its constructor has
// set the CRT's debug flags -- before WinMain runs. wx's application object
// drives it: see WxHost.h.
namespace
{
	CEditorAppSpecific theEditorApp;
}


CEditorApp& NWxHost::GetEditorApp()
{
	return theEditorApp;
}

