#include "stdafx.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "CommandHandlerDefines.h"
#include "MapInfoEditor.h"
#include "DrawToolsDC.h"
#include "Tools_SceneDraw.h"
#include "SceneB2/Camera.h"

#include "CameraPositionState.h"
#include "CameraPositionWindow.h"

#include <cstdint>

static bool GetStartCameraPositionFromDB( SCameraPos *pResult, IManipulator *pMapInfoMan, int nPlayerIndex )
{
	if ( !pResult || !pMapInfoMan )
		return false;
	//
	int n = 0;
	if ( !CManipulatorManager::GetValue( &n, pMapInfoMan, "Players" ) )
		return false;
	//
	if ( nPlayerIndex < 0 || nPlayerIndex >= n )
		return false;
	//
	const string szCamera = StrFmt( "Players.[%d].Camera.", nPlayerIndex );
	//
	SCameraPos camPos;
	bool bResult = true;
	bResult = bResult && CManipulatorManager::GetValue( &camPos.vAnchor, pMapInfoMan, szCamera + "Anchor" );
	bResult = bResult && CManipulatorManager::GetValue( &camPos.fYaw, pMapInfoMan, szCamera + "Yaw" );
	bResult = bResult && CManipulatorManager::GetValue( &camPos.fPitch, pMapInfoMan, szCamera + "Pitch" );
	bResult = bResult && CManipulatorManager::GetValue( &camPos.fDistance, pMapInfoMan, szCamera + "Dist" );
	bResult = bResult && CManipulatorManager::GetValue( &camPos.bUseAnchorOnly, pMapInfoMan, szCamera + "UseAnchorOnly" );
	//
	DebugTrace( "Camera Get: %s", szCamera.c_str() );
	if ( bResult )
	{
		*pResult = camPos;
		return true;
	}
	else
	{
		*pResult = SCameraPos();
		return false;
	}
	return false;
}


static bool SetStartCameraPositionInDB( IManipulator *pMapInfoMan, const SCameraPos &rCamPos, int nPlayerIndex ) 
{
	if ( !pMapInfoMan )
		return false;
	//
	int n = 0;
	if ( !CManipulatorManager::GetValue( &n, pMapInfoMan, "Players" ) )
		return false;
	//
	if ( nPlayerIndex < 0 || nPlayerIndex >= n )
		return false;
	//
	const string szCamera = StrFmt( "Players.[%d].Camera.", nPlayerIndex );
	//
	bool bResult = true;
	bResult = bResult && CManipulatorManager::SetValue( rCamPos.vAnchor, pMapInfoMan, szCamera + "Anchor" );
	bResult = bResult && CManipulatorManager::SetValue( rCamPos.fYaw, pMapInfoMan, szCamera + "Yaw" );
	bResult = bResult && CManipulatorManager::SetValue( rCamPos.fPitch, pMapInfoMan, szCamera + "Pitch" );
	bResult = bResult && CManipulatorManager::SetValue( rCamPos.fDistance, pMapInfoMan, szCamera + "Dist" );
	bResult = bResult && CManipulatorManager::SetValue( rCamPos.bUseAnchorOnly, pMapInfoMan, szCamera + "UseAnchorOnly" );
	//
	//bResult = bResult && CManipulatorManager::SetValue( rCamPos.vAnchor, pMapInfoMan, szDBAAnchor );
	DebugTrace( "Camera Set: %s", szCamera.c_str() );
	return bResult;
}


static bool GetCameraPosition( SCameraPos *pCamPos )
{
	if ( !Camera() || !pCamPos )
		return false;
	//
	SCameraPos camPos;
	camPos.vAnchor = Camera()->GetAnchor();
	Camera()->GetPlacement( &camPos.fDistance, &camPos.fPitch, &camPos.fYaw );
	*pCamPos = camPos;
	//
	return true;
}


//
//
//		START CAMERA POSITION STATE
//
//

CCameraPositionState::CCameraPositionState( CMapInfoEditor* _pMapInfoEditor )
	: pMapInfoEditor( _pMapInfoEditor ),
	nCurrentPlayer( 0 )
{
	NI_ASSERT( pMapInfoEditor != 0, "CCameraPositionState(): Invalid parameter: pMapInfoEditor == 0" );
}


void CCameraPositionState::Enter()
{
	cameraPositions.clear();
	bMove = false;
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_CAMERA_POSITION_STATE, this );
	RefreshWindow( true );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
}


