#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "wmdefines.h"
#include "pc_constants.h"

#include "PC_IntInputEditor.h"

#include <cstdint>

bool CPCIntInputEditor::GetPCItemStringValue( string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
{
	NI_ASSERT( pszValue != 0, "CPCIntInputEditor::GetPCItemStringValue() pszValue == 0" );
	( *pszValue ) = std::to_string(  (int)rValue );
	return true;
}


bool CPCIntInputEditor::GetPCItemValue( CVariant *pValue, const string &rszValue, const SPropertyDesc *pPropertyDesc )
{
	NI_ASSERT( pValue != 0, "CPCIntInputEditor::GetPCItemValue() pValue == 0" );
	int nValue = 0;
	if ( sscanf( rszValue.c_str(), "%d", &nValue ) == 1 )
	{
		( *pValue ) = nValue;
		return true;
	}
	return false;
}


CPCIntInputEditor::CPCIntInputEditor() : nDefaultValue( 0 ), bCreateControls( true )
{	
}


CPCIntInputEditor::~CPCIntInputEditor()
{
	DestroyWindow();
}


BEGIN_MESSAGE_MAP(CPCIntInputEditor, CEdit)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_CONTROL_REFLECT(EN_CHANGE, OnEnChange)
END_MESSAGE_MAP()


BOOL CPCIntInputEditor::PreTranslateMessage( MSG* pMsg ) 
{
	if ( pMsg->message == WM_KEYDOWN )	
	{		
		if ( ( pMsg->wParam == VK_RETURN ) || ( pMsg->wParam == VK_ESCAPE ) )
		{
			::TranslateMessage( pMsg );
			::DispatchMessage( pMsg );			
			return 1;
		}	
	}	
	return CEdit::PreTranslateMessage( pMsg );
}


void CPCIntInputEditor::OnSetFocus( CWnd* pOldWnd )
{
	CEdit::OnSetFocus( pOldWnd );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CPCIntInputEditor::OnKillFocus( CWnd* pNewWnd ) 
{	
	CEdit::OnKillFocus( pNewWnd );
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
	//
	if ( GetTargetWindow() )
	{
		GetTargetWindow()->SendMessage( WM_PC_ITEM_CHANGE, MAKEWPARAM( IC_KILL_FOCUS, PC_TEMPORARY_EDITOR ), 0 );
	}
}


void CPCIntInputEditor::OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) 
{
	if ( ( nChar == VK_ESCAPE ) || ( nChar == VK_RETURN ) )	
	{		
		if( nChar == VK_ESCAPE )
		{
			SetDefaultValue();
		}
		if ( GetTargetWindow() )
		{
			GetTargetWindow()->SetFocus();		
		}
		return;	
	}	
	CEdit::OnChar( nChar, nRepCnt, nFlags );
}


void CPCIntInputEditor::OnEnChange()
{
	if ( !bCreateControls )
	{
		SetValueChanged();
	}
}


// CPCItemEditor

bool CPCIntInputEditor::CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	bCreateControls = true;
	if ( CPCItemEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		const uint32_t dwStyle		= WS_CHILD | ES_AUTOHSCROLL | ES_LEFT;
		const uint32_t dwExStyle	= WS_EX_CLIENTEDGE;
		if ( CEdit::Create( dwStyle, CRect( 0, 0, 0, 0 ), GetTargetWindow(), GetControlID() ) )
		{
			ModifyStyleEx( 0, dwExStyle );
			if ( GetTargetWindow() && GetTargetWindow()->GetParent() )
			{
				if ( CFont* pFont = GetTargetWindow()->GetParent()->GetFont() )
				{
					SetFont( pFont );
				}
			}
			bCreateControls = false;
			return true;
		}
	}
	return false;
}


bool CPCIntInputEditor::PlaceEditor( const CTRect<int> &rPlaceRect )
{
	MoveWindow( rPlaceRect.left, rPlaceRect.top, rPlaceRect.Width() > 40 ? rPlaceRect.Width() : 40, rPlaceRect.Height(), true );
	return true;
}


bool CPCIntInputEditor::ActivateEditor( CDialog *pwndActiveDialog )
{
	ShowWindow( SW_SHOW );
	if ( pwndActiveDialog )
	{
		pwndActiveDialog->GotoDlgCtrl( this );
		return true;
	}
	return false;
}


void CPCIntInputEditor::SetValue( const CVariant &rValue )
{
	nDefaultValue = (int)rValue;
	if ( ::IsWindow( m_hWnd ) )
	{
		SetDefaultValue();
	}
}


void CPCIntInputEditor::GetValue( CVariant *pValue )
{
	if ( !pValue )
	{
		return;
	}
	//
	CString strText;
	GetWindowText( strText );
	if ( !GetPCItemValue( pValue, string( strText ), GetPropertyDesc() ) )
	{
		( *pValue ) = nDefaultValue;
	}
}


void CPCIntInputEditor::SetDefaultValue()
{
	CPCItemEditor::SetDefaultValue();
	//
	bCreateControls = true;
	string szValue;
	GetPCItemStringValue( &szValue, CVariant( nDefaultValue ) , GetPropertyDesc() );
	SetWindowText( szValue.c_str() );
	bCreateControls = false;
}


void CPCIntInputEditor::EnableEdit( bool bEnable )
{
	CPCItemEditor::EnableEdit( bEnable );
	SetReadOnly( !bEnable );
}


bool CPCIntInputEditor::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
		case ID_SELECTION_CUT:
			CEdit::Cut();
			return true;
		case ID_SELECTION_COPY:
			CEdit::Copy();
			return true;
		case ID_SELECTION_PASTE:
			CEdit::Paste();
			return true;
		case ID_SELECTION_CLEAR:
			CEdit::Clear();
			return true;
		case ID_SELECTION_SELECT_ALL:
			CEdit::SetSel( 0, -1, false );
			return true;
		case ID_SELECTION_RENAME:
		case ID_SELECTION_FIND:
		case ID_SELECTION_PROPERTIES:
			return false;
		default:
			return false;
	}
}


bool CPCIntInputEditor::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CPCIntInputEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CPCIntInputEditor::UpdateCommand(), pbCheck == 0" );
	//
	int nStartChar = 0;
	int nEndChar = 0;
	CEdit::GetSel( nStartChar, nEndChar );
	switch( nCommandID )
	{
		case ID_SELECTION_CUT:
			( *pbEnable ) = ( ( GetStyle() & ES_READONLY ) == 0 ) && ( nStartChar != nEndChar );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_COPY:
			( *pbEnable ) = ( nStartChar != nEndChar );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_PASTE:
			( *pbEnable ) = ( ( GetStyle() & ES_READONLY ) == 0 ) && ::IsClipboardFormatAvailable( CF_TEXT );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_CLEAR:
			( *pbEnable ) = ( ( GetStyle() & ES_READONLY ) == 0 ) && ( nStartChar != nEndChar );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_SELECT_ALL:
		{
			CString strText;
			GetWindowText( strText );
			( *pbEnable ) = ( ( nStartChar != 0 ) || ( nEndChar != strText.GetLength() ) );
			( *pbCheck ) = false;
			return true;
		}
		case ID_SELECTION_RENAME:
		case ID_SELECTION_FIND:
		case ID_SELECTION_PROPERTIES:
			return false;
		default:
			return false;
	}
}

// basement storage  


