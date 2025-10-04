#include "stdafx.h"

#include "ModelEditor.h"
#include "EditorScene.h"
#include "TargetMesh.h"

#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/Interface_Exporter.h"
#include "MapEditorLib/ManipulatorManager.h"

#include "libdb/ObjMan.h"
#include "libdb/EditorDb.h"

#include "libdb/ResourceManager.h"

#include "3Dmotor/GRTShare.h"
#include "3Dmotor/FrameTransition.h"
#include "3Dmotor/GfxBuffers.h"

#include "Image/ImageTGA.h"

#include "stats_b2_m1/RPGStats.h"
#include "stats_b2_m1/DBVisObj.h"

#include "SceneB2/Camera.h"

const int N_ICONS_SIZE_X = 42;
const int N_ICONS_SIZE_Y = 42;
const int N_TARGET_SIZE_X = 512;
const int N_TARGET_SIZE_Y = 512;

static bool CreateScreenShot( CArray2D<NGfx::SPixel8888> *pTexture, IEditorScene *pScene )
{
	if ( !pScene )
		return false;

	// Prepare target texture
	NGScene::CRTPtr *pTarget = NGScene::GetFrameTransitionCapture2();
	if ( !pTarget )
		return false;

	// Prepare scene
	CVec4 vPrevColor = pScene->SetBackgroundColor( CVec4( 0.5f, 0.5f, 0.5f, 0.0f ) );
	bool bPrevGetSizes = pScene->ToggleGetSizeFromTarget( true );
	NTargetMesh::DeleteTargetMesh();

	// Draw scene to target
	pScene->Draw( pTarget );

	// Restore scene
	pScene->SetBackgroundColor( vPrevColor );
	pScene->ToggleGetSizeFromTarget( bPrevGetSizes );
	NTargetMesh::CreateTargetMesh( N_ICONS_SIZE_X/(float)N_TARGET_SIZE_X, N_ICONS_SIZE_Y/(float)N_TARGET_SIZE_Y );

	// Getting data
	NGfx::GetRenderTargetData( pTexture, pTarget->GetTexture() );	

	return true;
}

static bool WriteTGA( const string &szFileName, const CArray2D<NGfx::SPixel8888> &texture )
{
	const int nIconSizeX = N_ICONS_SIZE_X;
	const int nIconSizeY = N_ICONS_SIZE_Y;

	// Prepare image for writing
	if ( nIconSizeX > texture.GetSizeX() || nIconSizeY > texture.GetSizeY() )
		return false;

	int nStartX = ( texture.GetSizeX() - nIconSizeX ) / 2;
	int nStartY = ( texture.GetSizeY() - nIconSizeY ) / 2;

	CArray2D<DWORD> image;
	image.SetSizes( nIconSizeX, nIconSizeY );

	for ( int iX = 0; iX < nIconSizeX; ++iX )
		for ( int iY = 0; iY < nIconSizeY; ++iY )
		{
			image[iX][iY] = texture[iX + nStartX][iY+nStartY].dwColor;
		}

	// Write file
	CFileStream fileTGA( szFileName, CFileStream::WIN_CREATE );
	if ( !fileTGA.IsOk() )
		return false;

	NImage::SaveImageAsTGA( &fileTGA, image);

	return true;
}

static bool CreateTextureItem( const string &szTextureName, const string &szTgaName )
{
	// Creating object
	IResourceManager *pRM = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();

	CPtr<IManipulator> pTexMan = pRM->CreateObjectManipulator("Texture", szTextureName);
	if ( !pTexMan )
	{
		pFolderCallback->InsertObject( "Texture", szTextureName);
		pTexMan = pRM->CreateObjectManipulator("Texture", szTextureName);
	}

	if ( !pTexMan )
		return false;

	// Setting object properties
	bool bResult = true;
	bResult = bResult && CManipulatorManager::SetValue( "TEXTURE_2D", pTexMan, "Type", false );
	bResult = bResult && CManipulatorManager::SetValue( "CONVERT_ORDINARY", pTexMan, "ConversionType", false );
	bResult = bResult && CManipulatorManager::SetValue( "TF_8888", pTexMan, "Format", false );
	bResult = bResult && CManipulatorManager::SetValue( "CLAMP", pTexMan, "AddrType", false );
	bResult = bResult && CManipulatorManager::SetValue( 1, pTexMan, "NMips" );
	bResult = bResult && CManipulatorManager::SetValue( szTgaName, pTexMan, "SrcName", false );

	if ( !bResult )
		return false;

	// Export object
	Singleton<IExporterContainer>()->ExportObject( pTexMan, "Texture", szTextureName, true, false );

	return true;
}

