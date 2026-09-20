#include "stdafx.h"
#include <fmt/format.h>

#include "InteractiveProcess.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <thread>

#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace
{
	const int PIPE_BUFSIZE = 4096;
	const int SLEEP_STEP = 100;


	// The platform layer, and the only part of this file that knows which
	// system it is on. Everything below it -- writing a script, scanning the
	// output for the prompt, counting how many prompts are still owed -- is the
	// same on both, because it is about what Maya says rather than about pipes.
	//
	// A pipe end and a child process, spelled for each platform. Windows names
	// its own HANDLE rather than reusing the one DXVK's windows.h declares off
	// Windows, which is a type without any of the functions that work on it.
#if BOOST_OS_WINDOWS
	typedef HANDLE TPipe;
	const TPipe NO_PIPE = 0;
#else
	typedef int TPipe;
	const TPipe NO_PIPE = -1;
#endif


	struct SChild
	{
#if BOOST_OS_WINDOWS
		HANDLE hProcess = 0;
		HANDLE hThread = 0;
#else
		pid_t nPid = -1;
		// waitpid() both waits and reaps, so a child asked about is a child
		// collected; Windows can wait as often as it likes. Remembering that it
		// has been reaped is what keeps the two behaving alike.
		bool bReaped = false;
#endif

		bool IsRunning() const
		{
#if BOOST_OS_WINDOWS
			return hProcess != 0;
#else
			return nPid != -1;
#endif
		}
	};


	// The system's account of whatever just failed, appended rather than
	// assigned: every caller has already put its own sentence in the string.
	void AppendLastError( std::string *pszText )
	{
		NI_ASSERT( pszText, "Supplied string pointer is null" );
#if BOOST_OS_WINDOWS
		LPVOID lpMsgBuf = 0;
		const DWORD nErrorID = ::GetLastError();
		if ( ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
													 NULL, nErrorID, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
													 reinterpret_cast<LPSTR>( &lpMsgBuf ), 0, NULL ) )
		{
			*pszText += static_cast<const char*>( lpMsgBuf );
			::LocalFree( lpMsgBuf );
		}
		else
		{
			*pszText += fmt::format( "FormatMessage failed, error {}", nErrorID );
		}
#else
		*pszText += std::strerror( errno );
#endif
	}


	void ClosePipe( TPipe *pPipe )
	{
		if ( *pPipe != NO_PIPE )
		{
#if BOOST_OS_WINDOWS
			::CloseHandle( *pPipe );
#else
			::close( *pPipe );
#endif
			*pPipe = NO_PIPE;
		}
	}


	// Start szCommandLine with its three standard streams on pipes back here.
	//
	// The Win32 side used to redirect by calling AllocConsole and pointing the
	// *editor's* own std handles at the pipes for the length of the call, so
	// that the child inherited them, then putting them back. That is process
	// wide state changed behind everything else in the editor, and it was never
	// what the code wanted: STARTF_USESTDHANDLES and the three hStd assignments
	// were sitting right there, commented out. They are used now, so the child
	// is told its streams directly, nothing global moves, and the console this
	// used to allocate and free is not needed at all.
	bool SpawnWithPipes( const std::string &szCommandLine, SChild *pChild,
											 TPipe *pParentIn, TPipe *pParentOut, TPipe *pParentErrIn,
											 std::string *pszErrorMessage )
	{
		*pParentIn = NO_PIPE;
		*pParentOut = NO_PIPE;
		*pParentErrIn = NO_PIPE;

#if BOOST_OS_WINDOWS
		SECURITY_ATTRIBUTES saAttr = {};
		saAttr.nLength = sizeof( saAttr );
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;

		HANDLE hChildIn = 0, hChildOut = 0, hChildErrOut = 0;
		bool bOk = true;

		// One pipe, keeping the end the child needs inheritable and replacing
		// this side's end with an uninheritable duplicate: an inheritable read
		// end here would be handed to the child too, and the pipe would then
		// never report end of file.
		const auto MakePipe = [&]( HANDLE *phChildSide, HANDLE *phParentSide, bool bChildWrites )
		{
			HANDLE hRead = 0, hWrite = 0;
			if ( !::CreatePipe( &hRead, &hWrite, &saAttr, PIPE_BUFSIZE ) )
			{
				*pszErrorMessage = "Can't create pipe: ";
				AppendLastError( pszErrorMessage );
				return false;
			}
			HANDLE hInheritableParentSide = bChildWrites ? hRead : hWrite;
			*phChildSide = bChildWrites ? hWrite : hRead;
			const BOOL bDuplicated = ::DuplicateHandle( ::GetCurrentProcess(), hInheritableParentSide,
																									::GetCurrentProcess(), phParentSide,
																									0, FALSE, DUPLICATE_SAME_ACCESS );
			::CloseHandle( hInheritableParentSide );
			if ( !bDuplicated )
			{
				*pszErrorMessage = "Can't duplicate handle to make uninheritable one: ";
				AppendLastError( pszErrorMessage );
				::CloseHandle( *phChildSide );
				*phChildSide = 0;
				return false;
			}
			return true;
		};

		bOk = MakePipe( &hChildOut, pParentIn, true );
		if ( bOk )
		{
			bOk = MakePipe( &hChildIn, pParentOut, false );
		}
		if ( bOk )
		{
			bOk = MakePipe( &hChildErrOut, pParentErrIn, true );
		}

		if ( bOk )
		{
			STARTUPINFOA startInfo = {};
			startInfo.cb = sizeof( startInfo );
			startInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			startInfo.wShowWindow = SW_HIDE;
			startInfo.hStdInput = hChildIn;
			startInfo.hStdOutput = hChildOut;
			startInfo.hStdError = hChildErrOut;

			PROCESS_INFORMATION procInfo = {};
			std::string szCmdLineCopy( szCommandLine );
			if ( ::CreateProcessA( NULL, &szCmdLineCopy[0], 0, 0, TRUE, CREATE_NO_WINDOW, 0, 0,
														 &startInfo, &procInfo ) )
			{
				pChild->hProcess = procInfo.hProcess;
				pChild->hThread = procInfo.hThread;
			}
			else
			{
				*pszErrorMessage = "Create process failed: ";
				AppendLastError( pszErrorMessage );
				bOk = false;
			}
		}

		// This side has no use for the child's ends whether it started or not.
		if ( hChildIn != 0 ) { ::CloseHandle( hChildIn ); }
		if ( hChildOut != 0 ) { ::CloseHandle( hChildOut ); }
		if ( hChildErrOut != 0 ) { ::CloseHandle( hChildErrOut ); }

		if ( !bOk )
		{
			ClosePipe( pParentIn );
			ClosePipe( pParentOut );
			ClosePipe( pParentErrIn );
		}
		return bOk;
#else
		// stdin, stdout, stderr, each [read, write].
		int childStdin[2] = { -1, -1 };
		int childStdout[2] = { -1, -1 };
		int childStderr[2] = { -1, -1 };
		if ( ( ::pipe( childStdin ) != 0 ) || ( ::pipe( childStdout ) != 0 ) || ( ::pipe( childStderr ) != 0 ) )
		{
			*pszErrorMessage = "Can't create pipe: ";
			AppendLastError( pszErrorMessage );
			for ( int *pPair : { childStdin, childStdout, childStderr } )
			{
				if ( pPair[0] != -1 ) { ::close( pPair[0] ); }
				if ( pPair[1] != -1 ) { ::close( pPair[1] ); }
			}
			return false;
		}

		const pid_t nPid = ::fork();
		if ( nPid < 0 )
		{
			*pszErrorMessage = "Create process failed: ";
			AppendLastError( pszErrorMessage );
			for ( int *pPair : { childStdin, childStdout, childStderr } )
			{
				::close( pPair[0] );
				::close( pPair[1] );
			}
			return false;
		}

		if ( nPid == 0 )
		{
			// The child. Only async signal safe calls from here to exec.
			::dup2( childStdin[0], STDIN_FILENO );
			::dup2( childStdout[1], STDOUT_FILENO );
			::dup2( childStderr[1], STDERR_FILENO );
			::close( childStdin[0] );  ::close( childStdin[1] );
			::close( childStdout[0] ); ::close( childStdout[1] );
			::close( childStderr[0] ); ::close( childStderr[1] );
			// A command line, not a program and arguments: the one caller passes
			// "mayabatch -prompt", and the shell resolves and splits that the way
			// CreateProcess's own parsing does on the other side.
			::execl( "/bin/sh", "sh", "-c", szCommandLine.c_str(), static_cast<char*>( nullptr ) );
			::_exit( 127 );
		}

		::close( childStdin[0] );
		::close( childStdout[1] );
		::close( childStderr[1] );
		*pParentOut = childStdin[1];
		*pParentIn = childStdout[0];
		*pParentErrIn = childStderr[0];
		// Read has to be able to answer "nothing yet" without blocking, which is
		// what PeekNamedPipe gives the other side for free.
		for ( TPipe pipe : { *pParentIn, *pParentErrIn } )
		{
			::fcntl( pipe, F_SETFL, ::fcntl( pipe, F_GETFL, 0 ) | O_NONBLOCK );
		}
		pChild->nPid = nPid;
		pChild->bReaped = false;
		return true;
#endif
	}


	// Write all of it, or say which write failed.
	bool WriteAll( TPipe pipe, const char *pData, size_t nSize )
	{
		size_t nWrittenTotal = 0;
		while ( nWrittenTotal < nSize )
		{
#if BOOST_OS_WINDOWS
			DWORD nWritten = 0;
			if ( !::WriteFile( pipe, pData + nWrittenTotal, static_cast<DWORD>( nSize - nWrittenTotal ), &nWritten, 0 ) )
			{
				return false;
			}
#else
			const ssize_t nWritten = ::write( pipe, pData + nWrittenTotal, nSize - nWrittenTotal );
			if ( nWritten < 0 )
			{
				if ( errno == EINTR )
				{
					continue;
				}
				return false;
			}
#endif
			if ( nWritten == 0 )
			{
				return false;
			}
			nWrittenTotal += static_cast<size_t>( nWritten );
		}
		return true;
	}


	// What can be read right now, without waiting. Returns the byte count, 0
	// when the child has simply not said anything yet, and -1 on a broken pipe
	// or a closed one, which is how a child that has exited shows up here.
	int ReadAvailable( TPipe pipe, char *pBuffer, int nBufferSize )
	{
#if BOOST_OS_WINDOWS
		DWORD nAvailable = 0;
		if ( !::PeekNamedPipe( pipe, 0, 0, 0, &nAvailable, 0 ) )
		{
			return -1;
		}
		if ( nAvailable == 0 )
		{
			return 0;
		}
		// Clamped: PeekNamedPipe reports everything queued, and CreatePipe's
		// size is a hint the system may exceed, so the old code could be told
		// of more bytes than the buffer it was about to read them into.
		const DWORD nWanted = (std::min)( nAvailable, static_cast<DWORD>( nBufferSize ) );
		DWORD nRead = 0;
		if ( !::ReadFile( pipe, pBuffer, nWanted, &nRead, 0 ) )
		{
			return -1;
		}
		return static_cast<int>( nRead );
#else
		const ssize_t nRead = ::read( pipe, pBuffer, static_cast<size_t>( nBufferSize ) );
		if ( nRead > 0 )
		{
			return static_cast<int>( nRead );
		}
		if ( nRead == 0 )
		{
			// End of file: every write end is shut, so the child is gone.
			return -1;
		}
		if ( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) || ( errno == EINTR ) )
		{
			return 0;
		}
		return -1;
