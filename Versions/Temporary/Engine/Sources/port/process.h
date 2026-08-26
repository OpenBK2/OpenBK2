#pragma once

#include <boost/predef.h>

#include <string>
#include <vector>

#if BOOST_OS_WINDOWS
#include <windows.h>

#include <shellapi.h>
#else
#include <spawn.h>

extern char **environ;
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
