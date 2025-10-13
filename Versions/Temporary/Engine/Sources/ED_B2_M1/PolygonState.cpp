#include "stdafx.h"

#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "Misc/2Darray.h"
#include "Misc/planegeometry.h"

#include "MapEditorLib/Interface_CommandHandler.h"
#include "PolygonState.h"

#include <cstdint>

#include <zconf.h>

const float	CPolygonState::CONTROL_POINT_RADIUS	= AI_TILE_SIZE / 1.0f;
const int		CPolygonState::CONTROL_POINT_PARTS	= 8;
const float	CPolygonState::CENTER_POINT_RADIUS	= AI_TILE_SIZE / 1.5f;
const uint32_t	CPolygonState::CONTROL_POINT_COLOR	= 0xFFFF4040;
const uint32_t	CPolygonState::CONTROL_LINE_COLOR		= 0xFFFF4040;

void CPolygonSelectState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
		//
		pParentState->pickPolygonIDList.clear();
		pParentState->nSelectedIndex = INVALID_NODE_ID;
		//
		pParentState->PickPolygon( pParentState->pStoreInputState->lastEventInfo.vTerrainPos, &( pParentState->pickPolygonIDList ) );
		// Выделяем верхний Polygon
		if ( !pParentState->pickPolygonIDList.empty() )
		{
			pParentState->nSelectedIndex = 0;
		}
		// Начинаем строить новый Polygon ( добавляем первую точку )
		else if ( pParentState->CanInsertPolygon() )
		{
			pParentState->nSelectedControlPoint = INVALID_NODE_ID;
			pParentState->controlPointList.clear();
			pParentState->controlPointList.push_back( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
			pParentState->SetActiveInputState( CPolygonState::IS_ADD, true, false );
		}
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CPolygonSelectState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonUp( nFlags, rMousePoint );
		//
		// Активируем STATE редактирования Polygon
		if ( !pParentState->pickPolygonIDList.empty() )
		{
			pParentState->ValidateSelectedIndex();
			pParentState->UpdatePolygon( pParentState->GetSelectedPolygonID(), CPolygonState::UT_START );
			// Запоманаем первоначальное расположение контрольных точек
			if ( CPolygonState::CControlPointList *pControlPointList = pParentState->GetControlPoints( pParentState->GetSelectedPolygonID() ) )
			{
				pParentState->controlPointList = ( *pControlPointList );
			}
			else
			{
				pParentState->controlPointList.clear();
			}
			//
			pParentState->SetActiveInputState( CPolygonState::IS_EDIT, true, false );
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CPolygonSelectState::OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnRButtonDown( nFlags, rMousePoint );
		//
		// Переключаемся между Polygon ( если они один над другим )
		if ( !pParentState->pickPolygonIDList.empty() )
		{
			++( pParentState->nSelectedIndex );
			pParentState->ValidateSelectedIndex();
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CPolygonEditState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnMouseMove( nFlags, rMousePoint );
		//
		if ( ( pParentState->GetSelectedPolygonID() != INVALID_NODE_ID ) &&
				 ( nFlags & MK_LBUTTON ) &&
				 ( pParentState->nSelectedControlPoint != INVALID_NODE_ID ) && 
				 pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].IsValid() )
		{
			if ( CPolygonState::CControlPointList *pControlPointList = pParentState->GetControlPoints( pParentState->GetSelectedPolygonID() ) )
			{
				CPolygonState::EMoveType eMoveType = pParentState->GetMoveType();
				switch( eMoveType )
				{
					case CPolygonState::MT_SINGLE:
					default:
					{
						( *pControlPointList )[pParentState->nSelectedControlPoint] += pParentState->pStoreInputState->lastEventInfo.vTerrainPos - 
																																					 pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].vTerrainPos;
						break;
					}
					case CPolygonState::MT_MULTI:
					{
						if ( ( nFlags & MK_CONTROL ) == 0 )
						{
							for ( int nControlPointIndex = pParentState->nSelectedControlPoint; nControlPointIndex < pControlPointList->size(); ++nControlPointIndex )
							{
								( *pControlPointList )[nControlPointIndex] += pParentState->pStoreInputState->lastEventInfo.vTerrainPos - 
																															pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].vTerrainPos;
							}
						}
						else
						{
							for ( int nControlPointIndex = 0; nControlPointIndex <= pParentState->nSelectedControlPoint; ++nControlPointIndex )
							{
								( *pControlPointList )[nControlPointIndex] += pParentState->pStoreInputState->lastEventInfo.vTerrainPos - 
																															pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].vTerrainPos;
							}
						}
						break;
					}
					case CPolygonState::MT_ALL:
					{
						for ( int nControlPointIndex = 0; nControlPointIndex < pControlPointList->size(); ++nControlPointIndex )
						{
							( *pControlPointList )[nControlPointIndex] += pParentState->pStoreInputState->lastEventInfo.vTerrainPos - 
																														pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].vTerrainPos;
						}
						break;
					}
				}
				// Необходимо обновить eventInfoList[CStoreInputState::ISE_LBUTTONDOWN];
				pParentState->pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
			}
			pParentState->UpdatePolygon( pParentState->GetSelectedPolygonID(), CPolygonState::UT_CONTINUE_MOVE );
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return;
		}
	}
}


