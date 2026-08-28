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
// GrannyEvaluateCurveAtT sits here rather than in a file of its own because it
// is the same sampler that GrannySampleModelAnimations drives internally. The
// engine calls it directly as well, to read named scalar channels out of a clip.

#include <gr2/granny.h>

#include "LocalPose.h"
#include "Structures.h"
#include "Trace.h"

#include <new>

using namespace NGr2;

//! No skeleton comes near this. It exists so a corrupt or wild bone count cannot
//! ask for a hundred gigabytes before anything notices; the largest skeleton in
//! the corpus has 134 bones.
static const granny_int32x MAX_POSE_BONES = 1 << 20;

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
	GR2_STUB( "BoneCount={}", BoneCount );
	return 0;
}

GR2_API( void ) GrannyFreeWorldPose( granny_world_pose *WorldPose )
{
	GR2_STUB( "WorldPose={}", WorldPose );
}

GR2_API( void ) GrannyBuildWorldPose( granny_skeleton const *Skeleton, granny_int32x FirstBone,
                                      granny_int32x BoneCount, granny_local_pose const *LocalPose,
                                      granny_real32 const *Offset4x4, granny_world_pose *Result )
{
	GR2_STUB( "Skeleton={} FirstBone={} BoneCount={} LocalPose={} Offset4x4={} Result={}",
	           Skeleton, FirstBone, BoneCount, LocalPose, Offset4x4, Result );
}

GR2_API( granny_real32 * ) GrannyGetWorldPose4x4( granny_world_pose const *WorldPose,
                                                  granny_int32x BoneIndex )
{
	GR2_STUB( "WorldPose={} BoneIndex={}", WorldPose, BoneIndex );
	return 0;
}

GR2_API( granny_real32 * ) GrannyGetWorldPoseComposite4x4( granny_world_pose const *WorldPose,
                                                           granny_int32x BoneIndex )
{
	GR2_STUB( "WorldPose={} BoneIndex={}", WorldPose, BoneIndex );
	return 0;
}

GR2_API( void ) GrannySampleModelAnimations( granny_model_instance const *ModelInstance,
                                             granny_int32x FirstBone, granny_int32x BoneCount,
                                             granny_local_pose *Result )
{
	GR2_STUB( "ModelInstance={} FirstBone={} BoneCount={} Result={}",
	           ModelInstance, FirstBone, BoneCount, Result );
}

GR2_API( void ) GrannyEvaluateCurveAtT( granny_int32x Dimension, bool Normalize, bool BackwardsLoop,
                                        granny_curve2 const *Curve, bool ForwardsLoop,
                                        granny_real32 CurveDuration, granny_real32 t,
                                        granny_real32 *Result, granny_real32 const *IdentityVector )
{
	GR2_STUB( "Dimension={} Normalize={} BackwardsLoop={} Curve={} ForwardsLoop={} "
	           "CurveDuration={} t={} Result={} IdentityVector={}",
	           Dimension, Normalize, BackwardsLoop, Curve, ForwardsLoop, CurveDuration, t, Result,
	           IdentityVector );
}

}
