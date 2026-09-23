#include "stdafx.h"
#include <fmt/format.h>
#include "Misc/StrProc.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "SceneB2/Scene.h"
#include "libdb/ResourceManager.h"

#include "MapInfoEditor.h"
#include "EditorOptions.h"
#include "EditorMethods.h"

//#include "../Input/Input.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/Interface_Exporter.h"
#include "SeasonMnemonics.h"


//#include "MiniMap_Addons.h"
#include "B2_M1_Terrain/fmtVSO.h"
#include "Image/Targa.h"
#include "MinimapImage.h"
#include "SeasonMnemonics.h"
#include "System/VFSOperations.h"
#include "System/FilePath.h"

#include <zconf.h>

namespace NImage
{
	template <> 
	inline void __fill_tga_header<NImage::SColor>( STGAFileHeader *pHdr )
	{
		pHdr->cImageType = TGAIT_TRUE_COLOR;
		pHdr->imagespec.descriptor.cAlphaChannelBits = 8;
	}
};


bool CMapInfoEditor::CreateMinimapImage()
{
	szSaveError.clear();
	IResourceManager *pRM = Singleton<IResourceManager>();
	std::string materialName, textureName, source;
	CManipulatorManager::GetValue( &materialName, GetViewManipulator(), "MiniMap" );
	CPtr<IManipulator> material;
	if ( !materialName.empty() && materialName != " " )
		material = pRM->CreateObjectManipulator( "Material", materialName );
	if ( material ) CManipulatorManager::GetValue( &textureName, material, "Texture" );
	CPtr<IManipulator> texture;
	if ( !textureName.empty() && textureName != " " )
		texture = pRM->CreateObjectManipulator( "Texture", textureName );
	if ( texture ) CManipulatorManager::GetValue( &source, texture, "SrcName" );
	if ( !pMapInfo || !texture || source.empty() || source == " " )
	{
		szSaveError = "The map has no valid minimap material, texture or source image path.";
		return false;
	}
	const std::string minimapName = NEditorOptions::GetMinimap( typeSeasonMnemonics.GetMnemonic( pMapInfo->eSeason ) );
	const NDb::SMinimap *minimap = NDb::Get<NDb::SMinimap>( CDBID( minimapName ) );
	if ( !pMapInfo->pTerraSet || !minimap )
	{
		szSaveError = "The terrain set or minimap settings are missing: " + minimapName;
		return false;
	}
	// SrcName is a database file path, already resolved relative to its XDB.
	// Use the same mounted mod source that the texture exporter will read.
	NMinimapImage::CCreateParameterList parameters;
	parameters.emplace_back( source, CTPoint<int>( 256, 256 ) );
	if ( !NMinimapImage::Create( pMapInfo, Scene()->GetTerraManager()->GetTerraInfo(), minimap, parameters, &szSaveError ) )
		return false;
	const EXPORT_RESULT result = Singleton<IExporterContainer>()->ExportObject( texture, "Texture", textureName, true, false );
	if ( result != ER_SUCCESS )
	{
		szSaveError = "Could not export the minimap DDS:\n" +
			NVFS::GetWritePath( NVFS::GetMainFileCreator(), NFile::CutFileExt( textureName, "xdb" ) + ".dds" ) +
			"\nSee the Log window for the texture export error.";
		return false;
	}
	return true;
}
