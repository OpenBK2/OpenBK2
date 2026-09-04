#pragma once

#include <cstdint>
#include <string>

// Make sure the trace this library writes actually goes somewhere.
//
// This is a static library, and a linker takes an object file out of one only to
// resolve a symbol something asked for. logging.cpp defines InitLogging() and an
// object whose constructor calls it, and for as long as nothing referred to
// either, logging.cpp.obj sat in stingray.lib and was never linked into
// anything: the constructor never ran, spdlog's default logger stayed the one
// that writes to stdout at info level, and all 269 trace lines in this library
// were dropped. This is the same hazard the editor modules are shared libraries
// to avoid, noted in AGENTS.md, and it is not a thing a static initialiser can
// solve for itself.
//
// So every source file in this library includes this header, and the object
// below is what refers to InitLogging. One reference from any translation unit
// that does get linked is enough to pull logging.cpp.obj in with it, and
// InitLogging is idempotent, so more than one costs nothing.

void InitLogging();

//! A string argument as the trace should print it, rather than as strlen sees it.
//!
//! Half the "string" pointers this library forwards are not strings:
//!
//!   - a null is ordinary. SEC_TREECLASS::SetItem is called with no text
//!     whenever the mask does not carry TVIF_TEXT, which SetItemImage,
//!     SetItemState and SetItemData all do;
//!   - MAKEINTRESOURCE packs a resource id into the low word of the pointer, and
//!     the editor names three toolbar bitmaps that way;
//!   - a window class name may be an atom, packed the same way;
//!   - LPSTR_TEXTCALLBACK is ( LPTSTR )-1.
//!
//! fmt renders {} on a const char * by reading it. It guards the null case only,
//! and turns that into a thrown format_error rather than into a value, so the
//! line is lost either way. Everything else is a read of an address that is not
//! one, and on Windows the low 64 KB is never mapped, so an integer resource is
//! a reliable access violation *inside the logging of a call*. That puts the
//! fault nowhere near the code that caused it and destroys the one trace line
//! that would have named it, which for a library whose whole purpose is the
//! trace is the worst place to put a crash.
//!
//! Narrow only, which is what LPCTSTR is here: nothing in this build defines
//! _UNICODE, and a wide overload would not match these format strings anyway.
inline std::string SafeString( const char *psz )
{
	if ( psz == nullptr )
	{
		return "<null>";
	}
	if ( psz == reinterpret_cast<const char *>( ~static_cast<uintptr_t>( 0 ) ) )
	{
		// LPSTR_TEXTCALLBACK, spelled without <windows.h>; see below.
		return "<callback>";
	}
	// IS_INTRESOURCE, spelled out so that this header need not pull <windows.h>
	// in ahead of the MFC headers its includers open with.
	const uintptr_t nValue = reinterpret_cast<uintptr_t>( psz );
	if ( ( nValue >> 16 ) == 0 )
	{
		return "#" + std::to_string( nValue );
	}
	return psz;
}

namespace
{

struct SStingrayLoggingInstaller
{
	SStingrayLoggingInstaller() { InitLogging(); }
};

const SStingrayLoggingInstaller g_StingrayLoggingInstaller;

}