void CCameraPositionState::Leave()
{
	cameraPositions.clear();
	sceneDrawTool.Clear();
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_CAMERA_POSITION_STATE );
}


void CCameraPositionState::SavePosition()
{
	SCameraPos camPos;
	GetCameraPosition( &camPos );
	if ( nCurrentPlayer >= 0 && nCurrentPlayer < cameraPositions.size() )
	{
		camPos.bUseAnchorOnly = cameraPositions[nCurrentPlayer].bUseAnchorOnly;
		cameraPositions[nCurrentPlayer] = camPos;
		SetStartCameraPositionInDB( pMapInfoEditor->GetViewManipulator(), cameraPositions[nCurrentPlayer], nCurrentPlayer );
	}
}


bool CCameraPositionState::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch ( nCommandID )
	{
		case ID_UPDATE_EDIT_PARAMETERS:
		{
			RefreshWindow( true );
			break;
		}
		case ID_CPE_ON_PLAYER_CHANGED:
		{
			SCameraPositionWindowData data;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_CAMERA_POSITION_WINDOW, ID_WINDOW_GET_DIALOG_DATA, reinterpret_cast<uint32_t>(&data) );
			nCurrentPlayer = data.nPlayerIndex;
			RefreshWindow( true );
			return true;	
		}
		case ID_CPW_PARAM_TYPE_CHANGED:
		{
			SCameraPositionWindowData data;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_CAMERA_POSITION_WINDOW, ID_WINDOW_GET_DIALOG_DATA, reinterpret_cast<uint32_t>(&data) );
			nCurrentPlayer = data.nPlayerIndex;
			cameraPositions[nCurrentPlayer].bUseAnchorOnly = !data.bAllParams;
			SavePosition();
			RefreshWindow( true );
			return true;
		}
		case ID_CPW_ON_SAVE:
		{
			SavePosition();
			RefreshWindow( true );
			return true;
		}
		break;
	}
	return false;
}


bool CCameraPositionState::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CCameraPositionState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CCameraPositionState::UpdateCommand(), pbCheck == 0" );
	//
	switch ( nCommandID )
	{
		case ID_UPDATE_EDIT_PARAMETERS:
		{
			(*pbEnable) = true;
			(*pbCheck) = false;
			return true;
		}
		default:
		{
		}
	}
	return false;
}


int CCameraPositionState::GetPlayersCountFromDB()
{
	int nCount = 0;
	CManipulatorManager::GetValue( &nCount, pMapInfoEditor->GetViewManipulator(), "Players" );
	return nCount;
}

void CCameraPositionState::RefreshWindow( bool bGetFromDB )
{
	if ( bGetFromDB )
	{
		GetDBInfo();
	}
	//
	SCameraPositionWindowData data;
	if ( bGetFromDB )
	{
		data.nPlayerCount = GetPlayersCountFromDB();
		if ( nCurrentPlayer >= data.nPlayerCount )
		{
			nCurrentPlayer = 0;
		}
	}
	else
	{
		data.nPlayerCount = cameraPositions.size();
	}
	data.nPlayerIndex = nCurrentPlayer;
	data.bAllParams = !cameraPositions[nCurrentPlayer].bUseAnchorOnly;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_CAMERA_POSITION_WINDOW, 
																												ID_WINDOW_SET_DIALOG_DATA, 
																												reinterpret_cast<uint32_t>(&data) );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
}


void CCameraPositionState::Draw( CPaintDC *pPaintDC )
{
	CPtr<IEditorScene> pScene = EditorScene();
	if ( !pScene )
		return;

	ICamera *pCamera = Camera();
	//
	sceneDrawTool.Clear();
	CVec3 vPos;

	for ( int i = 0; i < cameraPositions.size(); ++i )
	{
		vPos = CVec3( cameraPositions[i].vAnchor );
		Vis2AI( &vPos );
		vPos.z = GetTerrainHeight( vPos.x, vPos.y );

		sceneDrawTool.DrawCircle( vPos, SELECTION_POINT_RADIUS, SELECTION_POINT_PARTS, SELECTION_COLOR, false );
		sceneDrawTool.DrawCircle( vPos, SCENE_OBJECT_SELECTION_RADIUS0, SCENE_OBJECT_SELECTION_PARTS, SCENE_OBJECT_SELECTION_COLOR, false );

		if ( i == nCurrentPlayer )
		{
			sceneDrawTool.DrawCircle( vPos, SELECTION_RADIUS0, SELECTION_PARTS, SELECTION_COLOR, false );
			sceneDrawTool.DrawCircle( vPos, SELECTION_RADIUS1, SELECTION_PARTS, SELECTION_COLOR, false );
			//
			if ( bMove )
			{
				for ( int i = 0; i < 10; ++i )
				{
					float fRad = i;
					sceneDrawTool.DrawCircle( vPos, SELECTION_POINT_RADIUS * (fRad/10), SELECTION_POINT_PARTS, SELECTION_COLOR, false );
				}
			}
		}
	}
	sceneDrawTool.Draw();
}


