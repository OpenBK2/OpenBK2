// FindTextDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FindTextDlg.h"

// CFindTextDlg dialog

IMPLEMENT_DYNAMIC(CFindTextDlg, CDialog)
CFindTextDlg::CFindTextDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFindTextDlg::IDD, pParent)
	, m_szText(_T(""))
	, m_bWholeWord(FALSE)
	, m_bCase(FALSE)
{
}

CFindTextDlg::~CFindTextDlg()
{
}

void CFindTextDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FINDTEXT, m_szText);
	DDX_Check(pDX, IDC_MATCHWHOLEWORD, m_bWholeWord);
	DDX_Check(pDX, IDC_MATCHCASE, m_bCase);
}


BEGIN_MESSAGE_MAP(CFindTextDlg, CDialog)
	ON_BN_CLICKED(ID_FINDNEXT, OnBnClickedFindNext)
END_MESSAGE_MAP()


// CFindTextDlg message handlers

void CFindTextDlg::OnBnClickedFindNext()
{
	UpdateData();
	if ( IsValid( pHandler ) )
		pHandler->FindNext( (LPCSTR)m_szText, m_bWholeWord, m_bCase );
}

void CFindTextDlg::OnCancel()
{
	ShowWindow( SW_HIDE );
}

