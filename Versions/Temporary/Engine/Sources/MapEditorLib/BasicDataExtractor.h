#pragma once

#include "Interface_ObjectCollector.h"
struct IManipulator;

class CBasicDataExtractor : public IObjectDataExtractor
{
	bool LoadImagesFromCache( class CBitmap *pNormalBitmap, class CBitmap *pSmallBitmap, const string &rszObjectTypeName, const string &rszObjectName );
	void SaveImagesToCache( CArray2D<uint32_t> &rImageSmall, CArray2D<uint32_t> &rImageNormal, const string &rszObjectTypeName, const string &rszObjectName );
protected:
	enum ELoadImageMethod
	{
		LOAD_IMAGE_COPY,
		LOAD_IMAGE_SCALE,
	};
	//
	virtual bool GetImages( CArray2D<uint32_t> *pSmallImage, CArray2D<uint32_t> *pNormalImage, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator ) = 0;
	virtual bool GetLabel( CString *pstrLabel, const string &rszObjectTypeName, const string &rszObjectName, IManipulator *pObjectManipulator );
	//
	bool LoadImagesFromSource( CArray2D<uint32_t> *pSmallImage, CArray2D<uint32_t> *pNormalImage, const string &szFileName, ELoadImageMethod eMethod );
public:
	unsigned GetObjectData( class CBitmap *pNormalBitmap,
											class CBitmap *pSmallBitmap,
											CString *pstrLabel,
											const string &rszObjectTypeName,
											const string &rszObjectName,
											const string &rszDataExtractorType );
};



