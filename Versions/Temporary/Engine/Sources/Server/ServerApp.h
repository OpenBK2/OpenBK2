#pragma once

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

class CServerApp : public CWinApp
{
	class CConsole *pConsole;
	CObj<class CCommands> pCmds;
	CObj<class CGameServer> pGameServer;
public:
	CServerApp();

	virtual BOOL InitInstance();
	virtual int ExitInstance();

	void Segment();
};


