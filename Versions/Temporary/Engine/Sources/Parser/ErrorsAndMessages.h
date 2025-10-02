#pragma once

#include "Parser_export.h"


#define CHECK_TYPE( TDesiredType, pRawNode, statement )\
{\
	CDynamicCast< TDesiredType > pNode = pRawNode ? pRawNode : 0;\
	if ( pNode == 0 )\
	{\
		string szError = StrFmt( "%s expected, %s recieved\n", #TDesiredType, typeid( *pNode ).name() );\
		NErrors::ShowErrorNoLine( szError );\
		{ statement; }\
	}\
}

namespace NErrors
{
	PARSER_EXPORT void ShowWarningNoLine( const string &szWarning );
	void ShowError( const string &szError );
	PARSER_EXPORT void ShowErrorNoLine( const string &szError );
}

#define VERIFY( x, user_text, statement )\
{\
	bool bCheck = (x);\
	if ( !bCheck )\
	{\
		NErrors::ShowError( user_text );\
		{ statement; }\
	}\
}

#define VERIFY_NOLINE( x, user_text, statement )\
{\
	bool bCheck = (x);\
	if ( !bCheck )\
	{\
		NErrors::ShowErrorNoLine( user_text );\
		{ statement; }\
	}\
}

#define WARNING_NOLINE( x, user_text )\
{\
	bool bCheck = (x);\
	if ( !bCheck )\
		NErrors::ShowWarningNoLine( user_text );\
}


extern bool bInTestMode;

inline void Msg( char *s, ... )
{
	if ( !bInTestMode )
	{
		static char buff[10000];
		va_list va;
		va_start( va, s );
		vsprintf( buff, s, va );
		va_end( va );
		printf( "%s\n", buff );
	}
}


