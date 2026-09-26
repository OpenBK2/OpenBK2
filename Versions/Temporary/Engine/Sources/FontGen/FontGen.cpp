#include "stdafx.h"
#include "FontFormat.h"
#include "Image/Targa.h"
#include "Misc/2Darray.h"
#include "Misc/StrProc.h"
#include <algorithm>
#include "System/FileUtils.h"
#include "FontRaster.h"

#include "port/cdecl.h"

#include <cstdint>

#include <fmt/format.h>

const int N_LEADING_PIXELS = 2;

namespace NImage
{
	typedef CArray2D<CVec4> CImage;
}

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

HWND hWnd;
HINSTANCE hInst;

// CFontInfo
//      This class stores information about the currently loaded font.
//      This includes LOGFONT and CHOOSEFONT structures for use with the
//      ChooseFont dialog, as well as info about character dimensions.
//
struct SFontInfo
{
  HFONT hFont;                // HFONT used to draw with this font
  TEXTMETRIC tm;              // text metrics, e.g. character height
	std::vector<ABC> abc;									// character ABC widths
	std::vector<KERNINGPAIR> kps;					// kernging pairs
	int nTextureSizeX, nTextureSizeY;			// estimated texture size
	std::unordered_map<uint16_t, uint16_t> translate;	// ANSI => UNICODE translation table
	//
	uint16_t Translate( uint16_t code ) const
	{
		std::unordered_map<uint16_t, uint16_t>::const_iterator pos = translate.find( code );
		//ASSERT( pos != translate.end() );
		if ( pos == translate.end() )
		{
			DebugTrace( "Can't find code for symbol %d to re-map", code );
			return 0xffff;
		}
		return pos->second;
	}
	//
	SFontInfo() : hFont( 0 ), nTextureSizeX( 0 ), nTextureSizeY( 0 ) {  }
	virtual ~SFontInfo() { if ( hFont ) DeleteObject( hFont ); }
};

// estimate, is requested number of chars fit in the selected texture
inline bool IsFit( const SFontInfo &fi, uint32_t dwNumChars, uint32_t dwSizeX, uint32_t dwSizeY )
{
  return ( dwSizeX / (fi.tm.tmAveCharWidth + 2) ) * ( dwSizeY / fi.tm.tmHeight ) >= dwNumChars;
}

bool EstimateTextureSize( SFontInfo *pFI, uint32_t dwNumChars )
{
	SFontInfo &fi = *pFI;
  for ( int i=6; i<13; ++i )
  {
    // first, try to estimate 2:1 size
    if ( IsFit( fi, dwNumChars, 1 << i, 1 << (i - 1) ) )
    {
			fi.nTextureSizeX = 1 << i;
			fi.nTextureSizeY = 1 << (i - 1);
      return true;
    }
    // then, try to estimate 1:1 size
    else if ( IsFit( fi, dwNumChars, 1 << i, 1 << i ) )
    {
      fi.nTextureSizeX = fi.nTextureSizeY = 1 << i;
      return true;
    }
  }
  // too big texture!!!
  return false;
}

struct SKPZeroFunctional
{
  bool operator()( const KERNINGPAIR &kp ) const { return kp.iKernAmount == 0; }
};

//      Fills CFontInfo fi (global) with text metrics and char widths
//      -> hdc: HDC that the font is currently selected into
//
void MeasureFont( HDC hdc, SFontInfo *pFI, std::vector<uint16_t> *pChars )
{
	std::vector<uint16_t> &chars = *pChars;
	SFontInfo &fi = *pFI;
  GetTextMetrics( hdc, &fi.tm );
	sort( chars.begin(), chars.end() );
	if ( find( chars.begin(), chars.end(), fi.tm.tmDefaultChar ) == chars.end() )
		chars.push_back( fi.tm.tmDefaultChar );
  // Measure TrueType fonts with GetCharABCWidths:
	fi.abc.resize( chars.size() );
	if ( !GetCharABCWidths( hdc, chars[0], chars[0], &( fi.abc[0] ) ) )
	{
		// 
		ABC abc;
		Zero( abc );
		fill( fi.abc.begin(), fi.abc.end(), abc );
		// If it's not a TT font, use GetTextExtentPoint32 to fill array abc:
		SIZE size;
		for ( int i=0; i<chars.size(); ++i )
		{
			// get width of character...
			GetTextExtentPoint32( hdc, (TCHAR*)&( chars[i] ), 1, &size );
			// ...and store it in abcB:
			fi.abc[i].abcB = size.cx;
		}
	}
	else
	{
		for ( int i=0; i<chars.size(); ++i )
			GetCharABCWidths( hdc, chars[i], chars[i], &( fi.abc[i] ) );
	}
  // get kerning pairs
	KERNINGPAIR kernpair;
	Zero( kernpair );
	fi.kps.resize(0);
	fi.kps.resize( chars.size() * chars.size(), kernpair );
	GetKerningPairs( hdc, chars.size()*chars.size(), &( fi.kps[0] ) );
  // remove kerning pairs with '0' kern value
  fi.kps.erase( remove_if( fi.kps.begin(), fi.kps.end(), SKPZeroFunctional() ), fi.kps.end() );

  // estimate texture size
  if ( !EstimateTextureSize( &fi, chars.size() ) )
    throw 1; // too large texture !!!
  // check and correct size estimating
  int x = 0, y = 0;
	for ( int i=0; i<chars.size(); ++i )
	{
    int nNextCharShift = fi.abc[i].abcB + abs( fi.abc[i].abcC );
    if ( x + nNextCharShift + N_LEADING_PIXELS > fi.nTextureSizeX )
    {
      ++y;
      x = 0;
      if ( (y + 1) * fi.tm.tmHeight > fi.nTextureSizeY )
      {
        if ( fi.nTextureSizeX == fi.nTextureSizeY ) // if we have 1:1 sizes, make it 2:1
          fi.nTextureSizeX <<= 1;
        else                                   // else, if we have 2:1 already, make it 2:2 :)
          fi.nTextureSizeY = fi.nTextureSizeX;
        break;
      }
    }
    x += N_LEADING_PIXELS;
    x += nNextCharShift;
	}
}

