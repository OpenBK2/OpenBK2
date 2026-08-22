#pragma once
// LuaEditor.h : header file
//
#include "Scintilla/Scintilla.h"
#include "ReplaceTextDlg.h"

#include <cstdint>

// CLuaEditor window

class CLuaEditor : public CWnd
{
// Construction
public:
	CLuaEditor();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLuaEditor)
	//}}AFX_VIRTUAL

// Implementation
public:
	void InitScintilla();

	void SetLuaLexer();
	void SetKeywordColor( int nKeywordSet, uint32_t dwColor );/*D. Belyaev*/

	BOOL PreparePrint(CDC* pDC, CPrintInfo* pInfo);
	void PrintPage(CDC* pDC, CPrintInfo* pInfo);
	void EndPrint(CDC* pDC, CPrintInfo* pInfo);
	void SetStackTraceLevel(int nLevel);
	void SetCallStackMargins();
	int GetCurrentLine();
	void GotoLastLine();
	CString GetLine(int nLine);
	void ClearAll();
	void AddText(const char* szText);
	void SetReadOnly(BOOL bReadOnly);
	void SetEditorMargins();
	void GrabFocus();
	void Copy();
	void Paste();
	BOOL CanPaste();
	void Clear();
	void Cut();
	BOOL CanCutOrClear();
	void SelectAll();
	void Redo();
	BOOL CanRedo();
	void Undo();
	BOOL CanUndo();
	int LineFromPoint(CPoint& pt);
	BOOL Create(CWnd* pParentWnd, unsigned nCtrlID);
	void AutoComplete();
	void NewLineIndent();
	virtual ~CLuaEditor();

// thread safe functions
	//void tsAddText(const char *szMsg) {	AddText(szMsg); };
	//void tsSetReadOnly(BOOL bReadOnly) { SetReadOnly(bReadOnly); };

	void Print(bool showDialog);
	CharacterRange GetSelection();
	string GetText();
	void AddFunctionNames( const char *pList, int nFuncSet );
	void SetAutoComplete( const vector<string> &vszKeywords, const string &szKeywords );
	void Find();
	void FindNext( const string &szText, bool bWholeWord, bool bMatchCase );
	void Replace();
	bool Replace( const string &szReplaceWith );
	void ReplaceAll( const string &szText, const string &szWith, bool bWholeWord, bool bMatchCase );

protected:
	int (*m_fnScintilla)(void*,int,int,int);
	void *m_ptrScintilla;
	CUIntArray	m_pages;
	BOOL m_bShowCalltips;
	CString m_strCallTip;
	string szAutoComplete;
	CFindTextDlg findDlg;
	CReplaceTextDlg replaceDlg;
	string szLastTextToFind;
	bool bLastMatchCase;
	bool bLastWholeWord;
	vector<string> vszScriptKeywords;

	int Sci(int nCmd, int wParam=0, int lParam=0);

	// Generated message map functions
protected:
	//{{AFX_MSG(CLuaEditor)
	afx_msg void OnMouseMove(unsigned nFlags, CPoint point);
	afx_msg void OnKeyDown(unsigned nChar, unsigned nRepCnt, unsigned nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.



