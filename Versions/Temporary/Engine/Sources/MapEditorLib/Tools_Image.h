#pragma once

#include "MapEditorLib_export.h"

#include <cstdint>

namespace NImage
{
	void Copy( CArray2D<uint32_t> *pDestination, const CArray2D<uint32_t> &rSource, const CTPoint<int> &rStartPoint );
	//
	// Exported because CObjectCollector, in MapEditor, makes the front-end
	// bitmaps now: IObjectDataExtractor hands back pixels.
	MAPEDITORLIB_EXPORT void Load2Bitmap( CBitmap *pBitmap, const CArray2D<uint32_t> &rImage );
	MAPEDITORLIB_EXPORT void Load2Bitmap( CBitmap *pBitmap, const CArray2D<uint32_t> &rImage, const CTPoint<int> &rSize );
};


