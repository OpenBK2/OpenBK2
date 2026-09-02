#include "stdafx.h"

#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/CommonEditorMethods.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "SceneB2/Scene.h"
#include "ResourceDefines.h"

#include "EditorMethods.h"
#include "MapInfoEditor.h"
#include "VSOMultiState.h"
#include "VSOState.h"
#include "ED_B2_M1Dll.h"

#include <cstdint>

namespace NExtraDraw
{
	void DrawVector( CSceneDrawTool *pSceneDrawTool, unsigned uColor, const std::vector<CVec3> &vPoints )
	{
		if ( vPoints.size() < 2 )
			return;

		std::vector<CVec3>::const_iterator itPoint = vPoints.begin();
		std::vector<CVec3>::const_iterator itPointPrev = itPoint;
		++itPoint;

		while ( 1 )
		{
			if ( itPoint == vPoints.end() )
				break;

			pSceneDrawTool->DrawLine( *itPoint, *itPointPrev, uColor, false );

			itPointPrev = itPoint;
			++itPoint;
		}
		pSceneDrawTool->DrawLine( (*vPoints.begin()), (*itPointPrev), uColor, false );
	}
	//
	void DrawArray( CSceneDrawTool *pSceneDrawTool, unsigned uColor, const CArray2D<float> &array, const float fTileSize )
	{
		const int nSizeX = array.GetSizeX();
		const int nSizeY = array.GetSizeY();
		if ( (nSizeX < 1) | (nSizeY < 1) )
			return;

		for ( int g = 0; g < nSizeY; ++g )
		{
			for ( int i = 0; i < nSizeX; ++i )
			{
				const CVec3 vDown( i*fTileSize, g*fTileSize, 0 );
				const CVec3 vUp = vDown + V3_AXIS_Z * Vis2AI( array[g][i] );

				pSceneDrawTool->DrawLine( vDown, vUp, uColor, false );
			}
		}
	}
};


