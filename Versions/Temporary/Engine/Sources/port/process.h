#pragma once

#include <boost/predef.h>

#include <string>
#include <vector>

#if BOOST_OS_WINDOWS
#include <windows.h>

#include <shellapi.h>
#else
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>

extern char **environ;
#endif


#if !BOOST_OS_WINDOWS
// A word the shell will take as it stands. Single quotes stop every expansion
// sh does; an embedded quote is the one thing they cannot hold, so it is
// closed, escaped and reopened.
inline std::string ShellQuote( const std::string &rszText )
{
	std::string quoted = "'";
	for ( const char c : rszText )
	{
		if ( c == '\'' )
		{
			quoted += "'\\''";
		}
		else
		{
			quoted += c;
		}
	}
	return quoted + "'";
}


// 'cd <dir> && exec <program> <arguments>', the shell's way of saying what
// CreateProcess and ShellExecuteEx say with their own arguments. An empty
// program means szArguments is already a whole command line, which is how
// CreateProcess reads a null lpApplicationName.
inline std::string BuildShellCommand( const std::string &szProgram,
																			const std::string &szArguments,
																			const std::string &szWorkingDir )
{
	std::string command;
	if ( !szWorkingDir.empty() )
	{
		command += "cd " + ShellQuote( szWorkingDir ) + " && ";
	}
	// exec so the shell replaces itself with the program rather than waiting
	// on it, which leaves one process instead of two.
	command += "exec ";
	if ( !szProgram.empty() )
	{
		command += ShellQuote( szProgram );
		if ( !szArguments.empty() )
		{
			command += " ";
		}
	}
	command += szArguments;
	return command;
}
#endif

// Launch a command and do not wait for it.
//
// Windows keeps ShellExecute, the call this replaces off it, so that what the
// hook can launch there does not change: the "open" verb goes through the shell
// association table, so a document or a .bat opens the same way an .exe runs.
//
// Boost.Process would have covered both platforms in one call and was tried
// first. It does not fit here: v2::process needs Asio, Asio needs winsock2.h,
// and port/socket.h includes winsock.h, so the two cannot be in one translation
// unit. Fixing that means moving the whole network layer to winsock2, which is
// not something to do as a side effect of launching a test hook.
//
// So POSIX gets /bin/sh -c, which is the nearest thing to the "open" verb for a
// command string: the shell resolves it against PATH and applies its own quoting
// rules. posix_spawn returns as soon as the child exists, which is the part that
// matters - ShellExecute did not wait either, and both callers queue an exit
// immediately afterwards.
//
// The child is not reaped, so it is a zombie between its exit and the game's.
// Both callers end the game within the same second, so that window is not worth
// a SIGCHLD handler or a double fork.
inline bool LaunchDetached( const std::string &szCommand )
{
	if ( szCommand.empty() )
	{
		return false;
	}
#if BOOST_OS_WINDOWS
	// ShellExecute returns a fake HINSTANCE; anything above 32 means it launched
	const HINSTANCE hResult = ::ShellExecuteA( 0, "open", szCommand.c_str(), "", "", SW_SHOWNORMAL );
	return reinterpret_cast<INT_PTR>( hResult ) > 32;
#else
	char szShell[] = "/bin/sh";
	char szFlag[] = "-c";
	std::vector<char> command( szCommand.begin(), szCommand.end() );
	command.push_back( '\0' );
	char *argv[] = { szShell, szFlag, &command[0], 0 };
	pid_t pid = 0;
	return ::posix_spawn( &pid, szShell, 0, 0, argv, environ ) == 0;
#endif
}


