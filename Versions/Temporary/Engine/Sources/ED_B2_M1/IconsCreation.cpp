#include "stdafx.h"

#include "3Dmotor/FrameTransition.h"
#include "3Dmotor/GfxBuffers.h"
#include "3Dmotor/GRTShare.h"
#include "EditorScene.h"
#include "IconCapture.h"
#include "Image/ImageTGA.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/Interface_Exporter.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_MOD.h"
#include "MapEditorLib/Interface_Progress.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "ModelEditor.h"
#include "SceneB2/Camera.h"
#include "Stats_B2_M1/DBVisObj.h"
#include "Stats_B2_M1/RPGStats.h"
#include "StatsIconPak.h"
#include "StatsIconRenderer.h"
#include "System/VFSOperations.h"
#include "TargetMesh.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <filesystem>

namespace
{
void CreateIconTarget( const NIconRendering::SOptions &options )
{
	const float fScale = NIconRendering::GetFramingScale( options );
	NTargetMesh::CreateTargetMesh( options.nWidth / ( fScale * NIconRendering::TARGET_SIZE ),
	                               options.nHeight / ( fScale * NIconRendering::TARGET_SIZE ) );
}

bool CreateScreenShot( CArray2D<NGfx::SPixel8888> *pTexture,
                       IEditorScene *pScene,
                       const NIconRendering::SOptions &options )
{
	if ( !pScene )
	{
		return false;
	}

	// Prepare target texture
	NGScene::CRTPtr *pTarget = NGScene::GetFrameTransitionCapture2();
	if ( !pTarget || !pTarget->GetTexture() || !NGScene::Is3DActive() )
	{
		return false;
	}

	// Prepare scene
	CVec4 vPrevColor = pScene->SetBackgroundColor( CVec4( 0.5f, 0.5f, 0.5f, 0.0f ) );
	bool bPrevGetSizes = pScene->ToggleGetSizeFromTarget( true );
	NTargetMesh::DeleteTargetMesh();
	const float fPreviousFOV = Camera()->GetFOV();

	// Restore all preview state even if loading a model or readback fails.
	struct SRestorePreview
	{
		IEditorScene *pScene;
		CVec4 vColor;
		bool bGetSizes;
		float fFOV;
		NIconRendering::SOptions options;

		~SRestorePreview()
		{
			Camera()->SetFOV( fFOV );
			pScene->SetBackgroundColor( vColor );
			pScene->ToggleGetSizeFromTarget( bGetSizes );
			CreateIconTarget( options );
		}
	} restore{ pScene, vPrevColor, bPrevGetSizes, fPreviousFOV, options };

	Camera()->SetFOV( NIconRendering::GetCaptureFOV( fPreviousFOV, options ) );

	// Draw scene to target
	pScene->GetGView()->WaitForLoad( true );
	pScene->Draw( pTarget );

	// Getting data
	NGfx::GetRenderTargetData( pTexture, pTarget->GetTexture() );

	return pTexture->GetSizeX() >= options.nWidth && pTexture->GetSizeY() >= options.nHeight;
}

bool WriteTGA( const std::string &szFileName,
               const CArray2D<NGfx::SPixel8888> &texture,
               const NIconRendering::SOptions &options )
{
	CArray2D<NGfx::SPixel8888> cropped;
	if ( !NIconRendering::CropCapture( &cropped, texture, options ) )
	{
		return false;
	}
	CArray2D<uint32_t> image;
	image.SetSizes( options.nWidth, options.nHeight );
	for ( int y = 0; y < options.nHeight; ++y )
	{
		for ( int x = 0; x < options.nWidth; ++x )
		{
			image[y][x] = cropped[y][x].dwColor;
		}
	}

	std::error_code error;
	std::filesystem::create_directories( std::filesystem::u8path( szFileName ).parent_path(), error );
	if ( error )
	{
		return false;
	}
	// Write file
	CFileStream fileTGA( szFileName, CFileStream::WIN_CREATE );
	if ( !fileTGA.IsOk() )
	{
		return false;
	}

	NImage::SaveImageAsTGA( &fileTGA, image );
	fileTGA.Flush();

	return fileTGA.IsOk();
}

bool CreateTextureItem( const std::string &szTextureName, const std::string &szTgaName )
{
	// Creating object
	IResourceManager *pRM = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();

	CPtr<IManipulator> pTexMan = pRM->CreateObjectManipulator( "Texture", szTextureName );
	if ( !pTexMan )
	{
		pFolderCallback->InsertObject( "Texture", szTextureName );
		pTexMan = pRM->CreateObjectManipulator( "Texture", szTextureName );
	}

	if ( !pTexMan )
	{
		return false;
	}

	// Setting object properties
	bool bResult = true;
	bResult = bResult && CManipulatorManager::SetValue( "TEXTURE_2D", pTexMan, "Type", false );
	bResult = bResult && CManipulatorManager::SetValue( "CONVERT_ORDINARY", pTexMan, "ConversionType", false );
	bResult = bResult && CManipulatorManager::SetValue( "TF_8888", pTexMan, "Format", false );
	bResult = bResult && CManipulatorManager::SetValue( "CLAMP", pTexMan, "AddrType", false );
	bResult = bResult && CManipulatorManager::SetValue( 1, pTexMan, "NMips" );
	bResult = bResult && CManipulatorManager::SetValue( szTgaName, pTexMan, "SrcName", false );

	if ( !bResult )
	{
		return false;
	}

	if ( Singleton<IExporterContainer>()->ExportObject( pTexMan, "Texture", szTextureName, true, false ) != ER_SUCCESS )
	{
		return false;
	}
	// The exporter records the DDS's pixel dimensions. Unit icons must retain
	// their authored UI footprint while the renderer samples all of those pixels.
	return CManipulatorManager::SetValue( NIconRendering::DEFAULT_SIZE, pTexMan, "Width" ) &&
	       CManipulatorManager::SetValue( NIconRendering::DEFAULT_SIZE, pTexMan, "Height" );
}

bool SaveCameraPosition( NDb::IObjMan *pManipulator )
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

bool ExportIcon( const CDBID &dbid,
                 const CArray2D<NGfx::SPixel8888> &texture,
                 const NIconRendering::SOptions &options,
                 std::string *pError,
                 const std::string &szBatchFolder = "" )
{
	CPtr<NDb::IObjMan> pManipulator = NDb::GetManipulator( dbid );
	if ( !pManipulator )
	{
		*pError = "could not open the stats for editing";
		return false;
	}
	// Get unit name and generate directories and icon file names.
	const std::string &szUnitName = dbid.ToString();

	std::string szDirectory;
	CStringManager::SplitFileName( &szDirectory, 0, 0, szUnitName );
	// SplitFileName returns legacy backslashes, which are literal filename
	// characters on Linux. Normalize before touching the physical source tree.
	NFile::NormalizePath( &szDirectory );

	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	const std::string &szExportSourceFolder = pUserData->constUserData.szExportSourceFolder;

	// Batch variants can share a source directory. Give each record a unique
	// texture path, matching its archive namespace, instead of overwriting siblings.
	std::string szIconBase = szDirectory + "icon";
	if ( !szBatchFolder.empty() )
	{
		std::string szStatsPath = NDb::GetFileName( dbid );
		NFile::NormalizePath( &szStatsPath );
		szIconBase = szBatchFolder + "/" + szStatsPath.substr( 0, szStatsPath.size() - 4 ) + ".icon";
	}
	const std::string szIconTgaName = szIconBase + ".tga";
	const std::string szIconFullPath = NFile::JoinPath( szExportSourceFolder, szIconTgaName );
	const std::string szTextureItemName = szBatchFolder.empty() ? szIconBase + "_texture.xdb" : szIconBase + ".xdb";

	// Prepare icons data and write tga
	if ( !WriteTGA( szIconFullPath, texture, options ) )
	{
		*pError = "could not write " + szIconFullPath;
		return false;
	}

	// Create "Texture" item for icon and export it
	if ( !CreateTextureItem( szTextureItemName, szIconTgaName ) )
	{
		*pError = "could not export texture " + szTextureItemName;
		return false;
	}

	// Link texture with stats
	if ( !pManipulator->SetValue( "IconTexture", szTextureItemName ) )
	{
		*pError = "could not set IconTexture";
		return false;
	}
	return true;
}
} // namespace

