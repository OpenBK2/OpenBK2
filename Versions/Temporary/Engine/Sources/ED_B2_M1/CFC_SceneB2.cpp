#include "stdafx.h"
#include <fmt/format.h>
#include "MapEditorLib/ResourceDefines.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "ResourceDefines.h"
#include "CommandHandlerDefines.h"

#include "CFC_SceneB2.h"
//
#include "EditorScene.h"
#include "SceneB2/Camera.h"
#include "Main/Profiles.h"
#include "3Dmotor/Gfx.h"
#include "System/GResource.h"
#include "Sound/SFX.h"
#include "Main/MainLoop.h"
#include "Input/GameMessage.h"
#include "MapEditorLib/Interface_Logger.h"
#include "3Dmotor/GSceneUtils.h"
#include "Stats_B2_M1/SceneModes.h"
#include "SceneB2/TerraGen.h"
#include "SceneB2/TerrainInfo.h"
#include "SceneB2/CameraScriptMutators.h"

#include "DrawToolsDC.h"
#include "System/GResource.h"

#include <cstdint>

#include <zconf.h>

namespace 
{
	const float fHorisontalStep					= 1.0f;
	const float fVerticalStep						= 1.0f;
	const float fPitchStep							= 1.0f;
	const float fYawStep								= 1.0f;
	const float fDistanceStep						= 1.0f;

	const float fFastRatio							= 5.0f;

	const float fDefaultDistance				= 170.0f;
	const float fDefaultPitch						= 45.0f;
	const float fDefaultYaw							= 45.0f;
	const float fDefaultFOV							= 26.0f;
};


CCFCSceneB2::CCFCSceneB2()
{
}


CCFCSceneB2::~CCFCSceneB2()
{
}


bool CCFCSceneB2::OnCreateChildFrameWnd() 
{
	NGScene::SFLB3_RunResourceLoadingThread();

	if ( !NGfx::Init3D( m_hWnd ) )
	{
		// DX not found
		NLog::GetLogger()->Log( LT_ERROR, fmt::format( "CCFCSceneB2::OnCreateChildFrameWnd(): Failed to initialize Direct3D9" ) );
		return false;
	}
	// init input system
	NInput::InitInput( AfxGetMainWnd()->GetSafeHwnd() );
	Singleton<ISFX>()->Init( m_hWnd, 0, SFX_OUTPUT_DSOUND, 44100, 32 );
	//
	NProfile::LoadProfile();
	NGlobal::LoadConfig( NMainLoop::GetBaseDir() + "profiles\\autoexec.cfg" );

	// Small trick to force editor have the higthest graphic quality
	NGlobal::ProcessCommand( L"set_quality 1" );
	// init 
	if ( !EditorScene()->SetupMode( SCENE_MODE_WINDOWED, true ) )
	{
		NLog::GetLogger()->Log( LT_ERROR, "Failed to set display mode. Desktop must be in 32bit mode\n" );
		return false;
	}
	//
	return true;
}


void CCFCSceneB2::OnDestroyChildFrameWnd() 
{
	NInput::DoneInput();
  NGfx::Done3D();
}


void CCFCSceneB2::OnPreDrawChildFrameWnd() 
{
	NGfx::CheckBackBufferSize();
}


void CCFCSceneB2::OnDrawChildFrameWnd() 
{
	if ( IsSceneUpdateEnabled() )
	{
		EditorScene()->Draw( 0 );
	}
}


void CCFCSceneB2::OnResizeChildFrameWnd( int cx, int cy )
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAP_INFO_EDITOR, ID_UPDATE_SCENE_SIZE, PackCoords(CVec2(cx, cy)) );
}


bool CCFCSceneB2::KeyPressed( unsigned nChar, uintptr_t dwData )
{
	return ( ( dwData == nChar ) || ( ( GetAsyncKeyState( nChar ) & 0x8000 ) > 0 ) );
}


void CCFCSceneB2::ResetCamera( bool bAll )
{
	if ( ICamera *pCamera = Camera() )
	{
		if ( CScriptMoviesMutatorHolder *pMoviesHolder = pCamera->GetScriptMutatorsHolder() )
		{
			pMoviesHolder->Stop();
		}
		if ( bAll && EditorScene() && EditorScene()->GetTerraManager() )
		{
			pCamera->SetAnchor( CVec3(EditorScene()->GetTerraManager()->GetTilesCountX(), EditorScene()->GetTerraManager()->GetTilesCountY(), 0 ) * DEF_TILE_SIZE / 2.0f );
		}
		pCamera->SetPlacement( fDefaultDistance, fDefaultPitch, fDefaultYaw );
		pCamera->SetFOV( fDefaultFOV );
	}
	RedrawWindow();
}


