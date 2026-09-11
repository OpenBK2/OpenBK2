#include "stdafx.h"
#include "MapEditorLib/MfcWidget.h"
#include <fmt/format.h>

#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "Misc/2Darray.h"
#include "SeasonMnemonics.h"
#include "MapEditorLib/DefaultTabWindow.h"
#include "PointListView.h"
// This module's ids and strings; they came in through PointListDialog.h,
// which this no longer includes.
#include "ResourceDefines.h"
#include "PointsListState.h"
#include "MapEditorLib/EditorFactory.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/CommonEditorMethods.h"
#include "libdb/ResourceManager.h"
#include "EditorScene.h"
#include "Main/GameTimer.h"
#include "EditorMethods.h"
#include "EditorOptions.h"
#include "ED_B2_M1Dll.h"
#include "ExporterMethods.h"
#include "SceneB2/Camera.h"

#include "BuildingState.h"
#include "BuildingEditor.h"

#include <cstdint>

#include <zconf.h>

REGISTER_EDITOR_IN_DLL( BuildingRPGStats, CBuildingEditor )

const int N_POINT_TYPES_NUM = 5;
CString listLabels[N_POINT_TYPES_NUM];

CBuildingEditor::CBuildingEditor() : 
	pBuildingState(0),
	pwndShortcutBar(0),
	vLastCameraAnchor(VNULL3),
	szCurrSeason( "SEASON_SUMMER" ),
	bDrawPassability(false)
{
	listLabels[0].LoadString( theEDB2M1Instance, IDS_SMOKE_POINTS );
	listLabels[1].LoadString( theEDB2M1Instance, IDS_FIRE_POINTS );
	listLabels[2].LoadString( theEDB2M1Instance, IDS_ENTRANCE_POINTS );
	listLabels[3].LoadString( theEDB2M1Instance, IDS_SURFACE_POINTS );
	listLabels[4].LoadString( theEDB2M1Instance, IDS_DAMAGE_LEVELS );
}

CBuildingEditor::~CBuildingEditor()
{
}

const std::string &CBuildingEditor::GetCurrSeason() const
{
	return szCurrSeason;
}

void CBuildingEditor::ChangeSeason( const NDb::ESeason eSeason )
{
	szCurrSeason = typeSeasonMnemonics.GetMnemonic( eSeason );
	std::string szMapName = NEditorOptions::GetBgMap( GetCurrSeason() );
	CVec3 vCameraAnchor( NEditorOptions::GetBgMapAnchor(GetCurrSeason()) );
	if ( vCameraAnchor == VNULL3 ) 
	{
		vCameraAnchor.x = 16.0f * VIS_TILE_SIZE;
		vCameraAnchor.y = 16.0f * VIS_TILE_SIZE;
	}
	if ( !szMapName.empty() )
	{
		IEditorScene *pScene = EditorScene();
		pScene->ClearScene( SCENE_MISSION );
		pScene->SwitchScene( SCENE_MISSION );
		ReloadTerrain( szMapName, vCameraAnchor );
		ReloadModel( eSeason );
	}
}

void CBuildingEditor::ResetGUI( bool bActive )
{
    SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
    if ( !bActive )
        pUserData->SerializeSettings( editorSettings, "BuildingRPGStats", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );
    // Copy only GUI fields from the defaults, preserving all editing settings.
    const CBuildingRPGStatsEditorSettings defaults;
    editorSettings.bShowShortcutBar = defaults.bShowShortcutBar;
    pUserData->SerializeSettings( editorSettings, "BuildingRPGStats", SUserData::EDITOR_SETTINGS, SUserData::ST_SAVE );

    if ( pwndShortcutBar )
        pwndShortcutBar->Show( bActive && editorSettings.bShowShortcutBar );
}


