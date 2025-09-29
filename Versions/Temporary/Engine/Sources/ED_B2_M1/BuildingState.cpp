#include "StdAfx.h"

#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "seasonmnemonics.h"
#include "..\mapeditorlib\defaulttabwindow.h"
#include "pointlistdialog.h"
#include "..\mapeditorlib\interface_userdata.h"
#include "buildinginterface.h"
#include "pointsliststate.h"
#include "../MapEditorLib/CommonEditorMethods.h"

#include "BuildingState.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//		BUILDING STATE
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBuildingState::CBuildingState(  CBuildingEditor *_pBuildingEditor ) : 
	pBuildingEditor( _pBuildingEditor ), 
	bNeedLoadEnterConfig( true )
{
	int nStateIndex = INVALID_INPUT_STATE_INDEX;
	// IS_POINTS
	{
		CMultiInputState *pMultiInputState = new CMultiInputState();
		nStateIndex = AddInputState( pMultiInputState );
		NI_ASSERT( nStateIndex == IS_POINTS, StrFmt( "CBuildingState(): Wrong state number IS_POINTS: %d (%d)", nStateIndex, IS_POINTS ) );
		// POINTS_ISS_SMOKE_POINTS
		{
			CSmokePointsState *pState = new CSmokePointsState( pBuildingEditor );
			nStateIndex = pMultiInputState->AddInputState( pState );
			NI_ASSERT( nStateIndex == POINTS_ISS_SMOKE_POINTS, StrFmt( "CBuildingMainState(): Wrong state number POINTS_ISS_SMOKE_POINTS: %d, (%d)", nStateIndex, POINTS_ISS_SMOKE_POINTS ) );
		}
		// POINTS_ISS_SLOTS 
		{
			CSlotPointsState *pState = new CSlotPointsState( pBuildingEditor );
			nStateIndex = pMultiInputState->AddInputState( pState );
			NI_ASSERT( nStateIndex == POINTS_ISS_SLOTS, StrFmt( "CBuildingMainState(): Wrong state number POINTS_ISS_SLOTS: %d, (%d)", nStateIndex, POINTS_ISS_SLOTS ) );
		}
		// POINTS_ISS_ENTRANCE_POINTS
		{
			CEntrancePointsState *pState = new CEntrancePointsState( pBuildingEditor );
			nStateIndex = pMultiInputState->AddInputState( pState );
			NI_ASSERT( nStateIndex == POINTS_ISS_ENTRANCE_POINTS, StrFmt( "CBuildingMainState(): Wrong state number POINTS_ISS_ENTRANCE_POINTS: %d, (%d)", nStateIndex, POINTS_ISS_ENTRANCE_POINTS ) );
		}
		// POINTS_ISS_SURFACE_POINTS
		{
			CSurfacePointsState *pState = new CSurfacePointsState( pBuildingEditor );
			nStateIndex = pMultiInputState->AddInputState( pState );
			NI_ASSERT( nStateIndex == POINTS_ISS_SURFACE_POINTS, StrFmt( "CBuildingMainState(): Wrong state number POINTS_ISS_SURFACE_POINTS: %d, (%d)", nStateIndex, POINTS_ISS_SURFACE_POINTS ) );
		}
		// POINTS_ISS_DAMAGE_LEVELS
		{
			CDamageLevelsState *pState = new CDamageLevelsState( pBuildingEditor );
			nStateIndex = pMultiInputState->AddInputState( pState );
			NI_ASSERT( nStateIndex == POINTS_ISS_DAMAGE_LEVELS, StrFmt( "CBuildingMainState(): Wrong state number POINTS_ISS_DAMAGE_LEVELS: %d, (%d)", nStateIndex, POINTS_ISS_DAMAGE_LEVELS ) );
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CBuildingState::LoadEnterConfig()
{
	if ( bNeedLoadEnterConfig )
	{
		{
			CMultiInputState *pMultiInputState = checked_cast<CMultiInputState*>( GetInputState( IS_POINTS ) );
			pMultiInputState->SetActiveInputState( POINTS_ISS_SMOKE_POINTS, false, false );
			SetActiveInputState( IS_POINTS, false, false );
		}
		SetActiveInputState( IS_POINTS, false, false );
		bNeedLoadEnterConfig = false;
	}
	CMultiInputState::Enter();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CBuildingState::Enter()
{
	NI_ASSERT( pBuildingEditor != 0, "CBuildingState::Enter(), pBuildingEditor == 0" );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, 
																												ID_SCENE_ENABLE_GAME_INPUT, 
																												reinterpret_cast<DWORD>(new CBuildingInterfaceCommand(new CBuildingInterface())) );
	
	// читаем установки MapInfoEditor, чтобы выбрать террейн

	//UINT nMapID = -1;
	//if ( CPtr<IManipulator> pFolderMan = Singleton<IResourceManager>()->CreateFolderManipulator( "MapInfo" ) )
	//{
	//	string szMapName = NEditorOptions::GetBgMap( pBuildingEditor->GetCurrSeason() );
	//	nMapID = pFolderMan->GetID( szMapName );
	//}
	//CVec3 vCameraAnchor = NEditorOptions::GetBgMapAnchor( pBuildingEditor->GetCurrSeason() );
	//if ( vCameraAnchor == VNULL3 ) 
	//{
	//	vCameraAnchor.x = 16.0f * VIS_TILE_SIZE;
	//	vCameraAnchor.y = 16.0f * VIS_TILE_SIZE;
	//}

	//pBuildingEditor->ReloadTerrain( nMapID, vCameraAnchor ); // не нужно, так как выполняется в CBuildingEditor::ChangeSeason
	pBuildingEditor->ChangeSeason( (NDb::ESeason)typeSeasonMnemonics.GetValue(pBuildingEditor->GetCurrSeason()) );
	//
	LoadEnterConfig();
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_BUILDING_STATE, this );
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_RESET_CAMERA, false );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CBuildingState::Leave()
{
	NI_ASSERT( pBuildingEditor != 0, "CBuildingState::Leave(), pBuildingEditor == 0" );
	
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "CBuildingState::Leave(): pScene == 0" );
	
	//
	CMultiInputState::Leave();
	Singleton<ICommandHandlerContainer>()->Remove( CHID_BUILDING_STATE );
	
	//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
	const string szDebugParam = Singleton<IUserDataContainer>()->Get()->szDebugParam;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_DISABLE_GAME_INPUT, 0 );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CBuildingState::HandleCommand( UINT nCommandID, DWORD dwData )
{
	//DebugTrace( "Changing building state: nCommandID = %d, dwData = %d", nCommandID, dwData );
 	switch( nCommandID )
	{
		case ID_BUILDING_CHANGE_STATE:
		{
			UINT nShortcutIndex = HIWORD( dwData );
			UINT nTabIndex = LOWORD( dwData );
			if ( ( nShortcutIndex != INVALID_SHORTCUT_INDEX ) &&
					 ( nShortcutIndex >= 0 ) &&
					 ( nShortcutIndex < GetCount() ) )
			{
				SetActiveInputState( nShortcutIndex, true, false );
			}
			if ( ( nTabIndex != INVALID_TAB_INDEX ) && IsMultiInputState( nShortcutIndex ) )
			{
				CMultiInputState *pMultiInputState = checked_cast<CMultiInputState*>( GetInputState( nShortcutIndex ) );
				NI_ASSERT( pMultiInputState != 0, "CBuildingState::HandleCommand(), pMultiInputState == 0" );
				if ( ( nTabIndex >= 0 ) && 
						 ( nTabIndex < pMultiInputState->GetCount() ) )
				{
					pMultiInputState->SetActiveInputState( nTabIndex, true, false );
				}
			}
		}
		default:
			return false;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CBuildingState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CBuildingState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CBuildingState::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_BUILDING_CHANGE_STATE:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
