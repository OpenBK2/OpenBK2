#include "../stdafx.h"

#include "../RiverHeight.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <list>
#include <random>
#include <string>
#include <vector>

#include <gtest/gtest.h>

// GetMaxRiverHeight decides how deep the rivers cut the terrain, and that height
// reaches the simulation through GetTerraHeight, so any change to how it is computed
// has to give the same bits, not merely close values. These tests hold the production
// code to a frozen copy of the original algorithm, value for value, over rivers built
// the way CTerraGen::AddRiver builds them and over strips built to hit the corner
// cases, and count which paths of that copy the cases reach so that a path no case
// exercises fails the suite instead of passing unnoticed.

namespace
{
using SRiver = STerrainInfo::SRiver;
using TRivers = std::list<SRiver>;

// The code under test: the boxes are built once per case, as UpdateHeightsAfterRivers
// builds them once per area.
float ProductionHeight( const CRiverHeights &riverHeights, const CVec2 &v )
{
	return riverHeights.Get( v );
}

// The original algorithm, copied from CTerraGen::GetMaxRiverHeight, TerraHeight.h and
// TerrUtils.h on 2026-10-28 and not to be changed: it is what the production code is
// compared against. The only additions are the path counters, which do not take part
// in the arithmetic.
namespace NReference
{
	enum EPath
	{
		PATH_NO_RIVERS,
		PATH_OUTSIDE_EVERY_BOX,
		PATH_NO_HIT_INSIDE_BOX,
		PATH_BANK_HIT_LEFT_ONLY,
		PATH_BANK_HIT_RIGHT_ONLY,
		PATH_BANK_HIT_BOTH,
		PATH_BANK_HIT_SEVERAL_RIVERS,
		PATH_BED_HIT,
		PATH_BED_HIT_STOPS_EARLY,
		PATH_RESULT_NEGATIVE_ZERO,
		PATH_STRIP_TOO_SHORT,
		PATH_TRIANGLE_DEGENERATE,
		PATH_TRIANGLE_CLOCKWISE,
		PATH_TRIANGLE_COUNTERCLOCKWISE,
		PATH_TRIANGLE_MISSED,
		PATH_FIRST_TRIANGLE_HIT,
		PATH_SECOND_TRIANGLE_HIT,
		PATH_HIT_IN_EPS_SLACK,
		PATH_HIT_NEGATIVE_DROPPED,
		PATH_HIT_AVERAGED,
		PATH_PROFILE_FLAT,
		PATH_PROFILE_FALLOFF,
		PATH_COUNT
	};

	const char *const pathNames[PATH_COUNT] = {
		"no rivers",
		"outside every river's box",
		"inside a box, no bank or bed hit",
		"left bank hit only",
		"right bank hit only",
		"both banks hit",
		"banks of several rivers hit",
		"bed hit",
		"bed hit stops before a later river",
		"result is negative zero",
		"strip too short to have a quad",
		"degenerate triangle",
		"clockwise triangle",
		"counterclockwise triangle",
		"triangle missed",
		"first triangle of a quad hit",
		"second triangle of a quad hit",
		"hit only thanks to the eps slack",
		"hit dropped for a negative height",
		"hits averaged",
		"bank profile flat part",
		"bank profile falloff",
	};

	int64_t paths[PATH_COUNT] = {};

	void Count( const EPath ePath )
	{
		++paths[ePath];
	}

	void GetBaryCoords( const float px,
	                    const float py,
	                    const float p0x,
	                    const float p0y,
	                    const float p1x,
	                    const float p1y,
	                    const float p2x,
	                    const float p2y,
	                    CVec2 *pBary )
	{
		float d = ( p1x - p0x ) * ( p2y - p0y ) - ( p1y - p0y ) * ( p2x - p0x );
		if ( fabs( d ) > DEF_EPS )
		{
			Count( d < 0.0f ? PATH_TRIANGLE_CLOCKWISE : PATH_TRIANGLE_COUNTERCLOCKWISE );
			d = 1.0 / d;
			pBary->x = ( ( px - p0x ) * ( p2y - p0y ) - ( py - p0y ) * ( p2x - p0x ) ) * d;
			pBary->y = ( ( py - p0y ) * ( p1x - p0x ) - ( px - p0x ) * ( p1y - p0y ) ) * d;
		}
		else
		{
			Count( PATH_TRIANGLE_DEGENERATE );
			pBary->x = -1.0f;
			pBary->y = -1.0f;
		}
	}

