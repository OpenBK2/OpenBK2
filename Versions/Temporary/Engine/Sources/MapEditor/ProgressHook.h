#pragma once

#include "../MapEditorLib/Interface_ProgressHook.h"
#include "ProgressThread.h"


class CProgressHook : public IProgressHook
{
	OBJECT_NOCOPY_METHODS( CProgressHook );
	
	CProgressThread *pProgressThread;
	bool bHasCancelButton;

public:
	CProgressHook();
	virtual ~CProgressHook();

	virtual void Create( const string &rszActionName, CWnd *pWnd );
	virtual CProgressDlg* GetProgressDialog() const;
	//
	virtual void SetCancel( bool bHasCancel ) { bHasCancelButton = bHasCancel; }
	virtual bool HasCancel() const { return bHasCancelButton; }
	virtual bool WasCancelled() const { return ( HasCancel() && pProgressThread && pProgressThread->WasCancelled() ); }
	//
	virtual void SetProgressRange( int nStart, int nFinish );
	virtual void SetProgressPosition( int nPosition );
	virtual void IterateProgressPosition();
	//
	virtual void ClearLog();
	virtual void AddLog( const string &rszLogMessage );
};


