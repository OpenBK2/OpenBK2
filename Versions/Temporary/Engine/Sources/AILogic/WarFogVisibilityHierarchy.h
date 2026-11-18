#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// Derived visibility summaries. Update only when a real tile crosses hidden/visible,
// not when the number of observers changes while the tile remains visible.
class CWarFogVisibilityHierarchy
{
public:
	enum class EVisibility
	{
		Invisible,
		PartiallyVisible,
		Visible
	};

	static constexpr int BASE_TILE_SIZE = 8;

	struct SSearchStats
	{
		int nBlocks = 0;
		int nTiles = 0;
	};

private:
	struct SBlock
	{
		// At the base these count real tiles; above it they count child blocks.
		uint16_t nNonempty = 0;
		uint16_t nFull = 0;
		uint16_t nChildren = 0;

		EVisibility GetVisibility() const
		{
			if ( nNonempty == 0 )
			{
				return EVisibility::Invisible;
			}
			return nFull == nChildren ? EVisibility::Visible : EVisibility::PartiallyVisible;
		}
	};

	struct SLevel
	{
		int nTileSize = 0;
		int nWidth = 0;
		int nHeight = 0;
		std::vector<SBlock> blocks;
	};

	struct SQuery
	{
		double fX;
		double fY;
		double fBestSquared;
		int nTileSize;
		SSearchStats *pStats;

		double DistanceSquared( int x0, int y0, int x1, int y1 ) const
		{
			const double fDX = fX - std::clamp( fX, double( x0 ) * nTileSize, double( x1 ) * nTileSize );
			const double fDY = fY - std::clamp( fY, double( y0 ) * nTileSize, double( y1 ) * nTileSize );
			return fDX * fDX + fDY * fDY;
		}
	};

	int nWidth = 0;
	int nHeight = 0;
	std::vector<SLevel> levels;

	template <class TIsVisible>
	void SearchBlock( int nLevel, int x, int y, SQuery &query, const TIsVisible &isVisible ) const
	{
		const SLevel &level = levels[nLevel];
		if ( query.pStats )
		{
			++query.pStats->nBlocks;
		}
		const EVisibility visibility = level.blocks[y * level.nWidth + x].GetVisibility();
		if ( visibility == EVisibility::Invisible )
		{
			return;
		}
		const int x0 = x * level.nTileSize;
		const int y0 = y * level.nTileSize;
		const int x1 = (std::min)( x0 + level.nTileSize, nWidth );
		const int y1 = (std::min)( y0 + level.nTileSize, nHeight );
		const double fDistanceSquared = query.DistanceSquared( x0, y0, x1, y1 );
		if ( fDistanceSquared >= query.fBestSquared )
		{
			return;
		}
		if ( visibility == EVisibility::Visible )
		{
			// Every point in this clipped rectangle is visible, so no leaf scan is needed.
			query.fBestSquared = fDistanceSquared;
			return;
		}
		if ( nLevel > 0 )
		{
			const SLevel &children = levels[nLevel - 1];
			for ( int cy = y * 2; cy < (std::min)( y * 2 + 2, children.nHeight ); ++cy )
			{
				for ( int cx = x * 2; cx < (std::min)( x * 2 + 2, children.nWidth ); ++cx )
				{
					SearchBlock( nLevel - 1, cx, cy, query, isVisible );
				}
			}
			return;
		}
		for ( int ty = y0; ty < y1; ++ty )
		{
			for ( int tx = x0; tx < x1; ++tx )
			{
				const double fTileDistanceSquared = query.DistanceSquared( tx, ty, tx + 1, ty + 1 );
				if ( fTileDistanceSquared < query.fBestSquared )
				{
					if ( query.pStats )
					{
						++query.pStats->nTiles;
					}
					if ( isVisible( tx, ty ) )
					{
						query.fBestSquared = fTileDistanceSquared;
					}
				}
			}
		}
	}

public:
	void Clear()
	{
		levels.clear();
		nWidth = nHeight = 0;
	}

