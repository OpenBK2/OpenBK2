
#pragma once
#include "resource.h"

class CAboutDialog : public CDialog
{
public:
	CAboutDialog( CWnd* pParent = NULL );

	enum { IDD = IDD_ABOUT };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
};



