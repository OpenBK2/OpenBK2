#include "stdafx.h"
#include "GrannyMemoryMap.hpp"

#include "vendor/granny/include/granny.h"

#include <algorithm>

struct SMemoryInfo
{
	char const *pszFile;
	granny_int32x nLine;
	granny_int32x nAlignment;
	granny_int32x nSize;

	SMemoryInfo() :
		pszFile(0),
		nLine(0),
		nAlignment(0),
		nSize(0)
	{
	}

	SMemoryInfo( char const *_pszFile, granny_int32x _nLine, granny_int32x _nAlignment, granny_int32x _nSize ) :
		pszFile(_pszFile),
		nLine(_nLine),
		nAlignment(_nAlignment),
		nSize(_nSize)
	{
	}
};

struct SGrannyMemoryRequestComparer
{
	bool operator()( const std::pair<std::string, int> &left, std::pair<std::string, int> &right ) const
	{
		return left.first < right.first;
	}
};

typedef std::unordered_map<void *, SMemoryInfo > TGrannyMemoryMap;

static granny_allocate_callback *AllocateCallback;
static granny_deallocate_callback *DeallocateCallback;
static TGrannyMemoryMap *pGrannyMemoryMap; // do not initialize!

static GRANNY_CALLBACK(void *) GrannyReplacementAlloc( char const *pszFile, granny_int32x nLine, granny_uintaddrx nAlignment, granny_uintaddrx nSize, granny_int32x nAllocationIntent )
{
	if ( pGrannyMemoryMap == 0 )
		pGrannyMemoryMap = new std::unordered_map<void *, SMemoryInfo >;

	void *pMemory = (*AllocateCallback)(pszFile,nLine,nAlignment,nSize,nAllocationIntent);
	pGrannyMemoryMap->insert( std::pair <void *, SMemoryInfo>( pMemory, SMemoryInfo(pszFile,nLine,nAlignment,nSize) ) );
	return pMemory;
}

static GRANNY_CALLBACK(void) GrannyReplacementDealloc( char const *pszFile, granny_int32x nLine, void *pMemory )
{
	pGrannyMemoryMap->erase(pMemory);
	(*DeallocateCallback)(pszFile,nLine,pMemory);
}

void InitializeGrannyMemoryMap()
{
	GrannyGetAllocator(&AllocateCallback, &DeallocateCallback);
	GrannySetAllocator( GrannyReplacementAlloc, GrannyReplacementDealloc );
}

void DumpGrannyMemory()
{
	if ( pGrannyMemoryMap == 0 )
	{
		DebugTrace( "Granny memory statistics is not initialized." );
		return;
	}

	typedef std::unordered_map<std::string, int> TMemoryRequestMap;
	TMemoryRequestMap MemoryRequestMap;
	char szBuf[1024];
	for ( TGrannyMemoryMap::iterator it = pGrannyMemoryMap->begin(); it != pGrannyMemoryMap->end(); ++it )
	{
		const SMemoryInfo &info = it->second;
		sprintf( szBuf, "%s\t%d", info.pszFile, info.nLine );
		
		TMemoryRequestMap::iterator itRequest = MemoryRequestMap.find(szBuf);
		if ( itRequest == MemoryRequestMap.end() )
			MemoryRequestMap.insert( std::pair<std::string, int>( szBuf, 0 ) );
		else
			itRequest->second += info.nSize;
    }

	std::vector< std::pair< std::string, int > > MemoryRequestArray;
	for ( TMemoryRequestMap::iterator it = MemoryRequestMap.begin(); it != MemoryRequestMap.end(); ++it )
		MemoryRequestArray.push_back( std::pair<std::string, int>( it->first.data(), it->second ) );

	MemoryRequestMap.clear();

	std::sort( MemoryRequestArray.begin(), MemoryRequestArray.end(), SGrannyMemoryRequestComparer() );

	DebugTrace( "source file\tline\tsize" );

	int nTotalSize = 0;
	for ( int i = 0; i < MemoryRequestArray.size(); ++i )
	{
		const std::pair<std::string, int> &tmp = MemoryRequestArray[i];
		DebugTrace( "%s\t%d", tmp.first.data(), tmp.second );
		nTotalSize += tmp.second;
	}

	DebugTrace( "total\t\t%d", nTotalSize );
}


