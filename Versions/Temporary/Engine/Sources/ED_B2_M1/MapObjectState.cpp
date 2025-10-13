#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "mapeditorlib/tools_hashset.h"
#include "SceneB2/Scene.h"
#include "mapobjectstate.h"
#include "ResourceDefines.h"

#include "MapEditorLib/Interface_MainFrame.h"
#include "MapObjectMultiState.h"
#include "MapObjectState.h"
#include "ED_B2_M1Dll.h"
#include "MapInfoEditor.h"

#include <cstdint>

#include <zconf.h>

const uint32_t CMapObjectState::SELECTION_LINE_COLOR	= 0xFF00FF00;


void CMapObjectSelectState::OnMouseButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint, UINT nButtonType )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
		{
			bool bModified = false;
			if ( ( GetAsyncKeyState( VK_MENU ) & 0x8000 ) > 0 )
			{
				pParentState->GetObjectInfoCollector()->BackupSelectionPosition();
				pParentState->GetObjectInfoCollector()->PickSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
				pParentState->MoveSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos, false, true, false );
				pParentState->GetObjectInfoCollector()->PickSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
				pParentState->GetObjectInfoCollector()->SetSelectionType( NMapInfoEditor::SObjectSelection::ST_CENTER );
			}
			else if ( ( nFlags & MK_CONTROL ) > 0 )
			{
				pParentState->GetObjectInfoCollector()->BackupSelectionPosition();
				pParentState->GetObjectInfoCollector()->PickSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
				pParentState->RotateSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos, true, true, false );
				pParentState->GetObjectInfoCollector()->PickSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
				pParentState->GetObjectInfoCollector()->SetSelectionType( NMapInfoEditor::SObjectSelection::ST_DIRECTION );
			}
			else
			{
				pParentState->GetObjectInfoCollector()->PickSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
				if ( ( pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_CENTER ) ||
						 ( pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_DIRECTION ) )
				{
					pParentState->GetObjectInfoCollector()->BackupSelectionPosition();
				}
			}
			if ( ( pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_CENTER ) ||
					 ( pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_DIRECTION ) )
			{
				pParentState->SetActiveInputState( CMapObjectState::IS_EDIT, true, false );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE,	ID_UPDATE_EDIT_PARAMETERS, MIMOSEP_DIRECTION );
				return;
			}
		}
		//
		list<int> sceneIDList;
		pScene->PickObjects( sceneIDList, CVec2( rMousePoint.x, rMousePoint.y ) );
		pParentState->GetObjectInfoCollector()->Pick( &sceneIDList, rMousePoint );
		if ( !sceneIDList.empty() )
		{
			pParentState->selector.Clear();
			pParentState->nSelectedSceneID = sceneIDList.front();
			//
			list<UINT> selectedObjectSceneIDList;
			selectedObjectSceneIDList.push_back( pParentState->nSelectedSceneID );
			//
			if ( !( pParentState->GetObjectInfoCollector()->IsInSelection( selectedObjectSceneIDList ) ) )
			{
				// выделили объект, необходимо обновить Selection и перейти в стейт редактирования
				if ( ( nFlags & MK_SHIFT ) > 0 )
				{
					pParentState->GetObjectInfoCollector()->InsertToSelection( selectedObjectSceneIDList, false );
				}
				else
				{
					pParentState->GetObjectInfoCollector()->CreateSelection( selectedObjectSceneIDList );
				}
				pParentState->GetObjectInfoCollector()->PickSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
				pParentState->GetObjectInfoCollector()->SetSelectionType( NMapInfoEditor::SObjectSelection::ST_CENTER );
				pParentState->GetObjectInfoCollector()->BackupSelectionPosition();
				pParentState->SetActiveInputState( CMapObjectState::IS_EDIT, true, false );
			}
			else
			{
				if ( ( nFlags & MK_SHIFT ) > 0 )
				{
					pParentState->GetObjectInfoCollector()->RemoveFromSelection( selectedObjectSceneIDList );
				}
				else
				{
					pParentState->GetObjectInfoCollector()->PickSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
					pParentState->GetObjectInfoCollector()->SetSelectionType( NMapInfoEditor::SObjectSelection::ST_CENTER );
					pParentState->GetObjectInfoCollector()->BackupSelectionPosition();
					pParentState->SetActiveInputState( CMapObjectState::IS_EDIT, true, false );
				}
			}
		}
		else
		{
			/**
			pParentState->GetObjectInfoCollector()->PickSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
			if ( ( pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_CENTER ) ||
					 ( pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_DIRECTION ) )
			{
				pParentState->GetObjectInfoCollector()->BackupSelectionPosition();
				pParentState->SetActiveInputState( CMapObjectState::IS_EDIT, true, false );
			}
			/**/
			pParentState->GetObjectInfoCollector()->ResetPickSelection();
			//
			pParentState->selector.bTerrainSelector = ( nButtonType == MK_RBUTTON );
			pParentState->selector.frameRect.minx = pParentState->pStoreInputState->lastEventInfo.point.x;
			pParentState->selector.frameRect.miny = pParentState->pStoreInputState->lastEventInfo.point.y;
			pParentState->selector.frameRect.maxx = pParentState->pStoreInputState->lastEventInfo.point.x;
			pParentState->selector.frameRect.maxy = pParentState->pStoreInputState->lastEventInfo.point.y;
			pParentState->selector.vTerrainPos0 = pParentState->pStoreInputState->lastEventInfo.vTerrainPos;
			pParentState->selector.vTerrainPos1 = pParentState->pStoreInputState->lastEventInfo.vTerrainPos;
			pParentState->selector.bValid = true;
			pParentState->nSelectedSceneID = INVALID_NODE_ID;
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE,	ID_UPDATE_EDIT_PARAMETERS, MIMOSEP_DIRECTION );
	}
}


