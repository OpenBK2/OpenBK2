#include "StdAfx.h"
#include "revision.h"
#include "../mapeditorlib/resourcedefines.h"
#include "../misc/2darray.h"
#include "../stats_b2_m1/iconsset.h"
#include "../sceneb2/scene.h"
#include "../ED_B2_M1/ED_B2_M1DLL.h"
//
#include "../MapEditor/MapEditorApp.h"
#include "../MapEditorLib/MapEditorModule.h"
#include "../MapEditorLib/Interface_MainFrame.h"

#include "../ED_B2_M1/MapInfoEditor.h"
#include "../ED_B2_M1/ModelEditor.h"
#include "VFSDbObserver.h"
#include "../libdb/EditorDb.h"

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
	void PostStorageInitialize();
};
IEditorModule* GetEditorModule0();
IEditorModule* GetEditorModule1();
IEditorModule* GetEditorModule2();
IEditorModule* GetEditorModule3();
IEditorModule* GetEditorModule4();


BEGIN_BUTTON_MAP(COMMON_BUTTON_MAP)
#include "../MapEditor/ToolBarButtonsMap.h"
#include "ToolBarButtonsMapSpecific.h"
END_BUTTON_MAP()


class CEditorAppSpecific : public CEditorApp
{
	vector<IEditorModule*> extModules;
public:
	virtual BOOL InitInstance();

	void LoadMapEditorModule( const string &szModuleName );
	void UnloadMapEditorModule();
	const vector<IEditorModule*>& GetEditorModules();
	bool GameXInitialize() { return NGameX::Initialize(); }
	void GameXPostStorageInitialize() 
	{ 
		NGameX::PostStorageInitialize(); 
		if ( NGlobal::GetVar("delete_removed_object", 0) != 0 )
			NDb::AddDbObserver( NDb::CreateVFSDbObserver() );
	}
	const SECBtnMapEntry* GetToolbarButtonsMap() const	{ return COMMON_BUTTON_MAP; }
	void GetCursomToolBarsInfo( CCursomToolBarInfoList *pCursomToolBarInfoList ) const;
	void CreateMenus( IMainFrame *pMainFrame ) const;
};


void CEditorAppSpecific::LoadMapEditorModule( const string &szModuleName ) // "c:\\b2\\system\\b2.dle"
{
	if ( IEditorModule *pModule = GetEditorModule0() )
		extModules.push_back( pModule );
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


const vector<IEditorModule*>& CEditorAppSpecific::GetEditorModules()
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


BOOL CEditorAppSpecific::InitInstance()
{
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
	//
	SetMapFileName( "CMapEditorSingletonBase_B2MapEditor_1.0" );
	return CEditorApp::InitInstance();
}


void CEditorAppSpecific::GetCursomToolBarsInfo( CCursomToolBarInfoList *pCursomToolBarInfoList ) const
{
	AfxSetResourceHandle( theEDB2M1Instance );
	{
		CCursomToolBarInfoList::iterator itCursomToolBarInfo = pCursomToolBarInfoList->insert( pCursomToolBarInfoList->end(), SCursomToolBarInfo() );
		itCursomToolBarInfo->strName.LoadString( IDS_TOOLBAR_MAPINFO_TOOLS );
		itCursomToolBarInfo->nCount = TOOLBAR_MAPINFO_TOOLS_ELEMENTS_COUNT;
		itCursomToolBarInfo->pButtons = static_cast<const UINT*>( TOOLBAR_MAPINFO_TOOLS_ELEMENTS_ID );
	}
	{
		CCursomToolBarInfoList::iterator itCursomToolBarInfo = pCursomToolBarInfoList->insert( pCursomToolBarInfoList->end(), SCursomToolBarInfo() );
		itCursomToolBarInfo->strName.LoadString( IDS_TOOLBAR_MAPINFO_VIEW );
		itCursomToolBarInfo->nCount = TOOLBAR_MAPINFO_VIEW_ELEMENTS_COUNT;
		itCursomToolBarInfo->pButtons = static_cast<const UINT*>( TOOLBAR_MAPINFO_VIEW_ELEMENTS_ID );
	}
	{
		CCursomToolBarInfoList::iterator itCursomToolBarInfo = pCursomToolBarInfoList->insert( pCursomToolBarInfoList->end(), SCursomToolBarInfo() );
		itCursomToolBarInfo->strName.LoadString( IDS_TOOLBAR_MODEL );
		itCursomToolBarInfo->nCount = TOOLBAR_MODEL_ELEMENTS_COUNT;
		itCursomToolBarInfo->pButtons = static_cast<const UINT*>( TOOLBAR_MODEL_ELEMENTS_ID );
	}
	AfxSetResourceHandle( AfxGetInstanceHandle() );
}


void CEditorAppSpecific::CreateMenus( IMainFrame *pMainFrame ) const
{
	AfxSetResourceHandle( theEDB2M1Instance );
	vector<UINT> nIDs;
	nIDs.push_back( IDM_MAIN );
	nIDs.push_back( IDM_MAPINFO );
	nIDs.push_back( IDM_MODEL );
	pMainFrame->AddMenuResources( nIDs );
	AfxSetResourceHandle( AfxGetInstanceHandle() );
}

CEditorAppSpecific theApp;

