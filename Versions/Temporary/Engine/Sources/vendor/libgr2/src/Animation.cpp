// Binding a clip to a model instance, and masking which bones it reaches.
//
// Seven entry points that the engine always calls in one order, which
// granny_calls.log shows 198 times without variation:
//
//   BeginControlledAnimation( startTime, animation )
//   SetTrackGroupTarget( builder, 0, modelInstance )
//   SetTrackGroupAccumulation( builder, 0, GrannyNoAccumulation )
//   NewTrackMask( 1.0, boneCount )
//   SetSkeletonTrackMaskFromTrackGroup( mask, skeleton, trackGroup, 1, 1, 1 )
//   SetTrackGroupModelMask( builder, 0, mask )
//   EndControlledAnimation( builder )
//
// The builder exists so that a clip with several track groups can point each one
// at a different instance before any of it takes effect. This game's animations
// have one track group each in 11,328 of the 11,344 that have any, and two in
// the other 16, and the engine only ever binds group zero.
//
// Note what is missing from the 54: there is no GrannyFreeTrackMask. The engine
// allocates a mask per clip it starts and never releases one, so a mask has to
// outlive every control that refers to it, and this library has to outlive them
// too. They are kept in one list and released when the process ends.

#include <gr2/granny.h>

#include "Control.h"
#include "ModelInstance.h"
#include "Structures.h"
#include "Trace.h"

#include <cstring>
#include <memory>
#include <new>
#include <vector>

using namespace NGr2;

namespace
{

//! Every track mask ever handed out.
//!
//! There is no entry point that frees one, so this is the only thing that can.
//! A function-local static rather than a namespace-scope one so that the order
//! it is destroyed in is defined relative to its first use.
std::vector<std::unique_ptr<granny_track_mask>> &TrackMasks()
{
	static std::vector<std::unique_ptr<granny_track_mask>> masks;
	return masks;
}

//! Whether a transform track has anything to say about a bone.
//!
//! Granny's three values distinguish a bone the track group does not mention, a
//! bone whose track is constant, and a bone whose track is animated. The engine
//! passes 1 for all three, so every bone gets weight 1 and none of this is
//! visible in the game; it is written out because a mask with a wrong shape
//! would be invisible until some other caller used one.
enum ETrackKind
{
	TRACK_IDENTITY,
	TRACK_CONSTANT,
	TRACK_ANIMATED,
};

bool CurveIsAnimated( const SCurve2 &curve )
{
	const SCurveDataDaK32fC32f *pData =
		static_cast<const SCurveDataDaK32fC32f *>( curve.CurveData.pObject );
	return pData != nullptr && pData->nKnotCount > 1;
}

bool CurveHasKeys( const SCurve2 &curve )
{
	const SCurveDataDaK32fC32f *pData =
		static_cast<const SCurveDataDaK32fC32f *>( curve.CurveData.pObject );
	return pData != nullptr && pData->nKnotCount > 0;
}

ETrackKind KindOf( const STransformTrack &track )
{
	if ( CurveIsAnimated( track.PositionCurve ) || CurveIsAnimated( track.OrientationCurve )
	     || CurveIsAnimated( track.ScaleShearCurve ) )
	{
		return TRACK_ANIMATED;
	}
	if ( CurveHasKeys( track.PositionCurve ) || CurveHasKeys( track.OrientationCurve )
	     || CurveHasKeys( track.ScaleShearCurve ) )
	{
		return TRACK_CONSTANT;
	}
	return TRACK_IDENTITY;
}

}

