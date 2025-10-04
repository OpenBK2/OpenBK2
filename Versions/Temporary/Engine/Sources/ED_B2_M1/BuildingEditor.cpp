#include "stdafx.h"

#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "Misc/2Darray.h"
#include "seasonmnemonics.h"
#include "mapeditorlib/defaulttabwindow.h"
#include "pointlistdialog.h"
#include "pointsliststate.h"
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

const string &CBuildingEditor::GetCurrSeason() const
{
	return szCurrSeason;
}

void CBuildingEditor::ChangeSeason( const NDb::ESeason eSeason )
{
	szCurrSeason = typeSeasonMnemonics.GetMnemonic( eSeason );
	string szMapName = NEditorOptions::GetBgMap( GetCurrSeason() );
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

void CBuildingEditor::CreateControls()
{
	UINT nID = ID_BUILDING_EDITOR_DW;
	if ( pwndShortcutBar = Singleton<IMainFrameContainer>()->Get()->
		CreateControlBar( &nID, "BuildingEditorShortcutBar", CBRS_ALIGN_ANY, AFX_IDW_DOCKBAR_RIGHT, 0.5f, 200 ) )
	{
		nID = ID_BUILDING_EDITOR_SHORTCUT_PANE_0;
		if ( wndShortcutBar.Create( pwndShortcutBar, WS_CHILD | WS_VISIBLE | SEC_OBS_VERT | SEC_OBS_ANIMATESCROLL, nID ) )
		{
			// списки точек
			++nID;
			CDefault3DTabWindow *p3DTabWindow = new CDefault3DTabWindow();
			if ( wndShortcutBar.AddNewShortcut( p3DTabWindow ) )
			{
				p3DTabWindow->SetCommandHandlerID( CHID_BUILDING_POINTS_STATE, ID_BUILDING_POINTS_CHANGE_STATE ); 
				p3DTabWindow->Create( &wndShortcutBar, WS_CHILD | WS_VISIBLE | TWS_TABS_ON_BOTTOM | TWS_DRAW_3D_NORMAL );

				for ( int i = 0; i < N_POINT_TYPES_NUM; ++i )
				{
					CPointListDialog* pPointListDlg = new CPointListDialog( i, listLabels[i] );
					if ( p3DTabWindow->AddNewTab( pPointListDlg ) )
					{
						AfxSetResourceHandle( theEDB2M1Instance );
						int bResult = pPointListDlg->Create( CPointListDialog::IDD, p3DTabWindow );
						AfxSetResourceHandle( AfxGetInstanceHandle() );
						if ( !bResult )
						{
							NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Creation of CPointListDialog dialog failed" ) );
						}
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

			Singleton<IMainFrameContainer>()->Get()->SetControlBarWindowContents( pwndShortcutBar, &wndShortcutBar );
			Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndShortcutBar, false, true );
			pwndShortcutBar->ShowWindow( SW_SHOW );
			wndShortcutBar.ShowWindow( SW_SHOW );
			wndShortcutBar.SetCommandHandlerID( CHID_BUILDING_STATE, ID_BUILDING_CHANGE_STATE );
		}
	}
}

void CBuildingEditor::DestroyControls()
{
	if ( pwndShortcutBar )
	{
		if ( ::IsWindow( pwndShortcutBar->m_hWnd ) )
		{
			pwndShortcutBar->DestroyWindow();
		}
		delete pwndShortcutBar;
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
	Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndShortcutBar, editorSettings.bShowShortcutBar, true );
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
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndShortcutBar, false, true );
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

bool CBuildingEditor::HandleCommand( UINT nCommandID, DWORD dwData )
{
	return false;
}

bool CBuildingEditor::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
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

void CBuildingEditor::DrawPassability( class CPaintDC *pPaintDC )
{
	int nOldBkMode = pPaintDC->SetBkMode( TRANSPARENT );
	COLORREF oldColor = pPaintDC->GetTextColor(); 
	pPaintDC->SetTextColor( RGB(255,128,64) ); 

	if ( !szScreenTitle.empty() )
	{
		pPaintDC->TextOut( 8, 8, szScreenTitle.c_str(), szScreenTitle.length() );
	}

	if ( bDrawPassability )
	{
		// passability
		CBrush *pOldBrush = pPaintDC->GetCurrentBrush(); 
		CPen *pOldPen = pPaintDC->GetCurrentPen();

		CBrush brush;
		brush.CreateSolidBrush( RGB(255,128,64) );
		CPen pen;
		pen.CreatePen( PS_SOLID, 1, RGB(255,128,64) ); 
		pPaintDC->SelectObject( &brush );
		pPaintDC->SelectObject( &pen );

		pPaintDC->TextOut( 8, 40, "passability" );
		int yy = 80;

		for ( int y = 0; y < modelPassability.GetSizeY(); ++y )
		{
			int xx = 10;
			for ( int x = 0; x < modelPassability.GetSizeX(); ++x )
			{
				if ( !modelPassability[y][x] )
				{
					pPaintDC->MoveTo( xx, yy );
					pPaintDC->LineTo( xx + 7, yy ); 

					pPaintDC->MoveTo( xx + 7, yy );
					pPaintDC->LineTo( xx + 7, yy + 7 );

					pPaintDC->MoveTo( xx + 7, yy + 7 ); 
					pPaintDC->LineTo( xx, yy + 7 );

					pPaintDC->MoveTo( xx, yy + 7 ); 
					pPaintDC->LineTo( xx, yy ); 
				}
				else
				{
					pPaintDC->Rectangle( xx, yy, xx + 7, yy + 7 );
				}
				xx += 10;
			}
			yy += 10;
		}
		//
		pPaintDC->SelectObject( pOldPen );
		pPaintDC->SelectObject( pOldBrush );
	}

	pPaintDC->SetTextColor( oldColor ); 
	pPaintDC->SetBkMode( nOldBkMode );
}

void CBuildingEditor::ReloadTerrain( const string &rszMapInfoName, const CVec3 &_vLastCameraAnchor )
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

void CBuildingEditor::SetScreenTitle( const string &rszScreenTitle )
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


