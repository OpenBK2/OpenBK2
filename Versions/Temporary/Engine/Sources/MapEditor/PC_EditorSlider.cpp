#include "stdafx.h"

#include "wmdefines.h"
#include "pc_constants.h"
#include "PC_EditorSlider.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPCEditorSlider::CPCEditorSlider() : pwndTargetWindow( 0 ), pwndNextWindow( 0 ), pwndPreviousWindow( 0 )
{	
}


CPCEditorSlider::~CPCEditorSlider()
{
}


BEGIN_MESSAGE_MAP(CPCEditorSlider, CSliderCtrl)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


BOOL CPCEditorSlider::PreTranslateMessage( MSG* pMsg ) 
{
	if ( pMsg->message == WM_KEYDOWN )	
	{		
		if ( pMsg->wParam == VK_TAB )
		{
			if ( ( GetAsyncKeyState( VK_SHIFT ) & 0x8000 ) > 0 )
			{
				if ( pwndPreviousWindow )
				{
					pwndPreviousWindow->SetFocus();
					return 1;
				}
			}
			else
			{
				if ( pwndNextWindow )
				{
					pwndNextWindow->SetFocus();
					return 1;
				}
			}
		}
		else if ( pMsg->wParam == VK_ESCAPE )	
		{		
			if ( pwndTargetWindow )
			{
				pwndTargetWindow->SendMessage( WM_PC_EDITOR_SLIDER_CHANGE, MAKEWPARAM( ESC_KILL_FOCUS, 0 ), reinterpret_cast<LPARAM>( this ) );
			}
			return 1;
		}	
		else if ( pMsg->wParam == VK_RETURN )	
		{
			if ( pwndTargetWindow )
			{
				pwndTargetWindow->SetFocus();
				return 1;
			}
		}
	}	
	return CSliderCtrl::PreTranslateMessage( pMsg );
}


void CPCEditorSlider::OnKillFocus( CWnd* pNewWnd ) 
{	
	CSliderCtrl::OnKillFocus( pNewWnd );
	if ( ( pNewWnd != pwndTargetWindow ) && ( pNewWnd != pwndNextWindow ) && ( pNewWnd != pwndPreviousWindow ) )
	{
		if ( pwndTargetWindow )
		{
			pwndTargetWindow->SendMessage( WM_PC_EDITOR_SLIDER_CHANGE, MAKEWPARAM( ESC_KILL_FOCUS, 0 ), reinterpret_cast<LPARAM>( this ) );
		}
	}
}

// basement storage  


