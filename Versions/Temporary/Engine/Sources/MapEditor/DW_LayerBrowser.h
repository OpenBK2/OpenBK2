#pragma once


class CDWLayerBrowser : public SECControlBar
{
	//CWnd wndContents;

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnSize( unsigned nType, int cx, int cy );

public:
	CDWLayerBrowser();
	virtual ~CDWLayerBrowser();

	DECLARE_MESSAGE_MAP()
};


