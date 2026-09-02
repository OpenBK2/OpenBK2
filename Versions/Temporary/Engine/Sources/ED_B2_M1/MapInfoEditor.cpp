#include "stdafx.h"

#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ObjectController.h"
#include "SimpleObjectInfoData.h"
#include "UI/CommandParam.h"
#include "MapInfoEditor.h"

#include "MapInfoEditor.h"
#include "MapInfoBuilder.h"
#include "MapEditorLib/EditorFactory.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "libdb/ResourceManager.h"

#include "EditorScene.h"

#include "UISpecificB2/DBUISpecificB2.h"

#include <afxwin.h>

#include "MapEditorLib/DefaultTabWindow.h"
#include "EditorMethods.h"

// Docking Windows
#include "ScriptAreaWindow.h"
#include "CameraPositionWindow.h"
#include "UnitStartCmdWindow.h"
#include "MapObjectWindow.h"
#include "VSOWindow.h"
#include "AdvClipboardWindow.h"
#include "FieldWindow.h"
#include "HeightWindowV3.h"
#include "ReinfPointsWindow.h"
#include "ScriptCameraWindow.h"
#include "AIGeneralWindow.h"

//#include "MoviesEditorWindow.h"

#include "MapInfoViewFilterDlg.h"

#include <cstdint>

//#include "../GameX/DBGameRoot.h"
//#include "../GameX/DBConsts.h"
//#include "../GameX/GetConsts.h"
//#include "../AILogic/CreateAI.h"

#include "Stats_B2_M1/TerraAIObserver.h"
#include "ED_B2_M1_export.h"

REGISTER_EDITOR_IN_DLL( MapInfo, CMapInfoEditor )

#define RUN_GAME_BAT_FILE_PATH "Editor\\RunGame.bat"
#define GAME_CFG_FILE_PATH "Profiles\\Game.cfg"
#define GAME_CFG_NEW_FILE_PATH "Editor\\Game.cfg.new"
#define GAME_CFG_BACKUP_FILE_PATH "Editor\\Game.cfg.backup"


ED_B2_M1_EXPORT const unsigned TOOLBAR_MAPINFO_TOOLS_ELEMENTS_ID[TOOLBAR_MAPINFO_TOOLS_ELEMENTS_COUNT] = 
{
	ID_TOOLS_RESET_CAMERA,
	ID_TOOLS_UPDATE_VSO,
	ID_SEPARATOR,
	ID_TOOLS_FIT_TO_GRID,
	ID_TOOLS_ROTATE_90,
	ID_TOOLS_DRAW_SHOOT_AREAS,
	ID_TOOLS_DRAW_AI_MAP,
	ID_TOOLS_DRAW_PASSABILITY,
	ID_TOOLS_SHOW_GRID,
	ID_SEPARATOR,
	ID_VIEW_FILTER,
};


ED_B2_M1_EXPORT const unsigned TOOLBAR_MAPINFO_VIEW_ELEMENTS_ID[TOOLBAR_MAPINFO_VIEW_ELEMENTS_COUNT] = 
{
	ID_MI_VIEW_MINIMAP,
	ID_MI_VIEW_TOOL,
};


CMapInfoEditor::CMapInfoEditor()
	: nMapInfoToolsToolbarID( 0xFFFFFFFF ), 
		nMapInfoViewToolbarID( 0xFFFFFFFF ), 
		pMapInfo( 0 ), 
		pwndShortcutBar( 0 ), 
		pwndMiniMap( 0 ), 
		pwndMoviesEditor( 0 ),
		pMapInfoState( 0 ), 
		heightContainer( AI_TILE_SIZE * AI_TILES_IN_VIS_TILE * 1.0f )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_EDITOR, this );
	//
	Singleton<ICommandHandlerContainer>()->Register( CHID_MAPINFO_EDITOR, ID_TOOLS_RESET_CAMERA, ID_TOOLS_REGEN_VSO_NORMALS );
	Singleton<ICommandHandlerContainer>()->Register( CHID_MAPINFO_EDITOR, ID_UPDATE_SCENE_SIZE, ID_UPDATE_SCENE_VIEW );
	Singleton<ICommandHandlerContainer>()->Register( CHID_MAPINFO_EDITOR, ID_MI_VIEW_MINIMAP, ID_MI_VIEW_TOOLS_TOOLBAR );
}


//CRAP{ PLAIN_TEXT

