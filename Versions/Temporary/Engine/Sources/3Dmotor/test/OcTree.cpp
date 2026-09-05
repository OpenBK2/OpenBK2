#include "3Dmotor/stdafx.h"
#include "3DLib/Bound.h"
#include "3DLib/Transform.h"
#include "3Dmotor/OcTree.h"

#include <gtest/gtest.h>

namespace
{
class CTestNode : public COcTreeNode<CTestNode, 8, true>
{
	OBJECT_NOCOPY_METHODS( CTestNode );
public:
	bool occupied = false;

	bool IsEmpty() override { return !occupied; }
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
		EXPECT_LE( fabs( center.x - box.s.ptCenter.x ) + radius, box.ptHalfBox.x + 0.001f );
		EXPECT_LE( fabs( center.y - box.s.ptCenter.y ) + radius, box.ptHalfBox.y + 0.001f );
		EXPECT_LE( fabs( center.z - box.s.ptCenter.z ) + radius, box.ptHalfBox.z + 0.001f );
	}
}

bool CanSelect( CTestNode *node, CTestNode *target, CTransformStack *clip )
{
	SSphere bound;
	node->GetBound( &bound );
	if ( !clip->PushClipHint( bound ) )
		return false;
	bool found = node == target;
	for ( int i = 0; i < 8 && !found; ++i )
	{
		if ( node->GetNode( i ) )
			found = CanSelect( node->GetNode( i ), target, clip );
	}
	clip->PopClipHint();
	return found;
}
}

TEST( OcTree, EnclosesGeometryBeyondTwentyPatches )
{
	auto root = MakeTree();
	// A map patch is 16 * 2.75 = 44 render units; the old root ended at 896.
	for ( int patches : { 20, 21, 32, 64, 128 } )
	{
		SCOPED_TRACE( patches );
		for ( const CVec3 &center : { CVec3( patches * 44 - 2, 200, 0 ),
			CVec3( 200, patches * 44 - 2, 0 ), CVec3( patches * 44 - 2, patches * 44 - 2, 0 ) } )
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
	for ( const CVec3 &center : { CVec3( 896, 200, 0 ), CVec3( 4000, 5000, 2000 ),
		CVec3( -3000, -4000, -2000 ), CVec3( 10000, 0, 0 ) } )
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
