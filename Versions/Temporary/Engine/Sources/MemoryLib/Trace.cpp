#include "StdAfx.h"

#include "MemoryLib_export.h"

#include <stdio.h>
#include <stdlib.h>
#include "SymAccess.h"
#include "../Misc/nhash_map.h"
#include "../Misc/tools.h"

static bool bInternal = false;
struct SAlloc
{
	DWORD dwAddress;
	int nSize;
};
struct SPtrHash
{
	int operator()( void *p ) const { return (int)p; }
};
static nstl::hash_map<void*, SAlloc, SPtrHash> *pAllocs = 0;
static nstl::hash_map<DWORD, bool> *pIgnored = 0;
static nstl::hash_map<DWORD, DWORD> *pBuf2Address = 0;
static CRITICAL_SECTION block;
static bool bHasCreatedBlock;

inline bool CharCompare( const char c1, const char c2 )
{
	return ::CharLower( (LPTSTR)c1 ) == ::CharLower( (LPTSTR)c2 );
}

static int FindPattern( const char *pszString, const char *pszPattern )
{
	const int nStringSize = strlen( pszString );
	const int nPatternSize = strlen( pszPattern );
	for ( int i = 0; i <= nStringSize - nPatternSize; ++i )
	{
		int n = 0;
		while ( n < nPatternSize - 1 && CharCompare( pszString[i+n], pszPattern[n] ) )
			++n;
		if ( CharCompare( pszString[i+n], pszPattern[n] ) )
			return i;
	}
	return -1;
}

static bool AddressFits( DWORD dwAddress )
{
	nstl::hash_map<DWORD, bool>::iterator i = pIgnored->find( dwAddress );
	if ( i == pIgnored->end() )
	{
		int nSourceLine;
		CSymString szFileName;
		if ( !GetSymEngine().GetSymbol( dwAddress, 0, &szFileName, &nSourceLine, 0 ) || szFileName == "?" )
		{
			(*pIgnored)[dwAddress] = false;
			return false;
		}
		// analyze source file
		const char *pszFileName = szFileName.szStr;
		bool bOk = true;
		bOk &= ( FindPattern( pszFileName, ":\\program" ) == -1 );
		bOk &= ( FindPattern( pszFileName, "\\fileio\\" ) == -1 );
		bOk &= ( FindPattern( pszFileName, "\\memorylib\\" ) == -1 );
		bOk &= ( FindPattern( pszFileName, "\\misc\\" ) == -1 );

		return (*pIgnored)[dwAddress] = bOk;
	}
	return i->second;
}

/*
#include <vtuneapi.h>


struct SVTuneProfiler
{
	SVTuneProfiler() { VTResume(); }
	~SVTuneProfiler() { VTPause(); }
};
*/

#pragma optimize( "y", off )
void DebugRegister( size_t n, void *pRes )
{
//	SVTuneProfiler profiler;
	
	int *pBuf;
	_asm { mov pBuf, ebp }
	if ( !bHasCreatedBlock )
	{
		bHasCreatedBlock = true;
		InitializeCriticalSection( &block );
	}
	EnterCriticalSection( &block );
	if ( bInternal )
	{
		LeaveCriticalSection( &block );
		return;
	}
	bInternal = true;
	if ( !pIgnored )
	{
		LeaveCriticalSection( &block );
		pIgnored = new nstl::hash_map<DWORD, bool>;
		pAllocs = new nstl::hash_map<void*, SAlloc, SPtrHash>;
		EnterCriticalSection( &block );
	}

	DWORD dwAddress = 0;
	const DWORD dwStartAddress = pBuf[0];// for debug purposes
	pBuf = (int*)pBuf[0];
	for (;;)
	{
		if ( IsBadReadPtr( pBuf, 8 ) )
			break;

		dwAddress = pBuf[1];
		if ( AddressFits( dwAddress ) )
			break;
		pBuf = (int*)pBuf[0];
	}

	SAlloc &a = (*pAllocs)[pRes];
	a.dwAddress = dwAddress;
	a.nSize = n;
	bInternal = false;
	LeaveCriticalSection( &block );
}

