#pragma once

#include "Image_export.h"

#include <cstdint>

template<class T> class CArray2D;

namespace NImage
{
enum EImageScaleMethod
{
	IMAGE_SCALE_METHOD_FILTER   = 1,
	IMAGE_SCALE_METHOD_BOX      = 2,
	IMAGE_SCALE_METHOD_TRIANGLE = 3,
	IMAGE_SCALE_METHOD_BELL     = 4,
	IMAGE_SCALE_METHOD_BSPLINE  = 5,
	IMAGE_SCALE_METHOD_LANCZOS3 = 6,
	IMAGE_SCALE_METHOD_MITCHELL = 7,

	IMAGE_SCALE_METHOD_FORCE_DWORD = 0x7fffffff
};

IMAGE_EXPORT void Scale( CArray2D<uint32_t> *pDst, const CArray2D<uint32_t> &src, const EImageScaleMethod method );
}