// Launch one named program, with arguments, from a working directory, and do
// not wait for it. The editor starts the game this way.
//
// Separate from the overload above because the working directory is not a
// detail here: the game resolves its Data, Profiles and Editor folders against
// it, so starting it from anywhere else gives it an empty database.
//
// Windows keeps ShellExecuteEx rather than moving to CreateProcess, for the
// reason the overload above keeps ShellExecute: it is what the call being
// replaced did, and the shell's answer to "run this" is not always
// CreateProcess's -- elevation asked for by a manifest is the case that would
// bite. SEE_MASK_FLAG_NO_UI because the caller reports failures itself.
//
// POSIX builds a shell command instead of calling posix_spawn directly,
// because posix_spawn has no portable way to set the child's directory:
// addchdir_np is a glibc extension and recent. 'cd <dir> && exec <program>
// <arguments>' says the same thing with the shell already here.
inline bool LaunchDetachedIn( const std::string &szProgram,
															const std::string &szArguments,
															const std::string &szWorkingDir )
{
	if ( szProgram.empty() )
	{
		return false;
	}
#if BOOST_OS_WINDOWS
	SHELLEXECUTEINFOA info = {};
	info.cbSize = sizeof( info );
	info.fMask = SEE_MASK_FLAG_NO_UI;
	info.lpVerb = "open";
	info.lpFile = szProgram.c_str();
	info.lpParameters = szArguments.empty() ? 0 : szArguments.c_str();
	info.lpDirectory = szWorkingDir.empty() ? 0 : szWorkingDir.c_str();
	info.nShow = SW_SHOWNORMAL;
	return ::ShellExecuteExA( &info ) != FALSE;
#else
	return LaunchDetached( BuildShellCommand( szProgram, szArguments, szWorkingDir ) );
#endif
}


// Run one program and wait for it to finish, answering whether it started.
//
// The editor shells out to small command line tools this way, and waits
// because what it does next needs their output files: FontGen writes a font's
// texture and metrics, Maya in batch mode writes an exported model, a zip tool
// packs a folder into a pak. None of them has its output read back here, so
// the child keeps this process's own standard streams; CInteractiveProcess is
// the one that needs pipes, and it has its own.
//
// An empty szProgram means szArguments is already a whole command line and
// names the program in its first word, which is what CreateProcess does with a
// null lpApplicationName and what most callers here pass.
//
// pnExitCode is optional. Note that it is the tool's exit code, which is not
// what the return value reports: a tool that ran and failed returns true with
// a non-zero code, exactly as CreateProcess plus WaitForSingleObject did.
inline bool RunAndWait( const std::string &szProgram,
												const std::string &szArguments,
												const std::string &szWorkingDir,
												int *pnExitCode = nullptr )
{
	if ( pnExitCode != nullptr )
	{
		*pnExitCode = -1;
	}
	if ( szProgram.empty() && szArguments.empty() )
	{
		return false;
	}
#if BOOST_OS_WINDOWS
	STARTUPINFOA startInfo = {};
	startInfo.cb = sizeof( startInfo );
	PROCESS_INFORMATION procInfo = {};
	// CreateProcess may write to the command line it is given, so it gets a
	// buffer of its own rather than the caller's string.
	std::string szCommandLine( szArguments );
	if ( !::CreateProcessA( szProgram.empty() ? 0 : szProgram.c_str(),
													szCommandLine.empty() ? 0 : &szCommandLine[0],
													0, 0, FALSE, 0, 0,
													szWorkingDir.empty() ? 0 : szWorkingDir.c_str(),
													&startInfo, &procInfo ) )
	{
		return false;
	}
	::WaitForSingleObject( procInfo.hProcess, INFINITE );
	if ( pnExitCode != nullptr )
	{
		DWORD nExitCode = 0;
		if ( ::GetExitCodeProcess( procInfo.hProcess, &nExitCode ) )
		{
			*pnExitCode = static_cast<int>( nExitCode );
		}
	}
	::CloseHandle( procInfo.hThread );
	::CloseHandle( procInfo.hProcess );
	return true;
#else
	const std::string szCommand = BuildShellCommand( szProgram, szArguments, szWorkingDir );
	char szShell[] = "/bin/sh";
	char szFlag[] = "-c";
	std::vector<char> command( szCommand.begin(), szCommand.end() );
	command.push_back( '\0' );
	char *argv[] = { szShell, szFlag, &command[0], 0 };
	pid_t pid = 0;
	if ( ::posix_spawn( &pid, szShell, 0, 0, argv, environ ) != 0 )
	{
		return false;
	}
	int nStatus = 0;
	while ( ::waitpid( pid, &nStatus, 0 ) < 0 )
	{
		if ( errno != EINTR )
		{
			return false;
		}
	}
	if ( pnExitCode != nullptr )
	{
		// The shell passes the program's own code back, and exec'd itself away
		// so there is no extra process between. 128 plus the signal for a death
		// by signal is the shell's convention, and is used here for the same
		// reason: the caller wants one number.
		*pnExitCode = WIFEXITED( nStatus ) ? WEXITSTATUS( nStatus )
																			 : ( WIFSIGNALED( nStatus ) ? ( 128 + WTERMSIG( nStatus ) ) : -1 );
	}
	return true;
#endif
}
