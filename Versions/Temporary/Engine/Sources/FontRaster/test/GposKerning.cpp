// GPOS pair kerning against HarfBuzz's.
//
// Oswald, the font the game's headers use, keeps its kerning only in GPOS, in
// both a per-pair and a class-based subtable. The expected adjustments are
// HarfBuzz's (uharfbuzz 0.x), shaping each pair once with the kern feature and
// once without and taking the difference in the first glyph's advance, at the
// font's default instance and 1000 units per em. Glyph indices are used
// directly, so the test does not depend on the character map.

#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "FontFace.h"
#include "GposKerning.h"

#include <gtest/gtest.h>

namespace {

std::vector<uint8_t> ReadFile( const std::string &szPath )
{
	std::ifstream file( szPath, std::ios::binary );
	return std::vector<uint8_t>( std::istreambuf_iterator<char>( file ), std::istreambuf_iterator<char>() );
}

uint32_t ReadU32( const std::vector<uint8_t> &data, size_t nOffset )
{
	return ( uint32_t( data[nOffset] ) << 24 ) | ( uint32_t( data[nOffset + 1] ) << 16 ) | ( uint32_t( data[nOffset + 2] ) << 8 ) | data[nOffset + 3];
}

// The bytes of one table of an sfnt font, from its table directory
std::vector<uint8_t> ReadTable( const std::vector<uint8_t> &font, const char *pszTag )
{
	const int nTables = ( font[4] << 8 ) | font[5];
	for ( int i = 0; i < nTables; ++i )
	{
		const size_t nRecord = 12 + i * 16;
		if ( std::string( reinterpret_cast<const char*>( &font[nRecord] ), 4 ) == pszTag )
		{
			const uint32_t nOffset = ReadU32( font, nRecord + 8 );
			const uint32_t nLength = ReadU32( font, nRecord + 12 );
			return std::vector<uint8_t>( font.begin() + nOffset, font.begin() + nOffset + nLength );
		}
	}
	return std::vector<uint8_t>();
}

struct SPair
{
	const char *pszText;
	uint16_t nLeft, nRight;
	int nExpected;
};

// HarfBuzz's answers, see the top of the file; the Cyrillic pairs are named in
// Latin transliteration
const SPair PAIRS[] =
{
	{ "AV", 1, 198, -39 }, { "VA", 198, 1, -39 }, { "To", 170, 338, -63 }, { "Ta", 170, 218, -65 },
	{ "LT", 103, 170, -78 }, { "Yo", 205, 338, -60 }, { "P,", 152, 706, -103 }, { "Te", 170, 261, -63 },
	{ "Ty", 170, 423, -60 }, { "g.", 566, 705, 0 }, { "Ta (Cyrillic)", 473, 563, -65 }, { "TA (Cyrillic)", 473, 445, -41 },
	{ "Ku (Cyrillic)", 464, 592, -3 }, { "G. (Cyrillic)", 448, 705, -53 }, { "AU (Cyrillic)", 445, 474, 0 },
	{ "av", 218, 416, -3 }, { "rv", 373, 416, 0 }, { "f.", 284, 705, -13 },
};

// The font is game data, which a checkout of the sources alone does not have;
// the test then skips, as the DDS tests do without the shipped textures
bool HaveOswald()
{
	return !ReadFile( OSWALD_TTF ).empty();
}

TEST( GposKerning, MatchesHarfBuzzOnOswald )
{
	if ( !HaveOswald() )
		GTEST_SKIP() << "no font at " << OSWALD_TTF;
	const std::vector<uint8_t> font = ReadFile( OSWALD_TTF );
	const NFontRaster::CGposKerning kerning( ReadTable( font, "GPOS" ) );
	ASSERT_FALSE( kerning.IsEmpty() );
	for ( const SPair &pair : PAIRS )
	{
		EXPECT_EQ( kerning.GetAdjustment( pair.nLeft, pair.nRight ), pair.nExpected ) << pair.pszText;
	}
}

TEST( GposKerning, DamagedTablesGiveNothing )
{
	EXPECT_TRUE( NFontRaster::CGposKerning( std::vector<uint8_t>() ).IsEmpty() );
	if ( !HaveOswald() )
		GTEST_SKIP() << "no font at " << OSWALD_TTF;
	// every prefix of a real table must parse without reading past its end;
	// what the prefix still covers may kern, and nothing may crash
	const std::vector<uint8_t> gpos = ReadTable( ReadFile( OSWALD_TTF ), "GPOS" );
	ASSERT_FALSE( gpos.empty() );
	for ( size_t nLength = 0; nLength < gpos.size(); nLength += 97 )
	{
		const NFontRaster::CGposKerning truncated( std::vector<uint8_t>( gpos.begin(), gpos.begin() + nLength ) );
		truncated.GetAdjustment( 1, 198 );
	}
}

TEST( GposKerning, FaceWithoutKernTableIsKerned )
{
	// The same pair through CFace, in pixels: A V narrows at a 41 pixel cell,
	// which the kern table path alone would have left at 0
	if ( !HaveOswald() )
		GTEST_SKIP() << "no font at " << OSWALD_TTF;
	std::string szError;
	std::unique_ptr<NFontRaster::CFace> pFace = NFontRaster::CFace::OpenFile( OSWALD_TTF, 0, &szError );
	ASSERT_TRUE( pFace != nullptr ) << szError;
	NFontRaster::SOptions options;
	options.nCellHeight = 41;
	ASSERT_TRUE( pFace->Fit( options, { 'A', 'V', 'H' }, &szError ) ) << szError;
	EXPECT_TRUE( pFace->HasKerning() );
	EXPECT_LT( pFace->GetKerning( 'A', 'V' ), 0 );
	EXPECT_EQ( pFace->GetKerning( 'H', 'H' ), 0 );
}

}
