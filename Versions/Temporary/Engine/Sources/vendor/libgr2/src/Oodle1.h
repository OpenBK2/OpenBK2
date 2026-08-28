#pragma once

// Oodle1, the codec 61% of this game's GR2 files are compressed with.
//
// RAD's own, from around 2002, and unrelated to anything a general purpose
// library implements. It is not deflate, not LZ4, not LZMA, and the modern Oodle
// that shares the name is a different codec family entirely. It is an adaptive
// arithmetic coder over an LZ dictionary, and the only way to read these files is
// to implement it. See Oodle1.cpp for provenance and for how it works.

#include <cstdint>

namespace NGr2
{

//! Expand one Oodle1 compressed section.
//!
//! A section decodes in three stages with separate dictionaries but one shared
//! arithmetic decoder, split at nStop0 and nStop1 and ending at
//! nDecompressedSize. The two stops are the section record's first16Bit and
//! first8Bit, which is Granny keeping its 32-bit, 16-bit and 8-bit data apart so
//! that each gets its own statistics. In this game they are always equal, so the
//! middle stage is always empty, but the format has three and so does this.
//!
//! \param pCompressed nCompressedSize bytes, not modified and not retained.
//! \param pDecompressed nDecompressedSize bytes of room, written from the start.
//! \return false on a stream that does not decode, having written no more than
//!         nDecompressedSize bytes. A truncated or corrupt section fails here
//!         rather than reading or writing past either buffer.
bool Oodle1Decompress( const uint8_t *pCompressed, uint32_t nCompressedSize, uint32_t nStop0,
                       uint32_t nStop1, uint8_t *pDecompressed, uint32_t nDecompressedSize );

}
