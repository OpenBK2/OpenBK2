#pragma once

// Finding the real granny2, for the forwarding shim to call through to.
//
// Shim.cpp is generated and mechanical: 54 functions that log their arguments
// and hand them on. This is the part that is not mechanical, so it lives beside
// the generator's output rather than in it.
//
// The point of the shim is a second recording. libgr2 writes a log of what the
// engine asked it to do; drop this in as granny2_x64.dll instead, with the real
// DLL renamed alongside, and the same play produces the same log from the
// implementation that shipped. The two then diff, and the difference is what
// this library gets wrong. Everything that makes the diff possible is in
// Trace.h and Identify.h and is shared between the two: stable object ids
// instead of addresses, and files named by the hash of their bytes.
//
// Windows only, and deliberately so. The real granny2 is a Windows DLL; there is
// nothing to forward to anywhere else.

#if defined( _WIN32 )

namespace NGr2
{

//! One entry point of the real granny2, or null if it or the DLL is missing.
//!
//! Which DLL: LIBGR2_SHIM_TARGET if it is set, otherwise granny2_x64.dll.orig
//! (granny2.dll.orig on a 32-bit build) in the directory this module was loaded
//! from, which is the name build-libgr2.cmd's backup already uses. The failure
//! to find either is logged once, at error, and then every call returns a zero
//! rather than taking the game down.
void *ShimEntry( const char *pszName );

}

#endif
