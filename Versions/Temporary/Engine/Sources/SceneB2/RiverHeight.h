#pragma once

// How a river lowers the terrain around it. Each river has two banks, ridgeL and
// ridgeR, built as ridges (see RidgeHeight.h) from the river's edge out to
// DEF_RIVER_RIDGE_WIDTH beyond it, and its bed, samples, between the edges. Moved
// here unchanged from CTerraGen::GetMaxRiverHeight and RiversBuilder.cpp so that it
// can be tested without a whole CTerraGen.

#include "RidgeHeight.h"
#include "TerrainInfo.h"

#include <algorithm>
#include <list>

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

// The depth the rivers give the terrain at v, as a value to add to its height: the
// deepest of the banks that contain v, or DEF_RIVER_DEPTH in a river's bed, or 0.
inline float GetMaxRiverHeight( const CVec2 &v, const std::list<STerrainInfo::SRiver> &rivers )
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
	for ( std::list<STerrainInfo::SRiver>::const_iterator it = rivers.begin(); it != rivers.end(); ++it )
	{
		if ( IsInsideBB( v, it->vBBMin, it->vBBMax ) )
		{
			if ( GetIncRidgeHeight( v, it->ridgeL, &fH, ridgeProfile ) )
			{
				fHeight = (std::max)( fHeight, fH );
				bFlag = true;
			}
			if ( GetIncRidgeHeight( v, it->ridgeR, &fH, ridgeProfile ) )
			{
				fHeight = (std::max)( fHeight, fH );
				bFlag = true;
			}
		}
	}

	if ( !bFlag )
	{
		for ( std::list<STerrainInfo::SRiver>::const_iterator it = rivers.begin(); it != rivers.end(); ++it )
		{
			if ( IsInsideBB( v, it->vBBMin, it->vBBMax ) )
			{
				if ( GetIncRidgeHeight( v, it->samples, &fH, bottomProfile ) )
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
