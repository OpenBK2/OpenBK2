#pragma once

namespace NFontGen
{
class CFontInfo;

bool CreateFontImage( CArray2D<DWORD> *pRes, const CFontInfo &fi );
void CreateFontFormat( class CFontFormatInfo *pRes, const CFontInfo &fi );

}
