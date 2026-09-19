#include "stdafx.h"

#include "Misc/2Darray.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "CommandHandlerDefines.h"
#include "MapEditorLib/CommonEditorMethods.h"
#include "StringResources.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/EditorFactory.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/PaletteList.h"
#include "SceneB2/Camera.h"
// Explicitly, where it used to arrive through FormationWindow.h: this file
// uses ID_SQUAD_EDITOR_DW and three more of its own ids, and the boundary
// header in front of the palette has no business carrying them.
#include "ResourceDefines.h"
#include "FormationView.h"
#include "ED_B2_M1Dll.h"
#include "EditorMethods.h"
#include "SquadEditor.h"
#include "Tools_SceneGeometry.h"
#include "Stats_B2_M1/SceneModes.h"
#include "Stats_B2_M1/AnimModes.h"

#include <cstdint>

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
	pwndShortcutBar->Show( editorSettings.bShowShortcutBar );

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
		pwndShortcutBar->Show( false );
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

bool CSquadEditor::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	return false;
}

bool CSquadEditor::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CSquadEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CSquadEditor::UpdateCommand(), pbCheck == 0" );
	//
	return false;
}

void CSquadEditor::ReloadTerrain()
{
	//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
	NEditor::LoadBgMap( "SEASON_SUMMER", std::string(), 0 );
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

	std::list<SSquadMemberInfo>::iterator fndIt = std::find( membersInfo.begin(), membersInfo.end(), inf );
	if ( fndIt != membersInfo.end() )
	{
		membersInfo.erase(fndIt);
		return true;
	}
	return false;
}

void CSquadEditor::ResetGUI( bool bActive )
{
    SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
    if ( !bActive )
        pUserData->SerializeSettings( editorSettings, "SquadRPGStatsEditor", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );
    // Copy only GUI fields from the defaults, preserving all editing settings.
    const CSquadEditorSettings defaults;
    editorSettings.bShowShortcutBar = defaults.bShowShortcutBar;
    pUserData->SerializeSettings( editorSettings, "SquadRPGStatsEditor", SUserData::EDITOR_SETTINGS, SUserData::ST_SAVE );

    if ( pwndShortcutBar )
        pwndShortcutBar->Show( bActive && editorSettings.bShowShortcutBar );
}


void CSquadEditor::CreateControls()
{
	unsigned nID = ID_SQUAD_EDITOR_DW;
	if ( pwndShortcutBar = Singleton<IMainFrameContainer>()->Get()->
		CreateControlBar( &nID, "SquadEditorShortcutBar", NMainFrameBar::ALIGN_ANY, NMainFrameBar::DOCK_RIGHT, 0.5f, 200 ) )
	{
		nID = ID_SQUAD_EDITOR_SHORTCUT_PANE_0;
		pShortcutBarView.reset( NShortcutBar::Create() );
		if ( pShortcutBarView->Create( pwndShortcutBar, nID ) )
		{
			// The formation types: one bar with one tab. Its tab changes go to the
			// building points state, as they always have. Which toolkit draws the
			// palette is NFormationView's business.
			const int nBar = pShortcutBarView->BeginBar( CHID_BUILDING_POINTS_STATE, ID_BUILDING_POINTS_CHANGE_STATE );
			pShortcutBarView->AddTab( nBar, RCSTR( "Formations" ), &NFormationView::Create );
			pShortcutBarView->ActivateTab( nBar, 0 );
			pShortcutBarView->EndBar( nBar, RCSTR( "Squad" ) );
			pShortcutBarView->SelectBar( 0 );
			//
			Singleton<IMainFrameContainer>()->Get()->SetControlBarWindowContents( pwndShortcutBar, pShortcutBarView->GetWidget() );
			pwndShortcutBar->Show( false );
			pwndShortcutBar->ShowWithoutLayout( true );
			pShortcutBarView->Show( true );
			pShortcutBarView->SetCommandHandlerID( CHID_SQUAD_STATE, ID_SQUAD_CHANGE_STATE );
		}
	}
}

void CSquadEditor::DestroyControls()
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
	if ( pShortcutBarView )
	{
		pShortcutBarView->Destroy();
	}

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

	std::list<SSquadMemberInfo>::iterator fndIt = std::find( membersInfo.begin(), membersInfo.end(), inf );
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

	std::list<SSquadMemberInfo>::iterator fndIt = std::find( membersInfo.begin(), membersInfo.end(), inf );
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

	std::list<SSquadMemberInfo>::iterator fndIt = std::find( membersInfo.begin(), membersInfo.end(), inf );
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
	for ( std::list<SSquadMemberInfo>::const_iterator it = membersInfo.begin(); it != membersInfo.end(); ++it )
	{
		const SSquadMemberInfo *pMembInf = &(*it);
		if ( pMembInf->nSceneObjectID == nSceneID )
			return pMembInf->nMemberIndex;
	}
	return -1;
}