extern "C"
{

GR2_API( granny_controlled_animation_builder * )
	GrannyBeginControlledAnimation( granny_real32 StartTime, granny_animation const *Animation )
{
	GR2_TRACE( "StartTime={} Animation={}", StartTime, Animation );

	if ( Animation == 0 )
	{
		return 0;
	}

	granny_controlled_animation_builder *pBuilder =
		new ( std::nothrow ) granny_controlled_animation_builder;
	if ( pBuilder == nullptr )
	{
		return 0;
	}
	pBuilder->fStartTime = StartTime;
	pBuilder->pAnimation = reinterpret_cast<const SAnimation *>( Animation );
	pBuilder->Groups.resize(
		static_cast<size_t>( pBuilder->pAnimation->nTrackGroupCount > 0
		                         ? pBuilder->pAnimation->nTrackGroupCount
		                         : 0 ) );
	return pBuilder;
}

GR2_API( void ) GrannySetTrackGroupTarget( granny_controlled_animation_builder *Builder,
                                           granny_int32x TrackGroupIndex,
                                           granny_model_instance *Model )
{
	GR2_TRACE( "Builder={} TrackGroupIndex={} Model={}", Builder, TrackGroupIndex, Model );

	if ( Builder == 0 || TrackGroupIndex < 0
	     || static_cast<size_t>( TrackGroupIndex ) >= Builder->Groups.size() )
	{
		return;
	}
	Builder->Groups[static_cast<size_t>( TrackGroupIndex )].pTarget = Model;
}

GR2_API( void ) GrannySetTrackGroupAccumulation( granny_controlled_animation_builder *Builder,
                                                 granny_int32x TrackGroupIndex,
                                                 granny_accumulation_mode Mode )
{
	GR2_TRACE( "Builder={} TrackGroupIndex={} Mode={}", Builder, TrackGroupIndex, Mode );

	if ( Builder == 0 || TrackGroupIndex < 0
	     || static_cast<size_t>( TrackGroupIndex ) >= Builder->Groups.size() )
	{
		return;
	}
	// Recorded and not acted on. The engine passes GrannyNoAccumulation for every
	// clip it starts, and does its own root motion in
	// CSkeletonAnimator::ApplyGlobalMovementCorrection out of the local clock and
	// the effective weight, so there is nothing for the other modes to do here.
	Builder->Groups[static_cast<size_t>( TrackGroupIndex )].nAccumulation =
		static_cast<int32_t>( Mode );
}

GR2_API( void ) GrannySetTrackGroupModelMask( granny_controlled_animation_builder *Builder,
                                              granny_int32x TrackGroupIndex,
                                              granny_track_mask *ModelMask )
{
	GR2_TRACE( "Builder={} TrackGroupIndex={} ModelMask={}", Builder, TrackGroupIndex,
	           ModelMask );

	if ( Builder == 0 || TrackGroupIndex < 0
	     || static_cast<size_t>( TrackGroupIndex ) >= Builder->Groups.size() )
	{
		return;
	}
	Builder->Groups[static_cast<size_t>( TrackGroupIndex )].pMask = ModelMask;
}

GR2_API( granny_control * )
	GrannyEndControlledAnimation( granny_controlled_animation_builder *Builder )
{
	GR2_TRACE( "Builder={}", Builder );

	if ( Builder == 0 )
	{
		return 0;
	}
	// The builder is consumed whatever happens, so an early return below still
	// releases it.
	//
	// Its trace id goes back with it. The real DLL's allocator hands the same
	// address out again for the next builder, and without this the second one
	// inherits the first one's number, which is a difference between the two
	// logs that says nothing about either implementation.
	RetireHandle( Builder );
	std::unique_ptr<granny_controlled_animation_builder> builder( Builder );

	const SAnimation *pAnimation = builder->pAnimation;
	if ( pAnimation == nullptr || pAnimation->nTrackGroupCount <= 0
	     || pAnimation->ppTrackGroups == nullptr )
	{
		return 0;
	}

	// One control per bound group. The engine binds only group zero and reads
	// back a single pointer, so that is the one returned; the rest are bound to
	// their instances and driven by the same clock.
	granny_control *pFirst = nullptr;
	for ( size_t i = 0; i < builder->Groups.size(); ++i )
	{
		const SBoundTrackGroup &group = builder->Groups[i];
		if ( group.pTarget == nullptr )
		{
			continue;
		}

		granny_control *pControl = new ( std::nothrow ) granny_control;
		if ( pControl == nullptr )
		{
			continue;
		}
		pControl->pAnimation = pAnimation;
		pControl->pTrackGroup = pAnimation->ppTrackGroups[i];
		pControl->pTarget = group.pTarget;
		pControl->pMask = group.pMask;
		pControl->nAccumulation = group.nAccumulation;
		pControl->fStartTime = builder->fStartTime;
		// Granny initializes the control's own model clock at the requested start
		// time; later model-clock assignments are queued as deltas from here.
		pControl->fClock = builder->fStartTime;

		group.pTarget->Controls.push_back( pControl );
		if ( pFirst == nullptr )
		{
			pFirst = pControl;
		}
	}
	return pFirst;
}

GR2_API( granny_track_mask * ) GrannyNewTrackMask( granny_real32 DefaultWeight,
                                                   granny_int32x BoneCount )
{
	GR2_TRACE( "DefaultWeight={} BoneCount={}", DefaultWeight, BoneCount );

	std::unique_ptr<granny_track_mask> mask( new ( std::nothrow ) granny_track_mask );
	if ( mask == nullptr )
	{
		return 0;
	}
	mask->fDefaultWeight = DefaultWeight;
	if ( BoneCount > 0 )
	{
		mask->Weights.assign( static_cast<size_t>( BoneCount ), DefaultWeight );
	}

	granny_track_mask *pResult = mask.get();
	// Kept forever: there is no entry point that frees one.
	TrackMasks().push_back( std::move( mask ) );
	return pResult;
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

	if ( Mask == 0 || Skeleton == 0 || TrackGroup == 0 )
	{
		return;
	}

	const SSkeleton *pSkeleton = reinterpret_cast<const SSkeleton *>( Skeleton );
	const STrackGroup *pGroup = reinterpret_cast<const STrackGroup *>( TrackGroup );
	if ( pSkeleton->pBones == nullptr )
	{
		return;
	}

	if ( Mask->Weights.size() < static_cast<size_t>( pSkeleton->nBoneCount ) )
	{
		Mask->Weights.resize( static_cast<size_t>( pSkeleton->nBoneCount ),
		                      Mask->fDefaultWeight );
	}

	for ( int32_t i = 0; i < pSkeleton->nBoneCount; ++i )
	{
		const char *pszBone = pSkeleton->pBones[i].pszName;
		ETrackKind kind = TRACK_IDENTITY;
		for ( int32_t t = 0; pszBone != nullptr && t < pGroup->nTransformTrackCount; ++t )
		{
			const STransformTrack &track = pGroup->pTransformTracks[t];
			if ( track.pszName != nullptr && strcmp( track.pszName, pszBone ) == 0 )
			{
				kind = KindOf( track );
				break;
			}
		}
		Mask->Weights[static_cast<size_t>( i )] =
			kind == TRACK_ANIMATED ? AnimatedValue
			                       : kind == TRACK_CONSTANT ? ConstantValue : IdentityValue;
	}
}

}
