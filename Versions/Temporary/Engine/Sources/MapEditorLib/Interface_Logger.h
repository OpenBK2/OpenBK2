#pragma once

#include "Interface_MainFrame.h"

namespace NLog
{
	struct SLogBuffer
	{
		ELogOutputType eLogOutputType;
		std::string szText;
	};
	typedef std::list<SLogBuffer> CLogBufferList;
	//
	struct ILoggerSink : public CObjectBase
	{
		virtual ILogger * GetLogger() = 0;
	};
	void SetLogger( ILoggerSink * pLoggerSink );
	//	
	ILogger * GetLogger();
//	void FlushLogBuffer();
	//
	void Log( ELogOutputType eLogOutputType, const char *pszFormat, ... );
	void ClearLog();
}


