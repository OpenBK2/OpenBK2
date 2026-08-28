// Curve sampling, local pose, world pose, and the composite skinning matrices
// the renderer consumes.
//
// M3. This is the arithmetic core: sample every active control's curves at the
// instance clock, blend them by weight into a local pose of one transform per
// bone, run the skeleton hierarchy to get world matrices, and multiply through
// each bone's inverse world matrix to get the composite the skinning path wants.
//
// Two things make it smaller than the general case. This game's curves are the
// pre-curve2 legacy layout everywhere, so the sampler covers six cases rather
// than the full modern set. And animation is presentation-only in this engine:
// nothing here feeds back into AILogic, so this has to be visually right, not
// bit-exact against granny2.dll.
//
// GrannyEvaluateCurveAtT is next door in Curve.cpp. It is the same sampler this
// file drives internally once controls exist, and the engine calls it directly
// as well, to read named scalar channels out of a clip.

#include <gr2/granny.h>

#include "LocalPose.h"
#include "ModelInstance.h"
#include "Structures.h"
#include "Trace.h"
#include "WorldPose.h"

#include <cstdint>
#include <new>

using namespace NGr2;

//! No skeleton comes near this. It exists so a corrupt or wild bone count cannot
//! ask for a hundred gigabytes before anything notices; the largest skeleton in
//! the corpus has 134 bones.
static const granny_int32x MAX_POSE_BONES = 1 << 20;

namespace
{

//! A granny_transform as a row vector 4x4, translation in elements 12 to 14.
//!
//! Flags select the parts, and not as a bitmask, which is where a first version
//! of this went wrong. Measured over all eight values with a position, a rotation
//! and a scale all set and distinguishable:
//!
//!     0      identity
//!     1      position only
//!     2, 3   position and rotation
//!     4..7   position, rotation and scale
//!
//! So the DLL dispatches on the highest capability present and the branch
//! includes everything below it; a scale bit brings the rotation with it even
//! when the orientation bit is clear, and any non-zero value brings the position.
//! Reading the bits independently looks right, passes a test that only tries 0 and
//! 7, and drops the position of every bone whose flags are 2. The corpus found it
//! in files whose root bone is exactly that.
void ToMatrix4x4( const STransform &transform, float pResult[16] )
{
	// The upper 3x3 is ( R * S ) transposed, since this is stored for row vectors
	// and Transform.cpp composes for columns. Measured: a 90 degree turn about Z
	// with a diag(2,3,4) scale gives rows (0,2,0), (-3,0,0), (0,0,4).
	const bool bScale = ( transform.nFlags & TRANSFORM_HAS_SCALESHEAR ) != 0;
	const bool bRotate = bScale || ( transform.nFlags & TRANSFORM_HAS_ORIENTATION ) != 0;
	const bool bTranslate = transform.nFlags != 0;

	float rotation[3][3] = { { 1.0f, 0.0f, 0.0f },
	                         { 0.0f, 1.0f, 0.0f },
	                         { 0.0f, 0.0f, 1.0f } };
	if ( bRotate )
	{
		const float x = transform.Orientation[0];
		const float y = transform.Orientation[1];
		const float z = transform.Orientation[2];
		const float w = transform.Orientation[3];
		rotation[0][0] = 1.0f - 2.0f * ( y * y + z * z );
		rotation[0][1] = 2.0f * ( x * y - w * z );
		rotation[0][2] = 2.0f * ( x * z + w * y );
		rotation[1][0] = 2.0f * ( x * y + w * z );
		rotation[1][1] = 1.0f - 2.0f * ( x * x + z * z );
		rotation[1][2] = 2.0f * ( y * z - w * x );
		rotation[2][0] = 2.0f * ( x * z - w * y );
		rotation[2][1] = 2.0f * ( y * z + w * x );
		rotation[2][2] = 1.0f - 2.0f * ( x * x + y * y );
	}

	float linear[3][3];
	if ( bScale )
	{
		for ( int r = 0; r < 3; ++r )
		{
			for ( int c = 0; c < 3; ++c )
			{
				linear[r][c] = rotation[r][0] * transform.ScaleShear[0][c]
				               + rotation[r][1] * transform.ScaleShear[1][c]
				               + rotation[r][2] * transform.ScaleShear[2][c];
			}
		}
	}
	else
	{
		memcpy( linear, rotation, sizeof( linear ) );
	}

	for ( int r = 0; r < 3; ++r )
	{
		for ( int c = 0; c < 3; ++c )
		{
			pResult[r * 4 + c] = linear[c][r];
		}
		pResult[r * 4 + 3] = 0.0f;
	}

	pResult[12] = bTranslate ? transform.Position[0] : 0.0f;
	pResult[13] = bTranslate ? transform.Position[1] : 0.0f;
	pResult[14] = bTranslate ? transform.Position[2] : 0.0f;
	pResult[15] = 1.0f;
}

//! pResult = pLeft * pRight, row major, safe when pResult aliases an input.
void Multiply4x4( const float pLeft[16], const float pRight[16], float pResult[16] )
{
	float product[16];
	for ( int r = 0; r < 4; ++r )
	{
		for ( int c = 0; c < 4; ++c )
		{
			product[r * 4 + c] = pLeft[r * 4 + 0] * pRight[0 * 4 + c]
			                     + pLeft[r * 4 + 1] * pRight[1 * 4 + c]
			                     + pLeft[r * 4 + 2] * pRight[2 * 4 + c]
			                     + pLeft[r * 4 + 3] * pRight[3 * 4 + c];
		}
	}
	memcpy( pResult, product, sizeof( product ) );
}

const float IDENTITY_4X4[16] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

}

