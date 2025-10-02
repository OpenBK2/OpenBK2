#include "StdAfx.h"

#include "TileDataExtractor.h"
#include "MapEditorLib/ManipulatorManager.h"

bool CTileDataExtractor::GetImages( CArray2D<DWORD> *pSmallImage, CArray2D<DWORD> *pNormalImage, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator )
{
	if ( CPtr<IManipulator> pMaterialManipulator = CManipulatorManager::CreateManipulatorFromReference( "Material", pObjectManipulator, 0, 0, 0 ) )
	{
		if ( CPtr<IManipulator> pTexureManipulator = CManipulatorManager::CreateManipulatorFromReference( "Texture", pMaterialManipulator, 0, 0, 0 ) )
		{
			string szTextureName;
			CManipulatorManager::GetValue( &szTextureName, pTexureManipulator, "DestName" );
			return LoadImagesFromSource( pSmallImage, pNormalImage, szTextureName, LOAD_IMAGE_COPY );
		}
	}
	return false;
}


