#include "stdafx.h"

#include "ErrorsAndMessages.h"
#include "FileRead.h"
#include "Misc/StrProc.h"

void yyerror (char *s, ... );
void yyerror_no_line( char *s, ... );
extern int nyyLineNumber;
extern bool bNoTrace;

namespace NErrors
{

 void ShowWarningNoLine( const std::string &szWarning )
{
	printf( "%s\n", szWarning.c_str() );
	std::vector<std::string> strings;
	NStr::SplitString( szWarning, &strings, '\n' );
	for ( int i = 0; i < strings.size(); ++i )
		DbgTrc( "%s", strings[i].c_str() );
}

void ShowError( const std::string &szError )
{
	bNoTrace = true;
	yyerror( const_cast<char*>( szError.c_str() ) );
	bNoTrace = false;

	std::string szErr = StrFmt( "%s(%d) error: %s", NLang::GetParsingFileName(), nyyLineNumber, szError.c_str() );
	std::vector<std::string> strings;
	NStr::SplitString( szErr, &strings, '\n' );
	for ( int i = 0; i < strings.size(); ++i )
		DbgTrc( "%s", strings[i].c_str() );
}

void ShowErrorNoLine( const std::string &szError )
{
	bNoTrace = true;
	yyerror_no_line( const_cast<char*>( szError.c_str() ) );
	bNoTrace = false;

	std::vector<std::string> strings;
	NStr::SplitString( szError, &strings, '\n' );

	for ( int i = 0; i < strings.size(); ++i )
		DbgTrc( "%s", strings[i].c_str() );
}

}


