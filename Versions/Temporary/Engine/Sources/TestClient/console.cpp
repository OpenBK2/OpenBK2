#include "stdafx.h"

#include "..\scintilla\scintilla.h"
#include "Console.h"
#include "CommandsInterface.h"

#include "..\Scintilla\SciLexer.h"
#include "..\Misc\StrProc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int MAX_CONSOLE_SIZE = 10000;

enum EStyles
{
	ES_DEFAULT = 32,
	ES_WHITESPACE = 0,
	ES_KEYWORDS = 5,
	ES_DOUBLE_QUOTED_STR = 6,
	ES_SINGLE_QUOTED_STR = 7,
	ES_END_OF_NOT_CLOSED_STR = 12,
	ES_ERROR = 21
};

BEGIN_MESSAGE_MAP(CConsole, CWnd)
	//{{AFX_MSG_MAP(CConsole)
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CConsole::CConsole( CCommandsBase *_pCommands, const string &szWindowName )
: CWnd(), nConsoleSequenceID(0)
{
	m_bShowCalltips = false;

	CreateEx( WS_EX_CLIENTEDGE, "Scintilla", szWindowName.c_str(),
		WS_BORDER | WS_CAPTION | WS_THICKFRAME | WS_VISIBLE,
		50, 50, 550, 550, 0, 0 );

	InitScintilla();
	SetStyles();

	pCommands = _pCommands;

	vector<string> cmds;
	pCommands->GetStringCommands( &cmds );
	SetAutoComplete( cmds );
}

void CConsole::InitScintilla()
{
	m_fnScintilla = (int (*)(void *,int,int,int))SendMessage(SCI_GETDIRECTFUNCTION,0,0);
	m_ptrScintilla = (void *)SendMessage(SCI_GETDIRECTPOINTER,0,0);

	Sci(SCI_SETMARGINWIDTHN, 1, 0);
	//	Sci(SCI_ASSIGNCMDKEY, MAKEWORD( 'F', SCMOD_CTRL ), int message)
}

int CConsole::Sci(int nCmd, int wParam, int lParam)
{
	ASSERT(m_fnScintilla);
	ASSERT(m_ptrScintilla);

	return m_fnScintilla(m_ptrScintilla, nCmd, wParam, lParam);
}

void CConsole::ClearAll()
{
	Sci(SCI_CLEARALL);
}

void CConsole::SetAutoComplete( const vector<string> &vszKeywords )
{
	vszScriptKeywords = vszKeywords;
	string sz;
	for ( vector<string>::const_iterator i = vszKeywords.begin(); i != vszKeywords.end(); ++i )
		sz += *i + " ";
	//
	szAutoComplete = sz;

	//Sci( SCI_AUTOCSETAUTOHIDE, false );
	Sci( SCI_AUTOCSETCANCELATSTART, false );
	Sci( SCI_AUTOCSETFILLUPS, 0, (int)" (" );

	if ( !szAutoComplete.empty() )
		Sci(SCI_SETKEYWORDS, 0, (LPARAM)(&(szAutoComplete[0])) );
}

