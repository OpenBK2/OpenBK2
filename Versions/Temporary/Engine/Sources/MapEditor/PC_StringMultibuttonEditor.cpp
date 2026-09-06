#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "WMDefines.h"
#include "PC_Constants.h"

#include "PC_StringMultibuttonEditor.h"

#include <cstdint>

CPCStringMultibuttonEditor::CPCStringMultibuttonEditor( int _nButtonCount )
	: bIgnoreFocusChange( false ),
		bMultiLine( false ),
		bCreateControls( true ),
		nButtonCount( _nButtonCount )
{	
}


CPCStringMultibuttonEditor::~CPCStringMultibuttonEditor()
{
	DestroyWindow();
}


BEGIN_MESSAGE_MAP(CPCStringMultibuttonEditor, CEdit)
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_MESSAGE(WM_PC_EDITOR_BUTTON_CHANGE, OnMessageEditorButtonChange)
	ON_CONTROL_REFLECT(EN_CHANGE, OnEnChange)
END_MESSAGE_MAP()


void CPCStringMultibuttonEditor::OnDestroy() 
{
	for ( CPCEditorButtonList::iterator itPCEditorButton = buttonList.begin(); itPCEditorButton != buttonList.end(); ++itPCEditorButton )
	{
		if ( ( *itPCEditorButton ) != 0 )
		{
			( *itPCEditorButton )->DestroyWindow();
			delete ( *itPCEditorButton );
			( *itPCEditorButton ) = 0;
		}
	}
	buttonList.clear();
	CEdit::OnDestroy();
}


