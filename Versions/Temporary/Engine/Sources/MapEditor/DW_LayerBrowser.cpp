#include "stdafx.h"

#include "DW_LayerBrowser.h"

CDWLayerBrowser::CDWLayerBrowser()
{
}


CDWLayerBrowser::~CDWLayerBrowser()
{
}


BEGIN_MESSAGE_MAP(CDWLayerBrowser, SECControlBar)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


int CDWLayerBrowser::OnCreate( LPCREATESTRUCT pCreateStruct ) 
{
	if ( SECControlBar::OnCreate( pCreateStruct ) == -1 )
	{
		return -1;
	}
	/**
	if ( !wndContents.Create( CPCDialog::IDD, this ) )
	{
		return -1;
	}
	wndContents.ShowWindow( SW_SHOW );
	/**/
	return 0;
}


void CDWLayerBrowser::OnSize( unsigned nType, int cx, int cy ) 
{
	SECControlBar::OnSize( nType, cx, cy );
	
	/**
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
	/**/
}

// basement storage  



