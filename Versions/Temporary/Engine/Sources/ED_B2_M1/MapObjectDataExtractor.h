#pragma once

#include "MapEditorLib/BasicDataExtractor.h"

#include <cstdint>

class CMapObjectDataExtractor : public CBasicDataExtractor
{
	OBJECT_NOCOPY_METHODS( CMapObjectDataExtractor );
	//
	bool GetImages( CArray2D<uint32_t> *pSmallImage, CArray2D<uint32_t> *pNormalImage, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator );
	bool GetLabel( CString *pstrLabel, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator );
};


