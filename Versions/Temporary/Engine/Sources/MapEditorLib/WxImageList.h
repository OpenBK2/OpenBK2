#pragma once

// The wx side of IImageList: the object collector's icon caches.
//
// This was CNativeImageList over a Win32 HIMAGELIST, and the three palettes
// that show those icons handed the handle straight to their list controls with
// ListView_SetImageList, because wxImageList cannot adopt an existing
// HIMAGELIST and copying would have decoded every icon a second time.
//
// Nothing upstream was ever Win32. The data extractors answer with a
// CArray2D<uint32_t> of pixels, and Load2Bitmap turned those into an HBITMAP
// only so ImageList_AddMasked could copy it and the HBITMAP be deleted again.
// With the cache a wxImageList from the start there is no handle to adopt and
// no second decode, so the objection goes away with the Win32.
//
// The palettes call wxListCtrl::SetImageList, which does not take ownership --
// the same property LVS_SHAREIMAGELISTS gave them. The collector owns both
// lists and outlives every palette.
//
// Header-only, like the other Wx*.h here; see WxWidget.h for why these stay
// out of MapEditorLib's own translation units.


#include "Interface_Widget.h"
#include "Misc/2Darray.h"

#include <wx/bitmap.h>
#include <wx/colour.h>
#include <wx/image.h>
#include <wx/imaglist.h>

#include <cstdint>

namespace NWxImageList
{
	// A block of pixels as a wxImage.
	//
	// DDS decoding and the icon cache carry 0xAARRGGBB pixels. Preserve the
	// alpha channel: masking black instead punches holes in opaque details
	// and exposes the grey RGB stored behind transparent pixels.
	inline wxImage ToWxImage( const CArray2D<uint32_t> &rPixels )
	{
		const int nWidth = rPixels.GetSizeX();
		const int nHeight = rPixels.GetSizeY();
		if ( ( nWidth <= 0 ) || ( nHeight <= 0 ) )
		{
			return wxImage();
		}
		wxImage image( nWidth, nHeight, false );
		image.InitAlpha();
		unsigned char *const pRGB = image.GetData();
		unsigned char *const pAlpha = image.GetAlpha();
		for ( int nY = 0; nY < nHeight; ++nY )
		{
			for ( int nX = 0; nX < nWidth; ++nX )
			{
				const uint32_t nPixel = rPixels[nY][nX];
				unsigned char *const pTexel = pRGB + ( ( static_cast<size_t>( nY ) * nWidth ) + nX ) * 3;
				pTexel[0] = static_cast<unsigned char>( ( nPixel >> 16 ) & 0xFF );
				pTexel[1] = static_cast<unsigned char>( ( nPixel >> 8 ) & 0xFF );
				pTexel[2] = static_cast<unsigned char>( nPixel & 0xFF );
				pAlpha[static_cast<size_t>(nY) * nWidth + nX] = static_cast<unsigned char>( nPixel >> 24 );
			}
		}
		return image;
	}
}


class CWxImageList : public IImageList
{
	wxImageList imageList;
	bool bCreated = false;

public:
	CWxImageList() = default;
	CWxImageList( const CWxImageList& ) = delete;
	CWxImageList& operator=( const CWxImageList& ) = delete;

	// Masked, as ImageList_Create was asked for: every icon in these lists is
	// added with a mask colour. nGrow is wx's allocation hint, as it was Win32's.
	bool Create( int nSizeX, int nSizeY, int nGrow )
	{
		bCreated = imageList.Create( nSizeX, nSizeY, true, nGrow );
		return bCreated;
	}

	// The collector empties the lists rather than making them again, because
	// the palettes' list controls are pointed at these objects.
	bool IsCreated() const { return bCreated; }

	// -1 on failure, as CImageList::Add and ImageList_AddMasked answered.
	int Add( const wxBitmap &rBitmap, const wxColour &rMask )
	{
		return ( bCreated && rBitmap.IsOk() ) ? imageList.Add( rBitmap, rMask ) : -1;
	}

	// DDS thumbnails already have alpha; a colour mask would discard it.
	int Add( const wxBitmap &bitmap )
	{
		return ( bCreated && bitmap.IsOk() ) ? imageList.Add( bitmap ) : -1;
	}

	void RemoveAll()
	{
		if ( bCreated )
		{
			imageList.RemoveAll();
		}
	}

	wxImageList* GetList() { return &imageList; }
	virtual void* GetNativeImageList() { return &imageList; }
};


// The list behind a widget's image list token, or null for none.
inline wxImageList* ToWxImageList( IImageList *pImageList )
{
	if ( pImageList == 0 )
	{
		return nullptr;
	}
	return static_cast<wxImageList*>( pImageList->GetNativeImageList() );
}
