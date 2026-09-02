#include "stdafx.h"
#include "ProgressHook.h"


CProgressHook::CProgressHook()
	:pProgressThread( 0 ),
	bHasCancelButton( false )
{
	DebugTrace( "CProgressHook:: constructor" );
}


CProgressHook::~CProgressHook()
{
	DebugTrace( "CProgressHook:: destructor" );
	if ( pProgressThread )
	{
		delete pProgressThread;
	}
	pProgressThread = 0;
}


void CProgressHook::Create( const std::string &rszActionName, CWnd *pWnd )
{
	DebugTrace( "CProgressHook:: Creating dialog" );
	//
	//szProgressName = rszActionName;
	//pProgressThread = static_cast<CProgressThread*>( AfxBeginThread(RUNTIME_CLASS(CProgressThread)) );
	pProgressThread = new CProgressThread( rszActionName, pWnd );
	if ( !pProgressThread->CreateThread(CREATE_SUSPENDED) )
	{
		delete pProgressThread;
		return;
	}
	pProgressThread->ResumeThread();
	//
	NI_VERIFY( pProgressThread, "Progress dialog could not be created", return )
}


CProgressDlg* CProgressHook::GetProgressDialog() const
{
	DebugTrace( "CProgressHook:: get dialog" );
	//
	if ( pProgressThread )
		return pProgressThread->GetProgressDialog();
	else
		return 0;
}


void CProgressHook::SetProgressRange( int nStart, int nFinish )
{
	DebugTrace( "CProgressHook:: SetProgressRange" );
	//
	if ( pProgressThread )
	{
		pProgressThread->SetProgressRange( nStart, nFinish );
		//pDialog->RedrawWindow();
	}
}


void CProgressHook::SetProgressPosition( int nPosition )
{
	DebugTrace( "CProgressHook:: SetProgressPosition" );
	//
	if ( pProgressThread )
	{
		pProgressThread->SetProgressPosition( nPosition );
		//pDialog->RedrawWindow();
	}
	if ( nPosition >= 1000 )
	{
		//pProgressThread->KillThread();
	}
}


void CProgressHook::IterateProgressPosition()
{
	DebugTrace( "CProgressHook:: IterateProgressPosition" );
	//
	if ( pProgressThread )
	{
		pProgressThread->IterateProgressPosition();
		//pDialog->RedrawWindow();
	}
}


void CProgressHook::ClearLog()
{
	DebugTrace( "CProgressHook:: ClearLog" );
	//
	if ( pProgressThread )
	{
		pProgressThread->ClearLog();
	}
}


void CProgressHook::AddLog( const std::string &rszLogMessage )
{
	DebugTrace( "CProgressHook:: AddLog" );
	//
	if ( pProgressThread )
	{
		pProgressThread->AddLog( rszLogMessage );
	}
}



