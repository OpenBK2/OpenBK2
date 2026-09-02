#include "stdafx.h"

#include "MapObjectDataExtractor.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "System/Text.h"
#include "Misc/StrProc.h"

#include <cstdint>

bool CMapObjectDataExtractor::GetImages( CArray2D<uint32_t> *pSmallImage, CArray2D<uint32_t> *pNormalImage, const std::string &rszObjectTypeName, const std::string &rszObjectName, IManipulator *pObjectManipulator )
{
	if ( CPtr<IManipulator> pTextureManipulator = CManipulatorManager::CreateManipulatorFromReference( "IconTexture", pObjectManipulator, 0, 0, 0 ) )
	{
		std::string szTextureName;
		CManipulatorManager::GetValue( &szTextureName, pTextureManipulator, "DestName" );
		return LoadImagesFromSource( pSmallImage, pNormalImage, szTextureName, LOAD_IMAGE_SCALE );
	}
	return false;
}


bool CMapObjectDataExtractor::GetLabel( CString *pstrLabel, const std::string &rszObjectTypeName, const std::string &rszObjectName, IManipulator *pObjectManipulator )
{
	std::string szNameFileName;
	if ( CManipulatorManager::GetValue( &szNameFileName, pObjectManipulator, "LocalizedNameFileRef" ) != false )
	{
		*pstrLabel = NStr::ToMBCS( NText::GetText( szNameFileName ) ).c_str();
		return true;
	}
	return false;
}


