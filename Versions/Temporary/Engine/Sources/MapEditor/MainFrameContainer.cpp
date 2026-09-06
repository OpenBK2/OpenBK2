#include "stdafx.h"

#include "MainFrameContainer.h"
#include "MapEditorLib/Interface_Logger.h"

#include "MapEditor_export.h"



class CMainFrameLoggerSink : public NLog::ILoggerSink
{
	OBJECT_NOCOPY_METHODS(CMainFrameLoggerSink);

	ILogger *pLogger;

	CMainFrameLoggerSink() {}
public:
	CMainFrameLoggerSink( ILogger * _pLogger ) : pLogger(_pLogger) {}

	ILogger * GetLogger() { return pLogger; }
};


void CMainFrameContainer::Set( IMainFrame* _pMainFrame, IWidget* _pMainWindow )
{
	pMainFrame = _pMainFrame;
	pMainWindow = _pMainWindow;
	pLoggerSink = (pMainFrame ? new CMainFrameLoggerSink(pMainFrame) : 0);
	NLog::SetLogger( pLoggerSink );
}

BASIC_REGISTER_CLASS(MAPEDITOR, CMainFrameLoggerSink);

// basement storage  


