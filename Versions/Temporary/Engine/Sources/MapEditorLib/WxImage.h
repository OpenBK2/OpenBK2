#pragma once

// Turning the editor's Windows icons into something wx can draw.
//
// The editor's graphics are .ico and .bmp files compiled into the resource
// section of MapEditor.dll, and reached by numeric id -- IDR_MAINFRAME is 129.
// That is the one form wx cannot load portably. wxBitmapBundle::FromResources
// takes a *name*, wxICON takes a name, and none of it exists off Windows at all.
//
// So the icon is read from the file instead, the way the game's window icon
// already is off Windows: NWinImage parses the ICO container and hands back
// 32-bit ARGB pixels, and those go straight into a wxImage. Bytes in, pixels
// out, the same code on every platform, and no ::LoadIcon.
//
// Header-only, like the other Wx*.h here, because MapEditorLib does not link wx
// -- only the front-end targets that include these do.

#ifdef OBK2_WITH_WX

#include "System/WinImageFormats.h"

#include <wx/bmpbndl.h>
#include <wx/bitmap.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/stdpaths.h>

#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>

namespace NWxImage
{
	// Where the editor's own files live: beside the executable.
	//
	// Not NMainLoop::GetBaseDir(), which is the parent of the working directory
	// and points at the *game data* -- Data, Profiles, splash.bmp. An icon
	// belonging to the editor binary is not game data and should not have to be
	// found next to a map. wxStandardPaths answers this without asking Windows
	// directly, which is the whole reason it exists.
	inline wxString AssetPath( const wxString &rFileName )
	{
		wxFileName path( wxStandardPaths::Get().GetExecutablePath() );
		path.SetFullName( rFileName );
		return path.GetFullPath();
	}


	// One decoded entry of an ICO container as a wxImage, with its alpha.
	//
	// NWinImage hands back ARGB in a uint32 per pixel, top-down and unpadded;
	// wxImage wants three planes of bytes and a separate alpha plane. That is
	// the whole of this function.
	inline bool DecodeEntry( wxImage *pImage, const std::vector<uint8_t> &rData, int nIndex )
	{
		NWinImage::SImage decoded;
		if ( !NWinImage::DecodeImage( &decoded, &rData[0], rData.size(), 0, nIndex ) )
		{
			return false;
		}
		wxImage image( decoded.nWidth, decoded.nHeight, false );
		image.InitAlpha();
		unsigned char *pRGB = image.GetData();
		unsigned char *pAlpha = image.GetAlpha();
		const size_t nPixels = (size_t)decoded.nWidth * (size_t)decoded.nHeight;
		for ( size_t nPixel = 0; nPixel < nPixels; ++nPixel )
		{
			const uint32_t nARGB = decoded.pixels[nPixel];
			pRGB[nPixel * 3 + 0] = (unsigned char)( ( nARGB >> 16 ) & 0xFF );
			pRGB[nPixel * 3 + 1] = (unsigned char)( ( nARGB >> 8 ) & 0xFF );
			pRGB[nPixel * 3 + 2] = (unsigned char)( nARGB & 0xFF );
			pAlpha[nPixel] = (unsigned char)( ( nARGB >> 24 ) & 0xFF );
		}
		( *pImage ) = image;
		return true;
	}


	// The entry of exactly nSize pixels square, at the best colour depth there
	// is, or -1. Distinct from NWinImage::SelectImage, which answers "nearest".
	inline int FindExactSize( const std::vector<NWinImage::SImageInfo> &rInfos, int nSize )
	{
		int nBest = -1;
		for ( size_t nIndex = 0; nIndex < rInfos.size(); ++nIndex )
		{
			const NWinImage::SImageInfo &rInfo = rInfos[nIndex];
			if ( !rInfo.bSupported || rInfo.nWidth != nSize || rInfo.nHeight != nSize )
			{
				continue;
			}
			if ( nBest < 0 || rInfo.nBpp > rInfos[nBest].nBpp )
			{
				nBest = (int)nIndex;
			}
		}
		return nBest;
	}


	// The nSize icon out of an .ico, with its high-DPI variants if the file has
	// them, as a bundle.
	//
	// **nSize is not optional, and handing wx every entry instead is wrong.**
	// A wxBitmapBundle is one logical size with several resolutions of it: wx
	// takes the smallest bitmap's size as the bundle's size in DIPs and treats
	// everything larger as a 2x or 3x rendering of that same slot. An .ico is
	// not that -- its 16, 32 and 48 entries are drawings for three different
	// slots, a tree row and a dialog and a large-icon view. Passing all six
	// entries of main_frame.ico made the About box's icon 16x16, because wx
	// correctly concluded the bundle's logical size was 16.
	//
	// So the caller says which slot it is filling, and the 2x and 3x entries of
	// *that* size go in beside it, which is the scale-variant relationship wx is
	// actually asking about. For a 16 pixel slot in a file holding 16/32/48 that
	// is all three; for a 32 pixel slot it is usually just the one.
	//
	// An empty bundle if the file is missing or holds nothing decodable; every
	// caller should cope, because a missing icon is not worth a failed dialog.
	inline wxBitmapBundle LoadIconFile( const wxString &rPath, int nSize )
	{
		std::ifstream file( rPath.utf8_str().data(), std::ios::binary );
		const std::vector<uint8_t> data( ( std::istreambuf_iterator<char>( file ) ),
																		 std::istreambuf_iterator<char>() );
		std::vector<NWinImage::SImageInfo> infos;
		if ( data.empty() || !NWinImage::GetImages( &infos, &data[0], data.size(), 0 ) )
		{
			return wxBitmapBundle();
		}
		// Nearest, not exact: a file without the size asked for should still
		// give something rather than nothing.
		const int nBase = NWinImage::SelectImage( infos, nSize );
		wxImage image;
		if ( nBase < 0 || !DecodeEntry( &image, data, nBase ) )
		{
			return wxBitmapBundle();
		}
		wxVector<wxBitmap> bitmaps;
		bitmaps.push_back( wxBitmap( image ) );

		// Whatever was actually found, not what was asked for, because that is
		// the size wx will call this bundle's logical size.
		const int nLogical = image.GetWidth();
		for ( int nScale = 2; nScale <= 3; ++nScale )
		{
			const int nIndex = FindExactSize( infos, nLogical * nScale );
			if ( nIndex >= 0 && DecodeEntry( &image, data, nIndex ) )
			{
				bitmaps.push_back( wxBitmap( image ) );
			}
		}
		return wxBitmapBundle::FromBitmaps( bitmaps );
	}


	// The same, for a file installed beside the executable.
	inline wxBitmapBundle LoadIcon( const wxString &rFileName, int nSize )
	{
		return LoadIconFile( AssetPath( rFileName ), nSize );
	}
}

#endif // OBK2_WITH_WX
