#include "stdafx.h"
#include "BinaryBitFieldDialog.h"

#include <cstdint>

CBinaryBitFieldDialog::CBinaryBitFieldDialog( const std::string &_szFileName, uint8_t *_pData, const int _nSize, CWnd *pwndParent )
	: CResizeDialog( CBinaryBitFieldDialog::IDD, pwndParent ),
		bCreateControls( true ),
		szFileName( _szFileName ),
		pData( _pData ),
		nSize( _nSize )
{
	SetControlStyle( IDC_CT_FIELDS, ANCHORE_LEFT_TOP | RESIZE_HOR_VER );
	SetControlStyle( IDOK, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDCANCEL, ANCHORE_RIGHT_BOTTOM );
}


CBinaryBitFieldDialog::~CBinaryBitFieldDialog()
{
}


void CBinaryBitFieldDialog::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_CT_FIELDS, wndTablesList );
}


BEGIN_MESSAGE_MAP(CBinaryBitFieldDialog, CResizeDialog)
END_MESSAGE_MAP()


BOOL CBinaryBitFieldDialog::OnInitDialog()
{
	bCreateControls = true;

	CResizeDialog::OnInitDialog();

	if ( !NBitField::LoadFields( szFileName, &fields ) )
	{
		bCreateControls = false;
		return FALSE;
	}
	// IDC_CT_FIELDS is not LBS_SORT, so list index and field index stay the same.
	for ( size_t nField = 0; nField < fields.size(); ++nField )
	{
		const int nIndex = wndTablesList.AddString( fields[nField].szName.c_str() );
		if ( nIndex != LB_ERR && NBitField::IsSet( pData, nSize, fields[nField].nValue ) )
		{
			wndTablesList.SetCheck( nIndex, 1 );
		}
	}
	bCreateControls = false;
	return true;
}


void CBinaryBitFieldDialog::OnOK()
{
	CResizeDialog::OnOK();

	// EndDialog has not destroyed the list yet, so its checks can still be read.
	std::vector<bool> checked( fields.size(), false );
	for ( int nIndex = 0; nIndex < wndTablesList.GetCount() && nIndex < static_cast<int>( fields.size() ); ++nIndex )
	{
		checked[nIndex] = wndTablesList.GetCheck( nIndex ) != 0;
	}
	NBitField::Store( fields, checked, pData, nSize );
}
