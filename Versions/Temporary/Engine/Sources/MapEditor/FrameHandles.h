#pragma once

#include "MapEditorLib/Interface_Widget.h"

// The MFC front-end's IDockPanel, IToolBar and IFrameWindow.
//
// Front-end private, like MfcWidget.h and MfcPaintContext.h. IMainFrame used to
// hand SECControlBar, SECCustomToolBar and SECWorksheet pointers straight out to
// the domain editors, which then called MFC methods on them. These wrap the same
// windows behind the small method sets the editors actually use.
//
// They borrow: the windows belong to the frame and to the toolbar manager, and a
// handle staying readable after its window is destroyed is exactly why IsAlive
// exists. CMainFrame keeps them alive for its own lifetime rather than handing
// out ownership, because a domain editor holds one across the destruction of the
// window behind it and asks IsAlive afterwards.


class CDockPanelHandle : public IDockPanel
{
	CFrameWnd *pFrame;
	SECControlBar *pBar;

public:
	CDockPanelHandle( CFrameWnd *_pFrame, SECControlBar *_pBar )
		: pFrame( _pFrame ), pBar( _pBar ) {}

	SECControlBar* GetBar() const { return pBar; }

	// A panel is a widget: the editors create their contents as children of
	// it before handing them back through SetControlBarWindowContents.
	virtual void* GetNativeWidget() { return static_cast<CWnd*>( pBar ); }

	// Show and ShowWithoutLayout are both here because the editors did both, and
	// they are not the same thing: ShowControlBar tells the frame, which then
	// recalculates the docking layout, while ShowWindow just shows the window.
	// The toggle commands used the first and the creation paths used the second,
	// and this port has spent enough on the docking layout not to quietly merge
	// them.
	virtual void Show( bool bShow )
	{
		if ( pFrame && pBar )
		{
			pFrame->ShowControlBar( pBar, bShow ? TRUE : FALSE, TRUE );
		}
	}
	virtual void ShowWithoutLayout( bool bShow )
	{
		if ( pBar )
		{
			pBar->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
		}
	}
	virtual bool IsVisible() const
	{
		return ( pBar != 0 ) && ( pBar->IsVisible() != FALSE );
	}
	virtual bool IsAlive() const
	{
		return ( pBar != 0 ) && ( ::IsWindow( pBar->m_hWnd ) != FALSE );
	}
	virtual void Destroy()
	{
		if ( pBar )
		{
			pBar->DestroyWindow();
		}
	}
	virtual void Redraw()
	{
		if ( pBar )
		{
			pBar->Invalidate();
		}
	}
};


class CToolBarHandle : public IToolBar
{
	CFrameWnd *pFrame;
	SECCustomToolBar *pToolBar;

public:
	CToolBarHandle( CFrameWnd *_pFrame, SECCustomToolBar *_pToolBar )
		: pFrame( _pFrame ), pToolBar( _pToolBar ) {}

	virtual void Show( bool bShow )
	{
		if ( pFrame && pToolBar )
		{
			pFrame->ShowControlBar( pToolBar, bShow ? TRUE : FALSE, TRUE );
		}
	}
	virtual bool IsVisible() const
	{
		return ( pToolBar != 0 ) && ( pToolBar->IsVisible() != FALSE );
	}
};


class CFrameWindowHandle : public IFrameWindow
{
	SECWorksheet *pWorksheet;

public:
	explicit CFrameWindowHandle( SECWorksheet *_pWorksheet ) : pWorksheet( _pWorksheet ) {}

	SECWorksheet* GetWorksheet() const { return pWorksheet; }

	virtual void* GetNativeWidget() { return static_cast<CWnd*>( pWorksheet ); }

	virtual void Show( bool bShow )
	{
		if ( pWorksheet )
		{
			pWorksheet->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
		}
	}
	// Maximize also drops WS_SYSMENU, which is what CChildFrameBase did around
	// its MDIMaximize: a maximised MDI child otherwise puts its own close box in
	// the menu bar.
	virtual void Maximize()
	{
		if ( pWorksheet )
		{
			pWorksheet->ModifyStyle( WS_SYSMENU, 0 );
			pWorksheet->MDIMaximize();
		}
	}
	virtual void Focus()
	{
		if ( pWorksheet )
		{
			pWorksheet->SetFocus();
		}
	}
	virtual void Destroy()
	{
		if ( pWorksheet )
		{
			pWorksheet->MDIDestroy();
		}
	}
};