int CALLBACK EnumFontFamExProc( ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, uint32_t FontType, LPARAM lParam )
{
	std::vector<uint16_t> *pChars = (std::vector<uint16_t>*)lParam;

	fmt::print( "\nFont: {} {} {}", reinterpret_cast<const char*>( lpelfe->elfFullName ), reinterpret_cast<const char*>( lpelfe->elfStyle ), reinterpret_cast<const char*>( lpelfe->elfScript ) );
	if ( FontType == TRUETYPE_FONTTYPE )
	{
		for ( int nTemp = 0; nTemp < 126; nTemp++ )
		{
			int nIndex = nTemp / 32;
			int nShift = 31 - nTemp % 32;

			int nMask = 1 << nShift;
			if ( lpntme->ntmFontSig.fsUsb[nIndex] & nMask )
			{
				for ( int nRange = 0; nRange < std::size( sCharRanges ); nRange++ )
				{
					if ( sCharRanges[nRange].nBit != nTemp )
						continue;

					fmt::print( "\n\tFound {} [{}-{}]", sCharRanges[nRange].szName, sCharRanges[nRange].nRangeBegin, sCharRanges[nRange].nRangeEnd );
					for ( int nTemp = sCharRanges[nRange].nRangeBegin; nTemp < sCharRanges[nRange].nRangeEnd; nTemp++ )
						pChars->push_back( nTemp );
				}
			}
		}
	}
	else
	{
		fmt::print( "\nRaster" );
	}

	return TRUE;
}

void BuildTranslation( uint32_t dwCharSet, const std::vector<uint16_t> &chars, std::unordered_map<uint16_t, uint16_t> *pTranslate );

void LoadFont( HWND hWnd, SFontInfo *pFI, int nHeight, int nWeight, bool bItalic, uint32_t dwCharSet,
	bool bAntialias, uint32_t dwPitch, LPCTSTR pszFaceName, std::vector<uint16_t> *pChars )
{
	SFontInfo &fi = *pFI;
	std::vector<uint16_t> &chars = *pChars;
  // invoke ChooseFont common dialog:
  // create an HFONT:
  if ( fi.hFont )
  { 
    DeleteObject( fi.hFont ); 
    fi.hFont = 0;
  }

/*
	wstring wsUnicodeName( NStr::ToUnicode( szFaceName ) );

	fmt::print( "\n=============================================================" );

	uint32_t dwTemp = dwCharSet;
	CHARSETINFO sCharSetInfo;
	memset( &sCharSetInfo, 0, sizeof( CHARSETINFO ) );
	if ( TranslateCharsetInfo( (uint32_t*)dwTemp, &sCharSetInfo, TCI_SRCCHARSET ) )
	{
		fmt::print( "\nCS {} CP {} === {:x} {:x} {:x} {:x}", sCharSetInfo.ciCharset, sCharSetInfo.ciACP, sCharSetInfo.fs.fsUsb[0], sCharSetInfo.fs.fsUsb[1], sCharSetInfo.fs.fsUsb[2], sCharSetInfo.fs.fsUsb[3] );
		for ( int nTemp = 0; nTemp < 126; nTemp++ )
		{
			int nIndex = nTemp / 32;
			int nShift = 31 - nTemp % 32;

			int nMask = 1 << nShift;
			if ( sCharSetInfo.fs.fsUsb[nIndex] & nMask )
			{
				for ( int nRange = 0; nRange < std::size( sCharRanges ); nRange++ )
				{
					if ( sCharRanges[nRange].nBit != nTemp )
						continue;

					fmt::print( "\n\tFound {} [{}-{}]", sCharRanges[nRange].szName, sCharRanges[nRange].nRangeBegin, sCharRanges[nRange].nRangeEnd );
					for ( int nTemp = sCharRanges[nRange].nRangeBegin; nTemp < sCharRanges[nRange].nRangeEnd; nTemp++ )
						pChars->push_back( nTemp );
				}
			}
		}
	}
	else
		fmt::print( "\n ERROR: {}", GetLastError() );

	fmt::print( "\n=============================================================" );

	LOGFONT sLogFont;
	memset( &sLogFont, 0, sizeof( LOGFONT ) );
	sLogFont.lfCharSet = dwCharSet;
	sLogFont.lfPitchAndFamily = 0;
	wcsncpy( sLogFont.lfFaceName, wsUnicodeName.c_str(), 32 );
	EnumFontFamiliesEx( hdc, &sLogFont, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)pChars, 0 );
	if ( pChars->empty() )
	{
		DebugTrace( "Can't find codes for symbols" );
		return;
	}
*/

	// Still ClearType, but spelled with the constant rather than the literal 6
	// that was here, which was a number because the 2003 Platform SDK predates
	// it. It was also asked for only on XP or later, a test that has not chosen
	// anything for a long time, so the check goes with it.
	//
	// Keeping ClearType is deliberate and not the obvious choice. ClearType
	// renders subpixel coverage, so the three channels describe three different
	// subpixels rather than being three copies of one greyscale value, and
	// CreateFontImage takes the average of them. That is a horizontal
	// downsample of a 3x supersample, and it measures better than
	// ANTIALIASED_QUALITY at every size tried: around 31 to 35 distinct
	// intermediate alpha levels against 12, and against none at all at height
	// 16, where GDI declines to antialias and grid fits instead. Height 16 is
	// what the shipped default face is baked at, so that case decides it.
	//
	// What is not kept is the subpixel data itself. Baking that would encode
	// the stripe order of the panel it was baked on and be wrong on a BGR or
	// rotated display, with no way to correct it at draw time. Averaging
	// discards the subpixel positioning and keeps only the coverage it implies.
	uint32_t dwQuality = bAntialias ? CLEARTYPE_QUALITY : NONANTIALIASED_QUALITY;
  fi.hFont = ::CreateFont( nHeight, 0, 0, 0, nWeight, bItalic, FALSE, FALSE, 
                           dwCharSet, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                           dwQuality,
                           dwPitch, pszFaceName );
  // retrieve logfont
//  ::GetObject( fi.hFont, sizeof(fi.lf), &fi.lf );
  // get HDC:
  HDC hdc = GetDC( hWnd );
  // select font:
  HFONT hOldFont = (HFONT)::SelectObject( hdc, fi.hFont );
	//
  // get text metrics and char widths:
  MeasureFont( hdc, &fi, &chars );
	// translate chars to UNICODE and re-map kerns and chars
	BuildTranslation( dwCharSet, chars, &fi.translate );
  // select old font
  ::SelectObject( hdc, hOldFont );
  // release HDC:
  ReleaseDC( hWnd, hdc );
}

