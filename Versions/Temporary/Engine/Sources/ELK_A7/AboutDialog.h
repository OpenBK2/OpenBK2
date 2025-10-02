#if !defined(__ELK_ABOUT_DIALOG__)
#define __ELK_ABOUT_DIALOG__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

#endif // !defined(__ELK_ABOUT_DIALOG__)