void CCFCSceneB2::UpdateCameraPosition( uintptr_t dwData )
{
	if ( IsPacked2DCoords( dwData ) )
	{
		const CVec2 vCameraPosition2 = UnPackCoords( dwData );
		ICamera *pCamera = Camera();
		pCamera->SetAnchor( CVec3( vCameraPosition2.x, vCameraPosition2.y, 0 ) );
		RedrawWindow();
	}
	else
	{
		NLog::GetLogger()->Log( LT_ERROR, fmt::format( "CCFCSceneB2::UpdateCameraPosition(): dwData is not packed 2D coords" ) );
	}
}


void CCFCSceneB2::UpdateCamera( uintptr_t dwData )
{
	if ( ICamera *pCamera = Camera() )
	{
		const SHMatrix matView = pCamera->GetViewMatrix();
		CVec3 vAxisX = CVec3( matView._11, matView._21, matView._31 );
		CVec3 vAxisY = CVec3( matView._12, matView._22, matView._32 );
		CVec3 vAxisZ = CVec3( matView._13, matView._23, matView._33 );

		CVec3 vCameraPosition = pCamera->GetAnchor();
		float fDistance = fDefaultDistance;
		float fPitch = fDefaultPitch;
		float fYaw = fDefaultYaw;
		pCamera->GetPlacement( &fDistance, &fPitch, &fYaw );
		if ( KeyPressed( VK_CONTROL, dwData ) )
		{
			float fYawDifference = 0.0f;
			float fPitchDifference = 0.0f;
			if ( KeyPressed( VK_LEFT, dwData ) )
			{
				fYawDifference += fYawStep;
			}
			else if ( KeyPressed( VK_RIGHT, dwData ) )
			{
				fYawDifference -= fYawStep;
			}
			if ( KeyPressed( VK_UP, dwData ) )
			{
				fPitchDifference += fPitchStep;
			}
			else if ( KeyPressed( VK_DOWN, dwData ) )
			{
				fPitchDifference -= fPitchStep;
			}
			if ( !KeyPressed( VK_SHIFT, dwData ) )
			{
				fYawDifference *= fFastRatio;
				fPitchDifference *= fFastRatio;
			}
			fYaw += fYawDifference;
			fPitch += fPitchDifference;
		}
		else
		{
			const CQuat quat = CQuat( fYaw, V3_AXIS_Z ) * CQuat( fPitch, V3_AXIS_X );
			CVec3 vDifference = VNULL3;
			float fFwd = 0.0f;
			float fStrafe = 0.0f;
			if ( KeyPressed( VK_LEFT, dwData ) )
			{
				fStrafe = ( -1.0f ) * fVerticalStep * ( fDistance / fDefaultDistance );
			}
			else if ( KeyPressed( VK_RIGHT, dwData ) )
			{
				fStrafe = fVerticalStep * ( fDistance / fDefaultDistance );
			}
			if ( KeyPressed( VK_UP, dwData ) )
			{
				fFwd = fHorisontalStep * ( fDistance / fDefaultDistance );
			}
			else if ( KeyPressed( VK_DOWN, dwData ) )
			{
				fFwd = ( -1.0f ) * fHorisontalStep * ( fDistance / fDefaultDistance );
			}
			if ( !KeyPressed( VK_SHIFT, dwData ) )
			{
				fFwd *= fFastRatio;
				fStrafe *= fFastRatio;
			}
			// calculate anchor translation
			vAxisZ.z = 0;
			Normalize( &vAxisZ );
			vDifference += vAxisZ * fFwd;
			// strafe
			vAxisX.z = 0;
			Normalize( &vAxisX );
			vDifference += vAxisX * fStrafe;
			//
			vCameraPosition += vDifference;
		}
		{
			float fDistanceDifference = 0.0f;
			bool bDistanceChanged = false;
			if ( KeyPressed( VK_PRIOR, dwData ) )
			{
				fDistanceDifference -= fDistanceStep * ( fDistance / fDefaultDistance );
				bDistanceChanged = true;
			}
			else if ( KeyPressed( VK_NEXT, dwData ) )
			{
				fDistanceDifference += fDistanceStep * ( fDistance / fDefaultDistance );
				bDistanceChanged = true;
			}
			if ( bDistanceChanged )
			{
				if ( !KeyPressed( VK_SHIFT, dwData ) )
				{
					fDistanceDifference *= fFastRatio;
				}
				fDistance += fDistanceDifference;
			}
		}

		pCamera->SetAnchor( vCameraPosition );
		pCamera->SetPlacement( fDistance, fPitch, fYaw );
	}
	RedrawWindow();
}


