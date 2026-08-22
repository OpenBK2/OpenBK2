#pragma once

class CDefaultDockingWindow : public SECControlBar
{
	friend class CMainFrame;
	CWnd* pwndContents;

protected:
	afx_msg void OnSize( unsigned nType, int cx, int cy );

public:
	CDefaultDockingWindow() : pwndContents( 0 ) {}
	virtual ~CDefaultDockingWindow() {}

	DECLARE_MESSAGE_MAP()
};


