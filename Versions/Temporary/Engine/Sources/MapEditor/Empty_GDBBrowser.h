#pragma once


class CEmptyGDBBrowser : public CWnd
{
protected:
	virtual BOOL PreCreateWindow( CREATESTRUCT &rCreateStruct );

	afx_msg void OnPaint();

public:
	CEmptyGDBBrowser();
	virtual ~CEmptyGDBBrowser();

	DECLARE_MESSAGE_MAP()
};


