#pragma once

#include <cstdint>

namespace NImage
{
	void Copy( CArray2D<uint32_t> *pDestination, const CArray2D<uint32_t> &rSource, const CTPoint<int> &rStartPoint );
	//
	void Load2Bitmap( CBitmap *pBitmap, const CArray2D<uint32_t> &rImage );
	void Load2Bitmap( CBitmap *pBitmap, const CArray2D<uint32_t> &rImage, const CTPoint<int> &rSize );
};


