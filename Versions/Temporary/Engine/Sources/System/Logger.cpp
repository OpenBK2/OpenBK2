#include "stdafx.h"
#include "Logger.h"

#include "port/unicode.h"

#include <boost/predef.h>

#include <fmt/format.h>
#include <fmt/xchar.h>

namespace NLog
{

// ************************************************************************************************************************ //
// **
// ** dumpers
// **
// **
// **
// ************************************************************************************************************************ //

class CFileDumper : public ILogDumper
{
	OBJECT_BASIC_METHODS( CFileDumper );
	//
	const std::string szFullFileName;
	CFileDumper() {}
public:
	CFileDumper( const std::string &_szFullFileName ): szFullFileName( _szFullFileName ) {}
	void Dump( const std::wstring &wszString )
	{
		if ( FILE *f = fopen(szFullFileName.c_str(), "a") )
		{
			// the buffer this used to fill was char[1024] and was passed to
			// WideCharToMultiByte as its own size, so a line of exactly 1024
			// bytes put the terminator one past the end. The %s is not
			// decoration either: the line was the format string.
			const std::string szLine = WideToUTF8( wszString );
			fprintf( f, "%s", szLine.c_str() );
			fclose( f );
		}
	}
};
ILogDumper *CreateFileDumper( const std::string &szFullFileName ) { return new CFileDumper( szFullFileName ); }

class CDebugDumper : public ILogDumper
{
	OBJECT_BASIC_METHODS( CDebugDumper );
	void Dump( const std::wstring &wszString )
	{
		// no conversion at all now, and no buffer to overrun: the debugger
		// takes wide text directly
#if BOOST_OS_WINDOWS
		if ( wszString.empty() || wszString[wszString.size() - 1] != L'\n' )
		{
			OutputDebugStringW( ( wszString + L'\n' ).c_str() );
		}
		else
		{
			OutputDebugStringW( wszString.c_str() );
		}
#else
		// nothing to attach a debug output stream to, so stderr it is, which
		// is where DbgTrc already goes
		const std::string szLine = WideToUTF8( wszString );
		fprintf( stderr, "%s", szLine.c_str() );
		if ( szLine.empty() || szLine[szLine.size() - 1] != '\n' )
		{
			fputc( '\n', stderr );
		}
#endif
	}
};
ILogDumper *CreateDebugDumper() { return new CDebugDumper(); }

ILogDumper *CreateAssertDumper()
{
	return 0;
}

// ************************************************************************************************************************ //
// **
// ** logger 
// **
// **
// **
// ************************************************************************************************************************ //

CLogger &CLogger::operator<<( const bool bVal )
{
	wszLogBuffer += bVal ? L"true" : L"false";
	return *this;
}

CLogger &CLogger::operator<<( const int nVal )
{
	wszLogBuffer += std::to_wstring( nVal );
	return *this;
}

CLogger &CLogger::operator<<( const long lVal )
{
	wszLogBuffer += std::to_wstring( lVal );
	return *this;
}

CLogger &CLogger::operator<<( const double fVal )
{
	// {:g} is fmt's spelling of printf's %g; to_wstring would give %f, which
	// prints six decimals for every value
	wszLogBuffer += fmt::format( L"{:g}", fVal );
	return *this;
}

CLogger &CLogger::operator<<( const char cVal )
{
	wszLogBuffer += UTF8ToWide( std::string( 1, cVal ) );
	return *this;
}
CLogger &CLogger::operator<<( const wchar_t wcVal )
{
	wszLogBuffer += wcVal;
	return *this;
}

CLogger &CLogger::operator<<( const char *pszText )
{
	// the wchar_t[1024] this replaces truncated any longer line, and wrote
	// its terminator one element past the end when the line was exactly 1024
	wszLogBuffer += UTF8ToWide( pszText );

	return *this;
}

CLogger &CLogger::operator<<( const std::string &szText )
{
	return ( *this << szText.c_str() );
}

CLogger &CLogger::operator<<( const wchar_t *pwszText ) 
{
	wszLogBuffer += pwszText;
	return *this;
}

CLogger& CLogger::operator<<( const std::wstring &wszText )
{
	wszLogBuffer += wszText;
	return *this;
}

std::string CLogger::GetLoggerFullName() const
{
	std::string szName = GetLoggerLocalName();
	const CLogger *pParent = this;
	while ( pParent = pParent->GetParent() )
		szName = pParent->GetLoggerLocalName() + "." + szName;
	return szName;
}

CLogger &CLogger::Dump()
{
	if ( !wszLogBuffer.empty() )
	{
		if ( Dump(GetLoggerFullName(), wszLogBuffer) )
			wszLogBuffer.clear();
	}
	return *this;
}

bool CLogger::Dump( const std::string &szLoggerFullName, const std::wstring &wszString )
{
	for ( CDumpersList::iterator it = dumpers.begin(); it != dumpers.end(); ++it )
	{
		if ( (*it) )
			(*it)->Dump( wszString );
	}
	bool bRes = !dumpers.empty();
	if ( pParent )
	{
		bool bRes1 = pParent->Dump( szLoggerFullName, wszString );
		bRes = bRes1 || bRes;
	}
	return bRes;
}

}

NLog::CLogger logDebug( "Debug", 0 );
NLog::CLogger logInfo( "Info", 0 );
NLog::CLogger logNotice( "Notice", 0 );
NLog::CLogger logWarning( "Warning", 0 );
NLog::CLogger logError( "Error", 0 );
NLog::CLogger logCritical( "Critical", 0 );
NLog::CLogger logAlert( "Alert", 0 );
NLog::CLogger logEmergency( "Emergency", 0 );

