#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "WMDefines.h"
#include "PC_Constants.h"

#include "PC_StringSliderEditor.h"

#include <cstdint>

CPCStringSliderEditor::CPCStringSliderEditor() : bCreateControls( true )
{	
}


CPCStringSliderEditor::~CPCStringSliderEditor()
{
	wndSlider.DestroyWindow();
	DestroyWindow();
}


BEGIN_MESSAGE_MAP(CPCStringSliderEditor, CEdit)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_MESSAGE(WM_PC_EDITOR_SLIDER_CHANGE, OnMessageEditorSliderChange)
	ON_CONTROL_REFLECT(EN_CHANGE, OnEnChange)
END_MESSAGE_MAP()


BOOL CPCStringSliderEditor::PreTranslateMessage( MSG* pMsg ) 
{
	if ( pMsg->message == WM_KEYDOWN )	
	{		
		if ( pMsg->wParam == VK_TAB )
		{
			if ( ( GetAsyncKeyState( VK_SHIFT ) & 0x8000 ) == 0 )
			{
				wndSlider.SetFocus();
				return 1;
			}
		}
		if ( ( pMsg->wParam == VK_RETURN ) || ( pMsg->wParam == VK_ESCAPE ) )
		{
			::TranslateMessage( pMsg );
			::DispatchMessage( pMsg );			
			return 1;
		}	
	}	
	return CEdit::PreTranslateMessage( pMsg );
}


void CPCStringSliderEditor::OnSetFocus( CWnd* pOldWnd )
{
	CEdit::OnSetFocus( pOldWnd );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CPCStringSliderEditor::OnKillFocus( CWnd* pNewWnd ) 
{	
	CEdit::OnKillFocus( pNewWnd );
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
	//	
	if ( pNewWnd != &wndSlider )
	{
		if ( GetTargetWindow() )
		{
			GetTargetWindow()->SendMessage( WM_PC_ITEM_CHANGE, MAKEWPARAM( IC_KILL_FOCUS, PC_TEMPORARY_EDITOR ), 0 );
		}
	}
}


void CPCStringSliderEditor::OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) 
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


void CPCStringSliderEditor::OnEnChange()
{
	if ( !bCreateControls )
	{
		SetValueChanged();
		OnChangeEditBox();
	}
}


LRESULT CPCStringSliderEditor::OnMessageEditorSliderChange( WPARAM wParam, LPARAM lParam )
{
	if ( !bCreateControls )
	{
		switch( LOWORD( wParam ) )
		{
			case ESC_KILL_FOCUS:
				if ( GetTargetWindow() )
				{
					SetValueChanged();
					GetTargetWindow()->SendMessage( WM_PC_ITEM_CHANGE, MAKEWPARAM( IC_KILL_FOCUS, PC_TEMPORARY_EDITOR ), 0 );
				}
				break;
			default:
				break;
		}
	}
	return 0;
}


// CPCItemEditor

bool CPCStringSliderEditor::CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	bCreateControls = true;
	if ( CPCItemEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		const uint32_t dwStyle							= WS_CHILD | ES_AUTOHSCROLL | ES_LEFT;
		const uint32_t dwSliderStyle				= WS_CHILD | TBS_HORZ | TBS_AUTOTICKS;
		const uint32_t dwExStyle						= WS_EX_CLIENTEDGE;
		const uint32_t dwSliderExStyle			= WS_EX_CLIENTEDGE;
		bool bResult = CEdit::Create( dwStyle, CRect( 0, 0, 0, 0 ), GetTargetWindow(), GetControlID() ) &&
									 wndSlider.Create( dwSliderStyle, CRect( 0, 0, 0, 0 ), GetTargetWindow(), GetControlID() + 1 );
		if ( bResult )
		{
			ModifyStyleEx( 0, dwExStyle );
			wndSlider.ModifyStyleEx( 0, dwSliderExStyle );
			if ( GetTargetWindow() && GetTargetWindow()->GetParent() )
			{
				if ( CFont* pFont = GetTargetWindow()->GetParent()->GetFont() )
				{
					SetFont( pFont );
					wndSlider.SetFont( pFont );
					wndSlider.SetTargetWindow( this );
					wndSlider.SetPreviousWindow( this );
				}
			}
			bCreateControls = false;
			return true;
		}
	}
	return false;
}


bool CPCStringSliderEditor::PlaceEditor( const CTRect<int> &rPlaceRect )
{
	CTRect<int> editRect( rPlaceRect );
	CTRect<int> sliderRect( rPlaceRect );
	//
	sliderRect.left += 40;
	editRect.right = sliderRect.left;
	//
	MoveWindow( editRect.left, editRect.top, editRect.Width(), editRect.Height(), true );
	wndSlider.MoveWindow( sliderRect.left, sliderRect.top, sliderRect.Width(), sliderRect.Height(), true );
	return true;
}


bool CPCStringSliderEditor::ActivateEditor( CDialog *pwndActiveDialog )
{
	ShowWindow( SW_SHOW );
	wndSlider.ShowWindow( SW_SHOW );
	if ( pwndActiveDialog )
	{
		pwndActiveDialog->GotoDlgCtrl( this );
		return true;
	}
	return false;
}


void CPCStringSliderEditor::SetValue( const CVariant &rValue )
{
	szDefaultValue = rValue.GetStringRecode();
	if ( ::IsWindow( m_hWnd ) )
	{
		SetDefaultValue();
	}
}


void CPCStringSliderEditor::GetValue( CVariant *pValue )
{
	if ( !pValue )
	{
		return;
	}

	CString strText;
	GetWindowText( strText );
	*pValue = string( strText );
}


void CPCStringSliderEditor::SetDefaultValue()
{
	CPCItemEditor::SetDefaultValue();
	bCreateControls = true;
	SetWindowText( szDefaultValue.c_str()	);
	bCreateControls = false;
}


void CPCStringSliderEditor::ProcessMessage( unsigned nMessage, WPARAM wParam, LPARAM lParam )
{
	if ( !bCreateControls )
	{
		if ( reinterpret_cast<CPCEditorSlider*>( lParam ) == &wndSlider )
		{
			switch( LOWORD( wParam ) )
			{
				case ESC_POSITION_CHANGED:
					OnChangePos( wndSlider.GetPos() );
					break;
				default:
					break;
			}
		}
	}
}


bool CPCStringSliderEditor::HandleCommand( unsigned nCommandID, uint32_t dwData )
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


bool CPCStringSliderEditor::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CPCStringSliderEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CPCStringSliderEditor::UpdateCommand(), pbCheck == 0" );
	//
	int nStartChar = 0;
	int nEndChar = 0;
	CEdit::GetSel( nStartChar, nEndChar );
	switch( nCommandID )
	{
		case ID_SELECTION_CUT:
			( *pbEnable ) = ( nStartChar != nEndChar );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_COPY:
			( *pbEnable ) = ( nStartChar != nEndChar );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_PASTE:
			( *pbEnable ) = ::IsClipboardFormatAvailable( CF_TEXT );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_CLEAR:
			( *pbEnable ) = ( nStartChar != nEndChar );
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


