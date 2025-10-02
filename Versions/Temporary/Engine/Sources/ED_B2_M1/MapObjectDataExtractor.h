#pragma once

#include "../MapEditorLib/BasicDataExtractor.h"

class CMapObjectDataExtractor : public CBasicDataExtractor
{
	OBJECT_NOCOPY_METHODS( CMapObjectDataExtractor );
	//
	bool GetImages( CArray2D<DWORD> *pSmallImage, CArray2D<DWORD> *pNormalImage, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator );
	bool GetLabel( CString *pstrLabel, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator );
};


