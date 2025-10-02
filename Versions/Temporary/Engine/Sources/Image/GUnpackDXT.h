#pragma once

#include "Image_export.h"

namespace NImage
{
// DWORD == NGfx::SPixel8888
IMAGE_EXPORT void UnpackDXT( int nDxt, int nXSize, int nYSize, const void *pData, CArray2D<DWORD> *pRes );
}

