#include "StdAfx.h"

#include "MapObjectDataExtractor.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "System/Text.h"
#include "Misc/StrProc.h"

bool CMapObjectDataExtractor::GetImages( CArray2D<DWORD> *pSmallImage, CArray2D<DWORD> *pNormalImage, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator )
{
	if ( CPtr<IManipulator> pTextureManipulator = CManipulatorManager::CreateManipulatorFromReference( "IconTexture", pObjectManipulator, 0, 0, 0 ) )
	{
		string szTextureName;
		CManipulatorManager::GetValue( &szTextureName, pTextureManipulator, "DestName" );
		return LoadImagesFromSource( pSmallImage, pNormalImage, szTextureName, LOAD_IMAGE_SCALE );
	}
	return false;
}


bool CMapObjectDataExtractor::GetLabel( CString *pstrLabel, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator )
{
	string szNameFileName;
	if ( CManipulatorManager::GetValue( &szNameFileName, pObjectManipulator, "LocalizedNameFileRef" ) != false )
	{
		*pstrLabel = NStr::ToMBCS( NText::GetText( szNameFileName ) ).c_str();
		return true;
	}
	return false;
}


