#pragma once

#include "Interface_MainFrame.h"
#include "MapEditorLib_export.h"

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
	MAPEDITORLIB_EXPORT void SetLogger( ILoggerSink * pLoggerSink );
	//	
	MAPEDITORLIB_EXPORT ILogger * GetLogger();
//	void FlushLogBuffer();
	//
	MAPEDITORLIB_EXPORT void Log( ELogOutputType eLogOutputType, const char *pszFormat, ... );
	void ClearLog();
}