void CBuildingEditor::CreateControls()
{
	unsigned nID = ID_BUILDING_EDITOR_DW;
	if ( pwndShortcutBar = Singleton<IMainFrameContainer>()->Get()->
		CreateControlBar( &nID, "BuildingEditorShortcutBar", CBRS_ALIGN_ANY, AFX_IDW_DOCKBAR_RIGHT, 0.5f, 200 ) )
	{
		nID = ID_BUILDING_EDITOR_SHORTCUT_PANE_0;
		if ( wndShortcutBar.Create( ToCWnd( pwndShortcutBar ), WS_CHILD | WS_VISIBLE | SEC_OBS_VERT | SEC_OBS_ANIMATESCROLL, nID ) )
		{
			// списки точек
			++nID;
			CDefault3DTabWindow *p3DTabWindow = new CDefault3DTabWindow();
			if ( wndShortcutBar.AddNewShortcut( p3DTabWindow ) )
			{
				p3DTabWindow->SetCommandHandlerID( CHID_BUILDING_POINTS_STATE, ID_BUILDING_POINTS_CHANGE_STATE ); 
				p3DTabWindow->Create( &wndShortcutBar, WS_CHILD | WS_VISIBLE | TWS_TABS_ON_BOTTOM | TWS_DRAW_3D_NORMAL );

				// Which toolkit draws the lists is NPointListView's business, not
				// the editor's. It creates each window and registers it in the tab
				// list; the label and the tab are still put on here.
				for ( int i = 0; i < N_POINT_TYPES_NUM; ++i )
				{
					if ( CWnd *pPointListDlg = NPointListView::Create( p3DTabWindow, i, (const char*)listLabels[i] ) )
					{
						++nID;
						CString strPaneLabel = listLabels[i];
						p3DTabWindow->AddTab( pPointListDlg, strPaneLabel );
					}
				}
				//
				p3DTabWindow->ActivateTab( 0 );

				CString strPaneLabel;
				strPaneLabel.LoadString( IDS_BUILDING_POINTS );
				wndShortcutBar.AddBar( p3DTabWindow, strPaneLabel, true );
			}
			wndShortcutBar.SelectPane( 0 );

			CWndWidget contentsWidget( &wndShortcutBar );
			Singleton<IMainFrameContainer>()->Get()->SetControlBarWindowContents( pwndShortcutBar, &contentsWidget );
			pwndShortcutBar->Show( false );
			pwndShortcutBar->ShowWithoutLayout( true );
			wndShortcutBar.ShowWindow( SW_SHOW );
			wndShortcutBar.SetCommandHandlerID( CHID_BUILDING_STATE, ID_BUILDING_CHANGE_STATE );
		}
	}
}

void CBuildingEditor::DestroyControls()
{
	if ( pwndShortcutBar )
	{
		if ( pwndShortcutBar->IsAlive() )
		{
			pwndShortcutBar->Destroy();
		}
		// The frame owns this IDockPanel handle; only its window is destroyed here.
		pwndShortcutBar = 0;
	}
	wndShortcutBar.DestroyWindow();

	Destroy();
}

void CBuildingEditor::Create()
{	
	// грузим файл с установками редактора
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	pUserData->SerializeSettings( editorSettings, "BuildingRPGStats", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );
	//
	pwndShortcutBar->Show( editorSettings.bShowShortcutBar );
	// Создаем стейты редактирования
	if ( pBuildingState == 0 )
	{
		pBuildingState = new CBuildingState( this );
	}
}

void CBuildingEditor::Destroy()
{
	if ( pwndShortcutBar != 0 )
	{
		editorSettings.bShowShortcutBar = pwndShortcutBar->IsVisible();
		pwndShortcutBar->Show( false );
	}
	// Записываем файл с установками (его могли поменять во время работы редактора)
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	pUserData->SerializeSettings( editorSettings, "BuildingRPGStats", SUserData::EDITOR_SETTINGS, SUserData::ST_SAVE );
	//
	if ( pBuildingState )
	{
		delete pBuildingState;
		pBuildingState = 0;
	}

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
}

bool CBuildingEditor::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	return false;
}

bool CBuildingEditor::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CBuildingEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CBuildingEditor::UpdateCommand(), pbCheck == 0" );
	//
	return false;
}

void CBuildingEditor::Undo( IController* pController )
{ }

void CBuildingEditor::Redo( IController* pController )
{ }

void CBuildingEditor::ReloadModel( NDb::ESeason eSeason )
{
	ILogger *pLogger = NLog::GetLogger();

	if ( GetObjectSet().objectNameSet.empty() )
	{
		pLogger->Log( LT_ERROR, "CBuildingEditor::ReloadModel() GetObjectSet().objectNameSet is empty" );
		return;
	}

	IEditorScene *pScene = EditorScene();
	if ( !pScene )
		return;

	// load building
	CVec3 vVisCameraPos = GetLastCameraAnchor();
		
	Camera()->SetAnchor( vVisCameraPos );

	if ( const NDb::SBuildingRPGStats *pBuildingStats = NDb::Get<NDb::SBuildingRPGStats>( GetObjectSet().objectNameSet.begin()->first ) )
	{
		Vis2AI( &vVisCameraPos );
		vBuildingPos = vVisCameraPos;
		vBuildingPos.z = GetTerrainHeight( vBuildingPos.x, vBuildingPos.y );

		if ( !IsModelValid() )
		{
			pBuilding = 0;
			pScene->RemoveObject( EDITOR_BUILDING_ID );
		}

		pBuilding = new CMOBuilding;

		CPtr<SAINewUnitUpdate> pNewUnitUpdate = new SAINewUnitUpdate;
		pNewUnitUpdate->info.pStats = pBuildingStats;
		pNewUnitUpdate->info.fResize = 1;
		pNewUnitUpdate->info.fHitPoints = pBuildingStats->fMaxHP;
		pNewUnitUpdate->info.fFuel = 1;
		pNewUnitUpdate->info.eDipl = EDI_FRIEND;
		pNewUnitUpdate->info.nPlayer = 0;
		pNewUnitUpdate->info.nFrameIndex = -1;
		pNewUnitUpdate->info.nExpLevel = 0;
		pNewUnitUpdate->info.center.x = vBuildingPos.x;
		pNewUnitUpdate->info.center.y = vBuildingPos.y;
		pNewUnitUpdate->info.z = vBuildingPos.z;
		pNewUnitUpdate->info.dir = 0;
		pNewUnitUpdate->info.fSpeed = 0;
		pNewUnitUpdate->info.cSoil = 0;
		pNewUnitUpdate->info.bNewFormat = false;

		pNewUnitUpdate->nUpdateTime = GameTimer()->GetAbsTime();
		if ( pBuilding->Create( EDITOR_BUILDING_ID, pNewUnitUpdate, eSeason, NDb::DAY_DAY, true ) == false )
		{
			NLog::Log( LT_ERROR, "Failed to create building\n" );
			NLog::Log( LT_ERROR, "\tObject ID: %s\n", NDb::GetResName(pBuildingStats) );
			pBuilding = 0;
		}
	}

	CPtr<IManipulator> pBuildingManipulator = CreateBuildingManipulator();
	GetPassability( &modelPassability, pBuildingManipulator );
}

