#include "../stdafx.h"

#include "../BoundedSampling.h"
#include "../ExecutorSeaReinf.h"
#include "../LongObjectCreation.h"
#include "B2_M1_World/SmokeTrailEffect.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/Terrain.h"
#include "Common_RTS_AI/Tools.h"

#include <limits>
#include <list>
#include <vector>

#include <gtest/gtest.h>

namespace
{
class CConstructionSampling : public CLongObjectCreation
{
public:
	using CLongObjectCreation::SplitLineToSegrments;
};

struct STrailSnapshot
{
	float fInterval = 0.0f;
	float fTime = 0.0f;
	CVec3 vPosition = VNULL3;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 3, &fInterval );
		saver.Add( 5, &fTime );
		saver.Add( 6, &vPosition );
		return 0;
	}
};

STrailSnapshot Snapshot( CSmokeTrailEffect *pEffect )
{
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->AddPolymorphicBase( 1, pEffect );
	}
	stream.Seek( 0 );
	STrailSnapshot result;
	CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
	loader->Add( 1, &result );
	return result;
}
} // namespace

TEST( EffectLoopProgress, FlameSamplingPreservesSpacingAndExcludesEndpoint )
{
	NBoundedSampling::CTrace trace( 100.0f, 25.0f );
	ASSERT_TRUE( trace.IsValid() );
	std::vector<float> distances;
	float fDistance;
	while ( trace.GetNext( &fDistance ) )
	{
		distances.push_back( fDistance );
	}
	EXPECT_EQ( distances, ( std::vector<float>{ 0.0f, 25.0f, 50.0f, 75.0f } ) );
}

TEST( EffectLoopProgress, InvalidAndExcessivelyDenseFlamePathsHaveNoSamples )
{
	for ( const float fSpacing : { 0.0f,
	                               -1.0f,
	                               std::numeric_limits<float>::denorm_min(),
	                               std::numeric_limits<float>::infinity(),
	                               std::numeric_limits<float>::quiet_NaN() } )
	{
		NBoundedSampling::CTrace trace( 100.0f, fSpacing );
		float fDistance = -1.0f;
		EXPECT_FALSE( trace.IsValid() );
		EXPECT_FALSE( trace.GetNext( &fDistance ) );
		EXPECT_EQ( fDistance, -1.0f );
	}
	NBoundedSampling::CTrace invalidLength( std::numeric_limits<float>::infinity(), 1.0f );
	EXPECT_FALSE( invalidLength.IsValid() );
}

TEST( EffectLoopProgress, FlameSamplingHasAFixedWorkLimit )
{
	NBoundedSampling::CTrace trace( float( NBoundedSampling::MAX_SAMPLES ), 1.0f );
	int nSamples = 0;
	float fDistance;
	while ( trace.GetNext( &fDistance ) )
	{
		++nSamples;
	}
	EXPECT_EQ( nSamples, NBoundedSampling::MAX_SAMPLES );
	NBoundedSampling::CTrace excessive( float( NBoundedSampling::MAX_SAMPLES + 1 ), 1.0f );
	EXPECT_FALSE( excessive.IsValid() );
}

TEST( EffectLoopProgress, TrenchSamplingPreservesRegularConstructionPoints )
{
	std::vector<CVec2> points;
	CConstructionSampling::SplitLineToSegrments( &points, CVec2( 10, 20 ), CVec2( 110, 20 ), 25 );
	ASSERT_EQ( points.size(), 4 );
	for ( int i = 0; i < 4; ++i )
	{
		EXPECT_EQ( points[i], CVec2( 10 + 25 * i, 20 ) );
	}
}

TEST( EffectLoopProgress, InvalidTrenchWidthsAndRoundedAwayStepsLeaveNoPartialConstruction )
{
	for ( const float fSpacing : { 0.0f,
	                               -1.0f,
	                               std::numeric_limits<float>::denorm_min(),
	                               std::numeric_limits<float>::infinity(),
	                               std::numeric_limits<float>::quiet_NaN() } )
	{
		std::vector<CVec2> points;
		CConstructionSampling::SplitLineToSegrments( &points, CVec2( 10, 20 ), CVec2( 110, 20 ), fSpacing );
		EXPECT_TRUE( points.empty() );
	}
	std::vector<CVec2> points;
	// 0.25 is positive but is below the coordinate's representable step.
	CConstructionSampling::SplitLineToSegrments( &points, CVec2( 16777216, 0 ), CVec2( 16777220, 0 ), 0.25f );
	EXPECT_TRUE( points.empty() );
}

