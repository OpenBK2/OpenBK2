#pragma once

#include "Misc_export.h"

#include "port/debugging.h"
#include "port/stdcall.h"

#include <string>

// Declared here rather than by including Misc/Tools.h, which is where it lives:
// Tools.h uses ASSERT, so including it from this header forms a cycle and leaves
// whichever of the two the translation unit reaches first without the other.
MISC_EXPORT void PORT_STDCALL DbgTrcRaw( const char *pszText );

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
		if ( is_debugger_present() )                                      \
		{																																	\
			DbgTrcRaw( szAssertText.c_str() );																	\
			breakpoint();                                                   \
		}																																	\
		else                                                              \
		{                                                                 \
			switch( NBSU::ReportAssert(msg, szAssertText.c_str(), __FILE__, __LINE__) )\
			{                                                               \
				case BSU_CONTINUE: break;                                     \
				case BSU_DEBUG: breakpoint(); break;                          \
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

// ************************************************************************************************************************ //
// **
// ** VERIFY
// **
// ** MFC spells it
// **     #ifdef _DEBUG
// **     #define VERIFY( f )  ASSERT( f )
// **     #else
// **     #define VERIFY( f )  ( ( void )( f ) )
// **     #endif
// ** so f is evaluated in *both* configurations. That is the whole reason to
// ** write VERIFY rather than ASSERT: the expression is the work, and the check
// ** is only a check. The ASSERT redefinition immediately above breaks that
// ** guarantee -- NI_ASSERT without _DO_ASSERT_SLOW is ( ( void )0 ) and names
// ** its argument nowhere -- so under this header MFC's VERIFY quietly stopped
// ** calling whatever it was handed. It cost the map editor its toolbar bitmaps,
// ** and it did so with no warning and no symbol in the object file.
// **
// ** Since Misc/Asserts.h is included after <afxwin.h> everywhere that has one,
// ** replacing the macro here is what puts the guarantee back. This definition
// ** also holds where MFC does not exist at all, which is the point of doing it
// ** here rather than leaving it to <afx.h>:
// **
// **   - f is evaluated exactly once, in every configuration and on every
// **     platform;
// **   - the result is asserted on through NI_ASSERT, so it reports through the
// **     same path as the rest of this header and compiles away with it;
// **   - the result is handed back, so `if ( VERIFY( x ) )` is legal. MFC's
// **     release form is ( ( void )( f ) ) and cannot be used that way, so this
// **     is a superset of what callers may already assume.
// **
// ** It is an expression rather than a { } block on purpose: a block requires
// ** its callers to be statements, which is what NI_VERIFY above demands and
// ** MFC's VERIFY does not. The lambda is what makes an expression out of
// ** NI_ASSERT, which is a statement; it captures nothing, because f is
// ** evaluated by the caller and passed in.
// **
// ************************************************************************************************************************ //
#ifdef VERIFY
#undef VERIFY
#endif
#define VERIFY( f )                                                                             \
	( []( const bool bVerified ) -> bool                                                           \
		{                                                                                           \
			NI_ASSERT( bVerified, "VERIFY( " #f " ) failed" );                                       \
			return bVerified;                                                                       \
		}( !!( f ) ) )
