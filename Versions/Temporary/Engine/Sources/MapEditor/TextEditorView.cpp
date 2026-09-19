#include "stdafx.h"

#include "TextEditorView.h"
#include "ScriptDictionary.hpp"

#include "MapEditorLib/Interface_UserData.h"
#include "Script/Script.h"

#include <algorithm>
#include <cctype>
#include <fstream>

// The part of the script editor that is not drawing: the syntax check, the
// keywords and the styles, which the editors (TextEditorViewWx.cpp) use.

namespace
{
	// The Lua lexer's keyword-list styles, SCE_LUA_WORD2 to SCE_LUA_WORD8 in
	// Scintilla's SciLexer.h (wxSTC_LUA_WORD2 to wxSTC_LUA_WORD8 in wxSTC's).
	// Written out here since the bundled Scintilla went: they are the lexer's
	// numbers and do not change.
	const int N_LUA_STYLE_WORD2 = 13;
	const int N_LUA_STYLE_WORD8 = 19;

	// What the Lua parser says, collected the way CScriptEditor's ScriptLOG
	// collected it: each complaint appended, LF turned into CRLF for an edit box.
	// A file-scope string because the parser calls back through a plain function
	// pointer with nothing of ours in it.
	std::string szSyntaxErrors;

	int CollectSyntaxError( lua_State *pState )
	{
		Script script( pState );
		Script::Object obj = script.GetObject( script.GetTop() );
		const std::string sz = obj.GetString();
		for ( std::string::const_iterator it = sz.begin(); it != sz.end(); ++it )
		{
			if ( *it != '\n' )
			{
				szSyntaxErrors += *it;
			}
			else
			{
				szSyntaxErrors += "\r\n";
			}
		}
		return 0;
	}
}


namespace NTextEditor
{
	std::string CheckLuaSyntax( const std::string &rszText )
	{
		Script script( 0, true, CollectSyntaxError );
		szSyntaxErrors.clear();
		script.ParseBuffer( rszText.c_str(), static_cast<int>( rszText.size() ) );
		return szSyntaxErrors;
	}


	void LoadLuaKeywords( SLuaKeywords *pKeywords )
	{
		if ( pKeywords == 0 )
		{
			return;
		}
		pKeywords->sets.clear();
		pKeywords->completionWords.clear();
		pKeywords->szCompletionList.clear();
		// CScriptEditor::OnInitDialog, line for line, except that it handed each
		// piece to the control as it went and this writes them down instead.
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
			while ( !fKeywords.bad() && !fKeywords.eof() && !fKeywords.fail() )
			{
				char buf[512], *realStr = buf;
				fKeywords.getline( buf, sizeof( buf ) );
				while ( *realStr && isspace( static_cast<unsigned char>( *realStr ) ) )
				{
					++realStr;
				}
				if ( *realStr )
				{
					szKeywords += realStr;
					szKeywords += ' ';
					// The whole line, leading blanks and all, as the MFC editor kept it.
					pKeywords->completionWords.push_back( buf );
				}
			}

			// Set 1: the keywords file, in the lexer's own colour.
			SKeywordSet fileSet;
			fileSet.nSet = 1;
			fileSet.szWords = szKeywords;
			fileSet.bHasColor = false;
			fileSet.nColor = 0;
			pKeywords->sets.push_back( fileSet );

			// Sets 2 and up: the script dictionary, highest first, each in its own
			// colour.
			CPtr<IScriptDictionary> pDictionary = Singleton<IScriptDictionary>();
			if ( pDictionary )
			{
				for ( int i = pDictionary->GetDictionaryCount() - 1; i >= 0; --i )
				{
					std::vector<std::string> vszDictionary;
					pDictionary->GetKeywords( i, vszDictionary );
					pKeywords->completionWords.insert( pKeywords->completionWords.end(),
																						 vszDictionary.begin(), vszDictionary.end() );
					if ( vszDictionary.empty() )
					{
						continue;
					}
					std::string szKeywordSet;
					for ( std::vector<std::string>::iterator it = vszDictionary.begin(); it != vszDictionary.end(); ++it )
					{
						if ( it != vszDictionary.begin() )
						{
							szKeywordSet += ' ';
						}
						szKeywordSet += *it;
					}
					SKeywordSet dictionarySet;
					dictionarySet.nSet = i + 2;
					dictionarySet.szWords = szKeywordSet;
					dictionarySet.bHasColor = true;
					dictionarySet.nColor = pDictionary->GetKeywordsColor( i );
					pKeywords->sets.push_back( dictionarySet );

					szKeywords += szKeywordSet;
					szKeywords += ' ';
				}
			}

			// The trailing space goes. Guarded, where the MFC editor was not: with
			// no file and no dictionary it erased before the start of an empty
			// string.
			if ( !szKeywords.empty() )
			{
				szKeywords.erase( szKeywords.size() - 1 );
			}
			std::sort( pKeywords->completionWords.begin(), pKeywords->completionWords.end() );
			pKeywords->szCompletionList = szKeywords;
		}
		catch ( ... )
		{
		}
	}


	const std::vector<SLuaStyle>& LuaStyles()
	{
		// CLuaEditor::SetLuaLexer's styles, in its order. Colours are 0x00BBGGRR,
		// as its comment says -- "!!colors are in format BGR!!".
		static std::vector<SLuaStyle> styles;
		if ( styles.empty() )
		{
			const char *const pszFont = "Courier";
			const int nSize = 9;
			const SLuaStyle table[] =
			{
				{ 32, pszFont, nSize, -1, -1, false },				// default
				{ 0, nullptr, 0, 0x808080, -1, false },				// whitespace
				{ 2, pszFont, nSize, 0x00AA00, -1, false },		// line comment
				{ 3, nullptr, 0, 0x7F7F7F, -1, false },				// doc comment
				{ 4, nullptr, 0, 0xFF0000, -1, false },				// numbers
				{ 5, pszFont, nSize, 0xDD0000, -1, false },		// keywords
			};
			styles.assign( table, table + sizeof( table ) / sizeof( table[0] ) );
			for ( int nStyle = N_LUA_STYLE_WORD2; nStyle <= N_LUA_STYLE_WORD8; ++nStyle )
			{
				const SLuaStyle word = { nStyle, pszFont, nSize, -1, -1, false };
				styles.push_back( word );
			}
			const SLuaStyle rest[] =
			{
				{ N_LUA_STYLE_WORD2, nullptr, 0, 0x803280, -1, false },
				{ 6, nullptr, 0, 0x0099FF, -1, false },				// double quoted strings
				{ 7, nullptr, 0, 0x0099FF, -1, false },				// single quoted strings
				{ 10, pszFont, nSize, 0x000000, -1, false },	// operators
				{ 12, nullptr, 0, 0x000000, 0xE0C0E0, true },	// unclosed string at end of line
			};
			styles.insert( styles.end(), rest, rest + sizeof( rest ) / sizeof( rest[0] ) );
		}
		return styles;
	}


	const char* LuaReservedWords()
	{
		return "and break do else elseif end false for function global if in local nil not or "
					 "repeat return then true until while";
	}
}
