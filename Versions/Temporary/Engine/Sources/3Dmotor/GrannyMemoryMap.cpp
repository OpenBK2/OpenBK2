#include "StdAfx.h"
#include "GrannyMemoryMap.hpp"

#include "../vendor/Granny/include/granny.h"
#pragma comment(lib, "granny2.lib")

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

struct SSimplePointerHash
{
	template <class T> 
		int operator()( T *p ) const { return (int)p; }
};

struct SGrannyMemoryRequestComparer
{
	bool operator()( const pair<string, int> &left, pair<string, int> &right ) const
	{
		return left.first < right.first;
	}
};

typedef hash_map<void *, SMemoryInfo, SSimplePointerHash > TGrannyMemoryMap;

static granny_allocate_callback *AllocateCallback;
static granny_deallocate_callback *DeallocateCallback;
static TGrannyMemoryMap *pGrannyMemoryMap; // do not initialize!

static GRANNY_CALLBACK(void *) GrannyReplacementAlloc( char const *pszFile, granny_int32x nLine, granny_int32x nAlignment, granny_int32x nSize )
{
	if ( pGrannyMemoryMap == 0 )
		pGrannyMemoryMap = new hash_map<void *, SMemoryInfo, SSimplePointerHash >;

	void *pMemory = (*AllocateCallback)(pszFile,nLine,nAlignment,nSize);
	pGrannyMemoryMap->insert( pair <void *, SMemoryInfo>( pMemory, SMemoryInfo(pszFile,nLine,nAlignment,nSize) ) );
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
		OutputDebugString( "Granny memory statistics is not initialized.\n" );
		return;
	}

	typedef hash_map<string, int> TMemoryRequestMap;
	TMemoryRequestMap MemoryRequestMap;
	char szBuf[1024];
	for ( TGrannyMemoryMap::iterator it = pGrannyMemoryMap->begin(); it != pGrannyMemoryMap->end(); ++it )
	{
		const SMemoryInfo &info = it->second;
		sprintf( szBuf, "%s\t%d", info.pszFile, info.nLine );
		
		TMemoryRequestMap::iterator itRequest = MemoryRequestMap.find(szBuf);
		if ( itRequest == MemoryRequestMap.end() )
			MemoryRequestMap.insert( pair<string, int>( szBuf, 0 ) );
		else
			itRequest->second += info.nSize;
    }

	vector< pair< string, int > > MemoryRequestArray;
	for ( TMemoryRequestMap::iterator it = MemoryRequestMap.begin(); it != MemoryRequestMap.end(); ++it )
		MemoryRequestArray.push_back( pair<string, int>( it->first.data(), it->second ) );

	MemoryRequestMap.clear();

	sort( MemoryRequestArray.begin(), MemoryRequestArray.end(), SGrannyMemoryRequestComparer() );

	OutputDebugString( "source file\tline\tsize\n" );

	int nTotalSize = 0;
	for ( int i = 0; i < MemoryRequestArray.size(); ++i )
	{
		const pair<string, int> &tmp = MemoryRequestArray[i];
		sprintf( szBuf, "%s\t%d\n", tmp.first.data(), tmp.second );
		OutputDebugString( szBuf );
		nTotalSize += tmp.second;
	}

	sprintf( szBuf, "total\t\t%d\n", nTotalSize );
	OutputDebugString( szBuf );
}

