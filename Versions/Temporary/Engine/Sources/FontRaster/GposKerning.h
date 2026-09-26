#pragma once

// Pair kerning from an OpenType font's GPOS table.
//
// FreeType's FT_Get_Kerning reads only the legacy kern table, and many modern
// fonts carry their kerning in GPOS alone: Oswald and Noto Sans among the ones
// the game uses or considered. This reads the part of GPOS that holds it, the
// pair adjustment lookups (type 2, in both the per-pair and the class-based
// format, directly or wrapped in an extension lookup, type 9) that the font's
// "kern" feature refers to, and answers how much a pair adjusts the first
// glyph's advance.
//
// Not read: contextual kerning (chained lookups, which Noto Sans also has for a
// few sequences), anything but the horizontal advance, and a variable font's
// per-instance deltas, so a named instance gets the default instance's values.
// Every read is bounds checked, since a font can come from a mod.

#include <cstdint>
#include <vector>

namespace NFontRaster
{

class CGposKerning
{
	std::vector<uint8_t> table;
	// the pair adjustment subtables, as offsets into table, grouped by lookup
	// in lookup order: within a lookup the first subtable that covers a pair
	// decides it, and separate lookups add up
	std::vector<std::vector<uint32_t>> lookups;

	int FindInSubtable( uint32_t nSubtable, uint16_t nLeft, uint16_t nRight, bool *pbFound ) const;
public:
	// Takes the GPOS table's bytes, which it keeps; an empty or unreadable
	// table gives no kerning at all
	explicit CGposKerning( std::vector<uint8_t> _table );
	bool IsEmpty() const { return lookups.empty(); }
	// The adjustment to nLeft's advance when nRight follows it, in font units
	int GetAdjustment( uint16_t nLeft, uint16_t nRight ) const;
};

}
