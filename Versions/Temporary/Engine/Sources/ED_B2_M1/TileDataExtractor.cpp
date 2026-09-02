#include "stdafx.h"

#include "TileDataExtractor.h"
#include "MapEditorLib/ManipulatorManager.h"

#include <cstdint>

bool CTileDataExtractor::GetImages( CArray2D<uint32_t> *pSmallImage, CArray2D<uint32_t> *pNormalImage, const std::string &rszObjectTypeName, const std::string &rszObjectName, IManipulator *pObjectManipulator )
{
	if ( CPtr<IManipulator> pMaterialManipulator = CManipulatorManager::CreateManipulatorFromReference( "Material", pObjectManipulator, 0, 0, 0 ) )
	{
		if ( CPtr<IManipulator> pTexureManipulator = CManipulatorManager::CreateManipulatorFromReference( "Texture", pMaterialManipulator, 0, 0, 0 ) )
		{
			std::string szTextureName;
			CManipulatorManager::GetValue( &szTextureName, pTexureManipulator, "DestName" );
			return LoadImagesFromSource( pSmallImage, pNormalImage, szTextureName, LOAD_IMAGE_COPY );
		}
	}
	return false;
}


