#include "../stdafx.h"

#include "../AIMap.h"
#include "../CommonPathFinder.h"
#include "../StaticPath.h"
#include "../Terrain.h"

#include <list>

#include <gtest/gtest.h>

namespace
{
class PathFinderBounds : public testing::Test
{
protected:
	CObj<CAIMap> map;
	CObj<CCommonPathFinder> finder;
	const SVector start{ 639, 243 };
	const SVector finish{ 630, 177 };

	void SetUp() override
	{
		// A rectangular map also catches validation against the square scratch buffer.
		map = new CAIMap( 640, 320, 32, 2, 640 );
		CTerrain *terrain = map->GetTerrain();
		terrain->PrepareTerraTypes( 1 );
		terrain->SetTerraTypes( 0, 1.0f, EAC_NONE, 0, 0 );
		CArray2D<uint8_t> types( 320, 160 );
		types.FillZero();
		terrain->UpdateTypes( 0, 0, 320, 160, types );
		terrain->FinishInitMode();
		finder = new CCommonPathFinder;
	}

	void BlockStart()
	{
		const std::list<SObjTileInfo> tiles{ SObjTileInfo( start, EAC_WHELL ) };
		map->GetTerrain()->AddStaticObjectTiles( tiles );
		map->GetTerrain()->SmoothLock();
		ASSERT_EQ( map->GetTerrain()->CanUnitGo( 0, start, EAC_WHELL ), FREE_NONE );
	}

	void SetParameters( const SVector &lastGood )
	{
		finder->SetPathParameters(
				0, EAC_WHELL, map->GetPointByTile( start ), map->GetPointByTile( finish ), lastGood, map );
	}

	void ExpectInside( IStaticPath *path )
	{
		ASSERT_NE( path, nullptr );
		EXPECT_TRUE( map->IsTileInside( path->GetStartTile() ) );
		EXPECT_TRUE( map->IsTileInside( path->GetFinishTile() ) );
		for ( int i = 0; i < path->GetLength(); ++i )
		{
			EXPECT_TRUE( map->IsTileInside( path->GetTile( i ) ) );
		}
	}
};

TEST_F( PathFinderBounds, RejectsReportedOffMapFallbackBeforeSearching )
{
	ASSERT_NO_FATAL_FAILURE( BlockStart() );
	// All three match dumps indexed mapBuf[640][243]. An inside diagonal neighbor
	// passes the old escape check, but the first step toward the target stays outside.
	ASSERT_NE( map->GetTerrain()->CanUnitGo( 0, SVector( 639, 242 ), EAC_WHELL ), FREE_NONE );
	finder->SetPathParameters(
			0, EAC_WHELL, map->GetPointByTile( 640, 243 ), map->GetPointByTile( finish ), SVector( 640, 243 ), map );
	CObj<IStaticPath> path = finder->CreatePath( false );
	EXPECT_EQ( path, nullptr );
	EXPECT_EQ( finder->GetPathLength(), 0 );
	EXPECT_EQ( finder->GetStartTile(), start );
	EXPECT_EQ( finder->GetFinishTile(), start );
}

TEST_F( PathFinderBounds, RejectsFallbacksBeyondEveryMapEdge )
{
	ASSERT_NO_FATAL_FAILURE( BlockStart() );
	for ( const SVector &lastGood :
	      { SVector( -1, 243 ), SVector( 640, 243 ), SVector( 320, -1 ), SVector( 320, 320 ), SVector( -1, -1 ) } )
	{
		SCOPED_TRACE( testing::Message() << lastGood.x << ", " << lastGood.y );
		SetParameters( lastGood );
		EXPECT_FALSE( finder->DoesPathExist() );
		EXPECT_EQ( finder->GetPathLength(), 0 );
		EXPECT_EQ( finder->GetStartTile(), start );
		EXPECT_EQ( finder->GetFinishTile(), start );

		SetParameters( lastGood );
		CObj<IStaticPath> path = finder->CreatePath( true );
		ASSERT_NE( path, nullptr );
		ASSERT_EQ( path->GetLength(), 1 );
		EXPECT_EQ( path->GetTile( 0 ), start );
		EXPECT_EQ( path->GetFinishTile(), start );
		ExpectInside( path );
	}
}

TEST_F( PathFinderBounds, PassableStartDoesNotNeedValidFallback )
{
	ASSERT_NE( map->GetTerrain()->CanUnitGo( 0, start, EAC_WHELL ), FREE_NONE );
	SetParameters( SVector( 640, 243 ) );
	CObj<IStaticPath> path = finder->CreatePath( false );
	ASSERT_NE( path, nullptr );
	EXPECT_EQ( path->GetStartTile(), start );
	EXPECT_EQ( path->GetFinishTile(), finish );
	ExpectInside( path );
}

TEST_F( PathFinderBounds, ValidFallbackStillEscapesBlockedStartAfterFailedSearch )
{
	ASSERT_NO_FATAL_FAILURE( BlockStart() );
	SetParameters( SVector( -1, -1 ) );
	EXPECT_FALSE( finder->DoesPathExist() );
	const SVector lastGood( 630, 243 );
	ASSERT_NE( map->GetTerrain()->CanUnitGo( 0, lastGood, EAC_WHELL ), FREE_NONE );
	SetParameters( lastGood );
	CObj<IStaticPath> path = finder->CreatePath( false );
	ASSERT_NE( path, nullptr );
	EXPECT_EQ( path->GetStartTile(), lastGood );
	EXPECT_EQ( path->GetFinishTile(), finish );
	ExpectInside( path );
}
} // namespace
