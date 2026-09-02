#include "stdafx.h"
#include <fmt/format.h>

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "DrawToolsDC.h"

#include "ReinfPointsState.h"
#include "ReinfPointsWindow.h"
#include "ReinfPointsTypedDlg.h"

#include <cstdint>

//
//
//		REINF POINTS STATE
//
//

bool CReinfPointsState::CreateReinfPoint()
{
	CVec3 vCamAnchor;
	GetCameraPosition( &vCamAnchor );
	Vis2AI( &vCamAnchor );

	CPtr<IManipulator> pMapInfoMan = pMapInfoEditor->GetViewManipulator();
	//
	int n = 0;
	if ( !CManipulatorManager::GetValue( &n, pMapInfoMan, "Players" ) )
		return false;
	if ( nSelectedPlayer < 0 || nSelectedPlayer >= n )
		return false;
	//
	const std::string szName = fmt::format( "Players.[{}].ReinforcementPoints", nSelectedPlayer );
	//
	CPtr<CObjectBaseController> pObjectController = 0;
	pObjectController = pMapInfoEditor->CreateController();

	if ( pObjectController->AddInsertOperation( szName, NODE_ADD_INDEX, pMapInfoMan ) )
	{
		pObjectController->Redo( false, true, pMapInfoEditor );
		Singleton<IControllerContainer>()->Add( pObjectController );
		// setting new point coords
		int nPointsCount = 0;
		if ( !CManipulatorManager::GetValue( &nPointsCount, pMapInfoMan, szName ) )
			return false;

		const std::string szCurReinfPos = szName + fmt::format( ".[{}].Position", nPointsCount-1 );
		const std::string szCurAviaReinfPos = szName + fmt::format( ".[{}].AviationPosition", nPointsCount-1 );
		//
		CManipulatorManager::SetValue( vCamAnchor.x, pMapInfoMan, szCurReinfPos + ".x" );
		CManipulatorManager::SetValue( vCamAnchor.y, pMapInfoMan, szCurReinfPos + ".y" );
		CManipulatorManager::SetValue( vCamAnchor.x, pMapInfoMan, szCurAviaReinfPos + ".x" );
		CManipulatorManager::SetValue( vCamAnchor.y, pMapInfoMan, szCurAviaReinfPos + ".y" );
	}

	return true;
}


