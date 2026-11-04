#include "3Dmotor/stdafx.h"

#include "3DLib/Bound.h"
#include "3DLib/Transform.h"
#include "3Dmotor/OcTree.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <random>

#include <gtest/gtest.h>

namespace
{
class CTestNode : public COcTreeNode<CTestNode, 8, true>
{
	OBJECT_NOCOPY_METHODS( CTestNode );

public:
	bool occupied = false;

	bool IsEmpty() override
	{
		return !occupied;
	}
};

CObj<CTestNode> MakeTree()
{
	CObj<CTestNode> root = new CTestNode;
	root->SetSize( CVec3( -128, -128, -128 ), 1024 );
	return root;
}

void ExpectEnclosed( CTestNode *node, const CVec3 &center, float radius )
{
	// Every ancestor must enclose the object: rejecting any one hides its subtree.
	for ( ; node; node = node->GetUpLink() )
	{
		SSphere sphere;
		node->GetBound( &sphere );
		EXPECT_LE( fabs( center - sphere.ptCenter ) + radius, sphere.fRadius + 0.001f );
		SBound box;
		node->GetBound( &box );
		EXPECT_LE( std::fabs( center.x - box.s.ptCenter.x ) + radius, box.ptHalfBox.x + 0.001f );
		EXPECT_LE( std::fabs( center.y - box.s.ptCenter.y ) + radius, box.ptHalfBox.y + 0.001f );
		EXPECT_LE( std::fabs( center.z - box.s.ptCenter.z ) + radius, box.ptHalfBox.z + 0.001f );
	}
}

bool CanSelect( CTestNode *node, CTestNode *target, CTransformStack *clip )
{
	SSphere bound;
	node->GetBound( &bound );
	if ( !clip->PushClipHint( bound ) )
	{
		return false;
	}
	bool found = node == target;
	for ( int i = 0; i < 8 && !found; ++i )
	{
		if ( node->GetNode( i ) )
		{
			found = CanSelect( node->GetNode( i ), target, clip );
		}
	}
	clip->PopClipHint();
	return found;
}
} // namespace

TEST( OcTree, EnclosesGeometryBeyondTwentyPatches )
{
	auto root = MakeTree();
	// A map patch is 16 * 2.75 = 44 render units; the old root ended at 896.
	for ( int patches : { 20, 21, 32, 64, 128 } )
	{
		SCOPED_TRACE( patches );
		for ( const CVec3 &center : { CVec3( patches * 44 - 2, 200, 0 ),
		                              CVec3( 200, patches * 44 - 2, 0 ),
		                              CVec3( patches * 44 - 2, patches * 44 - 2, 0 ) } )
		{
			CTestNode *node = root->GetNode( center, 1 );
			node->occupied = true;
			ExpectEnclosed( node, center, 1 );
		}
	}
}

TEST( OcTree, CameraAndShadowPassesReachDistantGeometry )
{
	auto root = MakeTree();
	const CVec3 center( 1400, 1400, 0 );
	CTestNode *node = root->GetNode( center, 1 );
	node->occupied = true;

	CTransformStack camera;
	camera.MakeParallel( 20, 20, 0.1f, 100 );
	camera.Push( CVec3( -center.x, -center.y, 10 ) );
	EXPECT_TRUE( CanSelect( root, node, &camera ) );
	CTestNode *nearNode = root->GetNode( CVec3( 200, 200, 0 ), 1 );
	EXPECT_FALSE( CanSelect( root, nearNode, &camera ) );

	// Shadow-caster selection uses world-space clipping planes without a projection.
	CTransformStack shadow;
	shadow.Make();
	shadow.AddClipPlane( CVec4( 1, 0, 0, -center.x + 10 ) );
	shadow.AddClipPlane( CVec4( -1, 0, 0, center.x + 10 ) );
	shadow.AddClipPlane( CVec4( 0, 1, 0, -center.y + 10 ) );
	shadow.AddClipPlane( CVec4( 0, -1, 0, center.y + 10 ) );
	EXPECT_TRUE( CanSelect( root, node, &shadow ) );
	EXPECT_FALSE( CanSelect( root, nearNode, &shadow ) );
}

