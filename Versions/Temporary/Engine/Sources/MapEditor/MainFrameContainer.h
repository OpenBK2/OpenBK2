#pragma once

#include "MainFrame.h"

class CMainFrameLoggerSink;


class CMainFrameContainer : public IMainFrameContainer
{
	OBJECT_NOCOPY_METHODS( CMainFrameContainer );
	IMainFrame *pMainFrame;
	IWidget *pMainWindow;
	CObj<CMainFrameLoggerSink> pLoggerSink;

public:
	CMainFrameContainer() : pMainFrame( 0 ), pMainWindow( 0 ) {}
	~CMainFrameContainer() {}

	// IMainFrameContainer
	void Set( IMainFrame* _pMainFrame, IWidget* _pMainWindow );
	IMainFrame* Get() { return pMainFrame; }
	IWidget* GetMainWindow() { return pMainWindow; }
};



