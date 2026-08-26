#include "stdafx.h"

#include "System/FileUtils.h"
#include "System/FilePath.h"
#include "LogSaver.h"
#include "NetLogger.h"
#include "port/time.h"

#include <chrono>
#include <ctime>

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
		
		// _strdate wrote "MM/DD/YY", _strtime "HH:MM:SS", and _ftime64 supplied the
		// milliseconds. One reading of the clock serves all three here where the
		// original took three, so the stamp can no longer straddle a second boundary
		// and carry the seconds of one with the milliseconds of the next.
		const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		std::tm tmLocal;
		if ( GetLocalTime( &tmLocal, std::chrono::system_clock::to_time_t( now ) ) )
		{
			char buf[32];
			if ( std::strftime( buf, sizeof( buf ), "%m/%d/%y %H:%M:%S", &tmLocal ) != 0 )
			{
				szStr = buf;
			}
		}

		const std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch() ) % std::chrono::seconds( 1 );
		szStr += fmt::format( ".{}\n", ms.count() );

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


