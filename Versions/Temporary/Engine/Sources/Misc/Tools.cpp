#include "stdafx.h"
#include "Tools.h"

#include "port/stdcall.h"

#include <cstdio>
#include <string>

#include <boost/predef.h>
#include <fmt/printf.h>

void PORT_STDCALL DbgTrcArgs( fmt::string_view fmtStr, fmt::printf_args args )
{
	// Sized to the message. The 64 KB static buffer this replaces was shared between
	// threads and truncated anything longer without saying so.
	//
	// A trace call must not throw. The format string is still only checked at run
	// time, so a specifier with no argument behind it reaches fmt and raises
	// format_error, where the old printf would have read past the arguments and
	// printed nonsense. Report the bad call rather than propagating either.
	std::string szLine;
	try
	{
		szLine = fmt::vsprintf( fmtStr, args );
	}
	catch ( const fmt::format_error &e )
	{
		szLine = "DbgTrc: bad format \"";
		szLine.append( fmtStr.data(), fmtStr.size() );
		szLine += "\": ";
		szLine += e.what();
	}
#if BOOST_OS_WINDOWS
	// the debugger output pane is a Windows concept, and
	// OutputDebugString takes no format, so the newline is a second call
	OutputDebugString( szLine.c_str() );
	OutputDebugString( "\n" );
#else
	// stderr is the closest equivalent: unbuffered, and kept apart from
	// whatever the game itself writes to stdout
	std::fputs( szLine.c_str(), stderr );
	std::fputc( '\n', stderr );
#endif
}
