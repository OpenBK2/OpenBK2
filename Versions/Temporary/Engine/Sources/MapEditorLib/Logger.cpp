#include "stdafx.h"

#include "Interface_Logger.h"

#include <cstdarg>

namespace NLog
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CLogger : public ILogger
	{
		//
		CLogBufferList logBufferList;

	public:
		//
		void FlushLogBuffer( ILogger *pLogger )
		{
			for ( CLogBufferList::const_iterator itLogBuffer = logBufferList.begin(); itLogBuffer != logBufferList.end(); ++itLogBuffer )
			{
				pLogger->Log( itLogBuffer->eLogOutputType, itLogBuffer->szText );
			}
			logBufferList.clear();
		}

		// ILogger
		void Log( ELogOutputType eLogOutputType, const std::string &szText )
		{
			CLogBufferList::iterator posLogBuffer = logBufferList.insert( logBufferList.end(), SLogBuffer() );
			posLogBuffer->eLogOutputType = eLogOutputType;
			posLogBuffer->szText = szText;
			if ( logBufferList.size() > 1024 )
			{
				logBufferList.pop_front();
			}
		}
		void ClearLog()
		{
			logBufferList.clear();
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CDefaultLoggerSink : public ILoggerSink
	{
		OBJECT_NOCOPY_METHODS(CDefaultLoggerSink);

		CLogger logger;

	public:
		CDefaultLoggerSink() {}

		ILogger * GetLogger()
		{
			return &logger;
		}

		void FlushTo( ILoggerSink *pLoggerSink )
		{
			logger.FlushLogBuffer(pLoggerSink->GetLogger());
		}
	};
}

namespace NLog
{


CObj<CDefaultLoggerSink> pDefaultLoggerSink;
CPtr<ILoggerSink> pLoggerSink;


void SetLogger( ILoggerSink * pSink )
{
	if ( pLoggerSink == pDefaultLoggerSink && IsValid(pDefaultLoggerSink) )
	{
		pDefaultLoggerSink->FlushTo( pSink );
	}
	pLoggerSink = pSink;
}


ILogger *GetLogger()
{
//	if ( Singleton<IMainFrameContainer>() && Singleton<IMainFrameContainer>()->Get() )
//	{
//		return Singleton<IMainFrameContainer>()->Get();
//	}
//	else
//	{
//		return &loggerBuffer;
//	}
	if ( !IsValid(pDefaultLoggerSink) )
	{
		pDefaultLoggerSink = new CDefaultLoggerSink;
	}
	if ( !IsValid(pLoggerSink) )
	{
		pLoggerSink = pDefaultLoggerSink;
	}
	return pLoggerSink->GetLogger();
}

//void FlushLogBuffer()
//{
//	if ( Singleton<IMainFrameContainer>()->Get() )
//	{
//		loggerBuffer.FlushLogBuffer( Singleton<IMainFrameContainer>()->Get() );
//	}
//}

void Log( ELogOutputType eLogOutputType, const char *pszFormat, ... )
{
	static char buff[1024];

	va_list va;
	va_start( va, pszFormat );
	vsprintf( buff, pszFormat, va );
	va_end( va );
	GetLogger()->Log( eLogOutputType, buff );
}


void ClearLog()
{
//	if ( Singleton<IMainFrameContainer>() && Singleton<IMainFrameContainer>()->Get() )
//	{
//		Singleton<IMainFrameContainer>()->Get()->ClearLog();
//	}
//	else
//	{
//		loggerBuffer.ClearLog();
//	}
	GetLogger()->ClearLog();
}


}

// basement storage  


