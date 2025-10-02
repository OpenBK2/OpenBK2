#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "wmdefines.h"
#include "pc_constants.h"

#include "PC_StringMultibuttonEditor.h"

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


void CPCStringMultibuttonEditor::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags ) 
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

bool CPCStringMultibuttonEditor::CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	bCreateControls = true;
	if ( CPCItemEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		const DWORD dwStyle							= WS_CHILD | ES_AUTOHSCROLL | ES_LEFT | ( bMultiLine ? ES_MULTILINE : 0 );
		const DWORD dwButtonStyle				= WS_CHILD;
		const DWORD dwExStyle						= WS_EX_CLIENTEDGE;
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
			if ( GetTargetWindow() && GetTargetWindow()->GetParent() )
			{
				if ( CFont* pFont = GetTargetWindow()->GetParent()->GetFont() )
				{
					SetFont( pFont );
					CWnd *pPreviousWindow = this;
					for ( CPCEditorButtonList::iterator itPCEditorButton = buttonList.begin(); itPCEditorButton != buttonList.end(); ++itPCEditorButton )
					{
						if ( ( *itPCEditorButton ) != 0 )
						{
							( *itPCEditorButton )->SetFont( pFont );
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
				}
			}
			bCreateControls = false;
			return true;
		}
	}
	return false;
}


bool CPCStringMultibuttonEditor::PlaceEditor( const CTRect<int> &rPlaceRect )
{
	CTRect<int> editRect( rPlaceRect );
	//
	editRect.right -= 25 * nButtonCount;
	MoveWindow( editRect.left, editRect.top, editRect.Width(), editRect.Height(), true );
	int nButtonIndex = nButtonCount;
	for ( CPCEditorButtonList::iterator itPCEditorButton = buttonList.begin(); itPCEditorButton != buttonList.end(); ++itPCEditorButton )
	{
		if ( ( *itPCEditorButton ) != 0 )
		{
			CTRect<int> buttonRect( rPlaceRect );
			buttonRect.left = rPlaceRect.right - 25 * nButtonIndex;
			--nButtonIndex;
			buttonRect.right = rPlaceRect.right - 25 * nButtonIndex;
			buttonRect.bottom -= 1;
			( *itPCEditorButton )->MoveWindow( buttonRect.left, buttonRect.top, buttonRect.Width(), buttonRect.Height(), true );
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
	*pValue = string( strText );
}


void CPCStringMultibuttonEditor::SetDefaultValue()
{
	CPCItemEditor::SetDefaultValue();
	bCreateControls = true;
	SetWindowText( szDefaultValue.c_str()	);
	bCreateControls = false;
}


bool CPCStringMultibuttonEditor::HandleCommand( UINT nCommandID, DWORD dwData )
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


bool CPCStringMultibuttonEditor::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
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


