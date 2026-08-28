// Walking the skeleton hierarchy: world matrices, and the skinning matrices the
// renderer multiplies vertices by.
//
// This is where CAddBoneFilter::Recalc crashed against the stubs. It allocates a
// world pose, builds it, and reads the result with no null check:
//
//     granny_real32 *pMatrix = GrannyGetWorldPose4x4( pGlobal, nAddBone );
//     value.forward.Set( pMatrix[0], pMatrix[4], pMatrix[8], pMatrix[12], ... );
//
// so the layout is an ABI and the bounds are load bearing.
//
// Every convention below was measured against granny2.dll with inputs chosen so
// the candidates disagree, because each had a plausible alternative and a wrong
// one gives a skeleton that renders and is quietly wrong:
//
//   layout      row vector, translation in elements 12 to 14, and the upper 3x3
//               is the transpose of the column convention's rotation times
//               scale. A 90 degree turn about Z with a diag(2,3,4) scale gives
//               rows (0,2,0), (-3,0,0), (0,0,4).
//   chain       World[i] = Local[i] * World[parent], with the offset at the far
//               end: a root is Local * Offset and every child inherits it. An
//               offset translating by (1000,2000,3000) appears in every bone's
//               matrix, once.
//   composite   InverseWorld4x4[i] * World[i], in that order.
//   flags       select the parts as a cascade rather than as a bitmask: 0 is the
//               identity, 1 is position only, 2 and 3 add the rotation, and 4 to
//               7 add the scale. A scale bit brings the rotation with it, and any
//               non-zero value brings the position.
//   offset      null is treated as the identity.
//
// Agreement on real skeletons is measured separately and covers far more:
// scripts/port/gr2diff.py builds a world pose for every model in the corpus.

#include "LocalPose.h"
#include "Structures.h"
#include "WorldPose.h"

#include <gr2/granny.h>

#include <cmath>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

using namespace NGr2;

namespace
{

//! A chain of three bones, each a child of the one before.
struct SChain
{
	SChain()
	{
		Bones.resize( 3 );
		for ( int32_t i = 0; i < 3; ++i )
		{
			SBone &bone = Bones[static_cast<size_t>( i )];
			bone.pszName = "b";
			bone.nParentIndex = i - 1;
			bone.LocalTransform.Orientation[3] = 1.0f;
			for ( int k = 0; k < 3; ++k )
			{
				bone.LocalTransform.ScaleShear[k][k] = 1.0f;
			}
			// An inverse world matrix that is obviously not the identity, so the
			// composite shows which side it was applied on.
			for ( int k = 0; k < 16; ++k )
			{
				bone.InverseWorld4x4[k] = ( k % 5 == 0 ) ? 1.0f : 0.0f;
			}
			bone.InverseWorld4x4[12] = -100.0f * ( i + 1 );
		}

		Skeleton.pszName = "Skel";
		Skeleton.nBoneCount = 3;
		Skeleton.pBones = Bones.data();
	}

	granny_skeleton *Handle() { return reinterpret_cast<granny_skeleton *>( &Skeleton ); }