static bool SaveCameraPosition( NDb::IObjMan *pManipulator )
{
	// Getting parameters from camera
	float fDistance;
	float fPitch;
	float fYaw;	
	Singleton<ICamera>()->GetPlacement( &fDistance, &fPitch, &fYaw );

	CVec3 vAnchor = Singleton<ICamera>()->GetAnchor();
	float fFOV = Singleton<ICamera>()->GetFOV();

	// Save parameters to stats
	bool bResult = true;
	bResult = bResult && pManipulator->SetValue( "CameraPlacementIconShot.Anchor", vAnchor );
	bResult = bResult && pManipulator->SetValue( "CameraPlacementIconShot.Distance", fDistance );
	bResult = bResult && pManipulator->SetValue( "CameraPlacementIconShot.Pitch", fPitch );
	bResult = bResult && pManipulator->SetValue( "CameraPlacementIconShot.Yaw", fYaw );
	bResult = bResult && pManipulator->SetValue( "CameraPlacementIconShot.Fov", fFOV );

	return bResult;
}

void CModelEditor::CreateIcon()
{
	const NDb::SMechUnitRPGStats *pUnitStats = NDb::Get<NDb::SMechUnitRPGStats>( GetObjectSet().objectNameSet.begin()->first );
	if ( !pUnitStats )
		return;

	// Creating screenshot
	CArray2D<NGfx::SPixel8888> texture;
	if ( !CreateScreenShot( &texture, EditorScene() ) )
		return;

	// Get unit name and generate directories and icon file names.
	const string &szUnitName = pUnitStats->GetDBID().ToString();

	string szDirectory;
	CStringManager::SplitFileName( &szDirectory, 0, 0, szUnitName );

	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	const string &szExportSourceFolder = pUserData->constUserData.szExportSourceFolder;

	const string szIconTgaName = szDirectory + "icon.tga";
	const string szIconFullPath = szExportSourceFolder + szIconTgaName;
	const string szTextureItemName = szDirectory + "icon_texture.xdb";

	// Prepare icons data and write tga
	if ( !WriteTGA( szIconFullPath, texture ) )
		return;

	// Create "Texture" item for icon and export it
	if ( !CreateTextureItem( szTextureItemName, szIconTgaName ) )
		return;

	// Link texture with stats
	NDb::IObjMan *pManipulator = NDb::GetManipulator( pUnitStats->GetDBID() );
	if ( !pManipulator )
		return;

	pManipulator->SetValue( "IconTexture", szTextureItemName );

	// Save camera position to object
	if ( !SaveCameraPosition( pManipulator ) )
		return;
}

void CModelEditor::RestoreCameraParametersFromStats()
{
	// Getting unit manipulator
	const NDb::SMechUnitRPGStats *pUnitStats = NDb::Get<NDb::SMechUnitRPGStats>( GetObjectSet().objectNameSet.begin()->first );
	if ( !pUnitStats )
		return;

	NDb::IObjMan *pManipulator = NDb::GetManipulator( pUnitStats->GetDBID() );
	if ( !pManipulator )
		return;

	// Getting camera parameters
	float fDistance = 0.0f;
	float fPitch = 0.0f;
	float fYaw = 0.0f;	
	CVec3 vAnchor( VNULL3 );
	float fFOV = 0.0f;

	bool bResult = true;
	bResult = bResult && pManipulator->GetValue( "CameraPlacementIconShot.Anchor", &vAnchor );
	bResult = bResult && pManipulator->GetValue( "CameraPlacementIconShot.Distance", &fDistance );
	bResult = bResult && pManipulator->GetValue( "CameraPlacementIconShot.Pitch", &fPitch );
	bResult = bResult && pManipulator->GetValue( "CameraPlacementIconShot.Yaw", &fYaw );
	bResult = bResult && pManipulator->GetValue( "CameraPlacementIconShot.Fov", &fFOV );

	if ( !bResult )
		return;

	// Parameters is not defined in this stats
	if ( fDistance < FP_EPSILON )
		return;

	// Setting parameters
	editorSettings.currentCamera.vAnchor = vAnchor;
	editorSettings.currentCamera.fDistance = fDistance;
	editorSettings.currentCamera.fPitch = fPitch;
	editorSettings.currentCamera.fYaw = fYaw;
	editorSettings.currentCamera.fFOV = fFOV;
}

void CModelEditor::CreateTarget()
{
	NTargetMesh::CreateTargetMesh( N_ICONS_SIZE_X/(float)N_TARGET_SIZE_X, N_ICONS_SIZE_Y/(float)N_TARGET_SIZE_Y );
}

void CModelEditor::DeleteTarget()
{
	NTargetMesh::DeleteTargetMesh();
}