void CPolygonEditState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
		//
		pParentState->nSelectedControlPoint = INVALID_NODE_ID;
		if ( pParentState->GetSelectedPolygonID() != INVALID_NODE_ID )
		{
			if ( CPolygonState::CControlPointList *pControlPointList = pParentState->GetControlPoints( pParentState->GetSelectedPolygonID() ) )
			{
				for ( int nControlPointIndex = 0; nControlPointIndex < pControlPointList->size(); ++nControlPointIndex )
				{
					CVec3 vDifference = pParentState->pStoreInputState->lastEventInfo.vTerrainPos - ( *pControlPointList )[nControlPointIndex];
					vDifference.z = 0.0f;
					if ( fabs( vDifference ) <= CPolygonState::CONTROL_POINT_RADIUS )
					{
						pParentState->nSelectedControlPoint = nControlPointIndex;
						break;
					}
				}
				if ( pParentState->nSelectedControlPoint != INVALID_NODE_ID )
				{
					pParentState->UpdatePolygon( pParentState->GetSelectedPolygonID(), CPolygonState::UT_START_MOVE );
					//
					Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
					return;
				}
			}
		}
		// Необходимо выделить другой VSO если мы попали мимо текущего выделенного VSO
		const CPolygonState::CPolygonIDList pickPolygonIDList = pParentState->pickPolygonIDList;
		const int nSelectedIndex = pParentState->nSelectedIndex;
		const int nSelectedPolygonID = pParentState->GetSelectedPolygonID();
		//
		pParentState->pickPolygonIDList.clear();
		pParentState->nSelectedIndex = INVALID_NODE_ID;
		//
		pParentState->PickPolygon( pParentState->pStoreInputState->lastEventInfo.vTerrainPos, &( pParentState->pickPolygonIDList ) );
		bool bReturnOldPolygon = true;
		if ( !pParentState->pickPolygonIDList.empty() )
		{
			bool bOldPolygonNotPresent = true;
			for ( int nSelectedPolygonIndex = 0; nSelectedPolygonIndex < pParentState->pickPolygonIDList.size(); ++nSelectedPolygonIndex )
			{					
				if ( nSelectedPolygonID == pParentState->pickPolygonIDList[nSelectedPolygonIndex] )
				{
					pParentState->nSelectedIndex = nSelectedPolygonIndex;
					bOldPolygonNotPresent = false;
					break;
				}
			}	
			// Выделяем другой - первый обьект
			if ( bOldPolygonNotPresent )
			{
				pParentState->nSelectedIndex = 0;
				pParentState->SetActiveInputState( CPolygonState::IS_SELECT, true, false );
				bReturnOldPolygon = false;
			}
		}
		// нет других обьектов ( или есть старый выделенный обьект )- возвращаем старый
		if ( bReturnOldPolygon )
		{
			pParentState->pickPolygonIDList = pickPolygonIDList;
			pParentState->nSelectedIndex = nSelectedIndex;
		}
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CPolygonEditState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonUp( nFlags, rMousePoint );
		//
		if ( ( pParentState->GetSelectedPolygonID() != INVALID_NODE_ID ) &&
				 ( pParentState->nSelectedControlPoint != INVALID_NODE_ID ) )
		{
			pParentState->UpdatePolygon( pParentState->GetSelectedPolygonID(), CPolygonState::UT_FINISH_MOVE );
		}
		pParentState->nSelectedControlPoint = INVALID_NODE_ID;
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CPolygonEditState::OnLButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonDblClk( nFlags, rMousePoint );
		//
		if ( ( pParentState->GetSelectedPolygonID() != INVALID_NODE_ID ) &&
				 ( pParentState->nSelectedControlPoint != INVALID_NODE_ID ) )
		{
			pParentState->UpdatePolygon( pParentState->GetSelectedPolygonID(), CPolygonState::UT_FINISH );
		}
		pParentState->nSelectedControlPoint = INVALID_NODE_ID;
		pParentState->controlPointList.clear();
		pParentState->pickPolygonIDList.clear();
		pParentState->nSelectedIndex = INVALID_NODE_ID;
		pParentState->SetActiveInputState( CPolygonState::IS_SELECT, true, false );
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CPolygonEditState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnKeyDown( nChar, nRepCnt, nFlags );
		//
		if ( nChar == VK_INSERT ) 
		{
			if ( ( pParentState->GetSelectedPolygonID() != INVALID_NODE_ID ) &&
					 ( pParentState->nSelectedControlPoint != INVALID_NODE_ID ) )
			{
				if ( CPolygonState::CControlPointList *pControlPointList = pParentState->GetControlPoints( pParentState->GetSelectedPolygonID() ) )
				{
					int nShift = ( ( nFlags & MK_CONTROL ) == 0 ) ? 1 : -1;
					if ( pParentState->nSelectedControlPoint == ( pControlPointList->size() - 1 ) )
					{
						nShift = -1;
					}
					else if ( pParentState->nSelectedControlPoint == 0 )
					{
						nShift = 1;
					}
					if ( ( ( pParentState->nSelectedControlPoint + nShift ) > ( pControlPointList->size() - 1 ) ) ||
							 ( ( pParentState->nSelectedControlPoint + nShift ) < 0 ) )
					{
						nShift = 0;
					}
					if ( nShift != 0 )
					{
						const CVec3 vBegin = ( *pControlPointList )[pParentState->nSelectedControlPoint];
						const CVec3 vEnd = ( *pControlPointList )[pParentState->nSelectedControlPoint + nShift];
						const CVec3 vNewControlPoint = ( vBegin + vEnd ) / 2.0f;
						pControlPointList->insert( pControlPointList->begin() + ( pParentState->nSelectedControlPoint + nShift ), vNewControlPoint );
						pParentState->UpdatePolygon( pParentState->GetSelectedPolygonID(), CPolygonState::UT_CHANGE_POINT_NUMBER );
						//
						Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
					}
					return;
				}
			}
		}
		else if ( nChar == VK_DELETE ) 
		{
			pParentState->OnDelete();
		}
		else if ( ( nChar == VK_RETURN ) || ( nChar == VK_SPACE ) || ( nChar == VK_ESCAPE ) )
		{
			if ( pParentState->GetSelectedPolygonID() != INVALID_NODE_ID )
			{
				if ( nChar == VK_ESCAPE )
				{
					if ( CPolygonState::CControlPointList *pControlPointList = pParentState->GetControlPoints( pParentState->GetSelectedPolygonID() ) )
					{
						( *pControlPointList ) = pParentState->controlPointList;
					}
				}
				pParentState->UpdatePolygon( pParentState->GetSelectedPolygonID(), ( nChar != VK_ESCAPE ) ? CPolygonState::UT_FINISH : CPolygonState::UT_CANCEL );
			}
			//			
			pParentState->nSelectedControlPoint = INVALID_NODE_ID;
			pParentState->controlPointList.clear();
			pParentState->pickPolygonIDList.clear();
			pParentState->nSelectedIndex = INVALID_NODE_ID;
			pParentState->SetActiveInputState( CPolygonState::IS_SELECT, true, false );
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return;
		}
	}
}


