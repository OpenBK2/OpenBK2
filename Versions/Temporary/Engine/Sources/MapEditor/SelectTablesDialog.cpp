#include "stdafx.h"

#include "MapEditorLib/Tools_HashSet.h"
#include "SelectTablesDialog.h"

CSelectTablesDialog::CSelectTablesDialog( CWnd* pParent )
	: CResizeDialog( CSelectTablesDialog::IDD, pParent ), bCreateControls( true )
{
	SetControlStyle( IDC_CT_TABLES_LIST, ANCHORE_LEFT_TOP | RESIZE_HOR_VER );
	SetControlStyle( IDOK, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDCANCEL, ANCHORE_RIGHT_BOTTOM );
}


void CSelectTablesDialog::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_CT_TABLES_LIST, wndTablesList );
}


BEGIN_MESSAGE_MAP(CSelectTablesDialog, CResizeDialog)
END_MESSAGE_MAP()


BOOL CSelectTablesDialog::OnInitDialog() 
{
	bCreateControls = true;

	CResizeDialog::OnInitDialog();

	for ( list<string>::const_iterator itString = tables.begin(); itString != tables.end(); ++itString )
	{
		const int nAddedString = wndTablesList.AddString( itString->c_str() );
		if ( selectedTables.find( *itString ) != selectedTables.end() )
		{
			wndTablesList.SetCheck( nAddedString, 1 );
		}
		else
		{
			wndTablesList.SetCheck( nAddedString, 0 );
		}
	}

	bCreateControls = false;
	return true;
}


void CSelectTablesDialog::OnOK()
{
	CResizeDialog::OnOK();
	
	selectedTables.clear();

	const int nStringsCount = wndTablesList.GetCount();
	for ( int nStringIndex = 0; nStringIndex < nStringsCount; ++nStringIndex )
	{
		if ( wndTablesList.GetCheck( nStringIndex ) > 0 )
		{
			CString strText;
			wndTablesList.GetText( nStringIndex, strText );
			InsertHashSetElement( &selectedTables, string( strText ) );
		}
	}
}


// basement storage  


