// TextEditor.cpp : implementation file
//

#include "stdafx.h"

//#include "MapEdit.h"
#include "ScriptEditor.h"
#include "ScriptDictionary.hpp"
//#include "Export.h"
#include "Script/Script.h"
#include <fstream>
//#include "MEParams.h"
//#include "MEUserSettings.h"

//#pragma comment(linker, "/include:_ForceLuaLexer")

#include "MapEditorLib/Interface_UserData.h"


static std::string szErr;
static int ScriptLOG(lua_State* state)
{
	Script script(state);
	Script::Object obj = script.GetObject(script.GetTop());
	std::string sz = obj.GetString();
	for ( std::string::const_iterator i = sz.begin(); i != sz.end(); ++i )
		if ( *i != '\n' )
			szErr += *i;
		else
			szErr += "\r\n";
	return 0;
}

void CScriptEditor::OnEnSelchangeEditText(NMHDR *pNMHDR, LRESULT *pResult)
{
	SELCHANGE *pSelChange = reinterpret_cast<SELCHANGE *>(pNMHDR);
	// TODO:  The control will not send this notification unless you override the
	// CResizeDialog::OnInitDialog() function to send the EM_SETEVENTMASK message
	// to the control with the ENM_SELCHANGE flag ORed into the lParam mask.

	// TODO:  Add your control notification handler code here

	*pResult = 0;
}

// CScriptEditor dialog
IMPLEMENT_DYNAMIC(CScriptEditor, CResizeDialog)
CScriptEditor::CScriptEditor( bool _bInitiallySelected, CWnd* pParent /*=NULL*/)
	: CResizeDialog(CScriptEditor::IDD, pParent), bInitiallySelected(_bInitiallySelected)
{
	bCheckSyntax = true;//false;
	bFreezeUpdate = false;
	bModal = true;
}

CScriptEditor::~CScriptEditor()
{
	//m_fntDef.DeleteObject();
}

void CScriptEditor::EnableEdit( bool bEnable )
{
	bEnableEdit = bEnable;
	if ( ::IsWindow( m_hWnd ) )
	{
		m_LuaEditor.SetReadOnly( !bEnableEdit );
	}
}

void CScriptEditor::CheckSyntax( bool bCheck )
{
	bCheckSyntax = bCheck;
	if ( bCheck && ::IsWindow( m_hWnd ) )
		m_LuaEditor.SetLuaLexer();
}

void CScriptEditor::DoDataExchange(CDataExchange* pDX)
{
	CResizeDialog::DoDataExchange(pDX);
	//DDX_Text(pDX, IDC_EDIT_TEXT, m_szText);
	DDX_Text(pDX, IDC_ERRLOG, m_szErrLog);
	DDX_Control(pDX, IDC_EDIT_TEXT, m_LuaEditor );
	DDX_Control(pDX, IDC_ERRLOG, m_ctrlErrLog);
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDCANCEL, m_ctrlCancel);
}

void CScriptEditor::SetText( const std::string &szText )
{
	if ( !::IsWindow( m_hWnd ) )
	{
		szInitialText = szText;
		return;
	}
	bFreezeUpdate = true;
	m_LuaEditor.ClearAll();
	m_LuaEditor.AddText( szText.c_str() );
	bFreezeUpdate = false;
}

std::string CScriptEditor::GetText()
{
	if ( ::IsWindow( m_LuaEditor ) )
		return m_LuaEditor.GetText();
	return szLastText;
}

