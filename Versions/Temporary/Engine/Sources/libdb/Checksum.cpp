#include "stdafx.h"

#include "Checksum.h"

#include <cstdint>
#include <vector>

#include <zlib.h>

namespace NDb
{
namespace
{
struct SCheckSumTraversal
{
	unsigned int nDepth = 0;
	std::vector<uint32_t *> caches;
};

// One traversal across all DLLs, with no dependence on earlier checksum calls.
thread_local SCheckSumTraversal traversal;
}

CCheckSumScope::CCheckSumScope( uint32_t &cache )
{
	// A nonzero cache is either a completed shared child or an active cycle.
	// Register before incrementing depth so allocation failure cannot strand it.
	if ( cache == 0 )
		traversal.caches.push_back( &cache );
	++traversal.nDepth;
}

CCheckSumScope::~CCheckSumScope()
{
	if ( --traversal.nDepth == 0 )
	{
		// Also runs on exception: future calls must recompute changed descendants.
		for ( uint32_t *pCache : traversal.caches )
			*pCache = 0;
		traversal.caches.clear();
	}
}

uint32_t GetDefaultCheckSum()
{
	return adler32(0L, Z_NULL, 0);
}

uint32_t CalcCheckSum( const uint32_t dwLastCheckSum, const uint8_t *pBuf, const int nLen )
{
	return adler32( dwLastCheckSum, pBuf, nLen );
}
}