void CMapObjectSelectState::OnMouseButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint, UINT nButtonType )
{
	if ( pParentState->selector.IsValid() )
	{
		pParentState->selector.bTerrainSelector = ( nButtonType == MK_RBUTTON );
		pParentState->selector.frameRect.maxx = pParentState->pStoreInputState->lastEventInfo.point.x;
		pParentState->selector.frameRect.maxy = pParentState->pStoreInputState->lastEventInfo.point.y;
		pParentState->selector.vTerrainPos1 = pParentState->pStoreInputState->lastEventInfo.vTerrainPos;
		pParentState->selector.bValid = true;
		//
		UpdateSelectionBySelector( ( nFlags & MK_SHIFT ) > 0 );
	}
	pParentState->selector.Clear();
	pParentState->nSelectedSceneID = INVALID_NODE_ID;
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE,	ID_UPDATE_EDIT_PARAMETERS, MIMOSEP_DIRECTION );
}


void CMapObjectSelectState::UpdateSelectionBySelector( bool bShiftPressed )
{
	if ( pParentState->selector.IsValid() )
	{
		if ( pParentState->selector.bTerrainSelector )
		{
			if ( bShiftPressed )
			{
				pParentState->GetObjectInfoCollector()->InsertToSelection( pParentState->selector.vTerrainPos0, 
																											 pParentState->selector.vTerrainPos1,
																											 false );
			}
			else
			{
				pParentState->GetObjectInfoCollector()->CreateSelection( pParentState->selector.vTerrainPos0,
																										 pParentState->selector.vTerrainPos1 );
			}
		}
		else
		{
			if ( IEditorScene *pScene = EditorScene() )
			{
				list<int> sceneIDList;
				pScene->PickObjects( sceneIDList,
														 CVec2( pParentState->selector.frameRect.minx, pParentState->selector.frameRect.miny ),
														 CVec2( pParentState->selector.frameRect.maxx, pParentState->selector.frameRect.maxy ) );
				pParentState->GetObjectInfoCollector()->Pick( &sceneIDList, pParentState->selector.frameRect );
				if ( bShiftPressed )
				{
					pParentState->GetObjectInfoCollector()->InsertToSelection( sceneIDList, false );
				}
				else
				{
					pParentState->GetObjectInfoCollector()->CreateSelection( sceneIDList );
				}
			}
		}
	}
}


void CMapObjectSelectState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnMouseMove( nFlags, rMousePoint );
		//
		if ( ( ( nFlags & MK_LBUTTON ) > 0 ) || ( ( nFlags & MK_RBUTTON ) > 0 ) )
		{
			if ( pParentState->selector.IsValid() )
			{
				pParentState->selector.frameRect.maxx = pParentState->pStoreInputState->lastEventInfo.point.x;
				pParentState->selector.frameRect.maxy = pParentState->pStoreInputState->lastEventInfo.point.y;
				pParentState->selector.vTerrainPos1 = pParentState->pStoreInputState->lastEventInfo.vTerrainPos;
				pParentState->selector.bValid = true;
				pParentState->nSelectedSceneID = INVALID_NODE_ID;
				//
				UpdateSelectionBySelector( ( nFlags & MK_SHIFT ) > 0 );
				//
				//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE,	ID_UPDATE_EDIT_PARAMETERS, MIMOSEP_DIRECTION );
			}
		}
		else
		{
			pParentState->GetObjectInfoCollector()->ResetPickSelection();
		}
	}
	if ( IEditorScene *pScene = EditorScene() )
	{
		list<int> sceneIDList;
		pScene->PickObjects( sceneIDList, CVec2( rMousePoint.x, rMousePoint.y ) );
		CString strStatusBarMessage;
		if ( sceneIDList.size() > 1 )
		{
			strStatusBarMessage.LoadString( IDS_STATUS_STRING_OBJECTS );
			Singleton<IMainFrameContainer>()->Get()->SetStatusBarText( 1, StrFmt( strStatusBarMessage, sceneIDList.size() ) );
		}
		else if ( sceneIDList.size() > 0 )
		{
			strStatusBarMessage.LoadString( IDS_STATUS_STRING_OBJECT );
			if ( const NDb::SMapObjectInfo *pMapOnjectInfo = pParentState->GetObjectInfoCollector()->GetObjectStatusBarParams( *( sceneIDList.begin() ) ) )
			{
				CDBID dbid;
				if ( pMapOnjectInfo->pObject )
				{
					dbid = pMapOnjectInfo->pObject->GetDBID();
				}
				Singleton<IMainFrameContainer>()->Get()->SetStatusBarText( 1, StrFmt( strStatusBarMessage,
																																							pMapOnjectInfo->nScriptID,
																																							dbid.ToString().c_str() ) );
			}
		}
	}	
}


