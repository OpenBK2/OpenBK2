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

#include "Trace.h"

extern "C"
{

GR2_API( granny_local_pose * ) GrannyNewLocalPose( granny_int32x BoneCount )
{
	GR2_STUB( "BoneCount={}", BoneCount );
	return 0;
}

GR2_API( void ) GrannyFreeLocalPose( granny_local_pose *LocalPose )
{
	GR2_STUB( "LocalPose={}", LocalPose );
}

GR2_API( granny_int32x ) GrannyGetLocalPoseBoneCount( granny_local_pose const *LocalPose )
{
	GR2_STUB( "LocalPose={}", LocalPose );
	return 0;
}

GR2_API( granny_transform * ) GrannyGetLocalPoseTransform( granny_local_pose const *LocalPose,
                                                           granny_int32x BoneIndex )
{
	GR2_STUB( "LocalPose={} BoneIndex={}", LocalPose, BoneIndex );
	return 0;
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
