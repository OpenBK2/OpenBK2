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
	void SetText( const std::string &rszText );
	std::string GetText();
	//
	DECLARE_MESSAGE_MAP()
};


