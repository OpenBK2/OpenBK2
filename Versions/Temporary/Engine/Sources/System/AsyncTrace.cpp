#include "stdafx.h"
#include "AsyncTrace.h"

#include "GlobalVars.h"

#include <fmt/format.h>

#include <cstdio>
#include <mutex>

namespace NAsyncTrace
{
namespace
{
	// Read once. The variable is set on the command line before anything that
	// traces exists, and re-reading it per event would cost more than the
	// formatting does.
	bool IsEnabledOnce()
	{
		static const bool bEnabled = NGlobal::GetVar( "trace_async_events", 0 ) != 0;
		return bEnabled;
	}

	std::mutex traceMutex;
	FILE *pTraceFile = 0;
	// Monotonic, so that two files can be lined up even where the game time is
	// the same for a run of events, which it is for everything inside one
	// segment.
	long long nSequence = 0;
	int nCurGameTime = -1;

	// Opened on the first event rather than up front, so that a run with the
	// variable set but nothing to say leaves no file behind to be mistaken for
	// one.
	FILE *GetFile()
	{
		if ( pTraceFile == 0 )
		{
			pTraceFile = fopen( "async_events_trace.txt", "w" );
		}
		return pTraceFile;
	}
}

bool IsEnabled()
{
	return IsEnabledOnce();
}

void SetGameTime( const int nGameTime )
{
	if ( !IsEnabledOnce() )
	{
		return;
	}
	std::lock_guard<std::mutex> lock( traceMutex );
	nCurGameTime = nGameTime;
}

void Write( const char *szCategory, const std::string &szWhat )
{
	if ( !IsEnabledOnce() )
	{
		return;
	}
	// Locked because the resource side of this can be reached from the loader
	// thread, even though the hooks placed so far are all on the main one.
	std::lock_guard<std::mutex> lock( traceMutex );
	if ( FILE *f = GetFile() )
	{
		fmt::print( f, "{} t={} {} {}\n", nSequence, nCurGameTime, szCategory, szWhat );
		// Flushed per line. These are read after a run that may have been killed
		// for taking too long, and a buffered tail is worth nothing then.
		fflush( f );
	}
	++nSequence;
}

void WriteResource( const char *szWhat, const char *szResName, const int nID )
{
	if ( !IsEnabledOnce() )
	{
		return;
	}
	Write( "res", fmt::format( "{} {} id={}", szWhat, szResName != 0 ? szResName : "<none>", nID ) );
}
}
