#include "stdafx.h"
#include "Tools.h"

#include "port/stdcall.h"

#include <cstdarg>
#include <cstdio>

#include <boost/predef.h>

namespace
{
	const int BUF_SIZE = 65536;
	char charBuff[BUF_SIZE] = { '\0' };
	// partial initialization force the whole array to be initialized with zeros
}

void PORT_STDCALL DbgTrc( const char *pszFormat, ... )
{
	va_list va;
	va_start( va, pszFormat );
	_vsnprintf( charBuff, BUF_SIZE - 1, pszFormat, va );
	va_end( va );
#if BOOST_OS_WINDOWS
	// the debugger output pane is a Windows concept, and
	// OutputDebugString takes no format, so the newline is a second call
	OutputDebugString( charBuff );
	OutputDebugString( "\n" );
#else
	// stderr is the closest equivalent: unbuffered, and kept apart from
	// whatever the game itself writes to stdout
	std::fputs( charBuff, stderr );
	std::fputc( '\n', stderr );
#endif
}
