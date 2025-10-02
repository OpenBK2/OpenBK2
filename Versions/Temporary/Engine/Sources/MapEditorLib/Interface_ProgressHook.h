#pragma once

#include "Interface_Controller.h"

class CProgressDlg;
interface IProgressHook : public CObjectBase
{
	enum { tidTypeID = 0x1B24AB80 };

	//string szProgressName;

	virtual void Create( const string &rszName, CWnd *pWnd ) = 0;
	//virtual void GetName( string *pName ) const { (*pName) = szProgressName; }
	virtual CProgressDlg* GetProgressDialog() const = 0;
	//
	virtual void SetCancel( bool bHasCancel ) = 0;
	virtual bool HasCancel() const = 0;
	virtual bool WasCancelled() const = 0;
	//
	virtual void SetProgressRange( int nStart, int nFinish ) = 0;
	virtual void SetProgressPosition( int nPosition ) = 0;
	virtual void IterateProgressPosition() = 0;
	//
	virtual void ClearLog() = 0;
	virtual void AddLog( const string &rzsLogMessage ) = 0;
};



