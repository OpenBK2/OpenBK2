#pragma once

#define IDC_EMBEDDED_CONTROL 200


class CTreeDockWindow : public SECControlBar
{
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

	CWnd *pwndMainFrame;
public:
	SECTreeCtrl wndTree;

	CTreeDockWindow();
	virtual ~CTreeDockWindow();

	void SetMainFrameWindow( CWnd *_pwndMainFrame );
};