bool CReinfPointsState::DeleteSelectedReinfPoint()
{
	CPtr<IManipulator> pMapInfoMan = pMapInfoEditor->GetViewManipulator();

	if ( nSelectedReinfPoint == -1 )
		return false;
	//
	int n = 0;
	if ( !CManipulatorManager::GetValue( &n, pMapInfoMan, "Players" ) )
		return false;
	if ( nSelectedPlayer < 0 || nSelectedPlayer >= n )
		return false;
	//
	CString strMessage;
	strMessage.LoadString( IDS_MIMO_DELETE_OBJECT_MESSAGE );
	if ( ::MessageBox( Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
	{
		const std::string szName = fmt::format( "Players.[{}].ReinforcementPoints", nSelectedPlayer );
		CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController();

		if ( pObjectController->AddRemoveOperation( szName, nSelectedReinfPoint, pMapInfoMan ) )
		{
			pObjectController->Redo( false, true, pMapInfoEditor );
			Singleton<IControllerContainer>()->Add( pObjectController );
		}
	}

	return true;
}


bool CReinfPointsState::GetReinfPointsFromWindow()
{
	CPtr<IManipulator> pManipulator = pMapInfoEditor->GetViewManipulator();
	//
	reinfPoints.clear();
	//
	std::string szPlayers = fmt::format( "Players" );
	int nPlayersCount = 0;
	CManipulatorManager::GetValue( &nPlayersCount, pManipulator, szPlayers );
	if ( nSelectedPlayer < 0 || nSelectedPlayer >= nPlayersCount )
		return false;

	std::string szReinfPoints = szPlayers + fmt::format( ".[{}].ReinforcementPoints", nSelectedPlayer );
	int nReinfPointsCount = 0;
	CManipulatorManager::GetValue( &nReinfPointsCount, pManipulator, szReinfPoints );
	for ( int iReinfPoint = 0; iReinfPoint < nReinfPointsCount; ++iReinfPoint ) // for each point
	{
		SReinfPoint rp;
		std::string szDBA = szReinfPoints + fmt::format( ".[{}]", iReinfPoint );
		CManipulatorManager::GetValue( &rp.vPosition, pManipulator, szDBA + ".Position" );
		CManipulatorManager::GetValue( &rp.vAviationPosition, pManipulator, szDBA + ".AviationPosition" );
		CManipulatorManager::GetValue( &rp.nDirection, pManipulator, szDBA + ".Direction" );
		CManipulatorManager::GetValue( &rp.szDeployTemplate, pManipulator, szDBA + ".Template" );

		std::string szTypedTemplates = szDBA + ".TypedTemplates";
		int nTypedTemplates = 0;
		CManipulatorManager::GetValue( &nTypedTemplates, pManipulator, szTypedTemplates );
		rp.typedTemplates.clear();
		for ( int iTypedTemplate = 0; iTypedTemplate < nTypedTemplates; ++iTypedTemplate ) // for each template
		
		{
			STypedTemplate newTemplate;

			CManipulatorManager::GetValue( &newTemplate.szTemplateType, pManipulator, szTypedTemplates + fmt::format(".[{}].Type", iTypedTemplate) );
			CManipulatorManager::GetValue( &newTemplate.szTemplate, pManipulator, szTypedTemplates + fmt::format(".[{}].Template", iTypedTemplate) );

			rp.typedTemplates.push_back( newTemplate );
		}
		reinfPoints.push_back( rp );
	}

	return true;
}


bool CReinfPointsState::SaveCurrentReinfPoint( const std::vector<SReinfPoint> &rReinfPoints, int nPlayerIndex, int nSelectedReinfPoint )
{
	CPtr<IManipulator> pManipulator = pMapInfoEditor->GetViewManipulator();

	int n = 0;
	if ( !CManipulatorManager::GetValue( &n, pManipulator, "Players" ) )
		return false;
	if ( nPlayerIndex < 0 || nPlayerIndex >= n )
		return false;
	//
	const std::string szReinfPts = fmt::format( "Players.[{}].ReinforcementPoints", nPlayerIndex );
	int m = 0;
	if ( !CManipulatorManager::GetValue( &m, pManipulator, szReinfPts ) )
		return false;
	if ( nSelectedReinfPoint < 0 || nSelectedReinfPoint >= m )
		return false;
	//
	const std::string szCurReinfPt = szReinfPts + fmt::format( ".[{}]", nSelectedReinfPoint );
	const std::string szCurReinfPos = szCurReinfPt + ".Position";
	const std::string szCurAviaReinfPos = szCurReinfPt + ".AviationPosition";
	const std::string szCurReinfDir = szCurReinfPt + ".Direction";
	//
	CPtr<CObjectBaseController> pObjectController = 0;
	pObjectController = pMapInfoEditor->CreateController();
	{
		CVec2 vPos = VNULL2;
		CManipulatorManager::GetValue( &vPos, pManipulator, szCurReinfPos );

		if ( vPos != rReinfPoints[nSelectedReinfPoint].vPosition )
		{
			// the position was changed
			CManipulatorManager::SetValue( rReinfPoints[nSelectedReinfPoint].vPosition, pManipulator, szCurReinfPos );
			if ( pObjectController )
				pObjectController->AddChangeVec2Operation<CVec2, float>( szCurReinfPos, rReinfPoints[nSelectedReinfPoint].vPosition, pManipulator );
		}
	}
	//
	{
		CVec2 vAviaPos = VNULL2;
		CManipulatorManager::GetValue( &vAviaPos, pManipulator, szCurAviaReinfPos );

		if ( vAviaPos != rReinfPoints[nSelectedReinfPoint].vAviationPosition )
		{
			// the avia position was changed
			CManipulatorManager::SetValue( rReinfPoints[nSelectedReinfPoint].vAviationPosition, pManipulator, szCurAviaReinfPos );
			if ( pObjectController )
				pObjectController->AddChangeVec2Operation<CVec2, float>( szCurAviaReinfPos, rReinfPoints[nSelectedReinfPoint].vAviationPosition, pManipulator );
		}
	}
	//
	{
		int nDir = 0;
		CManipulatorManager::GetValue( &nDir, pManipulator, szCurReinfDir );

		if ( nDir != rReinfPoints[nSelectedReinfPoint].nDirection )
		{
			// the direction was changed
			CManipulatorManager::SetValue( rReinfPoints[nSelectedReinfPoint].nDirection, pManipulator, szCurReinfDir);
			if ( pObjectController )
				pObjectController->AddChangeOperation( szCurReinfDir, rReinfPoints[nSelectedReinfPoint].nDirection, pManipulator );
		}
	}

	int nPrevTemplCount = 0;
	CManipulatorManager::GetValue( &nPrevTemplCount, pManipulator, szCurReinfPt + ".TypedTemplates" );

	for ( int i = 0; i < nPrevTemplCount; ++i ) // removing old templates
	{
		if ( pObjectController->AddRemoveOperation( szCurReinfPt + ".TypedTemplates", i, pManipulator) )
		{
			pObjectController->Redo( false, true, pMapInfoEditor );
			Singleton<IControllerContainer>()->Add( pObjectController );
		}
	}
	for ( int i = 0; i < rReinfPoints[nSelectedReinfPoint].typedTemplates.size(); ++i ) // inserting new
	{
		CPtr<CObjectBaseController> pObjectController = 0;
		pObjectController = pMapInfoEditor->CreateController();

		if ( pObjectController->AddInsertOperation( szCurReinfPt + ".TypedTemplates", NODE_ADD_INDEX, pManipulator ) )
		{
			pObjectController->Redo( false, true, pMapInfoEditor );
			Singleton<IControllerContainer>()->Add( pObjectController );
		}
		const std::string szCurTypedTempl = szCurReinfPt + fmt::format( ".TypedTemplates.[{}]", i );
		CManipulatorManager::SetValue( rReinfPoints[nSelectedReinfPoint].typedTemplates[i].szTemplateType, pManipulator, szCurTypedTempl + ".Type" );
		CManipulatorManager::SetValue( rReinfPoints[nSelectedReinfPoint].typedTemplates[i].szTemplate, pManipulator, szCurTypedTempl + ".Template", true );
	}

	return true;
}


void CReinfPointsState::Enter()
{
	reinfPoints.clear();
	bMove = false;
	bRotate = false;
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_REINF_POINTS_STATE, this );
	RefreshReinfPointsWindow();
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand(CHID_SCENE, ID_SCENE_GET_FOCUS, 0) )
	{
		Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
	}
}


