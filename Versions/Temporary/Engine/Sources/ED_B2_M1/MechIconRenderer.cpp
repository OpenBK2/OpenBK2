#include "stdafx.h"

#include "3DLib/Transform.h"
#include "3Dmotor/FrameTransition.h"
#include "3Dmotor/GAnimation.hpp"
#include "3Dmotor/GRTShare.h"
#include "3Dmotor/GView.h"
#include "EditorOptions.h"
#include "IconCapture.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MechIconRenderer.h"
#include "ModelEditorSettings.h"
#include "SceneB2/Camera.h"
#include "SceneB2/Scene.h"
#include "Stats_B2_M1/DBVisObj.h"
#include "Stats_B2_M1/RPGStats.h"
#include "Stats_B2_M1/SceneModes.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "System/Commands.h"
#include "System/Dg.h"

#include <algorithm>
#include <cmath>

namespace
{
bool ReadShot( NDb::IObjMan *pManipulator, CModelEditorSettings::SCameraPlacement *pShot )
{
	return pManipulator && pManipulator->GetValue( "CameraPlacementIconShot.Anchor", &pShot->vAnchor ) &&
	       pManipulator->GetValue( "CameraPlacementIconShot.Distance", &pShot->fDistance ) &&
	       pManipulator->GetValue( "CameraPlacementIconShot.Pitch", &pShot->fPitch ) &&
	       pManipulator->GetValue( "CameraPlacementIconShot.Yaw", &pShot->fYaw ) &&
	       pManipulator->GetValue( "CameraPlacementIconShot.Fov", &pShot->fFOV ) && std::isfinite( pShot->vAnchor.x ) &&
	       std::isfinite( pShot->vAnchor.y ) && std::isfinite( pShot->vAnchor.z ) && std::isfinite( pShot->fDistance ) &&
	       std::isfinite( pShot->fPitch ) && std::isfinite( pShot->fYaw ) && std::isfinite( pShot->fFOV ) &&
	       pShot->fDistance > FP_EPSILON && pShot->fFOV > FP_EPSILON && pShot->fFOV < 179.0f;
}

bool HasVisiblePixels( const CArray2D<NGfx::SPixel8888> &capture, const NIconRendering::SOptions &options )
{
	CArray2D<NGfx::SPixel8888> image;
	if ( !NIconRendering::CropCapture( &image, capture, options ) )
	{
		return false;
	}
	for ( int y = 0; y < image.GetSizeY(); ++y )
	{
		for ( int x = 0; x < image.GetSizeX(); ++x )
		{
			if ( image[y][x].a != 0 )
			{
				return true;
			}
		}
	}
	return false;
}
} // namespace

