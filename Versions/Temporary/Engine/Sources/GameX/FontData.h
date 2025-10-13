#pragma once

#include <cstdint>

namespace NFontGen
{
class CFontInfo;

bool CreateFontImage( CArray2D<uint32_t> *pRes, const CFontInfo &fi );
void CreateFontFormat( class CFontFormatInfo *pRes, const CFontInfo &fi );

}