BOOL CScriptEditor::OnInitDialog()
{
	CDialog ::OnInitDialog();
	//
	m_LuaEditor.InitScintilla();
	m_LuaEditor.SetEditorMargins();
	//
	CRect r, rText, rErr;
	GetClientRect( &r );
	m_LuaEditor.GetWindowRect( &rText );
	ScreenToClient( &rText );
	ptTextTL = rText.TopLeft();
	//
	m_ctrlErrLog.GetWindowRect( &rErr );
	ScreenToClient( &rErr );
	ptErrorTL = rErr.TopLeft();
	ptErrorTL.y = r.Height() - rErr.top;
	//
	nErrorHeight = rErr.Height();
	nSpaceTextError = rErr.top - rText.bottom;
	//
	CRect rcancel, rok;
	m_ctrlCancel.GetWindowRect( &rcancel );
	m_ctrlOK.GetWindowRect( &rok );
	ScreenToClient( &rcancel );
	ScreenToClient( &rok );

	ptCancel = r.BottomRight() - rcancel.TopLeft();
	ptOK = r.BottomRight() - rok.TopLeft();
	// Create font
	/*
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));			// zero out structure
	lf.lfHeight = 15;							// request a ?-pixel-height font
	strcpy( lf.lfFaceName, "Courier New" );	// request a face name "Arial", "Courier", "MS Sans Serif"
	lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	m_fntDef.CreateFontIndirect(&lf);			// create the fonts
	SetFont( &m_fntDef );
*/
	//
	try
	{
		std::string szKeyWordsFile;
		if ( CPtr<IUserDataContainer> pUserData = Singleton<IUserDataContainer>() )
		{
			szKeyWordsFile = pUserData->Get()->constUserData.szStartFolder + 
				pUserData->Get()->constUserData.propertyControlData.szLUAKeyWordsFileName;
		}
		std::ifstream fKeywords( szKeyWordsFile.c_str() );

		std::string szKeywords;
		std::vector<std::string> vszAutoComplete;

		while ( !fKeywords.bad() && !fKeywords.eof() && !fKeywords.fail() )
		{
			char buf[512], *realStr = buf;
			fKeywords.getline( buf, sizeof(buf) );
			while ( *realStr && isspace( *realStr ) )
				++realStr;

			if ( *realStr )
			{
				szKeywords += realStr;
				szKeywords += ' ';
				vszAutoComplete.push_back( buf );
			}
		}

		m_LuaEditor.AddFunctionNames( szKeywords.c_str(), 1 );

		CPtr<IScriptDictionary> pDictionary = Singleton< IScriptDictionary >();
		if ( pDictionary )
		{
			for ( int i = pDictionary->GetDictionaryCount() - 1; i >= 0; --i )
			{
				std::string szKeywordSet;
				std::vector< std::string > vszDictionary;
				pDictionary->GetKeywords( i, vszDictionary );

				vszAutoComplete.insert( vszAutoComplete.end(),
					vszDictionary.begin(), vszDictionary.end() );
			

				std::vector< std::string >::iterator it = vszDictionary.begin();
				if ( it != vszDictionary.end() )
				{
					while (true)
					{
						szKeywordSet += (*it);
						++it;
						if ( it == vszDictionary.end() )
							break;
						else
							szKeywordSet += ' ';
					}

					m_LuaEditor.AddFunctionNames( szKeywordSet.c_str(), i + 2 );
					m_LuaEditor.SetKeywordColor( i + 2, pDictionary->GetKeywordsColor( i ) );

					szKeywords += szKeywordSet;
					szKeywords += ' ';
				}
			}
		} //if ( pDictionary )
		
		std::string::iterator end = szKeywords.end();
		szKeywords.erase( --end );

		sort( vszAutoComplete.begin(), vszAutoComplete.end() );
		/*m_LuaEditor.AddFunctionNames( szKeywords.c_str(), 1 );/*D. Belyaev*/
		m_LuaEditor.SetAutoComplete( vszAutoComplete, szKeywords );
	}
	catch (...) 
	{
	}

	SetText( szInitialText );
	CheckSyntax( bCheckSyntax );
	CheckSyntax();
	m_LuaEditor.SetReadOnly( !bEnableEdit );
	
	m_LuaEditor.ShowWindow( SW_SHOW );
	m_ctrlErrLog.ShowWindow( SW_SHOW );
	m_ctrlCancel.ShowWindow( SW_SHOW );
	m_ctrlOK.ShowWindow( SW_SHOW );
	
	LoadResizeDialogOptions();
	if ( !szTitle.empty() )
	{
		SetWindowText( szTitle.c_str() );
	}
	if ( IsRestoreSize() && ( resizeDialogOptions.rect.Width() != 0 ) && ( resizeDialogOptions.rect.Height() != 0 ) )
	{
		MoveWindow( resizeDialogOptions.rect.minx,
								resizeDialogOptions.rect.miny,
								resizeDialogOptions.rect.Width(),
								resizeDialogOptions.rect.Height() );
	}
	return true;
}