void CVSOSelectState::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
		//
		pParentState->ClearPickVSOIDList();
		//
		pParentState->PickVSO( pParentState->pStoreInputState->lastEventInfo.vTerrainPos, &( pParentState->pickVSOIDList ) );
		// Выделяем верхний VSO
		if ( !pParentState->pickVSOIDList.empty() )
		{
			pParentState->nSelectedIndex = 0;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
		else if ( !pParentState->PickOtherVSO( nFlags, rMousePoint, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			if ( pParentState->CanInsertVSO() )
			{
				// Начинаем строить новый VSO ( добавляем первую точку )
				pParentState->PrepareInsertVSO();
				pParentState->ClearStartVSOPointList();
				pParentState->startVSOInstance.controlPoints.push_back( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
				pParentState->SetActiveInputState( CVSOState::IS_ADD, true, false );
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CVSOSelectState::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonUp( nFlags, rMousePoint );
		//
		// Активируем STATE редактирования VSO
		if ( !pParentState->pickVSOIDList.empty() )
		{
			pParentState->ValidateSelectedIndex();
			//
			pParentState->UpdateVSO( pParentState->GetSelectedVSOID(), CVSOState::UT_START, CVSOManager::SVSOSelection::ST_UNKNOWN, 0 );
			{
				NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 );
				pParentState->startVSOInstance.points = pSelectedVSO->points;
				pParentState->startVSOInstance.controlPoints = pSelectedVSO->controlPoints;
			}
			pParentState->SetActiveInputState( CVSOState::IS_EDIT, true, false );
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CVSOSelectState::OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnRButtonDown( nFlags, rMousePoint );
		//
		// Переключаемся между VSO ( если они один над другим )
		if ( !pParentState->pickVSOIDList.empty() )
		{
			++( pParentState->nSelectedIndex );
			pParentState->ValidateSelectedIndex();
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
		else
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_VSO_WINDOW, ID_MIVSO_CLEAR_SELECTION, 0 );
		}
	}
}


void CVSOEditState::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnMouseMove( nFlags, rMousePoint );
		CVSOMultiState::SEditParameters *pEditParameters = pParentState->pParentState->GetEditParameters();
		if ( ( nFlags & MK_LBUTTON ) &&
				pParentState->currentSelection.IsValid() && 
				pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].IsValid() )
		{
			NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 );
			if ( pSelectedVSO == 0 )
			{
				pParentState->ClearPickVSOIDList();
				pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
				pParentState->OnMouseMove( nFlags, rMousePoint );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
				return;
			}
			//
			pEditParameters->nFlags = MIVSOSEP_POINT_NUMBER | MIVSOSEP_WIDTH | MIVSOSEP_OPACITY | MIVSOSEP_HEIGHT;
			::GetEditParameters( pEditParameters, CHID_MAPINFO_VSO_WINDOW );
			//
			if ( pParentState->currentSelection.IsControlPointType() )
			{
				pParentState->backupKeyPoints.LoadKeyPoints( &( pSelectedVSO->points ) );
				switch( pEditParameters->ePointNumber )
				{
					case CVSOMultiState::SEditParameters::PN_SINGLE:
					default:
					{
						pSelectedVSO->controlPoints[pParentState->currentSelection.nIndex] += pParentState->pStoreInputState->lastEventInfo.vTerrainPos - 
																																									pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].vTerrainPos;
						break;
					}
					case CVSOMultiState::SEditParameters::PN_MULTI:
					{
						if ( ( nFlags & MK_CONTROL ) == 0 )
						{
							for ( int nPointIndex = pParentState->currentSelection.nIndex; nPointIndex < pSelectedVSO->controlPoints.size(); ++nPointIndex )
							{
								pSelectedVSO->controlPoints[nPointIndex] += pParentState->pStoreInputState->lastEventInfo.vTerrainPos -
																														pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].vTerrainPos;
							}
						}
						else
						{
							for ( int nPointIndex = 0; nPointIndex <= pParentState->currentSelection.nIndex; ++nPointIndex )
							{
								pSelectedVSO->controlPoints[nPointIndex] += pParentState->pStoreInputState->lastEventInfo.vTerrainPos -
																														pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].vTerrainPos;
							}
						}
						break;
					}
					case CVSOMultiState::SEditParameters::PN_ALL:
					{
						for ( int nPointIndex = 0; nPointIndex < pSelectedVSO->controlPoints.size(); ++nPointIndex )
						{
							pSelectedVSO->controlPoints[nPointIndex] += pParentState->pStoreInputState->lastEventInfo.vTerrainPos -
																													pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].vTerrainPos;
						}
						break;
					}
				}
				// Необходимо обновить eventInfoList[CStoreInputState::ISE_LBUTTONDOWN];
				pParentState->pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
			}
			else if ( pParentState->currentSelection.IsCenterPointType() )
			{
				pParentState->backupKeyPoints.LoadKeyPoints( &( pSelectedVSO->points ) );
				if ( ICamera *pCamera = Camera() )
				{
					CTPoint<int> dimensions( 0, 0 );
					if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_GET_DIMENSIONS, reinterpret_cast<uint32_t>( &dimensions ) ) )
					{
						float vNewZ = 0.0f;
						{
							const int nDY = pParentState->pStoreInputState->lastEventInfo.point.y - pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].point.y;
							float fNear = 0.0f;
							float fFar = 0.0f;
							float fFOV = 0.0f;
							pCamera->GetCameraParams( &fNear, &fFar, &fFOV );
							const SHMatrix &rViewMatrix = pCamera->GetViewMatrix();
							const CVec3 vUPVector( rViewMatrix._12, rViewMatrix._22, rViewMatrix._32 );
							const CVec3 vDirection( rViewMatrix._13, rViewMatrix._23, rViewMatrix._33 );
							const CVec3 vOrigin( rViewMatrix._14, rViewMatrix._24, rViewMatrix._34 );
							const CVec3 vCenterPoint = pSelectedVSO->points[pParentState->currentSelection.nIndex].vPos;
							// fNear - сократили
							const float fDY = ( nDY * tan( fFOV / 2.0f ) /* *fNear */ ) / ( dimensions.x * 1.0f );
							const float fDZ = 2.0f * vUPVector.z * fDY * fabs( vOrigin.x - vCenterPoint.x, vOrigin.y - vCenterPoint.y ) / ( fabs( vDirection.x, vDirection.y ) /* *fNear */ );
							vNewZ = pParentState->currentSelection.vDifference.z + fDZ;
							//DebugTrace( "Dimensions: %dx%d, nDY: %d, fDZ: %g", dimensions.x, dimensions.y, nDY, fDZ );
						}
						//
						switch( pEditParameters->ePointNumber )
						{
							case CVSOMultiState::SEditParameters::PN_SINGLE:
							default:
							{
								pParentState->backupKeyPoints.LoadKeyPoints( &( pSelectedVSO->points ) );
								pSelectedVSO->points[pParentState->currentSelection.nIndex].vPos.z = vNewZ;
								break;
							}
							case CVSOMultiState::SEditParameters::PN_MULTI:
							{
								pSelectedVSO->points[pParentState->currentSelection.nIndex].vPos.z = vNewZ;
								pParentState->backupKeyPoints.SaveKeyPoints( pSelectedVSO->points );
								break;
							}
							case CVSOMultiState::SEditParameters::PN_ALL:
							{
								for ( int nPointIndex = 0; nPointIndex < pSelectedVSO->points.size(); ++nPointIndex )
								{
									pSelectedVSO->points[nPointIndex].vPos.z = vNewZ;
								}
								pParentState->backupKeyPoints.SaveKeyPoints( pSelectedVSO->points );
								break;
							}
						}
					}
				}
			}
			else if ( pParentState->currentSelection.IsNormalePointType() )
			{
				CVec3 vShift = pParentState->pStoreInputState->lastEventInfo.vTerrainPos - pSelectedVSO->points[pParentState->currentSelection.nIndex].vPos - pParentState->currentSelection.vDifference;
				float fNewWidth = fabs( pSelectedVSO->points[pParentState->currentSelection.nIndex].vNorm * ( vShift * pSelectedVSO->points[pParentState->currentSelection.nIndex].vNorm ) );
				switch( pEditParameters->ePointNumber )
				{
					case CVSOMultiState::SEditParameters::PN_SINGLE:
					default:
					{
						pParentState->backupKeyPoints.LoadKeyPoints( &( pSelectedVSO->points ) );
						pSelectedVSO->points[pParentState->currentSelection.nIndex].fWidth = fNewWidth;
						break;
					}
					case CVSOMultiState::SEditParameters::PN_MULTI:
					{
						pSelectedVSO->points[pParentState->currentSelection.nIndex].fWidth = fNewWidth;
						pParentState->backupKeyPoints.SaveKeyPoints( pSelectedVSO->points );
						break;
					}
					case CVSOMultiState::SEditParameters::PN_ALL:
					{
						for ( int nPointIndex = 0; nPointIndex < pSelectedVSO->points.size(); ++nPointIndex )
						{
							pSelectedVSO->points[nPointIndex].fWidth = fNewWidth;
						}
						pParentState->backupKeyPoints.SaveKeyPoints( pSelectedVSO->points );
						break;
					}
				}
				pEditParameters->fWidth = fNewWidth;
				SetEditParameters( *pEditParameters, CHID_MAPINFO_VSO_WINDOW );
			}
			if ( ( pSelectedVSO->controlPoints.size() > 2 ) && ( pParentState->IsEdgesMustBeOut() ) )
			{
				CVSOManager::MoveEdgeControlPointsOut( pSelectedVSO, pParentState->mapRect, pParentState->GetDefaultStepOut(), true, true );
			}
			CVSOManager::Update( pSelectedVSO,
													 pParentState->currentSelection.IsControlPointType(),
													 true,
													 pParentState->GetDefaultStep(),
													 pEditParameters->fWidth,
													 pEditParameters->fHeight,
													 pEditParameters->fOpacity,
													 pParentState->currentSelection.IsControlPointType() || pParentState->currentSelection.IsNormalePointType(),
													 pParentState->currentSelection.IsControlPointType() || pParentState->currentSelection.IsCenterPointType(),
													 true,
													 pParentState->IsClose(),
													 pParentState->IsComplete() );
			pParentState->UpdateVSO( pParentState->GetSelectedVSOID(), CVSOState::UT_CONTINUE_MOVE, pParentState->currentSelection.eSelectionType, nFlags );
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return;
		}
		else if ( ( nFlags & MK_RBUTTON ) &&
							pParentState->currentSelection.IsValid() && 
							pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_RBUTTONDOWN].IsValid() )
		{
			if ( pParentState->currentSelection.IsNormalePointType() )
			{
				NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 );
				if ( pSelectedVSO == 0 )
				{
					pParentState->ClearPickVSOIDList();
					pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
					pParentState->OnMouseMove( nFlags, rMousePoint );
					Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
					return;
				}
				//
				pEditParameters->nFlags = MIVSOSEP_POINT_NUMBER | MIVSOSEP_WIDTH | MIVSOSEP_OPACITY | MIVSOSEP_HEIGHT;
				::GetEditParameters( pEditParameters, CHID_MAPINFO_VSO_WINDOW );
				//
				int nShift = pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_RBUTTONDOWN].point.y - pParentState->pStoreInputState->lastEventInfo.point.y;
				if ( pEditParameters->ePointNumber == CVSOMultiState::SEditParameters::PN_ALL )
				{
					for ( int nPointIndex = 0; nPointIndex < pSelectedVSO->points.size(); ++nPointIndex )
					{
						pSelectedVSO->points[nPointIndex].fOpacity = pParentState->currentSelection.fOpacity + ( nShift / CVSOManager::OPACITY_DELIMITER );
						if ( pSelectedVSO->points[nPointIndex].fOpacity < 0.0f )
						{
							pSelectedVSO->points[nPointIndex].fOpacity = 0.0f;
						}
						else if ( pSelectedVSO->points[nPointIndex].fOpacity > 1.0f )
						{
							pSelectedVSO->points[nPointIndex].fOpacity = 1.0f;
						}
					}
				}
				else
				{
					pSelectedVSO->points[pParentState->currentSelection.nIndex].fOpacity = pParentState->currentSelection.fOpacity + ( nShift / CVSOManager::OPACITY_DELIMITER );
					if ( pSelectedVSO->points[pParentState->currentSelection.nIndex].fOpacity < 0.0f )
					{
						pSelectedVSO->points[pParentState->currentSelection.nIndex].fOpacity = 0.0f;
					}
					else if ( pSelectedVSO->points[pParentState->currentSelection.nIndex].fOpacity > 1.0f )
					{
						pSelectedVSO->points[pParentState->currentSelection.nIndex].fOpacity = 1.0f;
					}
				}
				//
				pEditParameters->fOpacity = pSelectedVSO->points[pParentState->currentSelection.nIndex].fOpacity;
				SetEditParameters( *pEditParameters, CHID_MAPINFO_VSO_WINDOW );
				CVSOManager::Update( pSelectedVSO,
														 pParentState->currentSelection.IsControlPointType(),
														 true,
														 pParentState->GetDefaultStep(),
														 pEditParameters->fWidth,
														 pEditParameters->fHeight,
														 pEditParameters->fOpacity,
														 false,
														 false,
														 true,
														 pParentState->IsClose(),
														 pParentState->IsComplete() );
				pParentState->UpdateVSO( pParentState->GetSelectedVSOID(), CVSOState::UT_CONTINUE_MOVE, pParentState->currentSelection.eSelectionType, nFlags );
				//
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
				return;
			}
		}
	}
}


