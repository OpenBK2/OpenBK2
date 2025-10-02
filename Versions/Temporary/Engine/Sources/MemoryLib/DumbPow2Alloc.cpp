#include "StdAfx.h"

#include <stdio.h>

#define ARRAY_SIZE( a ) ( sizeof( a ) / sizeof( (a)[0] ) )

//#define TRACK_MEMORY_ALLOC
const int N_SIZE = 0x18000000;
const int N_WAYS = 24;
const int N_CHUNKS = 96;
static void *pBase = 0;
static int nLazyFree = 0;
static void* freePtrs[N_WAYS];
static int nPrevAllocSize = -1;
static void **pPrevAllocFree = 0;
static unsigned char ways[N_CHUNKS];
int nDumbAllocBlockSizes[24] =
{
	8, 16, 24, 32, 48, 64, 96, 128,  
	192, 256, 384, 512, 768, 1024, 1536, 2048, 
	3072, 4096, 6144, 8192, 12288, 16384, 24576, 32768
};
static void* pAllocated[N_CHUNKS];
static int nLock = 0;
static void *pLazyBlocksList = 0;
//
static bool bFastAllocInited = false;

static __forceinline void RealEnterCritical()
{
	_asm
	{
Retry:
		mov eax, 1
		lock xchg nLock, eax
		test eax, eax
		jz Ok
		//pause
		push 0
		call dword ptr[ Sleep ]
		jmp Retry
Ok:
	}
}

static __forceinline void RealLeaveCritical()
{
	_asm 
	{
		mov nLock, 0
		//lock sub nLock, 1
	}
}

//#define _NIVAL_NET_SERVER
// STARFORCE{
#if defined(_FINALRELEASE) && !defined(_NIVAL_NET_SERVER)
void __declspec(dllexport) SFINIT1_ReserveMemoryRange()
{
	pBase = VirtualAlloc( 0, N_SIZE, MEM_RESERVE, PAGE_READWRITE );
}
#endif // defined(_FINALRELEASE) && !defined(_NIVAL_NET_SERVER)
// STARFORCE}
static void InitDumbAlloc()
{
	// STARFORCE{
#if !defined(_FINALRELEASE) || defined(_NIVAL_NET_SERVER)
	pBase = VirtualAlloc( 0, N_SIZE, MEM_RESERVE, PAGE_READWRITE );
#endif // !defined(_FINALRELEASE) || defined(_NIVAL_NET_SERVER)
#ifdef _NIVAL_NET_SERVER
#pragma message ("***** ==> WARNING!!! _NIVAL_NET_SERVER defined! StarForce protection for memory lib disabled! <== *****")
#endif 
	// STARFORCE}

	if ( pBase == 0 )
	{
		OutputDebugString( "Fast memory manager failed to find contiguous memory block\n" );
		for ( int k = 0; k < N_CHUNKS; ++k )
			pAllocated[k] = pBase;
		pBase = (void*)0xc0000000;
	}
	else
	{
		for ( int k = 0; k < N_CHUNKS; ++k )
			pAllocated[k] = ((char*)pBase) + k * (N_SIZE/N_CHUNKS);
		memset( ways, -1, sizeof(ways) );
	}
	bFastAllocInited = true;
}

static __forceinline void* RealFastDumbAlloc( int _nSize )
{
	nPrevAllocSize = _nSize;
	int nWay = 0;
	if ( _nSize > 8 )
	{
		int n = (_nSize - 1)|7;
		if ( n & 0x7e00 ) { n >>= 8; nWay += 16; }
		if ( n & 0x1e0 ) { n >>= 4; nWay += 8; }
		if ( n & 0x18 ) { n >>= 2; nWay += 4; }
		if ( n & 0x4 ) { n >>= 1; nWay += 2; }
		nWay += n - 6;
	}
	void **pWay = &freePtrs[ nWay ];
	void *pFree = *pWay;
	void *pRes = pFree;
	if ( !pRes )
	{
		bool bFound = false;
		for ( int k = 0; k < N_CHUNKS; ++k )
		{
			char **pNew = (char**)&pAllocated[ k ];
			if ( ( ways[k] == nWay || ways[k] == (unsigned char)0xff ) && *pNew )
			{
				ways[k] = nWay;
				char *pNewPlace = *pNew;
				void *pTest = VirtualAlloc( pNewPlace, 0x10000, MEM_COMMIT, PAGE_READWRITE );
				ASSERT( pTest == pNewPlace );
				int nSize = nDumbAllocBlockSizes[ nWay ], nMax = 0x10000 - nSize;
				for ( int nDelta = 0; nDelta <= nMax; nDelta += nSize )
				{
					void *pBlock = pNewPlace + nDelta;
					*(void**)pBlock = pFree;
					pFree = pBlock;
				}
				*pNew = pNewPlace + 0x10000;
				// check if any space has left in the chunk
				if ( ( ((int)*pNew) & ( N_SIZE/N_CHUNKS - 1 ) ) == 0 )
					*pNew = 0;
				pRes = pFree;
				bFound = true;
				break;
			}
		}
		if ( !bFound )
		{
			// fast memory allocator failed
			nPrevAllocSize = -1;
			return malloc( _nSize );
		}
	}
	pFree = *(void**)pRes;
	*pWay = pFree;
	pPrevAllocFree = pWay;
	return pRes;
}