BEGIN_MESSAGE_MAP(CScriptEditor, CResizeDialog)
	//ON_EN_UPDATE(IDC_EDIT_TEXT, OnEnUpdateEditText)
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_NOTIFY(EN_SELCHANGE, IDC_EDIT_TEXT, OnEnSelchangeEditText)
	ON_NOTIFY(SCN_CHARADDED, IDC_EDIT_TEXT, OnCnCharAdded)
	ON_NOTIFY(SCN_MODIFIED, IDC_EDIT_TEXT, OnCnModified)
	ON_EN_CHANGE(IDC_EDIT_TEXT, OnEnChangeEditText)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CScriptEditor message handlers


void CScriptEditor::OnChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags)
{

	CResizeDialog::OnChar(nChar, nRepCnt, nFlags);
}

void CScriptEditor::OnKeyDown(unsigned nChar, unsigned nRepCnt, unsigned nFlags)
{

	CResizeDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CScriptEditor::OnEnChangeEditText()
{
	CheckSyntax();
	CWnd *pWnd = GetParent();
	if ( pWnd && !bFreezeUpdate )
		pWnd->PostMessage( WM_ME_TEXTCHANGED );
}

void CScriptEditor::CheckSyntax()
{
	if ( !bCheckSyntax || !::IsWindow( m_hWnd ) )
		return;
	if ( 1 )//GetUserSettings().GetParam( ME_SCRIPT_SYNAXCOLORING ) )
	{
		CString str = GetText().c_str();

		//str.Replace( '\r', '\n' );
		Script script( 0, true, ScriptLOG );
		szErr.clear();
//		lua_State *pState = script.GetState();
//		if ( pState->currentThread == 0 )
//			lua_setThread( pState, lua_newThread( pState ) );
		script.ParseBuffer( (LPCSTR)str, str.GetLength() );
		m_ctrlErrLog.SetWindowText( szErr.c_str() );
	}
}

void CScriptEditor::OnOK()
{
	szLastText = GetText();
	if ( bModal )
		CResizeDialog::OnOK();
}

void CScriptEditor::OnCancel()
{
	if ( bModal )
		CResizeDialog::OnCancel();
}

void CScriptEditor::OnCnModified(NMHDR *pNMHDR, LRESULT *pResult)
{
	SCNotification *pNotify = reinterpret_cast<SCNotification*>(pNMHDR);
/*
	if ( bCheckSyntax )
	{
		m_LuaEditor.AutoComplete();
	}
*/
	*pResult = 0;
}

void CScriptEditor::OnCnCharAdded(NMHDR *pNMHDR, LRESULT *pResult)
{
	SCNotification *pNotify = reinterpret_cast<SCNotification*>(pNMHDR);
	char ch = pNotify->ch;

	if ( bCheckSyntax )
	{
		if  (ch  ==  '\r'  ||  ch  ==  '\n')
		{
			m_LuaEditor.NewLineIndent();
		}
		else if ( isalpha( ch ) )
		{
			m_LuaEditor.AutoComplete();
		}
	}

	*pResult = 0;
}

void CScriptEditor::OnSize(unsigned nType, int cx, int cy)
{
	CResizeDialog::OnSize(nType, cx, cy);

	if ( !::IsWindow( m_LuaEditor.m_hWnd ) || !::IsWindow( m_ctrlErrLog.m_hWnd ) )
		return;
	CRect r;
	GetClientRect( &r );

	int nTextBottom = r.Height() - ptErrorTL.y - nSpaceTextError;
	int nWidth = r.Width() - 2 * ptTextTL.x;
	m_LuaEditor.MoveWindow( ptTextTL.x, ptTextTL.y, nWidth, nTextBottom - ptTextTL.y );
	m_ctrlErrLog.MoveWindow( ptTextTL.x, r.Height() - ptErrorTL.y, nWidth, nErrorHeight );
	CPoint pt = r.BottomRight() - ptCancel;
	m_ctrlCancel.SetWindowPos( 0, pt.x, pt.y, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER );
	pt = r.BottomRight() - ptOK;
	m_ctrlOK.SetWindowPos( 0, pt.x, pt.y, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER );
}