// Maps each character code FontGen bakes, a single byte in the code page that
// dwCharSet implies, to the Unicode code point the engine looks glyphs up by.
// Used by both rasterisers; a byte with no mapping is left out of *pTranslate.
void BuildTranslation( uint32_t dwCharSet, const std::vector<uint16_t> &chars, std::unordered_map<uint16_t, uint16_t> *pTranslate )
{
	{
		CHARSETINFO cs;
		Zero( cs );
		// DWORD*, not uint32_t*: they are the same width but distinct types, and
		// this is the documented way to call TranslateCharsetInfo with
		// TCI_SRCCHARSET, where the first argument is a charset value rather than
		// a pointer at all.
		// Checked rather than asserted: ASSERT does not evaluate its argument in
		// this tree, and cs.ciACP is about to be used. An unknown charset would
		// otherwise reach MultiByteToWideChar as a garbage code page.
		// Widened through uintptr_t first, since on x64 a pointer is wider than
		// the charset value it carries.
		if ( !TranslateCharsetInfo( reinterpret_cast<DWORD*>( static_cast<uintptr_t>( dwCharSet ) ), &cs, TCI_SRCCHARSET ) )
		{
			fmt::print( "ERROR: no code page for charset {}, falling back to the system one\n",
			        static_cast<int>( dwCharSet ) );
			cs.ciACP = CP_ACP;
		}
		// Convert through the code page the requested charset implies, one byte at
		// a time, rather than NStr::ToUnicode.
		//
		// ToUnicode is UTF8ToWide now. These are single code page bytes and not
		// UTF-8, so every byte above 0x7F failed to convert and the mapping for it
		// was lost: a -russian bake kept its 96 ASCII characters and silently
		// dropped all 64 Cyrillic ones. The glyphs were still drawn into the
		// atlas, since that loop does not consult this table, so the texture
		// looked correct and only the metrics were missing.
		//
		// Taking the code page from TranslateCharsetInfo rather than the process
		// default also makes a bake depend on the charset asked for instead of on
		// the locale of the machine it runs on.
		//
		// Per byte because one unmappable byte should not lose the rest: CP1251
		// leaves 0x98 undefined, and a whole-string call with MB_ERR_INVALID_CHARS
		// would fail on it. A multibyte code page ends up here as a run of lead
		// bytes that convert to nothing, which is reported rather than guessed at.
		int nUnmapped = 0;
		for ( int i = 0; i != chars.size(); ++i )
		{
			const char cByte = static_cast<char>( chars[i] );
			wchar_t wch = 0;
			if ( ::MultiByteToWideChar( cs.ciACP, MB_ERR_INVALID_CHARS, &cByte, 1, &wch, 1 ) == 1 )
				( *pTranslate )[ chars[i] ] = wch;
			else
				++nUnmapped;
		}
		if ( nUnmapped > 0 )
		{
			fmt::print( "WARNING: {} of {} characters have no mapping in code page {}\n",
			        nUnmapped, static_cast<int>( chars.size() ), cs.ciACP );
		}
	}
}