static __declspec( thread ) int nFastThread = 0;
static __forceinline void* FastDumbAlloc( int _nSize )
{
	// short cut for the most probable path
	if ( pBase && nFastThread )
		return RealFastDumbAlloc( _nSize );
	if ( /*!pBase*/ !bFastAllocInited )
	{
		InitDumbAlloc();
		nFastThread = 1;
	}
	if ( nFastThread )
		return RealFastDumbAlloc( _nSize );
	else
		return malloc( _nSize );
}
void DisableFastMemAlloc()
{
	nFastThread = 0;
}

static __forceinline void RealFastDumbFree( void *pData )
{
	int nChunk = ((int)( ((char*)pData) - ((char*)pBase) )) / (N_SIZE/N_CHUNKS);
	void **pFree = &freePtrs[ ways[ nChunk ] ];
	*((void**)pData) = *pFree;
	*pFree = pData;
}

static __forceinline bool FastDumbFree( void *pData )
{
	if ( pData < pBase || pData > ( (char*)pBase + N_SIZE ) )
		return pData == 0;
	if ( nFastThread )
	{
		++nLazyFree;
		RealFastDumbFree( pData );
		if ( ( nLazyFree & 0xfff ) == 0 )
		{
			// perform lazy free
			RealEnterCritical();
			for ( void *pBlock = pLazyBlocksList; pBlock; )
			{
				void *pNextBlock = *((void**)pBlock);
				RealFastDumbFree( pBlock );
				pBlock = pNextBlock;
			}
			pLazyBlocksList = 0;
			RealLeaveCritical();
		}
	}
	else
	{
		// can not do it fast - add to lazy free list and be done
		RealEnterCritical();
		*(void**)pData = pLazyBlocksList;
		pLazyBlocksList = pData;
		RealLeaveCritical();
	}
	return true;
}

void DebugRegister( size_t n, void *pRes );
void* __cdecl operator new( size_t n )
{
	void *pRes;
#ifdef _DEBUG
	pRes = malloc( n );
#else
	// mighty shortcut if same size is requested
	if ( nPrevAllocSize == n && nFastThread && *pPrevAllocFree )
	{
		void *pRes = *pPrevAllocFree;
		*pPrevAllocFree = *(void**)pRes;
		return pRes;
	}
	if ( unsigned(n - 1) <= 32768U - 1U )//512 )//32768 )
		pRes = FastDumbAlloc( n );
	else
	{
		if ( n == 0 )
			pRes = FastDumbAlloc( 1 );
		else
			pRes = malloc( n );
	}
#endif
#ifdef TRACK_MEMORY_ALLOC
	DebugRegister( n, pRes );
#endif
	return pRes;
}

int nBlameLinker = 0;
#ifdef TRACK_MEMORY_ALLOC
void DumpMemoryStats();
class CLeakDetector
{
public:
	int nPad;
	~CLeakDetector()
	{
		DumpMemoryStats();
	}
};
#endif

void DebugFree( void *p );
void __cdecl operator delete( void *p )
{
#ifdef TRACK_MEMORY_ALLOC
	DebugFree( p );
	static CLeakDetector leakDetector;
	leakDetector.nPad++;
	nBlameLinker = leakDetector.nPad;
#endif
#ifdef _DEBUG
	free( p );
#else
	if ( !FastDumbFree(p) )
		free( p );
#endif
}

