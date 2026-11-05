#pragma once

#include "Image_export.h"

#include <cstdint>

namespace NImage
{
IMAGE_EXPORT bool RecognizeFormatTGA( CDataStream *pStream );
// exported for GameX's unit icon cache, which keeps its pictures as TGA
IMAGE_EXPORT bool LoadImageTGA( CArray2D<uint32_t> *pRes, CDataStream *pStream );
IMAGE_EXPORT bool SaveImageAsTGA( CDataStream *pStream, const CArray2D<uint32_t> &image );
}; // namespace NImage