void CMapObjectSelectState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
		//
		OnMouseButtonDown( nFlags, rMousePoint, MK_LBUTTON );
	}
}


void CMapObjectSelectState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonUp( nFlags, rMousePoint );
		//
		OnMouseButtonUp( nFlags, rMousePoint, MK_LBUTTON );
	}
}


void CMapObjectSelectState::OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnRButtonDown( nFlags, rMousePoint );
		//
		OnMouseButtonDown( nFlags, rMousePoint, MK_RBUTTON );
	}
}


void CMapObjectSelectState::OnRButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnRButtonDown( nFlags, rMousePoint );
		//
		OnMouseButtonUp( nFlags, rMousePoint, MK_RBUTTON );
	}
}


void CMapObjectSelectState::OnMButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnMButtonDown( nFlags, rMousePoint );
		//
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			::SetCursor( ::LoadCursor( 0, IDC_ARROW ) );
			ICommandHandlerContainer* pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
			pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE, ID_MIMO_SWITCH_MULTI_STATE, reinterpret_cast<uint32_t>( &( objectSet.szObjectTypeName ) ) );
			pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_STATE, ID_MIMO_SWITCH_ADD_STATE, 0 );	
		}
	}
}


void CMapObjectSelectState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnKeyDown( nChar, nRepCnt, nFlags );
		//
		if ( nChar == VK_ESCAPE )
		{
			pParentState->ClearSelection();
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
		else if ( nChar == VK_SPACE )
		{
			if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				pParentState->SetSelectionHeightsToZero( true );
				pParentState->GetObjectInfoCollector()->PickSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
			}
		}
		else if ( nChar == VK_RETURN )
		{
			if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				pParentState->GetObjectInfoCollector()->ShowSelectionPropertyManipulator( pParentState->GetMapInfoEditor()->GetViewManipulator(), pParentState->GetMapInfoEditor()->GetObjectSet() );
			}
		}
		else if ( nChar == VK_DELETE )
		{
			if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				pParentState->RemoveSelection();
			}
		}
		else if ( nChar == VK_HOME )
		{
			if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				pParentState->RemoveSelectionLinkTo();
			}
		}
		else if ( nChar == VK_END )
		{
			if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				pParentState->RemoveSelectionLinks();
			}
		}
		/**
		else if ( nChar == VK_INSERT )
		{
			SObjectSet objectSet;
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
			{
				::SetCursor( ::LoadCursor( 0, IDC_ARROW ) );
				ICommandHandlerContainer* pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
				pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE, ID_MIMO_SWITCH_MULTI_STATE, reinterpret_cast<uint32_t>( &( objectSet.szObjectTypeName ) ) );
				pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_STATE, ID_MIMO_SWITCH_ADD_STATE, 0 );	
			}
		}
		/**/
		else if ( nChar == 'Q' )
		{
			pParentState->GetObjectInfoCollector()->Trace();			
		}
	}
}


void CMapObjectEditState::RecalculateSelection( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->nSelectedSceneID != INVALID_NODE_ID )
	{
		if ( IEditorScene *pScene = EditorScene() )
		{
			list<int> sceneIDList;
			pScene->PickObjects( sceneIDList, CVec2( rMousePoint.x, rMousePoint.y ) );
			pParentState->GetObjectInfoCollector()->Pick( &sceneIDList, rMousePoint );
			if ( !sceneIDList.empty() )
			{
				const UINT nNewSelectedObjectSceneID = pParentState->GetNextSceneID( pParentState->nSelectedSceneID, &sceneIDList );
				if ( nNewSelectedObjectSceneID != pParentState->nSelectedSceneID )
				{
					// удаляем старый объект
					if ( ( nFlags & MK_SHIFT ) == 0 )
					{
						list<UINT> selectedObjectSceneIDList;
						selectedObjectSceneIDList.push_back( pParentState->nSelectedSceneID );
						pParentState->GetObjectInfoCollector()->RemoveFromSelection( selectedObjectSceneIDList );
						// записываем его положение в базу:
						const UINT nMapObjectInfoID = pParentState->GetObjectInfoCollector()->Pick( pParentState->nSelectedSceneID );
						if ( nMapObjectInfoID != INVALID_NODE_ID )
						{
							if ( CPtr<CObjectBaseController> pObjectController = pParentState->GetMapInfoEditor()->CreateController() )
							{
								if ( pParentState->GetObjectInfoCollector()->UpdateDB( nMapObjectInfoID,
																													 true,
																													 pObjectController,
																													 pParentState->GetMapInfoEditor()->GetViewManipulator() ) )
								{
									pObjectController->Redo( false, true, pParentState->GetMapInfoEditor() );
									Singleton<IControllerContainer>()->Add( pObjectController );
								}
							}
						}
					}
					pParentState->nSelectedSceneID = nNewSelectedObjectSceneID;
					// вставляем новывй
					{
						list<UINT> selectedObjectSceneIDList;
						selectedObjectSceneIDList.push_back( pParentState->nSelectedSceneID );
						pParentState->GetObjectInfoCollector()->InsertToSelection( selectedObjectSceneIDList, false );
					}
					//
					pParentState->GetObjectInfoCollector()->PickSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
					pParentState->GetObjectInfoCollector()->BackupSelectionPosition();
					Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
					Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE,	ID_UPDATE_EDIT_PARAMETERS, MIMOSEP_DIRECTION );
				}
			}
		}
	}
}


