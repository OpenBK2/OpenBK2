#pragma once

#include "Interface_Widget.h"

// The MFC front-end's side of the neutral handles in Interface_Widget.h.
//
// This header is front-end private: it names MFC, so nothing in the interface
// layer and nothing in a domain editor should include it. It exists so that the
// conversion between an IWidget and the CWnd behind it happens in one place,
// which is where a wrong one can be caught, rather than as a static_cast spread
// over the call sites.
//
// A second front-end gets its own equivalent of this file and never sees this
// one. That is the whole point of the split: the interfaces carry the handle,
// each front-end knows only its own.


// Mix into a CWnd-derived class so it can be handed through the interface layer:
//
//     class CMyWindow : public CWnd, public IWidget
//     {
//         DECLARE_CWND_WIDGET();
//         ...
//     };
//
// The static_cast is what makes this safe: it is a cast to a base of the class
// the macro is expanded in, so the compiler adjusts the pointer for the multiple
// inheritance rather than the far end guessing at the layout.
#define DECLARE_CWND_WIDGET()	virtual void* GetNativeWidget() { return static_cast<CWnd*>( this ); }


// Wraps a bare CWnd* that is not itself an IWidget, for the call sites that have
// one from MFC rather than from an editor class -- AfxGetMainWnd(), a dialog's
// GetParent(), and so on. Cheap enough to be a temporary at the call site, which
// is how it is meant to be used: it holds a borrowed pointer and owns nothing.
class CWndWidget : public IWidget
{
	CWnd *pWnd;

public:
	explicit CWndWidget( CWnd *_pWnd ) : pWnd( _pWnd ) {}
	virtual void* GetNativeWidget() { return pWnd; }
};


// Recover the CWnd behind a widget handle. Null in, null out, because most of
// the call sites pass an optional parent through unchecked.
inline CWnd* ToCWnd( IWidget *pWidget )
{
	if ( pWidget == 0 )
	{
		return 0;
	}
	return static_cast<CWnd*>( pWidget->GetNativeWidget() );
}


// The same for the image handles. CBitmap and CImageList are MFC objects the
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
