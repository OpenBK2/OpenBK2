#include "stdafx.h"
#include "FontInfo.h"
#include "Misc/StrProc.h"

namespace NFontGen
{

WORD CFontInfo::Translate( WORD code ) const
{
	std::unordered_map<WORD, WORD>::const_iterator pos = translate.find( code );
	return pos != translate.end() ? pos->second : 0xffff;
}

inline bool IsFit( const TEXTMETRIC &tm, DWORD dwNumChars, DWORD dwSizeX, DWORD dwSizeY )
{
	return ( dwSizeX / (tm.tmAveCharWidth + 2) ) * ( dwSizeY / tm.tmHeight ) >= dwNumChars;
}

bool CFontInfo::EstimateTextureSize( DWORD dwNumChars )
{
	// texture sizes from 64 (1<<6) to 2048 (1<<12)
	for ( int i = 6; i < 13; ++i )
	{
		// first, try to estimate 2:1 size
		if ( IsFit( tm, dwNumChars, 1 << i, 1 << (i - 1) ) )
		{
			nTextureSizeX = 1 << i;
			nTextureSizeY = 1 << (i - 1);
			return true;
		}
		// then, try to estimate 1:1 size
		else if ( IsFit( tm, dwNumChars, 1 << i, 1 << i ) )
		{
			nTextureSizeX = 1 << i;
			nTextureSizeY = 1 << i;
			return true;
		}
	}
	// too big texture!!!
	return false;
}

struct SKerningPairZeroFunctional 
{
	bool operator()( const KERNINGPAIR &kp ) const { return kp.iKernAmount == 0; } 
};
//
bool CFontInfo::MeasureFont( HDC hdc, std::vector<WORD> *pChars )
{
	::GetTextMetrics( hdc, &tm );
	sort( pChars->begin(), pChars->end() );
	if ( find( pChars->begin(), pChars->end(), tm.tmDefaultChar ) == pChars->end() )
		pChars->push_back( tm.tmDefaultChar );
	// measure TrueType fonts with GetCharABCWidths:
	abc.resize( pChars->size() );
	if ( GetCharABCWidths(hdc, (*pChars)[0], (*pChars)[0], &(abc[0])) == FALSE )
	{
		// if it's not a TrueType font, use GetTextExtentPoint32 to fill array abc:
		SIZE size;
		for ( int i = 0; i < pChars->size(); ++i )
		{
			// get width of character...
			GetTextExtentPoint32( hdc, (TCHAR*)&( (*pChars)[i] ), 1, &size );
			// ...and store it in abcB:
			abc[i].abcA = 0;
			abc[i].abcB = size.cx;
			abc[i].abcC = 0;
		}
	}
	else
	{
		for ( int i = 0; i < pChars->size(); ++i )
			GetCharABCWidths( hdc, (*pChars)[i], (*pChars)[i], &( abc[i] ) );
	}
	// get kerning pairs
	KERNINGPAIR kernpair;
	Zero( kernpair );
	kps.resize(0);
	kps.resize( pChars->size() * pChars->size(), kernpair );
	::GetKerningPairs( hdc, pChars->size() * pChars->size(), &( kps[0] ) );
	// remove kerning pairs with '0' kern value
	kps.erase( remove_if( kps.begin(), kps.end(), SKerningPairZeroFunctional() ), kps.end() );

	// estimate texture size
	if ( !EstimateTextureSize( pChars->size() ) )
		return false; // too large texture !!!
	// check and correct size estimating
	int x = 0, y = 0;
	for ( int i = 0; i < pChars->size(); ++i )
	{
		int nNextCharShift = abc[i].abcB + abs( abc[i].abcC );
		if ( x + nNextCharShift + N_LEADING_PIXELS > nTextureSizeX )
		{
			++y;
			x = 0;
			if ( (y + 1) * tm.tmHeight > nTextureSizeY )
			{
				if ( nTextureSizeX == nTextureSizeY ) // if we have 1:1 sizes, make it 2:1
					nTextureSizeX <<= 1;
				else                                   // else, if we have 2:1 already, make it 2:2 :)
					nTextureSizeY = nTextureSizeX;
				break;
			}
		}
		x += N_LEADING_PIXELS;
		x += nNextCharShift;
	}
	return true;
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

bool CFontInfo::LoadFontInfo( const SSourceParams &_source, std::vector<WORD> *pChars, HWND hWnd )
{
	source = _source;
	if ( hFont )
	{ 
		DeleteObject( hFont ); 
		hFont = 0;
	}
	const DWORD dwQuality = source.bAntialias ? (IsWinXPOrLater() ? 6 : ANTIALIASED_QUALITY) : NONANTIALIASED_QUALITY;
	hFont = ::CreateFont( source.nHeight, 0, 0, 0, source.nWeight, source.bItalic, FALSE, FALSE, 
		source.dwCharSet, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
		dwQuality, source.dwPitch, source.szFaceName.c_str() );
	// retrieve logfont
	// get HDC:
	HDC hdc = GetDC( hWnd );
	// select font:
	HFONT hOldFont = (HFONT)::SelectObject( hdc, hFont );
	//
	// get text metrics and char widths:
	if ( MeasureFont( hdc, pChars ) == false )
		return false;
	//
	mbcsChars = *pChars;
	// translate MBCS chars to UNICODE and re-map kerns and chars
	{
		CHARSETINFO cs;
		BOOL bRetVal = TranslateCharsetInfo( (DWORD*)source.dwCharSet, &cs, TCI_SRCCHARSET );
		ASSERT( bRetVal == TRUE );
		NStr::SetCodePage( cs.ciACP );
		// form string
		std::string szCharacters;
		szCharacters.resize( pChars->size() );
		for ( int i = 0; i != pChars->size(); ++i )
			szCharacters[i] = (*pChars)[i];
		std::wstring szUNICODE;
		NStr::ToUnicode( &szUNICODE, szCharacters );
		// create re-map table
		for ( int i = 0; i != pChars->size(); ++i )
			translate[ (*pChars)[i] ]= szUNICODE[i];
	}
	// select old font
	::SelectObject( hdc, hOldFont );
	// release HDC:
	ReleaseDC( hWnd, hdc );
	//
	return true;
}

}
