#pragma once

#include "MapEditorLib/IconRenderingOptions.h"
#include "Misc/2Darray.h"

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
	constexpr float fRadians = 3.14159265358979323846f / 180.0f;
	return 2.0f * std::atan( std::tan( fFOV * fRadians * 0.5f ) / GetFramingScale( options ) ) / fRadians;
}

template <class TPixel>
bool CropCapture( CArray2D<TPixel> *pImage, const CArray2D<TPixel> &texture, const SOptions &options )
{
	if ( !options.IsValid() || options.nWidth > texture.GetSizeX() || options.nHeight > texture.GetSizeY() )
	{
		return false;
	}
	const int nStartX = ( texture.GetSizeX() - options.nWidth ) / 2;
	const int nStartY = ( texture.GetSizeY() - options.nHeight ) / 2;
	pImage->SetSizes( options.nWidth, options.nHeight );
	for ( int y = 0; y < options.nHeight; ++y )
	{
		for ( int x = 0; x < options.nWidth; ++x )
		{
			// CArray2D is row-major. The old square-only exporter hid swapped axes.
			( *pImage )[y][x] = texture[y + nStartY][x + nStartX];
		}
	}
	return true;
}
} // namespace NIconRendering
