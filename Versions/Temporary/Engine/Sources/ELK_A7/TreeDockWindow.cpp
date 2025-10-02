#include "StdAfx.h"
#include "resource.h"
#include "TreeDockWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CTreeDockWindow::CTreeDockWindow()
	: pwndMainFrame( 0 )
{
}


CTreeDockWindow::~CTreeDockWindow()
{
}

void CTreeDockWindow::SetMainFrameWindow( CWnd *_pwndMainFrame )
{
	pwndMainFrame = _pwndMainFrame;
}


BEGIN_MESSAGE_MAP(CTreeDockWindow, SECControlBar)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



int CTreeDockWindow::OnCreate( LPCREATESTRUCT lpCreateStruct ) 
{
	if ( SECControlBar::OnCreate( lpCreateStruct ) == -1 )
	{
		return -1;
	}

	//InitImageLists();

	// create a tree control
	DWORD dwStyle = TVS_SHOWSELALWAYS |
									TVS_HASBUTTONS |
									TVS_LINESATROOT |
									TVS_HASLINES |
									TVS_SHOWSELALWAYS |
									TVS_DISABLEDRAGDROP |
									WS_CHILD | WS_VISIBLE;

	DWORD dwStyleEx = TVXS_FLYBYTOOLTIPS |
										LVXS_HILIGHTSUBITEMS;

	BOOL bCreated = wndTree.Create( dwStyle, dwStyleEx, CRect( 0, 0, 0, 0 ), this, IDC_EMBEDDED_CONTROL );

	NI_ASSERT( bCreated, StrFmt( _T( "CTreeDockWindow::OnCreate, cant't creat Tree" ) ) );

	wndTree.ModifyStyleEx( 0, WS_EX_CLIENTEDGE, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	
	// set the normal image list. This would cover overlay and selected/expanded images too.
	// you will need a separate image list for state images, if you use them.
	//wndTree.SetImageList( &imlNormal, TVSIL_NORMAL );

	//add a few items, just for example
	//LoadTree( &wndTree, 0 );
	return 0;
}


void CTreeDockWindow::OnSize( UINT nType, int cx, int cy ) 
{
	SECControlBar::OnSize( nType, cx, cy );
	
	if( wndTree.GetSafeHwnd() != 0 )
	{
		CRect insideRect;
		GetInsideRect( insideRect );
		wndTree.SetWindowPos( 0, insideRect.left, insideRect.top, insideRect.Width(), insideRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE );
	}
}


