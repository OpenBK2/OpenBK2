#include "ShimLoader.h"

#if defined( _WIN32 )

#include "Trace.h"

#include <cstdlib>
#include <mutex>
#include <set>
#include <string>

// After everything else, and with the lean macros, because windows.h defines
// min and max as macros and drags in a great deal this file does not use.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace NGr2
{

namespace
{

//! The default name of the DLL to forward to, beside this one.
//!
//! build-libgr2.cmd already moves the game's own granny2_x64.dll aside under
//! exactly this name before installing a replacement, and never overwrites the
//! backup, so a shim dropped in afterwards finds the real DLL where the script
//! left it with nothing further to set up.
const char *DefaultTargetName()
{
	return sizeof( void * ) == 8 ? "granny2_x64.dll.orig" : "granny2.dll.orig";
}

//! The directory this module was loaded from, with a trailing backslash.
//!
//! By the address of a function in it rather than by name, since the module is
//! called granny2_x64.dll and so is what it is standing in for.
std::string ThisDirectory()
{
	HMODULE hModule = 0;
	if ( !GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
	                              | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                          reinterpret_cast<LPCSTR>( &DefaultTargetName ), &hModule ) )
	{
		return std::string();
	}
	char szPath[MAX_PATH] = { 0 };
	const DWORD nLength = GetModuleFileNameA( hModule, szPath, MAX_PATH );
	if ( nLength == 0 || nLength >= MAX_PATH )
	{
		return std::string();
	}
	const std::string sPath( szPath, nLength );
	const size_t nSlash = sPath.find_last_of( "\\/" );
	return nSlash == std::string::npos ? std::string() : sPath.substr( 0, nSlash + 1 );
}

//! The real DLL, loaded on the first call and kept for the life of the process.
HMODULE Real()
{
	static HMODULE hReal = []() -> HMODULE {
#if defined( _MSC_VER )
	#pragma warning( push )
	#pragma warning( disable : 4996 )
#endif
		const char *pszOverride = std::getenv( "LIBGR2_SHIM_TARGET" );
#if defined( _MSC_VER )
	#pragma warning( pop )
#endif
		const std::string sPath = pszOverride != nullptr
		                              ? std::string( pszOverride )
		                              : ThisDirectory() + DefaultTargetName();
		const HMODULE hModule = LoadLibraryA( sPath.c_str() );
		if ( hModule == 0 )
		{
			Logger().error( "shim: cannot load {} (error {}). Every call from here "
			                "returns zero. Set LIBGR2_SHIM_TARGET to the real granny2.",
			                sPath, static_cast<unsigned long>( GetLastError() ) );
		}
		else
		{
			Logger().info( "shim: forwarding to {}", sPath );
		}
		return hModule;
	}();
	return hReal;
}

}

void *ShimEntry( const char *pszName )
{
	const HMODULE hReal = Real();
	if ( hReal == 0 )
	{
		return 0;
	}
	void *pEntry = reinterpret_cast<void *>( GetProcAddress( hReal, pszName ) );
	if ( pEntry == 0 )
	{
		// Once per name. A granny2 that is missing one of the 54 is a different
		// build from the one this was written against, and that is worth saying,
		// but not once per call.
		static std::mutex mutex;
		static std::set<std::string> missing;
		std::lock_guard<std::mutex> lock( mutex );
		if ( missing.insert( pszName ).second )
		{
			Logger().error( "shim: {} is not exported by the real granny2", pszName );
		}
	}
	return pEntry;
}

}

#endif
