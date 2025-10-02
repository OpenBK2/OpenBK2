#include "stdafx.h"
#include "ProgressThread.h"
#include "../MapEditorLib/Interface_ProgressHook.h"
#include "../MapEditorLib/Interface_MainFrame.h"


IMPLEMENT_DYNAMIC( CProgressThread, CWinThread )

BEGIN_MESSAGE_MAP( CProgressThread, CWinThread )
END_MESSAGE_MAP()

//CRITICAL_SECTION CProgressThread::m_csProgressLock;

//CProgressThread::CProgressThread()
//	:pDialog( 0 )
//{
//	DebugTrace( "CProgressThread:: constructor1" );
//	//
//	//m_bAutoDelete = FALSE;
//
//	// kill event starts out in the signaled state
//	//m_hEventKill = CreateEvent( NULL, TRUE, FALSE, NULL );
//	//m_hEventDead = CreateEvent( NULL, TRUE, FALSE, NULL );
//}


CProgressThread::CProgressThread( const string &rszName, CWnd *pParentWindow )
{
	pDialog = new CProgressDlg( rszName );
	pDialog->Create( pParentWindow );
	DebugTrace( "CProgressThread:: constructor2" );
	//
	//m_bAutoDelete = FALSE;

	// kill event starts out in the signaled state
	//m_hEventKill = CreateEvent( NULL, TRUE, FALSE, NULL );
	//m_hEventDead = CreateEvent( NULL, TRUE, FALSE, NULL );
}


CProgressThread::~CProgressThread()
{
	DebugTrace( "CProgressThread:: destructor" );
	//
	if ( pDialog )
		delete pDialog;

	pDialog = 0;
	//
	//CloseHandle( m_hEventKill );
	//CloseHandle( m_hEventDead );
}


BOOL CProgressThread::InitInstance()
{
	DebugTrace( "CProgressThread:: InitInstance" );
	//
	NI_VERIFY( pDialog, "Progress was not created", return FALSE )
	m_pMainWnd = pDialog;

	return TRUE;
}


int CProgressThread::ExitInstance()
{
	DebugTrace( "CProgressThread:: ExitInstance" );
	//
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}


void CProgressThread::KillThread()
{
	DebugTrace( "CProgressThread:: KillThread" );
	//
	// Note: this function is called in the context of other threads,
	//  not the thread itself.

	// reset the m_hEventKill which signals the thread to shutdown
	//VERIFY( SetEvent(m_hEventKill) );

	// allow thread to run at higher priority during kill process
	//SetThreadPriority( THREAD_PRIORITY_ABOVE_NORMAL );
	//WaitForSingleObject( m_hEventDead, INFINITE );
	//WaitForSingleObject( m_hThread, INFINITE );

	// now delete CWinThread object since no longer necessary
	delete this;
}


void CProgressThread::SingleStep()
{
	//DebugTrace( "CProgressThread:: SingleStep" );
	//
	//EnterCriticalSection( &m_csProgressLock );
	//{
	//	// main thread processing
	//}
	//LeaveCriticalSection( &m_csProgressLock );
}


void CProgressThread::SetProgressRange( int nStart, int nFinish )
{
	DebugTrace( "CProgressThread:: SetProgressRange" );
	//
	if ( pDialog )
	{
		pDialog->SetProgressRange( nStart, nFinish );
	}
}


void CProgressThread::SetProgressPosition( int nPosition )
{
	DebugTrace( "CProgressThread:: SetProgressPosition" );
	//
	if ( pDialog )
	{
		pDialog->SetProgressPosition( nPosition );
	}
}


void CProgressThread::IterateProgressPosition()
{
	DebugTrace( "CProgressThread:: IterateProgressPosition" );
	//
	if ( pDialog )
	{
		pDialog->IterateProgressPosition();
	}
}


void CProgressThread::ClearLog()
{
	DebugTrace( "CProgressThread:: ClearLog" );
	//
	if ( pDialog )
	{
		pDialog->ClearLog();
	}
}


void CProgressThread::AddLog( const string &rszLogMessage )
{
	DebugTrace( "CProgressThread:: AddLog" );
	//
	if ( pDialog )
	{
		pDialog->AddLog( rszLogMessage );
	}
}



