#include "stdafx.h"

#include "Logger.h"

CLogger theLogger;

CLogger::CLogger()
: bCheckReferences( false ), pStream( 0 )
{
}

CLogger::~CLogger()
{
	delete pStream;
}

void CLogger::WriteLog( const std::string &szLog, bool bAppendNL )
{
	std::string szResult = GetStackTrace() + szLog;
	if ( bAppendNL )
	{
		szResult += "\n";
	}
	printf( szResult.c_str() );
	DbgTrcRaw( szResult.c_str() );
	memoryStream.Write( szResult.c_str(), szResult.length() );
	if ( pStream != 0 )
	{
		pStream->Write( szResult.c_str(), szResult.length() );
		pStream->Flush();
	}
}

void CLogger::DumpEntireLog( CDataStream *pOutStream )
{
	memoryStream.Seek( 0 );
	memoryStream.ReadTo( pOutStream, memoryStream.GetSize() );
	memoryStream.Seek( memoryStream.GetSize() );
}

void CLogger::Finalize()
{
	delete pStream;
	pStream = 0;
}

void CLogger::PushStack( const std::string &szLevel )
{
	stkTrace.push_back( szLevel );
}

void CLogger::PopStack()
{
	stkTrace.pop_back();
}

std::string CLogger::GetStackTrace() const
{
	std::string szResult = "";
	for ( std::list<std::string>::const_iterator it = stkTrace.begin(); it != stkTrace.end(); ++it )
		szResult += *it + ":";
	return szResult;
}

void CLogger::SetReferenceChecking( bool bCheck )
{
	bCheckReferences = bCheck;
	if ( bCheck )
		WriteLog( "Reference checking on." );
	else
		WriteLog( "Reference checking off." );
}

void CLogger::SetLogStream( CDataStream *_pStream )
{
	pStream = _pStream;
}


