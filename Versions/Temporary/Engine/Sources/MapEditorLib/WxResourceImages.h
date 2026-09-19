#pragma once
#include "Resources.h"

// The editor's icon strips -- tree_types.bmp, pc_types.bmp and their headers --
// for wx.
//
// Each strip is a row of 16 pixel icons in one bitmap resource, with magenta
// for the transparent pixels, which MFC made into a CImageList with
// CImageList::Add( &bmp, RGB( 255, 0, 255 ) ). The strips are palette bitmaps.
// Handed to wx as LoadImage gives them, a wxImageList drew every icon as a
// black silhouette, so the pixels are read out as 32-bit colour into a wxImage
// and the mask colour set on that.


#include <wx/bitmap.h>
#include <wx/image.h>

#include <vector>

namespace NWxResourceImages
{
	// The strip nResourceID names, from whichever module holds it, masked; an
	// invalid image when it cannot be read.
	inline wxImage LoadStrip( UINT nResourceID )
	{
		const LPCTSTR pszResource = MAKEINTRESOURCE( nResourceID );
		const HBITMAP hBitmap = static_cast<HBITMAP>( ::LoadImage( NResources::FindModule( pszResource, RT_BITMAP ), pszResource,
																															 IMAGE_BITMAP, 0, 0, 0 ) );
		if ( hBitmap == 0 )
		{
			return wxImage();
		}
		BITMAP header = { 0 };
		if ( ( ::GetObject( hBitmap, sizeof( header ), &header ) == 0 ) || ( header.bmWidth <= 0 ) || ( header.bmHeight <= 0 ) )
		{
			::DeleteObject( hBitmap );
			return wxImage();
		}
		const int nWidth = header.bmWidth;
		const int nHeight = header.bmHeight;
		BITMAPINFO info = {};
		info.bmiHeader.biSize = sizeof( info.bmiHeader );
		info.bmiHeader.biWidth = nWidth;
		// Negative: rows top-down, as wxImage holds them.
		info.bmiHeader.biHeight = -nHeight;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		std::vector<unsigned char> pixels( static_cast<size_t>( nWidth ) * nHeight * 4 );
		const HDC hScreen = ::GetDC( 0 );
		const int nRows = ::GetDIBits( hScreen, hBitmap, 0, nHeight, &pixels[0], &info, DIB_RGB_COLORS );
		::ReleaseDC( 0, hScreen );
		::DeleteObject( hBitmap );
		if ( nRows != nHeight )
		{
			return wxImage();
		}
		wxImage strip( nWidth, nHeight, false );
		unsigned char *const pRGB = strip.GetData();
		for ( size_t nPixel = 0; nPixel < static_cast<size_t>( nWidth ) * nHeight; ++nPixel )
		{
			// BGRX in, RGB out.
			pRGB[nPixel * 3 + 0] = pixels[nPixel * 4 + 2];
			pRGB[nPixel * 3 + 1] = pixels[nPixel * 4 + 1];
			pRGB[nPixel * 3 + 2] = pixels[nPixel * 4 + 0];
		}
		strip.SetMaskColour( 255, 0, 255 );
		return strip;
	}


	// The strip cut into its nSize pixel icons, left to right; empty when it
	// cannot be read.
	inline std::vector<wxBitmap> LoadIcons( UINT nResourceID, int nSize = 16 )
	{
		std::vector<wxBitmap> icons;
		const wxImage strip = LoadStrip( nResourceID );
		if ( !strip.IsOk() )
		{
			return icons;
		}
		for ( int nX = 0; nX + nSize <= strip.GetWidth(); nX += nSize )
		{
			icons.push_back( wxBitmap( strip.GetSubImage( wxRect( nX, 0, nSize, ( std::min )( nSize, strip.GetHeight() ) ) ) ) );
		}
		return icons;
	}
}

