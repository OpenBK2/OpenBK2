#pragma once

#include "Misc_export.h"

#include "port/stdcall.h"

#include <string>

// native John Robbins's BSU functions

// our wrapper for BSU - smart asserts

enum EBSUReport
{
	BSU_ABORT,
	BSU_DEBUG,
	BSU_IGNORE,
	BSU_CONTINUE,
};

namespace NBSU
{
	MISC_EXPORT EBSUReport PORT_STDCALL ReportAssert( const char *pszCondition, const char *pszDescription,
		const char *pszFileName, int nLineNumber );
};

// ASSERT macros.
// For showing calling stack when errors occur in major functions.
// Meant to be enabled in release builds.
#if defined( _DO_ASSERT_SLOW )
#define NI_ASSERT( x, user_text )    NI_FORCE_ASSERT( x, #x, user_text )
#define NI_VERIFY( x, user_text, statement )  { bool bCheck = (x); NI_FORCE_ASSERT( bCheck, #x, user_text ); if ( !bCheck ) { statement; } }
#else
#define NI_ASSERT( x, user_text ) ((void)0);
#define NI_VERIFY( x, user_text, statement ) { if ( !( x ) ) { statement; } }
#endif // use ctrl + }
//
// main ASSERT macros
//
#if defined( _DO_ASSERT_SLOW )
#define NI_FORCE_ASSERT( x, msg, user_text )   												\
{																																			\
if ( !(x) )                                                           \
{                                                                     \
	static bool bIgnore;                                                \
	/* the text is a literal from most callers and the std::string that */ \
	/* fmt::format returns from the rest; bind it once so both work     */ \
	const std::string szAssertText( user_text );                        \
	if ( !bIgnore )                                                     \
	{                                                                   \
		if ( IsDebuggerPresent() )                                        \
		{																																	\
			OutputDebugString( szAssertText.c_str() );																	\
			__debugbreak();                                                 \
		}																																	\
		else                                                              \
		{                                                                 \
			switch( NBSU::ReportAssert(msg, szAssertText.c_str(), __FILE__, __LINE__) )\
			{                                                               \
				case BSU_CONTINUE: break;                                     \
				case BSU_DEBUG: __debugbreak(); break;                        \
				case BSU_IGNORE: break;                                       \
				case BSU_ABORT:                                               \
					FatalExit( 0xDEAD );                                        \
					break;																									  	\
			}                                                               \
		}                                                                 \
	}                                                                   \
}																																			\
}
#endif // use ctrl + }


// ************************************************************************************************************************ //
// **
// ** the same as static_cast, but with run-time type checking
// **
// **
// **
// ************************************************************************************************************************ //

template <class TOut, class TPtr>
inline TOut static_cast_ptr( TPtr ptr )
{
	return static_cast<TOut>( ptr.GetPtr() );
}
template <class TOut, class TPtr>
inline TOut dynamic_cast_ptr( TPtr ptr )
{
	return dynamic_cast<TOut>( ptr.GetPtr() );
}
#ifdef _DO_ASSERT_SLOW
template <class TOut, class TIn>
inline TOut checked_cast( TIn obj )
{
	NI_ASSERT( !((obj != 0) && (dynamic_cast<TOut>(obj) == 0)), "Wrong checked cast" );
	return static_cast<TOut>( obj );
}
template <class TOut, class TIn>
inline TOut checked_cast_ptr( TIn ptr )
{
	NI_ASSERT( !((ptr.GetPtr() != 0) && (dynamic_cast<TOut>(ptr.GetPtr()) == 0)), "Wrong checked cast" );
	return static_cast<TOut>( ptr.GetPtr() );
}
#else
#define checked_cast static_cast
#define checked_cast_ptr static_cast_ptr
#endif // _DO_CHECKED_CAST

#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(a) NI_ASSERT(a,#a)
