%option noyywrap

%{
#include "stdafx.h"
#include <malloc.h>
#include "LangToken.h"
#include "lang.tab.h"
#include "../Misc/StrProc.h"

int nyyLineNumber = 1;

//#define YY_ALWAYS_INTERACTIVE 1
#define YY_NEVER_INTERACTIVE 1
int yyReadData( char *pBuf, int nMaxSize );
void yyerror( char *s, ... );

#define YY_INPUT(buf,result,max_size) { (result) = yyReadData( (buf), (max_size) ); }

static char szStringBuf[100000], *pszStringBuf;
%}

%x strstate
%x comment
%x onelinecomment
%x strs
%x wstrs
%x strslash
%x wstrslash
%x onelinecomment_skip
%x default_val

%%

"class"			return CLASS;
"struct"		return STRUCT;
"true"			return TK_TRUE;
"false"			return TK_FALSE;
"{"				return '{';
"}"				return '}';
"["				return '[';
"]"				return ']';
"("				return '(';
")"				return ')';
";"				return ';';
","				return ',';
"="				return '=';
"*"				return '*';
":"				return ':';
"<"				return '<';
">"				return '>';
"typedef"		return TYPEDEF;
"enum"			return ENUM;
"attribute"		return ATTRIBUTE;
"#include"		return INCLUDE;
"baseclass"		return BASE_CLASS;
"basestruct"	return BASE_STRUCT;
"forward"			return FORWARD;
"bool"				return TYPE_BOOL;
"int"					return TYPE_INT;
"float"				return TYPE_FLOAT;
"WORD"				return TYPE_WORD;
"DWORD"				return TYPE_DWORD;
"string"			return TYPE_STRING;
"hexbinary"		return TYPE_HEX;
".."					return DOUBLE_DOT;
"[hExternal]"		return H_EXTERNAL;
"[cppExternal]" return CPP_EXTERNAL;
"SRndInstance"	return SRND_INSTANCE;

"public"		{ yylval.psz = yytext; return VIS_TYPE; }
"private"		{ yylval.psz = yytext; return VIS_TYPE; }
"protected"	{ yylval.psz = yytext; return VIS_TYPE; }

\n						++nyyLineNumber;
[ \t\r]+				/* eat up whitespace */
[A-Za-z_][A-Za-z0-9_]*	{ yylval.psz = yytext; return IDENTIFIER;}

("+"|"-")*[0-9]+(("."[0-9]+("f"?))|"e"(("+"|"-")?)[0-9]+("f"?))?	{
																																		yylval.psz = yytext;
																																		return NUMBER;
																																	}
0"x"[0-9|a-f|A-F]+	{
											yylval.psz = yytext;
											return NUMBER;
										}
						
"/*"					BEGIN(comment);
<comment>[^*\n]*        /* eat anything that's not a '*' */
<comment>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
<comment>\n             ++nyyLineNumber;
<comment>"*"+"/"        BEGIN(INITIAL);

"/""/"[^"/"]						BEGIN(onelinecomment);
<onelinecomment>[^\n]*	{
													yylval.psz = yytext;
													return COMMENT;
												}
<onelinecomment>\n		{ ++nyyLineNumber; BEGIN(INITIAL); }

"///"												BEGIN(onelinecomment_skip);
<onelinecomment_skip>[^\n]*	/* skip */
<onelinecomment_skip>\n			{ ++nyyLineNumber; BEGIN(INITIAL); }

L\"	{
		BEGIN(wstrs);
		pszStringBuf = szStringBuf;
	}
	
<wstrs>{
	\"	{
				*pszStringBuf = 0;
				yylval.psz = szStringBuf;
			
				BEGIN(INITIAL);
				return WSTRING;
		}

	\\	{ BEGIN(wstrslash); /* *pszStringBuf++ = *yytext; */ }
	\n	{ yyerror( "newline in string" ); BEGIN(INITIAL); }
	.		{ *pszStringBuf++ = *yytext; }
}
<wstrslash>{
	\"	{ *pszStringBuf++ = *yytext; BEGIN(wstrs); }
	\\	{ *pszStringBuf++ = *yytext; BEGIN(wstrs); }
	.		{ yyerror( "unrecognised character escape sequence \\%c char", yytext[0] ); BEGIN(wstrs); }
}

\"	{
			BEGIN(strs);
			pszStringBuf = szStringBuf;
	}

<strs>{
	\"	{
			*pszStringBuf = 0;
			yylval.psz = szStringBuf;
			
			BEGIN(INITIAL);
			return STRING;
		}

	\\	{ BEGIN(strslash); /* *pszStringBuf++ = *yytext; */ }
	\n	{ yyerror( "newline in string" ); BEGIN(INITIAL); }
	.	{ *pszStringBuf++ = *yytext; }
}

<strslash>{
	\"	{ *pszStringBuf++ = *yytext; BEGIN(strs); }
	\\	{ *pszStringBuf++ = *yytext; BEGIN(strs); }
	.	{ yyerror( "unrecognized character escape sequence \\%c char", yytext[0] ); BEGIN(strs); }
}
	
!	{
		BEGIN(default_val);
		pszStringBuf = szStringBuf;
	}
<default_val>{
  ! {
			*pszStringBuf = 0;
			yylval.psz = szStringBuf;
				
			BEGIN(INITIAL);
			return DEFAULT_VALUE;
		}
	. { *pszStringBuf++ = *yytext; }
}


.	yyerror( "Unrecognized character: %s", yytext );
%%

//string  \"[^\n\"]+\"

// Called by NLang::OpenFile before each file is scanned. A parse that stops on
// an error leaves the rest of that file in the scanner's buffer, and a file that
// ends inside a comment or a string leaves the scanner in that start condition;
// either way the next file would be scanned as a continuation of the last one.
// yyrestart drops the buffered input and BEGIN puts the start condition back.
void yyResetLexer()
{
	yyrestart( yyin );
	BEGIN(INITIAL);
}
