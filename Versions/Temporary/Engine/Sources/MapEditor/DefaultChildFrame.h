#if !defined(__DEFAULT_CHILD_FRAME__)
#define __DEFAULT_CHILD_FRAME__
#pragma once


class CMainFrame;
class CDefaultChildFrame : public SECWorksheet
{
	friend class CMainFrame;
	DECLARE_DYNCREATE(CDefaultChildFrame)

	CWnd *pwndContents;

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT &rCreateStruct );
	virtual BOOL OnCmdMsg( UINT nID, int nCode, void *pExtra, AFX_CMDHANDLERINFO *pHandlerInfo );

public:
	CDefaultChildFrame() : pwndContents( 0 ) {}
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__DEFAULT_CHILD_FRAME__)

