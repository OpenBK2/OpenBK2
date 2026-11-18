#include "../WarFogVisibilityHierarchy.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace
{
class SFog
{
public:
	int nWidth;
	int nHeight;
	std::vector<uint8_t> tiles;
	CWarFogVisibilityHierarchy hierarchy;

	SFog( int width, int height )
		: nWidth( width )
		, nHeight( height )
		, tiles( width * height, 0 )
	{
		hierarchy.Init( width, height );
	}

	void Set( int x, int y, bool visible )
	{
		if ( bool( tiles[y * nWidth + x] ) != visible )
		{
			tiles[y * nWidth + x] = visible;
			hierarchy.UpdateTile( x, y, visible );
		}
	}

	double Query( double x, double y, double limit, CWarFogVisibilityHierarchy::SSearchStats *stats = nullptr ) const
	{
		return hierarchy.FindDistanceSquared(
				x, y, limit, 1, [&]( int tx, int ty ) { return tiles[ty * nWidth + tx] != 0; }, stats );
	}

	double Reference( double px, double py, double limit ) const
	{
		double best = limit * limit;
		for ( int y = 0; y < nHeight; ++y )
		{
			for ( int x = 0; x < nWidth; ++x )
			{
				if ( tiles[y * nWidth + x] )
				{
					const double dx = (std::max)( 0.0, std::abs( px - x - 0.5 ) - 0.5 );
					const double dy = (std::max)( 0.0, std::abs( py - y - 0.5 ) - 0.5 );
					best = (std::min)( best, dx * dx + dy * dy );
				}
			}
		}
		return best;
	}
};

TEST( WarFogVisibilityHierarchy, OnePatchAndLargestEditorMapBothHaveFourTopBlocks )
{
	for ( int size : { 16, 512 } )
	{
		SFog fog( size, size );
		CWarFogVisibilityHierarchy::SSearchStats stats;
		EXPECT_DOUBLE_EQ( fog.Query( size / 2.0, size / 2.0, 10000, &stats ), 100000000.0 );
		EXPECT_EQ( stats.nBlocks, 4 );
		EXPECT_EQ( stats.nTiles, 0 );
	}
}

TEST( WarFogVisibilityHierarchy, DistantHiddenRegionSkipsAllRealTiles )
{
	SFog fog( 512, 512 );
	fog.Set( 10, 10, true );
	CWarFogVisibilityHierarchy::SSearchStats stats;
	EXPECT_DOUBLE_EQ( fog.Query( 400.5, 400.5, 30, &stats ), 900.0 );
	EXPECT_LE( stats.nBlocks, 4 );
	EXPECT_EQ( stats.nTiles, 0 );
}

TEST( WarFogVisibilityHierarchy, FullBlockUsesItsRectangleAndPartialChangesPropagate )
{
	SFog fog( 32, 32 );
	for ( int y = 0; y < 16; ++y )
	{
		for ( int x = 0; x < 16; ++x )
		{
			fog.Set( x, y, true );
		}
	}
	CWarFogVisibilityHierarchy::SSearchStats stats;
	EXPECT_DOUBLE_EQ( fog.Query( 17, 17, 100, &stats ), 2.0 );
	EXPECT_EQ( stats.nTiles, 0 );
	fog.Set( 15, 15, false );
	EXPECT_DOUBLE_EQ( fog.Query( 17, 17, 100 ), 5.0 );
	fog.Set( 14, 15, false );
	EXPECT_DOUBLE_EQ( fog.Query( 17, 17, 100 ), 5.0 );
	fog.Set( 15, 15, true );
	EXPECT_DOUBLE_EQ( fog.Query( 17, 17, 100 ), 2.0 );
	fog.Set( 14, 15, true );
	stats = {};
	EXPECT_DOUBLE_EQ( fog.Query( 17, 17, 100, &stats ), 2.0 );
	EXPECT_EQ( stats.nTiles, 0 );
	for ( int y = 0; y < 16; ++y )
	{
		for ( int x = 0; x < 16; ++x )
		{
			fog.Set( x, y, false );
		}
	}
	stats = {};
	EXPECT_DOUBLE_EQ( fog.Query( 17, 17, 100, &stats ), 10000.0 );
	EXPECT_EQ( stats.nTiles, 0 );
}

TEST( WarFogVisibilityHierarchy, PartialEdgeBlocksBecomeFullWithoutCountingOutsideTiles )
{
	SFog fog( 19, 9 );
	for ( int y = 0; y < fog.nHeight; ++y )
	{
		for ( int x = 0; x < fog.nWidth; ++x )
		{
			fog.Set( x, y, true );
		}
	}
	CWarFogVisibilityHierarchy::SSearchStats stats;
	EXPECT_DOUBLE_EQ( fog.Query( 20, 10, 100, &stats ), 2.0 );
	EXPECT_EQ( stats.nTiles, 0 );
	fog.Set( 18, 8, false );
	EXPECT_DOUBLE_EQ( fog.Query( 20, 10, 100 ), 5.0 );
}

TEST( WarFogVisibilityHierarchy, ChangingMixedMapsMatchesExhaustiveDistances )
{
	for ( const auto &size : { std::pair<int, int>( 1, 1 ), { 5, 37 }, { 67, 49 } } )
	{
		SFog fog( size.first, size.second );
		for ( int phase = 0; phase < 5; ++phase )
		{
			for ( int y = 0; y < fog.nHeight; ++y )
			{
				for ( int x = 0; x < fog.nWidth; ++x )
				{
					// Uniform, empty, dense and sparse phases exercise every state transition without RNG.
					fog.Set( x, y, phase == 1 || ( phase > 1 && ( x * 37 + y * 13 + phase ) % ( phase * 11 ) == 0 ) );
				}
			}
			for ( int sample = 0; sample < 40; ++sample )
			{
				const double x = ( sample * 17 ) % ( fog.nWidth + 8 ) - 4.25;
				const double y = ( sample * 29 ) % ( fog.nHeight + 8 ) - 3.5;
				for ( double limit : { 0.01, 1.0, 7.0, 16.0, 31.0, 200.0 } )
				{
					EXPECT_DOUBLE_EQ( fog.Query( x, y, limit ), fog.Reference( x, y, limit ) )
							<< "size=" << fog.nWidth << "," << fog.nHeight << " phase=" << phase << " sample=" << sample;
				}
			}
		}
	}
}

TEST( WarFogVisibilityHierarchy, ClearAndReinitializationDiscardAllSummaries )
{
	SFog fog( 32, 32 );
	fog.Set( 10, 10, true );
	EXPECT_DOUBLE_EQ( fog.Query( 10.5, 10.5, 10 ), 0.0 );
	fog.hierarchy.Clear();
	EXPECT_DOUBLE_EQ( fog.Query( 10.5, 10.5, 10 ), 100.0 );
	fog.hierarchy.Init( 32, 32 );
	EXPECT_DOUBLE_EQ( fog.Query( 10.5, 10.5, 10 ), 100.0 );
}
} // namespace
