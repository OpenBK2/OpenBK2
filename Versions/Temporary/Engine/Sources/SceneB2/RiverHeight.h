#pragma once

// How a river lowers the terrain around it. Each river has two banks, ridgeL and
// ridgeR, built as ridges (see RidgeHeight.h) from the river's edge out to
// DEF_RIVER_RIDGE_WIDTH beyond it, and its bed, samples, between the edges. Kept apart
// from CTerraGen so that it can be tested without one.

#include "RidgeHeight.h"
#include "TerrainInfo.h"

#include <algorithm>
#include <cmath>
#include <list>
#include <vector>

#define DEF_RIVER_DEPTH 4.0f
#define DEF_RIVER_RIDGE_WIDTH ( DEF_TILE_SIZE * 4 )
#define DEF_RIVER_RIDGE_NULL ( DEF_TILE_SIZE * FP_SQRT_2 / DEF_RIVER_RIDGE_WIDTH )
#define DEF_RIVER_RIDGE_COEFF ( 1.0f / ( 1.0f - DEF_RIVER_RIDGE_NULL ) )

struct SRiverProfile
{
	float operator()( const float x ) const
	{
		// TODO: old riverprofile
		if ( x <= DEF_RIVER_RIDGE_NULL )
		{
			return 1.0f;
		}
		const float tx = ( x - DEF_RIVER_RIDGE_NULL ) * DEF_RIVER_RIDGE_COEFF;
		return 1.0f - tx * tx;
		//return 1.0f - x * x;

		// TODO: new riverprofile
		//if ( x <= DEF_RIVER_RIDGE_NULL )
		//	return 1.0f;
		//const float tx = ( x - DEF_RIVER_RIDGE_NULL ) / ( 1.0f - DEF_RIVER_RIDGE_NULL );
		//return ( 1 + cos(FP_PI * tx) ) / 2.0f;
	}
};

struct SRiverBottomProfile
{
	float operator()( const float x ) const
	{
		return 1.0f;
	}
};

// A box around one quad of a ridge, padded so that a point outside it cannot pass
// the barycentric test of either of the quad's triangles.
struct SRidgeQuadBox
{
	float fMinX;
	float fMinY;
	float fMaxX;
	float fMaxY;

	bool MayContain( const CVec2 &v ) const
	{
		return v.x >= fMinX && v.x <= fMaxX && v.y >= fMinY && v.y <= fMaxY;
	}
};

// Twice the signed area of a triangle, in double: only used to judge its shape.
inline double GetDoubleArea( const CVec3fEx &p0, const CVec3fEx &p1, const CVec3fEx &p2 )
{
	return ( double( p1.x ) - p0.x ) * ( double( p2.y ) - p0.y ) - ( double( p1.y ) - p0.y ) * ( double( p2.x ) - p0.x );
}

// One box per quad of the ridge, in the order GetIncRidgeHeightFiltered visits them.
//
// The barycentric test accepts a point up to DEF_EPS outside each edge in barycentric
// units, which is at most 2 * DEF_EPS times the sum of two edges in distance, so under
// 4e-4 of the quad's longest span. On top of that comes the rounding in GetBaryCoords:
// relative to the quad's size it stays small while the triangle is not thin, and in
// absolute terms it grows with the distance from the origin. The box is padded by 1%
// of the longest span, 25 times the slack, plus a millionth of the largest coordinate,
// eight float steps or so. A quad with a thin triangle, whose rounding is not bounded
// that way, gets a box that contains everything and is always tested, as before.
inline void MakeRidgeQuadBoxes( const std::vector<CVec3fEx> &ridge, std::vector<SRidgeQuadBox> *pBoxes )
{
	// the area of a triangle that counts as thin, as a fraction of its quad's span squared
	const double fThin = 1e-3;
	const double fPadOfSpan = 1e-2;
	const double fPadOfCoord = 1e-6;
	const SRidgeQuadBox everything = { -FP_MAX_VALUE, -FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE };

	pBoxes->clear();
	const int nHalfSize = ( ridge.size() >> 1 ) - 1;
	for ( int k = 0; k < nHalfSize; ++k )
	{
		// the corners as GetIncRidgeHeight uses them: triangles 1, 2, 0 and 3, 0, 2
		const CVec3fEx *corners[4] = {
			&ridge[k], &ridge[k + 1], &ridge[ridge.size() - k - 2], &ridge[ridge.size() - k - 1] };
		double fMinX = corners[0]->x, fMaxX = corners[0]->x, fMinY = corners[0]->y, fMaxY = corners[0]->y;
		double fLargestCoord = 0.0;
		double fSpan2 = 0.0;
		for ( int i = 0; i < 4; ++i )
		{
			const CVec3fEx &v = *corners[i];
			fMinX = (std::min)( fMinX, double( v.x ) );
			fMaxX = (std::max)( fMaxX, double( v.x ) );
			fMinY = (std::min)( fMinY, double( v.y ) );
			fMaxY = (std::max)( fMaxY, double( v.y ) );
			fLargestCoord = (std::max)( fLargestCoord, (std::max)( fabs( double( v.x ) ), fabs( double( v.y ) ) ) );
			for ( int j = i + 1; j < 4; ++j )
			{
				const double dx = double( corners[j]->x ) - v.x;
				const double dy = double( corners[j]->y ) - v.y;
				fSpan2 = (std::max)( fSpan2, dx * dx + dy * dy );
			}
		}
		const double fArea1 = fabs( GetDoubleArea( *corners[1], *corners[2], *corners[0] ) );
		const double fArea2 = fabs( GetDoubleArea( *corners[3], *corners[0], *corners[2] ) );
		if ( !( fSpan2 > 0.0 ) || fArea1 < fThin * fSpan2 || fArea2 < fThin * fSpan2 )
		{
			pBoxes->push_back( everything );
			continue;
		}
		const double fPad = fPadOfSpan * sqrt( fSpan2 ) + fPadOfCoord * fLargestCoord;
		const SRidgeQuadBox box = {
			float( fMinX - fPad ), float( fMinY - fPad ), float( fMaxX + fPad ), float( fMaxY + fPad ) };
		pBoxes->push_back( box );
	}
}

