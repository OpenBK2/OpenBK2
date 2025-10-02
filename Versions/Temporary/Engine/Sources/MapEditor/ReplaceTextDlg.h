#pragma once
#include "FindTextDlg.h"


// CReplaceTextDlg dialog

class CReplaceTextDlg : public CDialog
{
	DECLARE_DYNAMIC(CReplaceTextDlg)

public:
	CReplaceTextDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CReplaceTextDlg();

	void SetHandler( IFindNextEvent *_pHandler ) { pHandler = _pHandler; }

// Dialog Data
	enum { IDD = IDD_REPLACETEXT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	BOOL bWholeWord;
	BOOL bMatchCase;
	CString m_szFindWhat;
	CString m_szReplaceWith;
	CObj<IFindNextEvent> pHandler;
public:
	afx_msg void OnBnClickedFindnext();
	afx_msg void OnBnClickedReplace();
	afx_msg void OnBnClickedReplaceall();
};

