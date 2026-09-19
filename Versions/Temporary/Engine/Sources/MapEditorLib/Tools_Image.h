#pragma once

#include "MapEditorLib_export.h"

#include <cstdint>

namespace NImage
{
	void Copy( CArray2D<uint32_t> *pDestination, const CArray2D<uint32_t> &rSource, const CTPoint<int> &rStartPoint );
	//
	// Exported because CObjectCollector, in MapEditor, makes the front-end
	// bitmaps now: IObjectDataExtractor hands back pixels. A new bitmap the
	// caller deletes (DeleteObject), or 0; it filled a CBitmap before MFC went.
	MAPEDITORLIB_EXPORT HBITMAP Load2Bitmap( const CArray2D<uint32_t> &rImage );
	MAPEDITORLIB_EXPORT HBITMAP Load2Bitmap( const CArray2D<uint32_t> &rImage, const CTPoint<int> &rSize );
};