void CMapInfoEditor::CreateControls()
{
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );

	// Сначала грузим файл с установками редактора
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		pUserData->SerializeSettings( editorSettings, "MapInfo", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );
	}

	// создаем minimap docking window
	//const std::string szDebugParam = Singleton<IUserDataContainer>()->Get()->szDebugParam;
	//const bool bShowMiniMap = CStringManager::GetBoolValueFromString( szDebugParam, "ShowMiniMap", 0, ";: ,|\t", true );
	unsigned nID = ID_MAPINFO_EDITOR_MINIMAP_DW;
	if ( pwndMiniMap = Singleton<IMainFrameContainer>()->Get()->CreateControlBar( &nID, "MiniMap", CBRS_ALIGN_ANY, AFX_IDW_DOCKBAR_LEFT, 0.2f, 265 ) )
	{
		if ( wndMiniMap.Create( pwndMiniMap ) )
		{
			Singleton<IMainFrameContainer>()->Get()->SetControlBarWindowContents( pwndMiniMap, &wndMiniMap );
			pwndMiniMap->ShowWindow( SW_SHOW );
			wndMiniMap.ShowWindow( SW_SHOW );
		}
	}
	DebugTrace( "CMapInfoEditor::CreateControls(): Create minimap window: %g", NHPTimer::GetTimePassed( &time ) );
	
	CString strPaneLabel;
	nID = ID_MAPINFO_EDITOR_SHORTCUT_DW;
	// создаем shortcut docking window
	if ( pwndShortcutBar = Singleton<IMainFrameContainer>()->Get()->CreateControlBar( &nID, "ShortcutBar", CBRS_ALIGN_ANY, AFX_IDW_DOCKBAR_LEFT, 0.8f, 265 ) )
	{
		nID = ID_MAPINFO_EDITOR_SHORTCUT_PANE_0;
		if ( wndShortcutBar.Create( pwndShortcutBar, WS_CHILD | WS_VISIBLE | SEC_OBS_VERT | SEC_OBS_ANIMATESCROLL, nID ) )
		{
			++nID;

			// Terrain
			//
			if ( CDefault3DTabWindow *p3DTabWindow = wndShortcutBar.AddNewShortcut(static_cast<CDefault3DTabWindow*>(0)) )
			{
				p3DTabWindow->Create( &wndShortcutBar, WS_CHILD | WS_VISIBLE | TWS_TABS_ON_BOTTOM | TWS_DRAW_3D_NORMAL );
				// height V3
				if ( CHeightWindowV3 *pDialog = p3DTabWindow->AddNewTab(static_cast<CHeightWindowV3*>(0)) )
				{
					AfxSetResourceHandle( theEDB2M1Instance );
					pDialog->Create( CHeightWindowV3::IDD, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					++nID;
					strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::TERRAIN_INPUT_SUSBSTATE_LABEL_ID[CMapInfoState::TERRAIN_ISS_HEIGHT_V3] );
					p3DTabWindow->AddTab( pDialog, strPaneLabel );
				}
				// field
				if ( CFieldWindow *pDialog = p3DTabWindow->AddNewTab(static_cast<CFieldWindow*>(0)) )
				{
					AfxSetResourceHandle( theEDB2M1Instance );
					pDialog->Create( CFieldWindow::IDD, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					++nID;
					strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::TERRAIN_INPUT_SUSBSTATE_LABEL_ID[CMapInfoState::TERRAIN_ISS_FIELD] );
					p3DTabWindow->AddTab( pDialog, strPaneLabel );
				}
				//
				CMapInfoEditorSettings::CActiveStateMap::const_iterator posActiveStateMap = editorSettings.activeStateMap.find( CMapInfoState::IS_TERRAIN );
				if ( ( posActiveStateMap != editorSettings.activeStateMap.end() ) &&
						 ( posActiveStateMap->second >= 0 ) &&
						 ( posActiveStateMap->second < CMapInfoState::INPUT_SUBSTATE_COUNT[CMapInfoState::IS_TERRAIN] ) )
				{
					p3DTabWindow->ActivateTab( posActiveStateMap->second );
				}
				else
				{
					p3DTabWindow->ActivateTab( CMapInfoState::DEFAULT_INPUT_SUBSTATE[CMapInfoState::IS_TERRAIN] );
				}
				//
				strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::INPUT_STATE_LABEL_ID[CMapInfoState::IS_TERRAIN] );
				wndShortcutBar.AddBar( p3DTabWindow, strPaneLabel, true );
			}

			// Objects
			//
			if ( CDefault3DTabWindow *p3DTabWindow = wndShortcutBar.AddNewShortcut(static_cast<CDefault3DTabWindow*>(0)) )
			{
				p3DTabWindow->Create( &wndShortcutBar, WS_CHILD | WS_VISIBLE | TWS_TABS_ON_BOTTOM | TWS_DRAW_3D_NORMAL );
				// map objects
				{
					CMapObjectWindow *pWindow = new CMapObjectWindow( false );
					p3DTabWindow->AddNewTab( pWindow );
					AfxSetResourceHandle( theEDB2M1Instance );
					pWindow->Create( CMapObjectWindow::IDD_NO_BUTTONS, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					++nID;
					strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::OBJECT_INPUT_SUSBSTATE_LABEL_ID[CMapInfoState::OBJECT_ISS_MAP_OBJECT] );
					p3DTabWindow->AddTab( pWindow, strPaneLabel );
				}
				// VSO
				{
					CVSOWindow *pWindow = new CVSOWindow();
					p3DTabWindow->AddNewTab( pWindow );
					AfxSetResourceHandle( theEDB2M1Instance );
					pWindow->Create( CVSOWindow::IDD, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					++nID;
					strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::OBJECT_INPUT_SUSBSTATE_LABEL_ID[CMapInfoState::OBJECT_ISS_VSO] );
					p3DTabWindow->AddTab( pWindow, strPaneLabel );
				}		
				//
				CMapInfoEditorSettings::CActiveStateMap::const_iterator posActiveStateMap = editorSettings.activeStateMap.find( CMapInfoState::IS_OBJECT );
				if ( ( posActiveStateMap != editorSettings.activeStateMap.end() ) &&
						 ( posActiveStateMap->second >= 0 ) &&
						 ( posActiveStateMap->second < CMapInfoState::INPUT_SUBSTATE_COUNT[CMapInfoState::IS_OBJECT] ) )
				{
					p3DTabWindow->ActivateTab( posActiveStateMap->second );
				}
				else
				{
					p3DTabWindow->ActivateTab( CMapInfoState::DEFAULT_INPUT_SUBSTATE[CMapInfoState::IS_OBJECT] );
				}
				//
				strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::INPUT_STATE_LABEL_ID[CMapInfoState::IS_OBJECT] );
				wndShortcutBar.AddBar( p3DTabWindow, strPaneLabel, true );
			}

			// Gameplay
			//
			if ( CDefault3DTabWindow *p3DTabWindow = wndShortcutBar.AddNewShortcut(static_cast<CDefault3DTabWindow*>(0)) )
			{
				p3DTabWindow->Create( &wndShortcutBar, WS_CHILD | WS_VISIBLE | TWS_TABS_ON_BOTTOM | TWS_DRAW_3D_NORMAL );
				// reinforcement points
				if ( CReinfPointsWindow *pDialog = p3DTabWindow->AddNewTab(static_cast<CReinfPointsWindow*>(0)) )
				{
					AfxSetResourceHandle( theEDB2M1Instance );
					pDialog->Create( CReinfPointsWindow::IDD, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					++nID;
					strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::GAMEPLAY_INPUT_SUSBSTATE_LABEL_ID[CMapInfoState::GAMEPLAY_ISS_REINF_POINTS] );
					p3DTabWindow->AddTab( pDialog, strPaneLabel );
				}
				// start camera positions
				if ( CCameraPositionWindow *pDialog = p3DTabWindow->AddNewTab(static_cast<CCameraPositionWindow*>(0)) )
				{
					AfxSetResourceHandle( theEDB2M1Instance );
					pDialog->Create( CCameraPositionWindow::IDD, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					++nID;
					strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::GAMEPLAY_INPUT_SUSBSTATE_LABEL_ID[CMapInfoState::GAMEPLAY_ISS_START_CAMERA] );
					p3DTabWindow->AddTab( pDialog, strPaneLabel );
				}						
				// ai general
				if ( CAIGeneralPointsWindow *pDialog = p3DTabWindow->AddNewTab( static_cast<CAIGeneralPointsWindow*>(0)) )
				{
					AfxSetResourceHandle( theEDB2M1Instance );
					pDialog->Create( CAIGeneralPointsWindow::IDD, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					++nID;
					strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::GAMEPLAY_INPUT_SUSBSTATE_LABEL_ID[CMapInfoState::GAMEPLAY_ISS_AIGENERAL] );
					p3DTabWindow->AddTab( pDialog, strPaneLabel );
				}
				// unit start command
				if ( CUnitStartCmdWindow *pDialog = p3DTabWindow->AddNewTab(static_cast<CUnitStartCmdWindow*>(0)) )
				{
					AfxSetResourceHandle( theEDB2M1Instance );
					pDialog->Create( CUnitStartCmdWindow::IDD, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					++nID;
					strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::GAMEPLAY_INPUT_SUSBSTATE_LABEL_ID[CMapInfoState::GAMEPLAY_ISS_UNIT_START_CMD] );
					p3DTabWindow->AddTab( pDialog, strPaneLabel );
				}
				//
				CMapInfoEditorSettings::CActiveStateMap::const_iterator posActiveStateMap = editorSettings.activeStateMap.find( CMapInfoState::IS_GAMEPLAY );
				if ( ( posActiveStateMap != editorSettings.activeStateMap.end() ) &&
						 ( posActiveStateMap->second >= 0 ) &&
						 ( posActiveStateMap->second < CMapInfoState::INPUT_SUBSTATE_COUNT[CMapInfoState::IS_GAMEPLAY] ) )
				{
					p3DTabWindow->ActivateTab( posActiveStateMap->second );
				}
				else
				{
					p3DTabWindow->ActivateTab( CMapInfoState::DEFAULT_INPUT_SUBSTATE[CMapInfoState::IS_GAMEPLAY] );
				}
				//
				strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::INPUT_STATE_LABEL_ID[CMapInfoState::IS_GAMEPLAY] );
				wndShortcutBar.AddBar( p3DTabWindow, strPaneLabel, true );
			}

			// Script
			//
			if ( CDefault3DTabWindow *p3DTabWindow = wndShortcutBar.AddNewShortcut(static_cast<CDefault3DTabWindow*>(0)) )
			{
				p3DTabWindow->Create( &wndShortcutBar, WS_CHILD | WS_VISIBLE | TWS_TABS_ON_BOTTOM | TWS_DRAW_3D_NORMAL );
				// script area
				if ( CScriptAreaWindow *pDialog = p3DTabWindow->AddNewTab(static_cast<CScriptAreaWindow*>(0)) )
				{
					AfxSetResourceHandle( theEDB2M1Instance );
					pDialog->Create( CScriptAreaWindow::IDD, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					++nID;
					strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::SCRIPT_INPUT_SUSBSTATE_LABEL_ID[CMapInfoState::SCRIPT_ISS_SCRIPT_AREAS] );
					p3DTabWindow->AddTab( pDialog, strPaneLabel );
				}
				// script movies
				if ( CScriptCameraWindow *pDialog = p3DTabWindow->AddNewTab(static_cast<CScriptCameraWindow*>(0)) )
				{
					AfxSetResourceHandle( theEDB2M1Instance );
					pDialog->Create( CScriptCameraWindow::IDD, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					++nID;
					strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::SCRIPT_INPUT_SUSBSTATE_LABEL_ID[CMapInfoState::SCRIPT_ISS_SCRIPT_MOVIES] );
					p3DTabWindow->AddTab( pDialog, strPaneLabel );
				}
				//
				CMapInfoEditorSettings::CActiveStateMap::const_iterator posActiveStateMap = editorSettings.activeStateMap.find( CMapInfoState::IS_SCRIPT );
				if ( ( posActiveStateMap != editorSettings.activeStateMap.end() ) &&
						 ( posActiveStateMap->second >= 0 ) &&
						 ( posActiveStateMap->second < CMapInfoState::INPUT_SUBSTATE_COUNT[CMapInfoState::IS_SCRIPT] ) )
				{
					p3DTabWindow->ActivateTab( posActiveStateMap->second );
				}
				else
				{
					p3DTabWindow->ActivateTab( CMapInfoState::DEFAULT_INPUT_SUBSTATE[CMapInfoState::IS_SCRIPT] );
				}
				//
				strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::INPUT_STATE_LABEL_ID[CMapInfoState::IS_SCRIPT] );
				wndShortcutBar.AddBar( p3DTabWindow, strPaneLabel, true );
			}
			/**
			// Advanced clipboard
			//
			if ( CDefault3DTabWindow *p3DTabWindow = wndShortcutBar.AddNewShortcut(static_cast<CDefault3DTabWindow*>(0)) )
			{
				p3DTabWindow->Create( &wndShortcutBar, WS_CHILD | WS_VISIBLE | TWS_TABS_ON_BOTTOM | TWS_DRAW_3D_NORMAL );
				// advanced clipboard
				if ( CAdvClipboardWindow *pDialog = p3DTabWindow->AddNewTab(static_cast<CAdvClipboardWindow*>(0)) )
				{
					AfxSetResourceHandle( theEDB2M1Instance );
					pDialog->Create( CAdvClipboardWindow::IDD, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					++nID;
					strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::ADV_INPUT_SUSBSTATE_LABEL_ID[CMapInfoState::ADV_ISS_CLIPBOARD] );
					p3DTabWindow->AddTab( pDialog, strPaneLabel );
				}		
				//
				CMapInfoEditorSettings::CActiveStateMap::const_iterator posActiveStateMap = editorSettings.activeStateMap.find( CMapInfoState::IS_ADVANCED );
				if ( ( posActiveStateMap != editorSettings.activeStateMap.end() ) &&
						 ( posActiveStateMap->second >= 0 ) &&
						 ( posActiveStateMap->second < CMapInfoState::INPUT_SUBSTATE_COUNT[CMapInfoState::IS_ADVANCED] ) )
				{
					p3DTabWindow->ActivateTab( posActiveStateMap->second );
				}
				else
				{
					p3DTabWindow->ActivateTab( CMapInfoState::DEFAULT_INPUT_SUBSTATE[CMapInfoState::IS_ADVANCED] );
				}
				//
				strPaneLabel.LoadString( theEDB2M1Instance, CMapInfoState::INPUT_STATE_LABEL_ID[CMapInfoState::IS_ADVANCED] );
				wndShortcutBar.AddBar( p3DTabWindow, strPaneLabel, true );
			}
			/**/
			//
			if ( ( editorSettings.nActiveStateIndex >= 0 ) &&
					 ( editorSettings.nActiveStateIndex < CMapInfoState::IS_COUNT ) )
			{
				wndShortcutBar.SelectPane(editorSettings.nActiveStateIndex );
			}
			else
			{
				wndShortcutBar.SelectPane( CMapInfoState::DEFAULT_INPUT_STATE );
			}
			Singleton<IMainFrameContainer>()->Get()->SetControlBarWindowContents( pwndShortcutBar, &wndShortcutBar );
			pwndShortcutBar->ShowWindow( SW_SHOW );
			wndShortcutBar.ShowWindow( SW_SHOW );
			wndShortcutBar.SetCommandHandlerID( CHID_MAPINFO_STATE, ID_MIS_CHANGE_STATE );
		}
	}
	DebugTrace( "CMapInfoEditor::CreateControls(): Create shotrcut window: %g", NHPTimer::GetTimePassed( &time ) );

	// создаем movies editor docking window
	//const bool bShowMoviesEditor = CStringManager::GetBoolValueFromString( szDebugParam, "ShowMoviesEditor", 0, ";: ,|\t", true );
	//if ( bShowMoviesEditor )
	nID = ID_MOVIES_EDITOR_DW;
	if ( pwndMoviesEditor = Singleton<IMainFrameContainer>()->Get()->CreateControlBar( &nID, "MoviesEditor", CBRS_ALIGN_ANY, AFX_IDW_DOCKBAR_BOTTOM, 0.5f, 200 ) )
	{
		if ( wndMoviesEditor.Create( pwndMoviesEditor ) )
		{
			Singleton<IMainFrameContainer>()->Get()->SetControlBarWindowContents( pwndMoviesEditor, &wndMoviesEditor );
			pwndMoviesEditor->ShowWindow( SW_SHOW );
			wndMoviesEditor.ShowWindow( SW_SHOW );
		}
	}
	DebugTrace( "CMapInfoEditor::CreateControls(): Create movies editor window: %g", NHPTimer::GetTimePassed( &time ) );

	AfxSetResourceHandle( theEDB2M1Instance );

	CString strToolbarName;
	strToolbarName.LoadString( IDS_TOOLBAR_MAPINFO_VIEW );
	Singleton<IMainFrameContainer>()->Get()->AddToolBarResource( IDT_MAPINFO_VIEW, IDT_MAPINFO_VIEW );
	Singleton<IMainFrameContainer>()->Get()->CreateToolBar( &nMapInfoViewToolbarID,
																													strToolbarName,
																													TOOLBAR_MAPINFO_VIEW_ELEMENTS_COUNT,
																													TOOLBAR_MAPINFO_VIEW_ELEMENTS_ID,
 																													CBRS_ALIGN_ANY,
																													AFX_IDW_DOCKBAR_TOP,
																													true,
																													false,
																													true );
	strToolbarName.LoadString( IDS_TOOLBAR_MAPINFO_TOOLS );
	Singleton<IMainFrameContainer>()->Get()->AddToolBarResource( IDT_MAPINFO_TOOLS, IDT_MAPINFO_TOOLS );
	Singleton<IMainFrameContainer>()->Get()->CreateToolBar( &nMapInfoToolsToolbarID,
																													strToolbarName,
																													TOOLBAR_MAPINFO_TOOLS_ELEMENTS_COUNT,
																													TOOLBAR_MAPINFO_TOOLS_ELEMENTS_ID,
 																													CBRS_ALIGN_ANY,
																													AFX_IDW_DOCKBAR_TOP,
																													true,
																													false,
																													true );
	AfxSetResourceHandle( AfxGetInstanceHandle() );
}
//CRAP} PLAIN_TEXT


void CMapInfoEditor::PostCreateControls()
{
	if ( pwndShortcutBar != 0 )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndShortcutBar, false, true );
	}
	if ( pwndMiniMap != 0 )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndMiniMap, false, true );
	}
	if ( pwndMoviesEditor != 0 )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndMoviesEditor, false, true );
	}

	if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nMapInfoToolsToolbarID ) )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, false, true );
	}
	if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nMapInfoViewToolbarID ) )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, false, true );
	}
}


void CMapInfoEditor::PreDestroyControls()
{
	if ( pwndShortcutBar != 0 )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndShortcutBar, false, true );
	}
	if ( pwndMiniMap != 0 )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndMiniMap, false, true );
	}
	if ( pwndMoviesEditor != 0 )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndMoviesEditor, false, true );
	}

	if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nMapInfoToolsToolbarID ) )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, false, true );
	}
	if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nMapInfoViewToolbarID ) )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, false, true );
	}
}


