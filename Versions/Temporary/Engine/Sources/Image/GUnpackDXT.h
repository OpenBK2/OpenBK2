#pragma once

#include "Image_export.h"

#include <cstdint>

namespace NImage
{
// uint32_t == NGfx::SPixel8888
IMAGE_EXPORT void UnpackDXT( int nDxt, int nXSize, int nYSize, const void *pData, CArray2D<uint32_t> *pRes );
}

