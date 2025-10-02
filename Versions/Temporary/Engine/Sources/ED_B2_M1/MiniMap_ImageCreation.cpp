#include "StdAfx.h"
#include "misc/strproc.h"
#include "misc/2darray.h"
#include "stats_b2_m1/iconsset.h"
#include "sceneb2/scene.h"
#include "libdb/resourcemanager.h"

#include "MapInfoEditor.h"
#include "EditorOptions.h"
#include "EditorMethods.h"

//#include "../Input/Input.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/Interface_Exporter.h"
#include "SeasonMnemonics.h"


//#include "MiniMap_Addons.h"
#include "B2_M1_Terrain/fmtvso.h"
#include "Image/Targa.h"
#include "MinimapImage.h"
#include "SeasonMnemonics.h"
#include "System/VFSOperations.h"

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
	IResourceManager *pRM = Singleton<IResourceManager>();
	string szMiniMapMaterialName;
	CManipulatorManager::GetValue( &szMiniMapMaterialName, GetViewManipulator(), "MiniMap" );
	if ( !szMiniMapMaterialName.empty() && szMiniMapMaterialName != " " ) 
	{
		if ( CPtr<IManipulator> pMaterialMan = pRM->CreateObjectManipulator("Material", szMiniMapMaterialName) )
		{
			string szMiniMapTextureName;
			CManipulatorManager::GetValue( &szMiniMapTextureName, pMaterialMan, "Texture" );
			if ( !szMiniMapTextureName.empty() && szMiniMapTextureName != " " ) 
			{
				// delete texture source and destination
				if ( CPtr<IManipulator> pTexMan = pRM->CreateObjectManipulator("Texture", szMiniMapTextureName) )
				{
					string szSrcName;
					if ( CManipulatorManager::GetValue( &szSrcName, pTexMan, "SrcName" ) != false && !szSrcName.empty() && szSrcName != " " )
					{
						const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
						NMinimapImage::CCreateParameterList createParameterList;
						createParameterList.insert( createParameterList.begin(),
																				NMinimapImage::SCreateParameter( pUserData->constUserData.szExportSourceFolder + szSrcName, CTPoint<int>( 256, 256 ) ) );
						//
						bool bTerrainLoaded = false;
						const STerrainInfo *pTerrainInfo = Scene()->GetTerraManager()->GetTerraInfo();
						//const string szTerrainInfoFileName = GetTerrainBinFileName( pMapInfo );
						//if ( !szTerrainInfoFileName.empty() )
						//{
						//	CFileStream stream( NVFS::GetMainVFS(), szTerrainInfoFileName );
						//	if ( stream.IsOk()  )
						//	{
						//		CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_READ );
						//		NI_ASSERT( pSaver != 0, StrFmt( "Can't open stream \"%s\" to read map", szTerrainInfoFileName.c_str() ) );
						//		pSaver->Add( 1, &terrainInfo );
						//		bTerrainLoaded = true;
						//	}
						//}
						//
						const string szMinimapName = NEditorOptions::GetMinimap( typeSeasonMnemonics.GetMnemonic( pMapInfo->eSeason ) );
						const NDb::SMinimap *pMinimap = NDb::Get<NDb::SMinimap>( CDBID( szMinimapName ) );
						if ( pMapInfo && pMapInfo->pTerraSet && pMinimap )
						{
							NMinimapImage::Create( pMapInfo, pTerrainInfo, pMinimap, createParameterList );  
							// export new texture
							Singleton<IExporterContainer>()->ExportObject( pTexMan, "Texture", szMiniMapTextureName, true, false );
							return true;
						}
					}
				}
			}
		}
	}
	return true;
}