extern "C"
{

GR2_API( granny_local_pose * ) GrannyNewLocalPose( granny_int32x BoneCount )
{
	GR2_TRACE( "BoneCount={}", BoneCount );

	// A negative count access violates in the real DLL. Not reproduced: this port
	// does not emulate the original's undefined behaviour, only its defined
	// behaviour. Zero is defined and does work there, giving an empty pose that
	// reports a bone count of zero, so that case is matched rather than refused.
	if ( BoneCount < 0 || BoneCount > MAX_POSE_BONES )
	{
		Logger().warn( "NewLocalPose: {} bones is not a count", BoneCount );
		return 0;
	}

	granny_local_pose *pPose = new ( std::nothrow ) granny_local_pose;
	if ( pPose == nullptr )
	{
		return 0;
	}

	// Zeroed, which is what the DLL leaves: a fresh pose has a (0,0,0,0)
	// orientation rather than an identity one. See LocalPose.h.
	pPose->Transforms.assign( static_cast<size_t>( BoneCount ), STransform() );
	return pPose;
}

GR2_API( void ) GrannyFreeLocalPose( granny_local_pose *LocalPose )
{
	GR2_TRACE( "LocalPose={}", LocalPose );

	// Null is safe in the real DLL too, and CSkeletonAnimator's destructor guards
	// it anyway.
	delete LocalPose;
}

GR2_API( granny_int32x ) GrannyGetLocalPoseBoneCount( granny_local_pose const *LocalPose )
{
	GR2_TRACE( "LocalPose={}", LocalPose );

	// Null access violates in the real DLL. Not reproduced.
	if ( LocalPose == 0 )
	{
		return 0;
	}
	return static_cast<granny_int32x>( LocalPose->Transforms.size() );
}

GR2_API( granny_transform * ) GrannyGetLocalPoseTransform( granny_local_pose const *LocalPose,
                                                           granny_int32x BoneIndex )
{
	GR2_TRACE( "LocalPose={} BoneIndex={}", LocalPose, BoneIndex );

	// Null access violates in the real DLL. Not reproduced.
	if ( LocalPose == 0 )
	{
		return 0;
	}

	// The range check is the DLL's own, measured: it returns null for an index at
	// or past the bone count and for a negative one. That is not a courtesy to
	// match optionally, it is the contract, because GAnimation.cpp writes
	//
	//     granny_transform *pBoneTransform = GrannyGetLocalPoseTransform( pose, i );
	//     if ( pBoneTransform ) { ... }
	//
	// and treats null as "this bone is not in the pose".
	if ( BoneIndex < 0
	     || static_cast<size_t>( BoneIndex ) >= LocalPose->Transforms.size() )
	{
		return 0;
	}

	// const in Granny's signature, and the caller writes through the result: the
	// engine sets Position, Orientation and Flags on what this returns. The const
	// is about the pose's shape rather than its contents, and the cast stays here
	// rather than becoming a mutable alias in the header.
	granny_local_pose *pPose = const_cast<granny_local_pose *>( LocalPose );
	return reinterpret_cast<granny_transform *>(
		&pPose->Transforms[static_cast<size_t>( BoneIndex )] );
}

GR2_API( granny_world_pose * ) GrannyNewWorldPose( granny_int32x BoneCount )
{
	GR2_TRACE( "BoneCount={}", BoneCount );

	// A negative count returns null in the real DLL, which is one of the few
	// edges it does check. Zero is accepted there and here.
	if ( BoneCount < 0 || BoneCount > MAX_POSE_BONES )
	{
		Logger().warn( "NewWorldPose: {} bones is not a count", BoneCount );
		return 0;
	}

	granny_world_pose *pPose = new ( std::nothrow ) granny_world_pose;
	if ( pPose == nullptr )
	{
		return 0;
	}
	pPose->World.assign( static_cast<size_t>( BoneCount ) * 16, 0.0f );
	pPose->Composite.assign( static_cast<size_t>( BoneCount ) * 16, 0.0f );
	return pPose;
}

GR2_API( void ) GrannyFreeWorldPose( granny_world_pose *WorldPose )
{
	GR2_TRACE( "WorldPose={}", WorldPose );

	// Null is safe in the real DLL too.
	delete WorldPose;
}

GR2_API( void ) GrannyBuildWorldPose( granny_skeleton const *Skeleton, granny_int32x FirstBone,
                                      granny_int32x BoneCount, granny_local_pose const *LocalPose,
                                      granny_real32 const *Offset4x4, granny_world_pose *Result )
{
	GR2_TRACE( "Skeleton={} FirstBone={} BoneCount={} LocalPose={} Offset4x4={} Result={}",
	           Skeleton, FirstBone, BoneCount, LocalPose, Offset4x4, Result );

	// A null result access violates in the real DLL. Not reproduced.
	if ( Skeleton == 0 || LocalPose == 0 || Result == 0 )
	{
		return;
	}

	const SSkeleton *pSkeleton = reinterpret_cast<const SSkeleton *>( Skeleton );
	const int64_t nLast = int64_t( FirstBone ) + BoneCount;

	// The range has to fit all three: the skeleton, the local pose it reads and
	// the world pose it writes. The last of those is not the same size as the
	// others in practice, since CAddBoneFilter::Recalc allocates a world pose of
	// nAddBone + 1 for a full sized skeleton and builds only that far.
	if ( FirstBone < 0 || BoneCount < 0 || nLast > pSkeleton->nBoneCount
	     || nLast > static_cast<int64_t>( LocalPose->Transforms.size() )
	     || nLast > static_cast<int64_t>( Result->BoneCount() ) )
	{
		Logger().warn( "BuildWorldPose: bones {}..{} of a skeleton of {}, a local pose "
		               "of {} and a world pose of {}",
		               FirstBone, nLast, pSkeleton->nBoneCount,
		               LocalPose->Transforms.size(), Result->BoneCount() );
		return;
	}

	// Null is treated as the identity, which the real DLL does too.
	const float *pOffset = Offset4x4 != 0 ? Offset4x4 : IDENTITY_4X4;

	for ( granny_int32x i = FirstBone; i < FirstBone + BoneCount; ++i )
	{
		float local[16];
		ToMatrix4x4( LocalPose->Transforms[static_cast<size_t>( i )], local );

		// Up the chain, with the offset at the far end: a root's world matrix is
		// its local times the offset, and every child inherits it. Measured with
		// an offset that translates by (1000, 2000, 3000), which appears in every
		// bone's world matrix and once only.
		const int32_t nParent = pSkeleton->pBones[i].nParentIndex;
		float *pWorld = &Result->World[static_cast<size_t>( i ) * 16];
		if ( nParent >= FirstBone && nParent < i )
		{
			Multiply4x4( local, &Result->World[static_cast<size_t>( nParent ) * 16],
			             pWorld );
		}
		else
		{
			// No parent, or a parent outside the range this call built. The DLL
			// reads the unbuilt entry in the second case, which is uninitialised
			// memory; treating it as a root is deterministic instead. The engine
			// always passes FirstBone 0, so this never arises there.
			Multiply4x4( local, pOffset, pWorld );
		}

		// The skinning matrix: undo the bind pose, then apply the current world.
		// Measured as InverseWorld4x4 times World, in that order.
		Multiply4x4( pSkeleton->pBones[i].InverseWorld4x4, pWorld,
		             &Result->Composite[static_cast<size_t>( i ) * 16] );
	}
}

//! Shared by the two accessors below, which differ only in which array they read.
static granny_real32 *WorldPoseMatrix( granny_world_pose const *WorldPose,
                                       granny_int32x BoneIndex, bool bComposite )
{
	// A null pose access violates in the real DLL. Not reproduced.
	if ( WorldPose == 0 )
	{
		return 0;
	}
	// Past the end returns null in the DLL, which CAddBoneFilter relies on. A
	// negative index does not: it returns a pointer, which is an out of bounds
	// read and is one of the things this library refuses rather than copies.
	if ( BoneIndex < 0 || static_cast<uint32_t>( BoneIndex ) >= WorldPose->BoneCount() )
	{
		return 0;
	}

	// const in Granny's signature and the caller may write through it, the same
	// oddity as GrannyGetLocalPoseTransform.
	granny_world_pose *pPose = const_cast<granny_world_pose *>( WorldPose );
	std::vector<float> &matrices = bComposite ? pPose->Composite : pPose->World;
	return &matrices[static_cast<size_t>( BoneIndex ) * 16];
}

GR2_API( granny_real32 * ) GrannyGetWorldPose4x4( granny_world_pose const *WorldPose,
                                                  granny_int32x BoneIndex )
{
	GR2_TRACE( "WorldPose={} BoneIndex={}", WorldPose, BoneIndex );
	return WorldPoseMatrix( WorldPose, BoneIndex, false );
}

GR2_API( granny_real32 * ) GrannyGetWorldPoseComposite4x4( granny_world_pose const *WorldPose,
                                                           granny_int32x BoneIndex )
{
	GR2_TRACE( "WorldPose={} BoneIndex={}", WorldPose, BoneIndex );
	return WorldPoseMatrix( WorldPose, BoneIndex, true );
}

GR2_API( void ) GrannySampleModelAnimations( granny_model_instance const *ModelInstance,
                                             granny_int32x FirstBone, granny_int32x BoneCount,
                                             granny_local_pose *Result )
{
	GR2_TRACE( "ModelInstance={} FirstBone={} BoneCount={} Result={}", ModelInstance,
	           FirstBone, BoneCount, Result );

	// Both of these access violate in the real DLL, writing offset 0x10 of a null
	// pose and reading offset 8 of a null instance. Not reproduced.
	if ( ModelInstance == 0 || Result == 0 )
	{
		return;
	}
	if ( ModelInstance->pModel == nullptr
	     || ModelInstance->pModel->pSkeleton == nullptr )
	{
		return;
	}

	const SSkeleton *pSkeleton = ModelInstance->pModel->pSkeleton;
	const int64_t nLast = int64_t( FirstBone ) + BoneCount;

	// A request that does not fit writes nothing at all, rather than as much of it
	// as fits. Measured: a skeleton of four asked for ten bones leaves the pose
	// untouched, and so does a pose of two asked for four. FirstBone indexes the
	// skeleton and the pose identically, so one range check covers both.
	if ( FirstBone < 0 || BoneCount < 0 || nLast > pSkeleton->nBoneCount
	     || nLast > static_cast<int64_t>( Result->Transforms.size() ) )
	{
		Logger().warn( "SampleModelAnimations: bones {}..{} of a skeleton of {} into a "
		               "pose of {}",
		               FirstBone, nLast, pSkeleton->nBoneCount,
		               Result->Transforms.size() );
		return;
	}

	// The rest pose, and with no controls bound that is the whole answer rather
	// than a placeholder for one. Measured against the DLL: with nothing playing
	// it copies each bone's LocalTransform verbatim, Flags included, and the
	// instance clock makes no difference.
	//
	// This is where the blend goes when controls exist. Each active control
	// samples its animation's curves at the instance clock and accumulates into
	// these transforms by weight, and the rest pose is what remains where no
	// control reaches. Nothing here has to change for that; it grows a loop after
	// this one.
	for ( granny_int32x i = FirstBone; i < FirstBone + BoneCount; ++i )
	{
		Result->Transforms[static_cast<size_t>( i )] =
			pSkeleton->pBones[i].LocalTransform;
	}
}

}
