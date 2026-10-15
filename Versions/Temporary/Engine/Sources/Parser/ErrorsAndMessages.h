#pragma once

#include "Parser_export.h"

#include <fmt/format.h>

// Reports what arrived when a node is not the type the caller needs, and runs
// statement, which is a return at every call site.
//
// The type is read from pRawNode. It used to be read from pNode, inside the
// branch that runs only when the cast to pNode failed, so pNode was always null
// there: naming the type dereferenced a null pointer on the one path this macro
// exists to handle, and had it survived that it would have named the type the
// cast was looking for rather than the one that came in.
//
// pRawNode is a raw pointer at every call site, so typeid names its dynamic
// type without calling anything. Going through CDynamicCast's operator* instead
// is what made typeid's operand an expression with side effects.
#define CHECK_TYPE( TDesiredType, pRawNode, statement )                                             \
	{                                                                                                 \
		CDynamicCast<TDesiredType> pNode = pRawNode ? pRawNode : 0;                                     \
		if ( pNode == 0 )                                                                               \
		{                                                                                               \
			const std::string szReceived = pRawNode ? typeid( *pRawNode ).name() : "nothing";             \
			std::string szError = fmt::format( "{} expected, {} received\n", #TDesiredType, szReceived ); \
			NErrors::ShowErrorNoLine( szError );                                                          \
			{                                                                                             \
				statement;                                                                                  \
			}                                                                                             \
		}                                                                                               \
	}

namespace NErrors
{
PARSER_EXPORT void ShowWarningNoLine( const std::string &szWarning );
void ShowError( const std::string &szError );
PARSER_EXPORT void ShowErrorNoLine( const std::string &szError );
} // namespace NErrors

// The parser's VERIFY has nothing to do with MFC's or with the one in
// Misc/Asserts.h: it takes three arguments, reports through NErrors rather than
// asserting, and runs `statement` on failure. It has always shadowed the
// one-argument macro of the same name; the #undef only makes that deliberate,
// so that including Misc/Asserts.h first is not a redefinition warning.
#ifdef VERIFY
#undef VERIFY
#endif
#define VERIFY( x, user_text, statement ) \
	{                                       \
		bool bCheck = ( x );                  \
		if ( !bCheck )                        \
		{                                     \
			NErrors::ShowError( user_text );    \
			{                                   \
				statement;                        \
			}                                   \
		}                                     \
	}

#define VERIFY_NOLINE( x, user_text, statement ) \
	{                                              \
		bool bCheck = ( x );                         \
		if ( !bCheck )                               \
		{                                            \
			NErrors::ShowErrorNoLine( user_text );     \
			{                                          \
				statement;                               \
			}                                          \
		}                                            \
	}

#define WARNING_NOLINE( x, user_text )         \
	{                                            \
		bool bCheck = ( x );                       \
		if ( !bCheck )                             \
			NErrors::ShowWarningNoLine( user_text ); \
	}
