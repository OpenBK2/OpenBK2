#pragma once

// What an entry point does until it is written: record that it was called, in
// what order, and with what arguments.
//
// The engine references 54 of the 61 entry points and the map editor the other
// seven, but a reference is not a call.
// Some of them sit on paths the shipped data never takes, and the linker has no
// way to know it. A run against this library answers that, and the answer is
// what orders the milestones. Full arguments matter as much as the sequence,
// because the arguments are what say which model was being loaded, how big it
// was, and which control the engine expected back.
//
// Two levels, so that one run serves both questions. Every call is logged at
// trace. The first call to each entry point is also logged at warn, so that
// running at warn gives the list of entry points reached and nothing else.
//
// The per-call trace is compiled out unless LIBGR2_TRACE_CALLS is defined, which
// the build does for Debug and for -DLIBGR2_TRACE=ON. It was unconditional while
// the library was a set of stubs and the log was the whole deliverable; now that
// the entry points do work, leaving it in makes the game a slide show and
// animation impossible to judge. The warn-once path stays in every build.

#include <gr2/granny.h>

#include <spdlog/spdlog.h>

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace NGr2
{

//! What kind of thing a handle is, or null for a pointer that is not one.
//!
//! Specialised below for the objects worth counting. Anything else, a caller's
//! buffer or a pointer into a pose, keeps its bare address: those are unbounded
//! in number and naming them would fill the id table with things nobody refers
//! to twice.
template <typename T>
struct SHandleKind
{
	static constexpr const char *NAME = nullptr;
};

#define GR2_HANDLE_KIND( type, name )              \
	template <>                                    \
	struct SHandleKind<type>                       \
	{                                              \
		static constexpr const char *NAME = name;  \
	};

GR2_HANDLE_KIND( granny_file, "file" )
GR2_HANDLE_KIND( granny_file_info, "info" )
GR2_HANDLE_KIND( granny_model_instance, "instance" )
GR2_HANDLE_KIND( granny_local_pose, "pose" )
GR2_HANDLE_KIND( granny_world_pose, "worldpose" )
GR2_HANDLE_KIND( granny_control, "control" )
GR2_HANDLE_KIND( granny_controlled_animation_builder, "builder" )
GR2_HANDLE_KIND( granny_track_mask, "mask" )
GR2_HANDLE_KIND( granny_skeleton, "skeleton" )
GR2_HANDLE_KIND( granny_model, "model" )
GR2_HANDLE_KIND( granny_animation, "animation" )
GR2_HANDLE_KIND( granny_track_group, "trackgroup" )
GR2_HANDLE_KIND( granny_mesh, "mesh" )

#undef GR2_HANDLE_KIND

//! "pose#7@0x1035b8": a number that survives the run, and the address that does not.
//!
//! Addresses are what a log has always carried, and they are useless for the one
//! thing this log is now for. They differ between two runs of the same build, and
//! they differ far more between this library and the real DLL, so two traces of
//! the same play cannot be diffed. The number is assigned on a handle's first
//! appearance and counted per kind, so given the same sequence of calls both
//! implementations produce the same ids. The address stays for correlating a
//! line with a debugger or a crash dump.
std::string HandleName( const char *pszKind, const void *pHandle );

//! Give up a handle's id, because the object behind it has been freed.
//!
//! Without this the next object to land on that address inherits the dead one's
//! name, which reads as one long-lived object rather than two. Called from the
//! entry points that free something, and only from those: whoever frees is the
//! one that knows.
void RetireHandle( const void *pHandle );

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
//! read or the bone being looked up, so those are passed through. A pointer to
//! one of the objects this API hands out becomes a stable name and its address,
//! see HandleName; every other pointer becomes the address alone, and an enum
//! becomes its number.
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
		using TObject = std::remove_cv_t<std::remove_pointer_t<T>>;
		if constexpr ( SHandleKind<TObject>::NAME != nullptr )
		{
			return HandleName( SHandleKind<TObject>::NAME, static_cast<const void *>( v ) );
		}
		else
		{
			return fmt::format( "{}", static_cast<const void *>( v ) );
		}
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

//! Swallow the arguments of a trace that is compiled out.
//!
//! Not an empty macro. Several entry points touch a parameter only in their
//! trace line, and an empty macro would leave those unused and /W4 would say so
//! at every one of them. Passing them here is a use, and an empty inline
//! function with no side effects compiles to nothing.
template <typename... TArgs>
void TraceIgnored( TArgs &&... )
{
}

}

//! Trace a call to an entry point that is implemented.
//!
//! A statement rather than an expression, so that it reads as the first line of
//! the body rather than as something wrapped around it.
//!
//! Compiled out unless LIBGR2_TRACE_CALLS is defined, which the build does for
//! Debug and for -DLIBGR2_TRACE=ON. It is not a cheap thing to leave in: it
//! formats and writes a line per call, and flushes each one, and the busiest
//! entry point here is called 17,880 times in a few seconds of play. With it on,
//! the game is a slide show and animation cannot be judged at all.
#if defined( LIBGR2_TRACE_CALLS )
	#define GR2_TRACE( ... ) ::NGr2::TraceCall( __func__, false, __VA_ARGS__ )
#else
	#define GR2_TRACE( ... ) ::NGr2::TraceIgnored( __VA_ARGS__ )
#endif

//! Trace a call to an entry point that is not written yet, and say so once.
//!
//! The difference from GR2_TRACE is the whole point of running at warn: that
//! level then lists exactly the entry points a run reached and still needs.
//!
//! This one is never compiled out. It fires once per entry point rather than
//! once per call, so it costs nothing, and a release build reaching something
//! that is not written is exactly what somebody needs to be told about.
#define GR2_STUB( ... ) ::NGr2::TraceCall( __func__, true, __VA_ARGS__ )