void CModelEditor::CreateIcon()
{
	if ( GetObjectSet().szObjectTypeName != "MechUnitRPGStats" || GetObjectSet().objectNameSet.empty() )
	{
		return;
	}
	const CDBID dbid( GetObjectSet().objectNameSet.begin()->first );
	const NIconRendering::SOptions options = NIconRendering::LoadOptions();
	try
	{
		CArray2D<NGfx::SPixel8888> texture;
		std::string szError;
		if ( !CreateScreenShot( &texture, EditorScene(), options ) )
		{
			szError = "could not capture the model preview";
		}
		else if ( ExportIcon( dbid, texture, options, &szError ) && !SaveCameraPosition( NDb::GetManipulator( dbid ) ) )
		{
			szError = "could not save the icon camera";
		}
		if ( !szError.empty() )
		{
			NLog::Log( LT_ERROR, "Icon rendering: %s: %s\n", dbid.ToString().c_str(), szError.c_str() );
		}
	}
	catch ( const std::exception &error )
	{
		NLog::Log( LT_ERROR, "Icon rendering: %s: %s\n", dbid.ToString().c_str(), error.what() );
	}
	catch ( ... )
	{
		NLog::Log( LT_ERROR, "Icon rendering: %s: unexpected error\n", dbid.ToString().c_str() );
	}
}

