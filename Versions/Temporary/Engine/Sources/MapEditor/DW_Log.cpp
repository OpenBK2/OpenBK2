#include "stdafx.h"

#include "MapEditorLib/MfcWidget.h"

#include "DW_Log.h"

#include <cstdint>

BEGIN_MESSAGE_MAP(CDWLog, SECControlBar)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


int CDWLog::OnCreate( LPCREATESTRUCT pCreateStruct )
{
	if ( SECControlBar::OnCreate( pCreateStruct ) == -1 )
	{
		return -1;
	}
	//
	// Which toolkit draws the log is NLogView's business, what it keeps the
	// contents'. The contents are what the view nominates as the selection
	// handler when it takes focus.
	ILogView *const pLogView = NLogView::Create();
	contents.SetView( pLogView );
	CWndWidget paneWidget( this );
	if ( pLogView == 0 || !pLogView->Create( &paneWidget, &contents ) )
	{
		return -1;
	}
	pLogView->Show( true );
	return 0;
}


void CDWLog::OnSize( unsigned nType, int cx, int cy )
{
	SECControlBar::OnSize( nType, cx, cy );

	ILogView *const pLogView = contents.GetView();
	if ( pLogView != 0 && pLogView->IsCreated() )
	{
		CRect insideRect;
		GetInsideRect( insideRect );
		pLogView->SetBounds( CTRect<int>( insideRect.left, insideRect.top,
																	insideRect.right, insideRect.bottom ) );
	}
}


// basement storage
