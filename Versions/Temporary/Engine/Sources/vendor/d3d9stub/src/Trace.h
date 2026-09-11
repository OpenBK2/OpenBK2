#pragma once

// What the stub d3d9.dll records, and where.
//
// Every method of every interface goes through one of these:
//
//   D3D9_STUB( "IDirect3DDevice9", State, Value )   in the generated bodies:
//       nothing implements this yet, and the call failed;
//   D3D9_TRACE( "IDirect3DDevice9", State, Value )  in the hand-written ones:
//       the call was answered.
//
// (and D3D9_STUB0 / D3D9_TRACE0 for a method with no arguments.)
//
// The trace exists to say which methods the engine really reaches, in what order
// and with what arguments -- the list the implementation has to satisfy. So
// every call is counted, but only the first few of each method are written out
// in full, or a frame loop would bury the rest: OBK2_D3D9STUB_TRACE=all writes
// every call. A method that is still a stub is written out more often than one
// that is implemented, because that is the line someone is looking for.
//
// The log is d3d9stub.log next to the DLL (OBK2_D3D9STUB_LOG names another
// file), truncated each run and flushed each line, so a crash leaves the call
// before it on disk. Beside it, d3d9stub.summary lists every method seen with
// its count, rewritten whenever a new one first appears and again at exit.
//
// Counting is per call site: each macro expansion owns a static SSite, so a
// call that is not written out costs one atomic increment and takes no lock.

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <type_traits>

namespace ND3D9Stub
{
	enum class EKind
	{
		Stub,		// not implemented; the call failed
		Call,		// implemented
	};

	// One macro expansion: what it is and how often it has run.
	struct SSite
	{
		EKind eKind;
		const char *pszInterface;
		const char *pszMethod;
		std::atomic<unsigned long long> nCalls{ 0 };
		std::atomic<bool> bRegistered{ false };

		SSite( EKind _eKind, const char *_pszInterface, const char *_pszMethod )
			: eKind( _eKind ), pszInterface( _pszInterface ), pszMethod( _pszMethod ) {}
	};

	// Counts the call and answers whether it should be written out.
	bool Count( SSite &rSite );
	// Writes one line to the log, prefixed with its sequence number.
	void Write( const std::string &rLine );
	// Free text, for what a call did as well as that it happened.
	void Note( const char *pszFormat, ... );
	// Rewrites the summary with the final counts; DllMain calls it at exit.
	void Shutdown();

	void PutGuid( std::string *pOut, const GUID &rGuid );
	void PutPointer( std::string *pOut, const void *p );
	void PutUnsigned( std::string *pOut, unsigned long long nValue );
	void PutSigned( std::string *pOut, long long nValue );
	void PutFloat( std::string *pOut, double fValue );
	// pszNames is the macro's stringised argument list, "Level, pRect, Flags".
	void PutName( std::string *pOut, const char **ppszNames );

	// One argument, by what kind of thing it is. Pointers are printed as
	// addresses and never followed: half of what arrives here is an out
	// parameter not yet written, and the trace must not be the thing that
	// faults.
	template<class T>
	void Put( std::string *pOut, const T &rValue )
	{
		if constexpr ( std::is_same_v<T, GUID> )
		{
			PutGuid( pOut, rValue );
		}
		else if constexpr ( std::is_pointer_v<T> || std::is_null_pointer_v<T> )
		{
			PutPointer( pOut, rValue );
		}
		else if constexpr ( std::is_enum_v<T> )
		{
			PutSigned( pOut, static_cast<long long>( rValue ) );
		}
		else if constexpr ( std::is_floating_point_v<T> )
		{
			PutFloat( pOut, rValue );
		}
		else if constexpr ( std::is_integral_v<T> && std::is_signed_v<T> )
		{
			PutSigned( pOut, rValue );
		}
		else if constexpr ( std::is_integral_v<T> )
		{
			PutUnsigned( pOut, rValue );
		}
		else
		{
			pOut->append( "<" );
			PutUnsigned( pOut, sizeof( T ) );
			pOut->append( " bytes>" );
		}
	}

	template<class... TArgs>
	void Record( const SSite &rSite, const char *pszNames, const TArgs &...args )
	{
		std::string line = rSite.eKind == EKind::Stub ? "stub " : "call ";
		line += rSite.pszInterface;
		line += "::";
		line += rSite.pszMethod;
		line += "(";
		bool bFirst = true;
		( ( line += bFirst ? " " : ", ", bFirst = false, PutName( &line, &pszNames ), Put( &line, args ) ), ... );
		line += sizeof...( TArgs ) > 0 ? " )" : ")";
		Write( line );
	}
}

#define D3D9_RECORD_( kind, iface, names, ... )                                        \
	do                                                                                \
	{                                                                                 \
		static ::ND3D9Stub::SSite site_( kind, iface, __func__ );                       \
		if ( ::ND3D9Stub::Count( site_ ) )                                              \
		{                                                                               \
			::ND3D9Stub::Record( site_, names, __VA_ARGS__ );                             \
		}                                                                               \
	} while ( 0 )

#define D3D9_STUB( iface, ... ) D3D9_RECORD_( ::ND3D9Stub::EKind::Stub, iface, #__VA_ARGS__, __VA_ARGS__ )
#define D3D9_TRACE( iface, ... ) D3D9_RECORD_( ::ND3D9Stub::EKind::Call, iface, #__VA_ARGS__, __VA_ARGS__ )

#define D3D9_STUB0( iface )                                                             \
	do                                                                                \
	{                                                                                 \
		static ::ND3D9Stub::SSite site_( ::ND3D9Stub::EKind::Stub, iface, __func__ );   \
		if ( ::ND3D9Stub::Count( site_ ) )                                              \
		{                                                                               \
			::ND3D9Stub::Record( site_, "" );                                             \
		}                                                                               \
	} while ( 0 )

#define D3D9_TRACE0( iface )                                                            \
	do                                                                                \
	{                                                                                 \
		static ::ND3D9Stub::SSite site_( ::ND3D9Stub::EKind::Call, iface, __func__ );   \
		if ( ::ND3D9Stub::Count( site_ ) )                                              \
		{                                                                               \
			::ND3D9Stub::Record( site_, "" );                                             \
		}                                                                               \
	} while ( 0 )