void CReinfPointsState::Leave()
{
	reinfPoints.clear();
	sceneDrawTool.Clear();
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_REINF_POINTS_STATE );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
}


void CReinfPointsState::OnSetFocus( class CWnd* pNewWnd )
{
	CDefaultInputState::OnSetFocus( pNewWnd );
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


bool CReinfPointsState::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	switch ( nCommandID )
	{
		case ID_UPDATE_EDIT_PARAMETERS:
		{
			RefreshReinfPointsWindow();
			break;
		}
		case ID_REINF_POINTS_WINDOW_CHANGE_STATE:
		{
			SReinfPointsWindowData data;
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand(  CHID_REINF_POINTS_WINDOW, 
																																	ID_WINDOW_GET_DIALOG_DATA, 
																																	reinterpret_cast<uintptr_t>(&data) ) )
			{
				nSelectedReinfPoint = data.nSelectedPoint;
				nSelectedPlayer = data.nPlayerIndex;
				switch ( data.eLastAction )
				{
					case SReinfPointsWindowData::RWA_POINT_JUMP:
					{
						SetCameraPosition( CVec3(reinfPoints[nSelectedReinfPoint].vPosition.x, reinfPoints[nSelectedReinfPoint].vPosition.y, 0) );
						RefreshReinfPointsWindow();
						return true;
					}
					//
					case SReinfPointsWindowData::RWA_POINT_SEL_CHANGE:
					{
						RefreshReinfPointsWindow();
						return true;
					}
					//
					case SReinfPointsWindowData::RWA_PLAYER_CHANGE:
					{
						nSelectedReinfPoint = -1;
						RefreshReinfPointsWindow();
						return true;
					}
					//
					case SReinfPointsWindowData::RWA_POINT_ADD:
					{
						CreateReinfPoint();
						RefreshReinfPointsWindow();
						return true;
					}
					//
					case SReinfPointsWindowData::RWA_POINT_DEL:
					{
						DeleteSelectedReinfPoint();
						nSelectedReinfPoint = -1;
						RefreshReinfPointsWindow();
						return true;
					}
					//
					case SReinfPointsWindowData::RWA_POINT_EDIT_DEPLOY:
					{
						EditPointDeployTemplate();
						RefreshReinfPointsWindow();
						return true;
					}
					//
					case SReinfPointsWindowData::RWA_POINT_EDIT_TYPED:
					{
						EditPointTypedTemplate();
						RefreshReinfPointsWindow();
						return true;
					}
				}
			}
			return false;
		}
		case ID_SELECTION_CLEAR:
		{
			DeleteSelectedReinfPoint();
			nSelectedReinfPoint = -1;
			RefreshReinfPointsWindow();
			return true;
		}
	}

	return false;
}