void CMapInfoEditor::DestroyControls()
{
	// разрушаем shortcut docking window 
	if ( pwndShortcutBar != 0 )
	{
		if ( ::IsWindow( pwndShortcutBar->m_hWnd ) )
		{
			pwndShortcutBar->DestroyWindow();
		}
		delete pwndShortcutBar;
		pwndShortcutBar = 0;
	}
	wndShortcutBar.DestroyWindow();

	// разрушаем minimap docking window
	const std::string szDebugParam = Singleton<IUserDataContainer>()->Get()->szDebugParam;
	const bool bShowMiniMap = CStringManager::GetBoolValueFromString( szDebugParam, "ShowMiniMap", 0, ";: ,|\t", true );
	if ( bShowMiniMap )
	{
		if ( pwndMiniMap != 0 )
		{
			if ( ::IsWindow( pwndMiniMap->m_hWnd ) )
			{
				pwndMiniMap->DestroyWindow();
			}
			delete pwndMiniMap;
			pwndMiniMap = 0;
		}
		wndMiniMap.Destroy();
	}

	// разрушаем Movies Editor docking window
	const bool bShowMoviesEditor = CStringManager::GetBoolValueFromString( szDebugParam, "ShowMoviesEditor", 0, ";: ,|\t", true );
	if ( bShowMoviesEditor )
	{
		if ( pwndMoviesEditor != 0 )
		{
			if ( ::IsWindow( pwndMoviesEditor->m_hWnd ) )
			{
				pwndMoviesEditor->DestroyWindow();
			}
			delete pwndMoviesEditor;
			pwndMoviesEditor = 0;
		}
		wndMoviesEditor.DestroyWindow();
	}

	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MAPINFO_EDITOR );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_EDITOR );
	//Destroy();
}


void CMapInfoEditor::Create()
{
	// Сначала грузим файл с установками редактора
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		pUserData->SerializeSettings( editorSettings, "MapInfo", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );
	}

	if ( pwndShortcutBar )
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndShortcutBar, editorSettings.bShowShortcutBar, true );

	if ( pwndMiniMap )
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndMiniMap, editorSettings.bShowMinimapBar, true );
	
	if ( pwndMoviesEditor )
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndMoviesEditor, editorSettings.bShowMoviesEditor, true );

	if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar(nMapInfoToolsToolbarID) )
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, editorSettings.bShowMapInfoToolsToolbar, true );

	if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar(nMapInfoViewToolbarID) )
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, editorSettings.bShowMapInfoViewToolbar, true );


	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	if ( !pMapInfoState )
		pMapInfoState = new CMapInfoState( this );

	//
	DebugTrace( "CMapInfoEditor::Create(): Create mapinfo state: %g", NHPTimer::GetTimePassed( &time ) );
	//
	AfxSetResourceHandle( theEDB2M1Instance );
	Singleton<IMainFrameContainer>()->Get()->ShowMenu( IDM_MAPINFO );
	AfxSetResourceHandle( AfxGetInstanceHandle() );
}


void CMapInfoEditor::Destroy()
{
	if ( Singleton<IMainFrameContainer>() &&
			 Singleton<IMainFrameContainer>()->Get() &&
			 Singleton<IMainFrameContainer>()->GetSECWorkbook() )
	{
		AfxSetResourceHandle( theEDB2M1Instance );
		Singleton<IMainFrameContainer>()->Get()->ShowMenu( IDM_MAIN );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		//
		if ( pwndShortcutBar )
		{
			editorSettings.bShowShortcutBar = pwndShortcutBar->IsVisible();
			Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndShortcutBar, false, true );
		}
		if ( pwndMiniMap )
		{
			editorSettings.bShowMinimapBar = pwndMiniMap->IsVisible();
			Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndMiniMap, false, true );
		}
		if ( pwndMoviesEditor )
		{
			editorSettings.bShowMoviesEditor = pwndMoviesEditor->IsVisible();
			Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndMoviesEditor, false, true );
		}

		if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nMapInfoToolsToolbarID ) )
		{
			editorSettings.bShowMapInfoToolsToolbar = pToolbar->IsVisible();
			Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, false, true );
		}
		if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nMapInfoViewToolbarID ) )
		{
			editorSettings.bShowMapInfoViewToolbar = pToolbar->IsVisible();
			Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, false, true );
		}
		// Записываем файл с установками (его могли поменять во время работы редактора)
		{
			SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
			pUserData->SerializeSettings( editorSettings, "MapInfo", SUserData::EDITOR_SETTINGS, SUserData::ST_SAVE );
		}
		//
		if ( pMapInfoState )
		{
			delete pMapInfoState;
			pMapInfoState = 0;
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
	}
}


void CMapInfoEditor::Save( bool bSaveChanges )
{
	if ( IsModified() && bSaveChanges )
	{
		try
		{
			if ( !NEditor::SaveTerrain( EditorScene()->GetTerraManager() ) )
			{
				NLog::GetLogger()->Log( LT_ERROR, "Failed to save map\n" );
				NLog::GetLogger()->Log( LT_ERROR, StrFmt("\tObjectID: %s\n", NDb::GetResName(pMapInfo)) );
			}
		}
		catch ( ... ) 
		{
		}
	}
	//
	CMapInfoBuilder::EnsureMinimapMaterialAndTexture( GetViewManipulator(), pMapInfo->GetDBID() );
	CreateMinimapImage();
	wndMiniMap.LoadMap( pMapInfo );
	wndMiniMap.RedrawWindow();
	//
	CEditorBase::Save( bSaveChanges );
}


bool CMapInfoEditor::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID ) 
	{
		case ID_TOOLS_RUN_GAME:
		{
			RunGame();
			return true;
		}
		//
		case ID_TOOLS_RESET_CAMERA:
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_RESET_CAMERA, true );
			return true;
		}
		//
		case ID_TOOLS_FIT_TO_GRID:
		{
      editorSettings.bFitToGrid = !editorSettings.bFitToGrid;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_FENCE_STATE, ID_TOOLS_FIT_TO_GRID, uint32_t(editorSettings.bFitToGrid) );
			return true;
		}
		//
		case ID_TOOLS_ROTATE_90:
		{
			editorSettings.bRotateTo90Degree = !editorSettings.bRotateTo90Degree;
			return true;
		}
		//
		case ID_TOOLS_SHOW_GRID:
		{
			if ( pMapInfo != 0 )
			{
				editorSettings.viewFilterData.bShowGrid = !editorSettings.viewFilterData.bShowGrid;
				ApplyViewFilter();
				return true;
			}
			return false;
		}
		//
		case ID_TOOLS_REGEN_VSO_NORMALS:
		{
			CPtr<CObjectBaseController> pObjectController = CreateController();
			IManipulator *pManipulator = GetViewManipulator();

			// roads
			for ( int i = 0; i < pMapInfo->roads.size(); ++i )
			{
				const NDb::SVSOInstance &road = pMapInfo->roads[i];
				if ( road.points.size() < 2 )
					continue;

				bool bResult = true;
				for ( int nPointIndex = 0; nPointIndex < road.points.size(); ++nPointIndex )
				{
					CVec3 vNorm = ( nPointIndex == 0 ) ? ( road.points[1].vPos - road.points[0].vPos ) : ( road.points[nPointIndex].vPos - road.points[nPointIndex - 1].vPos );
					Normalize( &vNorm );
					vNorm.Set( vNorm.y, -vNorm.x, 0.0f );

					bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( StrFmt("Roads.[%d].points.[%d].Norm", i, nPointIndex), vNorm, pManipulator );
				}
				if ( bResult )
				{
					pObjectController->Redo( false, true, this );
				}
			}

			// rivers
			for ( int i = 0; i < pMapInfo->rivers.size(); ++i )
			{
				const NDb::SVSOInstance &river = pMapInfo->rivers[i];
				if ( river.points.size() < 2 )
					continue;

				bool bResult = true;
				for ( int nPointIndex = 0; nPointIndex < river.points.size(); ++nPointIndex )
				{
					CVec3 vNorm = ( nPointIndex == 0 ) ? ( river.points[1].vPos - river.points[0].vPos ) : ( river.points[nPointIndex].vPos - river.points[nPointIndex - 1].vPos );
					Normalize( &vNorm );
					vNorm.Set( vNorm.y, -vNorm.x, 0.0f );

					bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( StrFmt("Rivers.[%d].points.[%d].Norm", i, nPointIndex), vNorm, pManipulator );
				}
				if ( bResult )
				{
					pObjectController->Redo( false, true, this );
				}
			}

			// crags
			for ( int i = 0; i < pMapInfo->crags.size(); ++i )
			{
				const NDb::SVSOInstance &crag = pMapInfo->crags[i];
				if ( crag.points.size() < 2 )
					continue;

				bool bResult = true;
				for ( int nPointIndex = 0; nPointIndex < crag.points.size(); ++nPointIndex )
				{
					CVec3 vNorm = ( nPointIndex == 0 ) ? ( crag.points[1].vPos - crag.points[0].vPos ) : ( crag.points[nPointIndex].vPos - crag.points[nPointIndex - 1].vPos );
					Normalize( &vNorm );
					vNorm.Set( vNorm.y, -vNorm.x, 0.0f );

					bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( StrFmt("Crags.[%d].points.[%d].Norm", i, nPointIndex), vNorm, pManipulator );
				}
				if ( bResult )
				{
					pObjectController->Redo( false, true, this );
				}
			}

			// lakes
			for ( int i = 0; i < pMapInfo->lakes.size(); ++i )
			{
				const NDb::SVSOInstance &lake = pMapInfo->lakes[i];
				if ( lake.points.size() < 2 )
					continue;

				bool bResult = true;
				for ( int nPointIndex = 0; nPointIndex < lake.points.size(); ++nPointIndex )
				{
					CVec3 vNorm = ( nPointIndex == 0 ) ? ( lake.points[1].vPos - lake.points[0].vPos ) : ( lake.points[nPointIndex].vPos - lake.points[nPointIndex - 1].vPos );
					Normalize( &vNorm );
					vNorm.Set( vNorm.y, -vNorm.x, 0.0f );

					bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( StrFmt("Lakes.[%d].points.[%d].Norm", i, nPointIndex), vNorm, pManipulator );
				}
				if ( bResult )
				{
					pObjectController->Redo( false, true, this );
				}
			}
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EXPORTER, ID_TOOLS_REGEN_GEOMETRY, 0 );
		}
		//
		case ID_TOOLS_UPDATE_VSO:
		{
			if ( IEditorScene *pEditorScene = EditorScene() )
			{
				if ( pEditorScene->GetTerraManager() )
				{
					pEditorScene->GetTerraManager()->UpdateAllObjectsInGeomModifyingArea();
					pEditorScene->GetTerraManager()->UpdateRiversDepthes();
				}
			}
			return true;
		}
		//
		case ID_VIEW_FILTER:
		{
			ConfigureViewFilter();
			return true;
		}
		//
		case ID_TOOLS_DRAW_SHOOT_AREAS:
		{
			editorSettings.bDrawShootAreas = !editorSettings.bDrawShootAreas;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return true;
		}
		//
		case ID_TOOLS_DRAW_AI_MAP:
		{
			editorSettings.bDrawAIMap = !editorSettings.bDrawAIMap;
			if ( IEditorScene *pEditorScene = EditorScene() )
			{
				while ( pEditorScene->ToggleAIGeometryMode() != editorSettings.bDrawAIMap );
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return true;
		}
		//
		case ID_TOOLS_DRAW_PASSABILITY:
		{
			editorSettings.bDrawPassability = !editorSettings.bDrawPassability;
			if ( IEditorScene *pEditorScene = EditorScene() )
			{
				while( pEditorScene->ToggleShow( SCENE_SHOW_PASS_MARKERS ) != editorSettings.bDrawPassability );
			}

			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return true;
		}
		//
		case ID_UPDATE_SCENE_SIZE:
		{
			NI_ASSERT( IsPacked2DCoords( dwData ), "CMapInfoEditor::HandleCommand: dwData is not packed coords" );
			if ( IsPacked2DCoords( dwData ) )
			{
				const CVec2 size = UnPackCoords( dwData );
				wndMiniMap.SetMapInfoEditorSize( size.x, size.y );
			}
			//if ( wndMoviesEditor )
			//	wndMoviesEditor.ReDraw();
			return true;
		}
		//
		case ID_UPDATE_SCENE_VIEW:
		{
			if ( pwndMiniMap )
			{
				pwndMiniMap->Invalidate();
			}
			//if ( pwndMoviesEditor )
			//	pwndMoviesEditor->Invalidate();
			return true;
		}
		//
		case ID_MIMCO_GENERATE_MINIMAP_IMAGE:
		{
			CreateMinimapImage();
			wndMiniMap.LoadMap( pMapInfo );
			wndMiniMap.RedrawWindow();
			return true;
		}
		//
		case ID_VIEW_APPLY_MI_FILTER:
		{
			ApplyViewFilter();
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return true;
		}
		//
		case ID_MI_VIEW_MINIMAP:
		{
			if ( pwndMiniMap != 0 ) 
			{
				Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndMiniMap, !pwndMiniMap->IsVisible(), true );
			}
			return true;
		}
		//
		case ID_MI_VIEW_MOVIE_EDITOR:
		{
			if ( pwndMoviesEditor != 0 ) 
			{
				Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndMoviesEditor, dwData != 0, true );
			}
			return true;
		}
		//
		case ID_MI_VIEW_TOOL:
		{
			if ( pwndShortcutBar != 0 ) 
			{
				Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndShortcutBar, !pwndShortcutBar->IsVisible(), true );
			}
			return true;
		}
		//
		case ID_MI_VIEW_VIEW_TOOLBAR:
		{
			if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nMapInfoViewToolbarID ) )
			{
				Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, !pToolbar->IsVisible(), true );
			}
			return true;
		}
		//
		case ID_MI_VIEW_TOOLS_TOOLBAR:
		{
			if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nMapInfoToolsToolbarID ) )
			{
				Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, !pToolbar->IsVisible(), true );
			}
			return true;
		}
		//
		default:
			return false;
	} 
	return false;
}

