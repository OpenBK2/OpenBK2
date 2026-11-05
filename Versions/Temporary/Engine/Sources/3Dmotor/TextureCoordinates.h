#pragma once

#include "Misc/Geom.h"

namespace NGScene
{
// UI rectangles use the texture record's logical size. The image on disk may
// have more texels, with independent and non-integral ratios on the two axes.
inline CVec2 GetTextureTexelScale( int nPixelWidth, int nPixelHeight, int nLogicalWidth, int nLogicalHeight )
{
	return CVec2( nPixelWidth > 0 && nLogicalWidth > 0 ? float( nPixelWidth ) / nLogicalWidth : 1.0f,
	              nPixelHeight > 0 && nLogicalHeight > 0 ? float( nPixelHeight ) / nLogicalHeight : 1.0f );
}

inline void ScaleTextureRect( CTRect<float> *pRect, const CVec2 &vScale )
{
	pRect->x1 *= vScale.x;
	pRect->x2 *= vScale.x;
	pRect->y1 *= vScale.y;
	pRect->y2 *= vScale.y;
}
} // namespace NGScene
