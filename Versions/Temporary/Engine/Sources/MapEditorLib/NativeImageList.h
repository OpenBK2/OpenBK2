#pragma once

#include "Interface_Widget.h"

#include <commctrl.h>

// The native side of IImageList: a Win32 image list.
//
// This was MfcWidget.h, whose CMfcImageList borrowed a CImageList. The object
// collector's icon caches are the only image lists behind IImageList, and they
// are Win32 image lists now, owned by this class; the palettes hand the handle
// straight to their list controls (ListView_SetImageList), which wx creates
// with LVS_SHAREIMAGELISTS, so nothing else takes ownership.
class CNativeImageList : public IImageList
{
	HIMAGELIST hImageList = 0;

public:
	CNativeImageList() = default;
	CNativeImageList( const CNativeImageList& ) = delete;
	CNativeImageList& operator=( const CNativeImageList& ) = delete;
	~CNativeImageList() { Destroy(); }

	bool Create( int nSizeX, int nSizeY, unsigned nFlags, int nGrow )
	{
		Destroy();
		hImageList = ::ImageList_Create( nSizeX, nSizeY, nFlags, 0, nGrow );
		return ( hImageList != 0 );
	}

	void Destroy()
	{
		if ( hImageList != 0 )
		{
			::ImageList_Destroy( hImageList );
			hImageList = 0;
		}
	}

	// ImageList_AddMasked copies the bitmap; the caller keeps its own. -1 on
	// failure, as CImageList::Add answered.
	int Add( HBITMAP hBitmap, COLORREF maskColor )
	{
		return ( hImageList != 0 ) ? ::ImageList_AddMasked( hImageList, hBitmap, maskColor ) : -1;
	}

	void RemoveAll()
	{
		if ( hImageList != 0 )
		{
			::ImageList_RemoveAll( hImageList );
		}
	}

	HIMAGELIST GetHandle() const { return hImageList; }
	virtual void* GetNativeImageList() { return hImageList; }
};


inline HIMAGELIST ToHImageList( IImageList *pImageList )
{
	if ( pImageList == 0 )
	{
		return 0;
	}
	return static_cast<HIMAGELIST>( pImageList->GetNativeImageList() );
}
