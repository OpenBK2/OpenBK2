// Exercise the same geometry-map APIs used by click selection and the 64 x 64
// box-selection grid, with in-memory hulls so no game data or GPU is required.
#include "3Dmotor/stdafx.h"
#include "3DLib/Bound.h"
#include "3DLib/MemObject.h"
#include "3Dmotor/aiMap.h"
#include "3Dmotor/aiRender.h"

#include <gtest/gtest.h>

namespace
{
class CTestArmor : public NDb::CResource
{
	OBJECT_BASIC_METHODS( CTestArmor );
public:
	int GetTypeID() const override { return 0; }
};

class CTestUnit : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTestUnit );
public:
	std::vector<CObj<CObjectBase>> hulls;
};

class CChangeCounter : public NAI::IAIMapTracker
{
	OBJECT_NOCOPY_METHODS( CChangeCounter );
public:
	int count = 0;
	void OnChange() override { ++count; }
};

class LargeMapSelection : public testing::Test
{
protected:
	// Destroy hulls before their map and geometry.
	CObj<NAI::IAIMap> map = NAI::CreateAIMap();
	CObj<CTestArmor> armor = new CTestArmor;
	CObj<CMemObject> geometry = new CMemObject;
	std::vector<CObj<CTestUnit>> units;

	void SetUp() override
	{
		geometry->CreateCube( CVec3( -1, -1, -1 ), CVec3( 2, 2, 2 ) );
	}

	void PlaceUnit( CTestUnit *unit, const CVec3 &center, int mask = 1 )
	{
		unit->hulls.clear();
		SVisitorBaseAccess access;
		access.StartNewObject( map, &unit->hulls, unit );
		SHMatrix placement;
		Identity( &placement );
		placement._14 = center.x;
		placement._24 = center.y;
		placement._34 = center.z;
		map->AddHull( geometry, placement, armor, 0, mask );
	}

	CTestUnit *AddUnit( const CVec3 &center, int mask = 1 )
	{
		CObj<CTestUnit> unit = new CTestUnit;
		units.push_back( unit );
		PlaceUnit( unit, center, mask );
		return unit;
	}

	std::vector<NAI::SInterval> Pick( const CVec3 &center, int mask = 1 )
	{
		// Offset the ray from the cube's triangle seam.
		CRay ray( center + CVec3( 0.25f, 0.375f, -20 ), CVec3( 0, 0, 1 ) );
		std::vector<NAI::SInterval> hits;
		map->Trace( ray, &hits, mask );
		return hits;
	}

	CObj<CChangeCounter> Track( const CVec3 &center, int mask = 1, float radius = 3 )
	{
		CObj<CChangeCounter> tracker = new CChangeCounter;
		SBound bound;
		bound.SphereInit( center, radius );
		map->AddTracker( tracker, bound, mask );
		return tracker;
	}
};

TEST_F( LargeMapSelection, ClickRaysReachGeometryBeyondTwentyPatches )
{
	for ( int patches : { 20, 21, 32, 64, 128 } )
	{
		SCOPED_TRACE( patches );
		const float edge = patches * 44.0f - 2;
		for ( const CVec3 &center : { CVec3( edge, 200, 0 ),
			CVec3( 200, edge, 0 ), CVec3( edge, edge, 0 ) } )
		{
			CTestUnit *unit = AddUnit( center );
			auto hits = Pick( center );
			ASSERT_EQ( hits.size(), 1u );
			EXPECT_EQ( hits.front().pSrc->pUserData.GetPtr(), unit );
			EXPECT_NEAR( hits.front().enter.fT, 19, 0.001f );
			EXPECT_TRUE( Pick( center, 2 ).empty() );
		}
	}
}

