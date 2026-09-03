#pragma once

// A clip bound to a model instance, and the state that plays it.
//
// The last of the opaque handles, and the one with no prior art: every open
// source Granny project stops at reading a file, because an importer or a viewer
// never needs a playback layer. So nothing here is derived from a reference
// implementation. It is a model of what granny2.dll was observed to do, driven
// through the same call sequence the engine issues and read back after every
// step; scripts/port/gr2diff.py replays those sequences against both.
//
// What was measured, in the terms below:
//
//   model clock changes are queued as float deltas on every bound control
//   raw local clock  advances by pendingDelta * fSpeed when it is next read
//   loop wrapping    subtracts whole float periods and updates nLoopIndex
//   clamped local    is the resulting raw clock clamped to [0, duration]
//
// That stateful order is significant, and it is not equivalent to calculating
// fmod( ( modelClock - startTime ) * speed, duration ). The difference is what
// a speed change does to time already played.
//
// The engine drives the model clock with absolute game time, 0.001f * time in
// GAnimation.cpp, and rewrites a moving soldier's speed on every placement
// update in CMOUnitInfantry::AIUpdatePlacement. The absolute form rescales every
// second already elapsed by the new speed, so five minutes in a 3% speed change
// moved the local clock by half a period: the looping run clip snapped to an
// unrelated keyframe several times a second. Advancing by pendingDelta * speed
// applies a speed change only to the frames that follow it, which is what
// granny2.dll does. Control.cpp's test AMidPlaybackSpeedChangeDoesNotJumpTheClock
// is that sequence.
//
// Keeping the intermediate float roundings matters separately and more subtly:
// they can leave the clock one ULP below the end of a period where an absolute
// fmod gives zero.
//
// A finite control stops wrapping on its last period and therefore clamps at the
// duration. A loop count of zero wraps forever.
//   GetControlDuration     = nLoopCount * animation duration / |speed|, and a
//                            sentinel near 1.9e38 when the loop count is zero
//   GetControlDurationLeft = that duration minus ( modelClock - fStartTime ),
//                            which is model time and not local time, and which
//                            is allowed to go negative
//   IsComplete             = only ever true after CompleteControlAt, and only
//                            once the model clock reaches the time it was given.
//                            A clip that has simply finished playing is not
//                            complete.
//   effective weight       = ease-in value * ease-out value, or zero when the
//                            control has been deactivated
//
// The ease curves are cubic Beziers over their four numbers in the Bernstein
// basis, which was fitted over a grid of tuples rather than guessed: (1,0,0,0)
// evaluates to (1-u)^3, (0,0,0,1) to u^3, and the engine's own (0,0,1,1) to
// 3u^2-2u^3. The four numbers are stored as bytes, floor(v * 255 + 0.5) / 255,
// which is why asking for a weight of 0.5 gives back 0.501961.
//
// Outside its interval each curve is one rather than its endpoint value, on the
// side where it is not easing: an ease-in is one after it ends, an ease-out is
// one before it starts. On the other side each holds its endpoint. That makes a
// discontinuity where an ease-in ends on anything but one, which the engine
// never does.

#include <gr2/granny.h>

#include "Structures.h"

#include <vector>

//! Per-bone weights for one track group, as GrannyNewTrackMask makes them.
//!
//! There is no GrannyFreeTrackMask among the 54 entry points and the engine
//! never releases one, so a mask has to outlive every control that refers to it.
//! It is owned by the library and freed when the process is.
struct granny_track_mask
{
	float fDefaultWeight = 1.0f;
	std::vector<float> Weights;

	float WeightFor( int32_t nBone ) const
	{
		if ( nBone < 0 || static_cast<size_t>( nBone ) >= Weights.size() )
		{
			return fDefaultWeight;
		}
		return Weights[static_cast<size_t>( nBone )];
	}
};

namespace NGr2
{

//! One of the two ease curves, as SetControlEaseInCurve and its twin set them.
struct SEaseCurve
{
	bool bEnabled = false;
	float fStartSeconds = 0.0f;
	float fEndSeconds = 0.0f;
	//! Four Bezier control values, already quantised to bytes.
	float Values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	void Set( float fStart, float fEnd, float fStartValue, float fStartTangent,
	          float fEndTangent, float fEndValue );

