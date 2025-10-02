#pragma once

#include "MapEditorLib/BasicDataExtractor.h"

class CSpotDataExtractor : public CBasicDataExtractor
{
	OBJECT_NOCOPY_METHODS( CSpotDataExtractor );
	//
	bool GetImages( CArray2D<DWORD> *pSmallImage, CArray2D<DWORD> *pNormalImage, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator );
};


