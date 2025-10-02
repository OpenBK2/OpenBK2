#pragma once

#include "../MapEditorLib/BasicDataExtractor.h"

class CTileDataExtractor : public CBasicDataExtractor
{
	OBJECT_NOCOPY_METHODS( CTileDataExtractor );
	//
	bool GetImages( CArray2D<DWORD> *pSmallImage, CArray2D<DWORD> *pNormalImage, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator );
};

