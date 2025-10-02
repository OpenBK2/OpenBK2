
#pragma once
#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif


class CELKApp : public CWinApp
{
public:
	class CMainFrame *pMainFrame;

	CELKApp();

	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

public:
	afx_msg void OnAppAbout();
	afx_msg void OnHelpContents();
	DECLARE_MESSAGE_MAP()
};