void CVSOEditState::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
		CVSOMultiState::SEditParameters *pEditParameters = pParentState->pParentState->GetEditParameters();
		pParentState->currentSelection.Invalidate();
		pParentState->backupKeyPoints.Clear();
		//
		NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 );
		if ( pSelectedVSO == 0 )
		{
			pParentState->ClearPickVSOIDList();
			pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
			pParentState->OnLButtonDown( nFlags, rMousePoint );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return;
		}
		CVec3 vOrigin = VNULL3;
		CVec3 vDirection = VNULL3;
		Camera()->GetProjectiveRay( &vOrigin, &vDirection, CVec2( rMousePoint.x, rMousePoint.y ) );
		Vis2AI( &vOrigin );
		if ( CVSOManager::GetVSOSelection( &( pParentState->currentSelection ),
																			 pParentState->pStoreInputState->lastEventInfo.vTerrainPos,
																			 vOrigin,
																			 vDirection,
																			 *pSelectedVSO,
																			 pParentState->leftButtonVSOSelectionParamList ) )
		{
			if ( pParentState->currentSelection.IsNormalePointType() )
			{
				pEditParameters->nFlags = MIVSOSEP_WIDTH | MIVSOSEP_OPACITY | MIVSOSEP_HEIGHT;
				pEditParameters->fWidth = pSelectedVSO->points[pParentState->currentSelection.nIndex].fWidth;
				pEditParameters->fHeight = pSelectedVSO->points[pParentState->currentSelection.nIndex].vPos.z;
				pEditParameters->fOpacity = pSelectedVSO->points[pParentState->currentSelection.nIndex].fOpacity;
				SetEditParameters( *pEditParameters, CHID_MAPINFO_VSO_WINDOW );
			}
			pParentState->backupKeyPoints.SaveKeyPoints( pSelectedVSO->points );
			pParentState->UpdateVSO( pParentState->GetSelectedVSOID(), CVSOState::UT_START_MOVE, pParentState->currentSelection.eSelectionType, MK_LBUTTON );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return;
		}
		// Необходимо выделить другой VSO если мы попали мимо текущего выделенного VSO
		const CVSOState::CVSOIDList pickVSOIDList = pParentState->pickVSOIDList;
		const int nSelectedIndex = pParentState->nSelectedIndex;
		const int nSelectedVSOID = pParentState->GetSelectedVSOID();
		//
		pParentState->ClearPickVSOIDList();
		//
		pParentState->PickVSO( pParentState->pStoreInputState->lastEventInfo.vTerrainPos, &( pParentState->pickVSOIDList ) );
		bool bReturnOldVSO = true;
		if ( !pParentState->pickVSOIDList.empty() )
		{
			bool bOldVSONotPresent = true;
			for ( int nSelectedVSOIndex = 0; nSelectedVSOIndex < pParentState->pickVSOIDList.size(); ++nSelectedVSOIndex )
			{					
				if ( nSelectedVSOID == pParentState->pickVSOIDList[nSelectedVSOIndex] )
				{
					pParentState->nSelectedIndex = nSelectedVSOIndex;
					bOldVSONotPresent = false;
					break;
				}
			}	
			// Выделяем другой - первый обьект
			if ( bOldVSONotPresent )
			{
				pParentState->nSelectedIndex = 0;
				pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
				bReturnOldVSO = false;
			}
		}
		else if ( pParentState->PickOtherVSO( nFlags, rMousePoint, pParentState->pStoreInputState->lastEventInfo.vTerrainPos ) )
		{
			return;
		}
		// нет других обьектов ( или есть старый выделенный обьект )- возвращаем старый
		if ( bReturnOldVSO )
		{
			pParentState->pickVSOIDList = pickVSOIDList;
			pParentState->nSelectedIndex = nSelectedIndex;
		}
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CVSOEditState::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonUp( nFlags, rMousePoint );
		//
		NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 );
		if ( pSelectedVSO == 0 )
		{
			pParentState->ClearPickVSOIDList();
			pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
			pParentState->OnLButtonUp( nFlags, rMousePoint );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return;
		}
		//
		if ( pParentState->currentSelection.IsValid() )
		{
			pParentState->UpdateVSO( pParentState->GetSelectedVSOID(), CVSOState::UT_FINISH_MOVE, pParentState->currentSelection.eSelectionType, MK_LBUTTON );
		}
		pParentState->currentSelection.Invalidate();
		pParentState->backupKeyPoints.Clear();
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CVSOEditState::OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		// Небходимо оба проапдейтить
		pParentState->pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
		pParentState->pStoreInputState->OnRButtonDown( nFlags, rMousePoint );
		if ( CVSOMultiState::SEditParameters *pEditParameters = pParentState->pParentState->GetEditParameters() )
		{
			pParentState->currentSelection.Invalidate();
			pParentState->backupKeyPoints.Clear();
			//
			NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 );
			if ( pSelectedVSO == 0 )
			{
				pParentState->ClearPickVSOIDList();
				pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
				pParentState->OnRButtonDown( nFlags, rMousePoint );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
				return;
			}
			//
			CVec3 vOrigin = VNULL3;
			CVec3 vDirection = VNULL3;
			//Camera()->GetProjectiveRay( &vOrigin, &vDirection, CVec2( rMousePoint.x, rMousePoint.y ) );
			//Vis2AI( &vOrigin );
			if ( CVSOManager::GetVSOSelection( &( pParentState->currentSelection ),
																				pParentState->pStoreInputState->lastEventInfo.vTerrainPos,
																				vOrigin,
																				vDirection,
																				*pSelectedVSO,
																				pParentState->rightButtonVSOSelectionParamList ) )
			{
				pEditParameters->nFlags = MIVSOSEP_WIDTH | MIVSOSEP_OPACITY | MIVSOSEP_HEIGHT;
				pEditParameters->fWidth = pSelectedVSO->points[pParentState->currentSelection.nIndex].fWidth;
				pEditParameters->fHeight = pSelectedVSO->points[pParentState->currentSelection.nIndex].vPos.z;
				pEditParameters->fOpacity = pSelectedVSO->points[pParentState->currentSelection.nIndex].fOpacity;
				SetEditParameters( *pEditParameters, CHID_MAPINFO_VSO_WINDOW );
				//
				pParentState->UpdateVSO( pParentState->GetSelectedVSOID(), CVSOState::UT_START_MOVE, pParentState->currentSelection.eSelectionType, MK_RBUTTON );
				//
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
				return;
			}
		}
	}
}


