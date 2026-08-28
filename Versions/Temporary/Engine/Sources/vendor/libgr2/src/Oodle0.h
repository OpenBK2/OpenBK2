#pragma once

// Oodle0, the codec the other 6,016 of the retail install's 21,720 GR2 files are
// compressed with, and the one the first mission needs.
//
// Not a variant of Oodle1 despite the neighbouring name. Oodle1 is an arithmetic
// coder over an LZ dictionary with weight windows; this is a range coder over an
// adaptive symbol model with escapes, and an LZ layer above it. The one thing
// they share is the twelve-byte parameter block, three of them, at the front.
//
// Knit's documentation calls Oodle0 "functionally the same as Oodle1 modulo
// endianness". That is not true of this game's files: its decoder throws or emits
// zeros on them while succeeding on Oodle1. See Oodle0.cpp.

#include <cstdint>

namespace NGr2
{

//! Expand one Oodle0 compressed section.
//!
//! The same three-stage shape as Oodle1: separate models per stage, one shared
//! bit stream, split at nStop0 and nStop1 and ending at nDecompressedSize. In
//! this game the two stops are equal, so the middle stage is always empty.
//!
//! \param pCompressed nCompressedSize bytes, not modified and not retained.
//! \param pDecompressed nDecompressedSize bytes of room, written from the start.
//! \return false on a stream that does not decode, having written no more than
//!         nDecompressedSize bytes.
bool Oodle0Decompress( const uint8_t *pCompressed, uint32_t nCompressedSize, uint32_t nStop0,
                       uint32_t nStop1, uint8_t *pDecompressed, uint32_t nDecompressedSize );

}
