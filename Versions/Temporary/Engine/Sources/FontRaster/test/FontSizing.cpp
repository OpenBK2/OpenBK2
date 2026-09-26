#include "CodePages.h"
#include "FontFace.h"
#include "FontFinder.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <gtest/gtest.h>

namespace
{
std::vector<uint32_t> SizingCharacters()
{
	std::vector<uint32_t> result;
	for ( int charset : { NCodePages::CHARSET_ANSI, NCodePages::CHARSET_EASTEUROPE,
		NCodePages::CHARSET_RUSSIAN, NCodePages::CHARSET_GREEK, NCodePages::CHARSET_TURKISH, NCodePages::CHARSET_BALTIC } )
	{
		const auto chars = NCodePages::GetPrintableCodePoints( charset );
		result.insert( result.end(), chars.begin(), chars.end() );
	}
	std::sort( result.begin(), result.end() );
	result.erase( std::unique( result.begin(), result.end() ), result.end() );
	return result;
}

std::shared_ptr<std::vector<uint8_t>> ReadFont( const char *name )
{
	std::ifstream file( std::string( FONT_DATA_DIR ) + "/" + name, std::ios::binary );
	return std::make_shared<std::vector<uint8_t>>( std::istreambuf_iterator<char>( file ), std::istreambuf_iterator<char>() );
}
}

TEST( FontSizing, KeepsLegacyCapHeightAndFitsEuropeanAccents )
{
	// Ratios measured from H in the shipped Body, Header2 and Numeric atlases.
	// In particular Oswald's Baltic accents must not reduce all its capitals.
	const struct { const char *file; int weight; double capRatio; } fonts[] = {
		{ "PTSans-Regular.ttf", 400, 10.0 / 16 },
		{ "Oswald-Variable.ttf", 700, 13.0 / 20 },
		{ "LiberationSans-Regular.ttf", 400, 8.0 / 14 },
	};
	const auto chars = SizingCharacters();
	for ( const auto &font : fonts )
	{
		auto data = ReadFont( font.file );
		if ( data->empty() )
			GTEST_SKIP() << "Missing shipped font: " << font.file;
		std::string error;
		auto face = NFontRaster::CFace::OpenMemory( data, NFontRaster::SelectFace( *data, font.weight, false ), &error );
		ASSERT_TRUE( face ) << error;
		for ( int size : { 6, 14, 20, 28, 33, 41, 79, 100 } )
		{
			SCOPED_TRACE( std::string( font.file ) + " size " + std::to_string( size ) );
			NFontRaster::SOptions options;
			options.eCellMetrics = NFontRaster::CELL_INK;
			options.nCellHeight = size;
			options.nCapHeight = static_cast<int>( std::lround( size * font.capRatio ) );
			ASSERT_TRUE( face->Fit( options, chars, &error ) ) << error;
			NFontRaster::SGlyphBitmap capital;
			ASSERT_TRUE( face->RenderGlyph( 'H', &capital, &error ) ) << error;
			EXPECT_NEAR( capital.nRows, options.nCapHeight, 1 );
			const auto &metrics = face->GetMetrics();
			EXPECT_EQ( metrics.nAscent + metrics.nDescent, metrics.nCellHeight );
			for ( uint32_t code : chars )
			{
				NFontRaster::SGlyphBitmap glyph;
				ASSERT_TRUE( face->RenderGlyph( code, &glyph, &error ) ) << error;
				if ( glyph.nRows == 0 )
					continue;
				EXPECT_GE( metrics.nAscent - glyph.nTop, 0 ) << code;
				EXPECT_LE( metrics.nAscent - glyph.nTop + glyph.nRows, metrics.nCellHeight ) << code;
			}
		}
	}
}

TEST( FontSizing, RasterizesHorizontalScaleWithoutChangingHeight )
{
	auto data = ReadFont( "Oswald-Variable.ttf" );
	if ( data->empty() )
		GTEST_SKIP() << "Missing shipped Oswald font";
	std::string error;
	auto face = NFontRaster::CFace::OpenMemory( data, NFontRaster::SelectFace( *data, 700, false ), &error );
	ASSERT_TRUE( face ) << error;
	NFontRaster::SOptions options;
	options.eCellMetrics = NFontRaster::CELL_INK;
	options.nCellHeight = 41;
	options.nCapHeight = 27;
	const auto chars = SizingCharacters();
	ASSERT_TRUE( face->Fit( options, chars, &error ) ) << error;
	NFontRaster::SGlyphBitmap normal;
	ASSERT_TRUE( face->RenderGlyph( 'H', &normal, &error ) ) << error;
	const int normalKern = face->GetKerning( 'A', 'V' );
	for ( double scale : { 0.75, 4.0 / 3, 2.0 } )
	{
		options.fWidthScale = scale;
		ASSERT_TRUE( face->Fit( options, chars, &error ) ) << error;
		NFontRaster::SGlyphBitmap wide;
		ASSERT_TRUE( face->RenderGlyph( 'H', &wide, &error ) ) << error;
		EXPECT_EQ( wide.nRows, normal.nRows );
		EXPECT_NEAR( wide.nA + wide.nB + wide.nC, ( normal.nA + normal.nB + normal.nC ) * scale, 1 );
		EXPECT_NEAR( face->GetKerning( 'A', 'V' ), normalKern * scale, 1 );
	}
}

TEST( FontSizing, BatchAtlasUsesTheExpandedCell )
{
	auto data = ReadFont( "Oswald-Variable.ttf" );
	if ( data->empty() )
		GTEST_SKIP() << "Missing shipped Oswald font";
	NFontRaster::SOptions options;
	options.szFontFile = std::string( FONT_DATA_DIR ) + "/Oswald-Variable.ttf";
	options.nFaceIndex = NFontRaster::SelectFace( *data, 700, false );
	options.eCellMetrics = NFontRaster::CELL_INK;
	options.nCellHeight = 20;
	options.nCapHeight = 13;
	options.fWidthScale = 4.0 / 3;
	NFontRaster::SFont font;
	std::string error;
	ASSERT_TRUE( NFontRaster::Rasterise( options, SizingCharacters(), &font, &error ) ) << error;
	EXPECT_GT( font.nCellHeight, options.nCellHeight );
	EXPECT_TRUE( font.clipped.empty() );
	for ( const auto &glyph : font.glyphs )
	{
		EXPECT_EQ( glyph.y2 - glyph.y1, font.nCellHeight );
		EXPECT_LE( glyph.x2, font.nAtlasWidth );
		EXPECT_LE( glyph.y2, font.nAtlasHeight );
	}
}
