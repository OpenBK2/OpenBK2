#pragma once

#include "MapEditorLib/Interface_MainFrame.h"

class CMainFrameLoggerSink;

class CMainFrameContainer : public IMainFrameContainer
{
	OBJECT_NOCOPY_METHODS( CMainFrameContainer );
	IMainFrame *pMainFrame;
	IWidget *pMainWindow;
	CObj<CMainFrameLoggerSink> pLoggerSink;

public:
	CMainFrameContainer()
		: pMainFrame( 0 )
		, pMainWindow( 0 )
	{
	}

	~CMainFrameContainer() override {}

	// IMainFrameContainer
	void Set( IMainFrame *_pMainFrame, IWidget *_pMainWindow ) override;

	IMainFrame *Get() override
	{
		return pMainFrame;
	}

	IWidget *GetMainWindow() override
	{
		return pMainWindow;
	}
};