bool CReinfPointsState::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CReinfPointsState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CReinfPointsState::UpdateCommand(), pbCheck == 0" );
	//
	switch ( nCommandID )
	{
		case ID_UPDATE_EDIT_PARAMETERS:
		{
			(*pbEnable) = true;
			(*pbCheck) = false;
			return true;
		}
		case ID_REINF_POINTS_WINDOW_CHANGE_STATE:
		{
			(*pbEnable) = true;
			(*pbCheck) = false;
			return true;
		}
		//
		case ID_SELECTION_CLEAR:
		{
			(*pbEnable) = true;
			(*pbCheck) = false;
			return true;
		}
	}

	return false;
}


CReinfPointsState::CReinfPointsState( CMapInfoEditor* _pMapInfoEditor )
	: pMapInfoEditor( _pMapInfoEditor )
{
	nSelectedPlayer = 0;
	nSelectedReinfPoint = -1;
	NI_ASSERT( pMapInfoEditor != 0, "CReinfPointsState(): Invalid parameter: pMapInfoEditor == 0" );
}


void CReinfPointsState::RefreshReinfPointsWindow()
{
	if ( nSelectedPlayer == -1 )
		return;
	//
	GetReinfPointsFromWindow();
	//
	SReinfPointsWindowData data;
	data.nPlayerCount = -1;
	CManipulatorManager::GetValue( &(data.nPlayerCount), pMapInfoEditor->GetViewManipulator(), "Players" );

	if ( nSelectedPlayer >= data.nPlayerCount )
	{
		nSelectedPlayer = 0;
	}
	data.nPlayerIndex = nSelectedPlayer;
	data.nSelectedPoint = nSelectedReinfPoint;
	//
	for ( int i = 0; i < reinfPoints.size(); ++i )
	{
		SReinfPointsWindowData::SReinfPoint rp;
		rp.vPosition = reinfPoints[i].vPosition;
		rp.bIsDefault = reinfPoints[i].bIsDefault;
		rp.eType = reinfPoints[i].eType;
		CManipulatorManager::GetValue( &(rp.szDeployTemplate), pMapInfoEditor->GetViewManipulator(), fmt::format("Players.[{}].ReinforcementPoints.[{}].Template" ,data.nPlayerIndex , i) );
		//
		data.reinfPoints.push_back( rp );
	}
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_REINF_POINTS_WINDOW, 
																												ID_WINDOW_SET_DIALOG_DATA, 
																												reinterpret_cast<uintptr_t>(&data) );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, 
																												ID_SCENE_UPDATE, 
																												0 );
}


