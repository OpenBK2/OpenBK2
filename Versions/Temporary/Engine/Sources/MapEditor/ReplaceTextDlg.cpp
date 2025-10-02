// ReplaceTextDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ReplaceTextDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CReplaceTextDlg dialog

IMPLEMENT_DYNAMIC(CReplaceTextDlg, CDialog)
CReplaceTextDlg::CReplaceTextDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CReplaceTextDlg::IDD, pParent)
	, bWholeWord(FALSE)
	, bMatchCase(FALSE)
	, m_szFindWhat(_T(""))
	, m_szReplaceWith(_T(""))
{
}

CReplaceTextDlg::~CReplaceTextDlg()
{
}

void CReplaceTextDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_MATCHWHOLEWORD, bWholeWord);
	DDX_Check(pDX, IDC_MATCHCASE, bMatchCase);
	DDX_Text(pDX, IDC_FINDTEXT, m_szFindWhat);
	DDX_Text(pDX, IDC_REPLACETEXT, m_szReplaceWith);
}

BEGIN_MESSAGE_MAP(CReplaceTextDlg, CDialog)
	ON_BN_CLICKED(ID_FINDNEXT, OnBnClickedFindnext)
	ON_BN_CLICKED(ID_REPLACE, OnBnClickedReplace)
	ON_BN_CLICKED(ID_REPLACEALL, OnBnClickedReplaceall)
END_MESSAGE_MAP()

// CReplaceTextDlg message handlers

void CReplaceTextDlg::OnBnClickedFindnext()
{
	UpdateData();
	if ( IsValid( pHandler ) )
		pHandler->FindNext( (LPCSTR)m_szFindWhat, bWholeWord, bMatchCase );
}

void CReplaceTextDlg::OnBnClickedReplace()
{
	UpdateData();
	if ( IsValid( pHandler ) )
		pHandler->Replace( (LPCSTR)m_szFindWhat, (LPCSTR)m_szReplaceWith, bWholeWord, bMatchCase );
}

void CReplaceTextDlg::OnBnClickedReplaceall()
{
	UpdateData();
	if ( IsValid( pHandler ) )
		pHandler->ReplaceAll( (LPCSTR)m_szFindWhat, (LPCSTR)m_szReplaceWith, bWholeWord, bMatchCase );
}
