#include "stdafx.h"
#include "revision.h"
#include "FontFormat.h"
#include "../Image/Targa.h"
#include "../Misc/2DArray.h"
#include "../Misc/StrProc.h"
#include "../Misc/nalgoritm.h"
#include "../System/FileUtils.h"

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
	vector<ABC> abc;									// character ABC widths
	vector<KERNINGPAIR> kps;					// kernging pairs
	int nTextureSizeX, nTextureSizeY;			// estimated texture size
	hash_map<WORD, WORD> translate;	// ANSI => UNICODE translation table
	//
	WORD Translate( WORD code ) const
	{
		hash_map<WORD, WORD>::const_iterator pos = translate.find( code );
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
inline bool IsFit( const SFontInfo &fi, DWORD dwNumChars, DWORD dwSizeX, DWORD dwSizeY )
{
  return ( dwSizeX / (fi.tm.tmAveCharWidth + 2) ) * ( dwSizeY / fi.tm.tmHeight ) >= dwNumChars;
}

bool EstimateTextureSize( SFontInfo *pFI, DWORD dwNumChars )
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
void MeasureFont( HDC hdc, SFontInfo *pFI, vector<WORD> *pChars )
{
	vector<WORD> &chars = *pChars;
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

int CALLBACK EnumFontFamExProc( ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam )
{
	vector<WORD> *pChars = (vector<WORD>*)lParam;

	printf( "\nFont: %S %S %S", lpelfe->elfFullName, lpelfe->elfStyle, lpelfe->elfScript );
	if ( FontType == TRUETYPE_FONTTYPE )
	{
		for ( int nTemp = 0; nTemp < 126; nTemp++ )
		{
			int nIndex = nTemp / 32;
			int nShift = 31 - nTemp % 32;

			int nMask = 1 << nShift;
			if ( lpntme->ntmFontSig.fsUsb[nIndex] & nMask )
			{
				for ( int nRange = 0; nRange < ARRAY_SIZE( sCharRanges ); nRange++ )
				{
					if ( sCharRanges[nRange].nBit != nTemp )
						continue;

					printf( "\n\tFound %s [%d-%d]", sCharRanges[nRange].szName, sCharRanges[nRange].nRangeBegin, sCharRanges[nRange].nRangeEnd );
					for ( int nTemp = sCharRanges[nRange].nRangeBegin; nTemp < sCharRanges[nRange].nRangeEnd; nTemp++ )
						pChars->push_back( nTemp );
				}
			}
		}
	}
	else
	{
		printf( "\nRaster" );
	}

	return TRUE;
}

static bool IsWinXPOrLater()
{
	OSVERSIONINFO osvi;

	Zero( osvi );
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if ( !GetVersionEx( &osvi ) )
		return false;

	return (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && ( osvi.dwMajorVersion > 4 ) && ( osvi.dwMinorVersion > 0 );
}

void LoadFont( HWND hWnd, SFontInfo *pFI, int nHeight, int nWeight, bool bItalic, DWORD dwCharSet, 
	bool bAntialias, DWORD dwPitch, LPCTSTR pszFaceName, vector<WORD> *pChars )
{
	SFontInfo &fi = *pFI;
	vector<WORD> &chars = *pChars;
  // invoke ChooseFont common dialog:
  // create an HFONT:
  if ( fi.hFont )
  { 
    DeleteObject( fi.hFont ); 
    fi.hFont = 0;
  }

/*
	wstring wsUnicodeName( NStr::ToUnicode( szFaceName ) );

	printf( "\n=============================================================" );

	DWORD dwTemp = dwCharSet;
	CHARSETINFO sCharSetInfo;
	memset( &sCharSetInfo, 0, sizeof( CHARSETINFO ) );
	if ( TranslateCharsetInfo( (DWORD*)dwTemp, &sCharSetInfo, TCI_SRCCHARSET ) )
	{
		printf( "\nCS %d CP %d === %x %x %x %x", sCharSetInfo.ciCharset, sCharSetInfo.ciACP, sCharSetInfo.fs.fsUsb[0], sCharSetInfo.fs.fsUsb[1], sCharSetInfo.fs.fsUsb[2], sCharSetInfo.fs.fsUsb[3] );
		for ( int nTemp = 0; nTemp < 126; nTemp++ )
		{
			int nIndex = nTemp / 32;
			int nShift = 31 - nTemp % 32;

			int nMask = 1 << nShift;
			if ( sCharSetInfo.fs.fsUsb[nIndex] & nMask )
			{
				for ( int nRange = 0; nRange < ARRAY_SIZE( sCharRanges ); nRange++ )
				{
					if ( sCharRanges[nRange].nBit != nTemp )
						continue;

					printf( "\n\tFound %s [%d-%d]", sCharRanges[nRange].szName, sCharRanges[nRange].nRangeBegin, sCharRanges[nRange].nRangeEnd );
					for ( int nTemp = sCharRanges[nRange].nRangeBegin; nTemp < sCharRanges[nRange].nRangeEnd; nTemp++ )
						pChars->push_back( nTemp );
				}
			}
		}
	}
	else
		printf( "\n ERROR: %d", GetLastError() );

	printf( "\n=============================================================" );

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

	DWORD dwQuality = bAntialias ? (IsWinXPOrLater() ? 6 : ANTIALIASED_QUALITY) : NONANTIALIASED_QUALITY;
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
	{
		CHARSETINFO cs;
		BOOL bRetVal = TranslateCharsetInfo( (DWORD*)dwCharSet, &cs, TCI_SRCCHARSET );
		ASSERT( bRetVal == TRUE );
		NStr::SetCodePage( cs.ciACP );
		// form string
		string szCharacters;
		szCharacters.resize( chars.size() );
		for ( int i = 0; i != chars.size(); ++i )
			szCharacters[i] = chars[i];
		wstring szUNICODE;
		NStr::ToUnicode( &szUNICODE, szCharacters );
		// create re-map table
		for ( int i = 0; i != chars.size(); ++i )
			fi.translate[ chars[i] ]= szUNICODE[i];
	}
  // select old font
  ::SelectObject( hdc, hOldFont );
  // release HDC:
  ReleaseDC( hWnd, hdc );
}

// draw font in the DC
bool DrawFont( HDC hdc, const SFontInfo &fi, const vector<WORD> &chars )
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

void CreateFontImage( const SFontInfo &fi, NImage::CImage *pRes, const vector<WORD> &chars )
{
  // Create an offscreen bitmap:
  int width = fi.nTextureSizeX;//16 * fi.tm.tmMaxCharWidth;
  int height = fi.nTextureSizeY;//14 * fi.tm.tmHeight;
  // Prepare to create a bitmap
  BYTE *pBitmapBits = 0;
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
    //DWORD b = pBitmapBits[i + 0];
    DWORD g = pBitmapBits[i + 1];
    //DWORD r = pBitmapBits[i + 2];
		(*pRes)[ j / fi.nTextureSizeX ][ j % fi.nTextureSizeX ] = CVec4( 1, 1, 1, g / 255.0f );
  }
	NImage::FlipY( *pRes );
}

// CFontGen

class CFontGen
{
public:
	static void CreateFontFormat( const char *pszDestFile, const SFontInfo &fi, const vector<WORD> &chars );
};
void CFontGen::CreateFontFormat( const char *pszDestFile, const SFontInfo &fi, const vector<WORD> &chars )
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
  //vector<SKerningPair> kerns( dwNumKerningPairs );
	for ( int i=0; i<fi.kps.size(); ++i )
	{
		DWORD dwFirst = fi.Translate( fi.kps[i].wFirst );
		DWORD dwSecond = fi.Translate( fi.kps[i].wSecond );
		format.kerns[(dwFirst << 16) | dwSecond] = fi.kps[i].iKernAmount;
	}
  // convert this structures to the STFLetterFull array
  int x = 0, y = 0;
	for ( int i=0; i<chars.size(); ++i )
	{
		WORD unicode = fi.Translate( chars[i] );
		//
		int nNextCharShift = fi.abc[i].abcB + abs( fi.abc[i].abcC );
		if ( x + nNextCharShift + N_LEADING_PIXELS > fi.nTextureSizeX )
		{
			++y;
			x = 0;
		}
		x += N_LEADING_PIXELS;

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

void Generate( LPCSTR pszDstPngFile, LPCSTR pszDstFile, DWORD dwHeight, DWORD dwWeight, bool bItalic, DWORD dwCharSet, 
	bool bAntialias, DWORD dwPitch, LPCTSTR pszFaceName, vector<WORD> *pChars )
{
	SFontInfo fi;
	LoadFont( GetDesktopWindow(), &fi, dwHeight, dwWeight, bItalic, dwCharSet, bAntialias, dwPitch, pszFaceName, pChars );
	// create font image and font data
	NImage::CImage image;
	CreateFontImage( fi, &image, *pChars );
	CFileStream stream( pszDstPngFile, CFileStream::WIN_CREATE );
	if ( stream.IsOk() ) 
	{
		CArray2D<DWORD> image2( image.GetSizeX(), image.GetSizeY() );
		NImage::Convert( &image2, image );
		NImage::SaveAsTGA( image2, &stream );
	}

	printf( "font data...\n" );
	CFontGen::CreateFontFormat( pszDstFile, fi, *pChars );
	printf( "well done\n" );
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
	printf( "FontGenerator utility\n(C) Nival Interactive, 2000\n" );
	printf( "Usage: FontGen.exe [options] <\"Font Face Name\"> <BinDstName> <PicDstName> [<CharsSrcName>]\n" );
	printf( "   -h# \t\t font height (in pixels)\n" );
	printf( "   -w# \t\t font weight (400 = normal. 100 <= w <= 900)\n" );
	printf( "   -it \t\t italic\n" );
	printf( "   -aa \t\t antialiased quality\n" );
	printf( "   -pitch \t font pitch (default, fixed, variable)\n" );
	printf( "   -<charset>\t second character set\n" );
	printf( "    charsets: ansi, baltic, chinesebig5, default, easteurope, gb2312,\n" );
	printf( "              greek, hangul, mac, oem, russian, shiftjis, symbol,\n" );
	printf( "              turkish, hebrew, arabic, thai\n" );
	printf( "   [<CharsSrcName>] chars in MBCS formart, all in doublebytes (words)\n" );
	
}

int __cdecl main( int argc, char *argv[] )
{
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
  // prepare command line
  vector<string> szParams( argc - 1 );
  for ( int i=0; i<argc - 1; ++i )
  {
    szParams[i] = argv[i + 1];
    NStr::ToLower( &szParams[i] );
		//
		if ( szParams[i] == "-show-version" )
		{
			printf( "Version: %s\n", REVISION_NUMBER_STR );
			printf( "Build date/time: %s\n", BUILD_DATE_TIME_STR );
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
  hash_map<string, DWORD> charsets;
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
  hash_map<string, DWORD> pitches;
  pitches["-default"]  = DEFAULT_PITCH;
  pitches["-fixed"]    = FIXED_PITCH;
  pitches["-variable"] = VARIABLE_PITCH;
  // read default values
  TCHAR buffer[1024];
  GetModuleFileName( 0, buffer, 1024 );
  string szString = buffer;
//  szString.erase( szString.find_last_of( '\\' ) );
  szString += "\\fontgen.ini";

  DWORD dwHeight = 20;
  DWORD dwWeight = 400;
  bool bItalic = 0;
  bool bAntialias = 0;
  // pitch
  DWORD dwPitch = VARIABLE_PITCH;
  // charset
  DWORD dwCharSet = ANSI_CHARSET;//DEFAULT_CHARSET;
  // font face name
  string szFaceName = "Times New Roman", szDstFile, szDstPngFile, szCharsSrcName;
	int nOrdinaryParamCount = 0;
  // -h20 -w400 -it -russian -aa -variable "Times New Roman"
  for ( vector<string>::const_iterator pos = szParams.begin(); pos != szParams.end(); ++pos )
  {
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
  printf( "generating font \"%s\" (%d:%d:%d:%d)\n", szFaceName.c_str(), dwHeight, dwWeight, bItalic, bAntialias );
  printf( "image...\n" );
  //
  hWnd = GetDesktopWindow();

	// пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅпїЅ WORD пїЅ пїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ MBCS
	vector<WORD> chars;
	if ( !szCharsSrcName.empty() && NFile::DoesFileExist( szCharsSrcName ) )
	{
		CFileStream fileStream( szCharsSrcName, CFileStream::WIN_READ_ONLY );
		if ( fileStream.IsOk() && fileStream.GetSize() > 0 )
		{
			const int nCharCount = fileStream.GetSize() / sizeof( WORD );
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
	Generate( szDstPngFile.c_str(), szDstFile.c_str(), dwHeight, dwWeight, bItalic, dwCharSet, 
		bAntialias, dwPitch, szFaceName.c_str(), &chars );

	return 0;
}

#endif //#if !defined(ELK)

