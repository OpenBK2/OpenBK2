#include "stdafx.h"

#include "ProgressDlg.h"

#include <cstdint>

const uint32_t CProgressDlg::START_TIMER_ID = 1;
const uint32_t CProgressDlg::START_TIMER_INTERVAL = 1000;

BEGIN_MESSAGE_MAP( CProgressDlg, CResizeDialog )
	ON_WM_TIMER()
END_MESSAGE_MAP()

CProgressDlg::CProgressDlg( const std::string &rszActionName, CWnd *pParentWindow )
	:CResizeDialog( CProgressDlg::IDD, pParentWindow ),
	szActionName( rszActionName )
{
	SetControlStyle( IDC_PROGRESS_LOG, ANCHORE_LEFT_TOP | RESIZE_HOR | RESIZE_VER );
	SetControlStyle( IDC_PROGRESS_LINE, ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
	//
	SetControlStyle( IDOK, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER, 1.0f / 3.0f, 0.5f, 1.0f, 1.0f );
	SetControlStyle( IDCANCEL, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER, 2.0f / 3.0f, 0.5f, 1.0f, 1.0f );
}


bool CProgressDlg::Create( CWnd *pParentWindow )
{
	return CResizeDialog::Create( IDD, pParentWindow );
}


BOOL CProgressDlg::OnInitDialog()
{
	if ( !CResizeDialog::OnInitDialog() )
		return FALSE;

	SetWindowText( StrFmt("Performing %s", szActionName) );
	//
	return TRUE;
}


void CProgressDlg::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX );

	//DDX_Control( pDX, IDC_PROGRESS_LOG, m_ProgressLabel );
	DDX_Control( pDX, IDC_PROGRESS_LINE, m_ProgressBar );
}


void CProgressDlg::OnTimer( unsigned nIDEvent ) 
{
	if ( nIDEvent == START_TIMER_ID )
	{
		OnStartTimer();
	}
	//else
	//{
	//	CResizeDialog::OnTimer( nIDEvent );
	//}
}


void CProgressDlg::SetStartTimer()
{
	KillStartTimer();
	dwStartTimer = SetTimer( START_TIMER_ID, START_TIMER_INTERVAL, 0 );
}


void CProgressDlg::KillStartTimer()
{
	if ( dwStartTimer != 0 )
	{
		KillTimer( dwStartTimer );
		dwStartTimer = 0;
	}
}


void CProgressDlg::OnStartTimer()
{
	KillStartTimer();
	ShowWindow( SW_SHOW );
}


void CProgressDlg::UpdateDialog()
{
	UpdateWindow();
	if ( GetParent() )
	{
		GetParent()->UpdateWindow();
	}
}


void CProgressDlg::SetProgressRange( int nStart, int nFinish )
{
	//NI_VERIFY( m_ProgressBar.GetSafeHwnd(), "CProgressDlg:: the progress control has not been initialized properly", return )

	m_ProgressBar.SetRange32( nStart, nFinish );
	UpdateDialog();
}


void CProgressDlg::SetProgressPosition( int nPosition )
{
	//NI_VERIFY( m_ProgressBar.GetSafeHwnd(), "CProgressDlg:: the progress control has not been initialized properly", return )

	m_ProgressBar.SetPos( nPosition );
	UpdateDialog();
}


void CProgressDlg::IterateProgressPosition()
{
	NI_VERIFY( m_ProgressBar.GetSafeHwnd(), "CProgressDlg:: the progress control has not been initialized properly", return )

	int nLower;
	int nUpper;
	m_ProgressBar.GetRange( nLower, nUpper );
	int nNewPos = m_ProgressBar.GetPos() + 1;
	if ( nNewPos > nUpper )
	{
		nNewPos = nLower;
	}
	m_ProgressBar.SetPos( nNewPos );
	UpdateDialog();
}


void CProgressDlg::ClearLog()
{
	//
	UpdateDialog();
}


void CProgressDlg::AddLog( const std::string &rszLogMessage )
{
	//
	UpdateDialog();
}



