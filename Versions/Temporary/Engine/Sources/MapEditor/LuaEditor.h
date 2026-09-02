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
	std::string GetText();
	void AddFunctionNames( const char *pList, int nFuncSet );
	void SetAutoComplete( const std::vector<std::string> &vszKeywords, const std::string &szKeywords );
	void Find();
	void FindNext( const std::string &szText, bool bWholeWord, bool bMatchCase );
	void Replace();
	bool Replace( const std::string &szReplaceWith );
	void ReplaceAll( const std::string &szText, const std::string &szWith, bool bWholeWord, bool bMatchCase );

protected:
	SciFnDirect m_fnScintilla;
	sptr_t m_ptrScintilla;
	CUIntArray	m_pages;
	BOOL m_bShowCalltips;
	CString m_strCallTip;
	std::string szAutoComplete;
	CFindTextDlg findDlg;
	CReplaceTextDlg replaceDlg;
	std::string szLastTextToFind;
	bool bLastMatchCase;
	bool bLastWholeWord;
	std::vector<std::string> vszScriptKeywords;

	sptr_t Sci(int nCmd, uptr_t wParam=0, sptr_t lParam=0);

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