void CMapObjectEditState::OperateSelection( UINT nFlags, const CTPoint<int> &rMousePoint, bool bSave )
{
	if ( ( nFlags & MK_LBUTTON ) > 0 )
	{
		if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
		{
			if (	(pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_OUT) ||
						(pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_CENTER) )
			{
				pParentState->MoveSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos, false, false, bSave );
			}
			else if ( pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_DIRECTION ) 
			{
				pParentState->RotateSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos, false, false, bSave );
			}
			// проверяем на наличие линка
			if ( ( nFlags & MK_SHIFT ) > 0 )
			{
				if ( IEditorScene *pScene = EditorScene() )
				{
					list<int> sceneIDList;
					pScene->PickObjects( sceneIDList, CVec2( rMousePoint.x, rMousePoint.y ) );
					pParentState->GetObjectInfoCollector()->Pick( &sceneIDList, rMousePoint );
					if ( !sceneIDList.empty() )
					{
						int nLinkToSceneID = INVALID_NODE_ID;
						for ( list<int>::const_iterator itDceneID = sceneIDList.begin(); itDceneID != sceneIDList.end(); ++itDceneID )
						{
							if ( !pParentState->GetObjectInfoCollector()->IsInSelection( *itDceneID ) )
							{
								nLinkToSceneID = *itDceneID;
								break;
							}
						}
						if ( ( nLinkToSceneID != INVALID_NODE_ID ) &&
									pParentState->GetObjectInfoCollector()->CheckSelectionLinkCapability( nLinkToSceneID ) )
						{
							if ( bSave )
							{
								pParentState->InsertSelectionLink( nLinkToSceneID );
								::SetCursor( ::LoadCursor( 0, IDC_ARROW ) );
							}
							else
							{
								::SetCursor( ::LoadCursor( 0, IDC_UPARROW ) );
							}
						}
						else
						{
							::SetCursor( ::LoadCursor( 0, IDC_ARROW ) );
						}
					}
				}
			}
		}
	}
	else if ( ( nFlags & MK_RBUTTON ) > 0 )
	{
		// поднимаем опускаем
		if ( ( pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_OUT ) ||
				 ( pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_CENTER )  ||
				 ( pParentState->GetObjectInfoCollector()->GetSelectionType() == NMapInfoEditor::SObjectSelection::ST_DIRECTION ) )
		{
			CVec3 vTerrainPos = pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_RBUTTONDOWN].vTerrainPos;
			vTerrainPos.z = ( pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_RBUTTONDOWN].point.y - pParentState->pStoreInputState->lastEventInfo.point.y ) / NMapInfoEditor::HEIGHT_DELIMITER;
			pParentState->MoveSelection( vTerrainPos, false, false, bSave );
		}
	}
}


void CMapObjectEditState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnMouseMove( nFlags, rMousePoint );
		//
		if ( IEditorScene *pScene = EditorScene() )
		{
			OperateSelection( nFlags, rMousePoint, false );
		}
	}
}


void CMapObjectEditState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
		//
		if ( ( nFlags & MK_RBUTTON ) > 0 )
		{
			RecalculateSelection( nFlags, rMousePoint );
		}
	}
}


void CMapObjectEditState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonUp( nFlags, rMousePoint );
		::SetCursor( ::LoadCursor( 0, IDC_ARROW ) );
		//
		if ( ( nFlags & MK_RBUTTON ) == 0 )
		{
			OperateSelection( nFlags | MK_LBUTTON, rMousePoint, true );
			//
			pParentState->GetObjectInfoCollector()->ResetPickSelection();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectEditState::OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnRButtonDown( nFlags, rMousePoint );
		//
		if ( ( nFlags & MK_LBUTTON ) > 0 )
		{
			RecalculateSelection( nFlags, rMousePoint );
		}
	}
}


void CMapObjectEditState::OnRButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnRButtonUp( nFlags, rMousePoint );
		//
		if ( ( nFlags & MK_LBUTTON ) == 0 )
		{
			OperateSelection( nFlags | MK_RBUTTON, rMousePoint, true );
			//
			pParentState->GetObjectInfoCollector()->ResetPickSelection();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectEditState::OnMButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnMButtonDown( nFlags, rMousePoint );
		//
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			::SetCursor( ::LoadCursor( 0, IDC_ARROW ) );
			ICommandHandlerContainer* pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
			pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE, ID_MIMO_SWITCH_MULTI_STATE, reinterpret_cast<uint32_t>( &( objectSet.szObjectTypeName ) ) );
			pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_STATE, ID_MIMO_SWITCH_ADD_STATE, 0 );	
		}
	}
}


