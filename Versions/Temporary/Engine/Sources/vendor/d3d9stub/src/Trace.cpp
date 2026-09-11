#include "Trace.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <map>
#include <mutex>
#include <vector>

// See Trace.h for what is recorded and why. This is the where and the how much.

namespace ND3D9Stub
{
	namespace
	{
		// How many calls of each method are written out in full before the rest
		// are only counted. A stub gets more, because it is the line being looked
		// for; either is plenty to see the arguments a method is called with.
		const unsigned long long STUB_LINES = 64;
		const unsigned long long CALL_LINES = 8;

		struct SState
		{
			std::mutex mutex;
			FILE *pLog = nullptr;
			std::string szSummaryPath;
			bool bAll = false;
			unsigned long long nSequence = 0;
			// Every call site that has run, for the summary.
			std::vector<SSite*> sites;
		};

		// The directory this DLL was loaded from, with its separator.
		std::string ModuleDirectory()
		{
			HMODULE hModule = nullptr;
			GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
													reinterpret_cast<LPCSTR>( &ModuleDirectory ), &hModule );
			char szPath[MAX_PATH] = {};
			GetModuleFileNameA( hModule, szPath, MAX_PATH );
			std::string szDirectory( szPath );
			const size_t nSlash = szDirectory.find_last_of( "\\/" );
			return nSlash == std::string::npos ? std::string() : szDirectory.substr( 0, nSlash + 1 );
		}

		std::string Environment( const char *pszName )
		{
			char szValue[MAX_PATH] = {};
			const DWORD nLength = GetEnvironmentVariableA( pszName, szValue, MAX_PATH );
			return ( nLength > 0 && nLength < MAX_PATH ) ? std::string( szValue ) : std::string();
		}

		// Built on first use, so it exists whichever static initialiser runs first
		// and whatever calls into the DLL first. Never destroyed: the last calls
		// can come from other DLLs' teardown, after this one's statics are gone.
		SState& State()
		{
			static SState *pState = []()
			{
				SState *p = new SState();
				std::string szLog = Environment( "OBK2_D3D9STUB_LOG" );
				if ( szLog.empty() )
				{
					szLog = ModuleDirectory() + "d3d9stub.log";
				}
				p->pLog = std::fopen( szLog.c_str(), "w" );
				const size_t nDot = szLog.find_last_of( '.' );
				p->szSummaryPath = ( nDot == std::string::npos ? szLog : szLog.substr( 0, nDot ) ) + ".summary";
				p->bAll = Environment( "OBK2_D3D9STUB_TRACE" ) == "all";
				if ( p->pLog != nullptr )
				{
					std::fprintf( p->pLog, "d3d9 stub, pid %lu. Every call is counted; the first %llu of each "
												"stub and %llu of each implemented method are written%s.\n",
												GetCurrentProcessId(), STUB_LINES, CALL_LINES,
												p->bAll ? " -- OBK2_D3D9STUB_TRACE=all, so all of them are" : "" );
					std::fflush( p->pLog );
				}
				return p;
			}();
			return *pState;
		}

