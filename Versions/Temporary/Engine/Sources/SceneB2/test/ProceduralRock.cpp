#include "SceneB2/stdafx.h"

#include "3DLib/Bound.h"
#include "3DLib/GGeometry.h"
#include "SceneB2/ProceduralRock.h"
#include "Stats_B2_M1/Actions.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/BinSaver.h"
#include "System/Dg.h"
#include "System/Streams.h"

#include <limits>
#include <map>
#include <utility>

#include <gtest/gtest.h>

TEST( ProceduralRock, RecreatesTheSeededShapeIndependentlyOfCreationOrder )
{
	const CVec3 vSize( 2.5f, 2.0f, 1.25f );
	CObj<CPtrFuncBase<NGScene::CObjectInfo>> first = CreateProceduralRock( vSize, 1, 42, nullptr );
	CObj<CPtrFuncBase<NGScene::CObjectInfo>> different = CreateProceduralRock( vSize, 1, 43, nullptr );
	CObj<CPtrFuncBase<NGScene::CObjectInfo>> repeated = CreateProceduralRock( vSize, 1, 42, nullptr );
	ASSERT_TRUE( first && different && repeated );
	const auto &points = first->GetValue()->GetPositions();
	EXPECT_EQ( points, repeated->GetValue()->GetPositions() );
	EXPECT_NE( points, different->GetValue()->GetPositions() );

	// A known crown position also catches silent changes to saved rocks after a generator edit.
	const auto peak = std::find_if( points.begin(), points.end(), [&]( const CVec3 &p ) { return p.z == vSize.z; } );
	ASSERT_NE( peak, points.end() );
	EXPECT_NEAR( peak->x, -0.07527595f, 1e-6f );
	EXPECT_NEAR( peak->y, 0.14234061f, 1e-6f );
}

TEST( ProceduralRock, MeshIsClosedAndBoundsMatchItsLogicalShape )
{
	for ( const uint32_t seed : { 0u, 1u, 42u, UINT32_MAX } )
	{
		SBound bound;
		CObj<CPtrFuncBase<NGScene::CObjectInfo>> provider = CreateProceduralRock( CVec3( 2, 2, 1 ), 1, seed, &bound );
		ASSERT_TRUE( provider );
		const NGScene::CObjectInfo &mesh = *provider->GetValue();
		const auto &points = mesh.GetPositions();
		SBoundCalcer meshBounds;
		meshBounds.LookSet( points, SGetSelf<CVec3>() );
		SBound expected;
		meshBounds.Make( &expected );
		EXPECT_EQ( bound.s.ptCenter, expected.s.ptCenter );
		EXPECT_EQ( bound.ptHalfBox, expected.ptHalfBox );

		std::vector<STriangle> triangles;
		mesh.GetPosTriangles( &triangles );
		ASSERT_FALSE( triangles.empty() );
		std::map<std::pair<uint16_t, uint16_t>, int> edges;
		for ( const STriangle &tri : triangles )
		{
			ASSERT_LT( tri.i1, points.size() );
			ASSERT_LT( tri.i2, points.size() );
			ASSERT_LT( tri.i3, points.size() );
			EXPECT_GT( fabs2( ( points[tri.i2] - points[tri.i1] ) ^ ( points[tri.i3] - points[tri.i1] ) ), 0.0f );
			++edges[{ tri.i1, tri.i2 }];
			++edges[{ tri.i2, tri.i3 }];
			++edges[{ tri.i3, tri.i1 }];
		}
		for ( const auto &[edge, count] : edges )
		{
			EXPECT_EQ( count, 1 );
			EXPECT_EQ( edges.count( { edge.second, edge.first } ), 1u );
		}
	}
}

TEST( ProceduralRock, RejectsInvalidRecipe )
{
	EXPECT_EQ( CreateProceduralRock( CVec3( 2, 0, 1 ), 0.5f, 1, nullptr ), nullptr );
	EXPECT_EQ( CreateProceduralRock( CVec3( 2, 2, 1 ), std::numeric_limits<float>::quiet_NaN(), 1, nullptr ), nullptr );
}

TEST( ProceduralRock, MapPlacementAndCreationUpdatePreserveTheSeed )
{
	NDb::SMapObjectInfo placement;
	SNewUnitInfo update;
	EXPECT_EQ( placement.nProceduralSeed, 0 );
	EXPECT_EQ( update.nProceduralSeed, 0 );

	placement.nProceduralSeed = -1234567;
	update.nProceduralSeed = placement.nProceduralSeed;
	update.center = VNULL2;
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &placement );
		saver->Add( 2, &update );
	}

	stream.Seek( 0 );
	NDb::SMapObjectInfo loadedPlacement;
	SNewUnitInfo loadedUpdate;
	CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
	loader->Add( 1, &loadedPlacement );
	loader->Add( 2, &loadedUpdate );
	EXPECT_EQ( loadedPlacement.nProceduralSeed, placement.nProceduralSeed );
	EXPECT_EQ( loadedUpdate.nProceduralSeed, placement.nProceduralSeed );
}
