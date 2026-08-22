#pragma once

#include <boost/uuid/uuid.hpp>

#include <algorithm>
#include <cstdint>

struct IBinSaver;

// A uid reaches disk in the byte order Windows uses for GUID: the first three
// fields little endian, the remaining eight in written order.
// boost::uuids::uuid keeps all sixteen bytes in written order, so the two
// disagree about the first eight and agree about the last eight.
//
// Every save game, replay and cached resource name written so far holds the
// GUID order, so that is what stays on disk; the swap happens on the way in
// and out. Reading these bytes as a uuid without swapping yields a different
// uid, which then names a resource file that does not exist.
//
// The permutation is its own inverse, so one function serves both directions.
inline void SwapUuidStorageOrder( boost::uuids::uuid *pUid )
{
	std::uint8_t *p = pUid->begin();
	std::swap( p[0], p[3] );
	std::swap( p[1], p[2] );
	std::swap( p[4], p[5] );
	std::swap( p[6], p[7] );
}

// Serialize a uid under the given chunk id, in the on-disk byte order above.
// The chunk is sixteen raw bytes, exactly as it was when the field was a GUID,
// so the chunk id keeps its meaning and old streams stay readable.
template < class TSaver >
inline void AddUuidChunk( TSaver &saver, const int idChunk, boost::uuids::uuid *pUid )
{
	if ( saver.IsReading() )
	{
		saver.AddRawData( idChunk, pUid, sizeof( *pUid ) );
		SwapUuidStorageOrder( pUid );
	}
	else
	{
		boost::uuids::uuid stored = *pUid;
		SwapUuidStorageOrder( &stored );
		saver.AddRawData( idChunk, &stored, sizeof( stored ) );
	}
}
