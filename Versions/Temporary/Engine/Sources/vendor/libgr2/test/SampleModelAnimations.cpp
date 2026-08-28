// Sampling a model into a local pose.
//
// With no controls bound, which is where this port is, the answer is the
// skeleton's rest pose: each bone's LocalTransform copied verbatim, Flags
// included, and the instance clock makes no difference. That is not a
// placeholder for the real thing, it is the complete correct answer for the case
// with nothing playing, measured against granny2.dll rather than assumed. When
// controls exist, a blend accumulates on top of these transforms and the rest
// pose is what remains where no control reaches.
//
// Everything asserted here was probed against the DLL first, including the parts
// that look like error handling and are actually contract:
//
//   FirstBone indexes the skeleton and the pose identically. Sampling bones 1 and
//   2 of a four bone skeleton fills pose[1] and pose[2] and leaves pose[0] and
//   pose[3] alone.
//
//   A range that does not fit writes nothing at all, rather than as much of it as
//   fits. A skeleton of four asked for ten leaves the pose untouched, and so does
//   a pose of two asked for four.
//
// Not matched: a null pose and a null instance both access violate in the DLL,
// writing offset 0x10 and reading offset 8.
//
// Agreement on real skeletons is measured separately and covers far more than
// these hand built ones: scripts/port/gr2diff.py instantiates every model in the
// corpus and samples it through both implementations.

#include "LocalPose.h"
#include "ModelInstance.h"
#include "Structures.h"

#include <gr2/granny.h>

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

using namespace NGr2;

namespace
{

//! A skeleton whose rest pose is obviously neither zero nor identity, so
//! whichever one comes back is unmistakable.
struct SRestPoseModel
{
	explicit SRestPoseModel( int32_t nBones = 4 )
	{
		Bones.resize( static_cast<size_t>( nBones ) );
		for ( int32_t i = 0; i < nBones; ++i )
		{
			SBone &bone = Bones[static_cast<size_t>( i )];
			bone.pszName = "bone";
			bone.nParentIndex = i - 1;
			bone.LocalTransform.nFlags = 7;
			bone.LocalTransform.Position[0] = 10.0f + i;
			bone.LocalTransform.Position[1] = 20.0f + i;
			bone.LocalTransform.Position[2] = 30.0f + i;
			bone.LocalTransform.Orientation[3] = 1.0f;
			for ( int k = 0; k < 3; ++k )
			{
				bone.LocalTransform.ScaleShear[k][k] = 2.0f + i;
			}
		}

		Skeleton.pszName = "Skel";
		Skeleton.nBoneCount = nBones;
		Skeleton.pBones = Bones.data();

		Model.pszName = "model";
		Model.pSkeleton = &Skeleton;
		GrannyMakeIdentity(
			reinterpret_cast<granny_transform *>( &Model.InitialPlacement ) );
	}

	granny_model *Handle() { return reinterpret_cast<granny_model *>( &Model ); }

	std::vector<SBone> Bones;
	SSkeleton Skeleton = {};
	SModel Model = {};
};

bool IsZero( const granny_transform *pTransform )
{
	const STransform zero = {};
	return memcmp( pTransform, &zero, sizeof( zero ) ) == 0;
}

const STransform &At( granny_local_pose *pPose, granny_int32x i )
{
	return *reinterpret_cast<const STransform *>( GrannyGetLocalPoseTransform( pPose, i ) );
}

}

TEST( SampleModelAnimations, WithNothingBoundGivesTheRestPose )
{
	SRestPoseModel built;
	granny_model_instance *pInstance = GrannyInstantiateModel( built.Handle() );
	granny_local_pose *pPose = GrannyNewLocalPose( 4 );
	ASSERT_NE( nullptr, pInstance );
	ASSERT_NE( nullptr, pPose );

	GrannySampleModelAnimations( pInstance, 0, 4, pPose );

	for ( granny_int32x i = 0; i < 4; ++i )
	{
		const STransform &got = At( pPose, i );
		const STransform &want = built.Bones[static_cast<size_t>( i )].LocalTransform;
		EXPECT_EQ( 0, memcmp( &got, &want, sizeof( want ) ) )
			<< "bone " << i << " is not its rest transform";
		EXPECT_EQ( 7u, got.nFlags ) << "the flags come across too";
	}

	GrannyFreeLocalPose( pPose );
	GrannyFreeModelInstance( pInstance );
}

TEST( SampleModelAnimations, TheClockChangesNothingWhileNothingIsBound )
{
	SRestPoseModel built;
	granny_model_instance *pInstance = GrannyInstantiateModel( built.Handle() );
	granny_local_pose *pFirst = GrannyNewLocalPose( 4 );
	granny_local_pose *pLater = GrannyNewLocalPose( 4 );
	ASSERT_NE( nullptr, pInstance );

	GrannySampleModelAnimations( pInstance, 0, 4, pFirst );
	GrannySetModelClock( pInstance, 12.5f );
	GrannySampleModelAnimations( pInstance, 0, 4, pLater );

	for ( granny_int32x i = 0; i < 4; ++i )
	{
		EXPECT_EQ( 0, memcmp( &At( pFirst, i ), &At( pLater, i ), sizeof( STransform ) ) )
			<< "bone " << i;
	}

	GrannyFreeLocalPose( pFirst );
	GrannyFreeLocalPose( pLater );
	GrannyFreeModelInstance( pInstance );
}