void CCFCSceneB2::ClearScene()
{
	EditorScene()->ClearScene( SCENE_MISSION );
	ClearHoldQueue();
	NGScene::CResourceFileOpener::Clear();
}


bool CCFCSceneB2::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	switch( nCommandID )
	{
		case ID_SCENE_CLEAR:
			ClearScene();
			return true;
		case ID_SCENE_RESET_CAMERA:
			ResetCamera( dwData > 0 );
			return true;
		case ID_SCENE_UPDATE_CAMERA:
			UpdateCamera( dwData );
			return true;
		case ID_SCENE_SET_CAMERA_POSITION:
			UpdateCameraPosition( dwData );
			return true;
	}
	return CChildFrameWndBase::HandleCommand( nCommandID, dwData );
}


bool CCFCSceneB2::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CCFCSceneB2::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CCFCSceneB2::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_SCENE_CLEAR:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_SCENE_RESET_CAMERA:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_SCENE_UPDATE_CAMERA:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_SCENE_SET_CAMERA_POSITION:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
	}
	return CChildFrameWndBase::UpdateCommand( nCommandID, pbEnable, pbCheck );
}


void CCFCSceneB2::DrawFocus( CPaintDC *pDC )
{
	CRect clientRect;
	GetClientRect( &clientRect );
	clientRect.right -= 1;
	clientRect.bottom -= 1;
	//
	CBrush solidBrush;
	solidBrush.CreateSolidBrush( RGB( 255, 30, 30 ) ); 
	pDC->FrameRect( &clientRect, &solidBrush );
}


void CCFCSceneB2::DrawStatistic( CPaintDC *pDC )
{
	CVec2 vScreenPos( 5, 5 );
	const int nSpacing = 15;

	//NGScene::SRenderStats sStats;
	//NGScene::GetRenderStats( &sStats );
	//pDC->TextOut( nX, nY + nYStep * 0, fmt::format( "nVertices: {}", sStats.nVertices ) );
	//pDC->TextOut( nX, nY + nYStep * 1, fmt::format( "nTris: {}", sStats.nTris ) );

	if ( CPtr<ICamera> pCamera = Camera() )
	{
		float fCamDist = 0.0f;
		float fCamPitch = 0.0f;
		float fCamYaw = 0.0f;
		float fFOV = pCamera->GetFOV();

		pCamera->GetPlacement( &fCamDist, &fCamPitch, &fCamYaw );
		CVec3 vCameraAnchor = pCamera->GetAnchor();
		Vis2AI( &vCameraAnchor );

		NDrawToolsDC::DrawTextDC( pDC, "Camera params:", vScreenPos );
		vScreenPos += V2_AXIS_Y * nSpacing;
		NDrawToolsDC::DrawTextDC( pDC, fmt::format("Position: ({:.0f}, {:.0f}, {:.0f})", vCameraAnchor.x, vCameraAnchor.y, vCameraAnchor.z ), vScreenPos );
		vScreenPos += V2_AXIS_Y * nSpacing;
		NDrawToolsDC::DrawTextDC( pDC, fmt::format("Distance: {:.0f}", fCamDist), vScreenPos );
		vScreenPos += V2_AXIS_Y * nSpacing;
		NDrawToolsDC::DrawTextDC( pDC, fmt::format("Yaw: {:.0f}", fCamYaw), vScreenPos );
		vScreenPos += V2_AXIS_Y * nSpacing;
		NDrawToolsDC::DrawTextDC( pDC, fmt::format("Pitch: {:.0f}", fCamPitch), vScreenPos );
		vScreenPos += V2_AXIS_Y * nSpacing;
		NDrawToolsDC::DrawTextDC( pDC, fmt::format("FOV: {:.0f}", fFOV), vScreenPos );
	}
}


void CCFCSceneB2::DrawFrameBorders( CPaintDC *pDC )
{
	NDrawToolsDC::DrawFrameBorders( pDC, rectBorder1, rectBorder2, rectWindow );
}


// basement storage  


