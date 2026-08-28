#pragma once

// A pose: one transform per bone, in the skeleton's own space.
//
// Another permanently opaque handle, so its shape is ours. The engine allocates
// one per animated object in CSkeletonAnimator, has GrannySampleModelAnimations
// fill it, then reads and sometimes overwrites individual bones through
// GrannyGetLocalPoseTransform before GrannyBuildWorldPose composes the chain.
//
// The real DLL spaces its transforms 80 bytes apart rather than 68, which is a
// granny_transform rounded up to sixteen, presumably so the maths underneath can
// use aligned loads. That is not reproduced, because it is not observable: the
// only way in is the accessor, which hands back one transform at a time, and no
// engine code does arithmetic on what it returns. A vector of transforms is
// simpler and says what it is.

#include <gr2/granny.h>

#include "Structures.h"

#include <vector>

//! Completes the opaque handle, as granny_file and granny_model_instance do.
struct granny_local_pose
{
	//! One per bone. Zeroed at allocation, which is what the real DLL leaves.
	//!
	//! Zeroed rather than identity, and that is the DLL's behaviour rather than a
	//! shortcut: a fresh pose has a (0,0,0,0) orientation, which is not a rotation
	//! at all. Every bone is expected to be written before it is read, and one
	//! that is not is meaningless in either implementation.
	std::vector<NGr2::STransform> Transforms;
};
