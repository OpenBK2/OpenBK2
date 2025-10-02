#include "stdafx.h"
#include "DW_PropertyBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CDWPropertyBrowser::CDWPropertyBrowser()
{
}


CDWPropertyBrowser::~CDWPropertyBrowser()
{
}


BEGIN_MESSAGE_MAP(CDWPropertyBrowser, SECControlBar)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


int CDWPropertyBrowser::OnCreate( LPCREATESTRUCT pCreateStruct ) 
{
	if ( SECControlBar::OnCreate( pCreateStruct ) == -1 )
	{
		return -1;
	}

	if ( !wndContents.Create( CPCDialog::IDD, this ) )
	{
		return -1;
	}
	wndContents.ShowWindow( SW_SHOW );
	return 0;
}


void CDWPropertyBrowser::OnSize( UINT nType, int cx, int cy ) 
{
	SECControlBar::OnSize( nType, cx, cy );
	
	if ( wndContents.GetSafeHwnd() != NULL )
	{
		CRect insideRect;
		GetInsideRect( insideRect );

		wndContents.SetWindowPos( 0,
															insideRect.left,
															insideRect.top,
															insideRect.Width(),
															insideRect.Height(),
															SWP_NOZORDER | SWP_NOACTIVATE );
	}
}

// basement storage  