// draw font in the DC
bool DrawFont( HDC hdc, const SFontInfo &fi, const std::vector<uint16_t> &chars )
{
  // Draw characters:
  int x = 0, y = 0;
	for ( int i=0; i<chars.size(); ++i )
	{
		int nNextCharShift = fi.abc[i].abcB + abs( fi.abc[i].abcC );
		if ( x + nNextCharShift + N_LEADING_PIXELS > fi.nTextureSizeX )
    {
      ++y;
      x = 0;
      if ( (y + 1) * fi.tm.tmHeight > fi.nTextureSizeY )
        return false;
    }
    x += N_LEADING_PIXELS;
		TextOut( hdc, x - fi.abc[i].abcA, y*fi.tm.tmHeight, (TCHAR*)&( chars[i] ), 1 );
    x += nNextCharShift;
  }
  return true;
}

void CreateFontImage( const SFontInfo &fi, NImage::CImage *pRes, const std::vector<uint16_t> &chars )
{
  // Create an offscreen bitmap:
  int width = fi.nTextureSizeX;//16 * fi.tm.tmMaxCharWidth;
  int height = fi.nTextureSizeY;//14 * fi.tm.tmHeight;
  // Prepare to create a bitmap
  uint8_t *pBitmapBits = 0;
  BITMAPINFO bmi;
  memset( &bmi.bmiHeader, 0, sizeof(bmi.bmiHeader) );
  bmi.bmiHeader.biSize        = sizeof( BITMAPINFOHEADER );
  bmi.bmiHeader.biWidth       = fi.nTextureSizeX;
  bmi.bmiHeader.biHeight      = fi.nTextureSizeY;
  bmi.bmiHeader.biPlanes      = 1;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biBitCount    = 24;
  bmi.bmiHeader.biSizeImage   = abs( bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight * bmi.bmiHeader.biBitCount / 8 );
  // Create a DC and a bitmap for the font
  HDC hDC = CreateCompatibleDC( 0 );
  HBITMAP hbmBitmap = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS, (void**)&pBitmapBits, 0, 0 );
  HBITMAP hOldBmp = (HBITMAP)SelectObject( hDC, hbmBitmap );
  HFONT hOldFont = (HFONT)SelectObject( hDC, fi.hFont );
  // Clear background to black:
  SelectObject( hDC, GetStockObject(BLACK_BRUSH) );
  Rectangle( hDC, 0, 0, width, height );
  SetBkMode( hDC, TRANSPARENT );           // do not fill character background
  SetTextColor( hDC, RGB(255, 255, 255) ); // text color white
  SetTextAlign( hDC, TA_TOP );
  // Draw characters:
  DrawFont( hDC, fi, chars );
  //
  SelectObject( hDC, hOldFont );
  SelectObject( hDC, hOldBmp );
  //
  // create image
	pRes->SetSizes( fi.nTextureSizeX, fi.nTextureSizeY );
  for ( int i=0, j=0; i< fi.nTextureSizeX * fi.nTextureSizeY * 3; i+=3, ++j )
  {
    // The mean of the three subpixels, where this took the green channel alone.
    //
    // Under ClearType the channels are coverage of three different subpixels,
    // so green by itself is the coverage of the middle one and not of the
    // pixel. Reading it as though it were greyscale is what made the atlases
    // close to masks: the shipped ones carry 7 distinct alpha levels despite
    // their records saying Antialiased. Averaging turns the subpixel triple
    // back into one coverage value, and being a downsample of a 3x horizontal
    // supersample it lands finer than greyscale GDI would have.
    const uint32_t nCoverage = ( pBitmapBits[i + 0] + pBitmapBits[i + 1] + pBitmapBits[i + 2] ) / 3;
		(*pRes)[ j / fi.nTextureSizeX ][ j % fi.nTextureSizeX ] = CVec4( 1, 1, 1, nCoverage / 255.0f );
  }
	NImage::FlipY( *pRes );
}

// CFontGen

class CFontGen
{
public:
	static void CreateFontFormat( const char *pszDestFile, const SFontInfo &fi, const std::vector<uint16_t> &chars );
	static void CreateFontFormat( const char *pszDestFile, const NFontRaster::SFont &font, uint8_t cCharSet, uint16_t wDefaultChar );
};

