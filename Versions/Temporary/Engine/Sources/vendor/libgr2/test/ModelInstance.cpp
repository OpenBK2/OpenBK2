// Model instances, and the query entry points beside them.
//
// The instance is opaque, so almost nothing about it is observable through the
// 54 entry points. What is observable is the lifecycle and the edges, and those
// were probed against granny2.dll first: it references the model rather than
// copying it, it returns null for a model with no skeleton, it tolerates
// FreeModelInstance(null), and it access violates on a null model and on
// SetModelClock(null). The last two are deliberately not reproduced.
//
// The models here are built by hand rather than loaded, because that is what the
// engine does: CSkeletonAnimator keeps a granny_model as a member, fills in a
// skeleton borrowed from a file and no mesh bindings, and instantiates that.

#include "MinimalGr2.h"

#include "ModelInstance.h"
#include "Structures.h"

#include <gr2/granny.h>

#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace NGr2;

namespace
{

//! A skeleton and a model the way CSkeletonAnimator builds them.
struct SHandBuiltModel
{
	SHandBuiltModel()
	{
		Bones.resize( 2 );
		Bones[0].pszName = "Root";
		Bones[0].nParentIndex = -1;
		Bones[1].pszName = "joint1";
		Bones[1].nParentIndex = 0;
		for ( SBone &bone : Bones )
		{
			GrannyMakeIdentity( reinterpret_cast<granny_transform *>( &bone.LocalTransform ) );
		}

		Skeleton.pszName = "Skel";
		Skeleton.nBoneCount = static_cast<int32_t>( Bones.size() );
		Skeleton.pBones = Bones.data();

		Model.pszName = "model";
		Model.pSkeleton = &Skeleton;
		Model.nMeshBindingCount = 0;
		Model.pMeshBindings = nullptr;
		GrannyMakeIdentity(
			reinterpret_cast<granny_transform *>( &Model.InitialPlacement ) );
	}

	granny_model *Handle() { return reinterpret_cast<granny_model *>( &Model ); }

	std::vector<SBone> Bones;
	SSkeleton Skeleton = {};
	SModel Model = {};
};

}

TEST( ModelInstance, InstantiatesAHandBuiltModel )
{
	SHandBuiltModel built;
	granny_model_instance *pInstance = GrannyInstantiateModel( built.Handle() );
	ASSERT_NE( nullptr, pInstance );

	// Referenced, not copied. The engine's model is a member of the object that
	// owns the instance, and Granny's own GrannyGetSourceModel returns the address
	// it was given.
	EXPECT_EQ( &built.Model, pInstance->pModel );
	EXPECT_EQ( &built.Skeleton, pInstance->pModel->pSkeleton );
	EXPECT_FLOAT_EQ( 0.0f, pInstance->fClock ) << "a new instance starts at zero";

	GrannyFreeModelInstance( pInstance );
}

TEST( ModelInstance, SeesChangesToTheModelBehindIt )
{
	// Because it references rather than copies. Measured on the DLL by changing a
	// skeleton after instantiating and asking for it again.
	SHandBuiltModel built;
	granny_model_instance *pInstance = GrannyInstantiateModel( built.Handle() );
	ASSERT_NE( nullptr, pInstance );

	built.Skeleton.nBoneCount = 1;
	EXPECT_EQ( 1, pInstance->pModel->pSkeleton->nBoneCount );

	GrannyFreeModelInstance( pInstance );
}

TEST( ModelInstance, TwoInstancesOfOneModelAreDistinct )
{
	SHandBuiltModel built;
	granny_model_instance *pFirst = GrannyInstantiateModel( built.Handle() );
	granny_model_instance *pSecond = GrannyInstantiateModel( built.Handle() );
	ASSERT_NE( nullptr, pFirst );
	ASSERT_NE( nullptr, pSecond );
	EXPECT_NE( pFirst, pSecond );

	// And their clocks move independently, which is the whole point of an
	// instance: one model, many things animating out of step.
	GrannySetModelClock( pFirst, 1.5f );
	GrannySetModelClock( pSecond, 9.25f );
	EXPECT_FLOAT_EQ( 1.5f, pFirst->fClock );
	EXPECT_FLOAT_EQ( 9.25f, pSecond->fClock );

	GrannyFreeModelInstance( pFirst );
	GrannyFreeModelInstance( pSecond );
}

TEST( ModelInstance, RefusesAModelWithNoSkeleton )
{
	// The real DLL checks this one and returns null, which is worth matching
	// rather than guessing: a skeleton-less model is what a half converted file
	// would produce.
	SHandBuiltModel built;
	built.Model.pSkeleton = nullptr;
	EXPECT_EQ( nullptr, GrannyInstantiateModel( built.Handle() ) );
}

TEST( ModelInstance, SurvivesNullWhereTheRealDllDoesNot )
{
	// granny2.dll access violates on both of these: InstantiateModel reading
	// offset 8 of a null model, SetModelClock reading offset 0x48 of a null
	// instance. Not reproduced. Being bug compatible with a crash buys nothing,
	// and the engine's resource path already turns a failed load into a crash
	// without any help.
	EXPECT_EQ( nullptr, GrannyInstantiateModel( nullptr ) );
	GrannySetModelClock( nullptr, 1.0f );
	GrannyFreeModelInstance( nullptr );
}

TEST( ModelInstance, TheClockIsWhateverItWasLastSet )
{
	SHandBuiltModel built;
	granny_model_instance *pInstance = GrannyInstantiateModel( built.Handle() );
	ASSERT_NE( nullptr, pInstance );

	for ( float fClock : { 0.0f, 1.0f, 0.001f, -2.5f, 1000.0f } )
	{
		GrannySetModelClock( pInstance, fClock );
		EXPECT_FLOAT_EQ( fClock, pInstance->fClock );
	}

	// The engine sets it from an integer millisecond count scaled by 0.001, so a
	// negative or very large value is not a case worth refusing.
	GrannyFreeModelInstance( pInstance );
}
