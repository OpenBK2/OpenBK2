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

#include "Unimplemented.h"

extern "C"
{

GR2_API( granny_controlled_animation_builder * )
	GrannyBeginControlledAnimation( granny_real32 StartTime, granny_animation const *Animation )
{
	GR2_STUB();
	(void)StartTime;
	(void)Animation;
	return 0;
}

GR2_API( granny_control * )
	GrannyEndControlledAnimation( granny_controlled_animation_builder *Builder )
{
	GR2_STUB();
	(void)Builder;
	return 0;
}

GR2_API( void ) GrannySetTrackGroupTarget( granny_controlled_animation_builder *Builder,
                                           granny_int32x TrackGroupIndex,
                                           granny_model_instance *Model )
{
	GR2_STUB();
	(void)Builder;
	(void)TrackGroupIndex;
	(void)Model;
}

GR2_API( void ) GrannySetTrackGroupAccumulation( granny_controlled_animation_builder *Builder,
                                                 granny_int32x TrackGroupIndex,
                                                 granny_accumulation_mode Mode )
{
	GR2_STUB();
	(void)Builder;
	(void)TrackGroupIndex;
	(void)Mode;
}

GR2_API( void ) GrannySetTrackGroupModelMask( granny_controlled_animation_builder *Builder,
                                              granny_int32x TrackGroupIndex,
                                              granny_track_mask *ModelMask )
{
	GR2_STUB();
	(void)Builder;
	(void)TrackGroupIndex;
	(void)ModelMask;
}

GR2_API( granny_track_mask * ) GrannyNewTrackMask( granny_real32 DefaultWeight,
                                                   granny_int32x BoneCount )
{
	GR2_STUB();
	(void)DefaultWeight;
	(void)BoneCount;
	return 0;
}

GR2_API( void ) GrannySetSkeletonTrackMaskFromTrackGroup( granny_track_mask *Mask,
                                                          granny_skeleton const *Skeleton,
                                                          granny_track_group const *TrackGroup,
                                                          granny_real32 IdentityValue,
                                                          granny_real32 ConstantValue,
                                                          granny_real32 AnimatedValue )
{
	GR2_STUB();
	(void)Mask;
	(void)Skeleton;
	(void)TrackGroup;
	(void)IdentityValue;
	(void)ConstantValue;
	(void)AnimatedValue;
}

}