// The FreeType counterpart of the GDI CreateFontFormat below. FontRaster
// already works in GDI's conventions, cell, ABC widths and all, so this is a
// field for field copy; the characters are keyed by code point, which is what
// the GDI path arrives at after translating.
void CFontGen::CreateFontFormat( const char *pszDestFile, const NFontRaster::SFont &font, uint8_t cCharSet, uint16_t wDefaultChar )
{
	CObj<CFontFormatInfo> pFormat( new CFontFormatInfo );
	CFontFormatInfo &format = *pFormat;
	format.nHeight = font.nCellHeight;
	format.nExternalLeading = font.nExternalLeading;
	format.nAveCharWidth = font.nAveCharWidth;
	format.nMaxCharWidth = font.nMaxCharWidth;
	format.cCharSet = cCharSet;
	format.wDefaultChar = wDefaultChar;
	for ( const NFontRaster::SKerningPair &pair : font.kerns )
	{
		format.kerns[( pair.nLeft << 16 ) | pair.nRight] = pair.nAmount;
	}
	for ( const NFontRaster::SGlyph &glyph : font.glyphs )
	{
		STFCharacter &character = format.chars[static_cast<uint16_t>( glyph.nCodePoint )];
		character.x1 = glyph.x1;
		character.y1 = glyph.y1;
		character.x2 = glyph.x2;
		character.y2 = glyph.y2;
		character.nA = glyph.nA;
		character.nBC = glyph.nBC;
		character.nWidth = glyph.nWidth;
	}
	CFileStream file( pszDestFile, CFileStream::WIN_CREATE );
	if ( file.IsOk() )
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &file, SAVER_MODE_WRITE );
		pSaver->Add( 1, &pFormat );
	}
}
void CFontGen::CreateFontFormat( const char *pszDestFile, const SFontInfo &fi, const std::vector<uint16_t> &chars )
{
	const TEXTMETRIC &tm = fi.tm;
	// textmetric and ABCs must be converted to the next data
  //   1. header data
  //   2. all characters
  //   3. all kerning pairs
  // fill texture font header
	CObj<CFontFormatInfo> pFormat( new CFontFormatInfo );
	CFontFormatInfo &format = *pFormat;
	format.nHeight          = tm.tmHeight;
	format.nExternalLeading = tm.tmExternalLeading;
  format.nAveCharWidth    = tm.tmAveCharWidth;
  format.nMaxCharWidth    = tm.tmMaxCharWidth;
  format.cCharSet         = tm.tmCharSet;
	format.wDefaultChar     = tm.tmDefaultChar;
  // kerning pairs
  //std::vector<SKerningPair> kerns( dwNumKerningPairs );
	for ( int i=0; i<fi.kps.size(); ++i )
	{
		uint32_t dwFirst = fi.Translate( fi.kps[i].wFirst );
		uint32_t dwSecond = fi.Translate( fi.kps[i].wSecond );
		// A pair naming a character that did not map is dropped rather than
		// stored under Translate's 0xffff, which would collide every such pair
		// onto one entry and kern unrelated characters by whatever landed last.
		if ( dwFirst == 0xffff || dwSecond == 0xffff )
			continue;
		format.kerns[(dwFirst << 16) | dwSecond] = fi.kps[i].iKernAmount;
	}
  // convert this structures to the STFLetterFull array
  int x = 0, y = 0;
	for ( int i=0; i<chars.size(); ++i )
	{
		uint16_t unicode = fi.Translate( chars[i] );
		//
		int nNextCharShift = fi.abc[i].abcB + abs( fi.abc[i].abcC );
		if ( x + nNextCharShift + N_LEADING_PIXELS > fi.nTextureSizeX )
		{
			++y;
			x = 0;
		}
		x += N_LEADING_PIXELS;

		// Only the entry is skipped when the character did not map, never the
		// advance below: DrawFont walked the same list and drew a glyph for this
		// slot either way, so leaving the advance out would shift every later
		// character's rect off the glyph it describes.
		if ( unicode != 0xffff )
		{
			STFCharacter &character = format.chars[unicode];
			// char ABC parameters in the texture's respective size
			character.nA = fi.abc[i].abcA;
			character.nBC = fi.abc[i].abcB + fi.abc[i].abcC;
			character.nWidth = fi.abc[i].abcB + ( fi.abc[i].abcC > 0 ? fi.abc[i].abcC : 0 );
			// character rect in the texture's coords
			character.x1 = x;
			character.y1 = y * tm.tmHeight;
			character.x2 = x + character.nWidth;
			character.y2 = ( y + 1 ) * tm.tmHeight;
		}
		//
		x += nNextCharShift;
	}
	//
	CFileStream file( pszDestFile, CFileStream::WIN_CREATE );
	if ( file.IsOk() )
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &file, SAVER_MODE_WRITE );
		pSaver->Add( 1, &pFormat );
	}
}

void Generate( LPCSTR pszDstPngFile, LPCSTR pszDstFile, uint32_t dwHeight, uint32_t dwWeight, bool bItalic, uint32_t dwCharSet,
	bool bAntialias, uint32_t dwPitch, LPCTSTR pszFaceName, std::vector<uint16_t> *pChars )
{
	SFontInfo fi;
	LoadFont( GetDesktopWindow(), &fi, dwHeight, dwWeight, bItalic, dwCharSet, bAntialias, dwPitch, pszFaceName, pChars );
	// create font image and font data
	NImage::CImage image;
	CreateFontImage( fi, &image, *pChars );
	CFileStream stream( pszDstPngFile, CFileStream::WIN_CREATE );
	if ( stream.IsOk() ) 
	{
		CArray2D<uint32_t> image2( image.GetSizeX(), image.GetSizeY() );
		NImage::Convert( &image2, image );
		NImage::SaveAsTGA( image2, &stream );
	}

	fmt::print( "font data...\n" );
	CFontGen::CreateFontFormat( pszDstFile, fi, *pChars );
	fmt::print( "well done\n" );
}

