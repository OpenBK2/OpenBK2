#pragma once

#include "System_export.h"


enum EConsoleColor
{
	CC_WHITE,
	CC_RED,
	CC_GREEN,
	CC_BLUE,
	CC_PINK,
	CC_GREY,
	CC_CYAN,
	CC_YELLOW,
	CC_BROWN,
	CC_ORANGE
};

class SYSTEM_EXPORT CLogStream
{
	wstring wsStreamBuffer;
	const int nStream;
public:
	CLogStream( const int _nStream ): nStream( _nStream )	{}
		
	CLogStream& operator<< ( const int &n );
	CLogStream& operator<< ( const long &l );
	CLogStream& operator<< ( const double &d );
	CLogStream& operator<< ( const bool &bVal );
	CLogStream& operator<< ( const char* szText );
	CLogStream& operator<< ( const wchar_t* szText );
	CLogStream& operator<< ( const wstring &szText );
	CLogStream& operator<< ( const string &szText ) { operator<<(szText.c_str()); return *this; }
	CLogStream& operator<< ( const EConsoleColor &eColor );
	
	CLogStream& operator<< ( CLogStream& (*Func)( CLogStream& csStream ) );
};

struct SConsoleLine
{
	int nID;
	const int nStream;

	bool bCommand;
	wstring szText;
	
	SConsoleLine( int _nID, const int _nStream, bool _bCommand, const wstring &_szText )
		: nID( _nID ), nStream( _nStream ), bCommand( _bCommand ), szText( _szText ) {}
};

inline CLogStream& endl( CLogStream& sStream )
{
	sStream << L"\n";
	return sStream;
}

EXTERNVAR SYSTEM_EXPORT CLogStream csSystem, csScript;

