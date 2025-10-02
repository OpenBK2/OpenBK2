#include "stdafx.h"
#include "resource.h"
#include "ProgressBarWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CProgressBarWindow::CProgressBarWindow()
	: CDialog( IDD_PROGRESS_BAR, 0 )
{
}


void CProgressBarWindow::DoDataExchange( CDataExchange* pDX )
{
	CDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_CAPTION, wndCaption );
}


BEGIN_MESSAGE_MAP(CProgressBarWindow, CDialog)
	//{{AFX_MSG_MAP(CProgressBarWindow)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CProgressBarWindow::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	wndProgress.AttachProgress( IDC_PROGRESS, this );
	wndProgress.SetStep( 1 );

	return TRUE;
}


bool CProgressBarWindow::Create( CWnd *pParent )
{
	// create window
	HINSTANCE t = AfxGetResourceHandle();
	AfxSetResourceHandle( theEDCommonInstance );
	bool bResult = CDialog::Create( IDD_PROGRESS_BAR, pParent );
	AfxSetResourceHandle( t );
	return bResult;
}


void CProgressBarWindow::Start( int nRange, const string & szCaption )
{
	wndProgress.ResetProgress();
	wndProgress.SetRange( 0, nRange );
	if ( !szCaption.empty() )
		SetCaption( szCaption );
	else
		SetCaption( "Working..." ); //{CRAP: in-code text }CRAP
	ShowWindow( SW_SHOW );
}


void CProgressBarWindow::StepIt()
{
	wndProgress.StepIt();
}


void CProgressBarWindow::Finish()
{
	ShowWindow( SW_HIDE );
}


void CProgressBarWindow::SetCaption( const string & szCaption )
{
	wndCaption.SetWindowText( szCaption.c_str() );
}