BOOL CPCStringMultibuttonEditor::PreTranslateMessage( MSG* pMsg ) 
{
	if ( pMsg->message == WM_KEYDOWN )	
	{		
		if ( pMsg->wParam == VK_TAB )
		{
			if ( ( GetAsyncKeyState( VK_SHIFT ) & 0x8000 ) == 0 )
			{
				if ( !buttonList.empty() )
				{
					CPCEditorButtonList::iterator posPCEditorButton = buttonList.begin();
					if ( ( *posPCEditorButton ) != 0 )
					{
						( *posPCEditorButton )->SetFocus();
					}
				}
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


void CPCStringMultibuttonEditor::OnSetFocus( CWnd* pOldWnd )
{
	CEdit::OnSetFocus( pOldWnd );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CPCStringMultibuttonEditor::OnKillFocus( CWnd* pNewWnd ) 
{	
	CEdit::OnKillFocus( pNewWnd );
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
	//
	if ( !bIgnoreFocusChange )
	{
		bool bButtonNotFocused = true;
		for ( CPCEditorButtonList::iterator itPCEditorButton = buttonList.begin(); itPCEditorButton != buttonList.end(); ++itPCEditorButton )
		{
			if ( pNewWnd == ( *itPCEditorButton ) )
			{
				bButtonNotFocused = false;
				break;
			}
		}
		if ( bButtonNotFocused && GetTargetWindow() )
		{
			GetTargetWindow()->SendMessage( WM_PC_ITEM_CHANGE, MAKEWPARAM( IC_KILL_FOCUS, PC_TEMPORARY_EDITOR ), 0 );
		}
	}
}


void CPCStringMultibuttonEditor::OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) 
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


void CPCStringMultibuttonEditor::OnEnChange()
{
	if ( !bCreateControls )
	{
		SetValueChanged();
	}
}


LRESULT CPCStringMultibuttonEditor::OnMessageEditorButtonChange( WPARAM wParam, LPARAM lParam )
{
	switch( LOWORD( wParam ) )
	{
		case EBC_KILL_FOCUS:
		{
			if ( GetTargetWindow() )
			{
				GetTargetWindow()->SendMessage( WM_PC_ITEM_CHANGE, MAKEWPARAM( IC_KILL_FOCUS, PC_TEMPORARY_EDITOR ), 0 );
			}
			break;
		}
		case EBC_PRESSED:
		{
			bIgnoreFocusChange = true;
			int nButtonIndex = 0;
			for ( CPCEditorButtonList::iterator itPCEditorButton = buttonList.begin(); itPCEditorButton != buttonList.end(); ++itPCEditorButton )
			{
				if ( lParam == reinterpret_cast<LPARAM>( ( *itPCEditorButton ) ) )
				{
					SetFocus();
					OnButtonPressed( nButtonIndex );
					SetFocus();
				}
				++nButtonIndex;
			}		
			bIgnoreFocusChange = false;
			break;
		}
		default:
		{	
			break;
		}
	}
	return 0;
}


// CPCItemEditor

bool CPCStringMultibuttonEditor::CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	bCreateControls = true;
	if ( CPCItemEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		const uint32_t dwStyle							= WS_CHILD | ES_AUTOHSCROLL | ES_LEFT | ( bMultiLine ? ES_MULTILINE : 0 );
		const uint32_t dwButtonStyle				= WS_CHILD;
		const uint32_t dwExStyle						= WS_EX_CLIENTEDGE;
		bool bResult = CEdit::Create( dwStyle, CRect( 0, 0, 0, 0 ), GetTargetWindow(), GetControlID() );
		for ( int nButtonIndex = 0; nButtonIndex < nButtonCount; ++nButtonIndex )
		{
			CString strButtonTitle;
			GetButtonTitle( &strButtonTitle, nButtonIndex );
			CPCEditorButtonList::iterator posPCEditorButton = buttonList.insert( buttonList.end(), 0 );
			if ( ( *posPCEditorButton ) = new CPCEditorButton() )
			{
				bResult = bResult && ( *posPCEditorButton )->Create( strButtonTitle, dwButtonStyle, CRect( 0, 0, 0, 0 ), GetTargetWindow(), GetControlID() + nButtonIndex + 1 );
			}
			else
			{
				bResult = false;
			}
		}
		if ( bResult )
		{
			ModifyStyleEx( 0, dwExStyle );
			CFont *pFont = GetEditorFont();
			if ( pFont )
			{
				SetFont( pFont );
			}
			// The wiring below runs whether or not a font was found. It used to
			// sit inside the font branch, so a missing font left the buttons
			// with no target and no tab order: a dead editor over an appearance
			// detail.
			CWnd *pPreviousWindow = this;
			for ( CPCEditorButtonList::iterator itPCEditorButton = buttonList.begin(); itPCEditorButton != buttonList.end(); ++itPCEditorButton )
			{
				if ( ( *itPCEditorButton ) != 0 )
				{
					if ( pFont )
					{
						( *itPCEditorButton )->SetFont( pFont );
					}
					( *itPCEditorButton )->SetTargetWindow( this );
					//
					( *itPCEditorButton )->SetPreviousWindow( pPreviousWindow );
					pPreviousWindow = ( *itPCEditorButton );
					//
					CPCEditorButtonList::iterator itNextPCEditorButton = itPCEditorButton;
					++itNextPCEditorButton;
					if ( itNextPCEditorButton != buttonList.end() )
					{
						( *itPCEditorButton )->SetNextWindow( ( *itNextPCEditorButton ) );
					}
				}
			}
			bCreateControls = false;
			return true;
		}
	}
	return false;
}


//! What one button needs to show its caption whole.
//!
//! The original gave every button 25 pixels. That fits "...", which is the only
//! caption a one button editor has, and it does not fit "New" or "Edit" in the
//! shell font this editor now draws in, so those came out clipped. Measuring
//! the caption in the button's own font is what makes the width follow the font
//! and the DPI instead of a number picked at one of each.
int CPCStringMultibuttonEditor::GetButtonWidth( CPCEditorButton *pButton )
{
	// The old fixed width, kept as the floor so "..." keeps the size it had.
	const int N_MIN_BUTTON_WIDTH = 25;
	const int N_BUTTON_TEXT_MARGIN = 10;
	if ( ( pButton == 0 ) || ( pButton->GetSafeHwnd() == 0 ) )
	{
		return N_MIN_BUTTON_WIDTH;
	}
	CFont *pFont = pButton->GetFont();
	if ( pFont == 0 )
	{
		return N_MIN_BUTTON_WIDTH;
	}
	CDC *pDC = pButton->GetDC();
	if ( pDC == 0 )
	{
		return N_MIN_BUTTON_WIDTH;
	}
	CString strTitle;
	pButton->GetWindowText( strTitle );
	CFont *pOldFont = pDC->SelectObject( pFont );
	const CSize textSize = pDC->GetTextExtent( strTitle );
	pDC->SelectObject( pOldFont );
	pButton->ReleaseDC( pDC );
	const int nWidth = textSize.cx + N_BUTTON_TEXT_MARGIN;
	return ( nWidth > N_MIN_BUTTON_WIDTH ) ? nWidth : N_MIN_BUTTON_WIDTH;
}


bool CPCStringMultibuttonEditor::PlaceEditor( const CTRect<int> &rPlaceRect )
{
	// Each button takes the width its own caption needs; the edit keeps the rest.
	std::vector<int> buttonWidths;
	int nButtonsWidth = 0;
	for ( CPCEditorButtonList::iterator itPCEditorButton = buttonList.begin(); itPCEditorButton != buttonList.end(); ++itPCEditorButton )
	{
		const int nWidth = ( ( *itPCEditorButton ) != 0 ) ? GetButtonWidth( *itPCEditorButton ) : 0;
		buttonWidths.push_back( nWidth );
		nButtonsWidth += nWidth;
	}
	//
	CTRect<int> editRect( rPlaceRect );
	editRect.right -= nButtonsWidth;
	// A value column narrower than its buttons leaves the edit with no width
	// rather than a negative one, which MoveWindow would take as a huge size.
	if ( editRect.right < editRect.left )
	{
		editRect.right = editRect.left;
	}
	MoveWindow( editRect.left, editRect.top, editRect.Width(), editRect.Height(), true );
	int nLeft = rPlaceRect.right - nButtonsWidth;
	size_t nIndex = 0;
	for ( CPCEditorButtonList::iterator itPCEditorButton = buttonList.begin(); itPCEditorButton != buttonList.end(); ++itPCEditorButton, ++nIndex )
	{
		if ( ( *itPCEditorButton ) != 0 )
		{
			CTRect<int> buttonRect( rPlaceRect );
			buttonRect.left = nLeft;
			buttonRect.right = nLeft + buttonWidths[nIndex];
			buttonRect.bottom -= 1;
			( *itPCEditorButton )->MoveWindow( buttonRect.left, buttonRect.top, buttonRect.Width(), buttonRect.Height(), true );
			nLeft += buttonWidths[nIndex];
		}
	}
	return true;
}


bool CPCStringMultibuttonEditor::ActivateEditor( CDialog *pwndActiveDialog )
{
	ShowWindow( SW_SHOW );
	for ( CPCEditorButtonList::iterator itPCEditorButton = buttonList.begin(); itPCEditorButton != buttonList.end(); ++itPCEditorButton )
	{
		if ( ( *itPCEditorButton ) != 0 )
		{
			( *itPCEditorButton )->ShowWindow( SW_SHOW );	
		}
	}
	if ( pwndActiveDialog )
	{
		pwndActiveDialog->GotoDlgCtrl( this );
		return true;
	}
	return false;
}


void CPCStringMultibuttonEditor::SetValue( const CVariant &rValue )
{
	szDefaultValue = rValue.GetStringRecode();
	if ( ::IsWindow( m_hWnd ) )
	{
		SetDefaultValue();
	}
}


void CPCStringMultibuttonEditor::GetValue( CVariant *pValue )
{
	if ( !pValue )
	{
		return;
	}

	CString strText;
	GetWindowText( strText );
	*pValue = std::string( strText );
}


void CPCStringMultibuttonEditor::SetDefaultValue()
{
	CPCItemEditor::SetDefaultValue();
	bCreateControls = true;
	SetWindowText( szDefaultValue.c_str()	);
	bCreateControls = false;
}


bool CPCStringMultibuttonEditor::HandleCommand( unsigned nCommandID, uintptr_t dwData )
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


bool CPCStringMultibuttonEditor::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CPCStringMultibuttonEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CPCStringMultibuttonEditor::UpdateCommand(), pbCheck == 0" );
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


