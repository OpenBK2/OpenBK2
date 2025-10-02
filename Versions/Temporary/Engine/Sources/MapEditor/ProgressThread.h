#pragma once

#include "ProgressDlg.h"

//class CProgressDlg;
class CProgressThread : public CWinThread
{
	//DECLARE_DYNCREATE( CProgressThread )

	CProgressDlg *pDialog;
	//HANDLE m_hEventKill;
	//HANDLE m_hEventDead;
	//static CRITICAL_SECTION m_csProgressLock;
	CWnd *pParent;

protected:
	//CProgressThread();           // protected constructor used by dynamic creation

public:
	DECLARE_DYNAMIC( CProgressThread )

	CProgressThread( const string &rszName, CWnd *pParentWindow );
	virtual ~CProgressThread();

	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//virtual int Run();

	// custom
	void KillThread();
	void SingleStep();
	//
	bool WasCancelled() const { return false; }
	//
	CProgressDlg* GetProgressDialog() const { return 0; }
	void SetProgressRange( int nStart, int nFinish );
	void SetProgressPosition( int nPosition );
	void IterateProgressPosition();
	//
	void ClearLog();
	void AddLog( const string &rszLogMessage );

protected:
	DECLARE_MESSAGE_MAP()
};