void CReinfPointsState::Draw( CPaintDC *pPaintDC )
{
	ICamera *pCam = Camera();
	CPtr<IEditorScene> pScene = EditorScene();
	if ( !pCam )
		return;
	//
	sceneDrawTool.Clear();
	CVec3 vPos, vAviaPos;

	int i = 0;
	for ( std::vector<SReinfPoint>::const_iterator itPoint = reinfPoints.begin(); itPoint != reinfPoints.end(); ++itPoint, ++i )
	{
		const SReinfPoint &point = *itPoint;
		vPos = CVec3( point.vPosition, 0 );
		vPos.z = GetTerrainHeight( vPos.x, vPos.y );
		float fDir = AI2VisRad( point.nDirection );
		fDir += FP_PI2;
		if ( fDir >= FP_2PI )
		{ 
			fDir -= FP_2PI;
		}
		// ground
		CVec2 vDirection = CreateFromPolarCoord( (SELECTION_RADIUS1 + SELECTION_RADIUS0) / 2.0f, fDir );
		CVec3 vDirPoint = CVec3( vDirection.x + vPos.x, vDirection.y + vPos.y, vPos.z );
		//
		sceneDrawTool.DrawCircle( vPos, SELECTION_POINT_RADIUS, SELECTION_POINT_PARTS, SELECTION_COLOR, false );
		sceneDrawTool.DrawCircle( vPos, SCENE_OBJECT_SELECTION_RADIUS0, SCENE_OBJECT_SELECTION_PARTS, SCENE_OBJECT_SELECTION_COLOR, false );
		sceneDrawTool.DrawLine( vPos, vDirPoint, SCENE_OBJECT_SELECTION_COLOR, false );

		// avia
		vAviaPos = CVec3( point.vAviationPosition, 0 );
		vAviaPos.z = GetTerrainHeight( vAviaPos.x, vAviaPos.y );
		//
		sceneDrawTool.DrawCircle( vAviaPos, SELECTION_POINT_RADIUS, SELECTION_POINT_PARTS, SELECTION_COLOR, false );
		sceneDrawTool.DrawCircle( vAviaPos, SCENE_OBJECT_SELECTION_RADIUS0, SCENE_OBJECT_SELECTION_PARTS, SCENE_OBJECT_SELECTION_COLOR, false );
		sceneDrawTool.DrawLine( vPos, vAviaPos, SCENE_OBJECT_SELECTION_COLOR, false );
		//
		sceneDrawTool.DrawCircle( vPos, SELECTION_RADIUS0, SELECTION_PARTS, SELECTION_COLOR, false );
		sceneDrawTool.DrawCircle( vPos, SELECTION_RADIUS1, SELECTION_PARTS, SELECTION_COLOR, false );
		sceneDrawTool.DrawCircle( vDirPoint, SELECTION_POINT_RADIUS, SELECTION_PARTS, SELECTION_COLOR, false );

		if ( i == nSelectedReinfPoint )
		{
			if ( bMove && !bIsAvia )
			{
				for ( int i = 0; i < 10; ++i )
				{
					float fRad = i;
					sceneDrawTool.DrawCircle( vPos, SELECTION_POINT_RADIUS * (fRad/10), SELECTION_POINT_PARTS, SELECTION_COLOR, false );
				}
			}
			if ( bRotate )
			{
				for ( int i = 0; i < 10; ++i )
				{
					float fRad = i;
					sceneDrawTool.DrawCircle( vDirPoint, SELECTION_POINT_RADIUS * (fRad/10), SELECTION_POINT_PARTS, SELECTION_COLOR, false );
				}
			}
			//
			if ( bIsAvia && bMove )
			{
				for ( int i = 0; i < 10; ++i )
				{
					float fRad = i;
					sceneDrawTool.DrawCircle( vAviaPos, SELECTION_POINT_RADIUS * (fRad/10), SELECTION_POINT_PARTS, SELECTION_COLOR, false );
				}
			}
		}
	}
	sceneDrawTool.Draw();
}


void CReinfPointsState::PostDraw( CPaintDC *pPaintDC )
{
	ICamera *pCamera = Camera();
	CPtr<IEditorScene> pScene = EditorScene();
	if ( !pCamera )
		return;
	//
	for ( int i = 0; i < reinfPoints.size(); ++i )
	{
		CVec3 vPos( reinfPoints[i].vPosition, 0 ); 	
		vPos.z = GetTerrainHeight( vPos.x, vPos.y );

		CTransformStack ts = pCamera->GetTransform();

		CVec2 res = VNULL2;
		AI2Vis( &vPos );

		if ( TestRayInFrustrum( vPos, ts, pScene->GetScreenRect(), &res ) )
			NDrawToolsDC::DrawLabelDC( pPaintDC, fmt::format( "{} : ({:.0f}, {:.0f})", i, reinfPoints[i].vPosition.x, reinfPoints[i].vPosition.y ) , res );			
	}
}


