#pragma once

#include "MapEditorLib/BasicDataExtractor.h"

class CVSODataExtractor : public CBasicDataExtractor
{
	OBJECT_NOCOPY_METHODS( CVSODataExtractor );
	//
	bool GetImages( CArray2D<DWORD> *pSmallImage, CArray2D<DWORD> *pNormalImage, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator );
};


