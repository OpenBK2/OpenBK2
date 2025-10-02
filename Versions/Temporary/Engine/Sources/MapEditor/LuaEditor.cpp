// LuaEditor.cpp : implementation file
//

#include "stdafx.h"
#include "..\scintilla\platform.h"
#include "LuaEditor.h"

#include "..\Scintilla\SciLexer.h"
//#include "MainFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CLuaEditor

CLuaEditor::CLuaEditor()
{
	m_bShowCalltips = TRUE;
	bLastWholeWord = false;
	bLastMatchCase = false;

}

CLuaEditor::~CLuaEditor()
{
}


BEGIN_MESSAGE_MAP(CLuaEditor, CWnd)
	//{{AFX_MSG_MAP(CLuaEditor)
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CLuaEditor::InitScintilla()
{
	m_fnScintilla = (int (*)(void *,int,int,int))
		SendMessage(SCI_GETDIRECTFUNCTION,0,0);
	m_ptrScintilla = (void *)SendMessage(SCI_GETDIRECTPOINTER,0,0);

	Sci(SCI_SETMARGINWIDTHN, 1, 0);
//	Sci(SCI_ASSIGNCMDKEY, MAKEWORD( 'F', SCMOD_CTRL ), int message)
}


// CLuaEditor message handlers

BOOL CLuaEditor::Create(CWnd *pParentWnd, UINT nCtrlId)
{
	BOOL bCreated = CreateEx(0, "Scintilla","", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS,
		CRect(0,0,0,0),pParentWnd,nCtrlId);

	if ( !bCreated )
		return FALSE;

	InitScintilla();

	return TRUE;
}

int CLuaEditor::Sci(int nCmd, int wParam, int lParam)
{
	ASSERT(m_fnScintilla);
	ASSERT(m_ptrScintilla);

	return m_fnScintilla(m_ptrScintilla, nCmd, wParam, lParam);
}


int CLuaEditor::LineFromPoint(CPoint &pt)
{
	return 1+Sci(SCI_LINEFROMPOSITION, Sci(SCI_POSITIONFROMPOINT, pt.x, pt.y), 0);
}

BOOL CLuaEditor::CanUndo()
{
	return Sci(SCI_CANUNDO);
}

void CLuaEditor::Undo()
{
	Sci(SCI_UNDO);
}

BOOL CLuaEditor::CanRedo()
{
	return Sci(SCI_CANREDO);
}

void CLuaEditor::Redo()
{
	Sci(SCI_REDO);
}

void CLuaEditor::SelectAll()
{
	Sci(SCI_SELECTALL);
}

BOOL CLuaEditor::CanCutOrClear()
{
	int currentPos = Sci(SCI_GETCURRENTPOS);
	int anchor = Sci(SCI_GETANCHOR);

	return currentPos != anchor;
}

void CLuaEditor::Cut()
{
	Sci(SCI_CUT);
}

void CLuaEditor::Clear()
{
	Sci(SCI_CLEAR);
}

BOOL CLuaEditor::CanPaste()
{
	return Sci(SCI_CANPASTE);
}

void CLuaEditor::Paste()
{
	Sci(SCI_PASTE);
}

void CLuaEditor::Copy()
{
	Sci(SCI_COPY);
}

void CLuaEditor::GrabFocus()
{
	Sci(SCI_GRABFOCUS);
}

