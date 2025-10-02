#pragma once

namespace NFontGen
{

bool GetFontCharsForCharset( vector<WORD> *pChars, HDC hDC, const string &szFaceName, DWORD dwCharSet );

}
