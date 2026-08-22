#include "stdafx.h"

#include "scintilla/scintilla.h"
#include "wmdefines.h"
#include "pc_constants.h"
#include "MapEditorLib/PCIEMnemonics.h"
#include "PC_MultilineStringEditor.h"

#include <cstdint>

CPCMultilineStringEditor::CPCMultilineStringEditor()
{	
}


CPCMultilineStringEditor::~CPCMultilineStringEditor()
{
	DestroyWindow();
}


BEGIN_MESSAGE_MAP(CPCMultilineStringEditor, CScintillaEditorWindow)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
END_MESSAGE_MAP()


BOOL CPCMultilineStringEditor::PreTranslateMessage( MSG* pMsg ) 
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
	return CScintillaEditorWindow::PreTranslateMessage( pMsg );
}


void CPCMultilineStringEditor::OnSetFocus( CWnd* pOldWnd )
{
	CScintillaEditorWindow::OnSetFocus( pOldWnd );
	//
	//Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CPCMultilineStringEditor::OnKillFocus( CWnd* pNewWnd ) 
{	
	CScintillaEditorWindow::OnKillFocus( pNewWnd );
	//
	//Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION );
	//
	if ( IsDefaultValue() )
	{
		string szValue;
		GetText( &szValue );
		if ( szValue.compare( szDefaultValue ) != 0 )
		{
			SetValueChanged();
		}
	}
	if ( GetTargetWindow() )
	{
		GetTargetWindow()->SendMessage( WM_PC_ITEM_CHANGE, MAKEWPARAM( IC_KILL_FOCUS, PC_MULTILINE_STRING_EDITOR ), 0 );
	}
}


void CPCMultilineStringEditor::OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) 
{
	if ( nChar == VK_ESCAPE )
	{		
		SetDefaultValue();
		return;	
	}
	else
	{
		SetValueChanged();
	}
	CScintillaEditorWindow::OnChar( nChar, nRepCnt, nFlags );
}


// CPCItemEditor

bool CPCMultilineStringEditor::CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	return CPCItemEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow );
}


bool CPCMultilineStringEditor::PlaceEditor( const CTRect<int> &rPlaceRect )
{
	MoveWindow( rPlaceRect.left, rPlaceRect.top, rPlaceRect.Width(), rPlaceRect.Height(), true );
	return true;
}


bool CPCMultilineStringEditor::ActivateEditor( CDialog *pwndActiveDialog )
{
	ShowWindow( SW_SHOW );
	if ( pwndActiveDialog )
	{
		pwndActiveDialog->GotoDlgCtrl( this );
		return true;
	}
	return false;
}


void CPCMultilineStringEditor::SetValue( const CVariant &rValue )
{
	szDefaultValue.clear();
	if ( typePCIEMnemonics.IsLeaf( GetItemEditorType() ) )
	{
		if ( !GetPCItemStringValue( &szDefaultValue, rValue, string(), GetItemEditorType(), GetPropertyDesc(), true ) )
		{
			szDefaultValue.clear();
		}
	}
	if ( ::IsWindow( m_hWnd ) )
	{
		SetDefaultValue();
	}
}


void CPCMultilineStringEditor::EnableEdit( bool bEnable )
{
	CPCItemEditor::EnableEdit( bEnable );
	Command( SCI_SETREADONLY, !bEnable, 0 );
}


void CPCMultilineStringEditor::GetValue( CVariant *pValue )
{
	if ( !pValue )
	{
		return;
	}
	//
	string szText;
	GetText( &szText );
	if ( typePCIEMnemonics.IsLeaf( GetItemEditorType() ) )
	{
		CVariant defaultValue;
		GetPCItemValue( &defaultValue, szDefaultValue, CVariant(), GetItemEditorType(), GetPropertyDesc() );
		if ( !GetPCItemValue( pValue, szText, defaultValue, GetItemEditorType(), GetPropertyDesc() ) )
		{
			( *pValue ) = defaultValue;
		}
	}
	else
	{
		( *pValue ) = CVariant();
	}
}


void CPCMultilineStringEditor::SetDefaultValue()
{
	SetText( szDefaultValue.c_str()	);
}


bool CPCMultilineStringEditor::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	/**
	switch( nCommandID )
	{
		default:
			return false;
	}
	/**/
	return false;
}


bool CPCMultilineStringEditor::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CPCMultilineStringEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CPCMultilineStringEditor::UpdateCommand(), pbCheck == 0" );
	//
	/**
	switch( nCommandID )
	{
		default:
			return false;
	}
	/**/
	return false;
}


void CPCMultilineStringEditor::ProcessMessage( unsigned nMessage, WPARAM wParam, LPARAM lParam )
{
	if ( nMessage == WM_ENABLE )
	{
		EnableWindow( wParam > 0 );
		ShowWindow( wParam > 0 ? SW_SHOW : SW_HIDE );
		if ( CWnd *pwndStatusStringWindow = GetStatusStringWindow() )
		{
			if ( ::IsWindow( pwndStatusStringWindow->m_hWnd ) )
			{
				pwndStatusStringWindow->EnableWindow( wParam > 0 );
				pwndStatusStringWindow->ShowWindow( wParam > 0 ? SW_SHOW : SW_HIDE );
			}
		}
	}
}

// basement storage  