void CLuaEditor::SetEditorMargins()
{
	Sci(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
	int pixelWidth = 6 * Sci(SCI_TEXTWIDTH, STYLE_LINENUMBER, (int)"9");
	Sci(SCI_SETMARGINWIDTHN, 0, pixelWidth);

	Sci(SCI_SETMARGINTYPEN, 1, SC_MARGIN_SYMBOL);
	Sci(SCI_SETMARGINWIDTHN, 1, 10);
	Sci(SCI_SETMARGINSENSITIVEN, 1, TRUE);

	Sci(SCI_MARKERDEFINE, 0, SC_MARK_CIRCLE);
	Sci(SCI_MARKERSETFORE, 0, RGB(0xff, 0x00, 0x00));
	Sci(SCI_MARKERSETBACK, 0, RGB(0xff, 0x00, 0x00));

	Sci(SCI_MARKERDEFINE, 1, SC_MARK_ARROW);

	Sci(SCI_SETVISIBLEPOLICY, CARET_SLOP, 1);
}

void CLuaEditor::SetCallStackMargins()
{
	Sci(SCI_SETMARGINTYPEN, 1, SC_MARGIN_SYMBOL);
	Sci(SCI_SETMARGINWIDTHN, 1, 10);
	Sci(SCI_SETMARGINSENSITIVEN, 1, FALSE);

	Sci(SCI_MARKERDEFINE, 0, SC_MARK_ARROW);
}

void CLuaEditor::SetReadOnly(BOOL bReadOnly)
{
	Sci(SCI_SETREADONLY, bReadOnly);
}

void CLuaEditor::AddText(const char* szText)
{
	Sci(SCI_ADDTEXT, strlen(szText), (int)szText);
}

void CLuaEditor::ClearAll()
{
	Sci(SCI_CLEARALL);
}

CString CLuaEditor::GetLine(int nLine)
{
	CString strLine;
	int nLineLength = Sci(SCI_LINELENGTH, nLine);
	if ( nLineLength>0 )
	{
		char *pszBuf = strLine.GetBuffer(nLineLength);
		Sci(SCI_GETLINE, nLine, (int)pszBuf);
		pszBuf[nLineLength] = '\0';
		strLine.ReleaseBuffer();
	}

	return strLine;
}

void CLuaEditor::GotoLastLine()
{
	int nLine = Sci(SCI_GETLINECOUNT);
	Sci(SCI_GOTOLINE, nLine);
}

int CLuaEditor::GetCurrentLine()
{
	return Sci(SCI_LINEFROMPOSITION, Sci(SCI_GETCURRENTPOS));

}

void CLuaEditor::SetStackTraceLevel(int nLevel)
{
	Sci(SCI_MARKERDELETEALL, 0);
	Sci(SCI_MARKERADD, nLevel, 0);
}

CharacterRange CLuaEditor::GetSelection() 
{
	CharacterRange crange;
	crange.cpMin = Sci(SCI_GETSELECTIONSTART);
	crange.cpMax = Sci(SCI_GETSELECTIONEND);
	return crange;
}

string CLuaEditor::GetText()
{
	int nLength = Sci( SCI_GETTEXTLENGTH ) + 1;
	vector<char> buf( nLength );
	Sci( SCI_GETTEXT, nLength, (int)&buf[0] );
	return &buf[0];
}

BOOL CLuaEditor::PreparePrint(CDC *pDC, CPrintInfo *pInfo)
{
	CharacterRange crange = GetSelection();
	int startPos = crange.cpMin;
	int endPos = crange.cpMax;

	LONG lengthDoc = Sci(SCI_GETLENGTH);
	LONG lengthPrinted = 0;
	LONG lengthDocMax = lengthDoc;

	// Requested to print selection
	if (pInfo->m_pPD->m_pd.Flags & PD_SELECTION) {
		if (startPos > endPos) {
			lengthPrinted = endPos;
			lengthDoc = startPos;
		} else {
			lengthPrinted = startPos;
			lengthDoc = endPos;
		}

		if (lengthPrinted < 0)
			lengthPrinted = 0;
		if (lengthDoc > lengthDocMax)
			lengthDoc = lengthDocMax;
	}

	Sci(SCI_SETWRAPMODE, SC_WRAP_WORD);

	m_pages.RemoveAll();

	RangeToFormat frPrint;
	frPrint.hdc = pDC->GetSafeHdc();
	frPrint.hdcTarget = pDC->m_hAttribDC;
	frPrint.rcPage.left		= frPrint.rc.left	= 0;
	frPrint.rcPage.right	= frPrint.rc.right	= pDC->GetDeviceCaps(HORZRES);
	frPrint.rcPage.top		= frPrint.rc.top	= 0;
	frPrint.rcPage.bottom	= frPrint.rc.bottom = pDC->GetDeviceCaps(VERTRES);

	while (lengthPrinted < lengthDoc) {
		frPrint.chrg.cpMin = lengthPrinted;
		frPrint.chrg.cpMax = lengthDoc;

		m_pages.Add(lengthPrinted);

		lengthPrinted = Sci(SCI_FORMATRANGE, FALSE,
		                           reinterpret_cast<LPARAM>(&frPrint));
	}

	Sci(SCI_FORMATRANGE, FALSE, 0);

	pInfo->SetMaxPage(m_pages.GetSize());

	return TRUE;
}

void CLuaEditor::PrintPage(CDC* pDC, CPrintInfo* pInfo)
{
	RangeToFormat frPrint;
	frPrint.hdc = pDC->GetSafeHdc();
	frPrint.hdcTarget = pDC->m_hAttribDC;
	frPrint.rc.left = pInfo->m_rectDraw.left;
	frPrint.rc.right = pInfo->m_rectDraw.right;
	frPrint.rc.top = pInfo->m_rectDraw.top;
	frPrint.rc.bottom = pInfo->m_rectDraw.bottom;
	frPrint.rcPage.left = pInfo->m_rectDraw.left;
	frPrint.rcPage.right = pInfo->m_rectDraw.right;
	frPrint.rcPage.top = pInfo->m_rectDraw.top;
	frPrint.rcPage.bottom = pInfo->m_rectDraw.bottom;

	frPrint.chrg.cpMin = m_pages[pInfo->m_nCurPage - 1];
	frPrint.chrg.cpMax = Sci(SCI_GETLENGTH);

	Sci(SCI_FORMATRANGE, TRUE, reinterpret_cast<LPARAM>(&frPrint));
}

void CLuaEditor::EndPrint(CDC *pDC, CPrintInfo *pInfo)
{
	Sci(SCI_SETWRAPMODE, SC_WRAP_NONE);
}

void CLuaEditor::AddFunctionNames( const char *pList, int nFuncSet )
{
	Sci( SCI_SETKEYWORDS, nFuncSet, (LPARAM)pList );
}

void CLuaEditor::SetAutoComplete( const vector<string> &vszKeywords,
																 const string &szKeywords )
{
	vszScriptKeywords = vszKeywords;
	/*string sz;
	for ( vector<string>::const_iterator i = vszKeywords.begin(); i != vszKeywords.end(); ++i )
		sz += *i + " ";/*D. Belyaev*/
	//
	szAutoComplete = szKeywords;
	//Sci( SCI_AUTOCSETAUTOHIDE, false );
	Sci( SCI_AUTOCSETCANCELATSTART, false );
	Sci( SCI_AUTOCSETFILLUPS, 0, (int)" (" );
}

void CLuaEditor::SetKeywordColor( int nKeywordSet, DWORD dwColor )
{
	NI_ASSERT( (nKeywordSet > 0) && (nKeywordSet < KEYWORDSET_MAX), "CLuaEditor: KeywordSet counter beyond the bounds " );
	Sci( SCI_STYLESETFORE, SCE_LUA_WORD2 - 1 + nKeywordSet, dwColor );
}

void CLuaEditor::SetLuaLexer()
{
	//const char font[] = "Verdana";
	const char font[] = "Courier";
	const char monospace[] = "Courier";
	const short fontsize = 9;
	const char keywords[] = "and break do else elseif end false for function global if in local nil not or repeat return then true until while";

	// set style bits, choose the right lexer (Lua) and set the keywords list
	Sci(SCI_SETSTYLEBITS,5,0);
	Sci(SCI_SETLEXER,SCLEX_LUA,0);
	Sci(SCI_SETKEYWORDS,0,(LPARAM)keywords);

	// set up basic features (iguides on, tab=3, tabs-to-spaces, EOL=CRLF)
	Sci(SCI_SETINDENTATIONGUIDES,1,0);
	Sci(SCI_SETTABWIDTH,4,0);
	Sci(SCI_SETUSETABS,1,0);
	Sci(SCI_SETEOLMODE,SC_EOL_CRLF,0);

	// now set up the styles (remember you have to set up font name for each style;
	// if you fail to do so, bold/italics will not work (only color will work)
	// !!colors are in format BGR!!

	// style 32: default
	Sci(SCI_STYLESETFONT,32, (LPARAM) font);
	Sci(SCI_STYLESETSIZE,32, fontsize);
	// style 0: whitespace
	Sci(SCI_STYLESETFORE,0, 0x808080);
	// style 1: comment (not used in Lua)
	// style 2: line comment (green)
	Sci(SCI_STYLESETFONT,2, (int)monospace);
	Sci(SCI_STYLESETSIZE,2, fontsize);
	Sci(SCI_STYLESETFORE,2, 0x00AA00);
	// style 3: doc comment (grey???)
	Sci(SCI_STYLESETFORE,3, 0x7F7F7F);      
	// style 4: numbers (blue)
	Sci(SCI_STYLESETFORE,4, 0xFF0000);
	// style 5: keywords (black bold)
	Sci(SCI_STYLESETFONT,5, (int)font);
	Sci(SCI_STYLESETSIZE,5, (int)fontsize);
	Sci(SCI_STYLESETFORE,5, 0xDD0000);
	//Sci(SCI_STYLESETBOLD,5, 1);
	// style SCE_LUA_WORD2: keywords (black bold)
	for ( int i = SCE_LUA_WORD2; i <= SCE_LUA_WORD8; ++i )
	{
		Sci( SCI_STYLESETFONT, i, (int)font );
		Sci( SCI_STYLESETSIZE, i, (int)fontsize );
	}
	Sci( SCI_STYLESETFORE, SCE_LUA_WORD2, 0x803280 );

	// style 6: double qouted strings (???)
	Sci(SCI_STYLESETFORE,6, 0x0099FF);
	// style 7: single quoted strings (???)
	Sci(SCI_STYLESETFORE,7, 0x0099FF);
	// style 8: UUIDs (IDL only, not used in Lua)
	// style 9: preprocessor directives (not used in Lua 4)
	// style 10: operators (black bold)
	Sci(SCI_STYLESETFONT,10, (int)font);
	Sci(SCI_STYLESETSIZE,10, fontsize);
	Sci(SCI_STYLESETFORE,10, 0x000000);
	//Sci(SCI_STYLESETBOLD,10, 1);
	// style 11: identifiers (leave to default)
	// style 12: end of line where string is not closed (black on violet, eol-filled)
	Sci(SCI_STYLESETFORE,12, 0x000000);
	Sci(SCI_STYLESETBACK,12, 0xE0C0E0);
	Sci(SCI_STYLESETEOLFILLED,12, 1);
}


void CLuaEditor::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if ( m_bShowCalltips /*&& pFrame->GetMode()==CMainFrame::modeDebugBreak*/ )
	{
		char  linebuf[1000];
		int  pos  =  Sci(SCI_POSITIONFROMPOINT, point.x, point.y);
		int start = Sci(SCI_WORDSTARTPOSITION, pos, TRUE);
		int end = Sci(SCI_WORDENDPOSITION, pos, TRUE);
		TextRange tr;
		tr.chrg.cpMin = start;
		tr.chrg.cpMax = end;
		tr.lpstrText = linebuf;
		Sci(SCI_GETTEXTRANGE, 0, long(&tr));
		
		CString strCalltip;
		if ( false/*pFrame->GetCalltip(linebuf, strCalltip)*/ )
		{
			if  (Sci(SCI_CALLTIPACTIVE) && m_strCallTip!=strCalltip)
					Sci(SCI_CALLTIPCANCEL);

			if (!Sci(SCI_CALLTIPACTIVE))
			{
				Sci(SCI_CALLTIPSHOW,  start,  (int)strCalltip.GetBuffer(0));
				strCalltip.ReleaseBuffer();
				m_strCallTip = strCalltip;
			};
		}
		else if (Sci(SCI_CALLTIPACTIVE))
					Sci(SCI_CALLTIPCANCEL);
	}
	else if (Sci(SCI_CALLTIPACTIVE))
				Sci(SCI_CALLTIPCANCEL);
	
	CWnd::OnMouseMove(nFlags, point);
}

