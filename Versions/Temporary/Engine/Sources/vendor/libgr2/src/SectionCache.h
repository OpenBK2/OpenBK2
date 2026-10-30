#pragma once

// Expanded section bytes, remembered so an identical section is not expanded
// twice.
//
// WHY THIS EXISTS. The same bytes arrive over and over. Measured across the
// 15,754 GR2 files in Versions/Current/Data/bin, 41,886 sections carry data and
// only 16,569 of them are distinct, so 174.5 MB of the 310.3 MB expanded is a
// repeat of something already expanded. The root type section is the worst of it,
// 15,754 sections holding 1,825 distinct trees, because the exporter wrote the
// same schema into every file it produced; one single tree appears in 4,330
// files. The data sections repeat 1.8 times over on top of that.
//
// Decoding is between 23 and 37 MB/s depending on the codec, and copying is
// several GB/s, so serving a repeat from here instead of decoding it is on the
// order of a hundred times cheaper.
//
// WHY IT IS SOUND. Expanded section bytes are a pure function of the compressed
// bytes and the three numbers that drive the codec. File.cpp writes a section's
// data once, at expansion time, and every later access is const; ResolvePointer
// resolves from the fixup table and never rewrites the section. So two sections
// agreeing on all four inputs expand to identical, read-only output, and handing
// out a copy of one is indistinguishable from expanding the other.
//
// The key is all four inputs, not just the bytes, because two sections could
// share compressed bytes and differ in a stop offset. And a hit is confirmed by
// comparing the compressed bytes rather than trusting the hash, which removes
// the collision argument rather than bounding it.
//
// WHAT IT HANDS BACK IS A COPY, not a reference into the cache. That keeps
// granny_file owning its own section data exactly as before, so no lifetime or
// aliasing question arises anywhere above this file, and it keeps every existing
// accessor unchanged. The copy is the price; at several GB/s against a decoder at
// tens of MB/s it is not a price worth optimising away until something says it
// is.
//
// THREADING. Bytes may cross threads in this engine and refcounted objects may
// not, which puts a byte cache on the right side of that line. The whole thing
// is guarded by one mutex. That is a real serialisation point if several threads
// ever expand sections at once, but the critical section is a hash lookup and a
// memcpy rather than a decode, so it is orders of magnitude shorter than the work
// it replaces.

#include <cstdint>
#include <cstddef>

namespace NGr2
{

//! The four inputs that decide what a section expands to.
struct SSectionKey
{
	const uint8_t *pCompressed = nullptr;
	uint32_t nCompressedSize = 0;
	uint32_t nStop0 = 0;
	uint32_t nStop1 = 0;
	uint32_t nExpandedSize = 0;
	//! Which codec, so Oodle0 and Oodle1 cannot collide on identical bytes.
	uint32_t nCompression = 0;
};

//! Copy a previously expanded section into pOut, if this exact input was seen.
//!
//! \return true when pOut now holds nExpandedSize bytes and no decode is needed.
bool SectionCacheLookup( const SSectionKey &key, uint8_t *pOut );

//! Remember an expansion. Ignored when the cache is full or the entry is large.
//!
//! Called only after a successful decode: a section that failed to expand has
//! nothing worth keeping, and keeping it would turn one corrupt file into a
//! cached wrong answer for every later file that shares its bytes.
void SectionCacheInsert( const SSectionKey &key, const uint8_t *pExpanded );

//! Totals, for the test and for anyone measuring. Not a performance counter in a
//! release build; reading it takes the lock.
struct SSectionCacheStats
{
	uint64_t nLookups = 0;
	uint64_t nHits = 0;
	//! Lookups that matched a hash but failed the byte comparison. Expected to be
	//! zero; a non-zero value means the hash is weaker than assumed.
	uint64_t nCollisions = 0;
	uint64_t nEntries = 0;
	uint64_t nBytesHeld = 0;
	uint64_t nBytesServed = 0;
};

SSectionCacheStats SectionCacheGetStats();

//! Drop everything. For tests, and for a caller that wants the memory back.
void SectionCacheClear();

}
