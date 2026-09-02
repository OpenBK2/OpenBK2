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

	bool Start( const std::string &szCommandLine, std::string *pszErrorMessage );
	bool Execute( const std::string &szScript, const std::string &szResponseEndLabel, std::string *pszOutput, std::string *pszErrorOutput, std::string *pszErrorMessage );
	bool Stop( const std::string &szQuitScript );
};


