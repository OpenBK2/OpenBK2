#include "stdafx.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "ResourceDefines.h"
#include "WMDefines.h"

#include "DW_GDBBrowser.h"
#include "MapEditorLib/Interface_MainFrame.h"

#include <cstdint>

CDWGDBBrowser::CDWGDBBrowser( int _nGDBBrowserID )
	: contents( this, _nGDBBrowserID ), ownerWidget( this )
{
}


CDWGDBBrowser::~CDWGDBBrowser()
{
}


BEGIN_MESSAGE_MAP(CDWGDBBrowser, SECControlBar)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	//ON_MESSAGE( TCM_TABSEL, OnTabSelected )
	ON_MESSAGE( WM_GDB_BROWSER, OnTabSelected )
	ON_CBN_SELCHANGE( IDC_TREE_GDB_BROWSER, OnTabSelected )
END_MESSAGE_MAP()


int CDWGDBBrowser::OnCreate( LPCREATESTRUCT pCreateStruct )
{
	if ( SECControlBar::OnCreate( pCreateStruct ) == -1 )
	{
		return -1;
	}
	//
	// The MFC browser tells this pane about a table change with WM_GDB_BROWSER
	// and CBN_SELCHANGE, handled below; a wx one tells the contents.
	IObjectBrowser *const pBrowser = NObjectBrowser::Create();
	CWndWidget paneWidget( this );
	if ( ( pBrowser == 0 ) ||
			 !pBrowser->Create( &paneWidget, &contents, IObjectBrowser::KIND_BROWSER, contents.GetID(), IDC_TREE_GDB_BROWSER ) )
	{
		delete pBrowser;
		return -1;
	}
	//
	if ( !wndEmptyContents.Create( 0,
																 0,
																 AFX_WS_DEFAULT_VIEW,
																 CRect( 0, 0, 0, 0 ),
																 this,
																 IDC_EMPTY_GDB_BROWSER,
																 0 ) )
	{
		delete pBrowser;
		return -1;
	}
	//
	contents.Start( pBrowser );
	return 0;
}


void CDWGDBBrowser::OnDestroy()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAIN, this );
	contents.Stop();
	SECControlBar::OnDestroy();
}


void CDWGDBBrowser::OnSize( unsigned nType, int cx, int cy )
{
	SECControlBar::OnSize( nType, cx, cy );
	LayoutContents();
}


void CDWGDBBrowser::LayoutContents()
{
	if ( GetSafeHwnd() == NULL )
	{
		return;
	}
	CRect insideRect;
	GetInsideRect( insideRect );
	//
	if ( IObjectBrowser *const pBrowser = contents.GetBrowser() )
	{
		pBrowser->SetBounds( CTRect<int>( insideRect.left, insideRect.top, insideRect.right, insideRect.bottom ) );
	}
	if ( wndEmptyContents.GetSafeHwnd() != NULL )
	{
		wndEmptyContents.SetWindowPos( 0,
																	 insideRect.left,
																	 insideRect.top,
																	 insideRect.Width(),
																	 insideRect.Height(),
																	 SWP_NOZORDER | SWP_NOACTIVATE );
	}
}


void CDWGDBBrowser::ShowEmpty( bool bEmpty )
{
	if ( wndEmptyContents.GetSafeHwnd() != NULL )
	{
		wndEmptyContents.ShowWindow( bEmpty ? SW_SHOW : SW_HIDE );
	}
}


BOOL CDWGDBBrowser::OnGripperClose()
{
	return true;
}


void CDWGDBBrowser::OnLButtonDown( unsigned nFlags, CPoint point )
{
	SECControlBar::OnLButtonDown( nFlags, point );
	//
	if ( contents.GetID() != -1 )
	{
		Singleton<IMainFrameContainer>()->Get()->SaveObjectStorage( contents.GetID() );
	}
}


void CDWGDBBrowser::OnRButtonDown( unsigned nFlags, CPoint point )
{
	SECControlBar::OnRButtonDown( nFlags, point );
	//
	if ( contents.GetID() != -1 )
	{
		Singleton<IMainFrameContainer>()->Get()->SaveObjectStorage( contents.GetID() );
	}
}


void CDWGDBBrowser::OnRButtonUp( unsigned nFlags, CPoint point )
{
	SECControlBar::OnRButtonUp( nFlags, point );

	CMenu mainPopupMenu;
	mainPopupMenu.LoadMenu( IDM_MAIN_CONTEXT_MENU );
	CMenu *pMenu = mainPopupMenu.GetSubMenu( MCMN_DW_GDB_BROWSER );
	if ( pMenu )
	{
		ClientToScreen( &point );
		pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, MainFrameWnd(), 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
	mainPopupMenu.DestroyMenu();
}


LRESULT CDWGDBBrowser::OnTabSelected( WPARAM wParam, LPARAM lParam )
{
	contents.OnTableSelected();
	return 0;
}


void CDWGDBBrowser::OnTabSelected()
{
	contents.OnTableSelected();
}

// basement storage
