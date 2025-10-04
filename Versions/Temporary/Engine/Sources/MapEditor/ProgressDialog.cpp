#include "stdafx.h"

#include "ProgressDialog.h"

const DWORD CProgressDialog::START_TIMER_ID = 10;
const DWORD CProgressDialog::START_TIMER_INTERVAL = 500;


CProgressDialog::CProgressDialog( CWnd* pParent )
	: CDialog( CProgressDialog::IDD, pParent )
{
}


void CProgressDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_LABEL, m_ProgressLabel);
	DDX_Control(pDX, IDC_PROGRESS_BAR, m_ProgressBar);
}


BEGIN_MESSAGE_MAP(CProgressDialog, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()


BOOL CProgressDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetStartTimer();
	return false;
}


void CProgressDialog::UpdateControls()
{
	UpdateWindow();
	if ( GetParent() )
	{
		GetParent()->UpdateWindow();
	}
}


void CProgressDialog::SetProgressTitle( const string &rszProgressTitle )
{
	SetWindowText( rszProgressTitle.c_str() );
	UpdateControls();
}


void CProgressDialog::SetProgressMessage( const string &rszProgressMessage )
{
	m_ProgressLabel.SetWindowText( rszProgressMessage.c_str() );
	UpdateControls();
}


void CProgressDialog::SetProgressRange( int nStart, int nFinish )
{
	m_ProgressBar.SetRange32( nStart, nFinish );
	UpdateControls();
}


void CProgressDialog::SetProgressPosition( int nPosition )
{
	m_ProgressBar.SetPos( nPosition );
	UpdateControls();
}


void CProgressDialog::IterateProgressPosition()
{
	int nLower;
	int nUpper;
	m_ProgressBar.GetRange( nLower, nUpper );
	int nNewPos = m_ProgressBar.GetPos() + 1;
	if ( nNewPos > nUpper )
	{
		nNewPos = nLower;
	}
	m_ProgressBar.SetPos( nNewPos );
	UpdateControls();
}


void CProgressDialog::OnTimer( UINT nIDEvent ) 
{
  if ( nIDEvent == START_TIMER_ID )
	{
		OnStartTimer();
	}
	else
	{
		CDialog::OnTimer(nIDEvent);
	}
}


void CProgressDialog::SetStartTimer()
{
  KillStartTimer();
  dwStartTimer = SetTimer( START_TIMER_ID, START_TIMER_INTERVAL, 0 );
}


void CProgressDialog::KillStartTimer()
{
  if ( dwStartTimer != 0 )
	{
		KillTimer( dwStartTimer );
		dwStartTimer = 0;
	}
}


void CProgressDialog::OnStartTimer()
{
  KillStartTimer();
	ShowWindow( SW_SHOW );
	UpdateControls();
}


