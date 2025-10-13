#pragma once

#include <cstdint>

namespace NImage
{
	int RecognizeFormatPSD( CDataStream *pStream );	// 0 - Not PSD-image at all
													// 1 - Valid PSD-image (RGB, 8-bit per channel)
													// 2 - Not valid PSD-image (rather, than RGB, 8-bit per channel)

	bool LoadImagePSD( CArray2D<uint32_t> &pRes, CDataStream *pStream );
};


