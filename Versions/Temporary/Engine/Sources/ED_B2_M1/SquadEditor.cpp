#include "StdAfx.h"

#include "misc/2darray.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "commandhandlerdefines.h"
#include "mapeditorlib/commoneditormethods.h"
#include "StringResources.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/EditorFactory.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/DefaultTabWindow.h"
#include "SceneB2/Camera.h"
#include "FormationWindow.h"
#include "ED_B2_M1Dll.h"
#include "EditorMethods.h"
#include "SquadEditor.h"
#include "Tools_SceneGeometry.h"
#include "Stats_B2_M1/SceneModes.h"
#include "Stats_B2_M1/AnimModes.h"

#include <zconf.h>

REGISTER_EDITOR_IN_DLL( SquadRPGStats, CSquadEditor )

//
//
//					SQUAD EDITOR
//
//

CSquadEditor::CSquadEditor() : 
	pSquadState(0),
	pMarkers(0),
	pwndShortcutBar(0),
	vSquadCenterPos(VNULL3)
{
}

CSquadEditor::~CSquadEditor()
{
}

void CSquadEditor::Create()
{
	// Сначало грузим файл с установками редактора
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		pUserData->SerializeSettings( editorSettings, "SquadRPGStatsEditor", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );
	}
	//
	Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndShortcutBar, editorSettings.bShowShortcutBar, true );

	// Создаем стейты редактирования
	if ( pSquadState == 0 )
	{
		pSquadState = new CSquadState( this );
	}
}

void CSquadEditor::Destroy()
{
	if ( pwndShortcutBar != 0 )
	{
		editorSettings.bShowShortcutBar = pwndShortcutBar->IsVisible();
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndShortcutBar, false, true );
	}
	// Записываем файл с установками (его могли поменять во время работы редактора)
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		pUserData->SerializeSettings( editorSettings, "SquadRPGStatsEditor", SUserData::EDITOR_SETTINGS, SUserData::ST_SAVE );
	}
	//
	if ( pSquadState )
	{
		delete pSquadState;
		pSquadState = 0;
	}

	HideAxis();
	
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
}

bool CSquadEditor::HandleCommand( UINT nCommandID, DWORD dwData )
{
	return false;
}

bool CSquadEditor::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CSquadEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CSquadEditor::UpdateCommand(), pbCheck == 0" );
	//
	return false;
}

void CSquadEditor::ReloadTerrain()
{
	//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
	NEditor::LoadBgMap( "SEASON_SUMMER", string(), 0 );
	// точку куда смотрит камера будем использовать как центр для постановки взвода
	vSquadCenterPos = Camera()->GetAnchor();
	Vis2AI( &vSquadCenterPos );
	vSquadCenterPos.z = GetTerrainHeight( vSquadCenterPos.x, vSquadCenterPos.y );
}

int CSquadEditor::AddModel( const NDb::SModel *pModel, const CVec3 &rvPosition, int nMemberIndex )
{
	IEditorScene *pScene = EditorScene();

	CVec3 cp = GetSquadCenterPos();
	
	CVec3 vHumanPos = cp + rvPosition;

	vHumanPos.z = GetTerrainHeight( vHumanPos.x, vHumanPos.y );
				
	int nRes = pScene->AddObject( OBJECT_ID_GENERATE, pModel, vHumanPos, CQuat( 0.0f, V3_AXIS_Z ), CVec3(1,1,1), OBJ_ANIM_MODE_DEFAULT, 0 );
		
	SSquadMemberInfo inf;
	inf.nSceneObjectID = nRes;
	inf.pos = vHumanPos;
	inf.nMemberIndex = nMemberIndex;
	membersInfo.push_back( inf );

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );

	return nRes;
}

bool CSquadEditor::RemoveModel( int nID )
{
	EditorScene()->RemoveObject( nID ); 

	SSquadMemberInfo inf;
	inf.nSceneObjectID = nID;

	list<SSquadMemberInfo>::iterator fndIt = std::find( membersInfo.begin(), membersInfo.end(), inf );
	if ( fndIt != membersInfo.end() )
	{
		membersInfo.erase(fndIt);
		return true;
	}
	return false;
}