bool CMapInfoEditor::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CMapInfoEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CMapInfoEditor::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID ) 
	{
	case ID_UPDATE_SCENE_SIZE:
	case ID_UPDATE_SCENE_VIEW:
	case ID_MIMCO_GENERATE_MINIMAP_IMAGE:
	{
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	}
	//
	case ID_TOOLS_FIT_TO_GRID:
	{
		( *pbEnable ) = true;
		( *pbCheck ) = editorSettings.bFitToGrid;
		return true;
	}
	//
	case ID_TOOLS_ROTATE_90:
	{
		( *pbEnable ) = true;
		( *pbCheck ) = editorSettings.bRotateTo90Degree;
		return true;
	}
	//
	case ID_TOOLS_SHOW_GRID:
	{
		( *pbEnable ) = ( pMapInfo != 0 );
		( *pbCheck ) = editorSettings.viewFilterData.bShowGrid;
		return true;
	}
	//
	case ID_TOOLS_UPDATE_VSO:
	case ID_VIEW_FILTER:
	case ID_TOOLS_RUN_GAME:
	case ID_TOOLS_RESET_CAMERA:
	{
		( *pbEnable ) = ( pMapInfo != 0 );
		( *pbCheck ) = false;
		return true;
	}
	//
	case ID_TOOLS_DRAW_SHOOT_AREAS:
	{
		( *pbEnable ) = ( pMapInfo != 0 );
		( *pbCheck ) = editorSettings.bDrawShootAreas;
		return true;
	}
	//
	case ID_TOOLS_DRAW_AI_MAP:
	{
		( *pbEnable ) = ( pMapInfo != 0 );
		( *pbCheck ) = editorSettings.bDrawAIMap;
		return true;
	}
	//
	case ID_TOOLS_DRAW_PASSABILITY:
	{
		( *pbEnable ) = ( pMapInfo != 0 );
		( *pbCheck ) = editorSettings.bDrawPassability;
		return true;
	}
	//
	case ID_MI_VIEW_MINIMAP:
	{
		if ( pwndMiniMap != 0 ) 
		{
			( *pbEnable ) = true;
			( *pbCheck ) = pwndMiniMap->IsVisible();
		}
		else
		{
			( *pbEnable ) = false;
			( *pbCheck ) = false;
		}
		return true;
	}
	//
	case ID_MI_VIEW_MOVIE_EDITOR:
	{
		if ( pwndMoviesEditor != 0 ) 
		{
			( *pbEnable ) = true;
			( *pbCheck ) = pwndMoviesEditor->IsVisible();
		}
		else
		{
			( *pbEnable ) = false;
			( *pbCheck ) = false;
		}
		return true;
	}
	//
	case ID_MI_VIEW_TOOL:
	{
		if ( pwndShortcutBar != 0 ) 
		{
			( *pbEnable ) = true;
			( *pbCheck ) = pwndShortcutBar->IsVisible();
		}
		else
		{
			( *pbEnable ) = false;
			( *pbCheck ) = false;
		}
		return true;
	}
	//
	case ID_MI_VIEW_VIEW_TOOLBAR:
	{
		if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nMapInfoViewToolbarID ) )
		{
			( *pbEnable ) = true;
			( *pbCheck ) = pToolbar->IsVisible();
		}
		else
		{
			( *pbEnable ) = false;
			( *pbCheck ) = false;
		}
		return true;
	}
	//
	case ID_MI_VIEW_TOOLS_TOOLBAR:
	{
		if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nMapInfoToolsToolbarID ) )
		{
			( *pbEnable ) = true;
			( *pbCheck ) = pToolbar->IsVisible();
		}
		else
		{
			( *pbEnable ) = false;
			( *pbCheck ) = false;
		}
		return true;
	}
	//
	case ID_TOOLS_REGEN_VSO_NORMALS:
	{
		( *pbEnable ) = ( pMapInfo != 0 );
		( *pbCheck ) = false;
		return true;
	}
	default:
		return false;
	}
	return false;
}
//

