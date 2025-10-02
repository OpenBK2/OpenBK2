#include "StdAfx.h"

#include "InteractiveProcess.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace
{
	const int PIPE_BUFSIZE = 4096;
	const int SLEEP_STEP = 100;

	bool RedirectStdStream( HANDLE *phParentSide, HANDLE *phChildSide, int nStdStream, string *pszErrorMessage )
	{
		// Set the bInheritHandle flag so pipe handles are inherited. 
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
		saAttr.bInheritHandle = TRUE; 
		saAttr.lpSecurityDescriptor = NULL;

		HANDLE hInheritableParentSide = 0;
		*phChildSide = 0;
		*phParentSide = 0;
		pszErrorMessage->clear();

		HANDLE *phInSide;
		HANDLE *phOutSide;
		if ( nStdStream == STD_INPUT_HANDLE )
		{
			phInSide = phChildSide;
			phOutSide = &hInheritableParentSide;
		}
		else
		{
			phInSide = &hInheritableParentSide;
			phOutSide = phChildSide;
		}

		bool bResult = false;

		if ( CreatePipe( phInSide, phOutSide, &saAttr, PIPE_BUFSIZE ) )
		{
			if ( SetStdHandle( nStdStream, *phChildSide ) )
			{
				if ( DuplicateHandle( GetCurrentProcess(), hInheritableParentSide, GetCurrentProcess(), phParentSide,
					0, FALSE, DUPLICATE_SAME_ACCESS) )
				{
					bResult = true;
				}
				else
				{
					*pszErrorMessage = "Can't duplicate handle to make uninheritable one";
				}
			}
			else
			{
				*pszErrorMessage = "Can't redirect stdout";
			}

			CloseHandle( hInheritableParentSide );
			if ( bResult == false )
			{
				CloseHandle( *phChildSide );
			}
		}
		else
		{
			*pszErrorMessage = "Can't create pipe";
		}

		return bResult;
	}
	/**
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline bool WriteStream( HANDLE h, const char *pData, int nBytesToWrite, int *pnBytesWritten )
	{
	int nBytesWrittenTotal = 0;
	int nBytesWritten = 0;
	bool bResult = false;
	do
	{
	bResult = ::WriteFile( h, pData + nBytesWrittenTotal, nBytesToWrite - nBytesWrittenTotal, (LPDWORD)&nBytesWritten, 0 );
	nBytesWrittenTotal += nBytesWritten;
	}
	while( bResult && (nBytesWrittenTotal < nBytesToWrite) );

	*pnBytesWritten = nBytesWrittenTotal;
	return bResult;
	}
	/**/

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void GetErrorMessage( string *pszText, UINT nErrorID )
	{
		NI_ASSERT( pszText, "Supplied string pointer is null" );
		LPVOID lpMsgBuf;
		if ( FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER
				| FORMAT_MESSAGE_FROM_SYSTEM
				| FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				nErrorID,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL ))
		{
			*pszText += (LPCTSTR)lpMsgBuf;
			LocalFree( lpMsgBuf );
		}
		else
		{
			*pszText += StrFmt( "::FormatMessage error %d", GetLastError() );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int StringCountNL( const string &s )
	{
		int n = 0;
		for ( int i = 0; i < s.size(); ++i )
		{
			if ( s[i] == '\n' )
			{
				n += 1;
			}
		}
		return n;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int StringCountSubstring( const char *s, const char *what, const int whatSize )
	{
		int n = 0;
		if ( whatSize )
		{
			const char *p = s;
			while(1)
			{
				p = strstr( p, what );
				if ( p )
				{
					p += whatSize;
					++n;
				}
				else
				{
					break;
				}
			}
		}
		return n;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
} // local namespace


// CInteractiveProcess
//
void CInteractiveProcess::CleanupHandles()
{
	if ( hParentIn )
	{
		CloseHandle( hParentIn );
		hParentIn = 0;
	}
	if ( hParentOut )
	{
		CloseHandle( hParentOut );
		hParentOut = 0;
	}
	if ( hParentErrIn )
	{
		CloseHandle( hParentErrIn );
		hParentErrIn = 0;
	}
}


void CInteractiveProcess::InternalStop()
{
	CleanupHandles();

	::WaitForSingleObject( procInfo.hProcess, INFINITE );

	CloseHandle( procInfo.hThread );
	CloseHandle( procInfo.hProcess );
	memset( &procInfo, 0, sizeof(PROCESS_INFORMATION) );

	::FreeConsole();
}


bool CInteractiveProcess::Start( const string &szCommandLine, string *pszErrorMessage )
{
	if ( IsStarted() )
	{
		return true;
	}

	HANDLE hSavedStdout = GetStdHandle( STD_OUTPUT_HANDLE );
	HANDLE hSavedStdin = GetStdHandle( STD_INPUT_HANDLE );
	HANDLE hSavedStderr = GetStdHandle( STD_ERROR_HANDLE );

	if ( !::AllocConsole() )
	{
		*pszErrorMessage = "Can't create console: ";
		GetErrorMessage( pszErrorMessage, GetLastError() );
		return false;
	}

	HANDLE hChildOut = 0;
	HANDLE hChildIn = 0;
	HANDLE hChildErrOut = 0;

	pszErrorMessage->clear();

	if ( !RedirectStdStream( &hParentIn, &hChildOut, STD_OUTPUT_HANDLE, pszErrorMessage ) )
	{
		return false;
	}
	if ( !RedirectStdStream( &hParentOut, &hChildIn, STD_INPUT_HANDLE, pszErrorMessage ) )
	{
		CloseHandle( hChildOut );
		CleanupHandles();
		return false;
	}
	if ( !RedirectStdStream( &hParentErrIn, &hChildErrOut, STD_ERROR_HANDLE, pszErrorMessage ) )
	{
		CloseHandle( hChildOut );
		CloseHandle( hChildIn );
		CleanupHandles();
		return false;
	}

	bool bResult = true;

	{
		STARTUPINFO startInfo;
		memset( &startInfo, 0, sizeof(STARTUPINFO) );
		startInfo.cb = sizeof(STARTUPINFO); 
		startInfo.dwFlags = STARTF_USESHOWWINDOW;//STARTF_USESTDHANDLES;
		startInfo.wShowWindow = SW_HIDE;
		//startInfo.hStdInput = 
		//startInfo.hStdOutput = 
		//startInfo.hStdError = 

		memset( &procInfo, 0, sizeof(PROCESS_INFORMATION) );

		string szCmdLineCopy( szCommandLine );
		bResult = CreateProcess( NULL,
			const_cast<char*>(szCmdLineCopy.c_str()),	// command line
			0,                      // process security attributes
			0,                      // primary thread security attributes
			true,                   // handles are inherited
			0,                      // creation flags
			0,                      // use parent's environment
			0,                      // use parent's current directory
			&startInfo,             // STARTUPINFO pointer
			&procInfo               // receives PROCESS_INFORMATION
			);
		if ( !bResult )
		{
			*pszErrorMessage = "Create process failed: ";
			GetErrorMessage( pszErrorMessage, GetLastError() );
			CleanupHandles();
		}
	}

	CloseHandle( hChildIn );
	CloseHandle( hChildOut );
	CloseHandle( hChildErrOut );

	SetStdHandle( STD_OUTPUT_HANDLE, hSavedStdout );
	SetStdHandle( STD_INPUT_HANDLE, hSavedStdin );
	SetStdHandle( STD_ERROR_HANDLE, hSavedStderr );

	return bResult;
}


bool CInteractiveProcess::Stop( const string &szQuitScript )
{
	if ( !IsStarted() )
	{
		return true;
	}

	bool bResult = false;

	int nBytesWrittenTotal = 0;
	int nBytesWritten = 0;
	do
	{
		bResult = ::WriteFile( hParentOut, szQuitScript.data() + nBytesWrittenTotal, szQuitScript.size() - nBytesWrittenTotal, (LPDWORD)&nBytesWritten, 0 );
		nBytesWrittenTotal += nBytesWritten;
	}
	while( bResult && (nBytesWrittenTotal < szQuitScript.size()) );
	// FIXME: what if write operation has failed?

	InternalStop();

	return true;
}


// Sends command to the interactive process and gets response on that command.
//
// Return false if any IO operation with process has been failed.
//
// szScript -- command text
// szResponseEndMark -- mark by which response end can be detected, usually prompt
// pszOutput -- stdout of the command
// pszErrorOutput -- stderr of the process at the moment when response had been detected
// pszErrorMessage -- contains description of the error in case Execute() fails (returns false)
//
// pszErrorOutput can be 0, pszOutput and pszErrorMessage cannot.
//
bool CInteractiveProcess::Execute( const string &szScript, const string &szResponseEndMark, string *pszOutput, string *pszErrorOutput, string *pszErrorMessage )
{
	pszOutput->clear();
	if ( pszErrorOutput )
	{
		pszErrorOutput->clear();
	}
	pszErrorMessage->clear();

	bool bResult = true;

	if ( szScript.size() )
	{
		int nBytesWrittenTotal = 0;
		int nBytesWritten = 0;
		do
		{
			bResult = ::WriteFile( hParentOut, szScript.data() + nBytesWrittenTotal, szScript.size() - nBytesWrittenTotal, (LPDWORD)&nBytesWritten, 0 );
			nBytesWrittenTotal += nBytesWritten;
		}
		while( bResult && (nBytesWrittenTotal < szScript.size()) );
	}

	int nSleepDuration = 0;
	if ( bResult )
	{
		char readBuffer[PIPE_BUFSIZE];
		bool bReadFurther = true;
		int nLineCount = StringCountNL( szScript );
		do
		{
			int nBytesAvail = 0;
			bResult = ::PeekNamedPipe( hParentIn, 0, 0, 0, (LPDWORD)&nBytesAvail, 0 );
			if ( bResult && nBytesAvail > 0 )
			{
				int nBytesReadTotal = 0;
				int nBytesRead = 0;
				do
				{
					bResult = ::ReadFile( hParentIn, (LPVOID)(readBuffer + nBytesReadTotal), nBytesAvail - nBytesReadTotal, (LPDWORD)&nBytesRead, 0 );
					nBytesReadTotal += nBytesRead;
				}
				while( bResult && (nBytesReadTotal < nBytesAvail) );
				readBuffer[nBytesReadTotal] = '\0';
				pszOutput->append( readBuffer );
				if ( bResult )
				{
					// маркер может быть разбит на две и более посылок, поэтому искать маркер нужно с захватом
					// некоторого количества (а именно (szResponseEndMark.size() - 1)) уже прочтённых символов
					int nMarkCharCount = szResponseEndMark.size();
					int nStartSearchPos = pszOutput->size() - nBytesReadTotal - (nMarkCharCount - 1);
					nStartSearchPos = max(0, nStartSearchPos);

					const int nMarkCount = StringCountSubstring( pszOutput->c_str() + nStartSearchPos, szResponseEndMark.c_str(), nMarkCharCount );
					nLineCount -= nMarkCount;
					if ( string::npos != pszOutput->find( szResponseEndMark, (pszOutput->size() - nMarkCharCount) ) )
					{
						bReadFurther = (nLineCount > 0);
					}
				}
				nSleepDuration = 0;
			}
			else
			{
				Sleep( SLEEP_STEP );
				nSleepDuration += SLEEP_STEP;
				if( nSleepDuration > RESPONSEWAIT_TIMEOUT )
				{
					*pszErrorMessage = StrFmt("time of execution exceeded %u seconds\n", RESPONSEWAIT_TIMEOUT / 1000 );
					bResult = false;
				}
			}
		}
		while( bResult && bReadFurther );

		// FIXME: добавить чтение stderr
	}

	if ( bResult == false )
	{
		if( pszErrorMessage->empty() )
			GetErrorMessage( pszErrorMessage, GetLastError() );

		DWORD nTimeout = 500;
		switch ( ::WaitForSingleObject( procInfo.hProcess, nTimeout ) )
		{
			case WAIT_OBJECT_0: // external process have been already terminated
			{
				InternalStop();
				break;
			}
		}
	}

	return bResult;
}



