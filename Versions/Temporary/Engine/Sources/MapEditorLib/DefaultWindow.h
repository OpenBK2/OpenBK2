#if !defined(__DEFAULT_WINDOW__)
#define __DEFAULT_WINDOW__
#pragma once

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


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

#endif // !defined(__DEFAULT_WINDOW__)
