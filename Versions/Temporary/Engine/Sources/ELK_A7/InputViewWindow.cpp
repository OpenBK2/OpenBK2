#include "StdAfx.h"
#include "resource.h"

#include "InputViewWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CInputViewWindow::CInputViewWindow() : bGameExists( false )
{
}


CInputViewWindow::~CInputViewWindow()
{
}


BEGIN_MESSAGE_MAP(CInputViewWindow,CWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

void CInputViewWindow::SetMainFrameWindow( CWnd *_pwndMainFrame )
{
	wndForm.pwndMainFrame = _pwndMainFrame;
}


BOOL CInputViewWindow::PreCreateWindow( CREATESTRUCT& cs ) 
{
	if ( !CWnd::PreCreateWindow( cs ) )
	{
		return false;
	}

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass( CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, ::LoadCursor( 0, IDC_ARROW ), HBRUSH( COLOR_WINDOW + 1 ), 0 );

	return true;
}


void CInputViewWindow::OnPaint() 
{
	CPaintDC dc(this);
	//...
	// Do not call CWnd::OnPaint() for painting messages
}


int CInputViewWindow::OnCreate( LPCREATESTRUCT lpCreateStruct ) 
{
	if (CWnd ::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if ( wndForm.Create( bGameExists ? IDD_INPUT_VIEW : IDD_INPUT_VIEW_NO_GAME, this ) )
	{
		wndForm.ShowWindow( SW_SHOW );
	}
	
	return 0;
}


void CInputViewWindow::OnDestroy() 
{
	if( wndForm.GetSafeHwnd() != 0 )
	{
		wndForm.DestroyWindow();
	}
	CWnd::OnDestroy();
}


void CInputViewWindow::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize( nType, cx, cy );
	
	if( wndForm.GetSafeHwnd() != 0 )
	{
		wndForm.MoveWindow( 0, 0, cx, cy );
	}
}


void CInputViewWindow::OnSetFocus(CWnd* pOldWnd) 
{
	wndForm.SetFocus();
	if ( CEdit *pWnd = static_cast<CEdit*>( wndForm.GetDlgItem( IDC_IV_TRANSLATE_EDIT ) ) )
	{
		pWnd->SetSel( 0, 0 );
	}
}


void CInputViewWindow::ClearControls()
{
	CString strEmptyString;
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_ORIGINAL_EDIT ) )
	{
		pWnd->SetWindowText( strEmptyString );
	}
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_ORIGINAL_DESCRIPTION_EDIT ) )
	{
		pWnd->SetWindowText( strEmptyString );
	}
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_TRANSLATE_EDIT ) )
	{
		pWnd->SetWindowText( strEmptyString );
	}

	EnableControlsForText( false );
}


void CInputViewWindow::EnableControlsForText( bool bEnable )
{
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_ORIGINAL_EDIT ) )
	{
		pWnd->EnableWindow( bEnable );
	}
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_TRANSLATE_EDIT ) )
	{
		pWnd->EnableWindow( bEnable );
	}
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON ) )
	{
		pWnd->EnableWindow( bEnable );
	}
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_TRANSLATED_RADIO_BUTTON ) )
	{
		pWnd->EnableWindow( bEnable );
	}
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_APPROVED_RADIO_BUTTON ) )
	{
		pWnd->EnableWindow( bEnable );
	}
}


void CInputViewWindow::EnableControlsForFolder( bool bEnable )
{
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON ) )
	{
		pWnd->EnableWindow( bEnable );
	}
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_TRANSLATED_RADIO_BUTTON ) )
	{
		pWnd->EnableWindow( bEnable );
	}
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_APPROVED_RADIO_BUTTON ) )
	{
		pWnd->EnableWindow( bEnable );
	}
}


void CInputViewWindow::EnableNextButton( bool bEnable )
{
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_NEXT_BUTTON ) )
	{
		pWnd->EnableWindow( bEnable );
	}
}


void CInputViewWindow::EnableBackButton( bool bEnable )
{
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_BACK_BUTTON ) )
	{
		pWnd->EnableWindow( bEnable );
	}
}


void CInputViewWindow::SetOriginalText( const CString &rstrText )
{
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_ORIGINAL_EDIT ) )
	{
		pWnd->SetWindowText( rstrText );
	}
}


void CInputViewWindow::SetTranslatedText( const CString &rstrText )
{
	if ( CEdit *pWnd = static_cast<CEdit*>( wndForm.GetDlgItem( IDC_IV_TRANSLATE_EDIT ) ) )
	{
		pWnd->SetWindowText( rstrText );
		wndForm.bTranslatedTextChanged = false;
		wndForm.bManualState = false;
		wndForm.strInitialTranslatedText = rstrText;
	}
}


void CInputViewWindow::SetDescription( const CString &rstrText )
{
	if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_ORIGINAL_DESCRIPTION_EDIT ) )
	{
		pWnd->SetWindowText( rstrText );
	}
}


void CInputViewWindow::SetState( int nState )
{
	wndForm.CheckRadioButton( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON, IDC_IV_APPROVED_RADIO_BUTTON, IDC_IV_NOT_TRANSLATED_RADIO_BUTTON + nState );
	wndForm.nInitialState = nState;
}


void CInputViewWindow::GetTranslatedText( CString *pstrText )
{
	NI_ASSERT( pstrText != 0, StrFmt( _T( "CInputViewWindow::GetTranslatedText() wrong parameter: pstrText %x" ), pstrText ) );
	if ( pstrText )
	{
		pstrText->Empty();
		if ( CWnd *pWnd = wndForm.GetDlgItem( IDC_IV_TRANSLATE_EDIT ) )
		{
			pWnd->GetWindowText( ( *pstrText ) );
		}
	}
}


int CInputViewWindow::GetState()
{
	return wndForm.GetCheckedRadioButton( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON, IDC_IV_APPROVED_RADIO_BUTTON ) - IDC_IV_NOT_TRANSLATED_RADIO_BUTTON;
}


CEdit* CInputViewWindow::GetTranslateEdit()
{
	return static_cast<CEdit*>( wndForm.GetDlgItem( IDC_IV_TRANSLATE_EDIT ) );
}


void CInputViewWindow::SelectText( const SMainFrameParams::SSearchParam &rSearchParam )
{
	CEdit *pWnd = 0;
	if ( rSearchParam.nWindowType == SMainFrameParams::SSearchParam::WT_ORIGINAL )
	{
		pWnd = static_cast<CEdit*>( wndForm.GetDlgItem( IDC_IV_ORIGINAL_EDIT ) );
	}
	else if ( rSearchParam.nWindowType == SMainFrameParams::SSearchParam::WT_DESCRIPTION )
	{
		pWnd = static_cast<CEdit*>( wndForm.GetDlgItem( IDC_IV_ORIGINAL_DESCRIPTION_EDIT ) );
	}
	else
	{
		pWnd = static_cast<CEdit*>( wndForm.GetDlgItem( IDC_IV_TRANSLATE_EDIT ) );
	}
	if ( pWnd )
	{
		pWnd->SetSel( rSearchParam.nPosition, rSearchParam.nPosition + rSearchParam.strFindText.GetLength() );
		pWnd->SetActiveWindow();
	}
}



void CInputViewWindow::LoadGameImage( const string &rszGameImagePath )
{
	wndForm.LoadGameImage( rszGameImagePath );
	//wndForm.UpdateWindow();
}


