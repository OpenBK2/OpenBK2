#pragma once

#include "MapEditorLib/Interface_Widget.h"

#include <cstdint>
#include <string>
#include <vector>

// The editor's two text editing dialogs, behind a boundary that names no
// toolkit.
//
// Both are opened by the property grid, never by anything else: a property
// whose string parameter says "editor:lua" gets the script editor, and any other
// editor gets the plain text editor. That happens in three places --
// PC_TextFileEditor, PC_ExTextFileEditor and PC_StringBigInputEditor -- which
// load the text, run one of these, and decide for themselves what to do with
// what comes back.
//
// Under MFC they are CScriptEditor over IDD_SCRIPT_EDITOR, with CLuaEditor and
// its own Find and Replace dialogs, and CTextEditorDialog over IDD_TEXT_EDITOR.
// Both sit on the Scintilla this tree vendors; the wx ones sit on wxSTC, which
// the wx build keeps for exactly this.
//
// What is not about drawing is shared, below, so the two script editors agree:
// the Lua syntax check that fills the error box, the keyword lists and
// completion words, and the colours the Lua lexer paints with.
namespace NTextEditor
{
	// The script editor. rszTitle replaces the dialog's caption when it is not
	// empty. True on OK with bEnableEdit set, with the text in *pszNewText; a
	// read-only editor still opens and still answers false, as the callers have
	// always treated it.
	bool RunScript( IWidget *pParent, const std::string &rszTitle, const std::string &rszText,
									bool bEnableEdit, std::string *pszNewText );

	// The plain text editor. rszEditor is the "editor:" value the property named,
	// which chooses the file its size and position are remembered in -- "lua" has
	// its own, and everything else shares one.
	bool RunText( IWidget *pParent, const std::string &rszTitle, const std::string &rszEditor,
								const std::string &rszText, bool bEnableEdit, std::string *pszNewText );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunScriptMfc( IWidget *pParent, const std::string &rszTitle, const std::string &rszText,
										 bool bEnableEdit, std::string *pszNewText );
	bool RunTextMfc( IWidget *pParent, const std::string &rszTitle, const std::string &rszEditor,
									 const std::string &rszText, bool bEnableEdit, std::string *pszNewText );
	bool RunScriptWx( IWidget *pParent, const std::string &rszTitle, const std::string &rszText,
										bool bEnableEdit, std::string *pszNewText );
	bool RunTextWx( IWidget *pParent, const std::string &rszTitle, const std::string &rszEditor,
									const std::string &rszText, bool bEnableEdit, std::string *pszNewText );


	// ---- the part that is not drawing ----

	// Parses rszText as Lua and answers what the parser complained about, with
	// line ends as CRLF for an edit box; empty when it parses.
	std::string CheckLuaSyntax( const std::string &rszText );

	// One set of words the Lua lexer colours as a group. Set 1 is the words from
	// the user's keywords file and keeps the lexer's own colour; sets 2 and up
	// come from the script dictionary, each with its own colour.
	struct SKeywordSet
	{
		int nSet;
		std::string szWords;				// space separated, as SCI_SETKEYWORDS takes them
		bool bHasColor;
		uint32_t nColor;						// 0x00BBGGRR
	};

	struct SLuaKeywords
	{
		std::vector<SKeywordSet> sets;
		// What completion offers: every word from every set, sorted, and the same
		// words as the one space-separated list SCI_AUTOCSHOW wants.
		std::vector<std::string> completionWords;
		std::string szCompletionList;
	};

	// The keywords file named in the user data and every script dictionary set.
	// A missing or unreadable file gives no words, not an error.
	void LoadLuaKeywords( SLuaKeywords *pKeywords );

	// How the Lua lexer paints one style. -1 leaves a colour alone; an empty face
	// leaves the font alone.
	struct SLuaStyle
	{
		int nStyle;
		const char *pszFace;
		int nSize;
		int nFore;									// 0x00BBGGRR, or -1
		int nBack;									// 0x00BBGGRR, or -1
		bool bEolFilled;
	};

	// Every style CLuaEditor::SetLuaLexer set, in its order, plus the reserved
	// keywords the lexer's set 0 is given.
	const std::vector<SLuaStyle>& LuaStyles();
	const char* LuaReservedWords();
}