void CReinfPointsState::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( reinfPoints.size() == 0 ) // no points
		return;
	CPtr<IEditorScene> pScene = EditorScene();
	if ( !pScene )
		return;

	CVec3 vPickedPosOnTerrain = VNULL3;
	Get2DPosOnMapHeights( &vPickedPosOnTerrain, CVec2(rMousePoint.x, rMousePoint.y) );

	bIsAvia = false;
	bMove = false;
	bRotate = false;
	int i = 0;
	float fDist = 0;
	for ( std::vector<SReinfPoint>::const_iterator it = reinfPoints.begin(); it != reinfPoints.end(); ++it, ++i )
	{
		// ground
		fDist = fabs( vPickedPosOnTerrain - CVec3(it->vPosition, 0) );
		if ( fDist <= SELECTION_POINT_RADIUS )
		{
			bMove = true;
			nSelectedReinfPoint = i;
			RefreshReinfPointsWindow();
			return;
		}
		if ( i == nSelectedReinfPoint )
		{
			float fDir = AI2VisRad( it->nDirection );
			fDir += FP_PI2;
			if ( fDir >= FP_2PI )
				fDir -= FP_2PI;

			CVec2 vDirection = CreateFromPolarCoord( (SELECTION_RADIUS1 + SELECTION_RADIUS0) / 2.0f, fDir );
			CVec3 vDirPoint = CVec3( vDirection.x + it->vPosition.x, vDirection.y + it->vPosition.y, 0 );

			fDist = fabs( vPickedPosOnTerrain - vDirPoint );
			if ( fDist <= SELECTION_POINT_RADIUS )
			{
				bRotate = true;
				return;
			}
		}
		// avia
		fDist = fabs( vPickedPosOnTerrain - CVec3(it->vAviationPosition, 0) );
		if ( fDist <= SELECTION_POINT_RADIUS )
		{
			bIsAvia = true;
			bMove = true;
			nSelectedReinfPoint = i;
			RefreshReinfPointsWindow();
			return;
		}
	}
	nSelectedReinfPoint = -1;
}


void CReinfPointsState::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( reinfPoints.size() == 0 ) // no points
		return;
	if ( nSelectedReinfPoint < 0 ) // empty selection
		return;

	CVec2 *pMovingPoint;

	if ( !bIsAvia ) // editing ground point
	{
		pMovingPoint = &(reinfPoints[nSelectedReinfPoint].vPosition);

		if ( bMove )
		{
			CVec2 vMapAISize = VNULL2;
			vMapAISize.x = pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE;
			vMapAISize.y = pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE;

			CVec3 vPickedPosOnTerrain = VNULL3;
			Get2DPosOnMapHeights( &vPickedPosOnTerrain, CVec2(rMousePoint.x, rMousePoint.y) );
			vPickedPosOnTerrain.x = Clamp( vPickedPosOnTerrain.x, 0.0f, vMapAISize.x );
			vPickedPosOnTerrain.y = Clamp( vPickedPosOnTerrain.y, 0.0f, vMapAISize.y );

			pMovingPoint->x = vPickedPosOnTerrain.x;
			pMovingPoint->y = vPickedPosOnTerrain.y;
		}
		else if ( bRotate )
		{
			CVec2 vMapAISize = VNULL2;
			vMapAISize.x = pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE;
			vMapAISize.y = pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE;

			CVec3 vPickedPosOnTerrain = VNULL3;
			Get2DPosOnMapHeights( &vPickedPosOnTerrain, CVec2(rMousePoint.x, rMousePoint.y) );
			vPickedPosOnTerrain.x = Clamp( vPickedPosOnTerrain.x, 0.0f, vMapAISize.x );
			vPickedPosOnTerrain.y = Clamp( vPickedPosOnTerrain.y, 0.0f, vMapAISize.y );

			const CVec2 vNewDir( vPickedPosOnTerrain.x - pMovingPoint->x, vPickedPosOnTerrain.y - pMovingPoint->y );
			float fNewDir = GetPolarAngle( vNewDir ) - FP_PI2;
			if ( fNewDir < 0 )
				fNewDir += FP_2PI;
			
			reinfPoints[nSelectedReinfPoint].nDirection = Vis2AIRad( fNewDir );
		}
		else
			return;
	}
	else // editing aviation point
	{
		pMovingPoint = &(reinfPoints[nSelectedReinfPoint].vAviationPosition);

		if ( bMove )
		{
			CVec2 vMapAISize = VNULL2;
			vMapAISize.x = pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE;
			vMapAISize.y = pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE;

			CVec3 vPickedPosOnTerrain = VNULL3;
			Get2DPosOnMapHeights( &vPickedPosOnTerrain, CVec2(rMousePoint.x, rMousePoint.y) );
			vPickedPosOnTerrain.x = Clamp( vPickedPosOnTerrain.x, 0.0f, vMapAISize.x );
			vPickedPosOnTerrain.y = Clamp( vPickedPosOnTerrain.y, 0.0f, vMapAISize.y );

			pMovingPoint->x = vPickedPosOnTerrain.x;
			pMovingPoint->y = vPickedPosOnTerrain.y;
		}
		else
			return;
	}

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CReinfPointsState::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( reinfPoints.size() == 0 ) // no points
		return;

	if ( bMove || bRotate )
	{
		CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController();

		if ( pObjectController )
		{
			pObjectController->Redo( false, true, pMapInfoEditor );
			Singleton<IControllerContainer>()->Add( pObjectController );
		}

		SaveCurrentReinfPoint( reinfPoints, nSelectedPlayer, nSelectedReinfPoint );
	}
	bMove = false;
	bRotate = false;
}


