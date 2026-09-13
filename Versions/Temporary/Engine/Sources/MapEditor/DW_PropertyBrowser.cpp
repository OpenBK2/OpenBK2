#include "stdafx.h"
#include "DW_PropertyBrowser.h"

#include "MapEditorLib/MfcWidget.h"

CDWPropertyBrowser::CDWPropertyBrowser()
	: pPane( 0 ),
		bEnableEdit( true )
{
}


CDWPropertyBrowser::~CDWPropertyBrowser()
{
	delete pPane;
	pPane = 0;
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

	pPane = NPropertyPane::Create();
	CWndWidget paneWidget( this );
	if ( pPane == 0 || !pPane->Create( &paneWidget, szOptionsLabel ) )
	{
		return -1;
	}
	pPane->Show( true );
	return 0;
}


void CDWPropertyBrowser::OnSize( unsigned nType, int cx, int cy )
{
	SECControlBar::OnSize( nType, cx, cy );

	if ( pPane != 0 && pPane->IsCreated() )
	{
		CRect insideRect;
		GetInsideRect( insideRect );
		pPane->SetBounds( CTRect<int>( insideRect.left, insideRect.top, insideRect.right, insideRect.bottom ) );
	}
}


void CDWPropertyBrowser::EnableEdit( bool bEnable )
{
	bEnableEdit = bEnable;
	if ( pPane != 0 && pPane->IsCreated() )
	{
		pPane->EnableEdit( bEnable );
	}
}

// basement storage
