#include "stdafx.h"

#include "CustomLobby.h"
#include "LadderLobby.h"
#include "Server.h"
#include "Server_Client_Common/Commands.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <string>
#include <filesystem>
#include <thread>

// Entry point for the dedicated lobby server.
//
// This replaces CServerApp, which was an MFC CWinApp whose InitInstance built
// the server and whose SetTimer( 0, 1, 50, TimerProc ) drove every tick off the
// Win32 message pump. None of that survives the port: the console it opened was
// a Scintilla control whose sources are not in this tree, and a daemon has no
// use for a message pump.
//
// The console is not replaced by anything here either, because it was already
// replaced: Specific.h defines USE_REMOTE_CONSOLE, which routes WriteMSG to
// CTerminal's TCP socket, and CServerApp had already commented out the call
// that pumped output into the window. Commands arrive over the terminal and
// leave through it, and CGameServer::Segment() pumps that end itself.

namespace
{

// The tick period the Win32 timer was set to. CGameServer::Segment() is cheap
// and mostly waits on the network, so this is a pacing figure rather than a
// budget.
const std::chrono::milliseconds SEGMENT_PERIOD( 50 );

// Set from a signal handler, so it is the only type the standard lets one
// touch. Checked once per tick, which is close enough for a shutdown.
std::atomic<bool> bStopRequested( false );

extern "C" void OnStopSignal( int )
{
	bStopRequested = true;
}

// The directory the server reads its configuration from: the parent of the
// working directory, because the original ran from a bin/ subdirectory and the
// config sits beside it.
//
// This was GetCurrentDirectory into a fixed 1024-byte buffer, a hand-rolled
// append of '\\' and "..\\", and then NStr::ToLower on the result. The
// lowercasing is dropped rather than ported: it was harmless on Windows and
// silently wrong anywhere paths are case sensitive.
std::string GetBaseDir()
{
	const std::filesystem::path base = std::filesystem::current_path().parent_path();
	// generic_string, so the separators are '/' on both platforms. Every
	// consumer here hands the result to CFileStream, which takes either.
	std::string szBaseDir = base.generic_string();
	if ( !szBaseDir.empty() && szBaseDir.back() != '/' )
	{
		szBaseDir += '/';
	}
	return szBaseDir;
}

}

int main( int argc, char **argv )
{
	std::signal( SIGINT, &OnStopSignal );
	std::signal( SIGTERM, &OnStopSignal );

	// Two options, both of which override server.xml. The file is where a
	// server that always runs the same way should say so; these are for
	// running one the other way without editing the configuration it normally
	// uses, which is the common case when trying the embedded backend.
	std::string szBackend;
	std::string szDatabaseFile;
	for ( int i = 1; i < argc; ++i )
	{
		const std::string szArg = argv[i];
		const bool bHasValue = ( i + 1 < argc );

		if ( szArg == "--database" && bHasValue )
		{
			szBackend = argv[++i];
		}
		else if ( szArg == "--database-file" && bHasValue )
		{
			szDatabaseFile = argv[++i];
		}
		else if ( szArg == "--sqlite" )
		{
			// The short way of saying it, since it is the one that needs
			// nothing installed.
			szBackend = "sqlite";
		}
		else
		{
			std::fprintf( stderr, "unrecognised option: %s\n", szArg.c_str() );
			std::fprintf( stderr,
				"usage: Server [--database mysql|sqlite] [--database-file PATH] [--sqlite]\n"
				"  Without either, server.xml decides, and by default that is mysql.\n" );
			return 1;
		}
	}

	if ( !szBackend.empty() && szBackend != "mysql" && szBackend != "sqlite" )
	{
		std::fprintf( stderr, "unknown database backend: %s (expected mysql or sqlite)\n", szBackend.c_str() );
		return 1;
	}

	const std::string szCfgFile = GetBaseDir() + "server.xml";

	// Before anything is built, because everything that follows needs it and
	// because the failure was previously an access violation with nothing
	// printed: the reader dereferenced a null saver, and the assert that would
	// have caught it compiles to nothing. Reported here, where there is still
	// somewhere to report it to.
	//
	// The path is worth printing rather than just the failure. It is derived
	// from the working directory, so the usual cause is being run from the
	// wrong one: the server expects to start in bin/ with its configuration in
	// the directory above.
	{
		CFileStream stream( szCfgFile, CFileStream::WIN_READ_ONLY );
		if ( !stream.IsOk() )
		{
			std::fprintf( stderr, "Cannot open the configuration file: %s\n", szCfgFile.c_str() );
			std::fprintf( stderr, "The server reads server.xml from the directory above the one it runs in.\n" );
			return 1;
		}
	}

	CObj<CCommands> pCmds = new CCommands( true );
	CObj<CGameServer> pGameServer = new CGameServer( pCmds, szCfgFile, szBackend, szDatabaseFile );

	// Order matters: CGameServer's constructor has already added the control
	// lobby and the chat lobby, and lobbies[0] gets first refusal on every
	// packet. See CGameServer::RecievePackets.
	pGameServer->AddLobby( new CCustomLobby( pGameServer->GetClients(), szCfgFile ) );
	pGameServer->AddLobby( new CLadderLobby( pGameServer->GetClients(), szCfgFile ) );

	WriteMSG( "Server running. Send SIGINT or SIGTERM to stop.\n" );

	while ( !bStopRequested )
	{
		const std::chrono::steady_clock::time_point tickStart = std::chrono::steady_clock::now();

		pGameServer->Segment();

		// sleep_until rather than sleep_for, so the tick period covers the work
		// instead of being added to it. CServerApp slept a flat 20 ms inside
		// Segment() and let the 50 ms timer make up the rest, which meant the
		// real period drifted with how long a segment took.
		std::this_thread::sleep_until( tickStart + SEGMENT_PERIOD );
	}

	WriteMSG( "Shutting down.\n" );

	// Explicit, and in this order: CGameServer's destructor closes the MySQL
	// connection, and the lobbies it owns hold pointers into CClients.
	pGameServer = 0;
	pCmds = 0;

	return 0;
}