bool CReinfPointsState::EditPointDeployTemplate()
{
	if ( nSelectedReinfPoint < 0 )
	{
		NI_ASSERT( nSelectedReinfPoint >= 0, "nSelectedReinfPoint must be >= 0" );
		return false;
	}

	std::string szLink;
	const std::string szCurReinfDir = fmt::format( "Players.[{}].ReinforcementPoints.[{}].Template", nSelectedPlayer, nSelectedReinfPoint );
	//
	if ( Singleton<IMainFrameContainer>()->Get()->BrowseLink(&szLink, "", dynamic_cast<const SPropertyDesc*>(pMapInfoEditor->GetViewManipulator()->GetDesc(szCurReinfDir)), false, true) )
	{
		if ( szLink == "" )
			CManipulatorManager::SetValue( CVariant(), pMapInfoEditor->GetViewManipulator(), szCurReinfDir );
		else
			CManipulatorManager::SetValue( szLink, pMapInfoEditor->GetViewManipulator(), szCurReinfDir, true );
		return true;
	}
	else
		return false;
}


bool CReinfPointsState::EditPointTypedTemplate()
{
	if ( nSelectedReinfPoint < 0 )
	{
		NI_ASSERT( nSelectedReinfPoint >= 0, "nSelectedReinfPoint must be >= 0" );
		return false;
	}

	std::vector<STypedTemplate> vNewTypedTemplate = reinfPoints[nSelectedReinfPoint].typedTemplates;
	CReinfPointsTypedDlg dlg( Singleton<IMainFrameContainer>()->GetSECWorkbook(), &vNewTypedTemplate, pMapInfoEditor, nSelectedPlayer, nSelectedReinfPoint );
	if ( dlg.DoModal() == IDOK )
	{
		reinfPoints[nSelectedReinfPoint].typedTemplates = vNewTypedTemplate;
		SaveCurrentReinfPoint( reinfPoints, nSelectedPlayer, nSelectedReinfPoint );
		return true;
	}

	return false;
}


void CReinfPointsState::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	switch ( nChar )
	{
		case VK_RETURN:
		{
			if ( nSelectedReinfPoint == -1 )
				return;

			SetCameraPosition( CVec3(reinfPoints[nSelectedReinfPoint].vPosition.x, reinfPoints[nSelectedReinfPoint].vPosition.y, 0) );
			RefreshReinfPointsWindow();
			break;
		}
		//
		case VK_ESCAPE:
		{
			nSelectedReinfPoint = -1;
			RefreshReinfPointsWindow();
			break;
		}
		//
		case VK_INSERT:
		{
			CreateReinfPoint();
			RefreshReinfPointsWindow();
			break;
		}
	}
}