	std::vector<SBone> Bones;
	SSkeleton Skeleton = {};
};

//! A local pose where bone 0 steps +1 in X, bone 1 +10 in Y, bone 2 +100 in Z.
granny_local_pose *StepPose()
{
	granny_local_pose *pPose = GrannyNewLocalPose( 3 );
	for ( granny_int32x i = 0; i < 3; ++i )
	{
		STransform *pTransform =
			reinterpret_cast<STransform *>( GrannyGetLocalPoseTransform( pPose, i ) );
		GrannyMakeIdentity( reinterpret_cast<granny_transform *>( pTransform ) );
		pTransform->nFlags = TRANSFORM_HAS_POSITION | TRANSFORM_HAS_ORIENTATION
		                     | TRANSFORM_HAS_SCALESHEAR;
		pTransform->Position[0] = ( i == 0 ) ? 1.0f : 0.0f;
		pTransform->Position[1] = ( i == 1 ) ? 10.0f : 0.0f;
		pTransform->Position[2] = ( i == 2 ) ? 100.0f : 0.0f;
	}
	return pPose;
}

const float IDENTITY[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

}

TEST( WorldPose, ComposesUpTheParentChain )
{
	SChain chain;
	granny_local_pose *pLocal = StepPose();
	granny_world_pose *pWorld = GrannyNewWorldPose( 3 );
	ASSERT_NE( nullptr, pWorld );

	GrannyBuildWorldPose( chain.Handle(), 0, 3, pLocal, IDENTITY, pWorld );

	// Each bone accumulates its ancestors' steps.
	const float *pB0 = GrannyGetWorldPose4x4( pWorld, 0 );
	const float *pB1 = GrannyGetWorldPose4x4( pWorld, 1 );
	const float *pB2 = GrannyGetWorldPose4x4( pWorld, 2 );
	ASSERT_NE( nullptr, pB0 );

	EXPECT_FLOAT_EQ( 1.0f, pB0[12] );
	EXPECT_FLOAT_EQ( 0.0f, pB0[13] );
	EXPECT_FLOAT_EQ( 1.0f, pB1[12] ) << "inherited from the root";
	EXPECT_FLOAT_EQ( 10.0f, pB1[13] );
	EXPECT_FLOAT_EQ( 1.0f, pB2[12] );
	EXPECT_FLOAT_EQ( 10.0f, pB2[13] );
	EXPECT_FLOAT_EQ( 100.0f, pB2[14] );

	GrannyFreeWorldPose( pWorld );
	GrannyFreeLocalPose( pLocal );
}

TEST( WorldPose, TheOffsetGoesAtTheFarEndOfTheChain )
{
	// Applied once, to the root, and inherited. Applying it per bone would have
	// multiplied it up the chain instead.
	SChain chain;
	granny_local_pose *pLocal = StepPose();
	granny_world_pose *pWorld = GrannyNewWorldPose( 3 );

	float offset[16];
	memcpy( offset, IDENTITY, sizeof( offset ) );
	offset[12] = 1000.0f;
	offset[13] = 2000.0f;
	offset[14] = 3000.0f;

	GrannyBuildWorldPose( chain.Handle(), 0, 3, pLocal, offset, pWorld );

	const float *pB2 = GrannyGetWorldPose4x4( pWorld, 2 );
	ASSERT_NE( nullptr, pB2 );
	EXPECT_FLOAT_EQ( 1001.0f, pB2[12] ) << "the offset once, not three times";
	EXPECT_FLOAT_EQ( 2010.0f, pB2[13] );
	EXPECT_FLOAT_EQ( 3100.0f, pB2[14] );

	GrannyFreeWorldPose( pWorld );
	GrannyFreeLocalPose( pLocal );
}

TEST( WorldPose, ANullOffsetIsTheIdentity )
{
	SChain chain;
	granny_local_pose *pLocal = StepPose();
	granny_world_pose *pWithNull = GrannyNewWorldPose( 3 );
	granny_world_pose *pWithIdentity = GrannyNewWorldPose( 3 );

	GrannyBuildWorldPose( chain.Handle(), 0, 3, pLocal, nullptr, pWithNull );
	GrannyBuildWorldPose( chain.Handle(), 0, 3, pLocal, IDENTITY, pWithIdentity );

	EXPECT_EQ( 0, memcmp( GrannyGetWorldPose4x4( pWithNull, 2 ),
	                      GrannyGetWorldPose4x4( pWithIdentity, 2 ), 16 * sizeof( float ) ) );

	GrannyFreeWorldPose( pWithNull );
	GrannyFreeWorldPose( pWithIdentity );
	GrannyFreeLocalPose( pLocal );
}

TEST( WorldPose, TheCompositeUndoesTheBindPoseFirst )
{
	// InverseWorld4x4 * World, in that order. The other order would put the
	// world matrix's translation through the inverse instead of the reverse.
	SChain chain;
	granny_local_pose *pLocal = StepPose();
	granny_world_pose *pWorld = GrannyNewWorldPose( 3 );

	GrannyBuildWorldPose( chain.Handle(), 0, 3, pLocal, IDENTITY, pWorld );

	// Bone 1: InverseWorld translates by -200 in X, World by (1, 10, 0).
	const float *pComposite = GrannyGetWorldPoseComposite4x4( pWorld, 1 );
	ASSERT_NE( nullptr, pComposite );
	EXPECT_FLOAT_EQ( -199.0f, pComposite[12] );
	EXPECT_FLOAT_EQ( 10.0f, pComposite[13] );

	GrannyFreeWorldPose( pWorld );
	GrannyFreeLocalPose( pLocal );
}

TEST( WorldPose, TheFlagsSelectThePartsAsACascade )
{
	// Not a bitmask, which is what a first version of this assumed and what a
	// test of only 0 and 7 will happily confirm. Measured over all eight values
	// with a position, a rotation and a scale that are all distinguishable:
	//
	//     0      identity
	//     1      position only
	//     2, 3   position and rotation
	//     4..7   position, rotation and scale
	//
	// The DLL dispatches on the highest capability present and the branch takes
	// everything below it, so a scale bit brings the rotation with it and any
	// non-zero value brings the position. Reading the bits independently drops
	// the position of every bone whose flags are 2, which is the root bone of
	// real files in the corpus.
	SChain chain;
	chain.Skeleton.nBoneCount = 1;

	granny_local_pose *pLocal = GrannyNewLocalPose( 1 );
	granny_world_pose *pWorld = GrannyNewWorldPose( 1 );
	const float s = 0.70710678f;

	for ( uint32_t nFlags = 0; nFlags < 8; ++nFlags )
	{
		STransform *pTransform =
			reinterpret_cast<STransform *>( GrannyGetLocalPoseTransform( pLocal, 0 ) );
		memset( pTransform, 0, sizeof( *pTransform ) );
		pTransform->nFlags = nFlags;
		pTransform->Position[0] = 5.0f;
		pTransform->Position[1] = 6.0f;
		pTransform->Position[2] = 7.0f;
		pTransform->Orientation[2] = s;
		pTransform->Orientation[3] = s;
		pTransform->ScaleShear[0][0] = 2.0f;
		pTransform->ScaleShear[1][1] = 3.0f;
		pTransform->ScaleShear[2][2] = 4.0f;

		GrannyBuildWorldPose( chain.Handle(), 0, 1, pLocal, IDENTITY, pWorld );
		const float *pMatrix = GrannyGetWorldPose4x4( pWorld, 0 );
		ASSERT_NE( nullptr, pMatrix );

		// The first row says which of rotation and scale were applied: (1,0,0)
		// for neither, (0,1,0) for a rotation alone, (0,2,0) for both.
		const float fExpectedX = ( nFlags < 2 ) ? 1.0f : 0.0f;
		const float fExpectedY = ( nFlags < 2 ) ? 0.0f : ( nFlags < 4 ? 1.0f : 2.0f );
		EXPECT_NEAR( fExpectedX, pMatrix[0], 1e-5f ) << "flags " << nFlags;
		EXPECT_NEAR( fExpectedY, pMatrix[1], 1e-5f ) << "flags " << nFlags;

		// And any non-zero value brings the position, including flags 2.
		const float fExpectedTranslation = ( nFlags == 0 ) ? 0.0f : 5.0f;
		EXPECT_FLOAT_EQ( fExpectedTranslation, pMatrix[12] ) << "flags " << nFlags;
	}

	GrannyFreeWorldPose( pWorld );
	GrannyFreeLocalPose( pLocal );
}

TEST( WorldPose, MayBeSmallerThanItsSkeleton )
{
	// What CAddBoneFilter::Recalc does: allocate nAddBone + 1 entries for a full
	// sized skeleton, build that far, and read the last one.
	SChain chain;
	granny_local_pose *pLocal = StepPose();
	granny_world_pose *pWorld = GrannyNewWorldPose( 2 );
	ASSERT_NE( nullptr, pWorld );

	GrannyBuildWorldPose( chain.Handle(), 0, 2, pLocal, IDENTITY, pWorld );

	const float *pB1 = GrannyGetWorldPose4x4( pWorld, 1 );
	ASSERT_NE( nullptr, pB1 );
	EXPECT_FLOAT_EQ( 1.0f, pB1[12] );
	EXPECT_FLOAT_EQ( 10.0f, pB1[13] );
	EXPECT_EQ( nullptr, GrannyGetWorldPose4x4( pWorld, 2 ) ) << "past the end of the pose";

	GrannyFreeWorldPose( pWorld );
	GrannyFreeLocalPose( pLocal );
}

TEST( WorldPose, ARangeThatDoesNotFitWritesNothing )
{
	SChain chain;
	granny_local_pose *pLocal = StepPose();
	granny_world_pose *pWorld = GrannyNewWorldPose( 2 );

	// More bones than the world pose holds.
	GrannyBuildWorldPose( chain.Handle(), 0, 3, pLocal, IDENTITY, pWorld );
	const float zero[16] = {};
	EXPECT_EQ( 0, memcmp( GrannyGetWorldPose4x4( pWorld, 0 ), zero, sizeof( zero ) ) );

	GrannyFreeWorldPose( pWorld );
	GrannyFreeLocalPose( pLocal );
}

TEST( WorldPose, StartsZeroedRatherThanUninitialised )
{
	// The real DLL leaves a fresh world pose full of whatever the allocator had,
	// NaNs included. Not reproduced: matching garbage is impossible and a
	// deterministic zero is easier to debug.
	granny_world_pose *pWorld = GrannyNewWorldPose( 2 );
	ASSERT_NE( nullptr, pWorld );

	const float zero[16] = {};
	EXPECT_EQ( 0, memcmp( GrannyGetWorldPose4x4( pWorld, 0 ), zero, sizeof( zero ) ) );
	EXPECT_EQ( 0, memcmp( GrannyGetWorldPoseComposite4x4( pWorld, 1 ), zero,
	                      sizeof( zero ) ) );

	GrannyFreeWorldPose( pWorld );
}

TEST( WorldPose, SurvivesWhatTheRealDllDoesNot )
{
	SChain chain;
	granny_local_pose *pLocal = StepPose();
	granny_world_pose *pWorld = GrannyNewWorldPose( 3 );

	// A null result and a null pose both access violate in the DLL.
	GrannyBuildWorldPose( chain.Handle(), 0, 3, pLocal, IDENTITY, nullptr );
	EXPECT_EQ( nullptr, GrannyGetWorldPose4x4( nullptr, 0 ) );
	EXPECT_EQ( nullptr, GrannyGetWorldPoseComposite4x4( nullptr, 0 ) );

	// And a negative index, where the DLL hands back a pointer: an out of bounds
	// read, which is exactly the kind of thing this library refuses.
	EXPECT_EQ( nullptr, GrannyGetWorldPose4x4( pWorld, -1 ) );

	EXPECT_EQ( nullptr, GrannyNewWorldPose( -1 ) );
	GrannyFreeWorldPose( nullptr );

	GrannyFreeWorldPose( pWorld );
	GrannyFreeLocalPose( pLocal );
}