void CPolygonAddState::InsertPolygon()
{
	UINT nNewPolygonID = INVALID_NODE_ID;
	if ( pParentState->PrepareControlPoints( &( pParentState->controlPointList ) ) )
	{
		bool bPolygonIsValid = true;
		int nMinCount = INVALID_NODE_ID;
		int nMaxCount = INVALID_NODE_ID;
		pParentState->GetBounds( &nMinCount, &nMaxCount );
		if ( ( nMinCount != INVALID_NODE_ID ) && ( pParentState->controlPointList.size() < nMinCount ) )
		{
			bPolygonIsValid = false;
		}
		else if ( ( nMaxCount != INVALID_NODE_ID ) && ( pParentState->controlPointList.size() > nMaxCount ) )
		{
			bPolygonIsValid = false;
		}
		if ( bPolygonIsValid )
		{
			nNewPolygonID = pParentState->InsertPolygon( pParentState->controlPointList );
		}
	}
	pParentState->nSelectedControlPoint = INVALID_NODE_ID;
	pParentState->controlPointList.clear();
	pParentState->pickPolygonIDList.clear();
	pParentState->nSelectedIndex = INVALID_NODE_ID;
	//
	if ( nNewPolygonID != INVALID_NODE_ID )
	{
		pParentState->pickPolygonIDList.push_back( nNewPolygonID );
		pParentState->nSelectedIndex = 0;
		//
		// Запоманаем первоначальное расположение контрольных точек
		if ( CPolygonState::CControlPointList *pControlPointList = pParentState->GetControlPoints( pParentState->GetSelectedPolygonID() ) )
		{
			pParentState->controlPointList = ( *pControlPointList );
		}
		else
		{
			pParentState->controlPointList.clear();
		}
		if ( pParentState->SkipEnterAfterInsert() )
		{
			pParentState->UpdatePolygon( pParentState->GetSelectedPolygonID(), CPolygonState::UT_FINISH );
			pParentState->nSelectedControlPoint = INVALID_NODE_ID;
			pParentState->controlPointList.clear();
			pParentState->pickPolygonIDList.clear();
			pParentState->nSelectedIndex = INVALID_NODE_ID;
			pParentState->SetActiveInputState( CPolygonState::IS_SELECT, true, false );
		}
		else
		{
			pParentState->UpdatePolygon( pParentState->GetSelectedPolygonID(), CPolygonState::UT_START );
			pParentState->SetActiveInputState( CPolygonState::IS_EDIT, true, false );
		}
	}
	else
	{
		pParentState->SetActiveInputState( CPolygonState::IS_SELECT, true, false );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CPolygonAddState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnMouseMove( nFlags, rMousePoint );
		//
		pParentState->controlPointList.pop_back();
		pParentState->controlPointList.push_back( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CPolygonAddState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonUp( nFlags, rMousePoint );
		//
		int nMinCount = INVALID_NODE_ID;
		int nMaxCount = INVALID_NODE_ID;
		pParentState->GetBounds( &nMinCount, &nMaxCount );
		if ( ( nMaxCount != INVALID_NODE_ID ) && ( pParentState->controlPointList.size() >= nMaxCount ) )
		{
			InsertPolygon();
			return;
		}
		pParentState->controlPointList.push_back( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CPolygonAddState::OnRButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnRButtonUp( nFlags, rMousePoint );

		if ( pParentState->controlPointList.size() > 1 )
		{
			pParentState->controlPointList.pop_back();
			pParentState->controlPointList.pop_back();
			pParentState->controlPointList.push_back( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
		}
		else
		{
			pParentState->controlPointList.clear();
			pParentState->pickPolygonIDList.clear();
			pParentState->nSelectedIndex = INVALID_NODE_ID;
			//
			pParentState->SetActiveInputState( CPolygonState::IS_SELECT, true, false );
		}
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CPolygonAddState::OnLButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonDblClk( nFlags, rMousePoint );
		InsertPolygon();
	}
}


void CPolygonAddState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnKeyDown( nChar, nRepCnt, nFlags );
		//
		if ( nChar == VK_RETURN )
		{
			InsertPolygon();
		}
		else if ( nChar == VK_SPACE )
		{
			pParentState->nSelectedControlPoint = INVALID_NODE_ID;
			pParentState->controlPointList.clear();
			pParentState->pickPolygonIDList.clear();
			pParentState->nSelectedIndex = INVALID_NODE_ID;
			//
			pParentState->SetActiveInputState( CPolygonState::IS_SELECT, true, false );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CPolygonState::Enter()
{
	nSelectedControlPoint = INVALID_NODE_ID;
	controlPointList.clear();
	pickPolygonIDList.clear();
	nSelectedIndex = INVALID_NODE_ID;
	//
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_GET_FOCUS, 0 ) )
	{
		Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
	}
	//
	SetActiveInputState( IS_SELECT, true, false );
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CPolygonState::Leave()
{
	sceneDrawTool.Clear();
	//
	if ( GetActiveInputStateIndex() == IS_EDIT )
	{
		if ( GetSelectedPolygonID() != INVALID_NODE_ID )
		{
			if ( CPolygonState::CControlPointList *pControlPointList = GetControlPoints( GetSelectedPolygonID() ) )
			{
				( *pControlPointList ) = controlPointList;
			}
			UpdatePolygon( GetSelectedPolygonID(), UT_CANCEL );
		}
	}
	//
	nSelectedControlPoint = INVALID_NODE_ID;
	controlPointList.clear();
	pickPolygonIDList.clear();
	nSelectedIndex = INVALID_NODE_ID;
	//
	SetActiveInputState( IS_SELECT, true, false );
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
	// Не надо
	//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CPolygonState::Draw( CPaintDC *pPaintDC )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ( GetActiveInputStateIndex() == IS_SELECT ) || ( GetActiveInputStateIndex() == IS_EDIT ) )
		{
			if ( GetSelectedPolygonID() != INVALID_NODE_ID )
			{
				if ( CPolygonState::CControlPointList *pControlPointList = GetControlPoints( GetSelectedPolygonID() ) )
				{
					list<CVec3> pointList;
					for ( int nControlPointIndex = 0; nControlPointIndex < pControlPointList->size(); ++nControlPointIndex )
					{
						CVec3 vControlPoint = ( *pControlPointList )[nControlPointIndex];
						UpdateTerrainHeight( &vControlPoint );
						//
						sceneDrawTool.DrawCircle( vControlPoint, CONTROL_POINT_RADIUS, CONTROL_POINT_PARTS, CONTROL_POINT_COLOR, false );
						pointList.push_back( vControlPoint );
					}
					sceneDrawTool.DrawPolyline( pointList, CONTROL_LINE_COLOR, IsClosedPolygon(), false );
					//
					if ( nSelectedControlPoint != INVALID_NODE_ID )
					{
						if ( ( nSelectedControlPoint >= 0 ) && ( nSelectedControlPoint < pControlPointList->size() ) )
						{
							float fRadius = CONTROL_POINT_RADIUS - 1.0f;
							CVec3 vControlPoint = ( *pControlPointList )[nSelectedControlPoint];
							UpdateTerrainHeight( &vControlPoint );
							//
							while( fRadius > 0.0f )
							{
								sceneDrawTool.DrawCircle( vControlPoint, fRadius, CONTROL_POINT_PARTS, CONTROL_POINT_COLOR, false );
								fRadius -= 2.0f;
							}
						}
					}
				}
			}
		}
		else if ( GetActiveInputStateIndex() == IS_ADD )
		{
			list<CVec3> pointList;
			for ( CControlPointList::const_iterator itControlPoint = controlPointList.begin(); itControlPoint != controlPointList.end(); ++itControlPoint )
			{
				CVec3 vPoint = *itControlPoint;
				UpdateTerrainHeight( &vPoint );
				//
				sceneDrawTool.DrawCircle( vPoint, CONTROL_POINT_RADIUS, CONTROL_POINT_PARTS, CONTROL_POINT_COLOR, false );
				pointList.push_back( vPoint );
			}	
			sceneDrawTool.DrawPolyline( pointList, CONTROL_POINT_COLOR, IsClosedPolygon(), false );
		}
		//
		if ( IsDrawSceneDrawTool() )
		{
			sceneDrawTool.Draw();
		}
	}
}


void CPolygonState::OnDelete()
{
	if ( ( GetSelectedPolygonID() != INVALID_NODE_ID ) && ( nSelectedControlPoint != INVALID_NODE_ID ) )
	{
		int nMinCount = INVALID_NODE_ID;
		int nMaxCount = INVALID_NODE_ID;
		GetBounds( &nMinCount, &nMaxCount );
		//
		if ( CPolygonState::CControlPointList *pControlPointList = GetControlPoints( GetSelectedPolygonID() ) )
		{
			if ( nMinCount < pControlPointList->size() )
			{
				pControlPointList->erase( pControlPointList->begin() + nSelectedControlPoint );
				UpdatePolygon( GetSelectedPolygonID(), CPolygonState::UT_CHANGE_POINT_NUMBER );
				nSelectedControlPoint = INVALID_NODE_ID;
			}
			else
			{
				RemovePolygon( GetSelectedPolygonID() );
				nSelectedControlPoint = INVALID_NODE_ID;
				controlPointList.clear();
				pickPolygonIDList.clear();
				nSelectedIndex = INVALID_NODE_ID;
				SetActiveInputState( CPolygonState::IS_SELECT, true, false );
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
		return;
	}
	else if ( GetSelectedPolygonID() != INVALID_NODE_ID )
	{
		RemovePolygon( GetSelectedPolygonID() );
		nSelectedControlPoint = INVALID_NODE_ID;
		controlPointList.clear();
		pickPolygonIDList.clear();
		nSelectedIndex = INVALID_NODE_ID;
		SetActiveInputState( CPolygonState::IS_SELECT, true, false );
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		return;
	}
}


void CPolygonState::OnSetFocus( class CWnd* pNewWnd )
{
	CMultiInputState::OnSetFocus( pNewWnd );
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


bool CPolygonState::HandleCommand( UINT nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
		case ID_SELECTION_CLEAR:
		{
			OnDelete();
			return true;
		}
		default:
			break;
	}
	return false;
}


bool CPolygonState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CPolygonState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CPolygonState::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_SELECTION_CLEAR:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		default:
			break;
	}
	return false;
}


// basement storage  


