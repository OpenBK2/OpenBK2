#pragma once

#include <cstdint>

namespace NFontGen
{

bool GetFontCharsForCharset( vector<uint16_t> *pChars, HDC hDC, const string &szFaceName, uint32_t dwCharSet );

}