TEST( EffectLoopProgress, ArcSearchDoesNotWrapItsFullTurnCounter )
{
	CObj<CAIMap> map = new CAIMap( 32, 32, 32, 2, 32 );
	int nVisits = 0;
	auto accept = [&]( const SVector & )
	{
		++nVisits;
		return true;
	};
	// At radius 1 the angle step is 8192: a uint16_t counter wraps before reaching 65535.
	EXPECT_EQ( CheckArcTiles( accept, CVec2( 500, 500 ), 0, 65535, 1.0f, true, true, map ), 65535 );
	EXPECT_GT( nVisits, 0 );
	EXPECT_LE( nVisits, 8 );
	EXPECT_EQ(
			CheckArcTiles( accept, CVec2( 500, 500 ), 0, 1000, std::numeric_limits<float>::quiet_NaN(), true, true, map ),
			0 );
	EXPECT_EQ( CheckArcTiles( accept, CVec2( 500, 500 ), 0, 1000, -1.0f, true, true, map ), 0 );
	// Huge radii used to truncate the angular step to zero. The arc must now finish or reject the map boundary.
	EXPECT_LE( CheckArcTiles( accept, CVec2( 500, 500 ), 0, 1000, 100000.0f, true, true, map ), 1000 );
}

TEST( EffectLoopProgress, SmokeTrailsUseTheirOwnIntervalAndBoundCatchup )
{
	SHMatrix identity;
	Identity( &identity );
	const CQuat rotation( 0, 0, 0, 1 );
	CObj<CSmokeTrailEffect> shortTrail = new CSmokeTrailEffect( identity, 2, nullptr, VNULL3, rotation, 0, false );
	shortTrail->UpdatePlacement( CVec3( 3, 0, 0 ), rotation, 100, false );
	CObj<CSmokeTrailEffect> longTrail = new CSmokeTrailEffect( identity, 10, nullptr, VNULL3, rotation, 0, false );
	longTrail->UpdatePlacement( CVec3( 3, 0, 0 ), rotation, 100, false );
	EXPECT_EQ( Snapshot( longTrail ).fTime, 0.0f );
	EXPECT_EQ( Snapshot( longTrail ).vPosition, VNULL3 );
	for ( const float fInterval : { 0.0f,
	                                -1.0f,
	                                std::numeric_limits<float>::quiet_NaN(),
	                                std::numeric_limits<float>::infinity(),
	                                std::numeric_limits<float>::denorm_min() } )
	{
		CObj<CSmokeTrailEffect> trail = new CSmokeTrailEffect( identity, fInterval, nullptr, VNULL3, rotation, 0, false );
		trail->UpdatePlacement( CVec3( 10000, 0, 0 ), rotation, 100, false );
		const STrailSnapshot snapshot = Snapshot( trail );
		EXPECT_EQ( snapshot.vPosition, CVec3( 10000, 0, 0 ) );
		EXPECT_EQ( snapshot.fTime, 100.0f );
	}
}

TEST( EffectLoopProgress, NavalShoreSearchStopsAtBlockedWaterAndHandlesIdenticalEndpoints )
{
	CObj<CAIMap> map = new CAIMap( 8, 8, 32, 2, 8 );
	const CVec2 start( 48, 48 );
	EXPECT_EQ( CExecutorTransportReinforcement::FindUnloadPoint( map, start, start ), start );
	EXPECT_EQ( CExecutorTransportReinforcement::FindUnloadPoint(
								 map, start, CVec2( std::numeric_limits<float>::quiet_NaN(), 48 ) ),
	           start );
	map->GetTerrain()->AddStaticObjectTiles( std::list<SObjTileInfo>{ SObjTileInfo( SVector( 4, 1 ), EAC_WATER ) } );
	EXPECT_EQ( CExecutorTransportReinforcement::FindUnloadPoint( map, start, CVec2( 80, 48 ) ), CVec2( 112, 48 ) );
	// Without a blocked tile, the boundary itself ends the scan.
	EXPECT_EQ( CExecutorTransportReinforcement::FindUnloadPoint( map, start, CVec2( 48, 80 ) ), CVec2( 48, 240 ) );
}

TEST( EffectLoopProgress, LoadedSmokeWithZeroSpacingIsRevalidated )
{
	CObj<CSmokeTrailEffect> trail = new CSmokeTrailEffect;
	CMemoryStream stream;
	{
		STrailSnapshot saved;
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &saved );
	}
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->AddPolymorphicBase( 1, trail.GetPtr() );
	}
	trail->UpdatePlacement( CVec3( 10000, 0, 0 ), CQuat( 0, 0, 0, 1 ), 100, false );
	const STrailSnapshot snapshot = Snapshot( trail );
	EXPECT_EQ( snapshot.fInterval, 2.0f );
	EXPECT_EQ( snapshot.vPosition, CVec3( 10000, 0, 0 ) );
	EXPECT_EQ( snapshot.fTime, 100.0f );
}
