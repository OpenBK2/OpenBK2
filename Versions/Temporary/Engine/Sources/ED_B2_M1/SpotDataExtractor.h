#pragma once

#include "MapEditorLib/BasicDataExtractor.h"

#include <cstdint>

class CSpotDataExtractor : public CBasicDataExtractor
{
	OBJECT_NOCOPY_METHODS( CSpotDataExtractor );
	//
	bool GetImages( CArray2D<uint32_t> *pSmallImage, CArray2D<uint32_t> *pNormalImage, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator );
};