// The depth the rivers give the terrain, as a value to add to its height: at each
// point the deepest of the banks that contain it, or DEF_RIVER_DEPTH in a river's bed,
// or 0. A point is in at most a few of a river's hundreds of quads, so each quad gets
// a box once, here, and a point skips the quads whose box it is outside: on Mission
// Impossible that rules out 99.97% of the triangles the original tested, and the
// result is the same to the bit (see SceneB2/test/RiverHeight.cpp). Keeps pointers
// into the river list, which must outlive it and not change meanwhile.
class CRiverHeights
{
	struct SRiverBoxes
	{
		const STerrainInfo::SRiver *pRiver;
		std::vector<SRidgeQuadBox> left;
		std::vector<SRidgeQuadBox> right;
		std::vector<SRidgeQuadBox> bed;
	};

	std::vector<SRiverBoxes> rivers;

	template <class THeightCalc>
	static bool GetHeight( const CVec2 &v,
	                       const std::vector<CVec3fEx> &ridge,
	                       const std::vector<SRidgeQuadBox> &boxes,
	                       float *pHeight,
	                       const THeightCalc &heightCalc )
	{
		return GetIncRidgeHeightFiltered(
				v, ridge, pHeight, heightCalc, [&]( const int k ) { return boxes[k].MayContain( v ); } );
	}

public:
	explicit CRiverHeights( const std::list<STerrainInfo::SRiver> &_rivers )
	{
		rivers.resize( _rivers.size() );
		std::vector<SRiverBoxes>::iterator itBoxes = rivers.begin();
		for ( std::list<STerrainInfo::SRiver>::const_iterator it = _rivers.begin(); it != _rivers.end(); ++it, ++itBoxes )
		{
			itBoxes->pRiver = &*it;
			MakeRidgeQuadBoxes( it->ridgeL, &itBoxes->left );
			MakeRidgeQuadBoxes( it->ridgeR, &itBoxes->right );
			MakeRidgeQuadBoxes( it->samples, &itBoxes->bed );
		}
	}

	// What CTerraGen::GetMaxRiverHeight returned, in the same order of operations.
	float Get( const CVec2 &v ) const
	{
		if ( rivers.empty() )
		{
			return 0.0f;
		}

		bool bFlag = false;
		float fHeight = -FP_MAX_VALUE;
		float fH;
		SRiverProfile ridgeProfile;
		SRiverBottomProfile bottomProfile;
		for ( std::vector<SRiverBoxes>::const_iterator it = rivers.begin(); it != rivers.end(); ++it )
		{
			const STerrainInfo::SRiver &river = *it->pRiver;
			if ( IsInsideBB( v, river.vBBMin, river.vBBMax ) )
			{
				if ( GetHeight( v, river.ridgeL, it->left, &fH, ridgeProfile ) )
				{
					fHeight = (std::max)( fHeight, fH );
					bFlag = true;
				}
				if ( GetHeight( v, river.ridgeR, it->right, &fH, ridgeProfile ) )
				{
					fHeight = (std::max)( fHeight, fH );
					bFlag = true;
				}
			}
		}

		if ( !bFlag )
		{
			for ( std::vector<SRiverBoxes>::const_iterator it = rivers.begin(); it != rivers.end(); ++it )
			{
				const STerrainInfo::SRiver &river = *it->pRiver;
				if ( IsInsideBB( v, river.vBBMin, river.vBBMax ) )
				{
					if ( GetHeight( v, river.samples, it->bed, &fH, bottomProfile ) )
					{
						fHeight = DEF_RIVER_DEPTH; //fH;
						bFlag = true;
						break;
					}
				}
			}
		}

		return bFlag ? -fHeight : 0.0f;
	}
};
