#include "stdafx.h"

#include "../MapEditorLib/Interface_MainFrame.h"
#include "ReinforcementTypes.h"
#include "ReinfPointsTypedTemplateAddDlg.h"

//
//		CReinfPointsTypedTemplateAddDlg dialog
//

IMPLEMENT_DYNAMIC( CReinfPointsTypedTemplateAddDlg, CResizeDialog )

CReinfPointsTypedTemplateAddDlg::CReinfPointsTypedTemplateAddDlg( CWnd *pParentWindow, string *pTypeDlgData, CMapInfoEditor *_pMapInfoEditor )
	: CResizeDialog( CReinfPointsTypedTemplateAddDlg::IDD, pParentWindow ),
	pMapInfoEditor( _pMapInfoEditor ),
	pDlgData( pTypeDlgData )
{
}


void CReinfPointsTypedTemplateAddDlg::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_COMBO_CMD_TYPE, wndComboType );
}


BEGIN_MESSAGE_MAP( CReinfPointsTypedTemplateAddDlg, CResizeDialog )
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_TYPED_TEMPL_LINK, OnBnClickedTemplate)
END_MESSAGE_MAP()

BOOL CReinfPointsTypedTemplateAddDlg::OnInitDialog()
{
	CResizeDialog::OnInitDialog();

	// get typedTemplate from DB
	string szType = (*pDlgData).c_str();
	szType += ".Type";
	string szTempl = (*pDlgData).c_str();
	szTempl += ".Template";

	CManipulatorManager::GetValue( &typedTemplate.szTemplateType, pMapInfoEditor->GetViewManipulator(), szType );
	CManipulatorManager::GetValue( &typedTemplate.szTemplate, pMapInfoEditor->GetViewManipulator(), szTempl );

	wndComboType.ResetContent();

	int nItem = wndComboType.AddString( typedTemplate.szTemplateType.c_str() ); 
	wndComboType.SetItemData( nItem, 0 );

	for ( int i = 0; i < typeReinforcementMnemonics.Size(); ++i )
	{
		if ( typeReinforcementMnemonics.GetMnemonic(i).c_str() != typedTemplate.szTemplateType )
		{
			int nIdx = wndComboType.AddString( typeReinforcementMnemonics.GetMnemonic(i).c_str() );
			wndComboType.SetItemData( nIdx, i );
		}
	}
	wndComboType.SelectString( 0, typedTemplate.szTemplateType.c_str() );

	return true;
}


void CReinfPointsTypedTemplateAddDlg::OnBnClickedOk()
{
	string szType = (*pDlgData).c_str();
	szType += ".Type";
	string szTempl = (*pDlgData).c_str();
	szTempl += ".Template";

	// set type
	int nSel = wndComboType.GetCurSel();
	CString szNewType;
	wndComboType.GetLBText( nSel, szNewType ); // new type value is in szNewType
	CManipulatorManager::SetValue( static_cast<string>(szNewType), pMapInfoEditor->GetViewManipulator(), szType, true );
	// set Template
	CString szNewTemplate;
	GetDlgItemText( IDC_EDIT_TEMPLATE, szNewTemplate );
	CManipulatorManager::SetValue( static_cast<string>(szNewTemplate), pMapInfoEditor->GetViewManipulator(), szTempl, true );

	OnOK();
}


void CReinfPointsTypedTemplateAddDlg::OnBnClickedTemplate()
{
	string szTempl = (*pDlgData).c_str();
	szTempl += ".Template";

	// browse link
	if ( Singleton<IMainFrameContainer>()->Get()->BrowseLink(&szLink, "", dynamic_cast<const SPropertyDesc*>(pMapInfoEditor->GetViewManipulator()->GetDesc(szTempl)), false, true) )
	{
		SetDlgItemText( IDC_EDIT_TEMPLATE, szLink.c_str() );
	}
}


