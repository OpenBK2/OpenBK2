#pragma once

#include <cstdint>

#include "Image_export.h"

namespace NImage
{
	IMAGE_EXPORT int RecognizeFormatPSD( CDataStream *pStream );	// 0 - Not PSD-image at all
													// 1 - Valid PSD-image (RGB, 8-bit per channel)
													// 2 - Not valid PSD-image (rather, than RGB, 8-bit per channel)

	IMAGE_EXPORT bool LoadImagePSD( CArray2D<uint32_t> &pRes, CDataStream *pStream );
};


