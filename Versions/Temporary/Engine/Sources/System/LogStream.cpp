#include "stdafx.h"
//#include "Commands.h"
#include "LogStream.h"

#include "port/unicode.h"

// xchar.h is what makes fmt::format( L"..." ) available, so a number
// never has to be formatted narrow and converted back
#include <fmt/format.h>
#include <fmt/xchar.h>

int nID = 0;
bool bConsoleUpdated = false;
std::list<SConsoleLine> consoleLines;

CLogStream csSystem( CONSOLE_STREAM_CONSOLE );	// Ответы на консольные комманды
CLogStream csScript( CONSOLE_STREAM_CONSOLE );//CONSOLE_STREAM_SCRIPT );		// сообщения скрипта
// максимальное кол-во строк в консоле
const int CONSOLE_MAX_SIZE = 256;

// Console stream

CLogStream& CLogStream::operator<< ( const bool &bVal )
{
	bConsoleUpdated = true;
	wsStreamBuffer += bVal ? L"<green>true<white>" : L"<red>false<white>";
	return *this;
}

CLogStream& CLogStream::operator<< ( const std::size_t &nVal )
{
	bConsoleUpdated = true;
	wsStreamBuffer += std::to_wstring( nVal );
	return *this;
}

CLogStream& CLogStream::operator<< ( const int &nVal )
{
	bConsoleUpdated = true;
	wsStreamBuffer += std::to_wstring( nVal );
	return *this;
}

CLogStream& CLogStream::operator<< ( const long &lVal )
{
	bConsoleUpdated = true;
	wsStreamBuffer += std::to_wstring( lVal );
	return *this;
}

CLogStream& CLogStream::operator<< ( const double &dVal )
{
	bConsoleUpdated = true;
	// {:g} is fmt's spelling of printf's %g; to_wstring would give %f, which
	// prints six decimals for every value
	wsStreamBuffer += fmt::format( L"{:g}", dVal );
	return *this;
}

CLogStream& CLogStream::operator<< ( const char* szText )
{
	// same overrun as the two in Logger.cpp: wchar_t[1024] handed to
	// MultiByteToWideChar as its own size, then indexed at the result
	bConsoleUpdated = true;
	*this << UTF8ToWide( szText );

	return *this;
}

CLogStream& CLogStream::operator<< ( const wchar_t* szText ) 
{
	bConsoleUpdated = true;

	*this << std::wstring( szText );

	return *this;
}

CLogStream& CLogStream::operator<< ( const std::wstring &szText )
{
	bConsoleUpdated = true;
	for ( int nTemp = 0; nTemp < szText.length(); nTemp++ )
	{
		if ( szText[nTemp] == L'\n' )
		{
			nID++;
//			AddConsoleLine( SConsoleLine( nID, eType, false, wsStreamBuffer ) );
			DebugTrace( "%s", WideToUTF8( wsStreamBuffer ).c_str() );
			Singleton<IConsoleBuffer>()->Write( nStream, wsStreamBuffer.c_str() );
			wsStreamBuffer.clear();
		}
	/*
		else if ( szText[nTemp] == L'<' )
			wsStreamBuffer.append( L"<lb>" );
		else if ( szText[nTemp] == L'>' )
			wsStreamBuffer.append( L"<rb>" );
		*/
		else if ( szText[nTemp] == L'\t' )
			wsStreamBuffer.append( 1, szText[nTemp] );
		else if ( iswprint( szText[nTemp] ) )
			wsStreamBuffer.append( 1, szText[nTemp] );
	}
	return *this;
}

CLogStream& CLogStream::operator<< ( const EConsoleColor &eColor )
{
	switch( eColor )
	{
		case CC_WHITE:
			wsStreamBuffer.append( L"<color=white>" );
			break;
		case CC_RED:
			wsStreamBuffer.append( L"<color=red>" );
			break;
		case CC_GREEN:
			wsStreamBuffer.append( L"<color=green>" );
			break;
		case CC_BLUE:
			wsStreamBuffer.append( L"<color=blue>" );
			break;
		case CC_PINK:
			wsStreamBuffer.append( L"<color=pink>" );
			break;
		case CC_GREY:
			wsStreamBuffer.append( L"<color=grey>" );
			break;
		case CC_CYAN:
			wsStreamBuffer.append( L"<color=cyan>" );
			break;
		case CC_YELLOW:
			wsStreamBuffer.append( L"<color=yellow>" );
			break;
		case CC_BROWN:
			wsStreamBuffer.append( L"<color=brown>" );
			break;
		case CC_ORANGE:
			wsStreamBuffer.append( L"<color=orange>" );
			break;
	}
	return *this;
}

CLogStream& CLogStream::operator<< ( CLogStream& (*Func)( CLogStream& csStream ) )
{
	return Func( *this );
}

//START_REGISTER(LogStream)
//	REGISTER_VAR_EX( "ui_messages", NGlobal::VarBoolHandler, &bConsoleMessages, 1, true )
//FINISH_REGISTER


