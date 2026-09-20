#pragma once

#include "MapEditorLib_export.h"

#include <cstdint>

namespace NImage
{
	void Copy( CArray2D<uint32_t> *pDestination, const CArray2D<uint32_t> &rSource, const CTPoint<int> &rStartPoint );
	//
	// Load2Bitmap was here, turning those pixels into an HBITMAP for the object
	// collector's image lists. The lists are wxImageLists now and take the
	// pixels directly: see NWxImageList::ToWxImage in WxImageList.h.
};
