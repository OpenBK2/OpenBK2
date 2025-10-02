#if !defined(__TREE__GDB_BROWSER__SEARCH_OBJECT_DIALOG__)
#define __TREE__GDB_BROWSER__SEARCH_OBJECT_DIALOG__
#pragma once

#include "ResourceDefines.h"



class CSearchObjectDialog : public CDialog
{
	CString	strText;

protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();

public:
	enum { IDD = IDD_SEARCH_OBJECT };
	//
	CSearchObjectDialog( CWnd* pParent = NULL );
	//
	void SetText( const string &rszText );
	string GetText();
	//
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__TREE__GDB_BROWSER__SEARCH_OBJECT_DIALOG__)
