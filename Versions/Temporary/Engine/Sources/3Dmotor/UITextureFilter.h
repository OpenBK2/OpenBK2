#pragma once

#include "GPixelFormat.h"
#include "Misc/2Darray.h"

#include "3Dmotor_export.h"

#include <vector>

namespace NGScene
{
// Preserve source texel coordinates while padding to power-of-two dimensions,
// then average covered colors without mixing in RGB from transparent pixels.
_3DMOTOR_EXPORT void BuildUITextureMips( std::vector<CArray2D<NGfx::SPixel8888>> *pMips,
                                         const CArray2D<NGfx::SPixel8888> &image );

// Coordinates are in base-level texels, with texel centers at half integers.
// The software icon compositor uses the same mip chain as hardware UI textures.
_3DMOTOR_EXPORT NGfx::SPixel8888
SampleUITexture( const std::vector<CArray2D<NGfx::SPixel8888>> &mips, float fU, float fV, float fLOD );
} // namespace NGScene
