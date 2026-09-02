#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "WMDefines.h"
#include "PC_Constants.h"

#include "PC_GUIDEditor.h"
#include "Misc/StrProc.h"

#include <cstdint>

bool CPCGUIDEditor::GetPCItemStringValue( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
{
	NI_ASSERT( pszValue != 0, "CPCBinaryBitFieldEditor::GetPCItemStringValue() pszValue == 0" );
	pszValue->clear();

	try
	{
		const GUID *pValue = static_cast<const GUID*>( rValue.GetPtr() );
		NStr::GUID2String( pszValue, *pValue );
	}
	catch ( ... )
	{
	}
	/*
	ASSERT( rValue.GetType() == CVariant::VT_GUID );
	*pszValue = rValue.ToString();
	*/

	return true;
}


bool CPCGUIDEditor::GetPCItemValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc )
{
	NI_ASSERT( pValue != 0, "CPCBinaryBitFieldEditor::GetPCItemValue() pValue == 0" );
	( *pValue ) = CVariant();
	uint8_t * pData = new uint8_t[sizeof( GUID )];
	GUID value;
	try
	{
		NStr::String2GUID( rszValue, &value ); 
		memcpy( pData, &value, sizeof( GUID ) );
	}
	catch ( ... )
	{
	}
	( *pValue ) = CVariant( static_cast<void*>( pData ), pPropertyDesc->nSize );
	pValue->SetDestructorDeleted( ( pPropertyDesc->nSize > 0 ), pPropertyDesc->nSize );
	return true;
}


CPCGUIDEditor::CPCGUIDEditor() : bCreateControls( true )
{	
}


CPCGUIDEditor::~CPCGUIDEditor()
{
	DestroyWindow();
}


BEGIN_MESSAGE_MAP(CPCGUIDEditor, CEdit)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_CONTROL_REFLECT(EN_CHANGE, OnEnChange)
END_MESSAGE_MAP()


BOOL CPCGUIDEditor::PreTranslateMessage( MSG* pMsg ) 
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


void CPCGUIDEditor::OnSetFocus( CWnd* pOldWnd )
{
	CEdit::OnSetFocus( pOldWnd );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CPCGUIDEditor::OnKillFocus( CWnd* pNewWnd ) 
{	
	CEdit::OnKillFocus( pNewWnd );
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
	//	
	SetDefaultValue();
	if ( GetTargetWindow() )
	{
		GetTargetWindow()->SendMessage( WM_PC_ITEM_CHANGE, MAKEWPARAM( IC_KILL_FOCUS, PC_TEMPORARY_EDITOR ), 0 );
	}
}


void CPCGUIDEditor::OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) 
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


void CPCGUIDEditor::OnEnChange()
{
	if ( !bCreateControls )
	{
		SetValueChanged();
	}
}


// CPCItemEditor

bool CPCGUIDEditor::CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	bCreateControls = true;
	if ( CPCItemEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		const uint32_t dwStyle		= WS_CHILD | ES_AUTOHSCROLL | ES_LEFT;
		const uint32_t dwExStyle	= WS_EX_CLIENTEDGE;
		bool bResult = CEdit::Create( dwStyle, CRect( 0, 0, 0, 0 ), GetTargetWindow(), GetControlID() );
		if ( bResult )
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
			SetReadOnly( true );
			return true;
		}
	}
	return false;
}


bool CPCGUIDEditor::PlaceEditor( const CTRect<int> &rPlaceRect )
{
	MoveWindow( rPlaceRect.left, rPlaceRect.top, rPlaceRect.Width() > 40 ? rPlaceRect.Width() : 40, rPlaceRect.Height(), true );
	return true;
}


bool CPCGUIDEditor::ActivateEditor( CDialog *pwndActiveDialog )
{
	ShowWindow( SW_SHOW );
	if ( pwndActiveDialog )
	{
		pwndActiveDialog->GotoDlgCtrl( this );
		return true;
	}
	return false;
}


void CPCGUIDEditor::SetValue( const CVariant &rValue )
{
	std::string szValue;
	GetPCItemStringValue( &szValue, rValue, GetPropertyDesc() );
	szDefaultValue = szValue;
	if ( ::IsWindow( m_hWnd ) )
	{
		SetDefaultValue();
	}
}


void CPCGUIDEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CString strText;
		GetWindowText( strText );
		GetPCItemValue( pValue, std::string( strText ), GetPropertyDesc() );
	}
}


void CPCGUIDEditor::SetDefaultValue()
{
	CPCItemEditor::SetDefaultValue();
	bCreateControls = true;
	SetWindowText( szDefaultValue.c_str()	);
	bCreateControls = false;
}


void CPCGUIDEditor::EnableEdit( bool bEnable )
{
	CPCItemEditor::EnableEdit( false );
	SetReadOnly( true );
}


bool CPCGUIDEditor::HandleCommand( unsigned nCommandID, uint32_t dwData )
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


bool CPCGUIDEditor::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CPCGUIDEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CPCGUIDEditor::UpdateCommand(), pbCheck == 0" );
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
			( *pbEnable ) = ( ( GetStyle() & ES_READONLY ) == 0 ) && ( nStartChar != nEndChar );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_PASTE:
			( *pbEnable ) = ::IsClipboardFormatAvailable( CF_TEXT );
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


