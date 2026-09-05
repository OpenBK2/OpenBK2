#include "stdafx.h"
#include "resource.h"
#include "ProgressBarWindow.h"
#include "MapEditorLib/ShellFont.h"

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
	// Not a CResizeDialog, so it does not get the shell font from there.
	NEditorFont::ApplyShellFont( this );

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


void CProgressBarWindow::Start( int nRange, const std::string & szCaption )
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


void CProgressBarWindow::SetCaption( const std::string & szCaption )
{
	wndCaption.SetWindowText( szCaption.c_str() );
}