#endif
	}


	// Whether the child has finished, waiting up to nTimeoutMs for it to.
	bool HasExited( SChild *pChild, int nTimeoutMs )
	{
		if ( !pChild->IsRunning() )
		{
			return true;
		}
#if BOOST_OS_WINDOWS
		return ::WaitForSingleObject( pChild->hProcess, static_cast<DWORD>( nTimeoutMs ) ) == WAIT_OBJECT_0;
#else
		// waitpid collects the child as well as reporting it, so the answer is
		// remembered: asking twice would otherwise fail the second time.
		if ( pChild->bReaped )
		{
			return true;
		}
		for ( int nWaited = 0; ; nWaited += SLEEP_STEP )
		{
			int nStatus = 0;
			const pid_t nResult = ::waitpid( pChild->nPid, &nStatus, WNOHANG );
			if ( nResult == pChild->nPid )
			{
				pChild->bReaped = true;
				return true;
			}
			if ( ( nResult < 0 ) && ( errno != EINTR ) )
			{
				// No such child: someone else reaped it, so it is certainly over.
				pChild->bReaped = true;
				return true;
			}
			if ( nWaited >= nTimeoutMs )
			{
				return false;
			}
			std::this_thread::sleep_for( std::chrono::milliseconds( SLEEP_STEP ) );
		}
#endif
	}


	// Wait for the child however long it takes, then let go of it.
	void WaitAndRelease( SChild *pChild )
	{
		if ( !pChild->IsRunning() )
		{
			return;
		}
#if BOOST_OS_WINDOWS
		::WaitForSingleObject( pChild->hProcess, INFINITE );
		::CloseHandle( pChild->hThread );
		::CloseHandle( pChild->hProcess );
		pChild->hProcess = 0;
		pChild->hThread = 0;
#else
		if ( !pChild->bReaped )
		{
			int nStatus = 0;
			while ( ( ::waitpid( pChild->nPid, &nStatus, 0 ) < 0 ) && ( errno == EINTR ) )
			{
			}
		}
		pChild->nPid = -1;
		pChild->bReaped = false;
#endif
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int StringCountNL( const std::string &s )
	{
		int n = 0;
		for ( size_t i = 0; i < s.size(); ++i )
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


// Everything the class does. The public class below is a forwarder; see
// InteractiveProcess.h for why none of this is declared there.
struct CInteractiveProcess::SImpl
{
	TPipe hParentIn = NO_PIPE;
	TPipe hParentOut = NO_PIPE;
	TPipe hParentErrIn = NO_PIPE;
	SChild child;
	int RESPONSEWAIT_TIMEOUT;

	SImpl( int nResponseWaitTimeout ) : RESPONSEWAIT_TIMEOUT( nResponseWaitTimeout ) {}

	void CleanupHandles();
	void InternalStop();

	bool IsStarted() const { return child.IsRunning(); }

	bool Start( const std::string &szCommandLine, std::string *pszErrorMessage );
	bool Execute( const std::string &szScript, const std::string &szResponseEndLabel, std::string *pszOutput, std::string *pszErrorOutput, std::string *pszErrorMessage );
	bool Stop( const std::string &szQuitScript );
};


CInteractiveProcess::CInteractiveProcess( int nResponseWaitTimeout )
	: pImpl( new SImpl( nResponseWaitTimeout ) )
{
}

CInteractiveProcess::~CInteractiveProcess() = default;

bool CInteractiveProcess::IsStarted()
{
	return pImpl->IsStarted();
}

void CInteractiveProcess::SetResponseTimeout( int nResponseWaitTimeout )
{
	pImpl->RESPONSEWAIT_TIMEOUT = nResponseWaitTimeout;
}

bool CInteractiveProcess::Start( const std::string &szCommandLine, std::string *pszErrorMessage )
{
	return pImpl->Start( szCommandLine, pszErrorMessage );
}

bool CInteractiveProcess::Execute( const std::string &szScript, const std::string &szResponseEndLabel, std::string *pszOutput, std::string *pszErrorOutput, std::string *pszErrorMessage )
{
	return pImpl->Execute( szScript, szResponseEndLabel, pszOutput, pszErrorOutput, pszErrorMessage );
}

bool CInteractiveProcess::Stop( const std::string &szQuitScript )
{
	return pImpl->Stop( szQuitScript );
}


// CInteractiveProcess::SImpl
//
void CInteractiveProcess::SImpl::CleanupHandles()
{
	ClosePipe( &hParentIn );
	ClosePipe( &hParentOut );
	ClosePipe( &hParentErrIn );
}


void CInteractiveProcess::SImpl::InternalStop()
{
	// The pipes go first: closing this side's write end is what tells a child
	// still reading its input that there will be no more of it, and without
	// that the wait below would not end.
	CleanupHandles();
	WaitAndRelease( &child );
}


bool CInteractiveProcess::SImpl::Start( const std::string &szCommandLine, std::string *pszErrorMessage )
{
	if ( IsStarted() )
	{
		return true;
	}
	pszErrorMessage->clear();
	return SpawnWithPipes( szCommandLine, &child, &hParentIn, &hParentOut, &hParentErrIn, pszErrorMessage );
}


bool CInteractiveProcess::SImpl::Stop( const std::string &szQuitScript )
{
	if ( !IsStarted() )
	{
		return true;
	}

	WriteAll( hParentOut, szQuitScript.data(), szQuitScript.size() );
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
bool CInteractiveProcess::SImpl::Execute( const std::string &szScript, const std::string &szResponseEndMark, std::string *pszOutput, std::string *pszErrorOutput, std::string *pszErrorMessage )
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
		bResult = WriteAll( hParentOut, szScript.data(), szScript.size() );
	}

	int nSleepDuration = 0;
	if ( bResult )
	{
		// Read one byte short of it, so the terminator below always fits.
		char readBuffer[PIPE_BUFSIZE];
		bool bReadFurther = true;
		int nLineCount = StringCountNL( szScript );
		do
		{
			const int nBytesRead = ReadAvailable( hParentIn, readBuffer, PIPE_BUFSIZE - 1 );
			if ( nBytesRead < 0 )
			{
				bResult = false;
			}
			else if ( nBytesRead > 0 )
			{
				readBuffer[nBytesRead] = '\0';
				pszOutput->append( readBuffer );
				// маркер может быть разбит на две и более посылок, поэтому искать маркер нужно с захватом
				// некоторого количества (а именно (szResponseEndMark.size() - 1)) уже прочтённых символов
				const int nMarkCharCount = szResponseEndMark.size();
				int nStartSearchPos = pszOutput->size() - nBytesRead - (nMarkCharCount - 1);
				nStartSearchPos = (std::max)(0, nStartSearchPos);

				const int nMarkCount = StringCountSubstring( pszOutput->c_str() + nStartSearchPos, szResponseEndMark.c_str(), nMarkCharCount );
				nLineCount -= nMarkCount;
				if ( std::string::npos != pszOutput->find( szResponseEndMark, (pszOutput->size() - nMarkCharCount) ) )
				{
					bReadFurther = (nLineCount > 0);
				}
				nSleepDuration = 0;
			}
			else
			{
				std::this_thread::sleep_for( std::chrono::milliseconds( SLEEP_STEP ) );
				nSleepDuration += SLEEP_STEP;
				if( nSleepDuration > RESPONSEWAIT_TIMEOUT )
				{
					*pszErrorMessage = fmt::format("time of execution exceeded {} seconds\n", RESPONSEWAIT_TIMEOUT / 1000 );
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
		{
			AppendLastError( pszErrorMessage );
		}

		if ( HasExited( &child, 500 ) )
		{
			// external process have been already terminated
			InternalStop();
		}
	}

	return bResult;
}