void CConsole::SetStyles()
{
	//const char font[] = "Verdana";
	const char font[] = "Courier";
	const char monospace[] = "Courier";
	const short fontsize = 10;
	const char keywords[] = "";

	// set style bits, choose the right lexer (Lua) and set the keywords list
	Sci( SCI_SETSTYLEBITS, 5, 0 );
	Sci( SCI_SETLEXER, SCLEX_CPP, 0 );
	Sci( SCI_SETKEYWORDS, 0, (LPARAM)keywords );

	// set up basic features (iguides on, tab=3, tabs-to-spaces, EOL=CRLF)
	Sci( SCI_SETINDENTATIONGUIDES, 1, 0 );
	Sci( SCI_SETTABWIDTH, 4, 0 );
	Sci( SCI_SETUSETABS, 1, 0 );
	Sci( SCI_SETEOLMODE, SC_EOL_CRLF, 0 );

	// now set up the styles (remember you have to set up font name for each style;
	// if you fail to do so, bold/italics will not work (only color will work)
	// !!colors are in format BGR!!

	Sci(SCI_STYLESETFONT, ES_DEFAULT, (LPARAM) font);
	Sci(SCI_STYLESETSIZE, ES_DEFAULT, fontsize);

	Sci(SCI_STYLESETFORE, ES_WHITESPACE, 0x808080);

	Sci(SCI_STYLESETFONT, ES_KEYWORDS, (int)font);
	Sci(SCI_STYLESETSIZE, ES_KEYWORDS, (int)fontsize);
	Sci(SCI_STYLESETFORE, ES_KEYWORDS, 0xDD0000);

	Sci(SCI_STYLESETFORE, ES_DOUBLE_QUOTED_STR, 0x009900 );
	Sci(SCI_STYLESETFORE, ES_SINGLE_QUOTED_STR, 0x009900 );

	Sci(SCI_STYLESETFORE, ES_END_OF_NOT_CLOSED_STR, 0x000000);
	Sci(SCI_STYLESETBACK, ES_END_OF_NOT_CLOSED_STR, 0xE0C0E0);
	Sci(SCI_STYLESETEOLFILLED, ES_END_OF_NOT_CLOSED_STR, 1 );
}

void CConsole::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if ( m_bShowCalltips )
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

void CConsole::AutoComplete()
{
	if ( !Sci( SCI_AUTOCACTIVE ) )
	{
		int currentPos = Sci(SCI_GETCURRENTPOS);
		int nCnt = 0;

		int nStart = Sci( SCI_WORDSTARTPOSITION, currentPos, true );
		int nEnd = Sci( SCI_WORDENDPOSITION, currentPos, true );
		nCnt = nEnd - nStart;

		if ( nCnt <= 1 )
			return;

		char  linebuf[1000];
		TextRange tr;
		tr.chrg.cpMin = nStart;
		tr.chrg.cpMax = nEnd;
		tr.lpstrText = linebuf;
		Sci(SCI_GETTEXTRANGE, 0, long(&tr));

		for ( int i = 0; i < vszScriptKeywords.size(); ++i )
		{
			const string &keyword = vszScriptKeywords[i];
			if ( keyword.find( linebuf ) == 0 )
			{
				Sci( SCI_AUTOCSHOW, nCnt, (LPARAM)szAutoComplete.c_str() );
				break;
			}
		}
	}
}

void CConsole::AddText( const string &szText )
{
	int nTextEnd = Sci( SCI_GETLENGTH );
	if ( nTextEnd > MAX_CONSOLE_SIZE )
	{
		ClearAll();
		nTextEnd = Sci( SCI_GETLENGTH );
	}
	Sci( SCI_APPENDTEXT, szText.size(), (int)szText.c_str() );

	nTextEnd = Sci( SCI_GETLENGTH );
	Sci( SCI_GOTOPOS, nTextEnd );
}

void CConsole::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	const int nLineNum = Sci( SCI_LINEFROMPOSITION, Sci(SCI_GETCURRENTPOS) );
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);

	const int nLineNumAfter = Sci( SCI_LINEFROMPOSITION, Sci(SCI_GETCURRENTPOS) );
	if ( nLineNumAfter == nLineNum + 1 && ( nChar == '\r' || nChar == '\n' ) )
	{
		const int nLineLength = Sci( SCI_LINELENGTH, nLineNum );
		if ( nLineLength > 0 )
		{
			string szLine;
			szLine.resize( nLineLength );
			Sci( SCI_GETLINE, nLineNum,	reinterpret_cast<LPARAM>(szLine.c_str()));

			string szErr;
			if ( !pCommands->LineEntered( szLine, &szErr ) )
				AddText( szErr );
		}
	}

	if ( isalpha( nChar ) )
		AutoComplete();
}

void CConsole::Segment()
{
	IConsoleBuffer *pBuf = Singleton<IConsoleBuffer>();
	IConsoleBuffer::SConsoleLine l;
	while ( pBuf->GetNextLine( &l, &nConsoleSequenceID ) )
	{
		if ( l.nStream == CONSOLE_STREAM_CONSOLE )
			AddText( NStr::ToMBCS( l.szText ) );
	}
}


