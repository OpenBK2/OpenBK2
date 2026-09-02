#pragma once

#include "MapEditorLib/BasicDataExtractor.h"

#include <cstdint>

class CMapObjectDataExtractor : public CBasicDataExtractor
{
	OBJECT_NOCOPY_METHODS( CMapObjectDataExtractor );
	//
	bool GetImages( CArray2D<uint32_t> *pSmallImage, CArray2D<uint32_t> *pNormalImage, const std::string &rszObjectTypeName, const std::string &rszObjectName, IManipulator *pObjectManipulator );
	bool GetLabel( CString *pstrLabel, const std::string &rszObjectTypeName, const std::string &rszObjectName, IManipulator *pObjectManipulator );
};


