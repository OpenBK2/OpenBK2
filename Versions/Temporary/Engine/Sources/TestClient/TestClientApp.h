#pragma once

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

class CTestClientApp : public CWinApp
{
	class CConsole *pConsole;
	CPtr<class CCommands> pCmds;

	CObj<class CTestClient> pTestClient;
public:
	CTestClientApp();

	virtual BOOL InitInstance();
	virtual int ExitInstance();

	void Segment();
};


