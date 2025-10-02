#include "stdafx.h"

#include "DefaultDockingWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CDefaultDockingWindow, SECControlBar)
	ON_WM_SIZE()
END_MESSAGE_MAP()


void CDefaultDockingWindow::OnSize( UINT nType, int cx, int cy ) 
{
	SECControlBar::OnSize( nType, cx, cy );

	if ( pwndContents )
	{
		if ( pwndContents->GetSafeHwnd() != NULL )
		{
			CRect insideRect;
			GetInsideRect( insideRect );
			//
			pwndContents->SetWindowPos( 0,
																	insideRect.left,
																	insideRect.top,
																	insideRect.Width(),
																	insideRect.Height(),
																	SWP_NOZORDER | SWP_NOACTIVATE );
		}
	}
}

// basement storage  


