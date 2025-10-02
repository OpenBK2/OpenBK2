#if !defined(__MAIN_FRAME__CONTAINER__)
#define __MAIN_FRAME__CONTAINER__
#pragma once

#include "MainFrame.h"

class CMainFrameLoggerSink;


class CMainFrameContainer : public IMainFrameContainer
{
	OBJECT_NOCOPY_METHODS( CMainFrameContainer );
	CMainFrame *pMainFrame;
	CObj<CMainFrameLoggerSink> pLoggerSink;

public:
	CMainFrameContainer() : pMainFrame( 0 ) {}
	~CMainFrameContainer() {}

	// IMainFrameContainer
	void Set( class CMainFrame* _pMainFrame );
	IMainFrame* Get() { return checked_cast<IMainFrame*>( pMainFrame ); }
	SECWorkbook* GetSECWorkbook() { return checked_cast<SECWorkbook*>( pMainFrame ); }
};

#endif // !defined(__MAIN_FRAME__CONTAINER__)