void CVSOEditState::OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnRButtonUp( nFlags, rMousePoint );
		//
		NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 );
		if ( pSelectedVSO == 0 )
		{
			pParentState->ClearPickVSOIDList();
			pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
			pParentState->OnRButtonUp( nFlags, rMousePoint );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return;
		}
		//
		if ( pParentState->currentSelection.IsValid() )
		{
			pParentState->UpdateVSO( pParentState->GetSelectedVSOID(), CVSOState::UT_FINISH_MOVE, pParentState->currentSelection.eSelectionType, MK_RBUTTON );
		}
		pParentState->currentSelection.Invalidate();
		pParentState->backupKeyPoints.Clear();
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CVSOEditState::OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonDblClk( nFlags, rMousePoint );
		//
		pParentState->currentSelection.Invalidate();
		pParentState->backupKeyPoints.Clear();
		//
		NHPTimer::STime time = 0;
		NHPTimer::GetTime( &time );
		//
		if ( pParentState->EdgesMustBeZero() )
		{
			NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 );
			if ( pSelectedVSO == 0 )
			{
				pParentState->ClearPickVSOIDList();
				pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
				pParentState->OnLButtonDblClk( nFlags, rMousePoint );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
				return;
			}
			DebugTrace( "CVSOEditState::OnLButtonDblClk(): pParentState->GetVSO(): %g", NHPTimer::GetTimePassed( &time ) );
			//
			if ( pSelectedVSO->points.size() > 0 )
			{
				pSelectedVSO->points[0].vPos.z = 0;
				pSelectedVSO->points[pSelectedVSO->points.size() - 1].vPos.z = 0;
			}
		}
		pParentState->UpdateVSO( pParentState->GetSelectedVSOID(), CVSOState::UT_FINISH, CVSOManager::SVSOSelection::ST_UNKNOWN, 0 );
		//
		DebugTrace( "CVSOEditState::OnLButtonDblClk(): pParentState->GetVSO(): %g", NHPTimer::GetTimePassed( &time ) );
		//
		pParentState->ClearPickVSOIDList();
		//
		pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CVSOEditState::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnKeyDown( nChar, nRepCnt, nFlags );
		CVSOMultiState::SEditParameters *pEditParameters = pParentState->pParentState->GetEditParameters();
		if ( nChar == VK_INSERT ) 
		{
			if ( pParentState->currentSelection.IsValid() && 
					pParentState->pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].IsValid() )
			{
				NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 );
				if ( pSelectedVSO == 0 )
				{
					pParentState->ClearPickVSOIDList();
					pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
					pParentState->OnKeyDown( nChar, nRepCnt, nFlags );
					Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
					return;
				}
				//
				pEditParameters->nFlags = MIVSOSEP_POINT_NUMBER | MIVSOSEP_WIDTH | MIVSOSEP_OPACITY | MIVSOSEP_HEIGHT;
				::GetEditParameters( pEditParameters, CHID_MAPINFO_VSO_WINDOW );
				//
				if ( pParentState->currentSelection.IsControlPointType() )
				{
					CVec3 vBegin = pSelectedVSO->controlPoints[pParentState->currentSelection.nIndex];
					if ( pParentState->currentSelection.nIndex < ( pSelectedVSO->controlPoints.size() - 1 ) )
					{
						CVec3 vEnd = pSelectedVSO->controlPoints[pParentState->currentSelection.nIndex + 1];
						CVec3 vNewControlPoint = ( vBegin + vEnd ) / 2.0f;
						pSelectedVSO->controlPoints.insert( pSelectedVSO->controlPoints.begin() + ( pParentState->currentSelection.nIndex + 1 ), vNewControlPoint );
						pParentState->backupKeyPoints.AddKeyPoint( ( pParentState->currentSelection.nIndex + 1 ), pEditParameters->fWidth, pEditParameters->fOpacity );
					}
					else
					{
						CVec3 vEnd = pSelectedVSO->controlPoints[pParentState->currentSelection.nIndex - 1];
						CVec3 vNewControlPoint = ( vBegin + vEnd ) / 2.0f;
						pSelectedVSO->controlPoints.insert( pSelectedVSO->controlPoints.begin() + ( pParentState->currentSelection.nIndex + 0 ), vNewControlPoint );
						pParentState->backupKeyPoints.AddKeyPoint( ( pParentState->currentSelection.nIndex + 0 ), pEditParameters->fWidth, pEditParameters->fOpacity );

						++( pParentState->currentSelection.nIndex );
					}
					//
					CVSOManager::Update( pSelectedVSO,
															 true,
															 true,
															 pParentState->GetDefaultStep(),
															 pEditParameters->fWidth,
															 pEditParameters->fHeight,
															 pEditParameters->fOpacity,
															 true,
															 true,
															 true,
															 pParentState->IsClose(),
															 pParentState->IsComplete() );
					pParentState->backupKeyPoints.LoadKeyPoints( &( pSelectedVSO->points ) );
					if ( ( pSelectedVSO->controlPoints.size() > 2 ) && ( pParentState->IsEdgesMustBeOut() ) )
					{
						CVSOManager::MoveEdgeControlPointsOut( pSelectedVSO, pParentState->mapRect, pParentState->GetDefaultStepOut(), true, true );
					}
					CVSOManager::Update( pSelectedVSO,
															 true,
															 true,
															 pParentState->GetDefaultStep(),
															 pEditParameters->fWidth,
															 pEditParameters->fHeight,
															 pEditParameters->fOpacity,
															 true,
															 true,
															 true,
															 pParentState->IsClose(),
															 pParentState->IsComplete() );
					pParentState->UpdateVSO( pParentState->GetSelectedVSOID(), CVSOState::UT_CHANGE_POINT_NUMBER, CVSOManager::SVSOSelection::ST_UNKNOWN, 0 );
					//
					Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
					return;
				}
			}
		}
		else if ( nChar == VK_DELETE ) 
		{
			NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 );
			if ( pSelectedVSO == 0 )
			{
				pParentState->ClearPickVSOIDList();
				pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
				OnKeyDown( nChar, nRepCnt, nFlags );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
				return;
			}
			pParentState->RemoveSelectedVSO();
			return;
		}
		else if ( ( nChar == VK_RETURN ) || ( nChar == VK_SPACE ) || ( nChar == VK_ESCAPE ) )
		{
			pParentState->currentSelection.Invalidate();
			pParentState->backupKeyPoints.Clear();
			//
			if ( nChar == VK_ESCAPE )
			{
				if ( NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 ) )
				{
					pSelectedVSO->points = pParentState->startVSOInstance.points;
					pSelectedVSO->controlPoints = pParentState->startVSOInstance.controlPoints;
				}
			}
			else
			{
				if ( NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 ) )
				{
					if ( pParentState->EdgesMustBeZero() )
					{
						if ( pSelectedVSO->points.size() > 0 )
						{
							pSelectedVSO->points[0].vPos.z = 0;
							pSelectedVSO->points[pSelectedVSO->points.size() - 1].vPos.z = 0;
						}
					}
				}
			}
			NHPTimer::STime time = 0;
			NHPTimer::GetTime( &time );
			//
			pParentState->UpdateVSO( pParentState->GetSelectedVSOID(), ( nChar != VK_ESCAPE ) ? CVSOState::UT_FINISH : CVSOState::UT_CANCEL, CVSOManager::SVSOSelection::ST_UNKNOWN, 0 );
			//
			DebugTrace( "CVSOEditState::OnKeyDown(): pParentState->UpdateVSO(): %g", NHPTimer::GetTimePassed( &time ) );
			//			
			pParentState->ClearPickVSOIDList();
			//
			pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			return;
		}
	}
}


