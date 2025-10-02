#include "StdAfx.h"
#include "TranslateEdit.h"
#include "WMDefines.h"

CTranslateEdit::CTranslateEdit() : bIgnoreSymbol( false )
{
}


CTranslateEdit::~CTranslateEdit()
{
}



BEGIN_MESSAGE_MAP(CTranslateEdit, CEdit)
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
END_MESSAGE_MAP()


void CTranslateEdit::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags ) 
{
	SHORT keyStatus = ::GetKeyState( VK_CONTROL );
	if ( ( keyStatus < 0 ) || ( nChar == VK_F7 ) || ( nChar == VK_F1 ) )
	{
		CWnd *pWnd = GetParent();
		if ( pWnd )
		{
			pWnd = pWnd->GetParent();
			if ( pWnd )
			{
				pWnd = pWnd->GetParent();
				if ( pWnd && ( pWnd->GetSafeHwnd() != 0 ) )
				{
					if ( ( nChar == 'N' ) || ( nChar == 'P' ) || ( nChar == 'F' ) || ( nChar == VK_F7 ) || ( nChar == VK_F1 ) )
					{
						if ( ( nChar != VK_F7 ) || ( nChar == VK_F1 ) )
						{
							bIgnoreSymbol= true;
						}
						pWnd->SendMessage( WM_ELK_TRANSLATE_EDIT_NOTIFY, TEN_NKEYDOWN, nChar );
						return;
					}
				}
			}
		}
	}
	CEdit::OnKeyDown( nChar, nRepCnt, nFlags );
}


void CTranslateEdit::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( bIgnoreSymbol )
	{
		bIgnoreSymbol = false;
		return;
	}
	CEdit::OnChar( nChar, nRepCnt, nFlags );
}


CTranslateButton::CTranslateButton() : bIgnoreSymbol( false )
{
}


CTranslateButton::~CTranslateButton()
{
}



BEGIN_MESSAGE_MAP(CTranslateButton, CButton)
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
END_MESSAGE_MAP()


void CTranslateButton::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags ) 
{
	SHORT keyStatus = ::GetKeyState( VK_CONTROL );
	if ( ( keyStatus < 0 ) || ( nChar == VK_F7 ) || ( nChar == VK_F1 ) )
	{
		CWnd *pWnd = GetParent();
		if ( pWnd )
		{
			pWnd = pWnd->GetParent();
			if ( pWnd )
			{
				pWnd = pWnd->GetParent();
				if ( pWnd && ( pWnd->GetSafeHwnd() != 0 ) )
				{
					if ( ( nChar == 'N' ) || ( nChar == 'P' ) || ( nChar == 'F' ) || ( nChar == VK_F7 ) || ( nChar == VK_F1 ) )
					{
						if ( ( nChar != VK_F7 ) || ( nChar == VK_F1 ) )
						{
							bIgnoreSymbol= true;
						}
						pWnd->SendMessage( WM_ELK_TRANSLATE_EDIT_NOTIFY, TEN_NKEYDOWN, nChar );
						return;
					}
				}
			}
		}
	}
	CButton::OnKeyDown( nChar, nRepCnt, nFlags );
}


void CTranslateButton::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( bIgnoreSymbol )
	{
		bIgnoreSymbol = false;
		return;
	}
	CButton::OnChar( nChar, nRepCnt, nFlags );
}

