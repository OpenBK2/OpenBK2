#include "stdafx.h"

namespace NFontGen
{

struct SCharRange
{
	int nBit;
	int nRangeBegin;
	int nRangeEnd;
	const char* szName;
};

static SCharRange sCharRanges[] =
{
	{  0, 0x0020, 0x007E, "Basic Latin" },
	{  1, 0x00A0, 0x00FF, "Latin-1 Supplement" },
	{  2, 0x0100, 0x017F, "Latin Extended-A" },
	{  3, 0x0180, 0x024F, "Latin Extended-B" },
	{  4, 0x0250, 0x02AF, "IPA Extensions" },
	{  5, 0x02B0, 0x02FF, "Spacing Modifier Letters" },
	{  6, 0x0300, 0x036F, "Combining Diacritical Marks" },
	{  7, 0x0370, 0x03FF, "Basic Greek" },
	{  8, 0x0000, 0x0000, "Reserved" },
	{  9, 0x0400, 0x04FF, "Cyrillic" },
	{ 10, 0x0530, 0x058F, "Armenian" },
	{ 11, 0x0590, 0x05FF, "Basic Hebrew" },
	{ 12, 0x0000, 0x0000, "Reserved" },
	{ 13, 0x0600, 0x06FF, "Basic Arabic" },
	{ 14, 0x0000, 0x0000, "Reserved" },
	{ 15, 0x0900, 0x097F, "Devanagari" },
	{ 16, 0x0980, 0x09FF, "Bengali" },
	{ 17, 0x0A00, 0x0A7F, "Gurmukhi" },
	{ 18, 0x0A80, 0x0AFF, "Gujarati" },
	{ 19, 0x0B00, 0x0B7F, "Oriya" },
	{ 20, 0x0B80, 0x0BFF, "Tamil" },
	{ 21, 0x0C00, 0x0C7F, "Telugu" },
	{ 22, 0x0C80, 0x0CFF, "Kannada" },
	{ 23, 0x0D00, 0x0D7F, "Malayalam" },
	{ 24, 0x0E00, 0x0E7F, "Thai" },
	{ 25, 0x0E80, 0x0EFF, "Lao" },
	{ 26, 0x10A0, 0x10FF, "Basic Georgian" },
	{ 27, 0x0000, 0x0000, "Reserved" },
	{ 28, 0x1100, 0x11FF, "Hangul Jamo" },
	{ 29, 0x1E00, 0x1EFF, "Latin Extended Additional" },
	{ 30, 0x1F00, 0x1FFF, "Greek Extended" },
	{ 31, 0x2000, 0x206F, "General Punctuation" },
	{ 32, 0x2070, 0x209F, "Subscripts and Superscripts" },
	{ 33, 0x20A0, 0x20CF, "Currency Symbols" },
	{ 34, 0x20D0, 0x20FF, "Combining Diacritical Marks for Symbols" },
	{ 35, 0x2100, 0x214F, "Letter-like Symbols" },
	{ 36, 0x2150, 0x218F, "Number Forms" },
	{ 37, 0x2190, 0x21FF, "Arrows" },
	{ 38, 0x2200, 0x22FF, "Mathematical Operators" },
	{ 39, 0x2300, 0x23FF, "Miscellaneous Technical" },
	{ 40, 0x2400, 0x243F, "Control Pictures" },
	{ 41, 0x2440, 0x245F, "Optical Character Recognition" },
	{ 42, 0x2460, 0x24FF, "Enclosed Alphanumerics" },
	{ 43, 0x2500, 0x257F, "Box Drawing" },
	{ 44, 0x2580, 0x259F, "Block Elements" },
	{ 45, 0x25A0, 0x25FF, "Geometric Shapes" },
	{ 46, 0x2600, 0x26FF, "Miscellaneous Symbols" },
	{ 47, 0x2700, 0x27BF, "Dingbats" },
	{ 48, 0x3000, 0x303F, "Chinese, Japanese, and Korean (CJK) Symbols and Punctuation" },
	{ 49, 0x3040, 0x309F, "Hiragana" },
	{ 50, 0x30A0, 0x30FF, "Katakana" },
	{ 51, 0x3100, 0x312F, "Bopomofo" },
	{ 51, 0x31A0, 0x31BF, "Extended Bopomofo" },
	{ 52, 0x3130, 0x318F, "Hangul Compatibility Jamo" },
	{ 53, 0x3190, 0x319F, "CJK Miscellaneous" },
	{ 54, 0x3200, 0x32FF, "Enclosed CJK Letters and Months" },
	{ 55, 0x3300, 0x33FF, "CJK Compatibility" },
	{ 56, 0xAC00, 0xD7A3, "Hangul" },
	{ 57, 0xD800, 0xDFFF, "Surrogates" },
	{ 58, 0x0000, 0x0000, "Reserved" },
	{ 59, 0x4E00, 0x9FFF, "CJK Unified Ideographs" },
	{ 59, 0x2E80, 0x2EFF, "CJK Radicals Supplement" },
	{ 59, 0x2F00, 0x2FDF, "Kangxi Radicals" },
	{ 59, 0x2FF0, 0x2FFF, "Ideographic Description" },
	{ 59, 0x3400, 0x4DBF, "CJK Unified Ideograph Extension A" },
	{ 60, 0xE000, 0xF8FF, "Private Use Area" },
	{ 61, 0xF900, 0xFAFF, "CJK Compatibility Ideographs" },
	{ 62, 0xFB00, 0xFB4F, "Alphabetic Presentation Forms" },
	{ 63, 0xFB50, 0xFDFF, "Arabic Presentation Forms-A" },
	{ 64, 0xFE20, 0xFE2F, "Combining Half Marks" },
	{ 65, 0xFE30, 0xFE4F, "CJK Compatibility Forms" },
	{ 66, 0xFE50, 0xFE6F, "Small Form Variants" },
	{ 67, 0xFE70, 0xFEFE, "Arabic Presentation Forms-B" },
	{ 68, 0xFF00, 0xFFEF, "Halfwidth and Fullwidth Forms" },
	{ 69, 0xFFF0, 0xFFFD, "Specials" },
	{ 70, 0x0F00, 0x0FCF, "Tibetan" },
	{ 71, 0x0700, 0x074F, "Syriac" },
	{ 72, 0x0780, 0x07BF, "Thaana" },
	{ 73, 0x0D80, 0x0DFF, "Sinhala" },
	{ 74, 0x1000, 0x109F, "Myanmar" },
	{ 75, 0x1200, 0x12BF, "Ethiopic" },
	{ 76, 0x13A0, 0x13FF, "Cherokee" },
	{ 77, 0x1400, 0x14DF, "Canadian Aboriginal Syllabics" },
	{ 78, 0x1680, 0x169F, "Ogham" },
	{ 79, 0x16A0, 0x16FF, "Runic" },
	{ 80, 0x1780, 0x17FF, "Khmer" },
	{ 81, 0x1800, 0x18AF, "Mongolian" },
	{ 82, 0x2800, 0x28FF, "Braille" },
	{ 83, 0xA000, 0xA48C, "Yi, Yi Radicals" }
};

int CALLBACK EnumFontFamExProc( ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD dwFontType, LPARAM lParam )
{
	std::vector<WORD> *pChars = (std::vector<WORD>*)lParam;

	printf( "\nFont: %S %S %S", lpelfe->elfFullName, lpelfe->elfStyle, lpelfe->elfScript );
	if ( dwFontType == TRUETYPE_FONTTYPE )
	{
		for ( int nTemp = 0; nTemp < 126; nTemp++ )
		{
			int nIndex = nTemp / 32;
			int nShift = 31 - nTemp % 32;

			int nMask = 1 << nShift;
			if ( lpntme->ntmFontSig.fsUsb[nIndex] & nMask )
			{
				for ( int nRange = 0; nRange < ARRAY_SIZE( sCharRanges ); ++nRange )
				{
					if ( sCharRanges[nRange].nBit != nTemp )
						continue;

//					printf( "\n\tFound %s [%d-%d]", sCharRanges[nRange].szName, sCharRanges[nRange].nRangeBegin, sCharRanges[nRange].nRangeEnd );
					for ( int nTemp = sCharRanges[nRange].nRangeBegin; nTemp < sCharRanges[nRange].nRangeEnd; nTemp++ )
						pChars->push_back( nTemp );
				}
			}
		}
	}
	else
	{
//		printf( "\nRaster" );
	}

	return TRUE;
}

bool GetFontCharsForCharset( std::vector<WORD> *pChars, HDC hDC, const std::string &szFaceName, DWORD dwCharSet )
{
	LOGFONT sLogFont;
	memset( &sLogFont, 0, sizeof( LOGFONT ) );
	sLogFont.lfCharSet = dwCharSet;
	sLogFont.lfPitchAndFamily = 0;
	memcpy( sLogFont.lfFaceName, szFaceName.c_str(), Min<size_t>(szFaceName.size() + 1, LF_FACESIZE) );
	EnumFontFamiliesEx( hDC, &sLogFont, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)pChars, 0 );
	//
	return !pChars->empty();
}

}

