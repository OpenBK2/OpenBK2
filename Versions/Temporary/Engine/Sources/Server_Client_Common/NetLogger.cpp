#include "stdafx.h"

#include "System/FileUtils.h"
#include "System/FilePath.h"
#include "LogSaver.h"
#include "NetLogger.h"
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#ifdef _FINALRELEASE
	#define _DONT_LOG_PACKETS
#endif
//#define _DONT_LOG_PACKETS

void CNetLogger::OpenLogFile( const string &szNick )
{
#ifndef _DONT_LOG_PACKETS 
	CloseLogFile( szNick );

	string szFullName;
	NFile::GetFullName( &szFullName, "..\\Logs\\" );
	NFile::CreatePath( szFullName.c_str() );
	szFullName += szNick + ".log";
	
	logs[szNick] = fopen( szFullName.c_str(), "a" );
	if ( logs[szNick] == 0 )
		logs.erase( szNick );
#endif
}

void CNetLogger::CloseLogFile( const string &szNick )
{
#ifndef _DONT_LOG_PACKETS	
	hash_map<string, FILE*>::iterator iter = logs.find( szNick	);
	if ( iter != logs.end() )
	{
		fclose( iter->second );
		logs.erase( szNick );
	}
#endif
}

CNetLogger::~CNetLogger()
{
#ifndef _DONT_LOG_PACKETS	
	for ( hash_map<string, FILE*>::iterator iter = logs.begin(); iter != logs.end(); ++iter )
		fclose( iter->second );
#endif
}

void CNetLogger::Log( const string &wszNick, const string &szLog )
{
#ifndef _DONT_LOG_PACKETS	
	hash_map<string, FILE*>::iterator iter = logs.find( wszNick	);
	if ( logs.find( wszNick ) != logs.end() )
	{
		string szStr;
		
		static char buf[1024];
		_strdate( buf );
		szStr = buf;

		_strtime( buf );
		szStr += string(" ") + buf;

		struct __timeb64 tstruct;
		_ftime64( &tstruct );
		szStr += StrFmt(".%d\n", tstruct.millitm );

		szStr += "\t" + szLog + "\n";

		FILE *pFile = iter->second;
		if ( pFile )
		{
			fprintf( pFile, szStr.c_str() );
			fflush( pFile );
		}
	}
#endif
}

static CNetLogger *pLogger = 0;

static struct SNetLoggerLife
{
	SNetLoggerLife()
	{
		pLogger = new CNetLogger();
	}

	~SNetLoggerLife()
	{
		delete pLogger;
		pLogger = 0;
	}
} netLoggerLife;

CNetLogger* GetNetLogger()
{
	return pLogger;
}

static IBinSaver *pLogSaver = 0;
static string *pszLogStr = 0;

static struct SLogSaverLife
{
	SLogSaverLife()
	{
		pszLogStr = new string;
		pLogSaver = CreateLogSaver( pszLogStr );
	}

	~SLogSaverLife()
	{
		delete pLogSaver;
		delete pszLogStr;
	}
} logSaverLife;

const char* GetPacketInfo( CNetPacket *pPacket )
{
#ifndef _DONT_LOG_PACKETS
	CPtr<CNetPacket> pNetPacket = pPacket;
	*pszLogStr = "";
	pLogSaver->Add( 1, &pNetPacket );

	return pszLogStr->c_str();
#else
	return "";
#endif
}