TEST( SampleModelAnimations, FirstBoneIndexesThePoseAsWellAsTheSkeleton )
{
	// The measured behaviour, and the one that could plausibly have gone the other
	// way: bones 1 and 2 land at pose[1] and pose[2], not at pose[0] and pose[1].
	SRestPoseModel built;
	granny_model_instance *pInstance = GrannyInstantiateModel( built.Handle() );
	granny_local_pose *pPose = GrannyNewLocalPose( 4 );
	ASSERT_NE( nullptr, pInstance );

	GrannySampleModelAnimations( pInstance, 1, 2, pPose );

	EXPECT_TRUE( IsZero( GrannyGetLocalPoseTransform( pPose, 0 ) ) ) << "before the range";
	EXPECT_FLOAT_EQ( 11.0f, At( pPose, 1 ).Position[0] );
	EXPECT_FLOAT_EQ( 12.0f, At( pPose, 2 ).Position[0] );
	EXPECT_TRUE( IsZero( GrannyGetLocalPoseTransform( pPose, 3 ) ) ) << "after the range";

	GrannyFreeLocalPose( pPose );
	GrannyFreeModelInstance( pInstance );
}

TEST( SampleModelAnimations, ARangeThatDoesNotFitWritesNothing )
{
	// Not "as much as fits", which is what a careless implementation would do and
	// what would leave a half filled pose behind. Measured both ways round.
	SRestPoseModel built;
	granny_model_instance *pInstance = GrannyInstantiateModel( built.Handle() );
	ASSERT_NE( nullptr, pInstance );

	{
		// More bones than the skeleton has.
		granny_local_pose *pPose = GrannyNewLocalPose( 10 );
		GrannySampleModelAnimations( pInstance, 0, 10, pPose );
		for ( granny_int32x i = 0; i < 10; ++i )
		{
			EXPECT_TRUE( IsZero( GrannyGetLocalPoseTransform( pPose, i ) ) ) << i;
		}
		GrannyFreeLocalPose( pPose );
	}
	{
		// A pose smaller than the range.
		granny_local_pose *pPose = GrannyNewLocalPose( 2 );
		GrannySampleModelAnimations( pInstance, 0, 4, pPose );
		EXPECT_TRUE( IsZero( GrannyGetLocalPoseTransform( pPose, 0 ) ) );
		EXPECT_TRUE( IsZero( GrannyGetLocalPoseTransform( pPose, 1 ) ) );
		GrannyFreeLocalPose( pPose );
	}
	{
		// A first bone past the end.
		granny_local_pose *pPose = GrannyNewLocalPose( 10 );
		GrannySampleModelAnimations( pInstance, 6, 2, pPose );
		for ( granny_int32x i = 0; i < 10; ++i )
		{
			EXPECT_TRUE( IsZero( GrannyGetLocalPoseTransform( pPose, i ) ) ) << i;
		}
		GrannyFreeLocalPose( pPose );
	}

	GrannyFreeModelInstance( pInstance );
}

TEST( SampleModelAnimations, AnEmptyRangeIsHarmless )
{
	SRestPoseModel built;
	granny_model_instance *pInstance = GrannyInstantiateModel( built.Handle() );
	granny_local_pose *pPose = GrannyNewLocalPose( 4 );
	ASSERT_NE( nullptr, pInstance );

	GrannySampleModelAnimations( pInstance, 0, 0, pPose );
	EXPECT_TRUE( IsZero( GrannyGetLocalPoseTransform( pPose, 0 ) ) );

	GrannyFreeLocalPose( pPose );
	GrannyFreeModelInstance( pInstance );
}

TEST( SampleModelAnimations, SurvivesWhatTheRealDllDoesNot )
{
	SRestPoseModel built;
	granny_model_instance *pInstance = GrannyInstantiateModel( built.Handle() );
	granny_local_pose *pPose = GrannyNewLocalPose( 4 );
	ASSERT_NE( nullptr, pInstance );

	// Both access violate in granny2.dll. Neither is reproduced.
	GrannySampleModelAnimations( pInstance, 0, 4, nullptr );
	GrannySampleModelAnimations( nullptr, 0, 4, pPose );
	EXPECT_TRUE( IsZero( GrannyGetLocalPoseTransform( pPose, 0 ) ) )
		<< "a refused call left the pose alone";

	// And a negative first bone, which the DLL happens to survive by writing
	// nothing; the same answer here, by checking rather than by luck.
	GrannySampleModelAnimations( pInstance, -1, 2, pPose );
	EXPECT_TRUE( IsZero( GrannyGetLocalPoseTransform( pPose, 0 ) ) );

	GrannyFreeLocalPose( pPose );
	GrannyFreeModelInstance( pInstance );
}

TEST( SampleModelAnimations, SamplingTwiceGivesTheSamePose )
{
	// Nothing accumulates while nothing is bound, so a second call into a pose
	// that already holds the rest pose must not double anything.
	SRestPoseModel built;
	granny_model_instance *pInstance = GrannyInstantiateModel( built.Handle() );
	granny_local_pose *pPose = GrannyNewLocalPose( 4 );
	ASSERT_NE( nullptr, pInstance );

	GrannySampleModelAnimations( pInstance, 0, 4, pPose );
	const STransform first = At( pPose, 2 );
	GrannySampleModelAnimations( pInstance, 0, 4, pPose );
	EXPECT_EQ( 0, memcmp( &first, &At( pPose, 2 ), sizeof( first ) ) );

	GrannyFreeLocalPose( pPose );
	GrannyFreeModelInstance( pInstance );
}
