// Binding a clip to a model instance, and masking which bones it reaches.
//
// M4. The engine builds a control per clip it starts: begin the builder, point
// each of the clip's track groups at a model instance, say what that group does
// with root motion, optionally give it a per-bone weight mask, and end the
// builder to get the granny_control that playback then drives.
//
// Nothing here has prior art. Every open source Granny project stops at reading
// a file, because an importer or a viewer never needs a binding layer.
//
// Note what is missing from the 54: there is no GrannyFreeTrackMask. The engine
// allocates track masks and never releases them, and reproducing that faithfully
// means this library has to outlive them too.

#include <gr2/granny.h>

#include "Trace.h"

extern "C"
{

GR2_API( granny_controlled_animation_builder * )
	GrannyBeginControlledAnimation( granny_real32 StartTime, granny_animation const *Animation )
{
	GR2_TRACE( "StartTime={} Animation={}", StartTime, Animation );
	return 0;
}

GR2_API( granny_control * )
	GrannyEndControlledAnimation( granny_controlled_animation_builder *Builder )
{
	GR2_TRACE( "Builder={}", Builder );
	return 0;
}

GR2_API( void ) GrannySetTrackGroupTarget( granny_controlled_animation_builder *Builder,
                                           granny_int32x TrackGroupIndex,
                                           granny_model_instance *Model )
{
	GR2_TRACE( "Builder={} TrackGroupIndex={} Model={}", Builder, TrackGroupIndex, Model );
}

GR2_API( void ) GrannySetTrackGroupAccumulation( granny_controlled_animation_builder *Builder,
                                                 granny_int32x TrackGroupIndex,
                                                 granny_accumulation_mode Mode )
{
	GR2_TRACE( "Builder={} TrackGroupIndex={} Mode={}", Builder, TrackGroupIndex, Mode );
}

GR2_API( void ) GrannySetTrackGroupModelMask( granny_controlled_animation_builder *Builder,
                                              granny_int32x TrackGroupIndex,
                                              granny_track_mask *ModelMask )
{
	GR2_TRACE( "Builder={} TrackGroupIndex={} ModelMask={}", Builder, TrackGroupIndex, ModelMask );
}

GR2_API( granny_track_mask * ) GrannyNewTrackMask( granny_real32 DefaultWeight,
                                                   granny_int32x BoneCount )
{
	GR2_TRACE( "DefaultWeight={} BoneCount={}", DefaultWeight, BoneCount );
	return 0;
}

GR2_API( void ) GrannySetSkeletonTrackMaskFromTrackGroup( granny_track_mask *Mask,
                                                          granny_skeleton const *Skeleton,
                                                          granny_track_group const *TrackGroup,
                                                          granny_real32 IdentityValue,
                                                          granny_real32 ConstantValue,
                                                          granny_real32 AnimatedValue )
{
	GR2_TRACE( "Mask={} Skeleton={} TrackGroup={} IdentityValue={} ConstantValue={} "
	           "AnimatedValue={}",
	           Mask, Skeleton, TrackGroup, IdentityValue, ConstantValue, AnimatedValue );
}

}