void CMapInfoEditor::GetChangesFromController( CObjectBaseController *pObjectController, bool bRedo )
{
	std::string szControllerTemporaryLabel;
	pObjectController->GetTemporaryLabel( &szControllerTemporaryLabel );
	if ( szControllerTemporaryLabel == CMapInfoController::TEMPORARY_LABEL )
	{
		CMapInfoController *pMapInfoController = dynamic_cast<CMapInfoController*>( pObjectController );
		if ( pMapInfoController != 0 )
		{
			if ( IEditorScene *pEditorScene = EditorScene() )
			{
				if ( ITerraManager *pTerraManager = pEditorScene->GetTerraManager() )
				{
					if ( !pMapInfoController->tileUndoDataList.empty() )
					{
						CMapInfoController::CTileUndoDataList::const_iterator posTileUndoData;
						if ( bRedo )
						{
							posTileUndoData = pMapInfoController->tileUndoDataList.begin();
						}
						else
						{
							posTileUndoData = pMapInfoController->tileUndoDataList.end();
						}
						while ( posTileUndoData != ( bRedo ? pMapInfoController->tileUndoDataList.end() :
																								 pMapInfoController->tileUndoDataList.begin() ) )
						{
							if ( !bRedo )
							{
								--posTileUndoData;
							}
							//
							if ( bRedo )
							{
								pTerraManager->UpdateTileAreaType( posTileUndoData->vBrushPos.x, posTileUndoData->vBrushPos.y, posTileUndoData->brush, NTerraBrush::TERRA_BRUSH_ADD );
							}
							else
							{
								pTerraManager->UpdateTileAreaType( posTileUndoData->vBrushPos.x, posTileUndoData->vBrushPos.y, posTileUndoData->brush, NTerraBrush::TERRA_BRUSH_SUB );
							}
							//
							if ( bRedo )
							{
								++posTileUndoData;
							}
						}
						pTerraManager->UpdateAfterTilesModifying();
						SetModified( true );
					}
					if ( !pMapInfoController->heightUndoDataList.empty() )
					{
						CMapInfoController::CHeightUndoDataList::const_iterator posHeightUndoData;
						if ( bRedo )
						{
							posHeightUndoData = pMapInfoController->heightUndoDataList.begin();
						}
						else
						{
							posHeightUndoData = pMapInfoController->heightUndoDataList.end();
						}
						while ( posHeightUndoData != ( bRedo ? pMapInfoController->heightUndoDataList.end() :
																									 pMapInfoController->heightUndoDataList.begin() ) )
						{
							if ( !bRedo )
							{
								--posHeightUndoData;
							}
							//
							if ( bRedo )
							{
								pTerraManager->ModifyTerraGeometryByBrush( posHeightUndoData->brushPos.x,
																													 posHeightUndoData->brushPos.y,
																													 true,
																													 posHeightUndoData->brush,
																													 NTerraBrush::TERRA_BRUSH_ADD );
							}
							else
							{
								pTerraManager->ModifyTerraGeometryByBrush( posHeightUndoData->brushPos.x,
																													 posHeightUndoData->brushPos.y,
																													 true,
																													 posHeightUndoData->brush,
																													 NTerraBrush::TERRA_BRUSH_SUB );
							}
							//
							if ( bRedo )
							{
								++posHeightUndoData;
							}
						}
						pTerraManager->UpdateAfterTilesModifying();
						SetModified( true );
					}
					if ( !pMapInfoController->vsoUndoDataList.empty() )
					{
						//NDb::SMapInfo* pMutableMapInfo = const_cast<NDb::SMapInfo*>( pMapInfo );
						CMapInfoController::CVSOUndoDataList::const_iterator posVSOUndoData;
						if ( bRedo )
						{
							posVSOUndoData = pMapInfoController->vsoUndoDataList.begin();
						}
						else
						{
							posVSOUndoData = pMapInfoController->vsoUndoDataList.end();
						}
						while ( posVSOUndoData != ( bRedo ? pMapInfoController->vsoUndoDataList.end() :
																								pMapInfoController->vsoUndoDataList.begin() ) )
						{
							if ( !bRedo )
							{
								--posVSOUndoData;
							}
							//
							if ( bRedo )
							{
								switch( posVSOUndoData->eVSOType )
								{
									case CMapInfoController::SVSOUndoData::VSO_ROAD:
									{
										switch( posVSOUndoData->eType )
										{
											case CMapInfoController::SVSOUndoData::TYPE_INSERT: 
											{
												if ( VSOCollector.InsertVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->AddRoad( &( posVSOUndoData->newValue ) );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_REMOVE: 
											{
												if ( VSOCollector.RemoveVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue.nVSOID ) )
												{
													pTerraManager->RemoveRoad( posVSOUndoData->newValue.nVSOID );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_CHANGE: 
											{
												if ( VSOCollector.UpdateVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->UpdateRoad( posVSOUndoData->newValue.nVSOID );
												}
												break;
											}
											default:
												break;
										}
										break;
									}
									case CMapInfoController::SVSOUndoData::VSO_RIVER:
									{
										switch( posVSOUndoData->eType )
										{
											case CMapInfoController::SVSOUndoData::TYPE_INSERT: 
											{
												if ( VSOCollector.InsertVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->AddRiver( &( posVSOUndoData->newValue ) );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_REMOVE: 
											{
												if ( VSOCollector.RemoveVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue.nVSOID ) )
												{
													pTerraManager->RemoveRiver( posVSOUndoData->newValue.nVSOID );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_CHANGE: 
											{
												if ( VSOCollector.UpdateVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->UpdateRiver( posVSOUndoData->newValue.nVSOID );
												}
												break;
											}
											default:
												break;
										}
										break;
									}
									case CMapInfoController::SVSOUndoData::VSO_CRAG:
									{
										switch( posVSOUndoData->eType )
										{
											case CMapInfoController::SVSOUndoData::TYPE_INSERT: 
											{
												if ( VSOCollector.InsertVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->AddCrag( &( posVSOUndoData->newValue ) );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_REMOVE: 
											{
												if ( VSOCollector.RemoveVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue.nVSOID ) )
												{
													pTerraManager->RemoveCrag( posVSOUndoData->newValue.nVSOID );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_CHANGE: 
											{
												if ( VSOCollector.UpdateVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->UpdateCrag( posVSOUndoData->newValue.nVSOID );
												}
												break;
											}
											default:
												break;
										}
										break;
									}
									case CMapInfoController::SVSOUndoData::VSO_LAKE:
									case CMapInfoController::SVSOUndoData::VSO_COAST:
									{
										switch( posVSOUndoData->eType )
										{
											case CMapInfoController::SVSOUndoData::TYPE_INSERT: 
											{
												if ( VSOCollector.InsertVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->UpdateWater();
													pTerraManager->UpdateRiversDepthes();
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_REMOVE: 
											{
												if ( VSOCollector.RemoveVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue.nVSOID ) )
												{
													pTerraManager->UpdateWater();
													pTerraManager->UpdateRiversDepthes();
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_CHANGE: 
											{
												if ( VSOCollector.UpdateVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->UpdateWater();
													pTerraManager->UpdateRiversDepthes();
												}
												break;
											}
											default:
												break;
										}
										break;
									}
									default:
										break;
								}
							}
							else
							{
								switch( posVSOUndoData->eVSOType )
								{
									case CMapInfoController::SVSOUndoData::VSO_ROAD:
									{
										switch( posVSOUndoData->eType )
										{
											case CMapInfoController::SVSOUndoData::TYPE_INSERT: 
											{
												if ( VSOCollector.RemoveVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue.nVSOID ) )
												{
													pTerraManager->RemoveRoad( posVSOUndoData->newValue.nVSOID );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_REMOVE: 
											{
												if ( VSOCollector.InsertVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->AddRoad( &( posVSOUndoData->newValue ) );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_CHANGE: 
											{
												if ( VSOCollector.UpdateVSO( posVSOUndoData->eVSOType, posVSOUndoData->oldValue, pMapInfo ) )
												{
													pTerraManager->UpdateRoad( posVSOUndoData->oldValue.nVSOID );
												}
												break;
											}
											default:
												break;
										}
										break;
									}
									case CMapInfoController::SVSOUndoData::VSO_RIVER:
									{
										switch( posVSOUndoData->eType )
										{
											case CMapInfoController::SVSOUndoData::TYPE_INSERT: 
											{
												if ( VSOCollector.RemoveVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue.nVSOID ) )
												{
													pTerraManager->RemoveRiver( posVSOUndoData->newValue.nVSOID );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_REMOVE: 
											{
												if ( VSOCollector.InsertVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->AddRiver( &( posVSOUndoData->newValue ) );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_CHANGE: 
											{
												if ( VSOCollector.UpdateVSO( posVSOUndoData->eVSOType, posVSOUndoData->oldValue, pMapInfo ) )
												{
													pTerraManager->UpdateRiver( posVSOUndoData->oldValue.nVSOID );
												}
												break;
											}
											default:
												break;
										}
										break;
									}
									case CMapInfoController::SVSOUndoData::VSO_CRAG:
									{
										switch( posVSOUndoData->eType )
										{
											case CMapInfoController::SVSOUndoData::TYPE_INSERT: 
											{
												if ( VSOCollector.RemoveVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue.nVSOID ) )
												{
													pTerraManager->RemoveCrag( posVSOUndoData->newValue.nVSOID );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_REMOVE: 
											{
												if ( VSOCollector.InsertVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->AddCrag( &( posVSOUndoData->newValue ) );
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_CHANGE: 
											{
												if ( VSOCollector.UpdateVSO( posVSOUndoData->eVSOType, posVSOUndoData->oldValue, pMapInfo ) )
												{
													pTerraManager->UpdateCrag( posVSOUndoData->oldValue.nVSOID );
												}
												break;
											}
											default:
												break;
										}
										break;
									}
									case CMapInfoController::SVSOUndoData::VSO_LAKE:
									case CMapInfoController::SVSOUndoData::VSO_COAST:
									{
										switch( posVSOUndoData->eType )
										{
											case CMapInfoController::SVSOUndoData::TYPE_INSERT: 
											{
												if ( VSOCollector.RemoveVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue.nVSOID ) )
												{
													pTerraManager->UpdateWater();
													pTerraManager->UpdateRiversDepthes();
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_REMOVE: 
											{
												if ( VSOCollector.InsertVSO( posVSOUndoData->eVSOType, posVSOUndoData->newValue, pMapInfo ) )
												{
													pTerraManager->UpdateWater();
													pTerraManager->UpdateRiversDepthes();
												}
												break;
											}
											case CMapInfoController::SVSOUndoData::TYPE_CHANGE: 
											{
												if ( VSOCollector.UpdateVSO( posVSOUndoData->eVSOType, posVSOUndoData->oldValue, pMapInfo ) )
												{
													pTerraManager->UpdateWater();
													pTerraManager->UpdateRiversDepthes();
												}
												break;
											}
											default:
												break;
										}
										break;
									}
									default:
										break;
								}
							}
							//
							if ( bRedo )
							{
								++posVSOUndoData;
							}
						}
					}
				}
			}
		}
	}
	//
	if ( !pObjectController->undoDataList.empty() )
	{
		NHPTimer::STime time = 0;
		NHPTimer::GetTime( &time );

		GetViewManipulator()->ClearCache();
		IManipulator::CNameMap manipulatorNameMap;
		GetViewManipulator()->GetNameList( &manipulatorNameMap );
		//
		char pPostfix[0xFFF];
		std::string szPostfix;
		//
		const std::string szPlayersLabel				= "Players";
		const std::string szObjectsLabel				= "Objects";
		const std::string szBridgesLabel				= "Bridges";
		const std::string szEntrenchmentsLabel	= "Entrenchments";
		const std::string szDirectionLabel			= "Dir";
		const std::string szPositionLabel			= "Pos";
		const std::string szLinkLabel					= "Link";
		const std::string szPlayerLabel				= "Player";
		const std::string szFrameIndexLabel		= "FrameIndex";
		const std::string szSpotsLabel					= "Spots";
		const std::string szStartCommandLabel	= "startCommandsList";
		//
		bool bUpdatePlayerList = false;
		bool bUpdateStartCommandList = false;
		//
		NMapInfoEditor::CIndicesList objectList;
		NMapInfoEditor::CIndicesList bridgeList;
		NMapInfoEditor::CIndicesList entrenchmentList;
		NMapInfoEditor::CIndicesList removedObjectBackList;
		NMapInfoEditor::CControllerChangeInfoList changedObjectList;
		//
		NMapInfoEditor::CIndicesList spotList;
		NMapInfoEditor::CIndicesList removedSpotBackList;
		NMapInfoEditor::CControllerChangeInfoList changedSpotList;

		CObjectController::CUndoDataList::const_iterator posUndoData;
		if ( bRedo )
		{
			posUndoData = pObjectController->undoDataList.begin();
		}
		else
		{
			posUndoData = pObjectController->undoDataList.end();
		}
		while ( posUndoData != ( bRedo ? pObjectController->undoDataList.end() :
																		 pObjectController->undoDataList.begin() ) )
		{
			if ( !bRedo )
			{
				--posUndoData;
			}
			//
			IManipulator::CNameMap nameMap;
			pObjectController->GetNameListToUpdate( &nameMap, manipulatorNameMap, posUndoData->szName );
			//
			for ( IManipulator::CNameMap::const_iterator posName = nameMap.begin(); posName != nameMap.end(); ++posName )
			{
				std::string szName = posName->first;
				//
				//DebugTrace( "CMapInfoEditor::GetChangesFromController(): type: %d, name: <%s>", posUndoData->eType, szName.c_str() );
				if ( szName == szPlayersLabel )
				{
					if ( ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_INSERT ) ||
							 ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_REMOVE ) )
					{
						bUpdatePlayerList = true;
					}
				}
				else if ( szName == szStartCommandLabel )
				{
					bUpdateStartCommandList = true;
				}
				else if ( szName == szBridgesLabel )
				{
					if ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_INSERT )
					{
						int nBridgeIndex = (int)posUndoData->oldValue;
						if ( nBridgeIndex != INVALID_NODE_ID )
						{
							//DebugTrace( "CMapInfoEditor::GetChangesFromController(): bridge index: %d", nBridgeIndex );
							if ( bRedo )
							{
								InsertIndexToIndicesList<NMapInfoEditor::CIndicesList>( &bridgeList, nBridgeIndex );
							}
						}
					}
					else if ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_REMOVE )
					{
						int nBridgeIndex = (int)posUndoData->newValue;
						if ( nBridgeIndex != INVALID_NODE_ID )
						{
							//DebugTrace( "CMapInfoEditor::GetChangesFromController(): bridge index: %d", nBridgeIndex );
							if ( !bRedo )
							{
								InsertIndexToIndicesList<NMapInfoEditor::CIndicesList>( &bridgeList, nBridgeIndex );
							}
						}
					}
				}
				else if ( szName == szEntrenchmentsLabel )
				{
					if ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_INSERT )
					{
						int nEntrenchmentIndex = (int)posUndoData->oldValue;
						if ( nEntrenchmentIndex != INVALID_NODE_ID )
						{
							//DebugTrace( "CMapInfoEditor::GetChangesFromController(): index: %d", nEntrenchmentIndex );
							if ( bRedo )
							{
								InsertIndexToIndicesList<NMapInfoEditor::CIndicesList>( &entrenchmentList, nEntrenchmentIndex );
							}
						}
					}
					else if ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_REMOVE )
					{
						int nEntrenchmentIndex = (int)posUndoData->newValue;
						if ( nEntrenchmentIndex != INVALID_NODE_ID )
						{
							//DebugTrace( "CMapInfoEditor::GetChangesFromController(): object index: %d", nEntrenchmentIndex );
							if ( !bRedo )
							{
								InsertIndexToIndicesList<NMapInfoEditor::CIndicesList>( &entrenchmentList, nEntrenchmentIndex );
							}
						}
					}
				}
				else if ( szName == szObjectsLabel )
				{
					if ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_INSERT )
					{
						int nObjectIndex = (int)posUndoData->oldValue;
						if ( nObjectIndex != INVALID_NODE_ID )
						{
							//DebugTrace( "CMapInfoEditor::GetChangesFromController(): object index: %d", nObjectIndex );
							if ( bRedo )
							{
								InsertIndexToIndicesList<NMapInfoEditor::CIndicesList>( &objectList, nObjectIndex );
								InsertIndexToIndicesList<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::SControllerChangeInfo>( &changedObjectList, nObjectIndex );
							}
							else
							{
								removedObjectBackList.push_front( nObjectIndex );
								RemoveIndexToIndicesList<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::SControllerChangeInfo>( &changedObjectList, nObjectIndex );
							}
						}
					}
					else if ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_REMOVE )
					{
						int nObjectIndex = (int)posUndoData->newValue;
						if ( nObjectIndex != INVALID_NODE_ID )
						{
							//DebugTrace( "CMapInfoEditor::GetChangesFromController(): object index: %d", nObjectIndex );
							if ( bRedo )
							{
								removedObjectBackList.push_front( nObjectIndex );
								RemoveIndexToIndicesList<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::SControllerChangeInfo>( &changedObjectList, nObjectIndex );
							}
							else
							{
								InsertIndexToIndicesList<NMapInfoEditor::CIndicesList>( &objectList, nObjectIndex );
								InsertIndexToIndicesList<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::SControllerChangeInfo>( &changedObjectList, nObjectIndex );
							}
						}
					}
				}
				else if ( szName.compare( 0, szObjectsLabel.size(), szObjectsLabel ) == 0 )
				{
					if ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_CHANGE )
					{
						pPostfix[0] = 0;
						int nObjectIndex = INVALID_NODE_ID;
						if ( ( sscanf( szName.c_str(), "Objects.[%d].%s", &nObjectIndex, &pPostfix ) == 2 ) && ( nObjectIndex != INVALID_NODE_ID ) )
						{
							szPostfix = pPostfix;
							NMapInfoEditor::CControllerChangeInfoList::iterator posControllerChangeInfo = FindIndex<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::CControllerChangeInfoList::iterator>( changedObjectList, nObjectIndex );
							if ( posControllerChangeInfo == changedObjectList.end() )
							{
								posControllerChangeInfo = changedObjectList.insert( changedObjectList.end(), NMapInfoEditor::SControllerChangeInfo( nObjectIndex ) );
							}
							if ( posControllerChangeInfo != changedObjectList.end() )
							{
								if ( szPostfix.compare( 0, szPositionLabel.size(), szPositionLabel ) == 0 )
								{
									posControllerChangeInfo->nFlags |= POSITION_CHANGED;
								}
								else if ( szPostfix.compare( 0, szDirectionLabel.size(), szDirectionLabel ) == 0 )
								{
									posControllerChangeInfo->nFlags |= DIRECTION_CHANGED;
								}
								else if ( szPostfix.compare( 0, szLinkLabel.size(), szLinkLabel ) == 0 )
								{
									posControllerChangeInfo->nFlags |= LINK_CHANGED;
								}
								else if ( szPostfix.compare( 0, szPlayerLabel.size(), szPlayerLabel ) == 0 )
								{
									posControllerChangeInfo->nFlags |= PLAYER_CHANGED;
								}
								else if ( szPostfix.compare( 0, szFrameIndexLabel.size(), szFrameIndexLabel ) == 0 )
								{
									posControllerChangeInfo->nFlags |= FRAME_INDEX_CHANGED;
								}
							}
						}
					}
				}		
				else if ( szName == szSpotsLabel )
				{
					if ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_INSERT )
					{
						int nSpotIndex = (int)posUndoData->oldValue;
						if ( nSpotIndex != INVALID_NODE_ID )
						{
							//DebugTrace( "CMapInfoEditor::GetChangesFromController(): spot index: %d", nSpotIndex );
							if ( bRedo )
							{
								InsertIndexToIndicesList<NMapInfoEditor::CIndicesList>( &spotList, nSpotIndex );
								InsertIndexToIndicesList<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::SControllerChangeInfo>( &changedSpotList, nSpotIndex );
							}
							else
							{
								removedSpotBackList.push_front( nSpotIndex );
								RemoveIndexToIndicesList<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::SControllerChangeInfo>( &changedSpotList, nSpotIndex );
							}
						}
					}
					else if ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_REMOVE )
					{
						int nSpotIndex = (int)posUndoData->newValue;
						if ( nSpotIndex != INVALID_NODE_ID )
						{
							//DebugTrace( "CMapInfoEditor::GetChangesFromController(): spot index: %d", nSpotIndex );
							if ( bRedo )
							{
								removedSpotBackList.push_front( nSpotIndex );
								RemoveIndexToIndicesList<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::SControllerChangeInfo>( &changedSpotList, nSpotIndex );
							}
							else
							{
								InsertIndexToIndicesList<NMapInfoEditor::CIndicesList>( &spotList, nSpotIndex );
								InsertIndexToIndicesList<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::SControllerChangeInfo>( &changedSpotList, nSpotIndex );
							}
						}
					}
				}
				else if ( szName.compare( 0, szSpotsLabel.size(), szSpotsLabel ) == 0 )
				{
					if ( posUndoData->eType == CObjectBaseController::SUndoData::TYPE_CHANGE )
					{
						pPostfix[0] = 0;
						int nSpotIndex = INVALID_NODE_ID;
						if ( ( sscanf( szName.c_str(), "Spots.[%d].%s", &nSpotIndex, &pPostfix ) == 2 ) && ( nSpotIndex != INVALID_NODE_ID ) )
						{
							szPostfix = pPostfix;
							NMapInfoEditor::CControllerChangeInfoList::iterator posControllerChangeInfo = FindIndex<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::CControllerChangeInfoList::iterator>( changedSpotList, nSpotIndex );
							if ( posControllerChangeInfo == changedSpotList.end() )
							{
								posControllerChangeInfo = changedSpotList.insert( changedSpotList.end(), NMapInfoEditor::SControllerChangeInfo( nSpotIndex ) );
							}
							if ( posControllerChangeInfo != changedSpotList.end() )
							{
								posControllerChangeInfo->nFlags |= POSITION_CHANGED;
							}
						}
					}
				}		
			}
			//
			if ( bRedo )
			{
				++posUndoData;
			}
		}
		if ( bUpdatePlayerList )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE, ID_UPDATE_EDIT_PARAMETERS, MIMOSEP_PLAYER_COUNT );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_REINF_POINTS_STATE, ID_UPDATE_EDIT_PARAMETERS, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_CAMERA_POSITION_STATE, ID_UPDATE_EDIT_PARAMETERS, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_AIGEN_POINTS_STATE, ID_UPDATE_EDIT_PARAMETERS, 0 );
		}
		if ( !removedObjectBackList.empty() && !objectList.empty() )
		{
			NI_ASSERT( 0, "CMapInfoEditor::GetChangesFromController(): cant Undo / Redo with two types: Remove and Insert" );
		}
		//
		IEditorScene *pEditorScene = EditorScene();
		IManipulator *pManipulator = GetViewManipulator();
		//
		if ( !objectList.empty() )
		{
			// Добавляем объекты
			std::unordered_map<int, int> objectIndexToLinkIDMap;
			objectList.sort();
			for ( NMapInfoEditor::CIndicesList::const_iterator itIndex = objectList.begin(); itIndex != objectList.end(); ++itIndex )
			{
				NMapInfoEditor::CControllerChangeInfoList::iterator posControllerChangeInfo = FindIndex<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::CControllerChangeInfoList::iterator>( changedObjectList, *itIndex );
				if ( posControllerChangeInfo != changedObjectList.end() )
				{
					changedObjectList.erase( posControllerChangeInfo );
				}
				//const std::string szObjectPrefix = StrFmt( "Objects.[%d]", *itIndex );
				const int nLinkID = objectInfoCollector.GetLinkIDByObjectIndex( *itIndex, pManipulator, true );
				objectInfoCollector.linkIDCollector.LockID( nLinkID );
				objectInfoCollector.linkIDToIndexCollector.Insert( nLinkID, *itIndex, true );
				objectIndexToLinkIDMap[*itIndex] = nLinkID;
			}
			//
			for ( NMapInfoEditor::CIndicesList::const_iterator itIndex = objectList.begin(); itIndex != objectList.end(); ++itIndex )
			{
				std::unordered_map<int, int>::const_iterator posObjectIndexToLinkID = objectIndexToLinkIDMap.find( *itIndex );
				if ( posObjectIndexToLinkID == objectIndexToLinkIDMap.end() )
				{
					NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Map object %d on the map \"%s\" has no objectID. Skiping...\n", *itIndex, NDb::GetResName(pMapInfo) ) );
					continue;				
				}
				const std::string szObjectPrefix = StrFmt( "Objects.[%d]", *itIndex );
				std::string szRPGStatsTypeName;
				std::string szRPGStatsName;
				CManipulatorManager::GetParamsFromReference( szObjectPrefix + ".Object", pManipulator, &szRPGStatsTypeName, &szRPGStatsName, 0 );
				if ( szRPGStatsTypeName.empty() || szRPGStatsName.empty() )
				{
					NI_ASSERT( 0, StrFmt( "Empty object %d", *itIndex ) );
					continue;
				}
				// создаем записи в mapInfo в зависимости от типа объекта
				if ( ( szRPGStatsTypeName == "MineRPGStats" )					||
						 ( szRPGStatsTypeName == "BuildingRPGStats" )			||
						 ( szRPGStatsTypeName == "MechUnitRPGStats" )			||
						 ( szRPGStatsTypeName == "ObjectRPGStats" )				||
						 ( szRPGStatsTypeName == "TerraObjSetRPGStats" )	||
						 ( szRPGStatsTypeName == "FenceRPGStats" )				||
						 ( szRPGStatsTypeName == "SquadRPGStats" ) )
				{
					//DebugTrace( "CMapInfoEditor::GetChangesFromController(): insert object: %d", *itIndex );
					//
					NMapInfoEditor::SObjectLoadInfo objectLoadInfo;
					objectLoadInfo.nObjectIndex = *itIndex;
					objectLoadInfo.nLinkID = posObjectIndexToLinkID->second;
					objectLoadInfo.bSearchIndices = true;
					objectLoadInfo.bAdditionalDataFilled = true;
					objectLoadInfo.szRPGStatsTypeName = szRPGStatsTypeName;
					objectLoadInfo.rpgStatsDBID = CDBID( szRPGStatsName );
					CManipulatorManager::GetValue( &( objectLoadInfo.nPlayer ), pManipulator, szObjectPrefix + ".Player" );
					CManipulatorManager::GetValue( &( objectLoadInfo.nFrameIndex ), pManipulator, szObjectPrefix + ".FrameIndex" );
					CManipulatorManager::GetValue( &( objectLoadInfo.fHP ), pManipulator, szObjectPrefix + ".HP" );
					CManipulatorManager::GetVec3<CVec3, float>( &( objectLoadInfo.vPosition ), pManipulator, szObjectPrefix + ".Pos" );
					uint16_t wDirection = 0;
					CManipulatorManager::GetValue( &wDirection, pManipulator, szObjectPrefix + ".Dir" );
					objectLoadInfo.fDirection = AI2VisRad( wDirection );
					CManipulatorManager::GetValue( &( objectLoadInfo.nLinkWith ), pManipulator, szObjectPrefix + ".Link.LinkWith" );
					unsigned nSimpleObjectInfoID = INVALID_NODE_ID;
					if ( NMapInfoEditor::SSimpleObjectInfo *pSimpleObjectInfo = objectInfoCollector.Insert( static_cast<NMapInfoEditor::SSimpleObjectInfo*>( 0 ), &nSimpleObjectInfoID ) )
					{
						pSimpleObjectInfo->Load( &objectLoadInfo, pEditorScene, pManipulator );
					}
				}
			}
			// мосты
			bridgeList.sort();
			for ( NMapInfoEditor::CIndicesList::const_iterator itIndex = bridgeList.begin(); itIndex != bridgeList.end(); ++itIndex )
			{
				const int nBridgeIndex = *itIndex;
				//DebugTrace( "CMapInfoEditor::GetChangesFromController(): insert bridge: %d", nBridgeIndex );
				//				
				NMapInfoEditor::SObjectLoadInfo objectLoadInfo;
				objectLoadInfo.nObjectIndex = nBridgeIndex;
				objectLoadInfo.bSearchIndices = true;
				unsigned nBridgeInfoID = INVALID_NODE_ID;
				if ( NMapInfoEditor::SBridgeInfo *pBridgeInfo = objectInfoCollector.Insert( static_cast<NMapInfoEditor::SBridgeInfo*>( 0 ), &nBridgeInfoID ) )
				{
					pBridgeInfo->Load( &objectLoadInfo, pEditorScene, pManipulator );
				}
			}
			// окопы
			entrenchmentList.sort();
			for ( NMapInfoEditor::CIndicesList::const_iterator itIndex = entrenchmentList.begin(); itIndex != entrenchmentList.end(); ++itIndex )
			{
				const int nEntrenchmentIndex = *itIndex;
				//DebugTrace( "CMapInfoEditor::GetChangesFromController(): insert entrenchment: %d", nEntrenchmentIndex );
				//				
				NMapInfoEditor::SObjectLoadInfo objectLoadInfo;
				objectLoadInfo.nObjectIndex = nEntrenchmentIndex;
				objectLoadInfo.bSearchIndices = true;
				unsigned nEntrenchmentInfoID = INVALID_NODE_ID;
				if ( NMapInfoEditor::SEntrenchmentInfo *pEntrenchmentInfo = objectInfoCollector.Insert( static_cast<NMapInfoEditor::SEntrenchmentInfo*>( 0 ), &nEntrenchmentInfoID ) )
				{
					pEntrenchmentInfo->Load( &objectLoadInfo, pEditorScene, pManipulator );
				}
			}
			objectInfoCollector.PostLoadByController( objectList, pEditorScene, pManipulator, true );
		}
		else if ( !removedObjectBackList.empty() )
		{
			for ( NMapInfoEditor::CIndicesList::const_iterator itIndex = removedObjectBackList.begin(); itIndex != removedObjectBackList.end(); ++itIndex )
			{
				InsertIndexToIndicesList<NMapInfoEditor::CIndicesList>( &objectList, *itIndex );
			}
			removedObjectBackList.clear();
			std::list<int> nObjectIDToRemoveList;
			for ( NMapInfoEditor::CIndicesList::iterator itIndex = objectList.begin(); itIndex != objectList.end(); ++itIndex )
			{
				const int nLinkID = objectInfoCollector.GetLinkIDByObjectIndex( *itIndex, 0, true );
				if ( nLinkID != INVALID_NODE_ID )
				{
					nObjectIDToRemoveList.push_back( nLinkID );
				}
			}
			// Удаляем объекты
			for ( std::list<int>::iterator itObjectNameToRemove = nObjectIDToRemoveList.begin(); itObjectNameToRemove != nObjectIDToRemoveList.end(); ++itObjectNameToRemove )
			{
				if ( const NMapInfoEditor::SObjectInfo* pObjectInfo = objectInfoCollector.GetObjectInfoByLinkID( *itObjectNameToRemove ) )
				{
					objectInfoCollector.Remove(	pObjectInfo->nObjectInfoID, true, pEditorScene, false, 0, 0 );
				}
			}
		}
		// обновляем объекты
		if ( !changedObjectList.empty() )
		{
			for( NMapInfoEditor::CControllerChangeInfoList::const_iterator itControllerChangeInfo = changedObjectList.begin();
					 itControllerChangeInfo != changedObjectList.end();
					 ++itControllerChangeInfo )
			{
				//DebugTrace( "CMapInfoEditor::GetChangesFromController(): update Object: %d", itControllerChangeInfo->nIndex );
				objectInfoCollector.UpdateObjectByController( itControllerChangeInfo->nIndex, itControllerChangeInfo->nFlags, pEditorScene, pManipulator );
			}
		}
		// Spots
		if ( !spotList.empty() )
		{
			//NDb::SMapInfo *pMutablMapInfo = const_cast<NDb::SMapInfo*>( pMapInfo );
			// Добавляем spots
			// Добавляем объекты
			std::unordered_map<int, int> spotIndexToLinkIDMap;
			spotList.sort();
			for ( NMapInfoEditor::CIndicesList::const_iterator itIndex = spotList.begin(); itIndex != spotList.end(); ++itIndex )
			{
				NMapInfoEditor::CControllerChangeInfoList::iterator posControllerChangeInfo = FindIndex<NMapInfoEditor::CControllerChangeInfoList, NMapInfoEditor::CControllerChangeInfoList::iterator>( changedSpotList, *itIndex );
				if ( posControllerChangeInfo != changedSpotList.end() )
				{
					changedSpotList.erase( posControllerChangeInfo );
				}
				//const std::string szObjectPrefix = StrFmt( "Objects.[%d]", *itIndex );
				const int nLinkID = objectInfoCollector.GetLinkIDByObjectIndex( *itIndex, pManipulator, false );
				objectInfoCollector.linkIDCollector.LockID( nLinkID );
				objectInfoCollector.spotIDToIndexCollector.Insert( nLinkID, *itIndex, true );
				spotIndexToLinkIDMap[*itIndex] = nLinkID;
			}
			//
			for ( NMapInfoEditor::CIndicesList::const_iterator itIndex = spotList.begin(); itIndex != spotList.end(); ++itIndex )
			{
				std::unordered_map<int, int>::const_iterator posSpotIndexToLinkID = spotIndexToLinkIDMap.find( *itIndex );
				if ( posSpotIndexToLinkID == spotIndexToLinkIDMap.end() )
				{
					NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Map spot %d on the map \"%s\" has no objectID. Skiping...\n", *itIndex, NDb::GetResName(pMapInfo) ) );
					continue;				
				}
				const std::string szSpotPrefix = StrFmt( "Spots.[%d]", *itIndex );
				std::string szRPGStatsTypeName;
				std::string szRPGStatsName;
				CManipulatorManager::GetParamsFromReference( szSpotPrefix + ".Descriptor", pManipulator, &szRPGStatsTypeName, &szRPGStatsName, 0 );
				if ( szRPGStatsTypeName.empty() || szRPGStatsName.empty() )
				{
					NI_ASSERT( 0, StrFmt( "Empty Spot %d", *itIndex ) );
					continue;
				}
				pManipulator->SetValue( szSpotPrefix + ".points", 4 );
				NMapInfoEditor::CSpotSquare spotSquare;
				CManipulatorManager::GetArray<NMapInfoEditor::CSpotSquare, CVec2>( &spotSquare, pManipulator, szSpotPrefix + ".points" );
				//
				NDb::STerrainSpotInstance terrainSpotInstance;
				terrainSpotInstance.pDescriptor = dynamic_cast<const NDb::STerrainSpotDesc*>( NDb::GetObject( CDBID( szRPGStatsName ) ) );
				terrainSpotInstance.nSpotID = posSpotIndexToLinkID->second;
				terrainSpotInstance.points = spotSquare;
				//std::vector<NDb::STerrainSpotInstance>::iterator itSpot = pMutablMapInfo->spots.insert( pMutablMapInfo->spots.begin() + *itIndex, terrainSpotInfoInstance );
				pEditorScene->GetTerraManager()->AddTerraSpot( &terrainSpotInstance );
				//
				NMapInfoEditor::SSpotLoadInfo spotLoadInfo;
				spotLoadInfo.nObjectIndex = *itIndex;
				spotLoadInfo.nLinkID = posSpotIndexToLinkID->second;
				spotLoadInfo.bAdditionalDataFilled = true;
				spotLoadInfo.szRPGStatsTypeName = szRPGStatsTypeName;
				spotLoadInfo.rpgStatsDBID = CDBID( szRPGStatsName );
				spotLoadInfo.spotSquare = spotSquare;
				unsigned nSpotInfoID = INVALID_NODE_ID;
				if ( NMapInfoEditor::SSpotInfo *pSpotInfo = objectInfoCollector.Insert( static_cast<NMapInfoEditor::SSpotInfo*>( 0 ), &nSpotInfoID ) )
				{
					pSpotInfo->Load( &spotLoadInfo, pEditorScene, pManipulator );
				}
			}
		}
		else if ( !removedSpotBackList.empty() )
		{
			for ( NMapInfoEditor::CIndicesList::const_iterator itIndex = removedSpotBackList.begin(); itIndex != removedSpotBackList.end(); ++itIndex )
			{
				InsertIndexToIndicesList<NMapInfoEditor::CIndicesList>( &spotList, *itIndex );
			}
			removedSpotBackList.clear();
			// Удаляем spots
			for ( NMapInfoEditor::CIndicesList::const_iterator itIndex = spotList.begin(); itIndex != spotList.end(); ++itIndex )
			{
				const int nLinkID = objectInfoCollector.GetLinkIDByObjectIndex( *itIndex, 0, false );
				if ( const NMapInfoEditor::SObjectInfo* pObjectInfo = objectInfoCollector.GetObjectInfoByLinkID( nLinkID ) )
				{
					objectInfoCollector.Remove(	pObjectInfo->nObjectInfoID, true, pEditorScene, false, 0, 0 );
				}
			}
		}
		// обновляем spots
		if ( !changedSpotList.empty() )
		{
			for( NMapInfoEditor::CControllerChangeInfoList::const_iterator itControllerChangeInfo = changedSpotList.begin();
					 itControllerChangeInfo != changedSpotList.end();
					 ++itControllerChangeInfo )
			{
				//DebugTrace( "CMapInfoEditor::GetChangesFromController(): update Spot: %d", itControllerChangeInfo->nIndex );
				objectInfoCollector.UpdateSpotByController( itControllerChangeInfo->nIndex, itControllerChangeInfo->nFlags, pEditorScene, pManipulator );
			}
		}
		//
		objectInfoCollector.UpdateSelectionByController();
		if ( bUpdateStartCommandList )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_UNIT_START_CMD_STATE, ID_UNIT_START_CMD_REFRESH_WINDOW, 0 );
		}
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CMapInfoEditor::Undo( IController* pController )
{
	if ( CObjectBaseController *pObjectController = dynamic_cast<CObjectBaseController*>( pController ) )
	{
		GetChangesFromController( pObjectController, false ); 
	}
}


void CMapInfoEditor::Redo( IController* pController )
{
	if ( CObjectBaseController *pObjectController = dynamic_cast<CObjectBaseController*>( pController ) )
	{
		GetChangesFromController( pObjectController, true ); 
	}
}

void CMapInfoEditor::ConfigureViewFilter()
{
	CMapInfoViewFilterDlg dlg( &editorSettings );
	if ( dlg.DoModal() == IDOK )
		ApplyViewFilter();
}

void CMapInfoEditor::ApplyViewFilter()
{
	CPtr<IEditorScene> pEditorScene = EditorScene();
	if ( !pEditorScene )
		return;

	// grid
	if ( editorSettings.viewFilterData.bShowGrid )
	{
		if ( editorSettings.viewFilterData.szGridSize == RCSTR("Visual tile") )
		{
			if ( pEditorScene->IsShowOn( SCENE_SHOW_AI_GRID ) )
			{
				pEditorScene->ToggleShow( SCENE_SHOW_AI_GRID );
			}
			if ( !pEditorScene->IsShowOn( SCENE_SHOW_GRID ) )
			{
				pEditorScene->ToggleShow( SCENE_SHOW_GRID );
			}
		}
		else if ( editorSettings.viewFilterData.szGridSize == RCSTR("AI tile") )
		{
			if ( pEditorScene->IsShowOn( SCENE_SHOW_GRID ) )
			{
				pEditorScene->ToggleShow( SCENE_SHOW_GRID );
			}
			if ( !pEditorScene->IsShowOn( SCENE_SHOW_AI_GRID ) )
			{
				pEditorScene->ToggleShow( SCENE_SHOW_AI_GRID );
			}
		}
	}
	else
	{
		if ( pEditorScene->IsShowOn( SCENE_SHOW_GRID ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_GRID );
		}
		if ( pEditorScene->IsShowOn( SCENE_SHOW_AI_GRID ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_AI_GRID );
		}
	}

	// wireframe
	if ( editorSettings.viewFilterData.bWireFrame )
	{
		if ( !pEditorScene->IsShowOn( SCENE_SHOW_WIREFRAME ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_WIREFRAME );
		}
	}
	else
	{
		if ( pEditorScene->IsShowOn( SCENE_SHOW_WIREFRAME ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_WIREFRAME );
		}
	}

	// bounding boxes
	if ( editorSettings.viewFilterData.bShowBBoxes )
	{
		if ( !pEditorScene->IsShowOn( SCENE_SHOW_BBOXES ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_BBOXES );
		}
	}
	else
	{
		if ( pEditorScene->IsShowOn( SCENE_SHOW_BBOXES ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_BBOXES );
		}
	}

	// terrain
	if ( editorSettings.viewFilterData.bShowTerrain )
	{
		if ( pEditorScene->IsShowOn( SCENE_SHOW_TERRAIN ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_TERRAIN );
		}
	}
	else
	{
		if ( !pEditorScene->IsShowOn( SCENE_SHOW_TERRAIN ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_TERRAIN );
		}
	}

	// shadows
	if ( editorSettings.viewFilterData.bShowShadows )
	{
		if ( pEditorScene->IsShowOn( SCENE_SHOW_SHADOWS ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_SHADOWS );
		}
	}
	else
	{
		if ( !pEditorScene->IsShowOn( SCENE_SHOW_SHADOWS ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_SHADOWS );
		}
	}

	// stats
	if ( editorSettings.viewFilterData.bShowStats )
	{
		if ( !pEditorScene->IsShowOn( SCENE_SHOW_STATISTICS ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_STATISTICS );
		}
	}
	else
	{
		if ( pEditorScene->IsShowOn( SCENE_SHOW_STATISTICS ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_STATISTICS );
		}
	}

	// warfog
	if ( editorSettings.viewFilterData.bShowWarfog )
	{
		// заполним бордюр
		int nDensity = 0;
		const int nBorderWidth = 4;
		const int nWarfogMinValue = 255;
		//
		const int nMapSizeX = pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH;
		const int nMapSizeY = pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH;
		const int nMaxMapSize = (std::max)( nMapSizeX, nMapSizeY );
		const int nWarFogSize = GetNextPow2( nMaxMapSize );
		const float fWarFogCellSize = AI_TILES_IN_VIS_TILE * VIS_TILES_IN_PATCH * VIS_TILE_SIZE / AI_TILES_IN_PATCH;

		CArray2D<uint8_t> warFogInfo;
		warFogInfo.SetSizes( nWarFogSize + 1, nWarFogSize + 1 );
		warFogInfo.FillEvery( nWarfogMinValue );

		for ( int i = 0; i < nBorderWidth; ++i )
		{
			nDensity = nWarfogMinValue * i / ( nBorderWidth - 1 );
			for ( int x = i; x <= nMapSizeX - i; ++x )
			{
				warFogInfo[i][x] = nDensity;
				warFogInfo[nMapSizeY - i][x] = nDensity;
			}
			for ( int y = i; y <= nMapSizeY - i; ++y )
			{
				warFogInfo[y][i] = nDensity;
				warFogInfo[y][nMapSizeX - i] = nDensity;
			}
		}
		pEditorScene->SetWarFog( warFogInfo, 1.0f / fWarFogCellSize );
	}
	else
	{
		CArray2D<uint8_t> warFog( 1, 1 );
		warFog.FillEvery( 255 );
		pEditorScene->SetWarFog( warFog, 1.0f );
	}

	// mipmaps
	if ( editorSettings.viewFilterData.bMipmap )
	{
		if ( !pEditorScene->IsShowOn( SCENE_SHOW_MIPMAPS ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_MIPMAPS );
		}
	}
	else
	{
		if ( pEditorScene->IsShowOn( SCENE_SHOW_MIPMAPS ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_MIPMAPS );
		}
	}

	// overdraw
	if ( editorSettings.viewFilterData.bOverdraw )
	{
		if ( !pEditorScene->IsShowOn( SCENE_SHOW_OVERDRAW ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_OVERDRAW );
		}
	}
	else
	{
		if ( pEditorScene->IsShowOn( SCENE_SHOW_OVERDRAW ) )
		{
			pEditorScene->ToggleShow( SCENE_SHOW_OVERDRAW );
		}
	}

	// object types
	if ( !pMapInfo )
		return;
	CPtr<IResourceManager> pResourceManager = Singleton<IResourceManager>();
	if ( !pResourceManager )
		return;
	CPtr<IManipulator> pTableManipulator = pResourceManager->CreateTableManipulator();
	if ( !pTableManipulator )
		return;

	// получить idшники типов объектов, которые должны быть скрыты фильтром
	std::unordered_map<std::string,bool> filterSettings;
	for ( int t = 0; t < editorSettings.viewFilterData.objTypeFilter.size(); ++t )
	{
		std::string szTypeName = editorSettings.viewFilterData.objTypeFilter[t].szObjTypeName;
		if ( szTypeName.empty()  )
		{
			continue;
		}
		filterSettings[szTypeName] = editorSettings.viewFilterData.objTypeFilter[t].bShow;
	}

	for ( NMapInfoEditor::SObjectInfoCollector::CObjectInfoMap::iterator itObjectInfo = objectInfoCollector.objectInfoMap.begin(); 
		itObjectInfo != objectInfoCollector.objectInfoMap.end(); ++itObjectInfo )
	{
		// по объектам
		NMapInfoEditor::SObjectInfo *pOI = itObjectInfo ->second;
		if ( !pOI )
			continue;

		for ( NMapInfoEditor::SObjectInfo::CSceneIDToLinkIDMap::iterator 
					itSceneIDToElemID = pOI->sceneIDToLinkIDMap.begin();
					itSceneIDToElemID != pOI->sceneIDToLinkIDMap.end(); ++itSceneIDToElemID )
		{
			// по элементам объекта
			int nSceneID = itSceneIDToElemID->first;
			int nElemID = itSceneIDToElemID->second;
			NMapInfoEditor::SObjectInfo::CMapInfoElementMap::iterator itElement = pOI->mapInfoElementMap.find( nElemID );
			if ( itElement == pOI->mapInfoElementMap.end() )
				continue;
			NMapInfoEditor::SObjectInfo::SMapInfoElement *pElement = &(itElement->second);
			//
			const std::string szElementTypeName = pElement->szRPGStatsTypeName;
			std::unordered_map<std::string,bool>::iterator itFilter = filterSettings.find( szElementTypeName );
			if ( itFilter == filterSettings.end() || itFilter->second == true )
				pEditorScene->ShowObject( nSceneID, true );
			else
				pEditorScene->ShowObject( nSceneID, false );
		}
	}
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CMapInfoEditor::RunGame()
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true );
	/**
	const std::string szStartFolder = Singleton<IUserDataContainer>()->Get()->szStartFolder;
	//
	SConfigFile configFile;	
	configFile.Load( szStartFolder + GAME_CFG_FILE_PATH, STREAM_PATH_ABSOLUTE );
	configFile.RemoveKeyword( "main_menu", true );
	configFile.RemoveKeyword( "map", true );
	configFile.AddKeyword( "map", std::to_string(  GetObjectSet().objectNameSet.begin()->first ) );
	configFile.Save( szStartFolder + GAME_CFG_NEW_FILE_PATH, STREAM_PATH_ABSOLUTE );
	//
	const std::string szRunGameBatFilePath = szStartFolder + RUN_GAME_BAT_FILE_PATH;
	::ShellExecute( 0, "open", szRunGameBatFilePath.c_str(), NULL, NULL, SW_SHOWNORMAL );
	/**/
	/**
//	ClearHoldQueue();
	// launch map
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_UPDATE, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_INPUT, 0 );

	pEditorScene = EditorScene();
	NSingleton::UnRegisterSingleton( IEditorScene::tidTypeID );
	NSingleton::RegisterSingleton( CreateScene(), IEditorScene::tidTypeID );
	EditorScene()->SwitchScene( SCENE_MISSION );
	CreateAI();
	// re-init DB-dependent consts
	if ( const NDb::SUIConstsB2 *pUIConsts = NGameX::GetUIConsts() )
		Singleton<IUIInitialization>()->SetUIConsts( pUIConsts );
	if ( const NDb::SClientGameConsts *pClientConsts = NGameX::GetClientConsts() )
		Singleton<IClientAckManager>()->SetClientConsts( pClientConsts );
	if ( const NDb::SSceneConsts *pEditorSceneConsts = NGameX::GetSceneConsts() )
		EditorScene()->SetSceneConsts( pEditorSceneConsts );
	Camera()->Init();
	//
	const std::wstring wszCommand = NStr::ToUnicode( StrFmt( "map %d", GetObjectSet().objectNameSet.begin()->first ) );
	NGlobal::ProcessCommand( wszCommand );
	/**/
}


// basement storage  

		/**
		// удаляемые объекты убираем из добавляемых
		for ( std::unordered_map<unsigned,int>::const_iterator itObjectIndex = removedObjectMap.begin(); itObjectIndex != removedObjectMap.end(); ++itObjectIndex )
		{
			{
				std::unordered_map<unsigned,int>::iterator posObjectIndex = insertedObjectMap.find( itObjectIndex->first );
				if ( posObjectIndex != insertedObjectMap.end() )
				{
					insertedObjectMap.erase( posObjectIndex );
				}
			}
			{
				std::unordered_map<unsigned,unsigned>::iterator posObjectIndex = changedObjectMap.find( itObjectIndex->first );
				if ( posObjectIndex != changedObjectMap.end() )
				{
					changedObjectMap.erase( posObjectIndex );
				}
			}
		}
		// добавляемые и убираемые объекты убираем из обновляемых
		for ( std::unordered_map<unsigned,int>::const_iterator itObjectIndex = insertedObjectMap.begin(); itObjectIndex != insertedObjectMap.end(); ++itObjectIndex )
		{
			std::unordered_map<unsigned,unsigned>::iterator posObjectIndex = changedObjectMap.find( itObjectIndex->first );
			if ( posObjectIndex != changedObjectMap.end() )
			{
				changedObjectMap.erase( posObjectIndex );
			}
		}
		for ( std::unordered_map<unsigned,int>::const_iterator itObjectIndex = removedObjectMap.begin(); itObjectIndex != removedObjectMap.end(); ++itObjectIndex )
		{
			std::unordered_map<unsigned,unsigned>::iterator posObjectIndex = changedObjectMap.find( itObjectIndex->first );
			if ( posObjectIndex != changedObjectMap.end() )
			{
				changedObjectMap.erase( posObjectIndex );
			}
		}
		// добавляем объекты
		for( std::unordered_map<unsigned,int>::const_iterator itObjectIndex = insertedObjectMap.begin(); itObjectIndex != insertedObjectMap.end(); ++itObjectIndex )
		{
			objectInfoCollector.InsertObjectByController( itObjectIndex->first, EditorScene(), GetViewManipulator(), EditorScene()->GetHeights() );
		}
		// удаляем объекты
		for( std::unordered_map<unsigned,int>::const_iterator itObjectIndex = removedObjectMap.begin(); itObjectIndex != removedObjectMap.end(); ++itObjectIndex )
		{
			objectInfoCollector.RemoveObjectByController( itObjectIndex->first, EditorScene(), GetViewManipulator(), EditorScene()->GetHeights() );
		}
		// добавляем мосты
		for( std::unordered_map<unsigned,int>::const_iterator itBridgeIndex = insertedBridgeMap.begin(); itBridgeIndex != insertedBridgeMap.end(); ++itBridgeIndex )
		{
		}
		// удаляем мосты
		for( std::unordered_map<unsigned,int>::const_iterator itBridgeIndex = removedBridgeMap.begin(); itBridgeIndex != removedBridgeMap.end(); ++itBridgeIndex )
		{
		}
		/**/

