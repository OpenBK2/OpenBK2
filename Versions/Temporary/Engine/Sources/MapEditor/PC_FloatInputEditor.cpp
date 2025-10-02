#include "stdafx.h"
#include "../mapeditorlib/resourcedefines.h"
#include "../mapeditorlib/commandhandlerdefines.h"
#include "wmdefines.h"
#include "pc_constants.h"

#include "PC_FloatInputEditor.h"
#include "../Misc/StrProc.h"
#include "../MapEditorLib/StringManager.h"

CPCFloatInputEditor::CPCFloatInputEditor() : fDefaultValue( 0.0f ), nPrecision( PCSV_DEFAULT_RECISION ), bCreateControls( true )
{	
}


CPCFloatInputEditor::~CPCFloatInputEditor()
{
	DestroyWindow();
}


BEGIN_MESSAGE_MAP(CPCFloatInputEditor, CEdit)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_CONTROL_REFLECT(EN_CHANGE, OnEnChange)
END_MESSAGE_MAP()


BOOL CPCFloatInputEditor::PreTranslateMessage( MSG* pMsg ) 
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


void CPCFloatInputEditor::OnSetFocus( CWnd* pOldWnd )
{
	CEdit::OnSetFocus( pOldWnd );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CPCFloatInputEditor::OnKillFocus( CWnd* pNewWnd ) 
{	
	CEdit::OnKillFocus( pNewWnd );
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
	
	if ( GetTargetWindow() )
	{
		GetTargetWindow()->SendMessage( WM_PC_ITEM_CHANGE, MAKEWPARAM( IC_KILL_FOCUS, PC_TEMPORARY_EDITOR ), 0 );
	}
}


void CPCFloatInputEditor::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags ) 
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


void CPCFloatInputEditor::OnEnChange()
{
	if ( !bCreateControls )
	{
		SetValueChanged();
	}
}


// CPCItemEditor

bool CPCFloatInputEditor::CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	bCreateControls = true;
	if ( CPCItemEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		const DWORD dwStyle		= WS_CHILD | ES_AUTOHSCROLL | ES_LEFT;
		const DWORD dwExStyle	= WS_EX_CLIENTEDGE;
		if ( CEdit::Create( dwStyle, CRect( 0, 0, 0, 0 ), GetTargetWindow(), GetControlID() ) )
		{
			ModifyStyleEx( 0, dwExStyle );
			if ( GetTargetWindow() && GetTargetWindow()->GetParent() )
			{
				if ( CFont* pFont = GetTargetWindow()->GetParent()->GetFont() )
				{
					SetFont( pFont );
				}
			}

			string szValues = GetPropertyDesc()->szStringParam;
			NStr::ToLowerASCII( &szValues );
			
			nPrecision = CStringManager::GetIntValueFromString( szValues, PCSPL_PRECISION, 0, PCSP_DIVIDERS, nPrecision );
			if ( nPrecision > PCSV_MAX_RECISION )
			{
				nPrecision = PCSV_DEFAULT_RECISION;
			}
			else if ( nPrecision < 0 )
			{
				nPrecision = PCSV_DEFAULT_RECISION;
			}
			bCreateControls = false;
			return true;
		}
	}
	return false;
}


bool CPCFloatInputEditor::PlaceEditor( const CTRect<int> &rPlaceRect )
{
	MoveWindow( rPlaceRect.left, rPlaceRect.top, rPlaceRect.Width() > 40 ? rPlaceRect.Width() : 40, rPlaceRect.Height(), true );
	return true;
}


bool CPCFloatInputEditor::ActivateEditor( CDialog *pwndActiveDialog )
{
	ShowWindow( SW_SHOW );
	if ( pwndActiveDialog )
	{
		pwndActiveDialog->GotoDlgCtrl( this );
		return true;
	}
	return false;
}


void CPCFloatInputEditor::SetValue( const CVariant &rValue )
{
	fDefaultValue = (float)rValue;
	if ( ::IsWindow( m_hWnd ) )
	{
		SetDefaultValue();
	}
}


void CPCFloatInputEditor::GetValue( CVariant *pValue )
{
	if ( !pValue )
	{
		return;
	}
	//
	CString strText;
	GetWindowText( strText );
	float fValue = fDefaultValue;
	if ( sscanf( strText, "%g", &fValue ) == 1 )
	{
		*pValue = fValue;
	}
	else
	{
		*pValue = fDefaultValue;
	}
}


void CPCFloatInputEditor::SetDefaultValue()
{
	CPCItemEditor::SetDefaultValue();
	bCreateControls = true;
	SetWindowText( CStringManager::GetFloatStringWithPrecision(fDefaultValue, nPrecision).c_str() );
	bCreateControls = false;
}


void CPCFloatInputEditor::EnableEdit( bool bEnable )
{
	CPCItemEditor::EnableEdit( bEnable );
	SetReadOnly( !bEnable );
}


bool CPCFloatInputEditor::HandleCommand( UINT nCommandID, DWORD dwData )
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


bool CPCFloatInputEditor::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
 	NI_ASSERT( pbEnable != 0, "CPCFloatInputEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CPCFloatInputEditor::UpdateCommand(), pbCheck == 0" );
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
			( *pbEnable ) = ( nStartChar != nEndChar );
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_PASTE:
			( *pbEnable ) = ( ( GetStyle() & ES_READONLY ) == 0 ) && ::IsClipboardFormatAvailable( CF_TEXT );
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


