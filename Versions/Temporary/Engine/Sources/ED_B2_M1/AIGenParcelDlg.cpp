#include "stdafx.h"
#include <fmt/format.h>

#include "AIGenParcelDlg.h"
#include "AIGeneralTypes.h"

//
//
//		CAIGenParcelDlg dialog
//
//

IMPLEMENT_DYNAMIC( CAIGenParcelDlg, CResizeDialog )

CAIGenParcelDlg::CAIGenParcelDlg( CWnd *pParentWindow, NDb::EParcelType *_pType, float *_pImportance )
	: CResizeDialog( CAIGenParcelDlg::IDD, pParentWindow ),
	pType( _pType ),
	pImportance( _pImportance )
{
}


void CAIGenParcelDlg::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_AIGEN_COMBO_PTYPE, comboType );
	DDX_Control( pDX, IDC_AIGEN_EDIT_IMPORTANCE, edImportance );
}


BOOL CAIGenParcelDlg::OnInitDialog()
{
	CResizeDialog::OnInitDialog();

	comboType.ResetContent();
	for ( int i = 0; i < typeAIGeneralParcel.Size(); ++i )
	{
		int nIdx = comboType.AddString( typeAIGeneralParcel.GetMnemonic(i).c_str() );
		comboType.SetItemData( nIdx, i );
	}
	comboType.SelectString( 0, typeAIGeneralParcel.GetMnemonic(*pType).c_str() );
	edImportance.SetWindowText( fmt::format("{:f}", *pImportance) );

	return true;
}


void CAIGenParcelDlg::OnOK()
{
	(*pType) = (NDb::EParcelType)comboType.GetCurSel();
	CString szImportance;
	edImportance.GetWindowText( szImportance );
	float fImportance = 0;
	sscanf( szImportance, "%f", &fImportance );
	(*pImportance) = fImportance;

	CResizeDialog::OnOK();
}