bool CVSOAddState::InsertVSO()
{
	NHPTimer::STime totalTime = 0;
	NHPTimer::GetTime( &totalTime );
	//
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	bool bResult = true;
	UniquePolygon<std::vector<CVec3>, CVec3>( &( pParentState->startVSOInstance.controlPoints ), CVSOManager::DEFAULT_POINT_RADIUS );
	//
	DebugTrace( "CVSOAddState::InsertVSO(): UniquePolygon: %g", NHPTimer::GetTimePassed( &time ) );
	//
	int nNewVSOID = INVALID_NODE_ID;
	if ( pParentState->startVSOInstance.controlPoints.size() > 1 )
	{
		nNewVSOID = pParentState->InsertVSO( pParentState->startVSOInstance.controlPoints );
	}
	//
	DebugTrace( "CVSOAddState::InsertVSO(): pParentState->InsertVSO(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	pParentState->ClearStartVSOPointList();
	pParentState->ClearPickVSOIDList();
	pParentState->currentSelection.Invalidate();
	//
	if ( nNewVSOID != INVALID_NODE_ID )
	{
		pParentState->pickVSOIDList.push_back( nNewVSOID );
		pParentState->nSelectedIndex = 0;
		//
		pParentState->UpdateVSO( pParentState->GetSelectedVSOID(), CVSOState::UT_START, CVSOManager::SVSOSelection::ST_UNKNOWN, 0 );
		//
		DebugTrace( "CVSOAddState::InsertVSO(): pParentState->UpdateVSO(): %g", NHPTimer::GetTimePassed( &time ) );
		//
		{
			NDb::SVSOInstance *pSelectedVSO = pParentState->GetVSO( pParentState->GetSelectedVSOID(), 0 );
			//
			DebugTrace( "CVSOAddState::InsertVSO(): pParentState->GetVSO(): %g", NHPTimer::GetTimePassed( &time ) );
			//
			pParentState->startVSOInstance.points = pSelectedVSO->points;
			pParentState->startVSOInstance.controlPoints = pSelectedVSO->controlPoints;
		}
		pParentState->SetActiveInputState( CVSOState::IS_EDIT, true, false );
	}
	else
	{
		pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
		bResult = false;
	}
	//
	DebugTrace( "CVSOAddState::InsertVSO(): total: %g", NHPTimer::GetTimePassed( &totalTime ) );
	//

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	return bResult;
}


void CVSOAddState::OnSetFocus( CWnd* pNewWnd )
{
	if ( pParentState->CanEdit() )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CVSOAddState::OnKillFocus( CWnd* pOldWnd )
{
	if ( pParentState->CanEdit() )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CVSOAddState::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnMouseMove( nFlags, rMousePoint );
		//
		pParentState->startVSOInstance.controlPoints.pop_back();
		pParentState->startVSOInstance.controlPoints.push_back( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
		pParentState->UpdateVisualVSO( &( pParentState->startVSOInstance ), false );
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CVSOAddState::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonUp( nFlags, rMousePoint );
		//
		pParentState->startVSOInstance.controlPoints.push_back( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
		pParentState->UpdateVisualVSO( &( pParentState->startVSOInstance ), false );
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CVSOAddState::OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnRButtonUp( nFlags, rMousePoint );

		if ( pParentState->startVSOInstance.controlPoints.size() > 1 )
		{
			pParentState->startVSOInstance.controlPoints.pop_back();
			pParentState->startVSOInstance.controlPoints.pop_back();
			pParentState->startVSOInstance.controlPoints.push_back( pParentState->pStoreInputState->lastEventInfo.vTerrainPos );
			pParentState->UpdateVisualVSO( &( pParentState->startVSOInstance ), false );
		}
		else
		{
			pParentState->ClearStartVSOPointList();
			pParentState->ClearPickVSOIDList();
			//
			pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
		}
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CVSOAddState::OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnLButtonDblClk( nFlags, rMousePoint );
		//
		if ( !InsertVSO() )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_VSO_WINDOW, ID_MIVSO_CLEAR_SELECTION, 0 );
		}
	}
}


void CVSOAddState::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( pParentState->CanEdit() )
	{
		pParentState->pStoreInputState->OnKeyDown( nChar, nRepCnt, nFlags );
		//
		if ( ( nChar == VK_RETURN ) || ( nChar == VK_SPACE ) )
		{
			if ( !InsertVSO() )
			{
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_VSO_WINDOW, ID_MIVSO_CLEAR_SELECTION, 0 );
			}
		}
		else if ( nChar == VK_ESCAPE )
		{
			pParentState->ClearStartVSOPointList();
			pParentState->ClearPickVSOIDList();
			//
			pParentState->SetActiveInputState( CVSOState::IS_SELECT, true, false );
			//
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_VSO_WINDOW, ID_MIVSO_CLEAR_SELECTION, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


void CVSOState::Enter()
{
	currentSelection.Invalidate();
	backupKeyPoints.Clear();
	ClearPickVSOIDList();
	ClearStartVSOPointList();
	leftButtonVSOSelectionParamList.clear();
	rightButtonVSOSelectionParamList.clear();
	// Создаем заново параметры селектора
	if ( CanEditPoints( CVSOManager::SVSOSelection::ST_NORMALE ) )
	{
		leftButtonVSOSelectionParamList.push_back( CVSOManager::SVSOSelectionParam( CVSOManager::SVSOSelection::ST_NORMALE, CVSOManager::NORMALE_POINT_RADIUS ) );
		rightButtonVSOSelectionParamList.push_back( CVSOManager::SVSOSelectionParam( CVSOManager::SVSOSelection::ST_NORMALE, CVSOManager::NORMALE_POINT_RADIUS ) );
	}
	if ( CanEditPoints( CVSOManager::SVSOSelection::ST_OPNORMALE ) )
	{
		leftButtonVSOSelectionParamList.push_back( CVSOManager::SVSOSelectionParam( CVSOManager::SVSOSelection::ST_OPNORMALE, CVSOManager::NORMALE_POINT_RADIUS ) );
		rightButtonVSOSelectionParamList.push_back( CVSOManager::SVSOSelectionParam( CVSOManager::SVSOSelection::ST_NORMALE, CVSOManager::NORMALE_POINT_RADIUS ) );
	}
	if ( CanEditPoints( CVSOManager::SVSOSelection::ST_CENTER ) )
	{
		leftButtonVSOSelectionParamList.push_back( CVSOManager::SVSOSelectionParam( CVSOManager::SVSOSelection::ST_CENTER, CVSOManager::CENTER_POINT_RADIUS ) );
	}
	if ( CanEditPoints( CVSOManager::SVSOSelection::ST_CONTROL ) )
	{
		leftButtonVSOSelectionParamList.push_back( CVSOManager::SVSOSelectionParam( CVSOManager::SVSOSelection::ST_CONTROL, CVSOManager::CONTROL_POINT_RADIUS ) );
	}
	//
	mapRect = CTRect<float>( 0.0f, 0.0f, GetMapInfoEditor()->pMapInfo->nNumPatchesX * AI_TILE_SIZE * AI_TILES_IN_PATCH, GetMapInfoEditor()->pMapInfo->nNumPatchesY * AI_TILE_SIZE * AI_TILES_IN_PATCH );
	SetActiveInputState( IS_SELECT, true, false );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_VSO_STATE, this );
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_GET_FOCUS, 0 ) )
	{
		Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CVSOState::Leave()
{
	sceneDrawTool.Clear();
	//
	if ( GetActiveInputStateIndex() == IS_EDIT )
	{
		if ( NDb::SVSOInstance *pSelectedVSO = GetVSO( GetSelectedVSOID(), 0 ) )
		{
			pSelectedVSO->points = startVSOInstance.points;
			pSelectedVSO->controlPoints = startVSOInstance.controlPoints;
		}
		UpdateVSO( GetSelectedVSOID(), CVSOState::UT_CANCEL, CVSOManager::SVSOSelection::ST_UNKNOWN, 0 );
	}
	//
	currentSelection.Invalidate();
	backupKeyPoints.Clear();
	ClearPickVSOIDList();
	ClearStartVSOPointList();
	leftButtonVSOSelectionParamList.clear();
	rightButtonVSOSelectionParamList.clear();
	//
	SetActiveInputState( IS_SELECT, true, false );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_VSO_STATE );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
	// Не надо
	//Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	Singleton<IMainFrameContainer>()->Get()->SetStatusBarText( 1, "" );
}