	// bInv selects GetHeightInv, which feeds the profile 1 - x instead of x.
	template <class THeightCalc>
	float GetHeight( const CVec2 &p,
	                 const CVec3fEx &p0,
	                 const CVec3fEx &p1,
	                 const CVec3fEx &p2,
	                 const THeightCalc &heightCalc,
	                 const bool bInv,
	                 const bool bBank )
	{
		CVec2 vBary;
		NReference::GetBaryCoords( p.x, p.y, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, &vBary );
		if ( ( vBary.x > -DEF_EPS ) && ( vBary.y > -DEF_EPS ) && ( ( vBary.x + vBary.y ) < ( 1.0f + DEF_EPS ) ) )
		{
			if ( vBary.x < 0.0f || vBary.y < 0.0f || ( vBary.x + vBary.y ) > 1.0f )
			{
				Count( PATH_HIT_IN_EPS_SLACK );
			}
			const float fProfileX = bInv ? 1.0f - vBary.x : vBary.x;
			if ( bBank )
			{
				Count( fProfileX <= DEF_RIVER_RIDGE_NULL ? PATH_PROFILE_FLAT : PATH_PROFILE_FALLOFF );
			}
			return ( ( p1.z - p0.z ) * vBary.x + ( p2.z - p0.z ) * vBary.y + p0.z ) * heightCalc( fProfileX );
		}
		else
		{
			Count( PATH_TRIANGLE_MISSED );
			return -1.0f;
		}
	}

	template <class THeightCalc>
	bool GetIncRidgeHeight( const CVec2 &v,
	                        const std::vector<CVec3fEx> &ridge,
	                        float *pIncHeight,
	                        const THeightCalc &heightCalc,
	                        const bool bBank )
	{
		*pIncHeight = 0.0f;
		int nCount = 0;
		const int nHalfSize = ( ridge.size() >> 1 ) - 1;
		if ( nHalfSize <= 0 )
		{
			Count( PATH_STRIP_TOO_SHORT );
		}
		for ( int k = 0; k < nHalfSize; ++k )
		{
			const float fH1 =
					NReference::GetHeight( v, ridge[k + 1], ridge[ridge.size() - k - 2], ridge[k], heightCalc, false, bBank );
			if ( fH1 >= 0.0f )
			{
				Count( PATH_FIRST_TRIANGLE_HIT );
				*pIncHeight += fH1;
				++nCount;
			}
			else if ( fH1 != -1.0f )
			{
				Count( PATH_HIT_NEGATIVE_DROPPED );
			}
			const float fH2 = NReference::GetHeight(
					v, ridge[ridge.size() - k - 1], ridge[k], ridge[ridge.size() - k - 2], heightCalc, true, bBank );
			if ( fH2 >= 0.0f )
			{
				Count( PATH_SECOND_TRIANGLE_HIT );
				*pIncHeight += fH2;
				++nCount;
			}
			else if ( fH2 != -1.0f )
			{
				Count( PATH_HIT_NEGATIVE_DROPPED );
			}
		}
		if ( nCount > 0 )
		{
			if ( nCount > 1 )
			{
				Count( PATH_HIT_AVERAGED );
			}
			*pIncHeight /= nCount;
			return true;
		}
		return false;
	}