// GDI's tmDefaultChar for every TrueType face measured (Tahoma, Impact and
// Arial, RUSSIAN_CHARSET), and the code the GDI path therefore stores the
// default glyph under. No font maps it, so it gets .notdef, the box GDI draws.
const uint16_t W_DEFAULT_CHAR = 0x1F;

// Generate with FreeType instead of GDI: the same character set, blob and TGA,
// but rasterised from szFontFile at exactly nCellHeight pixels. Returns false
// when the font cannot be rasterised.
bool GenerateWithFreeType( const char *pszDstPngFile, const char *pszDstFile, const NFontRaster::SOptions &options,
	uint32_t dwCharSet, std::vector<uint16_t> *pChars )
{
	std::vector<uint16_t> &chars = *pChars;
	if ( find( chars.begin(), chars.end(), W_DEFAULT_CHAR ) == chars.end() )
		chars.push_back( W_DEFAULT_CHAR );
	sort( chars.begin(), chars.end() );
	// The engine looks glyphs up by code point, so the code page bytes are
	// translated first, exactly as the GDI path does; a byte with no mapping
	// has no code point to be stored under and is left out.
	std::unordered_map<uint16_t, uint16_t> translate;
	BuildTranslation( dwCharSet, chars, &translate );
	std::vector<uint32_t> codePoints;
	codePoints.reserve( chars.size() );
	for ( int i = 0; i != chars.size(); ++i )
	{
		std::unordered_map<uint16_t, uint16_t>::const_iterator pos = translate.find( chars[i] );
		if ( pos != translate.end() )
			codePoints.push_back( pos->second );
	}

	NFontRaster::SFont font;
	std::string szError;
	if ( !NFontRaster::Rasterise( options, codePoints, &font, &szError ) )
	{
		fmt::print( "ERROR: {}\n", szError );
		return false;
	}
	fmt::print( "cell {} = ascent {} + descent {}, external leading {}, average width {}, max width {}, {:.3f} px per em\n",
	        font.nCellHeight, font.nAscent, font.nDescent, font.nExternalLeading, font.nAveCharWidth, font.nMaxCharWidth,
	        font.fPixelsPerEm );
	fmt::print( "{} glyphs, {} kerning pairs, atlas {}x{}\n", static_cast<int>( font.glyphs.size() ),
	        static_cast<int>( font.kerns.size() ), font.nAtlasWidth, font.nAtlasHeight );
	// .notdef for the default character is expected; anything more is a
	// character set the font does not cover
	for ( const uint32_t nCodePoint : font.missing )
	{
		if ( nCodePoint != W_DEFAULT_CHAR )
			fmt::print( "WARNING: U+{:04X} is not in the font and got its .notdef glyph\n", static_cast<unsigned>( nCodePoint ) );
	}
	for ( const uint32_t nCodePoint : font.clipped )
		fmt::print( "WARNING: U+{:04X} reaches outside its cell and was clipped to it\n", static_cast<unsigned>( nCodePoint ) );

	fmt::print( "image...\n" );
	// White, with the coverage as alpha, which is what the GDI path produces
	NImage::CImage image;
	image.SetSizes( font.nAtlasWidth, font.nAtlasHeight );
	for ( int y = 0; y < font.nAtlasHeight; ++y )
	{
		for ( int x = 0; x < font.nAtlasWidth; ++x )
			image[y][x] = CVec4( 1, 1, 1, font.atlas[static_cast<size_t>( y ) * font.nAtlasWidth + x] / 255.0f );
	}
	CFileStream stream( pszDstPngFile, CFileStream::WIN_CREATE );
	if ( stream.IsOk() )
	{
		CArray2D<uint32_t> image2( image.GetSizeX(), image.GetSizeY() );
		NImage::Convert( &image2, image );
		NImage::SaveAsTGA( image2, &stream );
	}

	fmt::print( "font data...\n" );
	CFontGen::CreateFontFormat( pszDstFile, font, static_cast<uint8_t>( dwCharSet ), W_DEFAULT_CHAR );
	fmt::print( "well done\n" );
	return true;
}

// params:
//   height (in pixels)
//   weight (100-900. normal == 400, bold == 700)
//   italic (t/f)
//   charset
//   antialiased (t/f)
//   pitch (default, fixed, variable)
//   face name (ZB "Times New Roman")

// ANSI_CHARSET
// BALTIC_CHARSET
// CHINESEBIG5_CHARSET
// DEFAULT_CHARSET
// EASTEUROPE_CHARSET
// GB2312_CHARSET
// GREEK_CHARSET
// HANGUL_CHARSET
// MAC_CHARSET
// OEM_CHARSET
// RUSSIAN_CHARSET
// SHIFTJIS_CHARSET
// SYMBOL_CHARSET
// TURKISH_CHARSET
// Windows NT/2000 or Middle-Eastern Windows 3.1 or later: 
// HEBREW_CHARSET
// ARABIC_CHARSET 
// Windows NT/2000 or Thai Windows 3.1 or later: 
// THAI_CHARSET 

#if !defined(ELK)