void DebugFree( void *p )
{
	if ( p == 0 )
		return;
	EnterCriticalSection( &block );
	if ( bInternal )
	{
		LeaveCriticalSection( &block );
		return;
	}
	bInternal = true;
	nstl::hash_map<void*, SAlloc, SPtrHash>::iterator i = pAllocs->find( p );
	if ( i != pAllocs->end() )
		pAllocs->erase( i );
	else
		assert( 0 );
	if ( pAllocs->empty() )
	{
		LeaveCriticalSection( &block );
		delete pAllocs;
		delete pIgnored;
		delete pBuf2Address;
		pAllocs = 0;
		pIgnored = 0;
		EnterCriticalSection( &block );
	}
	bInternal = false;
	LeaveCriticalSection( &block );
}
#pragma optimize( "", on )

struct SAllocStats
{
	DWORD dwAddress;
	int nMax, nTotal, nNumber;
	SAllocStats() { nMax = 0; nTotal = 0; nNumber = 0; }
};
struct SCompareAllocStats
{
	bool operator()( const SAllocStats &a, const SAllocStats &b ) const { return a.nTotal > b.nTotal; }
};
extern int nDumbAllocBlockSizes[24];
void DumpMemoryBlockUtilization();
static bool s_bDumpExcelCompatible = true;
MEMORYLIB_EXPORT void DumpMemoryStats()
{
	if ( pAllocs == 0 )
	{
		DumpMemoryBlockUtilization();
		return;
	}
	bInternal = true;
	{
		int nPow2Allocated = 0, nNewKidOnTheBlock = 0;
		nstl::hash_map<DWORD, SAllocStats> info;
		for ( nstl::hash_map<void*, SAlloc, SPtrHash>::iterator i = pAllocs->begin(); i != pAllocs->end(); ++i )
		{
			SAllocStats &s = info[ i->second.dwAddress ];
			s.nMax = max( s.nMax, i->second.nSize );
			s.nTotal += i->second.nSize;
			s.nNumber++;
			s.dwAddress = i->second.dwAddress;

			int nSize = i->second.nSize;
			nSize = GetMSB( nSize - 1 ) + 1;
			if ( nSize < 2 )
				nSize = 2;
			nPow2Allocated += 1 << nSize;

			int nTest = i->second.nSize;
			if ( nTest > 4096 )
				nNewKidOnTheBlock += nTest + 16;
			else
			{
				for ( int k = 0; k < ARRAY_SIZE(nDumbAllocBlockSizes); ++k )
				{
					if ( nTest <= nDumbAllocBlockSizes[k] )
					{
						nNewKidOnTheBlock += nDumbAllocBlockSizes[k];
						break;
					}
				}
			}
		}
		int nTotal = 0;
		char szBuf[1024];
		nstl::vector<SAllocStats> allocStats;
		for ( nstl::hash_map<DWORD, SAllocStats>::iterator k = info.begin(); k != info.end(); ++k )
			allocStats.push_back( k->second );
		nstl::sort( allocStats.begin(), allocStats.end(), SCompareAllocStats() );
		// header
		if ( s_bDumpExcelCompatible )
			OutputDebugString( "FileName\tFileLine\tMax block\tBlocks\tTotal\tTotalbefore\n" );
		//
		for ( int k = 0; k < allocStats.size(); ++k )
		{
			const SAllocStats &a = allocStats[k];
			int nSourceLine;
			CSymString szFileName;
			GetSymEngine().GetSymbol( a.dwAddress, 0, &szFileName, &nSourceLine, 0 );
			if ( s_bDumpExcelCompatible )
				sprintf( szBuf, "%s\t%d\t%d\t%d\t%d\t%d\n", szFileName.szStr, nSourceLine, a.nMax, a.nNumber, a.nTotal, nTotal );
			else
			{
				sprintf( szBuf, "%s(%d): max block = %d, blocks = %d, total = %d, totalbefore = %d\n", 
					szFileName.szStr, nSourceLine, a.nMax, a.nNumber, a.nTotal, nTotal );
			}
			OutputDebugString( szBuf );
			nTotal += a.nTotal;
		}
		DebugTraceMMgr( "allocated %d bytes\n", nTotal );
		DebugTraceMMgr( "pow2 allocator has used %d bytes\n", nPow2Allocated );
		DebugTraceMMgr( "smart ass allocator has used %d bytes\n", nNewKidOnTheBlock );
	}
	bInternal = false;
	DumpMemoryBlockUtilization();
}

/*
static struct SLeaksWatcher
{
	~SLeaksWatcher()
	{
		DumpMemoryStats();
	}
} leaksWatcher;
*/