void CLuaEditor::AutoComplete()
{
	if ( !Sci( SCI_AUTOCACTIVE ) )
	{
		//CString szLine = GetLine( GetCurrentLine() );
		int currentPos = Sci(SCI_GETCURRENTPOS);
		int nCnt = 0;

		int nStart = Sci( SCI_WORDSTARTPOSITION, currentPos, true );
		int nEnd = Sci( SCI_WORDENDPOSITION, currentPos, true );
		nCnt = nEnd - nStart;

		if ( nCnt <= 0 )
			return;

		char  linebuf[1000];
		TextRange tr;
		tr.chrg.cpMin = nStart;
		tr.chrg.cpMax = nEnd;
		tr.lpstrText = linebuf;
		Sci(SCI_GETTEXTRANGE, 0, long(&tr));
		//
		const string szLineBuf( linebuf );
		//
		for ( int i = 0; i < vszScriptKeywords.size(); ++i )
		{
			const string &keyword = vszScriptKeywords[i];
			if ( keyword.compare( 0, szLineBuf.size(), szLineBuf ) == 0 )
			{
				Sci( SCI_AUTOCSHOW, nCnt, (LPARAM)szAutoComplete.c_str() );
				break;
			}
		}
	}
}

void CLuaEditor::NewLineIndent()
{
	char  linebuf[1000];
	int  curLine  =  GetCurrentLine();
	int  lineLength  =  Sci( SCI_LINELENGTH,  curLine );
	if  ( curLine  >  0  &&  lineLength  <=  2 )
	{
		int  prevLineLength = Sci( SCI_LINELENGTH,  curLine  -  1 );
		if ( prevLineLength  <  sizeof(linebuf) )
		{
			WORD  buflen  =  sizeof(linebuf);
			memcpy(linebuf,  &buflen,  sizeof(buflen));
			Sci(SCI_GETLINE,  curLine  -  1,	reinterpret_cast<LPARAM>(static_cast<char  *>(linebuf)));
			linebuf[prevLineLength]  =  '\0';
			for  (int  pos  =  0;  linebuf[pos];  pos++)
			{
				if  (linebuf[pos]  !=  ' '  &&  linebuf[pos]  !=  '\t')
				{
					linebuf[pos]  =  '\0';
					break;
				}
			}
			int nStart = Sci( SCI_POSITIONFROMLINE, curLine );
			int nEnd   = Sci( SCI_GETLINEENDPOSITION, curLine );
			Sci( SCI_SETSEL, nStart, nEnd );
			Sci( SCI_REPLACESEL,  0,  reinterpret_cast<LPARAM>(static_cast<char  *>(linebuf)) );
		}
	}
}

