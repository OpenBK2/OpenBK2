#pragma once

// What an entry point does until it is written: record that it was called, in
// what order, and with what arguments.
//
// The engine references all 54 entry points, but a reference is not a call.
// Some of them sit on paths the shipped data never takes, and the linker has no
// way to know it. A run against this library answers that, and the answer is
// what orders the milestones. Full arguments matter as much as the sequence,
// because the arguments are what say which model was being loaded, how big it
// was, and which control the engine expected back.
//
// Two levels, so that one run serves both questions. Every call is logged at
// trace. The first call to each entry point is also logged at warn, so that
// running at warn gives the list of entry points reached and nothing else.

#include <spdlog/spdlog.h>

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace NGr2
{

//! Where the trace goes. Built on first use, never destroyed.
//!
//! A function local static rather than a namespace scope object because
//! GrannySetAllocator is called during engine startup, so a stub can run before
//! anything of this library's would otherwise have been constructed.
spdlog::logger &Logger();

//! The ordinal of this call, counting every traced call across the library.
//!
//! Timestamps order the log already, but they collapse when several calls land
//! in the same millisecond, which per bone per frame calls do constantly. The
//! ordinal is what makes "which of these two happened first" answerable.
uint64_t NextCallOrdinal();

//! Say once, at warn, that pszFunction has nothing behind it.
void ReportUnimplemented( const char *pszFunction );

//! An argument in a form fmt can print.
//!
//! A string is worth more than the pointer to it, since it names the file being
//! read or the bone being looked up, so those are passed through. Every other
//! pointer becomes an opaque address, which is enough to match a later call
//! against an earlier one, and an enum becomes its number.
template <typename T>
auto Loggable( T v )
{
	if constexpr ( std::is_same_v<T, const char *> || std::is_same_v<T, char *> )
	{
		return v != nullptr ? std::string( v ) : std::string( "(null)" );
	}
	else if constexpr ( std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>> )
	{
		// A function pointer is not an object pointer and cannot be cast to
		// void*. granny_allocate_callback is one of these, and GrannySetAllocator
		// is where the engine installs its own, so it is worth seeing.
		return reinterpret_cast<uintptr_t>( v );
	}
	else if constexpr ( std::is_pointer_v<T> )
	{
		return static_cast<const void *>( v );
	}
	else if constexpr ( std::is_enum_v<T> )
	{
		return static_cast<int>( v );
	}
	else
	{
		return v;
	}
}

//! Trace one call, formatting its arguments through Loggable.
//!
//! The format string is checked against the *converted* argument types, which is
//! why it names them through decltype: writing {} for a pointer that Loggable
//! turns into a std::string still has to compile, and a genuine mismatch still
//! has to fail the build rather than the run.
//!
//! \param bStub whether the function has anything behind it, which is what
//! decides between the two levels described at the top of this file.
template <typename... TArgs>
void TraceCall( const char *pszFunction, bool bStub,
                fmt::format_string<decltype( Loggable( std::declval<TArgs>() ) )...> fmtArgs,
                TArgs... args )
{
	if ( bStub )
	{
		ReportUnimplemented( pszFunction );
	}
	if ( Logger().should_log( spdlog::level::trace ) )
	{
		Logger().trace( "{:>8}  {}( {} )", NextCallOrdinal(), pszFunction,
		                fmt::format( fmtArgs, Loggable( args )... ) );
	}
}

}

//! Trace a call to an entry point that is implemented.
//!
//! A statement rather than an expression, so that it reads as the first line of
//! the body rather than as something wrapped around it.
#define GR2_TRACE( ... ) ::NGr2::TraceCall( __func__, false, __VA_ARGS__ )

//! Trace a call to an entry point that is not written yet, and say so once.
//!
//! The difference from GR2_TRACE is the whole point of running at warn: that
//! level then lists exactly the entry points a run reached and still needs.
#define GR2_STUB( ... ) ::NGr2::TraceCall( __func__, true, __VA_ARGS__ )
