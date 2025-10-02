#pragma once
#include "resourceDefines.h"

class IFindNextEvent: public CObjectBase
{
public:
	virtual void FindNext( const string &szText, bool bWholeWord, bool bMatchCase ) {};
	virtual void Replace( const string &szText, const string &szReplaceWith, bool bWholeWord, bool bMatchCase ) {};
	virtual void ReplaceAll( const string &szText, const string &szReplaceWith, bool bWholeWord, bool bMatchCase ) {};
};


class CLuaEditor;
class CFindNext: public IFindNextEvent
{
	OBJECT_NOCOPY_METHODS(CFindNext)
	CLuaEditor *pEditor;
public:
	CFindNext( CLuaEditor *_pEditor = 0 ): pEditor(_pEditor) {}

	virtual void FindNext( const string &szText, bool bWholeWord, bool bMatchCase );
	virtual void Replace( const string &szText, const string &szReplaceWith, bool bWholeWord, bool bMatchCase );
	virtual void ReplaceAll( const string &szText, const string &szWith, bool bWholeWord, bool bMatchCase );
};

// CFindTextDlg dialog
class CFindTextDlg : public CDialog
{
	DECLARE_DYNAMIC(CFindTextDlg)

public:
	CFindTextDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFindTextDlg();

	void SetHandler( IFindNextEvent *_pHandler ) { pHandler = _pHandler; }
// Dialog Data
	enum { IDD = IDD_FINDTEXT };

protected:
	CString m_szText;
	BOOL m_bWholeWord;
	BOOL m_bCase;
	CObj<IFindNextEvent> pHandler;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnCancel();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedFindNext();
};
