#pragma once

#include "Interface_Widget.h"

// The MFC side of the neutral image handles in Interface_Widget.h.
//
// This header was the MFC front-end's side of IWidget too: DECLARE_CWND_WIDGET,
// CWndWidget, ToCWnd and MainFrameWnd, the conversions between a widget and the
// CWnd behind it. The editor's windows are wx's now (WxWidget.h, and
// MainWindow.h for the main window's handle), and those went. What is left is
// the image lists the object collector still makes with MFC.


// CBitmap and CImageList are MFC objects the
// object collector already owns and keeps alive for as long as the widgets it
// handed them to; these wrappers borrow, they do not own.
class CBitmapImage : public IImage
{
	CBitmap *pBitmap;

public:
	explicit CBitmapImage( CBitmap *_pBitmap ) : pBitmap( _pBitmap ) {}
	virtual void* GetNativeImage() { return pBitmap; }
};


inline CBitmap* ToCBitmap( IImage *pImage )
{
	if ( pImage == 0 )
	{
		return 0;
	}
	return static_cast<CBitmap*>( pImage->GetNativeImage() );
}


class CMfcImageList : public IImageList
{
	CImageList *pImageList;

public:
	explicit CMfcImageList( CImageList *_pImageList ) : pImageList( _pImageList ) {}
	virtual void* GetNativeImageList() { return pImageList; }
};


inline CImageList* ToCImageList( IImageList *pImageList )
{
	if ( pImageList == 0 )
	{
		return 0;
	}
	return static_cast<CImageList*>( pImageList->GetNativeImageList() );
}
