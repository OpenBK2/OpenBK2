#pragma once


class CCommandsBase;

class CConsole : public CWnd
{
	int (*m_fnScintilla)(void*,int,int,int);
	void *m_ptrScintilla;
	CUIntArray m_pages;
	BOOL m_bShowCalltips;
	CString m_strCallTip;
	string szAutoComplete;
	vector<string> vszScriptKeywords;
	int nConsoleSequenceID;

	CPtr<CCommandsBase> pCommands;

	//
	int Sci(int nCmd, int wParam = 0, int lParam = 0 );
	void SetStyles();
	void InitScintilla();
	void AddText( const string &szText );
public:
	CConsole( CCommandsBase *pCommands, const string &szWindowName );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConsole)
	//}}AFX_VIRTUAL

	void ClearAll();
	void AutoComplete();
	void SetAutoComplete( const vector<string> &szKeywords );

	void Segment();

	//{{AFX_MSG(CConsole)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


