#pragma once

inline void WriteMSG( const char* pszFormat, ... )
{
	static char buff[1024];

	va_list va;
	va_start( va, pszFormat );
	vsprintf( buff, pszFormat, va );
	va_end( va );

	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, buff );
}