void CVSOState::ClearSelection()
{
	Enter();
}


void CVSOState::RemoveSelectedVSO()
{
	if ( CanEdit() )
	{
		CVSOMultiState::SEditParameters *pEditParameters = pParentState->GetEditParameters();
		if ( currentSelection.IsValid() && 
				pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].IsValid() )
		{
			NDb::SVSOInstance *pSelectedVSO = GetVSO( GetSelectedVSOID(), 0 );
			if ( pSelectedVSO == 0 )
			{
				return;
			}
			//
			pEditParameters->nFlags = MIVSOSEP_POINT_NUMBER | MIVSOSEP_WIDTH | MIVSOSEP_OPACITY | MIVSOSEP_HEIGHT;
			::GetEditParameters( pEditParameters, CHID_MAPINFO_VSO_WINDOW );
			//
			if ( currentSelection.IsControlPointType() && ( pSelectedVSO->controlPoints.size() > 2 ) )
			{
				pSelectedVSO->controlPoints.erase( pSelectedVSO->controlPoints.begin() + currentSelection.nIndex );
				backupKeyPoints.RemoveKeyPoint( currentSelection.nIndex );

				CVSOManager::Update( pSelectedVSO,
														 true,
														 true,
														 GetDefaultStep(),
														 pEditParameters->fWidth,
														 pEditParameters->fHeight,
														 pEditParameters->fOpacity,
														 true,
														 true,
														 true,
														 IsClose(),
														 IsComplete() );
				backupKeyPoints.LoadKeyPoints( &( pSelectedVSO->points ) );
				if ( ( pSelectedVSO->controlPoints.size() > 2 ) && ( IsEdgesMustBeOut() ) )
				{
					CVSOManager::MoveEdgeControlPointsOut( pSelectedVSO, mapRect, GetDefaultStepOut(), true, true );
				}
				CVSOManager::Update( pSelectedVSO,
														 true,
														 true,
														 GetDefaultStep(),
														 pEditParameters->fWidth,
														 pEditParameters->fHeight,
														 pEditParameters->fOpacity,
														 true,
														 true,
														 true,
														 IsClose(),
														 IsComplete() );
				UpdateVSO( GetSelectedVSOID(), CVSOState::UT_CHANGE_POINT_NUMBER, CVSOManager::SVSOSelection::ST_UNKNOWN, 0 );
				currentSelection.Invalidate();
				//					
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			}
			return;
		}
		else
		{
			CString strMessage;
			AfxSetResourceHandle( theEDB2M1Instance );
			strMessage.LoadString( IDS_MIMO_DELETE_OBJECT_MESSAGE );
			AfxSetResourceHandle( AfxGetInstanceHandle() );
			if ( ::MessageBox( Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
			{
				NHPTimer::STime time = 0;
				NHPTimer::GetTime( &time );
				//
				RemoveVSO( GetSelectedVSOID() );
				//
				DebugTrace( "CVSOEditState::OnKeyDown(): RemoveVSO(): %g", NHPTimer::GetTimePassed( &time ) );
				//
				currentSelection.Invalidate();
				backupKeyPoints.Clear();
				ClearPickVSOIDList();
				//
				SetActiveInputState( CVSOState::IS_SELECT, true, false);
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
}


CMapInfoEditor* CVSOState::GetMapInfoEditor()
{ 
	return pParentState->pMapInfoEditor;
}


bool CVSOState::PickOtherVSO( unsigned nFlags, const CTPoint<int> &rMousePoint, const CVec3 &rvPos )
{ 
	return pParentState->PickOtherVSO( nFlags, rMousePoint, rvPos );
}


void CVSOState::EmulateSelectLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint, const CVec3 &rvPos )
{
	if ( CanEdit() )
	{
		pStoreInputState->OnLButtonDown( nFlags, rMousePoint );
		//
		ClearPickVSOIDList();
		//
		PickVSO( pStoreInputState->lastEventInfo.vTerrainPos, &( pickVSOIDList ) );
		// Выделяем верхний VSO
		if ( !pickVSOIDList.empty() )
		{
			nSelectedIndex = 0;
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CVSOState::UpdateVisualVSO( NDb::SVSOInstance *pVSO, bool bBothEdges )
{
	if ( CanEdit() )
	{
		CVSOMultiState::SEditParameters *pEditParameters = pParentState->GetEditParameters();
		if ( pVSO )
		{
			pEditParameters->nFlags = MIVSOSEP_POINT_NUMBER | MIVSOSEP_WIDTH | MIVSOSEP_OPACITY | MIVSOSEP_HEIGHT;
			::GetEditParameters( pEditParameters, CHID_MAPINFO_VSO_WINDOW );
			int nMinCount = INVALID_NODE_ID;
			int nMaxCount = INVALID_NODE_ID;
			GetControlPointBounds( &nMinCount, &nMaxCount );
			//UniquePolygon<std::vector<CVec3>, CVec3>( &( pVSOtoAddVSOInstance.controlPoints ), CVSOState::DEFAULT_POINT_RADIUS );
			if ( ( ( nMinCount == INVALID_NODE_ID ) || ( pVSO->controlPoints.size() >= nMinCount ) ) && 
					( pVSO->controlPoints.size() > 1 ) )
			{
				if ( ( pVSO->controlPoints.size() > 3 ) && ( IsEdgesMustBeOut() ) )
				{
					CVSOManager::MoveEdgeControlPointsOut( pVSO, mapRect, GetDefaultStepOut(), bBothEdges, true );
				}
				CVSOManager::Update( pVSO,
														 true,
														 false,
														 GetDefaultStep(),
														 pEditParameters->fWidth,
														 pEditParameters->fHeight,
														 pEditParameters->fOpacity,
														 true,
														 false,
														 true,
														 IsClose(),
														 IsComplete() );
			}
			else
			{
				pVSO->points.clear();
			}
		}
	}
}


void CVSOState::Draw( CPaintDC *pPaintDC )
{
	if ( CanEdit() )
	{	
		CVSOManager::SVSODrawParams drawParams;

		drawParams.pScene = EditorScene();
		drawParams.pSceneDrawTool = &sceneDrawTool;
		drawParams.bIsDrawSceneDrawTool = IsDrawSceneDrawTool();
		drawParams.pCurrSelection = &currentSelection;
		drawParams.bIsClose = IsClose();

		if ( ( GetActiveInputStateIndex() == IS_SELECT ) || ( GetActiveInputStateIndex() == IS_EDIT ) )
		{
			drawParams.pSelectedVSO = GetVSO( GetSelectedVSOID(), 0 ); 
		}
		else
		{
			drawParams.pSelectedVSO = 0;
		}
		//
		if ( GetActiveInputStateIndex() == IS_ADD )
		{
			drawParams.pVSO  = &startVSOInstance;
		}
		else
		{
			drawParams.pVSO  = 0;
		}
		//
		switch ( GetActiveInputStateIndex() )
		{
		case IS_SELECT: 
			drawParams.eDrawer = CVSOManager::SVSODrawParams::VSO_IS_SELECT;
			break;
		case IS_EDIT: 
			drawParams.eDrawer = CVSOManager::SVSODrawParams::VSO_IS_EDIT;
			break;
		case IS_ADD:
			drawParams.eDrawer = CVSOManager::SVSODrawParams::VSO_IS_ADD;
			break;
		default:
			drawParams.eDrawer = CVSOManager::SVSODrawParams::BAD_DRAWER;
		}

		drawParams.SetCanEdit( CVSOManager::SVSOSelection::ST_CONTROL, CanEditPoints( CVSOManager::SVSOSelection::ST_CONTROL ) );
		drawParams.SetCanEdit( CVSOManager::SVSOSelection::ST_CENTER, CanEditPoints( CVSOManager::SVSOSelection::ST_CENTER ) );
		drawParams.SetCanEdit( CVSOManager::SVSOSelection::ST_NORMALE, CanEditPoints( CVSOManager::SVSOSelection::ST_NORMALE ) );
		drawParams.SetCanEdit( CVSOManager::SVSOSelection::ST_OPNORMALE, CanEditPoints( CVSOManager::SVSOSelection::ST_OPNORMALE ) );
		//
		CVSOManager::DrawVSO( &drawParams );
	}

	//	extra drawing
	//if ( IEditorScene *pScene = EditorScene() )
	//{
	//	if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
	//	{
	//		std::vector<CVec3> hole;
	//		pTerraManager->GetHoleSamples( &hole );
	//	}
	//}	
}


void CVSOState::OnSetFocus( class CWnd* pNewWnd )
{
	CMultiInputState::OnSetFocus( pNewWnd );
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CVSOState::SwitchToAddState()
{
	ClearSelection();
	if ( CanInsertVSO() )
	{
		PrepareInsertVSO();
		ClearStartVSOPointList();
		SetActiveInputState( CVSOState::IS_ADD, true, false );
	}
	else
	{
		SetActiveInputState( CVSOState::IS_SELECT, true, false );
	}
}


bool CVSOState::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
		case ID_MIMO_SWITCH_ADD_STATE:
			SwitchToAddState();
			return true;
		case ID_SELECTION_CLEAR:
		{
			RemoveSelectedVSO();
			return true;
		}
		default:
			return false;
	}
	return false;
}


bool CVSOState::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CVSOState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CVSOState::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_MIMO_SWITCH_ADD_STATE:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_CLEAR:
		{
			if ( currentSelection.IsValid() && 
					pStoreInputState->eventInfoList[CStoreInputState::ISE_LBUTTONDOWN].IsValid() )
			{
				( *pbEnable ) = true;
				( *pbCheck ) = false;
			}
			else
			{
				( *pbEnable ) = ( GetSelectedVSOID() != INVALID_NODE_ID );
				( *pbCheck ) = false;
			}
			return true;
		}
		default:
			return false;
	}
	return false;
}


// basement storage  