void CLuaEditor::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( ::IsWindow( findDlg.m_hWnd ) && nChar == VK_F3 )
	{
		findDlg.UpdateData();
		if ( szLastTextToFind.empty() )
			Find();
		else
			FindNext( szLastTextToFind, bLastWholeWord, bLastMatchCase );
	}
	if ( 0x8000 & GetAsyncKeyState( VK_CONTROL ) )
	{
		switch ( nChar )
		{
			case 'F':
				Find();
				return;
			case 'H':
				Replace();
				return;
		}
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFindNext::FindNext( const string &szText, bool bWholeWord, bool bMatchCase )
{
	if ( pEditor )
		pEditor->FindNext( szText, bWholeWord, bMatchCase );
}

void CFindNext::Replace( const string &szText, const string &szReplaceWith, bool bWholeWord, bool bMatchCase )
{
	if ( pEditor )
	{
		pEditor->Replace( szReplaceWith );
		pEditor->FindNext( szText, bWholeWord, bMatchCase );
	}
}

void CFindNext::ReplaceAll( const string &szText, const string &szWith, bool bWholeWord, bool bMatchCase )
{
	if ( pEditor )
		pEditor->ReplaceAll( szText, szWith, bWholeWord, bMatchCase );
}

void CLuaEditor::Find()
{
	if ( !::IsWindow( findDlg.m_hWnd ) )
	{
		if ( !findDlg.Create( CFindTextDlg::IDD, this ) )
			return;
	}
	findDlg.SetHandler( new CFindNext( this ) );
	findDlg.ShowWindow( SW_SHOW );
}

void CLuaEditor::FindNext( const string &szText, bool bWholeWord, bool bMatchCase )
{
	CString sz = szText.c_str();
	szLastTextToFind = szText;
	bLastMatchCase = bMatchCase;
	bLastWholeWord = bWholeWord;
	if ( sz.IsEmpty() )
		return;

	int nFlags = 0;
	nFlags |= bMatchCase ? SCFIND_MATCHCASE : 0;
	nFlags |= bWholeWord ? SCFIND_WHOLEWORD : 0;

	int nCurPos = Sci(SCI_GETCURRENTPOS);
	Sci( SCI_SETTARGETSTART, nCurPos );
	Sci( SCI_SETTARGETEND, Sci(SCI_GETTEXTLENGTH) );
	Sci( SCI_SETSEARCHFLAGS, nFlags );
	int nPos = Sci( SCI_SEARCHINTARGET, sz.GetLength(), (int)((LPCSTR)sz) );
	if ( nPos == -1 && nCurPos > 0 )
	{
		Sci( SCI_SETTARGETSTART, 0 );
		nPos = Sci( SCI_SEARCHINTARGET, sz.GetLength(), (int)((LPCSTR)sz) );
	}
	//
	if ( nPos >= 0 )
		Sci( SCI_SETSEL, nPos, nPos + sz.GetLength() );
}

void CLuaEditor::Replace()
{
	if ( !::IsWindow( replaceDlg.m_hWnd ) )
	{
		if ( !replaceDlg.Create( CReplaceTextDlg::IDD, this ) )
			return;
	}
	replaceDlg.SetHandler( new CFindNext( this ) );
	replaceDlg.ShowWindow( SW_SHOW );
}

bool CLuaEditor::Replace( const string &szReplaceWith )
{
	CharacterRange cr = GetSelection();

	if ( abs( cr.cpMax - cr.cpMin ) == 0 )
		return false;
	Sci( SCI_REPLACESEL, 0, (int)szReplaceWith.c_str() );
	return true;
}

void CLuaEditor::ReplaceAll( const string &szText, const string &szWith, bool bWholeWord, bool bMatchCase )
{
	CString sz = szText.c_str();
	szLastTextToFind = szText;
	bLastMatchCase = bMatchCase;
	bLastWholeWord = bWholeWord;
	if ( sz.IsEmpty() )
		return;

	int nFlags = 0;
	nFlags |= bMatchCase ? SCFIND_MATCHCASE : 0;
	nFlags |= bWholeWord ? SCFIND_WHOLEWORD : 0;

	int nCurPos = 0;
	Sci( SCI_SETTARGETSTART, nCurPos );
	Sci( SCI_SETTARGETEND, Sci(SCI_GETTEXTLENGTH) );
	Sci( SCI_SETSEARCHFLAGS, nFlags );

	int nPos = -1;
	while( (nPos = Sci( SCI_SEARCHINTARGET, sz.GetLength(), (int)((LPCSTR)sz) )) >= 0 )
	{
		Sci( SCI_SETTARGETSTART, nPos );
		Sci( SCI_SETTARGETEND, nPos + sz.GetLength() );
		Sci( SCI_REPLACETARGET, szWith.size(), (int)szWith.c_str() );
		//
		Sci( SCI_SETTARGETSTART, nPos + szWith.size() );
		Sci( SCI_SETTARGETEND, Sci(SCI_GETTEXTLENGTH) );
		Sci( SCI_SETSEARCHFLAGS, nFlags );
	}
}