void CCameraPositionState::PostDraw( CPaintDC *pPaintDC )
{
	CPtr<IEditorScene> pScene = EditorScene();
  if ( !pScene )
		return;

	ICamera *pCamera = Camera();
	//
	for ( int i = 0; i < cameraPositions.size(); ++i )
	{
		CVec3 pos = CVec3( cameraPositions[i].vAnchor.x, cameraPositions[i].vAnchor.y, 0 ); 	
		Vis2AI( &pos );
		pos.z = GetTerrainHeight( pos.x, pos.y );
		
		CTransformStack ts = pCamera->GetTransform();
		
		CVec2 res = VNULL2;
		
		AI2Vis( &pos, pos );

		if ( TestRayInFrustrum( pos, ts, pScene->GetScreenRect(), &res ) )
			NDrawToolsDC::DrawLabelDC( pPaintDC, StrFmt( "Player %d camera", i ) , res );			
	}
}


void CCameraPositionState::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( cameraPositions.size() == 0 ) // no points
		return;

	CPtr<IEditorScene> pScene = EditorScene();
	if ( !pScene )
		return;

	CVec3 vPickedPosOnTerrain = VNULL3;
	Get2DPosOnMapHeights( &vPickedPosOnTerrain, CVec2(rMousePoint.x, rMousePoint.y) );

	bMove = false;
	float fDist = 0;
	for ( int i = 0; i < cameraPositions.size(); ++i )
	{
		CVec3 vCurCamAnchor( cameraPositions[i].vAnchor );
		vCurCamAnchor.z = 0;
		Vis2AI( &vCurCamAnchor );
		//
		fDist = fabs( vPickedPosOnTerrain - vCurCamAnchor );
		if ( fDist <= SELECTION_POINT_RADIUS )
		{
			bMove = true;
			nCurrentPlayer = i;
			RefreshWindow( true );
			return;
		}
	}
}


void CCameraPositionState::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( cameraPositions.size() == 0 ) // no points
		return;
	if ( nCurrentPlayer < 0 ) // empty selection
		return;

	CVec3 *pMovingPoint = &(cameraPositions[nCurrentPlayer].vAnchor);

	if ( bMove )
	{
		CVec2 vMapAISize = VNULL2;
		vMapAISize.x = pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE;
		vMapAISize.y = pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE;

		CVec3 vPickedPosOnTerrain = VNULL3;
		Get2DPosOnMapHeights( &vPickedPosOnTerrain, CVec2(rMousePoint.x, rMousePoint.y) );

		vPickedPosOnTerrain.x = Clamp( vPickedPosOnTerrain.x, 0.0f, vMapAISize.x );
		vPickedPosOnTerrain.y = Clamp( vPickedPosOnTerrain.y, 0.0f, vMapAISize.y );
		vPickedPosOnTerrain.z = 0;

		AI2Vis( &vPickedPosOnTerrain );
		(*pMovingPoint) = vPickedPosOnTerrain;
	}
}


void CCameraPositionState::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( !bMove )
		return;

	if ( cameraPositions.size() == 0 ) // no points
		return;

	SetStartCameraPositionInDB( pMapInfoEditor->GetViewManipulator(), cameraPositions[nCurrentPlayer], nCurrentPlayer );
	bMove = false;
}


void CCameraPositionState::GetDBInfo()
{
	cameraPositions.clear();

	if ( GetPlayersCountFromDB() > 0 )
	{
		for ( int i = 0; i < GetPlayersCountFromDB(); ++i )
		{
			SCameraPos camPos;
			if ( GetStartCameraPositionFromDB( &camPos, pMapInfoEditor->GetViewManipulator(), i ) )
				cameraPositions.push_back( camPos );
		}
	}
}



