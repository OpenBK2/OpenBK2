#pragma once

// A pose after the skeleton hierarchy has been walked: two 4x4 matrices per
// bone, one for where the bone is in the world and one for what the skinning
// path multiplies vertices by.
//
// The engine reads both, and reads them as raw floats:
//
//     granny_real32 *pMatrix = GrannyGetWorldPose4x4( pGlobal, nAddBone );
//     value.forward.Set( pMatrix[0], pMatrix[4], pMatrix[8], pMatrix[12], ... );
//
// with no null check, in CAddBoneFilter::Recalc. So the layout is an ABI and the
// bounds are load bearing.
//
// Row vector convention, translation in elements 12, 13 and 14, which is what
// that transpose in the engine confirms and what the DLL was measured to
// produce. The upper 3x3 is the transpose of the column convention's rotation
// times scale, so the two agree: Transform.cpp composes in columns, this stores
// in rows.
//
// A world pose may be smaller than its skeleton. CAddBoneFilter allocates one of
// nAddBone + 1 entries and builds only that far, because it wants a single
// bone's matrix and has no use for the rest.

#include <gr2/granny.h>

#include <vector>

//! Completes the opaque handle, as granny_file and the others do.
struct granny_world_pose
{
	//! Sixteen floats per bone, laid out for the accessor to hand back directly.
	//!
	//! Zeroed at allocation. The real DLL leaves them uninitialised, complete with
	//! the occasional NaN, which is not a behaviour worth reproducing: matching
	//! garbage is impossible and a deterministic zero is easier to debug. Nothing
	//! reads a bone the build did not reach, and if something ever does, zeros are
	//! a better clue than whatever the allocator had.
	std::vector<float> World;
	std::vector<float> Composite;

	uint32_t BoneCount() const { return static_cast<uint32_t>( World.size() / 16 ); }
};
