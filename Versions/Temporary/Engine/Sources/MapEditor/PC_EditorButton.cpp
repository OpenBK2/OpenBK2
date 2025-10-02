#include "stdafx.h"

//#include "..\MapEditorLib\Tools_SysCodes.h"
#include "wmdefines.h"
#include "pc_constants.h"
#include "PC_EditorButton.h"

CPCEditorButton::CPCEditorButton() : pwndTargetWindow( 0 ), pwndNextWindow( 0 ), pwndPreviousWindow( 0 )
{	
}


CPCEditorButton::~CPCEditorButton()
{
}


BEGIN_MESSAGE_MAP(CPCEditorButton, CButton)
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
END_MESSAGE_MAP()


void CPCEditorButton::SetCaption( const string &rszText )
{
	szText = rszText;
	if ( ::IsWindow( m_hWnd ) )
	{
		SetWindowText( szText.c_str() );
	}
}


//CWMMnemonicCodes mnemonicCodes;

BOOL CPCEditorButton::PreTranslateMessage( MSG* pMsg ) 
{
	//DebugTrace( "Message: %s, wParam: 0x%X(%u), lParam: 0x%X\n", mnemonicCodes.Get( pMsg->message ).c_str(), pMsg->wParam, pMsg->wParam, pMsg->lParam );
	if ( pMsg->message == WM_KEYDOWN )	
	{		
		if ( pMsg->wParam == VK_TAB )
		{
			if ( ( GetAsyncKeyState( VK_SHIFT ) & 0x8000 ) > 0 )
			{
				if ( ::IsWindow( m_hWnd ) && pwndPreviousWindow && ::IsWindow( pwndPreviousWindow->m_hWnd ) )
				{
					pwndPreviousWindow->SetFocus();
					return 1;
				}
			}
			else
			{
				if ( ::IsWindow( m_hWnd ) && pwndNextWindow && ::IsWindow( pwndNextWindow->m_hWnd ) )
				{
					pwndNextWindow->SetFocus();
					return 1;
				}
			}
		}
		else if ( pMsg->wParam == VK_LEFT )
		{
			if ( ::IsWindow( m_hWnd ) && pwndPreviousWindow && ::IsWindow( pwndPreviousWindow->m_hWnd ) )
			{
				pwndPreviousWindow->SetFocus();
				return 1;
			}
		}
		else if ( pMsg->wParam == VK_RIGHT )
		{
			if ( ::IsWindow( m_hWnd ) && pwndNextWindow && ::IsWindow( pwndNextWindow->m_hWnd ) )
			{
				pwndNextWindow->SetFocus();
				return 1;
			}
		}
		else if ( pMsg->wParam == VK_ESCAPE )	
		{		
			if ( ::IsWindow( m_hWnd ) && pwndTargetWindow && ::IsWindow( pwndTargetWindow->m_hWnd ) )
			{
				pwndTargetWindow->SendMessage( WM_PC_EDITOR_BUTTON_CHANGE, MAKEWPARAM( EBC_KILL_FOCUS, 0 ), reinterpret_cast<LPARAM>( this ) );
			}
			return 1;
		}	
		else if ( pMsg->wParam == VK_RETURN )	
		{
			if ( ::IsWindow( m_hWnd ) && pwndTargetWindow && ::IsWindow( pwndTargetWindow->m_hWnd ) )
			{
				pwndTargetWindow->SendMessage( WM_PC_EDITOR_BUTTON_CHANGE, MAKEWPARAM( EBC_PRESSED, 0 ), reinterpret_cast<LPARAM>( this ) );
			}
			return 1;
		}
	}	
	return CButton::PreTranslateMessage( pMsg );
}


void CPCEditorButton::OnKillFocus( CWnd* pNewWnd ) 
{	
	CButton::OnKillFocus( pNewWnd );
	if ( ( pNewWnd != pwndTargetWindow ) && ( pNewWnd != pwndNextWindow ) && ( pNewWnd != pwndPreviousWindow ) )
	{
		if ( ::IsWindow( m_hWnd ) && pwndTargetWindow && ::IsWindow( pwndTargetWindow->m_hWnd ) )
		{
			pwndTargetWindow->SendMessage( WM_PC_EDITOR_BUTTON_CHANGE, MAKEWPARAM( EBC_KILL_FOCUS, 0 ), reinterpret_cast<LPARAM>( this ) );
		}
	}
}


void CPCEditorButton::OnClicked() 
{
	if ( ::IsWindow( m_hWnd ) && pwndTargetWindow && ::IsWindow( pwndTargetWindow->m_hWnd ) )
	{
		pwndTargetWindow->SendMessage( WM_PC_EDITOR_BUTTON_CHANGE, MAKEWPARAM( EBC_PRESSED, 0 ), reinterpret_cast<LPARAM>( this ) );
	}
}

// basement storage  


