#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "WMDefines.h"
#include "PC_Constants.h"

#include "PC_StringComboEditor.h"

#include <cstdint>

CPCStringComboEditor::CPCStringComboEditor() : bCreateControls( true )
{	
}


CPCStringComboEditor::~CPCStringComboEditor()
{
	DestroyWindow();
}


BEGIN_MESSAGE_MAP(CPCStringComboEditor, CComboBox)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_CONTROL_REFLECT(EN_CHANGE, OnEnChange)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
END_MESSAGE_MAP()


BOOL CPCStringComboEditor::PreTranslateMessage( MSG* pMsg ) 
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
	return CComboBox::PreTranslateMessage( pMsg );
}


void CPCStringComboEditor::OnSetFocus( CWnd* pOldWnd )
{
	CComboBox::OnSetFocus( pOldWnd );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CPCStringComboEditor::OnKillFocus( CWnd* pNewWnd ) 
{	
	CComboBox::OnKillFocus( pNewWnd );
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
	//	
	if ( CWnd *pwnd = GetParent() )
	{
		pwnd->SendMessage( WM_PC_ITEM_CHANGE, MAKEWPARAM( IC_KILL_FOCUS, PC_TEMPORARY_EDITOR ), 0 );
	}
}


void CPCStringComboEditor::OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) 
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
	CComboBox::OnChar( nChar, nRepCnt, nFlags );
}


void CPCStringComboEditor::OnEnChange()
{
	if ( !bCreateControls )
	{
		SetValueChanged();
	}
}


void CPCStringComboEditor::OnSelchange()
{
	if ( !bCreateControls )
	{
		SetValueChanged();
		if ( GetTargetWindow() )
		{
			GetTargetWindow()->SendMessage( WM_PC_ITEM_CHANGE, MAKEWPARAM( IC_VALUE_CHANGED, PC_TEMPORARY_EDITOR ), 0 );
		}
	}
}


// CPCItemEditor

bool CPCStringComboEditor::CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	bCreateControls = true;
	if ( CPCItemEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		const uint32_t dwStyle =  WS_CHILD | WS_VSCROLL | CBS_AUTOHSCROLL | CBS_DISABLENOSCROLL | CBS_DROPDOWNLIST;
		if ( CComboBox::Create( dwStyle, CRect( 0, 0, 0, 0 ), GetTargetWindow(), GetControlID() ) )
		{
			SetExtendedUI( true );
			if ( GetTargetWindow() && GetTargetWindow()->GetParent() )
			{
				if ( CFont* pFont = GetTargetWindow()->GetParent()->GetFont() )
				{
					SetFont( pFont );
				}
			}
			if ( GetItemEditorType() == PCIE_STRING_COMBO )
			{
				for( SPropertyDesc::CValuesList::const_iterator itValue = _pPropertyDesc->values.begin(); itValue != _pPropertyDesc->values.end(); ++itValue )
				{
					AddString( itValue->c_str() );
				}
			}
			bCreateControls = false;
			return true;
		}
	}
	return false;
}


bool CPCStringComboEditor::PlaceEditor( const CTRect<int> &rPlaceRect )
{
	MoveWindow( rPlaceRect.left, rPlaceRect.top - 2, rPlaceRect.Width() > 40 ? rPlaceRect.Width() : 40, 100, true );
	return true;
}


bool CPCStringComboEditor::ActivateEditor( CDialog *pwndActiveDialog )
{
	ShowWindow( SW_SHOW );
	if ( pwndActiveDialog )
	{
		pwndActiveDialog->GotoDlgCtrl( this );
		return true;
	}
	return false;
}


void CPCStringComboEditor::SetValue( const CVariant &rValue )
{
	szDefaultValue = rValue.GetStringRecode();
	if ( ::IsWindow( m_hWnd ) )
	{
		SetDefaultValue();
	}
}


void CPCStringComboEditor::GetValue( CVariant *pValue )
{
	if ( !pValue )
	{
		return;
	}
	//
	const int nSelectedText = GetCurSel();
	if ( nSelectedText != CB_ERR )
	{
		CString strText;
		GetLBText( nSelectedText, strText );
		*pValue = std::string( strText );
	}
	else
	{
		*pValue = szDefaultValue;
	}
}


void CPCStringComboEditor::SetDefaultValue()
{
	CPCItemEditor::SetDefaultValue();
	bCreateControls = true;
	SelectString( -1, szDefaultValue.c_str() );
	bCreateControls = false;
}


bool CPCStringComboEditor::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
		case ID_SELECTION_CUT:
			CComboBox::Cut();
			return true;
		case ID_SELECTION_COPY:
			CComboBox::Copy();
			return true;
		case ID_SELECTION_PASTE:
			CComboBox::Paste();
			return true;
		case ID_SELECTION_CLEAR:
			CComboBox::Clear();
			return true;
		case ID_SELECTION_SELECT_ALL:
			CComboBox::SetEditSel( 0, -1 );
			return true;
		case ID_SELECTION_RENAME:
		case ID_SELECTION_FIND:
		case ID_SELECTION_PROPERTIES:
			return false;
		default:
			return false;
	}
}


bool CPCStringComboEditor::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CPCStringComboEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CPCStringComboEditor::UpdateCommand(), pbCheck == 0" );
	//
	uint32_t dwSelection = 0;
	dwSelection = CComboBox::GetEditSel();
	switch( nCommandID )
	{
		case ID_SELECTION_CUT:
			( *pbEnable ) = ( LOWORD( dwSelection ) != HIWORD( dwSelection ) );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_COPY:
			( *pbEnable ) = ( LOWORD( dwSelection ) != HIWORD( dwSelection ) );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_PASTE:
			( *pbEnable ) = ::IsClipboardFormatAvailable( CF_TEXT );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_CLEAR:
			( *pbEnable ) = ( LOWORD( dwSelection ) != HIWORD( dwSelection ) );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_SELECT_ALL:
		{
			CString strText;
			GetWindowText( strText );
			( *pbEnable ) = ( ( LOWORD( dwSelection ) != 0 ) || ( HIWORD( dwSelection ) != strText.GetLength() ) );
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


