#include "StdAfx.h"
#include "FontInfo.h"
#include "Misc/2DArray.h"
#include "Image/ImageConvertor.h"
#include "3Dmotor/FontFormat.h"

namespace NFontGen
{

static bool DrawFont( HDC hdc, const CFontInfo &fi )
{
	const vector<ABC> &abc = fi.GetABC();
	const vector<WORD> &chars = fi.GetMBCSChars();
	const TEXTMETRIC &tm = fi.GetTextMetrics();
	const CTPoint<int> textureSize = fi.GetTextureSize();
	//
	int x = 0, y = 0;
	for ( int i = 0; i < chars.size(); ++i )
	{
		const int nNextCharShift = abc[i].abcB + abs( abc[i].abcC );
		if ( x + nNextCharShift + N_LEADING_PIXELS > textureSize.x )
		{
			++y;
			x = 0;
			if ( (y + 1) * tm.tmHeight > textureSize.y )
				return false;
		}
		x += N_LEADING_PIXELS;
		TextOut( hdc, x - abc[i].abcA, y*tm.tmHeight, (TCHAR*)&( chars[i] ), 1 );
		x += nNextCharShift;
	}
	return true;
}

void CreateFontImage( CArray2D<DWORD> *pRes, const CFontInfo &fi )
{
	const CTPoint<int> textureSize = fi.GetTextureSize();
	// Prepare to create a bitmap
	BITMAPINFO bmi;
	memset( &bmi.bmiHeader, 0, sizeof(bmi.bmiHeader) );
	bmi.bmiHeader.biSize        = sizeof( BITMAPINFOHEADER );
	bmi.bmiHeader.biWidth       = textureSize.x;
	bmi.bmiHeader.biHeight      = textureSize.y;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount    = 24;
	bmi.bmiHeader.biSizeImage   = abs( bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight * bmi.bmiHeader.biBitCount / 8 );
	// Create a DC and a bitmap for the font
	HDC hDC = CreateCompatibleDC( 0 );
	BYTE *pBitmapBits = 0;
	HBITMAP hbmBitmap = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS, (void**)&pBitmapBits, 0, 0 );
	HBITMAP hOldBmp = (HBITMAP)SelectObject( hDC, hbmBitmap );
	HFONT hOldFont = (HFONT)SelectObject( hDC, fi.GetFont() );
	// Clear background to black:
	SelectObject( hDC, GetStockObject(BLACK_BRUSH) );
	Rectangle( hDC, 0, 0, textureSize.x, textureSize.y );
	SetBkMode( hDC, TRANSPARENT );           // do not fill character background
	SetTextColor( hDC, RGB(255, 255, 255) ); // text color white
	SetTextAlign( hDC, TA_TOP );
	// Draw characters:
	DrawFont( hDC, fi );
	//
	SelectObject( hDC, hOldFont );
	SelectObject( hDC, hOldBmp );
	//
	// create image
	pRes->SetSizes( textureSize.x, textureSize.y );
	// b, g, r = 0, 1, 2 
	for ( int i = 0, j = 0; i< textureSize.x*textureSize.y*3; i += 3, ++j )
		(*pRes)[j / textureSize.x][j % textureSize.x] = (DWORD(pBitmapBits[i + 1]) << 24) | 0x00ffffff;
	NImage::FlipY( *pRes );
}

void CreateFontFormat( CFontFormatInfo *pRes, const CFontInfo &fi )
{
	const TEXTMETRIC &tm = fi.GetTextMetrics();
	// textmetric and ABCs must be converted to the next data
	//   1. header data
	//   2. all characters
	//   3. all kerning pairs
	// fill texture font header
	pRes->nHeight          = tm.tmHeight;
	pRes->nExternalLeading = tm.tmExternalLeading;
	pRes->nAveCharWidth    = tm.tmAveCharWidth;
	pRes->nMaxCharWidth    = tm.tmMaxCharWidth;
	pRes->cCharSet         = tm.tmCharSet;
	pRes->wDefaultChar     = tm.tmDefaultChar;
	// kerning pairs
	const vector<KERNINGPAIR> &kps = fi.GetKerningPairs();
	for ( int i = 0; i < kps.size(); ++i )
	{
		DWORD dwFirst = fi.Translate( kps[i].wFirst );
		DWORD dwSecond = fi.Translate( kps[i].wSecond );
		pRes->kerns[(dwFirst << 16) | dwSecond] = kps[i].iKernAmount;
	}
	// convert this structures to the STFLetterFull array
	int x = 0, y = 0;
	const vector<WORD> &chars = fi.GetMBCSChars();
	const vector<ABC> &abc = fi.GetABC();
	const CTPoint<int> textureSize = fi.GetTextureSize();
	for ( int i = 0; i < chars.size(); ++i )
	{
		WORD unicode = fi.Translate( chars[i] );
		//
		int nNextCharShift = abc[i].abcB + abs( abc[i].abcC );
		if ( x + nNextCharShift + N_LEADING_PIXELS > textureSize.x )
		{
			++y;
			x = 0;
		}
		x += N_LEADING_PIXELS;

		STFCharacter &character = pRes->chars[unicode];
		// char ABC parameters in the texture's respective size
		character.nA = abc[i].abcA;
		character.nBC = abc[i].abcB + abc[i].abcC;
		character.nWidth = abc[i].abcB + ( abc[i].abcC > 0 ? abc[i].abcC : 0 );
		// character rect in the texture's coords
		character.x1 = x;
		character.y1 = y * tm.tmHeight;
		character.x2 = x + character.nWidth;
		character.y2 = ( y + 1 ) * tm.tmHeight;
		//
		x += nNextCharShift;
	}
}

}
