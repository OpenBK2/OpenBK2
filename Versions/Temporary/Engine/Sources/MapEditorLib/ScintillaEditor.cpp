#include "stdafx.h"
#include "Scintilla/Scintilla.h"
#include "ScintillaEditor.h"

#include <cstdint>

CScintillaEditorWindow::CScintillaEditorWindow()
	: pwndStatusStringWindow( 0 ) //, pwndTargetWindow( 0 )
{
}


CScintillaEditorWindow::~CScintillaEditorWindow()
{
}


BEGIN_MESSAGE_MAP(CScintillaEditorWindow, CWnd)
	//ON_WM_SETFOCUS()
	//ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


void CScintillaEditorWindow::SetStatusStringWindow( CWnd* _pwndStatusStringWindow )
{
	pwndStatusStringWindow = _pwndStatusStringWindow;
	UpdateStatusStringWindow();
}


void CScintillaEditorWindow::UpdateStatusStringWindow()
{
	if ( ( pwndStatusStringWindow != 0 ) && ::IsWindow( pwndStatusStringWindow->m_hWnd ) )
	{
	}
}

/**

void CScintillaEditorWindow::OnSetFocus( CWnd* pOldWnd )
{
	CWnd::OnSetFocus( pOldWnd );
	//
}


void CScintillaEditorWindow::OnKillFocus( CWnd* pNewWnd )
{
	CWnd::OnKillFocus( pNewWnd );
	//
}
/**/


BOOL CScintillaEditorWindow::CreateEx(  CWnd* pwndParent, uint32_t dwStyleEx, uint32_t dwStyle, const CRect &rStartRect, unsigned nControlID /**, CWnd *_pwndTargetWindow **/ )
{
	BOOL bCreated = CWnd::CreateEx( dwStyleEx, "Scintilla", "", dwStyle, rStartRect, pwndParent, nControlID );
	if ( !bCreated )
	{
		return FALSE;
	}
	
	//Настраеваем метод посылки команд
	pfnScintilla = ( int( * )( void*, int, int, int ) )( SendMessage( SCI_GETDIRECTFUNCTION, 0, 0 ) );
	pScintilla = ( void* )( SendMessage( SCI_GETDIRECTPOINTER, 0, 0 ) );

	//Скрываем по умолчанию все Margins
	Command( SCI_SETMARGINWIDTHN, 0, 0 );
	Command( SCI_SETMARGINWIDTHN, 1, 0 );
	Command( SCI_SETMARGINWIDTHN, 2, 0 );

	//Ставим по умолчанию CRLF
	Command( SCI_SETEOLMODE, SC_EOL_CRLF );

	// set word wrap
	Command( SCI_SETWRAPMODE, SC_WRAP_WORD );

	//pwndTargetWindow = _pwndTargetWindow;
	return TRUE;
}


int CScintillaEditorWindow::Command( int nCommand, int wParam, int lParam )
{
	NI_ASSERT( pfnScintilla != 0, "CScintillaEditorWindow::Command(): pfnScintilla == 0" );
	NI_ASSERT( pScintilla != 0, "CScintillaEditorWindow::Command(): pScintilla == 0" );

	return pfnScintilla( pScintilla, nCommand, wParam, lParam );
}


void CScintillaEditorWindow::SetText( const string &rszText )
{
	const bool bReadOnly = Command( SCI_GETREADONLY );
	Command( SCI_SETREADONLY, 0, 0 );
	Command( SCI_CLEARALL );
	Command( SCI_ADDTEXT, rszText.size(), (int)( rszText.c_str() ) );
	Command( SCI_EMPTYUNDOBUFFER );
	Command( SCI_SETREADONLY, bReadOnly, 0 );
}


int CScintillaEditorWindow::GetText( string *pszText )
{
	if ( pszText != 0 )
	{
		const int nLength = Command( SCI_GETTEXTLENGTH ) + 1;
		if ( nLength > 1 )
		{
			pszText->resize( nLength );
			Command( SCI_GETTEXT, nLength, (int)( &( ( *pszText )[0] ) ) );
			pszText->resize( nLength - 1 );
		}
		else
		{
			pszText->clear();
		}
		return nLength - 1;
	}
	return 0;
}

// basement storage  


