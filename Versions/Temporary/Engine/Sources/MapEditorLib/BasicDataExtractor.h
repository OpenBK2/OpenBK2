#pragma once

#include "Interface_ObjectCollector.h"
struct IManipulator;

class CBasicDataExtractor : public IObjectDataExtractor
{
	bool LoadImagesFromCache( class CBitmap *pNormalBitmap, class CBitmap *pSmallBitmap, const string &rszObjectTypeName, const string &rszObjectName );
	void SaveImagesToCache( CArray2D<DWORD> &rImageSmall, CArray2D<DWORD> &rImageNormal, const string &rszObjectTypeName, const string &rszObjectName );
protected:
	enum ELoadImageMethod
	{
		LOAD_IMAGE_COPY,
		LOAD_IMAGE_SCALE,
	};
	//
	virtual bool GetImages( CArray2D<DWORD> *pSmallImage, CArray2D<DWORD> *pNormalImage, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator ) = 0;
	virtual bool GetLabel( CString *pstrLabel, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator );
	//
	bool LoadImagesFromSource( CArray2D<DWORD> *pSmallImage, CArray2D<DWORD> *pNormalImage, const string &szFileName, ELoadImageMethod eMethod );
public:
	UINT GetObjectData( class CBitmap *pNormalBitmap,
											class CBitmap *pSmallBitmap,
											CString *pstrLabel,
											const string &rszObjectTypeName,
											const string &rszObjectName,
											const string &rszDataExtractorType );
};