static void ShowUsage()
{
	fmt::print( "FontGenerator utility\n(C) Nival Interactive, 2000\n" );
	fmt::print( "Usage: FontGen.exe [options] <\"Font Face Name\"> <BinDstName> <PicDstName> [<CharsSrcName>]\n" );
	fmt::print( "   -h# \t\t font height (in pixels)\n" );
	fmt::print( "   -w# \t\t font weight (400 = normal. 100 <= w <= 900)\n" );
	fmt::print( "   -it \t\t italic\n" );
	fmt::print( "   -aa \t\t antialiased quality\n" );
	fmt::print( "   -pitch \t font pitch (default, fixed, variable)\n" );
	fmt::print( "   -<charset>\t second character set\n" );
	fmt::print( "    charsets: ansi, baltic, chinesebig5, default, easteurope, gb2312,\n" );
	fmt::print( "              greek, hangul, mac, oem, russian, shiftjis, symbol,\n" );
	fmt::print( "              turkish, hebrew, arabic, thai\n" );
	fmt::print( "   [<CharsSrcName>] chars in MBCS formart, all in doublebytes (words)\n" );
	fmt::print( "\n" );
	fmt::print( "   -ft \t\t rasterise with FreeType instead of GDI. The face name is then\n" );
	fmt::print( "       \t\t the font file, e.g. C:\\Windows\\Fonts\\tahoma.ttf, and -h is the\n" );
	fmt::print( "       \t\t exact cell height, which GDI can only approximate. -w, -it and\n" );
	fmt::print( "       \t\t -pitch do not apply: pick the bold or italic file instead\n" );
	fmt::print( "   -hint=<mode>\t FreeType hinting: none, light (default) or normal\n" );
	fmt::print( "   -gamma=<g>\t FreeType coverage gamma, default 1; above 1 thickens edges\n" );
	fmt::print( "   -pad=<n>\t FreeType blank pixels between atlas cells, default 2\n" );
	fmt::print( "   -cell=<m>\t what -h is fitted to: win (default), usWinAscent + usWinDescent\n" );
	fmt::print( "       \t\t as GDI does, or ink, the tallest and deepest of the characters\n" );
	fmt::print( "       \t\t baked, which sizes fonts made for many scripts comparably\n" );
	fmt::print( "   -instance=<n>\t a variable font's named instance, counted from 1\n" );

}