void CMapObjectEditState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnKeyDown( nChar, nRepCnt, nFlags );
		//
		if ( nChar == VK_ESCAPE )
		{
			if ( IEditorScene *pScene = EditorScene() )
			{
				NMapInfoEditor::SObjectEditInfo objectEditInfo;
				objectEditInfo.bRotateTo90Degree = false;
				objectEditInfo.bFitToGrid = false;
				//
				pParentState->GetObjectInfoCollector()->RollbackSelectionPosition( &objectEditInfo, pScene );
				pParentState->GetObjectInfoCollector()->ResetPickSelection();
				pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
				//
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			}
		}
		else if ( nChar == VK_SPACE )
		{
			if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				// для правильной работы поднимания / опускания юнитов.
				pParentState->pStoreInputState->OnRButtonDown( pParentState->pStoreInputState->lastEventInfo.nFlags, pParentState->pStoreInputState->lastEventInfo.point );
				pParentState->SetSelectionHeightsToZero( false );
				pParentState->GetObjectInfoCollector()->PickSelection( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
			}
		}
		else if ( nChar == VK_RETURN )
		{
			if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				pParentState->GetObjectInfoCollector()->ShowSelectionPropertyManipulator( pParentState->GetMapInfoEditor()->GetViewManipulator(), pParentState->GetMapInfoEditor()->GetObjectSet() );
			}
		}
		else if ( nChar == VK_DELETE ) 
		{
			if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				pParentState->RemoveSelection();
			}
		}
		else if ( nChar == VK_HOME )
		{
			if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				pParentState->RemoveSelectionLinkTo();
			}
		}
		else if ( nChar == VK_END )
		{
			if ( !pParentState->GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				pParentState->RemoveSelectionLinks();
			}
		}
		/**
		else if ( nChar == VK_INSERT )
		{
			SObjectSet objectSet;
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
			{
				::SetCursor( ::LoadCursor( 0, IDC_ARROW ) );
				ICommandHandlerContainer* pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
				pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE, ID_MIMO_SWITCH_MULTI_STATE, reinterpret_cast<uint32_t>( &( objectSet.szObjectTypeName ) ) );
				pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_STATE, ID_MIMO_SWITCH_ADD_STATE, 0 );	
			}
		}
		/**/
		else if ( nChar == 'Q' )
		{
			pParentState->GetObjectInfoCollector()->Trace();			
		}
	}
}


