#pragma once

#include "ResourceDefines.h"


class CAboutDialog : public CDialog
{
	enum { IDD = IDD_ABOUT };

protected:
	virtual BOOL OnInitDialog();
	
public:
	CAboutDialog( CWnd* pParent = NULL );

	DECLARE_MESSAGE_MAP()
};