	float GetMaxRiverHeight( const CVec2 &v, const TRivers &rivers )
	{
		if ( rivers.empty() )
		{
			Count( PATH_NO_RIVERS );
			return 0.0f;
		}

		bool bFlag = false;
		bool bInsideAnyBox = false;
		int nBankRivers = 0;
		float fHeight = -FP_MAX_VALUE;
		float fH;
		SRiverProfile ridgeProfile;
		SRiverBottomProfile bottomProfile;
		for ( TRivers::const_iterator it = rivers.begin(); it != rivers.end(); ++it )
		{
			if ( IsInsideBB( v, it->vBBMin, it->vBBMax ) )
			{
				bInsideAnyBox = true;
				bool bLeft = false;
				bool bRight = false;
				if ( NReference::GetIncRidgeHeight( v, it->ridgeL, &fH, ridgeProfile, true ) )
				{
					fHeight = (std::max)( fHeight, fH );
					bFlag = true;
					bLeft = true;
				}
				if ( NReference::GetIncRidgeHeight( v, it->ridgeR, &fH, ridgeProfile, true ) )
				{
					fHeight = (std::max)( fHeight, fH );
					bFlag = true;
					bRight = true;
				}
				if ( bLeft || bRight )
				{
					++nBankRivers;
					Count( bLeft && bRight ? PATH_BANK_HIT_BOTH
					                       : ( bLeft ? PATH_BANK_HIT_LEFT_ONLY : PATH_BANK_HIT_RIGHT_ONLY ) );
				}
			}
		}
		if ( nBankRivers > 1 )
		{
			Count( PATH_BANK_HIT_SEVERAL_RIVERS );
		}

		if ( !bFlag )
		{
			for ( TRivers::const_iterator it = rivers.begin(); it != rivers.end(); ++it )
			{
				if ( IsInsideBB( v, it->vBBMin, it->vBBMax ) )
				{
					if ( NReference::GetIncRidgeHeight( v, it->samples, &fH, bottomProfile, false ) )
					{
						Count( PATH_BED_HIT );
						for ( TRivers::const_iterator itLater = std::next( it ); itLater != rivers.end(); ++itLater )
						{
							if ( IsInsideBB( v, itLater->vBBMin, itLater->vBBMax ) )
							{
								Count( PATH_BED_HIT_STOPS_EARLY );
								break;
							}
						}
						fHeight = DEF_RIVER_DEPTH; //fH;
						bFlag = true;
						break;
					}
				}
			}
		}

		if ( !bFlag )
		{
			Count( bInsideAnyBox ? PATH_NO_HIT_INSIDE_BOX : PATH_OUTSIDE_EVERY_BOX );
		}
		const float fResult = bFlag ? -fHeight : 0.0f;
		if ( fResult == 0.0f && std::signbit( fResult ) )
		{
			Count( PATH_RESULT_NEGATIVE_ZERO );
		}
		return fResult;
	}
} // namespace NReference

// One point of a river's centre line, as CTerraGen::AddRiver sees it after converting
// to visual units: a position, the unit normal across the river, the half width of the
// water and the depth of each bank as a fraction of DEF_RIVER_DEPTH.
struct SCentrePoint
{
	CVec2 vPos;
	CVec2 vNorm;
	float fWidth;
	float fDepthL;
	float fDepthR;
};

void AddToBox( SRiver *pRiver, const CVec3fEx &v )
{
	pRiver->vBBMin.Minimize( CVec2( v.x, v.y ) );
	pRiver->vBBMax.Maximize( CVec2( v.x, v.y ) );
}

// The banks, the bed and the box exactly as CTerraGen::AddRiver lays them out.
SRiver MakeRiver( const std::vector<SCentrePoint> &points, const int nID )
{
	SRiver river;
	river.nID = nID;
	river.nRandSeed = 0;
	river.nWaterSamplesNum = 0;
	river.nWater2SamplesNum = 0;
	river.vBBMin.Set( FP_MAX_VALUE, FP_MAX_VALUE );
	river.vBBMax.Set( -FP_MAX_VALUE, -FP_MAX_VALUE );
	river.ridgeL.resize( points.size() * 2 );
	river.ridgeR.resize( points.size() * 2 );
	river.samples.resize( points.size() * 2 );
	for ( int nInd = 0; nInd < int( points.size() ); ++nInd )
	{
		const SCentrePoint &point = points[nInd];
		const int nInvInd = int( river.ridgeL.size() ) - 1 - nInd;
		const float fRidgeHeightL = Clamp( point.fDepthL, 0.0f, 1.0f ) * DEF_RIVER_DEPTH;
		const float fRidgeHeightR = Clamp( point.fDepthR, 0.0f, 1.0f ) * DEF_RIVER_DEPTH;
		const CVec3 vPos( point.vPos.x, point.vPos.y, 0.0f );
		const CVec3 vNorm( point.vNorm.x, point.vNorm.y, 0.0f );
		river.ridgeL[nInd].Set( vPos - vNorm * point.fWidth, 0 );
		river.ridgeL[nInd].z = fRidgeHeightL;
		AddToBox( &river, river.ridgeL[nInd] );
		river.ridgeL[nInvInd].Set( vPos - vNorm * ( point.fWidth + DEF_RIVER_RIDGE_WIDTH ), 0 );
		river.ridgeL[nInvInd].z = fRidgeHeightL;
		AddToBox( &river, river.ridgeL[nInvInd] );
		river.ridgeR[nInd].Set( vPos + vNorm * point.fWidth, 0 );
		river.ridgeR[nInd].z = fRidgeHeightR;
		AddToBox( &river, river.ridgeR[nInd] );
		river.ridgeR[nInvInd].Set( vPos + vNorm * ( point.fWidth + DEF_RIVER_RIDGE_WIDTH ), 0 );
		river.ridgeR[nInvInd].z = fRidgeHeightR;
		AddToBox( &river, river.ridgeR[nInvInd] );
	}
	for ( int nInd = 0; nInd < int( points.size() ); ++nInd )
	{
		river.samples[nInd].Set( river.ridgeL[nInd].x, river.ridgeL[nInd].y, river.ridgeL[nInd].z, 0 );
		river.samples[river.samples.size() - 1 - nInd].Set(
				river.ridgeR[nInd].x, river.ridgeR[nInd].y, river.ridgeR[nInd].z, 0 );
		AddToBox( &river, river.samples[nInd] );
		AddToBox( &river, river.samples[river.samples.size() - 1 - nInd] );
	}
	return river;
}

// A centre line through the given positions, each normal the average of the normals of
// the segments meeting there, as the editor's spline sampling produces.
std::vector<SCentrePoint>
CentreLine( const std::vector<CVec2> &positions, const float fWidth, const float fDepthL, const float fDepthR )
{
	std::vector<SCentrePoint> points( positions.size() );
	for ( int i = 0; i < int( positions.size() ); ++i )
	{
		CVec2 vNorm( 0.0f, 0.0f );
		if ( i > 0 )
		{
			const CVec2 vDir = positions[i] - positions[i - 1];
			vNorm += CVec2( -vDir.y, vDir.x );
		}
		if ( i + 1 < int( positions.size() ) )
		{
			const CVec2 vDir = positions[i + 1] - positions[i];
			vNorm += CVec2( -vDir.y, vDir.x );
		}
		const float fLength = fabs( vNorm );
		points[i].vPos = positions[i];
		points[i].vNorm = fLength > 0.0f ? vNorm * ( 1.0f / fLength ) : CVec2( 0.0f, 1.0f );
		points[i].fWidth = fWidth;
		points[i].fDepthL = fDepthL;
		points[i].fDepthR = fDepthR;
	}
	return points;
}

// A case: some rivers and the points to ask about.
struct SScenario
{
	std::string szName;
	TRivers rivers;
	std::vector<CVec2> points;
};

// Every vertex of the tile grid that covers the rivers' boxes, plus a margin of tiles
// outside them: the points UpdateHeightsAfterRivers asks about.
void AddTileGrid( SScenario *pScenario, const int nMarginTiles )
{
	CVec2 vMin( FP_MAX_VALUE, FP_MAX_VALUE ), vMax( -FP_MAX_VALUE, -FP_MAX_VALUE );
	for ( TRivers::const_iterator it = pScenario->rivers.begin(); it != pScenario->rivers.end(); ++it )
	{
		vMin.Minimize( it->vBBMin );
		vMax.Maximize( it->vBBMax );
	}
	if ( pScenario->rivers.empty() )
	{
		vMin.Set( 0.0f, 0.0f );
		vMax.Set( 8 * DEF_TILE_SIZE, 8 * DEF_TILE_SIZE );
	}
	const int nX1 = int( floorf( vMin.x / DEF_TILE_SIZE ) ) - nMarginTiles;
	const int nY1 = int( floorf( vMin.y / DEF_TILE_SIZE ) ) - nMarginTiles;
	const int nX2 = int( ceilf( vMax.x / DEF_TILE_SIZE ) ) + nMarginTiles;
	const int nY2 = int( ceilf( vMax.y / DEF_TILE_SIZE ) ) + nMarginTiles;
	for ( int g = nY1; g <= nY2; ++g )
	{
		for ( int i = nX1; i <= nX2; ++i )
		{
			pScenario->points.push_back( CVec2( (float)i * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE ) );
		}
	}
}

void AddStripPoints( std::vector<CVec2> *pPoints, const std::vector<CVec3fEx> &strip )
{
	// Offsets across an edge, as fractions of its length: on it, inside and outside the
	// eps slack of the barycentric test, and clear of it.
	static const float offsets[] = { 0.0f, 1e-6f, -1e-6f, 5e-5f, -5e-5f, 2e-4f, -2e-4f, 1e-2f, -1e-2f };
	static const float alongs[] = { 0.0f, 0.25f, 0.5f, 1.0f };
	for ( int i = 0; i < int( strip.size() ); ++i )
	{
		const CVec2 vA( strip[i].x, strip[i].y );
		pPoints->push_back( vA );
		pPoints->push_back( CVec2( std::nextafter( vA.x, FP_MAX_VALUE ), vA.y ) );
		pPoints->push_back( CVec2( vA.x, std::nextafter( vA.y, -FP_MAX_VALUE ) ) );
		for ( int j = i + 1; j < int( strip.size() ) && j <= i + 2; ++j )
		{
			const CVec2 vB( strip[j].x, strip[j].y );
			const CVec2 vEdge = vB - vA;
			const CVec2 vAcross( -vEdge.y, vEdge.x );
			for ( const float fAlong : alongs )
			{
				for ( const float fOffset : offsets )
				{
					pPoints->push_back( vA + vEdge * fAlong + vAcross * fOffset );
				}
			}
		}
		// The quad's diagonals, which are shared edges between its two triangles.
		const int nOpposite = int( strip.size() ) - 1 - i;
		if ( nOpposite != i )
		{
			const CVec2 vB( strip[nOpposite].x, strip[nOpposite].y );
			for ( const float fAlong : alongs )
			{
				pPoints->push_back( vA + ( vB - vA ) * fAlong );
			}
		}
	}
}

// The vertices of every bank and bed, and points on and around each of their edges.
void AddEdgePoints( SScenario *pScenario )
{
	for ( TRivers::const_iterator it = pScenario->rivers.begin(); it != pScenario->rivers.end(); ++it )
	{
		AddStripPoints( &pScenario->points, it->ridgeL );
		AddStripPoints( &pScenario->points, it->ridgeR );
		AddStripPoints( &pScenario->points, it->samples );
	}
}

void AddRandomPoints( SScenario *pScenario, std::mt19937 *pRandom, const int nCount )
{
	for ( TRivers::const_iterator it = pScenario->rivers.begin(); it != pScenario->rivers.end(); ++it )
	{
		std::uniform_real_distribution<float> x( it->vBBMin.x, it->vBBMax.x );
		std::uniform_real_distribution<float> y( it->vBBMin.y, it->vBBMax.y );
		for ( int i = 0; i < nCount; ++i )
		{
			pScenario->points.push_back( CVec2( x( *pRandom ), y( *pRandom ) ) );
		}
	}
}

void AddAllPoints( SScenario *pScenario, std::mt19937 *pRandom )
{
	AddTileGrid( pScenario, 3 );
	AddEdgePoints( pScenario );
	AddRandomPoints( pScenario, pRandom, 2000 );
}

std::vector<CVec2> Line( const CVec2 &vStart, const CVec2 &vStep, const int nCount )
{
	std::vector<CVec2> positions;
	for ( int i = 0; i < nCount; ++i )
	{
		positions.push_back( vStart + vStep * float( i ) );
	}
	return positions;
}

std::vector<CVec2>
Arc( const CVec2 &vCentre, const float fRadius, const float fFrom, const float fTo, const int nCount )
{
	std::vector<CVec2> positions;
	for ( int i = 0; i < nCount; ++i )
	{
		const float fAngle = fFrom + ( fTo - fFrom ) * float( i ) / float( nCount - 1 );
		positions.push_back( vCentre + CVec2( cosf( fAngle ), sinf( fAngle ) ) * fRadius );
	}
	return positions;
}

// A meandering centre line: a random walk whose heading drifts.
std::vector<CVec2> RandomWalk( std::mt19937 *pRandom, const CVec2 &vStart, const int nCount, const float fStep )
{
	std::uniform_real_distribution<float> turn( -0.6f, 0.6f );
	std::uniform_real_distribution<float> stretch( 0.3f, 1.7f );
	std::vector<CVec2> positions;
	CVec2 vPos = vStart;
	float fHeading = turn( *pRandom ) * 5.0f;
	for ( int i = 0; i < nCount; ++i )
	{
		positions.push_back( vPos );
		fHeading += turn( *pRandom );
		vPos += CVec2( cosf( fHeading ), sinf( fHeading ) ) * ( fStep * stretch( *pRandom ) );
	}
	return positions;
}

// A strip from explicit corners, for the cases a centre line cannot produce. pointsA
// runs along one edge and pointsB along the other, both in the same direction.
std::vector<CVec3fEx> Strip( const std::vector<CVec3> &pointsA, const std::vector<CVec3> &pointsB )
{
	std::vector<CVec3fEx> strip( pointsA.size() + pointsB.size() );
	for ( int i = 0; i < int( pointsA.size() ); ++i )
	{
		strip[i].Set( pointsA[i], 0 );
	}
	for ( int i = 0; i < int( pointsB.size() ); ++i )
	{
		strip[strip.size() - 1 - i].Set( pointsB[i], 0 );
	}
	return strip;
}

// A river whose banks and bed are given directly, its box around all of them.
SRiver RawRiver( const std::vector<CVec3fEx> &ridgeL,
                 const std::vector<CVec3fEx> &ridgeR,
                 const std::vector<CVec3fEx> &samples )
{
	SRiver river = MakeRiver( {}, 0 );
	river.ridgeL = ridgeL;
	river.ridgeR = ridgeR;
	river.samples = samples;
	for ( const std::vector<CVec3fEx> *pStrip : { &ridgeL, &ridgeR, &samples } )
	{
		for ( const CVec3fEx &v : *pStrip )
		{
			AddToBox( &river, v );
		}
	}
	return river;
}

std::vector<SScenario> MakeScenarios()
{
	std::mt19937 random( 20261028 );
	std::vector<SScenario> scenarios;
	const float fTile = DEF_TILE_SIZE;
	auto add = [&]( const std::string &szName, TRivers rivers )
	{
		scenarios.push_back( SScenario() );
		SScenario &scenario = scenarios.back();
		scenario.szName = szName;
		scenario.rivers = std::move( rivers );
		AddAllPoints( &scenario, &random );
	};

	add( "no rivers", {} );
	add( "straight",
	     { MakeRiver( CentreLine( Line( CVec2( 40, 40 ), CVec2( fTile, 0.3f ), 30 ), 3.0f, 1.0f, 1.0f ), 1 ) } );
	add( "diagonal, uneven banks",
	     { MakeRiver( CentreLine( Line( CVec2( 100, 20 ), CVec2( 2.1f, 1.7f ), 25 ), 5.0f, 0.25f, 0.8f ), 1 ) } );
	add( "banks of no depth",
	     { MakeRiver( CentreLine( Line( CVec2( 30, 30 ), CVec2( fTile, fTile ), 12 ), 3.0f, 0.0f, 0.0f ), 1 ) } );
	add( "gentle bend",
	     { MakeRiver( CentreLine( Arc( CVec2( 200, 200 ), 60.0f, 0.0f, 1.5f, 30 ), 4.0f, 1.0f, 0.5f ), 1 ) } );
	// The inner banks of a tight bend fold over themselves, so quads overlap and a point
	// can fall in several of them at once.
	add( "hairpin",
	     { MakeRiver( CentreLine( Arc( CVec2( 300, 100 ), 6.0f, 0.0f, 3.1416f, 14 ), 3.0f, 1.0f, 1.0f ), 1 ) } );
	add( "self crossing loop",
	     { MakeRiver( CentreLine( Arc( CVec2( 150, 150 ), 25.0f, 0.0f, 7.5f, 40 ), 3.0f, 0.6f, 1.0f ), 1 ) } );
	{
		std::vector<CVec2> positions = Line( CVec2( 50, 80 ), CVec2( fTile, 0.0f ), 10 );
		positions.insert( positions.begin() + 5, positions[5] );
		positions.insert( positions.begin() + 5, positions[5] );
		add( "repeated points", { MakeRiver( CentreLine( positions, 3.0f, 1.0f, 1.0f ), 1 ) } );
	}
	add( "no water width",
	     { MakeRiver( CentreLine( Line( CVec2( 60, 60 ), CVec2( fTile, 0.5f ), 10 ), 0.0f, 1.0f, 1.0f ), 1 ) } );
	// Segments short enough that a quad's area is near DEF_EPS, on either side of it.
	add( "tiny segments",
	     { MakeRiver( CentreLine( Line( CVec2( 20, 20 ), CVec2( 4e-6f, 0.0f ), 12 ), 3.0f, 1.0f, 1.0f ), 1 ),
	       MakeRiver( CentreLine( Line( CVec2( 20, 30 ), CVec2( 1.2e-5f, 0.0f ), 12 ), 3.0f, 1.0f, 1.0f ), 2 ),
	       MakeRiver( CentreLine( Line( CVec2( 20, 40 ), CVec2( 4e-5f, 0.0f ), 12 ), 3.0f, 1.0f, 1.0f ), 3 ) } );
	// Far from the origin a float step is about 0.004, so short segments round onto
	// nearly collinear points and the barycentric arithmetic loses most of its bits.
	add( "far from the origin",
	     { MakeRiver( CentreLine( RandomWalk( &random, CVec2( 44000, 43000 ), 40, fTile ), 3.0f, 1.0f, 0.7f ), 1 ),
	       MakeRiver( CentreLine( Line( CVec2( 44100, 43100 ), CVec2( 0.01f, 0.003f ), 20 ), 3.0f, 1.0f, 1.0f ), 2 ) } );
	add( "two rivers crossing",
	     { MakeRiver( CentreLine( Line( CVec2( 100, 300 ), CVec2( fTile, 0.0f ), 30 ), 4.0f, 1.0f, 0.5f ), 1 ),
	       MakeRiver( CentreLine( Line( CVec2( 140, 260 ), CVec2( 0.0f, fTile ), 30 ), 3.0f, 0.3f, 1.0f ), 2 ) } );
	add( "two rivers, same bed",
	     { MakeRiver( CentreLine( Line( CVec2( 10, 10 ), CVec2( fTile, 0.0f ), 20 ), 6.0f, 1.0f, 1.0f ), 1 ),
	       MakeRiver( CentreLine( Line( CVec2( 10, 10 ), CVec2( fTile, 0.0f ), 20 ), 3.0f, 0.5f, 0.5f ), 2 ) } );
	{
		// Strips with no quad at all: sizes 0 to 3.
		std::vector<CVec3fEx> strips[4];
		for ( int n = 1; n < 4; ++n )
		{
			strips[n] = strips[n - 1];
			strips[n].push_back( CVec3fEx( 10.0f * n, 5.0f * n, 1.0f, 0 ) );
		}
		add( "strips too short",
		     { RawRiver( strips[0], strips[1], strips[2] ), RawRiver( strips[3], strips[2], strips[3] ) } );
	}
	{
		// An odd sized strip: the middle point belongs to neither half.
		std::vector<CVec3fEx> odd = Strip( { CVec3( 0, 0, 2 ), CVec3( 5, 0, 2 ), CVec3( 10, 0, 2 ) },
		                                   { CVec3( 0, 8, 0 ), CVec3( 5, 8, 0 ), CVec3( 10, 8, 0 ) } );
		odd.insert( odd.begin() + 3, CVec3fEx( 20, 4, 1, 0 ) );
		add( "odd sized strip", { RawRiver( odd, odd, odd ) } );
	}
	{
		// Collinear corners, and corners at the same place: the degenerate branch.
		const std::vector<CVec3fEx> collinear = Strip( { CVec3( 0, 0, 1 ), CVec3( 5, 0, 1 ), CVec3( 10, 0, 1 ) },
		                                               { CVec3( 2, 0, 0 ), CVec3( 7, 0, 0 ), CVec3( 12, 0, 0 ) } );
		const std::vector<CVec3fEx> pinched = Strip( { CVec3( 0, 0, 1 ), CVec3( 5, 0, 1 ), CVec3( 10, 0, 1 ) },
		                                             { CVec3( 0, 0, 0 ), CVec3( 5, 6, 0 ), CVec3( 10, 0, 0 ) } );
		add( "degenerate quads", { RawRiver( collinear, pinched, collinear ) } );
	}
	{
		// Corner heights below zero inside a triangle: the dropped hit path.
		const std::vector<CVec3fEx> sunken =
				Strip( { CVec3( 0, 0, -0.75f ), CVec3( 6, 0, -0.75f ) }, { CVec3( 0, 6, -0.75f ), CVec3( 6, 6, -0.75f ) } );
		add( "negative corner heights", { RawRiver( sunken, sunken, sunken ) } );
	}
	{
		// Thin quads far from the origin, where a float step is about 0.004: their area
		// is above DEF_EPS but made of few bits, so the barycentric arithmetic rounds
		// badly, and the points along the lines of their long edges, well beyond the
		// quad, are where that rounding can let a point pass that is nowhere near it.
		TRivers rivers;
		std::vector<CVec2> points;
		const float fX = 44000.0f, fY = 43000.0f;
		const float fStep = std::nextafter( fY, FP_MAX_VALUE ) - fY;
		for ( int nRise = 1; nRise <= 4; ++nRise )
		{
			const float fY0 = fY + 200.0f * nRise;
			const std::vector<CVec3fEx> thin = Strip(
					{ CVec3( fX, fY0, 1 ), CVec3( fX + 10.0f, fY0 + fStep * nRise, 1 ) },
					{ CVec3( fX + 0.5f, fY0 + fStep * ( nRise + 1 ), 0 ), CVec3( fX + 10.5f, fY0 + fStep * 2 * nRise, 0 ) } );
			rivers.push_back( RawRiver( thin, thin, thin ) );
			for ( int i = 0; i < 4; ++i )
			{
				const CVec2 vA( thin[i].x, thin[i].y );
				for ( int j = 0; j < 4; ++j )
				{
					if ( i == j )
					{
						continue;
					}
					const CVec2 vDir = CVec2( thin[j].x, thin[j].y ) - vA;
					for ( const float fFar : { -40.0f, -8.0f, -2.0f, -1.1f, 1.1f, 2.0f, 8.0f, 40.0f } )
					{
						const CVec2 vOnLine = vA + vDir * fFar;
						for ( int nAcross = -6; nAcross <= 6; ++nAcross )
						{
							points.push_back( CVec2( vOnLine.x, vOnLine.y + fStep * nAcross ) );
						}
					}
				}
			}
		}
		add( "thin quads far from the origin", std::move( rivers ) );
		// The box of each river spans its whole quad, so these points are inside it and
		// reach the quad's triangles.
		for ( TRivers::iterator it = scenarios.back().rivers.begin(); it != scenarios.back().rivers.end(); ++it )
		{
			it->vBBMin.Set( -FP_MAX_VALUE, -FP_MAX_VALUE );
			it->vBBMax.Set( FP_MAX_VALUE, FP_MAX_VALUE );
		}
		scenarios.back().points.insert( scenarios.back().points.end(), points.begin(), points.end() );
	}
	for ( int n = 0; n < 40; ++n )
	{
		std::uniform_real_distribution<float> position( 0.0f, 3000.0f );
		std::uniform_real_distribution<float> width( 0.0f, 12.0f );
		std::uniform_real_distribution<float> depth( -0.2f, 1.2f );
		std::uniform_int_distribution<int> count( 2, 50 );
		const int nRivers = 1 + n % 3;
		TRivers rivers;
		for ( int r = 0; r < nRivers; ++r )
		{
			const CVec2 vStart( position( random ), position( random ) );
			rivers.push_back( MakeRiver( CentreLine( RandomWalk( &random, vStart, count( random ), fTile * 1.5f ),
			                                         width( random ),
			                                         depth( random ),
			                                         depth( random ) ),
			                             r + 1 ) );
		}
		add( "random " + std::to_string( n ), std::move( rivers ) );
	}
	return scenarios;
}

const std::vector<SScenario> &Scenarios()
{
	static const std::vector<SScenario> scenarios = MakeScenarios();
	return scenarios;
}

uint32_t Bits( const float f )
{
	uint32_t n;
	memcpy( &n, &f, sizeof( n ) );
	return n;
}

TEST( RiverHeight, MatchesTheOriginalBitForBit )
{
	int64_t nPoints = 0;
	for ( const SScenario &scenario : Scenarios() )
	{
		int nMismatches = 0;
		const CRiverHeights riverHeights( scenario.rivers );
		for ( const CVec2 &v : scenario.points )
		{
			const float fExpected = NReference::GetMaxRiverHeight( v, scenario.rivers );
			const float fActual = ProductionHeight( riverHeights, v );
			++nPoints;
			if ( Bits( fExpected ) != Bits( fActual ) && ++nMismatches <= 5 )
			{
				ADD_FAILURE() << scenario.szName << ": at (" << v.x << ", " << v.y << ") expected " << fExpected << " (0x"
											<< std::hex << Bits( fExpected ) << "), got " << fActual << " (0x" << Bits( fActual ) << ")"
											<< std::dec;
			}
		}
		EXPECT_EQ( nMismatches, 0 ) << scenario.szName;
	}
	EXPECT_GT( nPoints, 100000 );
}

// Every path of the original is reached by some case, so a change that breaks one of
// them shows up as a mismatch above rather than passing because nothing went there.
TEST( RiverHeight, CasesReachEveryPath )
{
	memset( NReference::paths, 0, sizeof( NReference::paths ) );
	for ( const SScenario &scenario : Scenarios() )
	{
		for ( const CVec2 &v : scenario.points )
		{
			NReference::GetMaxRiverHeight( v, scenario.rivers );
		}
	}
	for ( int i = 0; i < NReference::PATH_COUNT; ++i )
	{
		EXPECT_GT( NReference::paths[i], 0 ) << "no case reaches: " << NReference::pathNames[i];
	}
}
} // namespace
