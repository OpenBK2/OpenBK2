#pragma once



class CPCEditorButton : public CButton
{
	std::string szText;
	CWnd *pwndTargetWindow;
	CWnd *pwndNextWindow;
	CWnd *pwndPreviousWindow;

protected:
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void OnClicked();

public:
	CPCEditorButton();	
	virtual ~CPCEditorButton();
	//
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	//
	void SetTargetWindow( CWnd *_pwndTargetWindow ) { pwndTargetWindow = _pwndTargetWindow; }
	void SetNextWindow( CWnd *_pwndNextWindow ) { pwndNextWindow = _pwndNextWindow; }
	void SetPreviousWindow( CWnd *_pwndPreviousWindow ) { pwndPreviousWindow = _pwndPreviousWindow; }
	void SetCaption( const std::string &rszText );
	//
	DECLARE_MESSAGE_MAP()
};
typedef std::vector<CPCEditorButton*> CPCEditorButtonList;