	//! The curve at a model time, with the outside-the-interval rules applied.
	//!
	//! \param bEaseIn true for the ease-in curve, which is one after its end;
	//!                false for the ease-out, which is one before its start.
	float At( float fTime, bool bEaseIn ) const;
};

//! What one track group of a clip is pointed at, while the builder is open.
struct SBoundTrackGroup
{
	granny_model_instance *pTarget = nullptr;
	granny_track_mask *pMask = nullptr;
	int32_t nAccumulation = 0;
};

}

//! Collects what EndControlledAnimation needs, and nothing else.
struct granny_controlled_animation_builder
{
	float fStartTime = 0.0f;
	const NGr2::SAnimation *pAnimation = nullptr;
	std::vector<NGr2::SBoundTrackGroup> Groups;
};

//! A clip playing against one model instance.
struct granny_control
{
	const NGr2::SAnimation *pAnimation = nullptr;
	const NGr2::STrackGroup *pTrackGroup = nullptr;
	granny_model_instance *pTarget = nullptr;
	granny_track_mask *pMask = nullptr;
	int32_t nAccumulation = 0;

	//! Model time at which the clip's local clock is zero.
	float fStartTime = 0.0f;
	//! Last model clock assigned to this control and the delta not yet consumed by
	//! a local-clock query. GrannySetModelClock updates both for every bound clip.
	float fClock = 0.0f;
	mutable float fPendingClockDelta = 0.0f;
	//! Stateful local time and loop number. Mutable because Granny's clock getters
	//! consume the pending model-clock delta in the original implementation.
	mutable float fLocalClock = 0.0f;
	mutable int32_t nLoopIndex = 0;
	float fSpeed = 1.0f;
	//! Whole periods to play. Zero means forever, which is what the engine sets
	//! for a looping clip.
	int32_t nLoopCount = 1;
	bool bForceClampedLooping = false;
	bool bActive = true;

	bool bCompleteAtSet = false;
	float fCompleteAt = 0.0f;
	//! Set by GrannyFreeControlOnceUnused, which the engine calls on any clip it
	//! gives no end time. It has no observable effect while the control is bound;
	//! see Control.cpp.
	bool bFreeOnceUnused = false;

	NGr2::SEaseCurve EaseIn;
	NGr2::SEaseCurve EaseOut;

	float AnimationDuration() const;
	//! nLoopCount periods at this speed, or the sentinel for an endless clip.
	float Duration() const;
	//! Queues a model-clock change exactly as GrannySetModelClock does.
	void SetClock( float fNewClock );
	//! Applies the queued delta and performs Granny's stateful loop-index update.
	void UpdateLocalClock() const;
	float RawLocalClock( float fModelClock ) const;
	float ClampedLocalClock( float fModelClock ) const;
	float EffectiveWeight( float fModelClock ) const;
	bool IsComplete( float fModelClock ) const;

	//! Whether the curves should wrap at each end when sampled from here.
	//!
	//! Measured, and it is the loop index rather than the loop count that decides
	//! it: a curve wraps backwards when there is a period before this one and
	//! forwards when there is one after. So the first pass of a clip that plays
	//! twice does not wrap at its start but the second does, and an endless clip
	//! wraps at both ends always. Getting this wrong is invisible except within
	//! one span of the ends, which is exactly where a loop shows.
	void LoopFlags( float fModelClock, bool *pbForwards, bool *pbBackwards ) const;
};

namespace NGr2
{

//! What GetControlDuration returns for a clip that never ends.
//!
//! The bit pattern 0x7f0fffff, about 1.914e38. Not FLT_MAX (0x7f7fffff), not an
//! infinity, and constant: the same value comes back for animations of 0.3 and
//! 2.63 seconds and at every speed, so it is a sentinel rather than a
//! calculation that overflowed. Written as the bit pattern rather than as a
//! decimal, since that is what it is.
float EndlessDuration();

}
