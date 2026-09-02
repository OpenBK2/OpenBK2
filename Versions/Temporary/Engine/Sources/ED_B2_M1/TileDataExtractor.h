#pragma once

#include "MapEditorLib/BasicDataExtractor.h"

#include <cstdint>

class CTileDataExtractor : public CBasicDataExtractor
{
	OBJECT_NOCOPY_METHODS( CTileDataExtractor );
	//
	bool GetImages( CArray2D<uint32_t> *pSmallImage, CArray2D<uint32_t> *pNormalImage, const std::string &rszObjectTypeName, const std::string &rszObjectName, IManipulator *pObjectManipulator );
};


