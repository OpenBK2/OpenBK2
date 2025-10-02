#include "StdAfx.h"

#include "VSODataExtractor.h"
#include "MapEditorLib/ManipulatorManager.h"

bool CVSODataExtractor::GetImages( CArray2D<DWORD> *pSmallImage, CArray2D<DWORD> *pNormalImage, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator )
{
	// получаем материал
	CPtr<IManipulator> pMaterialManipulator = 0;
	if ( rszObjectTypeName == "RoadDesc" )
	{
		pMaterialManipulator = CManipulatorManager::CreateManipulatorFromReference( "Center.Materials.[0]", pObjectManipulator, 0, 0, 0 );
	}
	else if ( rszObjectTypeName == "RiverDesc" )
	{
		pMaterialManipulator = CManipulatorManager::CreateManipulatorFromReference( "WaterMaterial", pObjectManipulator, 0, 0, 0 );
	}
	else if ( rszObjectTypeName == "CragDesc" )
	{
		pMaterialManipulator = CManipulatorManager::CreateManipulatorFromReference( "RidgeMaterial", pObjectManipulator, 0, 0, 0 );
	}
	else if ( rszObjectTypeName == "LakeDesc" )
	{
		if ( CPtr<IManipulator> pWaterMan = CManipulatorManager::CreateManipulatorFromReference( "WaterParams", pObjectManipulator, 0, 0, 0 ) )
		{
			if ( CPtr<IManipulator> pWaterSetMan = CManipulatorManager::CreateManipulatorFromReference( "WaterSet", pWaterMan, 0, 0, 0 ) )
			{
				pMaterialManipulator = CManipulatorManager::CreateManipulatorFromReference( "Water.Material", pWaterSetMan, 0, 0, 0 );
			}
		}
	}
	else if ( rszObjectTypeName == "CoastDesc" )
	{
		if ( CPtr<IManipulator> pWaterMan = CManipulatorManager::CreateManipulatorFromReference( "Water", pObjectManipulator, 0, 0, 0 ) )
		{
			if ( CPtr<IManipulator> pWaterSetMan = CManipulatorManager::CreateManipulatorFromReference( "WaterSet", pWaterMan, 0, 0, 0 ) )
			{
				pMaterialManipulator = CManipulatorManager::CreateManipulatorFromReference( "Water.Material", pWaterSetMan, 0, 0, 0 );
			}
		}
	}
	//
	if ( pMaterialManipulator != 0 )
	{
		if ( CPtr<IManipulator> pTextureManipulator = CManipulatorManager::CreateManipulatorFromReference( "Texture", pMaterialManipulator, 0, 0, 0 ) )
		{
			string szTextureName;
			CManipulatorManager::GetValue( &szTextureName, pTextureManipulator, "DestName" );
			return LoadImagesFromSource( pSmallImage, pNormalImage, szTextureName, LOAD_IMAGE_COPY );
		}
	}
	return false;
}