TEST_F( LargeMapSelection, BoxSelectionGridIncludesOnlyObjectsInRectangle )
{
	const CVec3 center( 1400, 1400, 0 );
	CTestUnit *first = AddUnit( center );
	CTestUnit *second = AddUnit( center + CVec3( 5, 0, 0 ) );
	CTestUnit *outsideBox = AddUnit( center + CVec3( 9, 0, 0 ) );
	CTestUnit *outsideView = AddUnit( center + CVec3( 50, 0, 0 ) );

	CTransformStack camera;
	camera.MakeProjective( 1.0f, 60, 0.1f, 100 );
	camera.Push( CVec3( -center.x, -center.y, 20 ) );
	NAI::CFastRenderer grid;
	grid.Init( camera, 32 );
	// Explicit frame markers keep this standalone rasterizer fixture reproducible.
	grid.nTraceFrame = 0;
	grid.gridFrames.FillEvery( -1 );
	map->TraceGrid( &grid, -1, NAI::IAIMap::STH_SORT_INTERVALS );

	std::set<CObjectBase*> inView, inBox;
	for ( int y = 0; y < grid.resGrid.GetSizeY(); ++y )
	{
		for ( int x = 0; x < grid.resGrid.GetSizeX(); ++x )
		{
			for ( auto *hit = grid.resGrid[y][x]; hit; hit = hit->pNext )
			{
				inView.insert( hit->pObject );
				if ( x >= 16 && x < 48 && y >= 16 && y < 48 )
					inBox.insert( hit->pObject );
			}
		}
	}
	EXPECT_EQ( inBox, (std::set<CObjectBase*>{ first, second }) );
	EXPECT_EQ( inView, (std::set<CObjectBase*>{ first, second, outsideBox }) );
	EXPECT_EQ( inView.count( outsideView ), 0u );
}

TEST_F( LargeMapSelection, PendingNotificationsSurviveGrowth )
{
	const CVec3 nearCenter( 200, 200, 0 );
	auto nearTracker = Track( nearCenter );
	auto otherMask = Track( nearCenter, 2 );
	auto rootTracker = Track( CVec3( 0, 0, 0 ), 1, 300 );
	CTestUnit *unit = AddUnit( nearCenter );

	// Adding a tracker grows the tree without inserting a new hull notification.
	// Pending events below the old root must still be delivered by the next Sync.
	auto farTracker = Track( CVec3( 9000, 9000, 0 ) );
	EXPECT_EQ( nearTracker->count, 0 );
	map->Sync( NAI::IAIMap::ST_FAST );
	EXPECT_EQ( nearTracker->count, 0 );
	map->Sync();
	EXPECT_EQ( nearTracker->count, 1 );
	EXPECT_EQ( rootTracker->count, 1 );
	EXPECT_EQ( farTracker->count, 0 );
	EXPECT_EQ( otherMask->count, 0 );
	map->Sync();
	EXPECT_EQ( nearTracker->count, 1 );
	EXPECT_EQ( rootTracker->count, 1 );

	// Repeat for a pending removal and growth in the opposite direction.
	unit->hulls.clear();
	auto negativeTracker = Track( CVec3( -9000, -9000, 0 ) );
	map->Sync();
	EXPECT_EQ( nearTracker->count, 2 );
	EXPECT_EQ( rootTracker->count, 2 );
	EXPECT_EQ( farTracker->count, 0 );
	EXPECT_EQ( negativeTracker->count, 0 );
	EXPECT_EQ( otherMask->count, 0 );
}

TEST_F( LargeMapSelection, MovingAndRemovingGeometryAcrossBoundary )
{
	const CVec3 original( 878, 200, 0 ), destination( 1400, 1400, 0 );
	auto oldTracker = Track( original );
	auto newTracker = Track( destination );
	CTestUnit *unit = AddUnit( original );
	map->Sync();
	EXPECT_EQ( oldTracker->count, 1 );
	EXPECT_EQ( newTracker->count, 0 );

	PlaceUnit( unit, destination );
	map->Sync();
	EXPECT_TRUE( Pick( original ).empty() );
	auto hits = Pick( destination );
	ASSERT_EQ( hits.size(), 1u );
	EXPECT_EQ( hits.front().pSrc->pUserData.GetPtr(), unit );
	EXPECT_EQ( oldTracker->count, 2 );
	EXPECT_EQ( newTracker->count, 1 );

	unit->hulls.clear();
	map->Sync();
	EXPECT_TRUE( Pick( destination ).empty() );
	EXPECT_EQ( newTracker->count, 2 );
}
}
