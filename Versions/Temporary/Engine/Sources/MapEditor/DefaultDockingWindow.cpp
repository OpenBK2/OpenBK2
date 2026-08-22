#include "stdafx.h"

#include "DefaultDockingWindow.h"

BEGIN_MESSAGE_MAP(CDefaultDockingWindow, SECControlBar)
	ON_WM_SIZE()
END_MESSAGE_MAP()


void CDefaultDockingWindow::OnSize( unsigned nType, int cx, int cy ) 
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