		// Under the lock. Sites with the same name -- a method implemented in a
		// template has one per instantiation -- are added together.
		void WriteSummary( SState &rState )
		{
			FILE *pSummary = std::fopen( rState.szSummaryPath.c_str(), "w" );
			if ( pSummary == nullptr )
			{
				return;
			}
			std::map<std::pair<int, std::string>, unsigned long long> rows;
			for ( const SSite *pSite : rState.sites )
			{
				const std::string szName = std::string( pSite->pszInterface ) + "::" + pSite->pszMethod;
				rows[{ pSite->eKind == EKind::Stub ? 0 : 1, szName }] += pSite->nCalls.load();
			}
			std::fprintf( pSummary, "%-6s %12s  %s\n", "kind", "calls", "method" );
			for ( const auto &row : rows )
			{
				std::fprintf( pSummary, "%-6s %12llu  %s\n", row.first.first == 0 ? "stub" : "call", row.second,
											row.first.second.c_str() );
			}
			std::fclose( pSummary );
		}
	}


	bool Count( SSite &rSite )
	{
		const unsigned long long nCall = ++rSite.nCalls;
		if ( !rSite.bRegistered.load( std::memory_order_acquire ) )
		{
			SState &rState = State();
			std::lock_guard<std::mutex> lock( rState.mutex );
			if ( !rSite.bRegistered.load( std::memory_order_relaxed ) )
			{
				rState.sites.push_back( &rSite );
				rSite.bRegistered.store( true, std::memory_order_release );
				WriteSummary( rState );
			}
		}
		static const bool bAll = State().bAll;
		return bAll || nCall <= ( rSite.eKind == EKind::Stub ? STUB_LINES : CALL_LINES );
	}


	void Write( const std::string &rLine )
	{
		SState &rState = State();
		std::lock_guard<std::mutex> lock( rState.mutex );
		if ( rState.pLog == nullptr )
		{
			return;
		}
		std::fprintf( rState.pLog, "%8llu %s\n", ++rState.nSequence, rLine.c_str() );
		std::fflush( rState.pLog );
	}


	void Note( const char *pszFormat, ... )
	{
		char szText[1024];
		va_list args;
		va_start( args, pszFormat );
		std::vsnprintf( szText, sizeof( szText ), pszFormat, args );
		va_end( args );
		Write( std::string( "note " ) + szText );
	}


	void PutName( std::string *pOut, const char **ppszNames )
	{
		const char *psz = *ppszNames;
		while ( *psz == ' ' || *psz == ',' )
		{
			++psz;
		}
		const char *pszEnd = psz;
		while ( *pszEnd != '\0' && *pszEnd != ',' )
		{
			++pszEnd;
		}
		pOut->append( psz, pszEnd );
		pOut->append( "=" );
		*ppszNames = pszEnd;
	}


	void PutGuid( std::string *pOut, const GUID &rGuid )
	{
		char sz[64];
		std::snprintf( sz, sizeof( sz ), "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", rGuid.Data1,
									 rGuid.Data2, rGuid.Data3, rGuid.Data4[0], rGuid.Data4[1], rGuid.Data4[2], rGuid.Data4[3],
									 rGuid.Data4[4], rGuid.Data4[5], rGuid.Data4[6], rGuid.Data4[7] );
		pOut->append( sz );
	}


	void PutPointer( std::string *pOut, const void *p )
	{
		if ( p == nullptr )
		{
			pOut->append( "null" );
			return;
		}
		char sz[32];
		std::snprintf( sz, sizeof( sz ), "%p", p );
		pOut->append( sz );
	}


	// Small numbers in decimal, and anything that looks like flags, a colour or
	// a FOURCC in hex as well, which is what it will be compared against.
	void PutUnsigned( std::string *pOut, unsigned long long nValue )
	{
		char sz[48];
		if ( nValue > 0xFFFF )
		{
			std::snprintf( sz, sizeof( sz ), "%llu(0x%llX)", nValue, nValue );
		}
		else
		{
			std::snprintf( sz, sizeof( sz ), "%llu", nValue );
		}
		pOut->append( sz );
	}


	void PutSigned( std::string *pOut, long long nValue )
	{
		if ( nValue >= 0 )
		{
			PutUnsigned( pOut, static_cast<unsigned long long>( nValue ) );
			return;
		}
		char sz[32];
		std::snprintf( sz, sizeof( sz ), "%lld", nValue );
		pOut->append( sz );
	}


	void PutFloat( std::string *pOut, double fValue )
	{
		char sz[32];
		std::snprintf( sz, sizeof( sz ), "%g", fValue );
		pOut->append( sz );
	}


	void Shutdown()
	{
		SState &rState = State();
		std::lock_guard<std::mutex> lock( rState.mutex );
		WriteSummary( rState );
		if ( rState.pLog != nullptr )
		{
			std::fprintf( rState.pLog, "%8llu note process detaching; final counts in the summary\n", ++rState.nSequence );
			std::fflush( rState.pLog );
		}
	}
}
