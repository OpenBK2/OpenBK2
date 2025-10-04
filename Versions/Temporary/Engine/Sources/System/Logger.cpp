#include "stdafx.h"
#include "Logger.h"

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
	const string szFullFileName;
	CFileDumper() {}
public:
	CFileDumper( const string &_szFullFileName ): szFullFileName( _szFullFileName ) {}
	void Dump( const wstring &wszString )
	{
		if ( FILE *f = fopen(szFullFileName.c_str(), "a") )
		{
			char szBuffer[1024];
			int nLength = WideCharToMultiByte( GetACP(), 0, wszString.c_str(), wszString.length(), szBuffer, 1024, 0, 0 );
			if ( nLength > 0 )
			{
				szBuffer[nLength] = 0;
				fprintf( f, szBuffer );
			}
			fclose( f );
		}
	}
};
ILogDumper *CreateFileDumper( const string &szFullFileName ) { return new CFileDumper( szFullFileName ); }

class CDebugDumper : public ILogDumper
{
	OBJECT_BASIC_METHODS( CDebugDumper );
	void Dump( const wstring &wszString )
	{
		char szBuffer[1024];
		int nLength = WideCharToMultiByte( GetACP(), 0, wszString.c_str(), wszString.length(), szBuffer, 1024, 0, 0 );
		if ( nLength > 0 )
		{
			if ( szBuffer[nLength - 1] != '\n' )
				szBuffer[nLength++] = '\n';
			szBuffer[nLength] = 0;
			OutputDebugString( szBuffer );
		}
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
	wchar_t wszBuffer[1024];
	swprintf( wszBuffer, L"%d", nVal );
	wszLogBuffer += wszBuffer;
	return *this;
}

CLogger &CLogger::operator<<( const long lVal )
{
	wchar_t wszBuffer[1024];
	swprintf( wszBuffer, L"%d", lVal );
	wszLogBuffer += wszBuffer;
	return *this;
}

CLogger &CLogger::operator<<( const double fVal )
{
	wchar_t wszBuffer[1024];
	swprintf( wszBuffer, L"%g", fVal );
	wszLogBuffer += wszBuffer;
	return *this;
}

CLogger &CLogger::operator<<( const char cVal )
{
	wchar_t wszText[8];
	int nLength = MultiByteToWideChar( GetACP(), 0, &cVal, 1, wszText, 8 );
	if ( nLength > 0 )
	{
		wszText[nLength] = 0;
		wszLogBuffer += wszText;
	}
	return *this;
}
CLogger &CLogger::operator<<( const wchar_t wcVal )
{
	wszLogBuffer += wcVal;
	return *this;
}

CLogger &CLogger::operator<<( const char *pszText )
{
	int nLen = 0;
	wchar_t wszText[1024];
	nLen = MultiByteToWideChar( GetACP(), 0, pszText, strlen(pszText), wszText, 1024 );

	if ( nLen > 0 )
	{
		wszText[nLen] = 0;
		wszLogBuffer += wszText;
	}

	return *this;
}

CLogger &CLogger::operator<<( const string &szText )
{
	return ( *this << szText.c_str() );
}

CLogger &CLogger::operator<<( const wchar_t *pwszText ) 
{
	wszLogBuffer += pwszText;
	return *this;
}

CLogger& CLogger::operator<<( const wstring &wszText )
{
	wszLogBuffer += wszText;
	return *this;
}

string CLogger::GetLoggerFullName() const
{
	string szName = GetLoggerLocalName();
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

bool CLogger::Dump( const string &szLoggerFullName, const wstring &wszString )
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

