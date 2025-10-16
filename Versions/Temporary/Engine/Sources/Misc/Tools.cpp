#include "stdafx.h"
#include "Tools.h"

#include "port/stdcall.h"

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
	OutputDebugString( charBuff );
	OutputDebugString( "\n" );
}
