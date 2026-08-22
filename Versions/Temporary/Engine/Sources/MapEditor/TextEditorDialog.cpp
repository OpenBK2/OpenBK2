#include "stdafx.h"

#include "Scintilla/Scintilla.h"
#include "TextEditorDialog.h"

CTEMnemonics::CTEMnemonics() : CMnemonicsCollector<int>( TE_UNKNOWN, "" )
{
	Insert( TE_LUA, "lua" );
}


ETextEditorType CTEMnemonics::Get( const string &rszMnemonic )
{
	return static_cast<ETextEditorType>( GetValue( rszMnemonic ) );
}


CTEMnemonics typeTEMnemonics;


CTextEditorDialog::CTextEditorDialog( CWnd* pParent )
	: CResizeDialog( CTextEditorDialog::IDD, pParent ),
		bCreateControls( false ),
		editorType( TE_UNKNOWN ),
		bEnableEdit( true )
{
	//SetControlStyle( IDC_TE_TOOLBAR, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_TE_TEXT, ANCHORE_LEFT_TOP | RESIZE_HOR | RESIZE_VER );
	SetControlStyle( IDC_TE_TEXT_PLACEHOLDER, ANCHORE_LEFT_TOP | RESIZE_HOR | RESIZE_VER );
	SetControlStyle( IDC_TE_STATUSBAR, ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
	SetControlStyle( IDOK, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDCANCEL, ANCHORE_RIGHT_BOTTOM );
}


void CTextEditorDialog::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_TE_STATUSBAR, wndEditorStatusStringWindow );
}


BEGIN_MESSAGE_MAP(CTextEditorDialog, CResizeDialog)
END_MESSAGE_MAP()


void CTextEditorDialog::GetXMLFilePath( string *pszXMLFilePath )
{
	( *pszXMLFilePath ) = TE_XML_FILE_NAME_LABEL_PREFIX + typeTEMnemonics.GetMnemonic( editorType );
}


BOOL CTextEditorDialog::OnInitDialog() 
{
	bCreateControls = true;
	CResizeDialog::OnInitDialog();
	//
	/**
	toolBar.CreateEx( this,
										TBSTYLE_FLAT,
										WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS,
										CRect( 0, 0, 0, 0 ), 
										IDC_TE_TOOLBAR );
	{
		toolBar.LoadToolBar( IDT_TEXT_EDITOR );
		CRect clientRect;
		GetClientRect( &clientRect );
		toolBar.MoveWindow( 0, -2, clientRect.Width(), HIWORD( toolBar.GetToolBarCtrl().GetButtonSize() ) + 2, false );
	}
	/**/
	//
	scintillaEditorWindow.CreateEx( this, WS_EX_CLIENTEDGE , WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect( 0, 0, 0, 0 ), IDC_TE_TEXT );
	scintillaEditorWindow.SetStatusStringWindow( &wndEditorStatusStringWindow );
	if ( CWnd *pWnd = GetDlgItem( IDC_TE_TEXT_PLACEHOLDER ) )
	{
		CRect windowRect;
		pWnd->GetWindowRect( &windowRect );
		ScreenToClient( &windowRect );
		scintillaEditorWindow.MoveWindow( &windowRect );
		scintillaEditorWindow.SetText( szText );
		scintillaEditorWindow.Command( SCI_SETREADONLY, !bEnableEdit, 0 );
	}
	//
	UpdateControlPositions();
	//UpdateToolBar();

	if ( !szTitle.empty() )
	{
		SetWindowText( szTitle.c_str() );
	}
	//
	bCreateControls = false;
	return true;
}


void CTextEditorDialog::OnOK() 
{
	scintillaEditorWindow.GetText( &szText );
	CResizeDialog::OnOK();
}


void CTextEditorDialog::OnCancel() 
{
	//scintillaEditorWindow.GetText( &szText );
	CResizeDialog::OnCancel();
}


/**
void CTextEditorDialog::UpdateToolBar()
{
	//CToolBarCtrl& rToolBarCtrl = toolBar.GetToolBarCtrl();
	//rToolBarCtrl.CheckButton( IDC_PC_PIN, resizeDialogOptions.nParameters[PC_TREE_COLUMN_COUNT] > 0 );
}
/**/


void CTextEditorDialog::GetText( string *pszText )
{
	if ( pszText )
	{
		( *pszText ) = szText;
	}
}

// basement storage  


