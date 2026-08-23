#include "stdafx.h"

#include "System/FileUtils.h"
#include "System/FilePath.h"
#include "LogSaver.h"
#include "NetLogger.h"
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#include <fmt/format.h>

#ifdef _FINALRELEASE
	#define _DONT_LOG_PACKETS
#endif
//#define _DONT_LOG_PACKETS

void CNetLogger::OpenLogFile( const std::string &szNick )
{
#ifndef _DONT_LOG_PACKETS 
	CloseLogFile( szNick );

	std::string szFullName;
	NFile::GetFullName( &szFullName, "..\\Logs\\" );
	NFile::CreatePath( szFullName.c_str() );
	szFullName += szNick + ".log";
	
	logs[szNick] = fopen( szFullName.c_str(), "a" );
	if ( logs[szNick] == 0 )
		logs.erase( szNick );
#endif
}

void CNetLogger::CloseLogFile( const std::string &szNick )
{
#ifndef _DONT_LOG_PACKETS	
	std::unordered_map<std::string, FILE*>::iterator iter = logs.find( szNick	);
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
	for ( std::unordered_map<std::string, FILE*>::iterator iter = logs.begin(); iter != logs.end(); ++iter )
		fclose( iter->second );
#endif
}

void CNetLogger::Log( const std::string &wszNick, const std::string &szLog )
{
#ifndef _DONT_LOG_PACKETS	
	std::unordered_map<std::string, FILE*>::iterator iter = logs.find( wszNick	);
	if ( logs.find( wszNick ) != logs.end() )
	{
		std::string szStr;
		
		static char buf[1024];
		_strdate( buf );
		szStr = buf;

		_strtime( buf );
		szStr += std::string(" ") + buf;

		struct __timeb64 tstruct;
		_ftime64( &tstruct );
		szStr += fmt::format(".{}\n", tstruct.millitm );

		szStr += "\t" + szLog + "\n";

		FILE *pFile = iter->second;
		if ( pFile )
		{
			fmt::print( pFile, "{}", szStr );
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
static std::string *pszLogStr = 0;

static struct SLogSaverLife
{
	SLogSaverLife()
	{
		pszLogStr = new std::string;
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