// The one colour the passability overlay is drawn in: text, cell outlines and
// cell fill each spelled RGB(255,128,64) separately before.
static const TWidgetColor PASSABILITY_COLOR = RGB( 255, 128, 64 );


void CBuildingEditor::DrawPassability( IPaintContext *pPaintContext )
{
	// The old code saved the background mode and text colour by hand and put them
	// back at the end; SaveState/RestoreState is that pair, and it also covers the
	// brush and pen the passability block used to restore separately.
	pPaintContext->SaveState();
	pPaintContext->SetTextBackgroundOpaque( false );
	pPaintContext->SetTextColor( PASSABILITY_COLOR );

	if ( !szScreenTitle.empty() )
	{
		pPaintContext->DrawString( 8, 8, szScreenTitle );
	}

	if ( bDrawPassability )
	{
		// passability
		pPaintContext->SetBrush( PASSABILITY_COLOR );
		pPaintContext->SetPen( PEN_SOLID, 1, PASSABILITY_COLOR );

		pPaintContext->DrawString( 8, 40, "passability" );
		int yy = 80;

		for ( int y = 0; y < modelPassability.GetSizeY(); ++y )
		{
			int xx = 10;
			for ( int x = 0; x < modelPassability.GetSizeX(); ++x )
			{
				if ( !modelPassability[y][x] )
				{
					// An impassable cell is outlined rather than filled.
					pPaintContext->MoveTo( xx, yy );
					pPaintContext->LineTo( xx + 7, yy );

					pPaintContext->MoveTo( xx + 7, yy );
					pPaintContext->LineTo( xx + 7, yy + 7 );

					pPaintContext->MoveTo( xx + 7, yy + 7 );
					pPaintContext->LineTo( xx, yy + 7 );

					pPaintContext->MoveTo( xx, yy + 7 );
					pPaintContext->LineTo( xx, yy );
				}
				else
				{
					pPaintContext->Rectangle( CTRect<int>( xx, yy, xx + 7, yy + 7 ) );
				}
				xx += 10;
			}
			yy += 10;
		}
	}

	pPaintContext->RestoreState();
}

void CBuildingEditor::ReloadTerrain( const std::string &rszMapInfoName, const CVec3 &_vLastCameraAnchor )
{
	vLastCameraAnchor = _vLastCameraAnchor;
	//
	CVec2 bgMapSize = VNULL2;
	//
	NEditor::LoadBgMap( szCurrSeason, szLastTerrainName, &bgMapSize );
	//
	szLastTerrainName = rszMapInfoName;
	terrainSize.x = bgMapSize.x;
	terrainSize.y = bgMapSize.y;
}

void CBuildingEditor::SetScreenTitle( const std::string &rszScreenTitle )
{
	szScreenTitle = rszScreenTitle;
}

void CBuildingEditor::SetBuildingOrigin( const CVec2 &rvOrigin )
{
	vBuildingOrigin = rvOrigin;
}

CVec2 CBuildingEditor::GetBuildingOrigin()
{
	return vBuildingOrigin;
}

CVec3 CBuildingEditor::GetBuildingPos()
{
	return vBuildingPos;
}

IManipulator* CBuildingEditor::CreateBuildingManipulator()
{
	return Singleton<IResourceManager>()->CreateObjectManipulator( GetObjectSet().szObjectTypeName,
																																 GetObjectSet().objectNameSet.begin()->first );
}

int CBuildingRPGStatsEditorSettings::operator&( IXmlSaver &xs )
{
	xs.Add( "ShowShortcutBar", &bShowShortcutBar );
	return 0;
}


