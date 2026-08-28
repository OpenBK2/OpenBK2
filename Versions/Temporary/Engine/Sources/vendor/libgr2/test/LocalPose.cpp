// The local pose: one transform per bone, allocated by the engine and filled in
// by the sampler.
//
// Every behaviour asserted here was probed against granny2.dll first, and the
// split between what is matched and what is not is the standing rule this
// library follows: reproduce the original's defined behaviour, refuse its
// undefined behaviour. Crashes, hangs and out of bounds reads are not a contract
// worth honouring, and a caller that relied on one was already broken.
//
//   matched      GetLocalPoseTransform returns null outside the range, which the
//                engine depends on; NewLocalPose(0) gives an empty pose that
//                reports zero; FreeLocalPose tolerates null; a fresh pose is
//                zeroed rather than set to identity.
//   not matched  NewLocalPose(-1), GetLocalPoseBoneCount(null) and
//                GetLocalPoseTransform(null, 0) all access violate in the DLL.
//                Here they return a null, a zero and a null.
//
// The one thing deliberately different in shape rather than behaviour: the DLL
// spaces its transforms 80 bytes apart, a granny_transform rounded up to sixteen.
// That is invisible through the accessor and is not reproduced.

#include "LocalPose.h"
#include "Structures.h"

#include <gr2/granny.h>

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

using namespace NGr2;

TEST( LocalPose, HoldsTheBonesItWasAskedFor )
{
	for ( granny_int32x nBones : { 0, 1, 2, 10, 134 } )
	{
		granny_local_pose *pPose = GrannyNewLocalPose( nBones );
		ASSERT_NE( nullptr, pPose ) << nBones << " bones";
		EXPECT_EQ( nBones, GrannyGetLocalPoseBoneCount( pPose ) );
		GrannyFreeLocalPose( pPose );
	}
}

TEST( LocalPose, StartsZeroedRatherThanAtIdentity )
{
	// The DLL's behaviour, matched because it costs nothing and because a caller
	// that noticed the difference would be reading a bone it never wrote. Note
	// that a zeroed orientation is (0,0,0,0), which is not a rotation: every bone
	// is expected to be written by the sampler before anything reads it.
	granny_local_pose *pPose = GrannyNewLocalPose( 4 );
	ASSERT_NE( nullptr, pPose );

	const STransform zero = {};
	for ( granny_int32x i = 0; i < 4; ++i )
	{
		const granny_transform *pTransform = GrannyGetLocalPoseTransform( pPose, i );
		ASSERT_NE( nullptr, pTransform ) << "bone " << i;
		EXPECT_EQ( 0, memcmp( pTransform, &zero, sizeof( zero ) ) ) << "bone " << i;
	}
	GrannyFreeLocalPose( pPose );
}

TEST( LocalPose, EachBoneIsItsOwnTransform )
{
	// The engine writes through the accessor, one bone at a time, so writing one
	// must not disturb its neighbours.
	granny_local_pose *pPose = GrannyNewLocalPose( 5 );
	ASSERT_NE( nullptr, pPose );

	for ( granny_int32x i = 0; i < 5; ++i )
	{
		granny_transform *pTransform = GrannyGetLocalPoseTransform( pPose, i );
		ASSERT_NE( nullptr, pTransform );
		GrannyMakeIdentity( pTransform );
		reinterpret_cast<STransform *>( pTransform )->Position[0] =
			static_cast<float>( i );
	}

	for ( granny_int32x i = 0; i < 5; ++i )
	{
		const STransform *pTransform = reinterpret_cast<const STransform *>(
			GrannyGetLocalPoseTransform( pPose, i ) );
		ASSERT_NE( nullptr, pTransform );
		EXPECT_FLOAT_EQ( static_cast<float>( i ), pTransform->Position[0] );
		EXPECT_FLOAT_EQ( 1.0f, pTransform->Orientation[3] );
	}

	// And the same index hands back the same address every time, since the engine
	// keeps no pointer between calls but the sampler will.
	EXPECT_EQ( GrannyGetLocalPoseTransform( pPose, 2 ),
	           GrannyGetLocalPoseTransform( pPose, 2 ) );

	GrannyFreeLocalPose( pPose );
}

TEST( LocalPose, RefusesAnIndexOutsideTheRange )
{
	// Matched from the DLL, and not optional: GAnimation.cpp asks for a transform
	// and treats null as "this bone is not in the pose".
	granny_local_pose *pPose = GrannyNewLocalPose( 4 );
	ASSERT_NE( nullptr, pPose );

	EXPECT_NE( nullptr, GrannyGetLocalPoseTransform( pPose, 3 ) );
	EXPECT_EQ( nullptr, GrannyGetLocalPoseTransform( pPose, 4 ) ) << "one past the end";
	EXPECT_EQ( nullptr, GrannyGetLocalPoseTransform( pPose, -1 ) );
	EXPECT_EQ( nullptr, GrannyGetLocalPoseTransform( pPose, 9999 ) );

	GrannyFreeLocalPose( pPose );
}

TEST( LocalPose, AnEmptyPoseIsValidAndHasNoBones )
{
	// NewLocalPose(0) is defined in the DLL and returns a usable handle.
	granny_local_pose *pPose = GrannyNewLocalPose( 0 );
	ASSERT_NE( nullptr, pPose );
	EXPECT_EQ( 0, GrannyGetLocalPoseBoneCount( pPose ) );
	EXPECT_EQ( nullptr, GrannyGetLocalPoseTransform( pPose, 0 ) );
	GrannyFreeLocalPose( pPose );
}

TEST( LocalPose, SurvivesWhatTheRealDllDoesNot )
{
	// Three access violations in granny2.dll, none reproduced here.
	EXPECT_EQ( nullptr, GrannyNewLocalPose( -1 ) );
	EXPECT_EQ( 0, GrannyGetLocalPoseBoneCount( nullptr ) );
	EXPECT_EQ( nullptr, GrannyGetLocalPoseTransform( nullptr, 0 ) );

	// And one the DLL does handle, matched.
	GrannyFreeLocalPose( nullptr );
}

TEST( LocalPose, RefusesACountThatIsNotACount )
{
	// A wild bone count reaching this would otherwise be an allocation the size of
	// the address space. The largest skeleton in the corpus has 134 bones.
	EXPECT_EQ( nullptr, GrannyNewLocalPose( -1000000 ) );
	EXPECT_EQ( nullptr, GrannyNewLocalPose( 0x7fffffff ) );
}

TEST( LocalPose, PosesAreIndependent )
{
	granny_local_pose *pFirst = GrannyNewLocalPose( 3 );
	granny_local_pose *pSecond = GrannyNewLocalPose( 3 );
	ASSERT_NE( nullptr, pFirst );
	ASSERT_NE( nullptr, pSecond );
	EXPECT_NE( pFirst, pSecond );

	GrannyMakeIdentity( GrannyGetLocalPoseTransform( pFirst, 0 ) );
	reinterpret_cast<STransform *>( GrannyGetLocalPoseTransform( pFirst, 0 ) )
		->Position[1] = 7.0f;

	const STransform *pOther = reinterpret_cast<const STransform *>(
		GrannyGetLocalPoseTransform( pSecond, 0 ) );
	EXPECT_FLOAT_EQ( 0.0f, pOther->Position[1] ) << "the other pose was untouched";

	GrannyFreeLocalPose( pFirst );
	GrannyFreeLocalPose( pSecond );
}
