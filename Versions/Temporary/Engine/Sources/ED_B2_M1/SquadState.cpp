#include "StdAfx.h"

#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commoneditormethods.h"
#include "resourcedefines.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "commandhandlerdefines.h"
#include "../SceneB2/Camera.h"
#include "../MapEditorLib/Interface_UserData.h"
#include "../MapEditorLib/DefaultTabWindow.h"
#include "SquadInterface.h"
#include "SquadState.h"
#include "SquadEditor.h"
#include "FormationsState.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const char *GetDesiredMapSeason()
{
	return "SEASON_SUMMER";
}

//
//
//			
//					SQUAD STATE					
//
//
//

CSquadState::CSquadState(  CSquadEditor *_pSquadEditor ) : 
	pSquadEditor( _pSquadEditor ), 
	bNeedLoadEnterConfig( true )
{
	int nStateIndex = INVALID_INPUT_STATE_INDEX;
	// IS_FORMATION
	{
		CMultiInputState *pMultiInputState = new CMultiInputState();
		nStateIndex = AddInputState( pMultiInputState );
		NI_ASSERT( nStateIndex == IS_FORMATION, StrFmt( "CSquadState(): Wrong state number IS_FORMATION: %d (%d)", nStateIndex, IS_FORMATION ) );
		// IS_FORMATION_ISS_FORMATION
		{
			CFormationsState *pObjectState = new CFormationsState( pSquadEditor );
			nStateIndex = pMultiInputState->AddInputState( pObjectState );
			NI_ASSERT( nStateIndex == FORMATION_ISS_FORMATION, StrFmt( "CSquadState(): Wrong state number FORMATION_ISS_FORMATION: %d, (%d)", nStateIndex, FORMATION_ISS_FORMATION ) );
		}
	}
}

bool CSquadState::IsMultiInputState( int nStateIndex )
{
	if ( ( nStateIndex == IS_FORMATION ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CSquadState::LoadEnterConfig()
{
	if ( bNeedLoadEnterConfig )
	{
		{
			CMultiInputState *pMultiInputState = checked_cast<CMultiInputState*>( GetInputState( IS_FORMATION ) );
			pMultiInputState->SetActiveInputState( FORMATION_ISS_FORMATION, false, false );
			SetActiveInputState( IS_FORMATION, false, false );
		}
		SetActiveInputState( IS_FORMATION, false, false );
		bNeedLoadEnterConfig = false;
	}
	CMultiInputState::Enter();
}

void CSquadState::Enter()
{
	NI_ASSERT( pSquadEditor != 0, "CSquadState::Enter(), pSquadEditor == 0" );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_GAME_INPUT, reinterpret_cast<DWORD>( new CSquadInterfaceCommand( new CSquadInterface() ) ) );
	//
	pSquadEditor->ReloadTerrain(); 
	//
	LoadEnterConfig();
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_BUILDING_STATE, this );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CSquadState::Leave()
{
	NI_ASSERT( pSquadEditor != 0, "CSquadState::Leave(), pSquadEditor == 0" );
	
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "CSquadState::Leave(): pScene == 0" );
	
	// store last loaded map and camera's anchor
	pSquadEditor->editorSettings.vLastBECameraAnchor = Camera()->GetAnchor();
	//
	CMultiInputState::Leave();
	Singleton<ICommandHandlerContainer>()->Remove( CHID_BUILDING_STATE );
	//
	const string szDebugParam = Singleton<IUserDataContainer>()->Get()->szDebugParam;
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_DISABLE_GAME_INPUT, 0 );
}


bool CSquadState::HandleCommand( UINT nCommandID, DWORD dwData )
{
 	switch( nCommandID )
	{
		case ID_SQUAD_CHANGE_STATE:
		{
			UINT nShortcutIndex = HIWORD( dwData );
			UINT nTabIndex = LOWORD( dwData );
			if ( ( nShortcutIndex != INVALID_SHORTCUT_INDEX ) &&
					 ( nShortcutIndex >= 0 ) &&
					 ( nShortcutIndex < GetCount() ) )
			{
				SetActiveInputState( nShortcutIndex, true, false );
			}
			if ( ( nTabIndex != INVALID_TAB_INDEX ) &&
					 IsMultiInputState( nShortcutIndex ) )
			{
				CMultiInputState *pMultiInputState = checked_cast<CMultiInputState*>( GetInputState( nShortcutIndex ) );
				NI_ASSERT( pMultiInputState != 0, "CSquadState::HandleCommand(), pMultiInputState == 0" );
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


bool CSquadState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CSquadState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CSquadState::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_SQUAD_CHANGE_STATE:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
	return false;
}


