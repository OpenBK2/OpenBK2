
#pragma once

class CDefaultWindow : public CWnd
{
protected:
	virtual BOOL PreCreateWindow( CREATESTRUCT &rCreateStruct );

	afx_msg void OnPaint();

public:
	CDefaultWindow();
	virtual ~CDefaultWindow();

	DECLARE_MESSAGE_MAP()
};


