#pragma once

#include "MapEditorLib/IconRenderingOptions.h"
#include "Misc/2Darray.h"
#include "SceneB2/UnitIconRendering.h"

#include <algorithm>
#include <cmath>

namespace NIconRendering
{
inline float GetFramingScale( const SOptions &options )
{
	return float( (std::min)( options.nWidth, options.nHeight ) ) / DEFAULT_SIZE;
}

inline float GetCaptureFOV( float fFOV, const SOptions &options )
{
	// Keep the old 42-pixel window on the shorter side. Rectangular exports
	// gain space along the longer side without stretching the model.
	return NUnitIconRendering::GetWindowFov( fFOV, (std::min)( options.nWidth, options.nHeight ) );
}

template <class TPixel>
bool CropCapture( CArray2D<TPixel> *pImage, const CArray2D<TPixel> &texture, const SOptions &options )
{
	return NUnitIconRendering::CropCapture( pImage, texture, options.nWidth, options.nHeight );
}
} // namespace NIconRendering