TEST( OcTree, GrowthPreservesExistingNodesAndParentLinks )
{
	auto root = MakeTree();
	const CVec3 original( 200, 300, 0 );
	CTestNode *first = root->GetNode( original, 1 );
	first->occupied = true;
	// Exercise both directions on all axes, including the old exact boundaries.
	for ( const CVec3 &center :
	      { CVec3( 896, 200, 0 ), CVec3( 4000, 5000, 2000 ), CVec3( -3000, -4000, -2000 ), CVec3( 10000, 0, 0 ) } )
	{
		CTestNode *node = root->GetNode( center, 1 );
		node->occupied = true;
		ExpectEnclosed( node, center, 1 );
		EXPECT_EQ( root->GetNode( original, 1 ), first );
		ExpectEnclosed( first, original, 1 );
		EXPECT_FALSE( root->Walk() );
		EXPECT_EQ( root->GetNode( original, 1 ), first );
	}
}

TEST( OcTree, LargeUpdateHintsStayAtRoot )
{
	auto root = MakeTree();
	// MakeLargeHintBound deliberately covers the whole scene. Its radius must not
	// drive tree growth or move the update list away from the stable root object.
	EXPECT_EQ( root->GetNode( CVec3( 0, 0, 0 ), 1e6f ), root.GetPtr() );
	EXPECT_FLOAT_EQ( root->GetSize(), 1024 );
	root->occupied = true;
	root->GetNode( CVec3( 2000, 2000, 0 ), 1 );
	EXPECT_TRUE( root->occupied );
	EXPECT_FALSE( root->Walk() );
	EXPECT_EQ( root->GetNode( CVec3( 0, 0, 0 ), 1e6f ), root.GetPtr() );
}

// The placement shortcut of CAnimatedBoundPart::Update. The tracked descent has
// to return exactly what the plain one returns, and whenever its recorded ranges
// say a new centre and radius are placed the same, a real descent has to agree.
// Moves are mostly small steps from the last placement, the way a unit drives,
// with jumps, centres on split planes, signed zeros, radii at the size thresholds,
// points outside the root that make it grow, and NaN mixed in.
TEST( OcTree, RecordedPlacementPredictsTheDescent )
{
	CObj<CTestNode> root = MakeTree();
	std::mt19937 rng( 20261103 );
	std::uniform_real_distribution<float> unit( 0.0f, 1.0f );
	std::uniform_int_distribution<int> kind( 0, 19 );
	SOcTreePlacement place;
	CTestNode *pPlaced = 0;
	CVec3 vCenter( 100.0f, 100.0f, 10.0f );
	float fRadius = 3.0f;
	int nPredicted = 0, nMoves = 0;
	for ( int k = 0; k < 200000; ++k )
	{
		switch ( kind( rng ) )
		{
			case 0:
				vCenter = CVec3( unit( rng ) * 1000.0f - 64.0f, unit( rng ) * 1000.0f - 64.0f, unit( rng ) * 100.0f );
				break;
			case 1:
				// A split plane of the root or below it, or the root's edge.
				vCenter.x = -128.0f + 1024.0f * ( ( rng() % 9 ) / 8.0f );
				break;
			case 2:
				vCenter.y = ( rng() % 2 ) ? 0.0f : -0.0f;
				break;
			case 3:
				// Exactly a node's stop threshold, half its GetMaxElementSize.
				fRadius = 0.5f * ( 0.25f * ( 1024.0f / float( 1 << ( rng() % 8 ) ) ) * sqrtf( 3.0f ) );
				break;
			case 4:
				fRadius = unit( rng ) * 200.0f;
				break;
			case 5:
				if ( rng() % 50 == 0 )
				{
					vCenter.z = std::numeric_limits<float>::quiet_NaN();
				}
				break;
			case 6:
				if ( rng() % 200 == 0 )
				{
					// Far outside: the root grows, which must invalidate placements.
					vCenter = CVec3( -3000.0f * unit( rng ), 4000.0f * unit( rng ), 0.0f );
				}
				break;
			default:
				vCenter.x += ( unit( rng ) - 0.5f ) * 2.0f;
				vCenter.y += ( unit( rng ) - 0.5f ) * 2.0f;
				break;
		}
		if ( std::isnan( vCenter.z ) && kind( rng ) < 10 )
		{
			vCenter.z = 0.0f;
		}
		++nMoves;
		if ( pPlaced && place.IsSamePlacement( vCenter, fRadius, root->GetLayoutChanges() ) )
		{
			++nPredicted;
			ASSERT_EQ( root->GetNode( vCenter, fRadius ), pPlaced ) << k;
			continue;
		}
		CTestNode *pPlain = root->GetNode( vCenter, fRadius );
		pPlaced = root->GetNode( vCenter, fRadius, &place );
		ASSERT_EQ( pPlain, pPlaced ) << k;
	}
	// The shortcut has to hit for the common small step, or it saves nothing.
	EXPECT_GT( nPredicted, nMoves / 2 );
	printf( "placements predicted: %d of %d\n", nPredicted, nMoves );
}
