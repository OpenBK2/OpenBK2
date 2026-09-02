#pragma once

#include "MapEditorLib/BasicDataExtractor.h"

#include <cstdint>

class CVSODataExtractor : public CBasicDataExtractor
{
	OBJECT_NOCOPY_METHODS( CVSODataExtractor );
	//
	bool GetImages( CArray2D<uint32_t> *pSmallImage, CArray2D<uint32_t> *pNormalImage, const std::string &rszObjectTypeName, const std::string &rszObjectName, IManipulator *pObjectManipulator );
};
