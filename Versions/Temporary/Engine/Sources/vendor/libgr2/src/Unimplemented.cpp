#include "Unimplemented.h"

#include <cstdio>
#include <mutex>
#include <set>
#include <string>

namespace NGr2
{

namespace
{

//! The entry points reported so far, and the lock that guards them.
//!
//! Function local statics so that there is no order-of-initialisation question:
//! GrannySetAllocator is called early, and a stub can therefore run before any
//! namespace scope object of this library would have been constructed.
//!
//! Keyed on the name pointer's contents rather than the pointer, because
//! __func__ produces a distinct array per function and nothing promises that
//! identical names are pooled.
std::set<std::string> &Reported()
{
	static std::set<std::string> reported;
	return reported;
}

std::mutex &ReportedMutex()
{
	static std::mutex mutex;
	return mutex;
}

}

void ReportUnimplemented( const char *pszFunction )
{
	{
		std::lock_guard<std::mutex> lock( ReportedMutex() );
		if ( !Reported().insert( pszFunction ).second )
		{
			return;
		}
	}

	// stderr rather than a log file: this library has no configuration yet, and
	// a caller that wants the output in a file can redirect it.
	std::fprintf( stderr, "libgr2: %s is not implemented\n", pszFunction );
	std::fflush( stderr );
}

}
