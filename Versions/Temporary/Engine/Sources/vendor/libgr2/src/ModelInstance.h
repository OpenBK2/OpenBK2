#pragma once

// A model being animated: what it was made from, and where its clock is.
//
// granny_model_instance is one of the handles that stays opaque permanently, so
// unlike the data records this library reproduces, its contents are ours to
// choose. What it has to hold is set by the entry points that take one:
// GrannySetModelClock, GrannySampleModelAnimations, and at M4 the controls bound
// to it through GrannySetTrackGroupTarget.
//
// It references its model rather than copying it, which is measured rather than
// assumed: the real DLL's GrannyGetSourceModel hands back the exact address that
// was passed to GrannyInstantiateModel, and still does after the skeleton behind
// it is modified. That matters here because the engine does not instantiate a
// model out of a file. CSkeletonAnimator keeps a granny_model as a member, fills
// in a name and a skeleton borrowed from a loaded file, sets MeshBindingCount to
// zero, and instantiates that. So the model an instance points at may be a
// caller's own object, alive only as long as the caller keeps it.

#include <gr2/granny.h>

#include "Structures.h"

//! Completes the opaque handle, the same way granny_file does in File.h.
struct granny_model_instance
{
	//! The caller's model. Not owned, not copied, and possibly not from a file.
	const NGr2::SModel *pModel = nullptr;

	//! Where this instance is in time, in seconds.
	//!
	//! Zero at instantiation. Nothing among the 54 entry points reads it back, so
	//! its only visible effect is on sampling, which is why it is stored and
	//! otherwise untouched until the pose runtime exists.
	float fClock = 0.0f;
};