void *__cdecl operator new[](size_t count) //_THROW1(std::bad_alloc)
{
	return operator new(count);
}

void __cdecl operator delete[]( void * p )
{
	operator delete(p);
}

const int N_PAGE_SIZE = 4096;
void DumpMemoryBlockUtilization()
{
	if ( pBase == 0 || pBase == (void*)0xc0000000 )
		return;
	DebugTraceMMgr( "memory blocks utilisation stats:\n" );
	int nTotalAllocated = 0, nTotalFree = 0, nTotalBadPages = 0, nTotalPages = 0;
	char *entries;
	entries = (char*)malloc( (0x10000 / 4) * ( N_SIZE / N_CHUNKS / 0x10000 ) );
	for ( int k = 0; k < N_CHUNKS; ++k )
	{
		if ( ways[k] == (unsigned char)0xff )
			continue;
		int nWay = ways[k];
		int nSize = nDumbAllocBlockSizes[ nWay ];
		char *pStart = ((char*)pBase) + k * (N_SIZE / N_CHUNKS), *pCurrent = (char*)pAllocated[k];
		if ( pCurrent == 0 )
			pCurrent = pStart + N_SIZE / N_CHUNKS;
		int nEntries = 0x10000 / nSize;
		int nEntriesTotal = nEntries * ( N_SIZE / N_CHUNKS / 0x10000 );
		memset( entries, 0, nEntriesTotal );
		int nCount = 0;
		void **pFree = &freePtrs[ nWay ];
		if ( *pFree )
		{
			for ( void **p = pFree; *p; p = (void**)*p )
			{
				char *pBlock = (char*)*p;
				if ( pBlock >= pStart && pBlock < pCurrent )
				{
					++nCount;
					int nShift = pBlock - pStart;
					entries[ ( nShift / 0x10000 ) * nEntries + (nShift & 0xffff) / nSize ] = 1;
				}
			}
		}
		int nAllocatedEntries = ( ( pCurrent - pStart ) / 0x10000 ) * nEntries;
		memset( entries + nAllocatedEntries, 1, nEntriesTotal - nAllocatedEntries );
		char pages[ N_SIZE / N_CHUNKS / N_PAGE_SIZE ];
		memset( pages, 0, sizeof(pages) );
		for ( int z = 0; z < nEntriesTotal; z += nEntries )
		{
			int nShift = ( z / nEntries ) * 0x10000;
			for ( int i = 0; i < nEntries; ++i, nShift += nSize )
			{
				int nBit = 0;
				if ( entries[ z + i ] )
					nBit = 1; // free entry
				else
					nBit = 2; // used entry
				for ( int nDelta = nSize - 1; nDelta >= 0; nDelta -= N_PAGE_SIZE )
					pages[ ( nShift + nDelta ) / N_PAGE_SIZE ] |= nBit;
			}
		}
		int nBadPages = 0;
		for ( int z = 0; z < ARRAY_SIZE(pages); ++z )
		{
			nBadPages += pages[z] == 3;
			nTotalPages += pages[z] != 1;
		}
		int nAllocated = pCurrent - pStart;
		DebugTraceMMgr( "chunk = %d; size = %d; allocated = %d, free = %d; utilisation = %g, fragmentation = %g\n", 
			k, nSize, nAllocated, nCount * nSize,  
			( nAllocated - nCount * nSize ) * 100.0f / (pCurrent - pStart), 100.0f * nBadPages / ARRAY_SIZE(pages) );
		nTotalAllocated += nAllocated;
		nTotalFree += nCount * nSize;
		nTotalBadPages += nBadPages;
	}
	free( entries );
	DebugTraceMMgr( "Total allocated = %d, free = %d, utilisation = %g, fragmentation = %g\n", nTotalAllocated, nTotalFree, 
		100.0f * (nTotalAllocated - nTotalFree ) / nTotalAllocated, 100.0f * nTotalBadPages / ( N_SIZE / N_PAGE_SIZE ) );
	DebugTraceMMgr( "Total bytes in used pages = %d\n", nTotalPages * N_PAGE_SIZE ); 
}


void __cdecl DebugTraceMMgr( const char *pszFormat, ... )
{
	static char buff[20000];
	va_list va;
	// 
	va_start( va, pszFormat );
	vsprintf( buff, pszFormat, va );
	va_end( va );
	//
	OutputDebugString( buff );
	ASSERT( strlen( buff ) < 20000 );
}