void CModelEditor::ReRenderAllIcons( NIconRendering::EBatchType eType )
{
	const NIconRendering::SOptions options = NIconRendering::LoadOptions();
	const auto &batch = NIconRendering::BATCHES[eType];
	std::vector<CDBID> units;
	if ( !NDb::GetObjectsList( &units, batch.pszTypeName ) )
	{
		NLog::Log( LT_ERROR, "Icon rendering: could not enumerate %s objects.\n", batch.pszTypeName );
		return;
	}
	// Snapshot the IDs before exporting, since creating textures changes the database.
	std::sort( units.begin(), units.end(), []( const CDBID &a, const CDBID &b ) { return a.ToString() < b.ToString(); } );
	NLog::Log( LT_IMPORTANT,
	           "Re-rendering %d %s icons at %d x %d.\n",
	           static_cast<int>( units.size() ),
	           batch.pszTypeName,
	           options.nWidth,
	           options.nHeight );
	NProgress::Create( true );

	struct SProgressScope
	{
		~SProgressScope()
		{
			NProgress::Destroy();
		}
	} progress;

	NProgress::SetRange( 0, static_cast<int>( units.size() ) );
	std::unique_ptr<CStatsIconPak> pPak;
	if ( options.*batch.pSeparatePak )
	{
		pPak = std::make_unique<CStatsIconPak>( eType );
	}
	int nSucceeded = 0;
	int nFailed = 0;
	for ( const CDBID &dbid : units )
	{
		NProgress::SetMessage( "Rendering icon: " + dbid.ToString() );
		std::string szError;
		try
		{
			CArray2D<NGfx::SPixel8888> texture;
			if ( NStatsIconRenderer::Render( dbid, options, &texture, &szError ) &&
			     ( pPak ? pPak->Add( dbid, texture, options, &szError )
			            : ExportIcon( dbid, texture, options, &szError, batch.pszArchiveBase ) ) )
			{
				++nSucceeded;
			}
			else if ( szError.empty() )
			{
				szError = "could not render or export the icon";
			}
		}
		catch ( const std::exception &error )
		{
			szError = error.what();
		}
		catch ( ... )
		{
			szError = "unexpected error";
		}
		if ( !szError.empty() )
		{
			++nFailed;
			NLog::Log( LT_ERROR, "Icon rendering: %s: %s\n", dbid.ToString().c_str(), szError.c_str() );
		}
		NProgress::IteratePosition();
	}
	if ( pPak )
	{
		const std::string szArchiveName = std::string( batch.pszArchiveBase ) + ".pak";
		const std::string szDestination = NFile::JoinPath(
				Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ), szArchiveName );
		std::string szError;
		try
		{
			NProgress::SetMessage( "Saving " + szArchiveName );
			if ( pPak->Save( szDestination, &szError ) )
			{
				NLog::Log( LT_IMPORTANT,
				           "Icon rendering finished: %d icons saved, %d failed. Archive: %s\n",
				           nSucceeded,
				           nFailed,
				           NVFS::GetWritePath( NVFS::GetMainFileCreator(), szDestination ).c_str() );
				return;
			}
		}
		catch ( const std::exception &error )
		{
			szError = error.what();
		}
		catch ( ... )
		{
			szError = "unexpected error saving the icon archive";
		}
		NLog::Log( LT_ERROR, "Icon rendering: could not save %s: %s\n", szDestination.c_str(), szError.c_str() );
		return;
	}
	NLog::Log(
			LT_IMPORTANT,
			"Icon rendering finished: %d succeeded, %d failed. Save editor changes to keep updated texture records and links.\n",
			nSucceeded,
			nFailed );
}

void CModelEditor::RestoreCameraParametersFromStats()
{
	// Getting unit manipulator
	const NDb::SMechUnitRPGStats *pUnitStats =
			NDb::Get<NDb::SMechUnitRPGStats>( GetObjectSet().objectNameSet.begin()->first );
	if ( !pUnitStats )
	{
		return;
	}

	NDb::IObjMan *pManipulator = NDb::GetManipulator( pUnitStats->GetDBID() );
	if ( !pManipulator )
	{
		return;
	}

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
	{
		return;
	}

	// Parameters is not defined in this stats
	if ( fDistance < FP_EPSILON )
	{
		return;
	}

	// Setting parameters
	editorSettings.currentCamera.vAnchor = vAnchor;
	editorSettings.currentCamera.fDistance = fDistance;
	editorSettings.currentCamera.fPitch = fPitch;
	editorSettings.currentCamera.fYaw = fYaw;
	editorSettings.currentCamera.fFOV = fFOV;
}

void CModelEditor::CreateTarget()
{
	CreateIconTarget( NIconRendering::LoadOptions() );
}

void CModelEditor::DeleteTarget()
{
	NTargetMesh::DeleteTargetMesh();
}