	void Init( int nTilesX, int nTilesY )
	{
		Clear();
		if ( nTilesX <= 0 || nTilesY <= 0 )
		{
			return;
		}
		nWidth = nTilesX;
		nHeight = nTilesY;
		int nChildrenX = nWidth;
		int nChildrenY = nHeight;
		int nGroupSize = BASE_TILE_SIZE;
		int nTileSize = BASE_TILE_SIZE;
		for ( ;; )
		{
			levels.emplace_back();
			SLevel &level = levels.back();
			level.nTileSize = nTileSize;
			level.nWidth = ( nChildrenX + nGroupSize - 1 ) / nGroupSize;
			level.nHeight = ( nChildrenY + nGroupSize - 1 ) / nGroupSize;
			level.blocks.resize( level.nWidth * level.nHeight );
			for ( int y = 0; y < level.nHeight; ++y )
			{
				for ( int x = 0; x < level.nWidth; ++x )
				{
					level.blocks[y * level.nWidth + x].nChildren = (std::min)( nGroupSize, nChildrenX - x * nGroupSize ) *
					                                               (std::min)( nGroupSize, nChildrenY - y * nGroupSize );
				}
			}
			if ( level.nWidth <= 2 && level.nHeight <= 2 )
			{
				break;
			}
			nChildrenX = level.nWidth;
			nChildrenY = level.nHeight;
			nGroupSize = 2;
			nTileSize *= 2;
		}
	}

	void UpdateTile( int x, int y, bool bVisible )
	{
		x /= BASE_TILE_SIZE;
		y /= BASE_TILE_SIZE;
		int nNonemptyDelta = bVisible ? 1 : -1;
		int nFullDelta = nNonemptyDelta;
		for ( SLevel &level : levels )
		{
			SBlock &block = level.blocks[y * level.nWidth + x];
			const EVisibility oldVisibility = block.GetVisibility();
			block.nNonempty += nNonemptyDelta;
			block.nFull += nFullDelta;
			const EVisibility newVisibility = block.GetVisibility();
			if ( oldVisibility == newVisibility )
			{
				// Parent summaries count child states, so changes inside a partial block stop here.
				break;
			}
			nNonemptyDelta = int( newVisibility != EVisibility::Invisible ) - int( oldVisibility != EVisibility::Invisible );
			nFullDelta = int( newVisibility == EVisibility::Visible ) - int( oldVisibility == EVisibility::Visible );
			x /= 2;
			y /= 2;
		}
	}

	// Inputs are finite, the limit is positive, and isVisible reads the same tiles used for updates.
	// Optional work counts let tests verify pruning without relying on timing thresholds.
	template <class TIsVisible>
	double FindDistanceSquared( double fX,
	                            double fY,
	                            double fLimit,
	                            int nTileSize,
	                            const TIsVisible &isVisible,
	                            SSearchStats *pStats = nullptr ) const
	{
		SQuery query{ fX, fY, fLimit * fLimit, nTileSize, pStats };
		if ( levels.empty() )
		{
			return query.fBestSquared;
		}
		int nLevel = 0;
		while ( nLevel + 1 < levels.size() && double( levels[nLevel].nTileSize ) * nTileSize < 2.0 * fLimit )
		{
			++nLevel;
		}
		const SLevel &level = levels[nLevel];
		const double fBlockSize = double( level.nTileSize ) * nTileSize;
		// Clip before integer conversion; include touching lower edges even for tiny limits.
		const double fMinX = (std::max)( 0.0, std::ceil( ( fX - fLimit ) / fBlockSize ) - 1.0 );
		const double fMinY = (std::max)( 0.0, std::ceil( ( fY - fLimit ) / fBlockSize ) - 1.0 );
		const double fMaxX = (std::min)( double( level.nWidth - 1 ), std::floor( ( fX + fLimit ) / fBlockSize ) );
		const double fMaxY = (std::min)( double( level.nHeight - 1 ), std::floor( ( fY + fLimit ) / fBlockSize ) );
		if ( fMinX <= fMaxX && fMinY <= fMaxY )
		{
			for ( int y = int( fMinY ); y <= int( fMaxY ); ++y )
			{
				for ( int x = int( fMinX ); x <= int( fMaxX ); ++x )
				{
					SearchBlock( nLevel, x, y, query, isVisible );
				}
			}
		}
		return query.fBestSquared;
	}
};
