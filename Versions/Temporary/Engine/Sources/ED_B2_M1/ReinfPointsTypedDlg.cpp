#include "stdafx.h"
#include "MapEditorLib/MfcWidget.h"
#include <fmt/format.h>

#include "ReinfPointsTypedDlg.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "ReinfPointsDialogs.h"

//
//	CReinfPointsTypedDlg dialog
//

IMPLEMENT_DYNAMIC( CReinfPointsTypedDlg, CResizeDialog )
CReinfPointsTypedDlg::CReinfPointsTypedDlg( CWnd *pParentWindow, CReinfPointsState::CTypedTemplateType *_pTypedTemplateDlgData, 	CMapInfoEditor *_pMapInfoEditor, int _nCurrentPlayer, int _nCurrentReinfPt )
	: CResizeDialog( CReinfPointsTypedDlg::IDD, pParentWindow ),
	pTypedTemplateDlgData( _pTypedTemplateDlgData ),
	pMapInfoEditor( _pMapInfoEditor ),
	nCurrentPlayer( _nCurrentPlayer ),
	nCurrentReinfPt( _nCurrentReinfPt )
{
}

void CReinfPointsTypedDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizeDialog::DoDataExchange(pDX);
	DDX_Control( pDX, IDC_LIST_REINF_POINTS_TYPED_TEMPLATE, lcTypedTempl );
}

BOOL CReinfPointsTypedDlg::OnInitDialog()
{
	CResizeDialog::OnInitDialog();
	nSelectedTemplate = -1;
	CRect r;
	lcTypedTempl.ModifyStyle( 0, LVS_ALIGNLEFT | LVS_REPORT, 0 ); 
	lcTypedTempl.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP ); 
	lcTypedTempl.EnableToolTips( TRUE );

	lcTypedTempl.InsertColumn( 0, RCSTR("ID"), LVCFMT_LEFT, 30 );
	lcTypedTempl.InsertColumn( 1, RCSTR("Type"), LVCFMT_LEFT, 200 );
	lcTypedTempl.InsertColumn( 2, RCSTR("Template"), LVCFMT_LEFT, 200 );
	//
	SetDialogData();
	return TRUE;
}

BEGIN_MESSAGE_MAP(CReinfPointsTypedDlg, CResizeDialog)
	ON_NOTIFY(NM_CLICK, IDC_LIST_REINF_POINTS_TYPED_TEMPLATE, OnNMClickListReinfPointsTypedTemplate)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_REINF_TYPED_ADD, OnBnClickedTypedAdd)
	ON_BN_CLICKED(IDC_TYPED_REMOVE, OnBnClickedTypedRemove)
END_MESSAGE_MAP()

void CReinfPointsTypedDlg::OnBnClickedTypedAdd()
{
	// The node, the add dialog and the rollback if it is cancelled are shared
	// with the wx dialog; see ReinfPointsDialogs.cpp. The add dialog's owner is
	// the main frame, as it was when this opened it directly.
	if ( NReinfPointsTemplates::Add( Singleton<IMainFrameContainer>()->GetMainWindow(), pTypedTemplateDlgData,
																	 pMapInfoEditor, nCurrentPlayer, nCurrentReinfPt ) )
	{
		SetDialogData();
	}
}

void CReinfPointsTypedDlg::SetDialogData()
{
	lcTypedTempl.DeleteAllItems();
	for ( int i = 0; i < pTypedTemplateDlgData->size(); ++i )
	{
		int nItem = lcTypedTempl.InsertItem( i, "" );
		lcTypedTempl.SetItemText( nItem, 0, fmt::format("{}", i).c_str()  );
		lcTypedTempl.SetItemText( nItem, 1, fmt::format("{}", (*pTypedTemplateDlgData)[i].szTemplateType).c_str() );
		lcTypedTempl.SetItemText( nItem, 2, fmt::format("{}", (*pTypedTemplateDlgData)[i].szTemplate).c_str() );
		lcTypedTempl.SetItemData( nItem, i );
	}
	if ( nSelectedTemplate >= 0 && nSelectedTemplate < lcTypedTempl.GetItemCount() )
	{
		if ( lcTypedTempl.GetItemState(nSelectedTemplate, LVIS_SELECTED) != LVIS_SELECTED )
			lcTypedTempl.SetItemState( nSelectedTemplate, LVIS_SELECTED, LVIS_SELECTED );
	}

	if ( CWnd *pWnd = GetDlgItem(IDC_TYPED_REMOVE) )
	{
		pWnd->EnableWindow( nSelectedTemplate != -1 );
	}
}

void CReinfPointsTypedDlg::OnBnClickedOk()
{
	GetDialogData();
	//
	OnOK();
}

void CReinfPointsTypedDlg::OnBnClickedTypedRemove()
{
	CString strMessage;
	strMessage.LoadString( IDS_MIMO_DELETE_OBJECT_MESSAGE );
	if ( MessageBox( strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
	{
		// Shared with the wx dialog; see ReinfPointsDialogs.cpp.
		NReinfPointsTemplates::Remove( pTypedTemplateDlgData, pMapInfoEditor, nCurrentPlayer, nCurrentReinfPt,
																	 nSelectedTemplate );
		nSelectedTemplate = -1;
		SetDialogData();
	}
}

void CReinfPointsTypedDlg::OnNMClickListReinfPointsTypedTemplate( NMHDR *pNMHDR, LRESULT *pResult )
{
	CWaitCursor wcur;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if ( pNMLV->iItem >= 0 )
		nSelectedTemplate = pNMLV->iItem;

	SetDialogData();
	//
	*pResult = 0;
}

void CReinfPointsTypedDlg::GetDialogData()
{
	pTypedTemplateDlgData->clear();
	pTypedTemplateDlgData->resize( lcTypedTempl.GetItemCount() );

	int i = 0;
	for ( CReinfPointsState::CTypedTemplateType::iterator it = pTypedTemplateDlgData->begin(); it < pTypedTemplateDlgData->end(); ++it, ++i )
	{
		const std::string szType = lcTypedTempl.GetItemText(i, 1);
		it->szTemplateType = szType;
		it->szTemplate = lcTypedTempl.GetItemText( i, 2 );
	}
}

void CReinfPointsTypedDlg::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	switch ( nChar )
	{
	case VK_INSERT:
		OnBnClickedTypedAdd();
		break;
	case VK_DELETE:
		OnBnClickedTypedRemove();
		break;
	}
}