void CSquadEditor::CreateControls()
{
	UINT nID = ID_SQUAD_EDITOR_DW;
	if ( pwndShortcutBar = Singleton<IMainFrameContainer>()->Get()->
		CreateControlBar( &nID, "SquadEditorShortcutBar", CBRS_ALIGN_ANY, AFX_IDW_DOCKBAR_RIGHT, 0.5f, 200 ) )
	{
		nID = ID_SQUAD_EDITOR_SHORTCUT_PANE_0;
		if ( wndShortcutBar.Create( pwndShortcutBar, WS_CHILD | WS_VISIBLE | SEC_OBS_VERT | SEC_OBS_ANIMATESCROLL, nID ) )
		{
			// список типов формаций
			++nID;
			CDefault3DTabWindow *p3DTabWindow = new CDefault3DTabWindow();
			if ( wndShortcutBar.AddNewShortcut( p3DTabWindow ) )
			{
				p3DTabWindow->SetCommandHandlerID( CHID_BUILDING_POINTS_STATE, ID_BUILDING_POINTS_CHANGE_STATE ); 
				p3DTabWindow->Create( &wndShortcutBar, WS_CHILD | WS_VISIBLE | TWS_TABS_ON_BOTTOM | TWS_DRAW_3D_NORMAL );

				CFormationWindow *pDlg = new CFormationWindow();
				if ( p3DTabWindow->AddNewTab( pDlg ) )
				{
					AfxSetResourceHandle( theEDB2M1Instance );
					int bResult = pDlg->Create( CFormationWindow::IDD, p3DTabWindow );
					AfxSetResourceHandle( AfxGetInstanceHandle() );
					NI_ASSERT( bResult, "Creation of CFormationWindow dialog failed" );

					CString strPaneLabel = RCSTR("Formations");
					p3DTabWindow->AddTab( pDlg, strPaneLabel );
				}
				p3DTabWindow->ActivateTab( 0 );

				CString strPaneLabel = RCSTR("Squad");
				wndShortcutBar.AddBar( p3DTabWindow, strPaneLabel, true );
			}
			wndShortcutBar.SelectPane( 0 );

			Singleton<IMainFrameContainer>()->Get()->SetControlBarWindowContents( pwndShortcutBar, &wndShortcutBar );
			Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndShortcutBar, false, true );
			pwndShortcutBar->ShowWindow( SW_SHOW );
			wndShortcutBar.ShowWindow( SW_SHOW );
			wndShortcutBar.SetCommandHandlerID( CHID_SQUAD_STATE, ID_SQUAD_CHANGE_STATE );
		}
	}
}

void CSquadEditor::DestroyControls()
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


void CSquadEditor::RemoveAllModels()
{
	while ( !membersInfo.empty() )
		RemoveModel(membersInfo.front().nSceneObjectID);
}

IManipulator* CSquadEditor::CreateSquadManipulator()
{
	IManipulator* pSquadManip = Singleton<IResourceManager>()->
		CreateObjectManipulator
		( 
			GetObjectSet().szObjectTypeName, 
			GetObjectSet().objectNameSet.begin()->first 
		);
	return pSquadManip;
}

CVec3 CSquadEditor::GetModelPosition( int nID )
{
	SSquadMemberInfo inf;
	inf.nSceneObjectID = nID;

	list<SSquadMemberInfo>::iterator fndIt = std::find( membersInfo.begin(), membersInfo.end(), inf );
	if ( fndIt != membersInfo.end() )
	{
		return fndIt->pos;
	}
	return VNULL3;
}

CVec3 CSquadEditor::GetSquadCenterPos()
{
	return vSquadCenterPos;
}

bool CSquadEditor::SetModelPosition( int nID, const CVec3 &rvPos )
{
	SSquadMemberInfo inf;
	inf.nSceneObjectID = nID;

	list<SSquadMemberInfo>::iterator fndIt = std::find( membersInfo.begin(), membersInfo.end(), inf );
	if ( fndIt != membersInfo.end() )
	{
		fndIt->pos = rvPos;
		return true;
	}
	return false;
}

int CSquadEditor::GetModelMemberIndex( int nID )
{
	SSquadMemberInfo inf;
	inf.nSceneObjectID = nID;

	list<SSquadMemberInfo>::iterator fndIt = std::find( membersInfo.begin(), membersInfo.end(), inf );
	if ( fndIt != membersInfo.end() )
	{
		return fndIt->nMemberIndex;
	}
	return -1;
}

void CSquadEditor::ShowAxis()
{
	pMarkers = new SMarkerSet( GetSquadCenterPos(), VNULL2 );
	pMarkers->AddMarker( VNULL3, 0, DIR_IN_AIGRAD, true, true, false );
	pMarkers->AttachToScene();
}

void CSquadEditor::HideAxis()
{
	pMarkers = 0;
}

int CSquadEditor::GetMemberIndexBySceneID( int nSceneID )
{
	for ( list<SSquadMemberInfo>::const_iterator it = membersInfo.begin(); it != membersInfo.end(); ++it )
	{
		const SSquadMemberInfo *pMembInf = &(*it);
		if ( pMembInf->nSceneObjectID == nSceneID )
			return pMembInf->nMemberIndex;
	}
	return -1;
}


