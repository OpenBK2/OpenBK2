#pragma once

#include "Parser_export.h"

#include <fmt/format.h>

#define CHECK_TYPE( TDesiredType, pRawNode, statement )\
{\
	CDynamicCast< TDesiredType > pNode = pRawNode ? pRawNode : 0;\
	if ( pNode == 0 )\
	{\
		std::string szError = fmt::format( "{} expected, {} recieved\n", #TDesiredType, typeid( *pNode ).name() );\
		NErrors::ShowErrorNoLine( szError );\
		{ statement; }\
	}\
}

namespace NErrors
{
	PARSER_EXPORT void ShowWarningNoLine( const std::string &szWarning );
	void ShowError( const std::string &szError );
	PARSER_EXPORT void ShowErrorNoLine( const std::string &szError );
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