void CMapObjectAddState::OnSetFocus( CWnd* pNewWnd )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		if ( pParentState->InsertObjectSetFocus( pNewWnd ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnKillFocus( CWnd* pOldWnd )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		if ( pParentState->InsertObjectKillFocus( pOldWnd ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnMouseMove( nFlags, rMousePoint );
		if ( pParentState->InsertObjectMouseMove( nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
		if ( pParentState->InsertObjectLButtonDown( nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnLButtonUp( nFlags, rMousePoint );
		if ( pParentState->InsertObjectLButtonUp( nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnLButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnLButtonDblClk( nFlags, rMousePoint );
		if ( pParentState->InsertObjectLButtonDblClk( nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnRButtonDown( nFlags, rMousePoint );
		if ( pParentState->InsertObjectRButtonDown( nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnRButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnRButtonUp( nFlags, rMousePoint );
		if ( pParentState->InsertObjectRButtonUp( nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnRButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnRButtonDblClk( nFlags, rMousePoint );
		if ( pParentState->InsertObjectRButtonDblClk( nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnMButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnMButtonDown( nFlags, rMousePoint );
		if ( pParentState->InsertObjectMButtonDown( nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnMButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnMButtonUp( nFlags, rMousePoint );
		if ( pParentState->InsertObjectMButtonUp( nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnMButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnMButtonDblClk( nFlags, rMousePoint );
		if ( pParentState->InsertObjectMButtonDblClk( nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( pParentState->CanEdit() )
	{
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnKeyDown( nChar, nRepCnt, nFlags );
		/**
		if ( nChar == VK_INSERT )
		{
			SObjectSet objectSet;
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
			{
				::SetCursor( ::LoadCursor( 0, IDC_ARROW ) );
				pParentState->InsertObjectLeave();
				pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
				ICommandHandlerContainer* pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
				pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE, ID_MIMO_SWITCH_MULTI_STATE, reinterpret_cast<uint32_t>( &( objectSet.szObjectTypeName ) ) );
				pCommandHandlerContainer->HandleCommand( CHID_MAPINFO_MAPOBJECT_STATE, ID_MIMO_SWITCH_ADD_STATE, 0 );	
				return;
			}
		}
		/**/
		if ( nChar == 'Q' )
		{
			pParentState->GetObjectInfoCollector()->Trace();			
			return;
		}
		if ( pParentState->InsertObjectKeyDown( nChar, nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectAddState::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( pParentState->CanEdit() )
	{
		/**
		if ( nChar == VK_INSERT )
		{
			return;	
		}
		/**/
		if ( nChar == 'Q' )
		{
			return;	
		}
		//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		pParentState->pStoreInputState->OnKeyUp( nChar, nRepCnt, nFlags );
		if ( pParentState->InsertObjectKeyUp( nChar, nFlags, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			pParentState->InsertObjectLeave();
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectPasteState::Enter()
{
	if ( pParentState->CanEdit() )
	{
		if ( IEditorScene *pScene = EditorScene() )
		{
			NMapInfoEditor::SObjectEditInfo objectEditInfo;
			objectEditInfo.bRotateTo90Degree = pParentState->GetMapInfoEditor()->editorSettings.bRotateTo90Degree;
			objectEditInfo.bFitToGrid = pParentState->GetMapInfoEditor()->editorSettings.bFitToGrid;
			//
			pParentState->GetObjectInfoCollector()->ShowClipboard( &objectEditInfo,
																														 pParentState->pStoreInputState->lastEventInfo.vTerrainPos,
																														 false,
																														 false,
																														 false,
																														 pScene,
																														 pParentState->GetMapInfoEditor()->GetViewManipulator() ); 
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}	
}


void CMapObjectPasteState::Leave()
{
	if ( pParentState->CanEdit() )
	{
		if ( IEditorScene *pScene = EditorScene() )
		{
			pParentState->GetObjectInfoCollector()->HideClipboard( pScene, pParentState->GetMapInfoEditor()->GetViewManipulator() );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectPasteState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnMouseMove( nFlags, rMousePoint );
		if ( IEditorScene *pScene = EditorScene() )
		{
			NMapInfoEditor::SObjectEditInfo objectEditInfo;
			objectEditInfo.bRotateTo90Degree = pParentState->GetMapInfoEditor()->editorSettings.bRotateTo90Degree;
			objectEditInfo.bFitToGrid = pParentState->GetMapInfoEditor()->editorSettings.bFitToGrid;
			//
			pParentState->GetObjectInfoCollector()->MoveClipboard( &objectEditInfo,
																														 pParentState->pStoreInputState->lastEventInfo.vTerrainPos,
																														 false,
																														 false,
																														 false,
																														 pScene,
																														 pParentState->GetMapInfoEditor()->GetViewManipulator() ); 
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CMapObjectPasteState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonUp( nFlags, rMousePoint );
		if ( IEditorScene *pScene = EditorScene() )
		{
			CPtr<CObjectBaseController> pObjectController = pParentState->GetMapInfoEditor()->CreateController();
			//
			NMapInfoEditor::SObjectEditInfo objectEditInfo;
			objectEditInfo.bRotateTo90Degree = pParentState->GetMapInfoEditor()->editorSettings.bRotateTo90Degree;
			objectEditInfo.bFitToGrid = pParentState->GetMapInfoEditor()->editorSettings.bFitToGrid;
			//
			if ( pParentState->GetObjectInfoCollector()->PasteClipboard( &objectEditInfo,
																																	 pParentState->pStoreInputState->lastEventInfo.vTerrainPos,
																																	 false, false, false,
																																	 true, pScene,
																																	 true, pObjectController,pParentState->GetMapInfoEditor()->GetViewManipulator() ) )
		 {
				pObjectController->Redo( false, true, pParentState->GetMapInfoEditor() );
				Singleton<IControllerContainer>()->Add( pObjectController );
			}
		}			
		pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
	}
}


void CMapObjectPasteState::OnRButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnRButtonUp( nFlags, rMousePoint );
		pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
	}
}


void CMapObjectPasteState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnKeyDown( nChar, nRepCnt, nFlags );
		//
		if ( nChar == VK_ESCAPE )
		{
			pParentState->SetActiveInputState( CMapObjectState::IS_SELECT, true, false );
		}
	}
}


int CMapObjectState::GetNextSceneID( int nSceneID, list<int> *pSceneIDList )
{
	if ( ( pSceneIDList != 0 ) && ( !pSceneIDList->empty() ) )
	{
		pSceneIDList->sort();
		list<int>::iterator itSceneID = find( pSceneIDList->begin(), pSceneIDList->end(), nSceneID );
		if ( itSceneID != pSceneIDList->end() )
		{
			++itSceneID;
			if ( itSceneID == pSceneIDList->end() )
			{
				return pSceneIDList->front();
			}
			else
			{
				return *itSceneID;
			}
		}
	}
	return nSceneID;
}


void CMapObjectState::ClearSelection()
{
	nSelectedSceneID = INVALID_NODE_ID;
	selector.Clear();
	GetMapInfoEditor()->objectInfoCollector.ClearSelection();
}


NMapInfoEditor::SObjectInfoCollector* CMapObjectState::GetObjectInfoCollector()
{ 
	return &( pParentState->pMapInfoEditor->objectInfoCollector );
}


CMapInfoEditor* CMapObjectState::GetMapInfoEditor()
{ 
	return pParentState->pMapInfoEditor;
}


void CMapObjectState::OnSetFocus( CWnd* pNewWnd )
{
	CMultiInputState::OnSetFocus( pNewWnd );
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CMapObjectState::Enter()
{
	if ( GetActiveInputStateIndex() == IS_ADD )
	{
		InsertObjectLeave();
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
	}
	//
	ClearSelection();
	//
	SetActiveInputState( IS_SELECT, true, false );
	::SetCursor( ::LoadCursor( 0, IDC_ARROW ) );
	//
	if ( pParentState )
	{
		pStoreInputState->SetSizes( CTPoint<int>( pParentState->pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH,
																							pParentState->pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH ),
																true );
	}
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_MAPOBJECT_STATE, this );
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_GET_FOCUS, 0 ) )
	{
		Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CMapObjectState::Leave()
{
	if ( GetActiveInputStateIndex() == IS_ADD )
	{
		InsertObjectLeave();
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_WINDOW, ID_MIMO_CLEAR_SELECTION, 0 );
	}
	//
	sceneDrawTool.Clear();
	//
	ClearSelection();
	//
	SetActiveInputState( IS_SELECT, true, false );
	::SetCursor( ::LoadCursor( 0, IDC_ARROW ) );

	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_MAPOBJECT_STATE );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );

	// Не надо
	//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	GetObjectInfoCollector()->ClearShootAreas();
	Singleton<IMainFrameContainer>()->Get()->SetStatusBarText( 1, "" );
}


void CMapObjectState::Draw( CPaintDC *pPaintDC )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ( GetActiveInputStateIndex() == IS_SELECT ) || ( GetActiveInputStateIndex() == IS_EDIT ) || ( GetActiveInputStateIndex() == IS_PASTE ) )
		{
			if ( selector.IsValid() )
			{
				if ( selector.IsTerrainSelector() )
				{
					list<CVec3> pointList;
					pointList.push_back( CVec3( selector.vTerrainPos0.x, selector.vTerrainPos0.y, 0.0f ) );
					pointList.push_back( CVec3( selector.vTerrainPos1.x, selector.vTerrainPos0.y, 0.0f ) );
					pointList.push_back( CVec3( selector.vTerrainPos1.x, selector.vTerrainPos1.y, 0.0f ) );
					pointList.push_back( CVec3( selector.vTerrainPos0.x, selector.vTerrainPos1.y, 0.0f ) );
					for ( list<CVec3>::iterator itPoint = pointList.begin(); itPoint != pointList.end(); ++itPoint )
					{
						UpdateTerrainHeight( &( *itPoint ) );
					}
					sceneDrawTool.DrawPolyline( pointList, SELECTION_LINE_COLOR, true, false );
				}
			}
			//
			GetObjectInfoCollector()->DrawSelection( &sceneDrawTool );
			//
			GetObjectInfoCollector()->ClearShootAreas();
			if ( GetMapInfoEditor()->editorSettings.bDrawShootAreas )
			{
				GetObjectInfoCollector()->DrawShootAreas( &sceneDrawTool );
			}
		}
		else if ( GetActiveInputStateIndex() == IS_ADD )
		{
			InsertObjectDraw( pPaintDC );
		}
		//
		if ( IsDrawSceneDrawTool() )
		{
			sceneDrawTool.Draw();
		}
	}
}


void CMapObjectState::PostDraw( class CPaintDC *pPaintDC )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( selector.IsValid() )
		{
			if ( !selector.IsTerrainSelector() )
			{
				// нарисовать рамку
				CRect selectorRect( selector.frameRect.minx, selector.frameRect.miny, selector.frameRect.maxx, selector.frameRect.maxy );
				selectorRect.NormalizeRect();
				//
				CBrush solidBrush;
				solidBrush.CreateSolidBrush( GetBGRColorFromARGBColor( SELECTION_LINE_COLOR ) ); 
				pPaintDC->FrameRect( &selectorRect, &solidBrush );
			}
		}
	}
}


bool CMapObjectState::CanEdit() 
{
	return ( ( GetObjectInfoCollector() != 0 ) && ( GetMapInfoEditor()->GetViewManipulator() != 0 ) && GetParentState()->GetEditParameters() );
}


void CMapObjectState::SetSelectionHeightsToZero( bool bSave )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		NMapInfoEditor::SObjectEditInfo objectEditInfo;
		objectEditInfo.bRotateTo90Degree = GetMapInfoEditor()->editorSettings.bRotateTo90Degree;
		objectEditInfo.bFitToGrid = GetMapInfoEditor()->editorSettings.bFitToGrid;
		//
		CPtr<CObjectBaseController> pObjectController = 0;
		if ( bSave )
		{
			pObjectController = GetMapInfoEditor()->CreateController();
		}
		GetObjectInfoCollector()->MoveSelection( &objectEditInfo, VNULL3, false, false, true,
																						 true, pScene,
																						 bSave, pObjectController, GetMapInfoEditor()->GetViewManipulator() );
		if ( bSave && pObjectController )
		{
			pObjectController->Redo( false, true, GetMapInfoEditor() );
			Singleton<IControllerContainer>()->Add( pObjectController );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CMapObjectState::InsertSelectionLink( UINT nLinkToSceneID )
{
	if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
	{
		GetObjectInfoCollector()->InsertSelectionLink( nLinkToSceneID, pObjectController, GetMapInfoEditor()->GetViewManipulator() );
		pObjectController->Redo( false, true, GetMapInfoEditor() );
		Singleton<IControllerContainer>()->Add( pObjectController );
	}
}

void CMapObjectState::RemoveSelectionLinkTo()
{
	if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
	{
		GetObjectInfoCollector()->RemoveSelectionLinkTo( pObjectController,
																										 GetMapInfoEditor()->GetViewManipulator() );
		pObjectController->Redo( false, true, GetMapInfoEditor() );
		Singleton<IControllerContainer>()->Add( pObjectController );
	}
}


void CMapObjectState::RemoveSelectionLinks()
{
	if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
	{
		GetObjectInfoCollector()->RemoveSelectionLinks( pObjectController,
																										GetMapInfoEditor()->GetViewManipulator() );
		pObjectController->Redo( false, true, GetMapInfoEditor() );
		Singleton<IControllerContainer>()->Add( pObjectController );
	}
}

void CMapObjectState::RemoveSelection()
{
	CString strMessage;
	AfxSetResourceHandle( theEDB2M1Instance );
	strMessage.LoadString( IDS_MIMO_DELETE_OBJECTS_MESSAGE );
	AfxSetResourceHandle( AfxGetInstanceHandle() );
	if ( ::MessageBox( Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
	{
		if ( IEditorScene *pScene = EditorScene() )
		{
			if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
			{
				GetObjectInfoCollector()->RemoveSelection( pScene,
																									 pObjectController,
																									 GetMapInfoEditor()->GetViewManipulator() );
				pObjectController->Redo( false, true, GetMapInfoEditor() );
				Singleton<IControllerContainer>()->Add( pObjectController );
			}
			ClearSelection();
		}
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_UNIT_START_CMD_STATE, ID_UNIT_START_CMD_REFRESH_WINDOW, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CMapObjectState::MoveSelection( const CVec3 &rvPosition, bool bExactPosition, bool bIgnoreDifference, bool bSave )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		NMapInfoEditor::SObjectEditInfo objectEditInfo;
		objectEditInfo.bRotateTo90Degree = GetMapInfoEditor()->editorSettings.bRotateTo90Degree;
		objectEditInfo.bFitToGrid = GetMapInfoEditor()->editorSettings.bFitToGrid;
		//
		CPtr<CObjectBaseController> pObjectController = 0;
		if ( bSave )
		{
			pObjectController = GetMapInfoEditor()->CreateController();
		}

		GetObjectInfoCollector()->MoveSelection( &objectEditInfo, rvPosition, bExactPosition, bIgnoreDifference, false,
																						 true, pScene,
																						 bSave, pObjectController, GetMapInfoEditor()->GetViewManipulator() );
		if ( bSave && pObjectController )
		{
			pObjectController->Redo( false, true, GetMapInfoEditor() );
			Singleton<IControllerContainer>()->Add( pObjectController );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CMapObjectState::RotateSelection( const CVec3 &rvPosition, bool bExactDirection, bool bIgnoreDifference, bool bSave )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		NMapInfoEditor::SObjectEditInfo objectEditInfo;
		objectEditInfo.bRotateTo90Degree = GetMapInfoEditor()->editorSettings.bRotateTo90Degree;
		objectEditInfo.bFitToGrid = GetMapInfoEditor()->editorSettings.bFitToGrid;
		//
		CPtr<CObjectBaseController> pObjectController = 0;
		if ( bSave )
		{
			pObjectController = GetMapInfoEditor()->CreateController();
		}
		float fSelectionDirection = GetPolarAngle( rvPosition - GetObjectInfoCollector()->GetSelectionPosition() ) - FP_PI2;
		if ( fSelectionDirection < 0.0f )
		{ 
			fSelectionDirection += FP_2PI;
		}
		GetObjectInfoCollector()->RotateSelection( &objectEditInfo, fSelectionDirection, bExactDirection, bIgnoreDifference,
																							 true, pScene,
																							 bSave, pObjectController, GetMapInfoEditor()->GetViewManipulator() );
		if ( bSave && pObjectController )
		{
			pObjectController->Redo( false, true, GetMapInfoEditor() );
			Singleton<IControllerContainer>()->Add( pObjectController );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_MAPOBJECT_MULTI_STATE,	ID_UPDATE_EDIT_PARAMETERS, MIMOSEP_DIRECTION );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CMapObjectState::SwitchToAddState()
{
	ClearSelection();
	SetActiveInputState( CMapObjectState::IS_ADD, true, false );
	InsertObjectEnter();
}


bool CMapObjectState::HandleCommand( UINT nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
		case ID_MIMO_SWITCH_ADD_STATE:
		{	
			SwitchToAddState();
			return true;
		}
		case ID_SELECTION_COPY:
		{
			if ( !GetObjectInfoCollector()->IsSelectionEmpty() )
			{
				GetObjectInfoCollector()->CopyClipboard();
			}
			return true;
		}
		case ID_SELECTION_PASTE:
		{
			SetActiveInputState( CMapObjectState::IS_PASTE, true, false );
			return true;
		}
		case ID_SELECTION_CLEAR:
		{
			if ( !GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				RemoveSelection();
			}
			return true;
		}
		case ID_SELECTION_PROPERTIES:
		{
			if ( !GetObjectInfoCollector()->IsSelectionEmpty() )
			{	
				GetObjectInfoCollector()->ShowSelectionPropertyManipulator( GetMapInfoEditor()->GetViewManipulator(), GetMapInfoEditor()->GetObjectSet() );
			}
			return true;
		}
		default:
			return false;
	}
	return false;
}


bool CMapObjectState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CMapObjectState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CMapObjectState::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_MIMO_SWITCH_ADD_STATE:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		case ID_SELECTION_COPY:
		{
			( *pbEnable ) = !( GetObjectInfoCollector()->IsSelectionEmpty() );
			( *pbCheck ) = false;
			return true;
		}
		case ID_SELECTION_PASTE:
		{
			( *pbEnable ) = !( GetObjectInfoCollector()->IsClipboardEmpty() );
			( *pbCheck ) = false;
			return true;
		}
		case ID_SELECTION_CLEAR:
		{
			( *pbEnable ) = !( GetObjectInfoCollector()->IsSelectionEmpty() );
			( *pbCheck ) = false;
			return true;
		}
		case ID_SELECTION_PROPERTIES:
		{
			( *pbEnable ) = !( GetObjectInfoCollector()->IsSelectionEmpty() );
			( *pbCheck ) = false;
			return true;
		}
		default:
			return false;
	}
	return false;
}

// basement storage  


