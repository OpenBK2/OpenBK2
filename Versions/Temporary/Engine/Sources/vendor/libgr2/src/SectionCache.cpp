// The section cache. SectionCache.h says why it exists and why it is sound.

#include "SectionCache.h"

#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace NGr2
{

namespace
{

//! Largest entry worth keeping, expanded.
//!
//! Sections above this are kept out rather than allowed to evict a thousand
//! small ones. The duplication that pays is in small sections: the root type
//! section averages 5.6 KB and the median section of any kind is 5.4 KB, while
//! the largest single section in this game's data is 580 KB and is unique.
constexpr size_t MAX_ENTRY_BYTES = 128 * 1024;

//! Total expanded bytes held before insertion stops.
//!
//! The whole distinct set is about 123 MB expanded plus the compressed copies
//! kept for comparison, which is more than a decoder has any business holding.
//! 48 MB covers every root type section in this data, 1,825 distinct trees at
//! 5.6 KB being about 10 MB, with room left for the data sections a single map
//! load repeats. Past the cap the cache stops growing and keeps serving what it
//! already has, which degrades to the old behaviour rather than to thrashing.
constexpr size_t MAX_TOTAL_BYTES = 48 * 1024 * 1024;

struct SEntry
{
	//! Kept so a hash hit can be confirmed by comparison rather than trusted.
	std::vector<uint8_t> compressed;
	std::vector<uint8_t> expanded;
	uint32_t nStop0 = 0;
	uint32_t nStop1 = 0;
	uint32_t nCompression = 0;
};

//! FNV-1a over the compressed bytes and the four numbers.
//!
//! Eight bytes at a time over the bulk, because this runs on every section
//! including the misses, and a byte at a time would make the lookup a measurable
//! cost of its own on a corpus where most sections are unique.
uint64_t HashKey( const SSectionKey &key )
{
	uint64_t nHash = 0xcbf29ce484222325ull;
	const uint8_t *pAt = key.pCompressed;
	size_t nLeft = key.nCompressedSize;
	while ( nLeft >= 8 )
	{
		uint64_t nWord = 0;
		memcpy( &nWord, pAt, sizeof( nWord ) );
		nHash = ( nHash ^ nWord ) * 0x100000001b3ull;
		pAt += 8;
		nLeft -= 8;
	}
	while ( nLeft-- > 0 )
	{
		nHash = ( nHash ^ *pAt++ ) * 0x100000001b3ull;
	}
	const uint32_t nNumbers[4] = { key.nCompressedSize, key.nStop0, key.nStop1,
	                               key.nExpandedSize };
	for ( uint32_t nNumber : nNumbers )
	{
		nHash = ( nHash ^ nNumber ) * 0x100000001b3ull;
	}
	return ( nHash ^ key.nCompression ) * 0x100000001b3ull;
}

struct SCache
{
	std::mutex mutex;
	//! Several entries can share a hash, so the bucket is a list and the match is
	//! decided by comparing the bytes.
	std::unordered_map<uint64_t, std::vector<SEntry>> entries;
	SSectionCacheStats stats;
};

SCache &Cache()
{
	// Function local so there is no static initialisation order question, and
	// deliberately never destroyed: this is reached from a DLL that the engine
	// unloads at exit, and a cache being torn down while a loader thread is still
	// inside it is a crash at shutdown rather than a leak anyone measures.
	static SCache *pCache = new SCache();
	return *pCache;
}

//! Does this entry describe the same expansion as the key?
bool Matches( const SEntry &entry, const SSectionKey &key )
{
	return entry.nStop0 == key.nStop0 && entry.nStop1 == key.nStop1
	       && entry.nCompression == key.nCompression
	       && entry.expanded.size() == key.nExpandedSize
	       && entry.compressed.size() == key.nCompressedSize
	       && memcmp( entry.compressed.data(), key.pCompressed, key.nCompressedSize ) == 0;
}

}

bool SectionCacheLookup( const SSectionKey &key, uint8_t *pOut )
{
	if ( key.pCompressed == nullptr || pOut == nullptr || key.nExpandedSize == 0
	     || key.nExpandedSize > MAX_ENTRY_BYTES )
	{
		return false;
	}

	SCache &cache = Cache();
	const uint64_t nHash = HashKey( key );

	std::lock_guard<std::mutex> lock( cache.mutex );
	++cache.stats.nLookups;
	const auto it = cache.entries.find( nHash );
	if ( it == cache.entries.end() )
	{
		return false;
	}
	for ( const SEntry &entry : it->second )
	{
		if ( Matches( entry, key ) )
		{
			memcpy( pOut, entry.expanded.data(), key.nExpandedSize );
			++cache.stats.nHits;
			cache.stats.nBytesServed += key.nExpandedSize;
			return true;
		}
	}
	// A hash hit that no entry matched: the bucket exists for a different input.
	++cache.stats.nCollisions;
	return false;
}

void SectionCacheInsert( const SSectionKey &key, const uint8_t *pExpanded )
{
	if ( key.pCompressed == nullptr || pExpanded == nullptr || key.nExpandedSize == 0
	     || key.nExpandedSize > MAX_ENTRY_BYTES )
	{
		return;
	}

	SCache &cache = Cache();
	const uint64_t nHash = HashKey( key );

	std::lock_guard<std::mutex> lock( cache.mutex );
	if ( cache.stats.nBytesHeld + key.nExpandedSize > MAX_TOTAL_BYTES )
	{
		return;
	}

	std::vector<SEntry> &bucket = cache.entries[nHash];
	for ( const SEntry &entry : bucket )
	{
		if ( Matches( entry, key ) )
		{
			// Already here. Two threads can expand the same section concurrently,
			// both miss, and both insert; the second one is a no-op rather than a
			// duplicate.
			return;
		}
	}

	SEntry entry;
	entry.compressed.assign( key.pCompressed, key.pCompressed + key.nCompressedSize );
	entry.expanded.assign( pExpanded, pExpanded + key.nExpandedSize );
	entry.nStop0 = key.nStop0;
	entry.nStop1 = key.nStop1;
	entry.nCompression = key.nCompression;
	cache.stats.nBytesHeld += key.nExpandedSize;
	++cache.stats.nEntries;
	bucket.push_back( std::move( entry ) );
}

SSectionCacheStats SectionCacheGetStats()
{
	SCache &cache = Cache();
	std::lock_guard<std::mutex> lock( cache.mutex );
	return cache.stats;
}

void SectionCacheClear()
{
	SCache &cache = Cache();
	std::lock_guard<std::mutex> lock( cache.mutex );
	cache.entries.clear();
	cache.stats = SSectionCacheStats();
}

}