int PORT_CDECL main( int argc, char *argv[] )
{
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
  // prepare command line
  std::vector<std::string> szParams( argc - 1 );
  for ( int i=0; i<argc - 1; ++i )
  {
    szParams[i] = argv[i + 1];
    NStr::ToLower( &szParams[i] );
		//
		if ( szParams[i] == "-show-version" )
		{
			fmt::print( "Version: {}\n", REVISION_NUMBER_STR );
			fmt::print( "Build date/time: {}\n", BUILD_DATE_TIME_STR );
			return 0;
		}
  }
  //
  if ( szParams.empty() )
  {
		ShowUsage();
    return 0xDEAD;
  }
  // initialize charsets map
  std::unordered_map<std::string, uint32_t> charsets;
  charsets["-ansi"]        = ANSI_CHARSET;
  charsets["-baltic"]      = BALTIC_CHARSET;
  charsets["-chinesebig5"] = CHINESEBIG5_CHARSET;
	charsets["-default"]     = DEFAULT_CHARSET;
	charsets["-def_charset"] = DEFAULT_CHARSET;
  charsets["-easteurope"]  = EASTEUROPE_CHARSET;
  charsets["-gb2312"]      = GB2312_CHARSET;
  charsets["-greek"]       = GREEK_CHARSET;
  charsets["-hangul"]      = HANGUL_CHARSET;
  charsets["-mac"]         = MAC_CHARSET;
  charsets["-oem"]         = OEM_CHARSET;
  charsets["-russian"]     = RUSSIAN_CHARSET;
  charsets["-shiftjis"]    = SHIFTJIS_CHARSET;
  charsets["-symbol"]      = SYMBOL_CHARSET;
  charsets["-turkish"]     = TURKISH_CHARSET;
  charsets["-hebrew"]      = HEBREW_CHARSET;
  charsets["-arabic"]      = ARABIC_CHARSET;
  charsets["-thai"]        = THAI_CHARSET;
  // initialize pitch map
  std::unordered_map<std::string, uint32_t> pitches;
  pitches["-default"]  = DEFAULT_PITCH;
  pitches["-fixed"]    = FIXED_PITCH;
  pitches["-variable"] = VARIABLE_PITCH;
  // read default values
  TCHAR buffer[1024];
  GetModuleFileName( 0, buffer, 1024 );
  std::string szString = buffer;
//  szString.erase( szString.find_last_of( '\\' ) );
  szString += "\\fontgen.ini";

  uint32_t dwHeight = 20;
  uint32_t dwWeight = 400;
  bool bItalic = 0;
  bool bAntialias = 0;
  // pitch
  uint32_t dwPitch = VARIABLE_PITCH;
  // charset
  uint32_t dwCharSet = ANSI_CHARSET;//DEFAULT_CHARSET;
  // font face name
  std::string szFaceName = "Times New Roman", szDstFile, szDstPngFile, szCharsSrcName;
	int nOrdinaryParamCount = 0;
	// FreeType settings; the defaults are FontRaster's own
	bool bFreeType = false;
	NFontRaster::SOptions ftOptions;
  // -h20 -w400 -it -russian -aa -variable "Times New Roman"
  for ( std::vector<std::string>::const_iterator pos = szParams.begin(); pos != szParams.end(); ++pos )
  {
		// Before the -h test below, which would otherwise take -hint=... for a
		// height. The parameters are already lower case.
		if ( *pos == "-ft" )
		{
			bFreeType = true;
			continue;
		}
		if ( pos->find( "-hint=" ) == 0 )
		{
			const std::string szMode = pos->substr( 6 );
			if ( szMode == "none" )
				ftOptions.eHinting = NFontRaster::HINTING_NONE;
			else if ( szMode == "light" )
				ftOptions.eHinting = NFontRaster::HINTING_LIGHT;
			else if ( szMode == "normal" )
				ftOptions.eHinting = NFontRaster::HINTING_NORMAL;
			else
			{
				fmt::print( "ERROR: unknown hinting mode \"{}\"\n", szMode );
				return 0xDEAD;
			}
			continue;
		}
		if ( pos->find( "-gamma=" ) == 0 )
		{
			ftOptions.fGamma = static_cast<float>( atof( pos->c_str() + 7 ) );
			continue;
		}
		if ( pos->find( "-pad=" ) == 0 )
		{
			ftOptions.nPadding = atoi( pos->c_str() + 5 );
			continue;
		}
		if ( pos->find( "-cell=" ) == 0 )
		{
			const std::string szMode = pos->substr( 6 );
			if ( szMode == "win" )
				ftOptions.eCellMetrics = NFontRaster::CELL_WIN;
			else if ( szMode == "ink" )
				ftOptions.eCellMetrics = NFontRaster::CELL_INK;
			else
			{
				fmt::print( "ERROR: unknown cell metrics \"{}\"\n", szMode );
				return 0xDEAD;
			}
			continue;
		}
		if ( pos->find( "-instance=" ) == 0 )
		{
			// a variable font's named instance, counted from 1, in the high half
			// of the face index as FreeType takes it
			ftOptions.nFaceIndex = ( ftOptions.nFaceIndex & 0xFFFF ) | ( atoi( pos->c_str() + 10 ) << 16 );
			continue;
		}
    if ( charsets.find(*pos) != charsets.end() )
      dwCharSet = charsets[*pos];
    else if ( pitches.find(*pos) != pitches.end() )
      dwPitch = pitches[*pos];
    else if ( pos->find( "-h" ) == 0 )
      dwHeight = atoi( &((*pos)[2]) );
    else if ( pos->find( "-w" ) == 0 )
      dwWeight = atoi( &((*pos)[2]) );
    else if ( *pos == "-it" )
      bItalic = true;
    else if ( *pos == "-aa" )
      bAntialias = true;
    else
		{
			nOrdinaryParamCount++;
			switch( nOrdinaryParamCount )
			{
				case 1: szFaceName = *pos; break;
				case 2: szDstFile = *pos; break;
				case 3: szDstPngFile = *pos; break;
				case 4: szCharsSrcName = *pos; break;
			}
		}
  }
	if ( nOrdinaryParamCount < 3 )
	{
		ShowUsage();
    return 0xDEAD;
	}
  //
  NStr::TrimInside( szFaceName, '"' );
  NStr::TrimInside( szDstFile, '"' );
  NStr::TrimInside( szDstPngFile, '"' );
  NStr::TrimInside( szCharsSrcName, '"' );
  //
  fmt::print( "generating font \"{}\" ({}:{}:{:d}:{:d})\n", szFaceName, dwHeight, dwWeight, bItalic, bAntialias );
  fmt::print( "image...\n" );
  //
  hWnd = GetDesktopWindow();

	// пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅпїЅ uint16_t пїЅ пїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ MBCS
	std::vector<uint16_t> chars;
	if ( !szCharsSrcName.empty() && NFile::DoesFileExist( szCharsSrcName ) )
	{
		CFileStream fileStream( szCharsSrcName, CFileStream::WIN_READ_ONLY );
		if ( fileStream.IsOk() && fileStream.GetSize() > 0 )
		{
			const int nCharCount = fileStream.GetSize() / sizeof( uint16_t );
			if ( nCharCount > 0 )
			{
				chars.resize( nCharCount );
				fileStream.Read( &( chars[0] ), nCharCount * 2 );
				//
				sort( chars.begin(), chars.end() );
				chars.erase( unique( chars.begin(), chars.end() ), chars.end() );
			}
		}
	}
	if ( chars.empty() )
	{
		chars.reserve( 256 );
		// load font
		for ( int i=32; i<256; ++i ) 
		{
			chars.push_back( i );
		}
	}
	if ( bFreeType )
	{
		ftOptions.szFontFile = szFaceName;
		ftOptions.nCellHeight = static_cast<int>( dwHeight );
		ftOptions.bAntialias = bAntialias;
		return GenerateWithFreeType( szDstPngFile.c_str(), szDstFile.c_str(), ftOptions, dwCharSet, &chars ) ? 0 : 1;
	}
	Generate( szDstPngFile.c_str(), szDstFile.c_str(), dwHeight, dwWeight, bItalic, dwCharSet,
		bAntialias, dwPitch, szFaceName.c_str(), &chars );

	return 0;
}

#endif //#if !defined(ELK)

