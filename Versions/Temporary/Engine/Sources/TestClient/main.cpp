#include "stdafx.h"

#include "TestClient.h"

#include "Misc/StrProc.h"
#include "Server_Client_Common/Commands.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

// Entry point for the lobby server's test client.
//
// This replaces CTestClientApp, an MFC CWinApp whose 50 ms Win32 timer was the
// only main loop, and CConsole, a Scintilla editor control that both read the
// commands and displayed the output. Scintilla has no sources in this tree,
// only prebuilt DLLs, so that console could not have been ported.
//
// Both halves of what it did are reproduced here against stdin and stdout, and
// the program becomes something that can run without a desktop, which is what
// a load generator and a watchdog wanted in the first place. What is lost is
// the tab completion Scintilla provided over the command table.

namespace
{

// The period the Win32 timer ran at. Holding to it matters more here than it
// does in the server: CTestClient::Segment paces tester creation and the
// pinger off wall-clock time, and every simulated client shares this thread.
const std::chrono::milliseconds SEGMENT_PERIOD( 50 );

std::atomic<bool> bStopRequested( false );

extern "C" void OnStopSignal( int )
{
	bStopRequested = true;
}

std::mutex inputMutex;
std::deque<std::string> pendingInput;

// Commands arrive on a thread of their own because std::getline blocks, and
// this program has to keep stepping while nobody is typing: the pinger logs in
// on a timer and multitest's simulated clients only advance when Segment runs.
// CTerminal in the server reads its socket the same way and for the same
// reason.
//
// Detached rather than joined. There is no portable way to interrupt a thread
// parked in a blocking read, so it ends when the process does.
void ReadInput()
{
	std::string szLine;
	while ( std::getline( std::cin, szLine ) )
	{
		std::lock_guard<std::mutex> lock( inputMutex );
		pendingInput.push_back( szLine );
	}
}

// Everything the client prints goes through WriteMSG into IConsoleBuffer, and
// something has to take it out again; CConsole::Segment was what did. The
// sequence id is how the buffer tracks what this reader has already seen.
void DrainConsole( int *pnSequenceID )
{
	IConsoleBuffer *pBuffer = Singleton<IConsoleBuffer>();
	IConsoleBuffer::SConsoleLine line;
	while ( pBuffer->GetNextLine( &line, pnSequenceID ) )
	{
		if ( line.nStream == CONSOLE_STREAM_CONSOLE )
		{
			std::fputs( NStr::ToMBCS( line.szText ).c_str(), stdout );
		}
	}
	std::fflush( stdout );
}

// The directory holding client.xml: the parent of the working directory,
// because this runs from a bin/ subdirectory with the configuration beside it.
//
// As in the server's main, the NStr::ToLower that the original applied to this
// path is dropped rather than carried over. It was harmless on Windows and
// wrong anywhere paths are case sensitive.
std::string GetBaseDir()
{
	const std::filesystem::path base = std::filesystem::current_path().parent_path();
	std::string szBaseDir = base.generic_string();
	if ( !szBaseDir.empty() && szBaseDir.back() != '/' )
	{
		szBaseDir += '/';
	}
	return szBaseDir;
}

}

int main()
{
	std::signal( SIGINT, &OnStopSignal );
	std::signal( SIGTERM, &OnStopSignal );

	// false: the client half of the shared command table. The server passes
	// true and gets a different set of commands out of the same class.
	CObj<CCommands> pCmds = new CCommands( false );
	CObj<CTestClient> pTestClient = new CTestClient( pCmds, GetBaseDir() + "client.xml" );

	std::thread( &ReadInput ).detach();

	int nConsoleSequenceID = 0;
	std::fputs( "Test client running. Type a command, or send SIGINT to stop.\n", stdout );

	while ( !bStopRequested )
	{
		const std::chrono::steady_clock::time_point tickStart = std::chrono::steady_clock::now();

		for ( ;; )
		{
			std::string szLine;
			{
				std::lock_guard<std::mutex> lock( inputMutex );
				if ( pendingInput.empty() )
				{
					break;
				}
				szLine = pendingInput.front();
				pendingInput.pop_front();
			}

			std::string szError;
			if ( !pCmds->LineEntered( szLine, &szError ) )
			{
				std::fputs( szError.c_str(), stdout );
			}
		}

		pTestClient->Segment();
		DrainConsole( &nConsoleSequenceID );

		std::this_thread::sleep_until( tickStart + SEGMENT_PERIOD );
	}

	DrainConsole( &nConsoleSequenceID );
	std::fputs( "Shutting down.\n", stdout );

	pTestClient = 0;
	pCmds = 0;

	return 0;
}
