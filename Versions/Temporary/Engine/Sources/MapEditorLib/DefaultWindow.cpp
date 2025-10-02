#include "stdafx.h"
#include "DefaultWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CDefaultWindow, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()



CDefaultWindow::CDefaultWindow() {}


CDefaultWindow::~CDefaultWindow() {}


void CDefaultWindow::OnPaint() 
{
	CPaintDC dc( this );
	//
	CRect clientRect;
	GetClientRect( &clientRect );
	dc.FillSolidRect( clientRect, ::GetSysColor( COLOR_3DFACE ) );
}


BOOL CDefaultWindow::PreCreateWindow( CREATESTRUCT &rCreateStruct ) 
{
	if ( !CWnd::PreCreateWindow( rCreateStruct ) )
	{
		return FALSE;
	}

	//rCreateStruct.dwExStyle |= WS_EX_CLIENTEDGE;
	rCreateStruct.style &= ~WS_BORDER;
	rCreateStruct.lpszClass = AfxRegisterWndClass( CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
																								 ::LoadCursor( NULL, IDC_ARROW ),
																								  HBRUSH( COLOR_WINDOW + 1 ),
																									NULL );
	return TRUE;
}

// basement storage  