bool NMechIconRenderer::Render( const CDBID &dbid,
                                const NIconRendering::SOptions &options,
                                CArray2D<NGfx::SPixel8888> *pCapture,
                                std::string *pError )
{
	const NDb::SMechUnitRPGStats *pStats = NDb::Get<NDb::SMechUnitRPGStats>( dbid );
	const NDb::SModel *pModel = nullptr;
	if ( pStats && pStats->pvisualObject )
	{
		for ( const auto &model : pStats->pvisualObject->models )
		{
			if ( model.eSeason == NDb::SEASON_SUMMER && model.pModel )
			{
				pModel = model.pModel;
				break;
			}
		}
		if ( !pModel && !pStats->pvisualObject->models.empty() )
		{
			pModel = pStats->pvisualObject->models.front().pModel;
		}
	}
	if ( !pModel || !pModel->pGeometry )
	{
		*pError = "no usable model or geometry";
		return false;
	}
	NGScene::CRTPtr *pTarget = NGScene::GetFrameTransitionCapture2();
	if ( !options.IsValid() || !pTarget || !pTarget->GetTexture() || !NGScene::Is3DActive() )
	{
		*pError = "icon render target is unavailable";
		return false;
	}

	CModelEditorSettings settings;
	Singleton<IUserDataContainer>()->Get()->SerializeSettings(
			settings, "UnitStats", SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );
	std::string szLight = settings.lightDBID.ToString();
	if ( szLight.empty() )
	{
		szLight = NEditorOptions::GetMiscString( "Light" );
	}
	if ( szLight.empty() )
	{
		szLight = NEditorOptions::GetLight( "SEASON_SUMMER", "DAY_DAY" );
	}
	const NDb::SAmbientLight *pLight = NDb::Get<NDb::SAmbientLight>( CDBID( szLight ) );
	if ( !pLight )
	{
		*pError = "no icon lighting configured in editor options";
		return false;
	}

	CObj<NGScene::IGameView> pView = NGScene::CreateNewView();
	pView->SetAmbient( pLight );
	// Match CModelState's placement and default pose, but keep all nodes in
	// this private view. Re-rendering is safe even while a map is open.
	const float fGridCentre = Clamp<int>( settings.nGridSize, 8, 64 ) * VIS_TILE_SIZE * 0.5f;
	const CVec3 vPosition = CVec3( fGridCentre, fGridCentre, 0 ) + CModelEditorSettings::vShift;
	const SFBTransform transform = MakeTransform( vPosition );
	const bool bAnimated = pModel->pSkeleton && !pModel->pSkeleton->animations.empty();
	CObj<NAnimation::ISkeletonAnimator> pAnimator;
	CObj<CCSTime> pTime = new CCSTime();
	pTime->Set( 0 );
	CObj<CCSFBTransform> pTransform = new CCSFBTransform();
	pTransform->Set( transform );
	if ( bAnimated )
	{
		NAnimation::SGrannySkeletonHandle handle;
		handle.pSkeleton = pModel->pSkeleton;
		handle.nModelInFile = 0;
		pAnimator = NAnimation::CreateSkeletonAnimator( handle, pTime );
		if ( !pAnimator )
		{
			*pError = "could not load the model skeleton";
			return false;
		}
		pAnimator->SetGlobalTransform( pTransform );
	}
	NGScene::IGameView::SMeshInfo meshInfo;
	pView->CreateMeshInfo( pModel, &meshInfo, bAnimated );
	SBound bound;
	bound.BoxExInit( vPosition + pModel->pGeometry->vCenter, pModel->pGeometry->vSize * 0.5f * FP_SQRT_2 );
	CObj<CObjectBase> pMesh =
			pView->CreateMesh( meshInfo, pTransform.GetPtr(), bound, NGScene::CMeshAnimStuff( pModel, pAnimator ) );
	if ( !pMesh )
	{
		*pError = "could not create the model render mesh";
		return false;
	}
	pView->WaitForLoad( true );

	CModelEditorSettings::SCameraPlacement shot;
	if ( !ReadShot( NDb::GetManipulator( dbid ), &shot ) )
	{
		// Fit the geometry's bounding sphere into the old 42-pixel window.
		// This conservative fit also covers tall units and long gun barrels.
		shot.vAnchor = vPosition + pModel->pGeometry->vCenter;
		shot.fPitch = 20.0f;
		shot.fYaw = 205.2f;
		shot.fFOV = 54.0f;
		const float fRadius = (std::max)( 0.1f, fabs( pModel->pGeometry->vSize ) * 0.5f );
		const float fHalfAngle = std::atan( std::tan( ToRadian( shot.fFOV ) * 0.5f ) * NIconRendering::DEFAULT_SIZE /
		                                    float( NIconRendering::TARGET_SIZE ) );
		shot.fDistance = fRadius / std::sin( fHalfAngle ) / 0.9f;
		NLog::Log( LT_IMPORTANT,
		           "Icon rendering: %s has no valid saved camera; using automatic framing.\n",
		           dbid.ToString().c_str() );
	}
	CObj<ICamera> pCamera = CreateCamera();
	pCamera->SetHandleType( true );
	pCamera->SwitchAutoPositioning( false );
	pCamera->SwitchManualScrolling( "IconRendering", false );
	pCamera->SetAnchor( shot.vAnchor );
	pCamera->SetPlacement( shot.fDistance, shot.fPitch, shot.fYaw );
	pCamera->SetFOV( NIconRendering::GetCaptureFOV( shot.fFOV, options ) );
	const float fNear = NGlobal::GetVar( "camera_near_plane", 5.0f );
	const float fFar = (std::max)( float( NGlobal::GetVar( "camera_far_plane", 500.0f ) ), shot.fDistance * 2.0f );
	pCamera->SetPerspectiveTransform( NIconRendering::TARGET_SIZE, NIconRendering::TARGET_SIZE, fNear, fFar );
	MarkNewDGFrame();
	pCamera->Update();
	NGScene::IGameView::SDrawInfo drawInfo;
	drawInfo.pTS = &pCamera->GetTransform();
	drawInfo.pTarget = pTarget->GetTexture();
	drawInfo.rtClear.vColor = CVec4( 0.5f, 0.5f, 0.5f, 0.0f );
	drawInfo.bUseDefaultClearColor = false;
	pView->Draw( drawInfo );
	NGfx::GetRenderTargetData( pCapture, pTarget->GetTexture() );
	if ( !HasVisiblePixels( *pCapture, options ) )
	{
		*pError = "rendered icon is empty; check the model and saved camera";
		return false;
	}
	return true;
}
