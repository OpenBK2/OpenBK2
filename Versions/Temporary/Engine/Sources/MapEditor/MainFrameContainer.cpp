#include "StdAfx.h"

#include "MainFrameContainer.h"
#include "../MapEditorLib/Interface_Logger.h"



class CMainFrameLoggerSink : public NLog::ILoggerSink
{
	OBJECT_NOCOPY_METHODS(CMainFrameLoggerSink);

	ILogger *pLogger;

	CMainFrameLoggerSink() {}
public:
	CMainFrameLoggerSink( ILogger * _pLogger ) : pLogger(_pLogger) {}

	ILogger * GetLogger() { return pLogger; }
};


void CMainFrameContainer::Set( class CMainFrame* _pMainFrame )
{
	pMainFrame = _pMainFrame;
	pLoggerSink = (pMainFrame ? new CMainFrameLoggerSink(pMainFrame) : 0);
	NLog::SetLogger( pLoggerSink );
}

BASIC_REGISTER_CLASS(CMainFrameLoggerSink);

// basement storage  


