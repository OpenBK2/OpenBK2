#if !defined(__INTERACTIVE_PROCESS__)
#define __INTERACTIVE_PROCESS__
#pragma once


class CInteractiveProcess
{
	HANDLE hParentIn;
	HANDLE hParentOut;
	HANDLE hParentErrIn;
	PROCESS_INFORMATION procInfo;
	int RESPONSEWAIT_TIMEOUT;

	void CleanupHandles();
	void InternalStop();

public:
	CInteractiveProcess( int nResponseWaitTimeout = (60 * 1000) )
		: hParentIn(0), hParentOut(0), hParentErrIn(0), RESPONSEWAIT_TIMEOUT(nResponseWaitTimeout)
	{
		memset( &procInfo, 0, sizeof(PROCESS_INFORMATION) );
	}

	bool IsStarted()
	{
		return procInfo.hProcess;
	}

	void SetResponseTimeout( int nResponseWaitTimeout )
	{
		RESPONSEWAIT_TIMEOUT = nResponseWaitTimeout;
	}

	bool Start( const string &szCommandLine, string *pszErrorMessage );
	bool Execute( const string &szScript, const string &szResponseEndLabel, string *pszOutput, string *pszErrorOutput, string *pszErrorMessage );
	bool Stop( const string &szQuitScript );
};

#endif //#define __INTERACTIVE_PROCESS__

