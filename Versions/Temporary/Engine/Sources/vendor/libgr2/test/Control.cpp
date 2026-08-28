// The playback layer: binding a clip, and the clock and weight that drive it.
//
// These fixtures build a skeleton, an animation and a track group by hand, bind
// them the way CSkeletonAnimator does, and drive the model clock forwards. What
// they assert is the arithmetic written down in Control.h, which was measured out
// of granny2.dll by scripting the same call sequences against it.
//
// The corpus side of that measurement is scripts/port/gr2control.py, which
// replays fifteen scenarios per file against both implementations. It cannot be
// a unit test: it needs the DLL and it needs the corpus. What it reported,
// including the two things that still differ, is in docs/GrannyReplacement.md.

#include "Control.h"
#include "ModelInstance.h"
#include "Structures.h"

#include <gr2/granny.h>

#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{

//! A skeleton, an animation and one track group, all owned by the fixture.
//!
//! Built directly rather than through a .gr2, because the entry points under test
//! take granny211.h structures and know nothing about files. Animation.cpp's
//! tests cover the path from a file to these.
class CClip
{
public:
	//! \param nBones how many bones, all named "bone0", "bone1", ...
	//! \param fDuration the animation's duration.
	explicit CClip( int32_t nBones = 3, float fDuration = 2.0f )
	{
		m_Names.resize( static_cast<size_t>( nBones ) );
		m_Bones.resize( static_cast<size_t>( nBones ) );
		m_Tracks.resize( static_cast<size_t>( nBones ) );
		for ( int32_t i = 0; i < nBones; ++i )
		{
			m_Names[static_cast<size_t>( i )] = "bone" + std::to_string( i );
			const char *pszName = m_Names[static_cast<size_t>( i )].c_str();

			NGr2::SBone &bone = m_Bones[static_cast<size_t>( i )];
			bone.pszName = pszName;
			bone.nParentIndex = i == 0 ? -1 : i - 1;
			bone.LocalTransform.nFlags = NGr2::TRANSFORM_HAS_POSITION;
			bone.LocalTransform.Position[0] = static_cast<float>( i ) + 100.0f;
			bone.LocalTransform.Orientation[3] = 1.0f;
			bone.LocalTransform.ScaleShear[0][0] = 1.0f;
			bone.LocalTransform.ScaleShear[1][1] = 1.0f;
			bone.LocalTransform.ScaleShear[2][2] = 1.0f;

			m_Tracks[static_cast<size_t>( i )].pszName = pszName;
		}

		m_Skeleton.pszName = "skeleton";
		m_Skeleton.nBoneCount = nBones;
		m_Skeleton.pBones = m_Bones.data();

		m_Model.pszName = "model";
		m_Model.pSkeleton = &m_Skeleton;
		m_Model.InitialPlacement.Orientation[3] = 1.0f;

		m_Group.pszName = "group";
		m_Group.nTransformTrackCount = nBones;
		m_Group.pTransformTracks = m_Tracks.data();

		m_pGroup = &m_Group;
		m_Animation.pszName = "clip";
		m_Animation.fDuration = fDuration;
		m_Animation.fTimeStep = 1.0f / 30.0f;
		m_Animation.nTrackGroupCount = 1;
		m_Animation.ppTrackGroups = &m_pGroup;
	}

	//! Give a bone a position curve that runs from one value to another.
	//!
	//! Degree 1 over two knots, so the value at a local clock is easy to state:
	//! the fixture is about the clock and the weight, not about the spline.
	void SetLinearPosition( int32_t nBone, float fFrom, float fTo )
	{
		const size_t n = static_cast<size_t>( nBone );
		m_Knots.push_back( { 0.0f, m_Animation.fDuration } );
		m_Controls.push_back( { fFrom, 0.0f, 0.0f, fTo, 0.0f, 0.0f } );

		NGr2::SCurveDataDaK32fC32f &data = *new NGr2::SCurveDataDaK32fC32f();
		m_Curves.push_back( &data );
		data.Header.nFormat = NGr2::CURVE_DA_K32F_C32F;
		data.Header.nDegree = 1;
		data.nPadding = 0;
		data.nKnotCount = 2;
		data.pKnots = m_Knots.back().data();
		data.nControlCount = 6;
		data.pControls = m_Controls.back().data();
		m_Tracks[n].PositionCurve.CurveData.pObject = &data;
	}

	~CClip()
	{
		for ( size_t i = 0; i < m_Curves.size(); ++i )
		{
			delete m_Curves[i];
		}
	}

	CClip( const CClip & ) = delete;
	CClip &operator=( const CClip & ) = delete;

	granny_model const *Model() const
	{
		return reinterpret_cast<granny_model const *>( &m_Model );
	}
	granny_animation const *Animation() const
	{
		return reinterpret_cast<granny_animation const *>( &m_Animation );
	}
	granny_skeleton const *Skeleton() const
	{
		return reinterpret_cast<granny_skeleton const *>( &m_Skeleton );
	}
	granny_track_group const *Group() const
	{
		return reinterpret_cast<granny_track_group const *>( &m_Group );
	}
	const NGr2::STransformTrack *TrackOf( int32_t nBone ) const
	{
		return &m_Tracks[static_cast<size_t>( nBone )];
	}
	int32_t BoneCount() const { return m_Skeleton.nBoneCount; }
	float Duration() const { return m_Animation.fDuration; }

private:
	std::vector<std::string> m_Names;
	std::vector<NGr2::SBone> m_Bones;
	std::vector<NGr2::STransformTrack> m_Tracks;
	std::vector<std::array<float, 2>> m_Knots;
	std::vector<std::array<float, 6>> m_Controls;
	std::vector<NGr2::SCurveDataDaK32fC32f *> m_Curves;
	NGr2::SSkeleton m_Skeleton{};
	NGr2::SModel m_Model{};
	NGr2::STrackGroup m_Group{};
	NGr2::STrackGroup *m_pGroup = nullptr;
	NGr2::SAnimation m_Animation{};
};

//! The engine's seven binding calls, in the engine's order.
granny_control *Bind( const CClip &clip, granny_model_instance *pInstance,
                      float fStartTime = 0.0f )
{
	granny_controlled_animation_builder *pBuilder =
		GrannyBeginControlledAnimation( fStartTime, clip.Animation() );
	if ( pBuilder == nullptr )
	{
		return nullptr;
	}
	GrannySetTrackGroupTarget( pBuilder, 0, pInstance );
	GrannySetTrackGroupAccumulation( pBuilder, 0, GrannyNoAccumulation );
	granny_track_mask *pMask = GrannyNewTrackMask( 1.0f, clip.BoneCount() );
	GrannySetSkeletonTrackMaskFromTrackGroup( pMask, clip.Skeleton(), clip.Group(),
	                                          1.0f, 1.0f, 1.0f );
	GrannySetTrackGroupModelMask( pBuilder, 0, pMask );
	return GrannyEndControlledAnimation( pBuilder );
}

}

TEST( Control, TheBuilderProducesAControlBoundToItsInstance )
{
	CClip clip;
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	ASSERT_NE( nullptr, pInstance );

	granny_control *pControl = Bind( clip, pInstance );
	ASSERT_NE( nullptr, pControl );
	EXPECT_FLOAT_EQ( 1.0f, GrannyGetControlSpeed( pControl ) );
	// One period by default, which is what the clock rules below key off.
	EXPECT_FLOAT_EQ( clip.Duration(), GrannyGetControlDuration( pControl ) );

	// A builder with nothing pointed anywhere produces no control, and a null
	// animation produces no builder.
	EXPECT_EQ( nullptr, GrannyBeginControlledAnimation( 0.0f, nullptr ) );
	granny_controlled_animation_builder *pEmpty =
		GrannyBeginControlledAnimation( 0.0f, clip.Animation() );
	ASSERT_NE( nullptr, pEmpty );
	EXPECT_EQ( nullptr, GrannyEndControlledAnimation( pEmpty ) );
	EXPECT_EQ( nullptr, GrannyEndControlledAnimation( nullptr ) );

	GrannyFreeModelInstance( pInstance );
}

TEST( Control, TheLocalClockClampsAtOnePeriodAndWrapsWhenLooping )
{
	CClip clip( 3, 2.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_control *pControl = Bind( clip, pInstance );
	ASSERT_NE( nullptr, pControl );

	// One period: the clip holds its last frame rather than stopping or wrapping.
	GrannySetControlLoopCount( pControl, 1 );
	const float Clocks[] = { 0.0f, 0.5f, 1.5f, 2.0f, 3.0f, 10.0f };
	const float Expected[] = { 0.0f, 0.5f, 1.5f, 2.0f, 2.0f, 2.0f };
	for ( size_t i = 0; i < sizeof( Clocks ) / sizeof( Clocks[0] ); ++i )
	{
		GrannySetModelClock( pInstance, Clocks[i] );
		EXPECT_FLOAT_EQ( Expected[i], GrannyGetControlClampedLocalClock( pControl ) )
			<< "at model clock " << Clocks[i];
	}

	// Endless: it wraps forever, and the duration is the sentinel.
	GrannySetControlLoopCount( pControl, 0 );
	const float Wrapped[] = { 0.0f, 0.5f, 0.0f, 1.0f, 0.0f };
	const float At[] = { 0.0f, 0.5f, 2.0f, 3.0f, 10.0f };
	for ( size_t i = 0; i < sizeof( At ) / sizeof( At[0] ); ++i )
	{
		GrannySetModelClock( pInstance, At[i] );
		EXPECT_FLOAT_EQ( Wrapped[i], GrannyGetControlClampedLocalClock( pControl ) )
			<< "at model clock " << At[i];
	}
	EXPECT_GT( GrannyGetControlDuration( pControl ), 1.0e38f );

	// Two periods: wraps once and then holds.
	GrannySetControlLoopCount( pControl, 2 );
	EXPECT_FLOAT_EQ( 4.0f, GrannyGetControlDuration( pControl ) );
	GrannySetModelClock( pInstance, 2.5f );
	EXPECT_FLOAT_EQ( 0.5f, GrannyGetControlClampedLocalClock( pControl ) );
	GrannySetModelClock( pInstance, 4.5f );
	EXPECT_FLOAT_EQ( 2.0f, GrannyGetControlClampedLocalClock( pControl ) );

	GrannyFreeModelInstance( pInstance );
}

TEST( Control, SpeedScalesTheClockAndTheDuration )
{
	CClip clip( 3, 2.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_control *pControl = Bind( clip, pInstance );
	GrannySetControlLoopCount( pControl, 1 );

	GrannySetControlSpeed( pControl, 2.0f );
	EXPECT_FLOAT_EQ( 1.0f, GrannyGetControlDuration( pControl ) );
	GrannySetModelClock( pInstance, 0.25f );
	EXPECT_FLOAT_EQ( 0.5f, GrannyGetControlClampedLocalClock( pControl ) );

	GrannySetControlSpeed( pControl, 0.5f );
	EXPECT_FLOAT_EQ( 4.0f, GrannyGetControlDuration( pControl ) );
	GrannySetModelClock( pInstance, 1.0f );
	EXPECT_FLOAT_EQ( 0.5f, GrannyGetControlClampedLocalClock( pControl ) );

	// A negative speed keeps the duration positive, which is measured and not
	// obvious: the clip simply never advances.
	GrannySetControlSpeed( pControl, -1.0f );
	EXPECT_FLOAT_EQ( 2.0f, GrannyGetControlDuration( pControl ) );
	GrannySetModelClock( pInstance, 1.0f );
	EXPECT_FLOAT_EQ( 0.0f, GrannyGetControlClampedLocalClock( pControl ) );

	GrannyFreeModelInstance( pInstance );
}

TEST( Control, DurationLeftIsInModelTimeAndGoesNegative )
{
	CClip clip( 3, 2.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_control *pControl = Bind( clip, pInstance, 0.5f );
	GrannySetControlLoopCount( pControl, 1 );

	// Counted from the start time, so before the clip begins there is more left
	// than the clip is long.
	GrannySetModelClock( pInstance, 0.0f );
	EXPECT_FLOAT_EQ( 2.5f, GrannyGetControlDurationLeft( pControl ) );
	GrannySetModelClock( pInstance, 1.5f );
	EXPECT_FLOAT_EQ( 1.0f, GrannyGetControlDurationLeft( pControl ) );
	GrannySetModelClock( pInstance, 4.5f );
	EXPECT_FLOAT_EQ( -2.0f, GrannyGetControlDurationLeft( pControl ) );

	GrannyFreeModelInstance( pInstance );
}

TEST( Control, IsCompleteOnlyAfterCompleteControlAt )
{
	// A clip that has simply finished playing is not complete. Measured over four
	// periods: a control with a loop count of one reports false forever. Only
	// CompleteControlAt makes it true, and only once the clock reaches the time it
	// was given. CSkeletonAnimator::Recalc deactivates complete controls, so
	// getting this wrong stops every clip that has no end time.
	CClip clip( 3, 2.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_control *pControl = Bind( clip, pInstance );
	GrannySetControlLoopCount( pControl, 1 );

	for ( float t : { 0.0f, 1.0f, 2.0f, 4.0f, 100.0f } )
	{
		GrannySetModelClock( pInstance, t );
		EXPECT_FALSE( GrannyControlIsComplete( pControl ) ) << "at " << t;
	}

	GrannyCompleteControlAt( pControl, 3.0f );
	GrannySetModelClock( pInstance, 2.99f );
	EXPECT_FALSE( GrannyControlIsComplete( pControl ) );
	GrannySetModelClock( pInstance, 3.0f );
	EXPECT_TRUE( GrannyControlIsComplete( pControl ) );
	GrannySetModelClock( pInstance, 100.0f );
	EXPECT_TRUE( GrannyControlIsComplete( pControl ) );

	GrannyFreeModelInstance( pInstance );
}

TEST( Control, TheEaseCurvesAreCubicBeziersOverBytes )
{
	CClip clip( 3, 2.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_control *pControl = Bind( clip, pInstance );

	// The engine's own ease-in: Bezier(0, 0, 1, 1), which is 3u^2 - 2u^3.
	GrannySetControlEaseIn( pControl, true );
	GrannySetControlEaseInCurve( pControl, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f );
	const float Us[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
	for ( float u : Us )
	{
		GrannySetModelClock( pInstance, u );
		EXPECT_NEAR( 3.0f * u * u - 2.0f * u * u * u,
		             GrannyGetControlEffectiveWeight( pControl ), 1e-6f )
			<< "at u=" << u;
	}
	// After the ease-in ends the weight is one, not the curve's end value.
	GrannySetModelClock( pInstance, 2.0f );
	EXPECT_FLOAT_EQ( 1.0f, GrannyGetControlEffectiveWeight( pControl ) );

	// The four values are stored as bytes: floor(v * 255 + 0.5) / 255. Asking
	// for a half gives back 128/255, which is where 0.501961 comes from.
	GrannySetControlEaseInCurve( pControl, 0.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.5f );
	GrannySetModelClock( pInstance, 0.5f );
	EXPECT_FLOAT_EQ( 128.0f / 255.0f, GrannyGetControlEffectiveWeight( pControl ) );
	// And 0.7 rounds up rather than to even: 179/255, not 178/255.
	GrannySetControlEaseInCurve( pControl, 0.0f, 1.0f, 0.7f, 0.7f, 0.7f, 0.7f );
	GrannySetModelClock( pInstance, 0.5f );
	EXPECT_FLOAT_EQ( 179.0f / 255.0f, GrannyGetControlEffectiveWeight( pControl ) );

	GrannyFreeModelInstance( pInstance );
}

TEST( Control, AnEaseIsOneOnTheSideItIsNotEasing )
{
	CClip clip( 3, 2.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_control *pControl = Bind( clip, pInstance );

	// Ease in over [1, 2] from 0.2 to 0.8: before it, the start value; after it,
	// one. Measured, and the discontinuity at the end is real.
	GrannySetControlEaseIn( pControl, true );
	GrannySetControlEaseInCurve( pControl, 1.0f, 2.0f, 0.2f, 0.2f, 0.8f, 0.8f );
	GrannySetModelClock( pInstance, 0.0f );
	EXPECT_NEAR( 0.2f, GrannyGetControlEffectiveWeight( pControl ), 0.005f );
	GrannySetModelClock( pInstance, 2.0f );
	EXPECT_NEAR( 0.8f, GrannyGetControlEffectiveWeight( pControl ), 0.005f );
	GrannySetModelClock( pInstance, 2.5f );
	EXPECT_FLOAT_EQ( 1.0f, GrannyGetControlEffectiveWeight( pControl ) );
	GrannySetControlEaseIn( pControl, false );

	// Ease out over [1, 2]: one before it, the end value after it.
	GrannySetControlEaseOut( pControl, true );
	GrannySetControlEaseOutCurve( pControl, 1.0f, 2.0f, 0.8f, 0.8f, 0.2f, 0.2f );
	GrannySetModelClock( pInstance, 0.0f );
	EXPECT_FLOAT_EQ( 1.0f, GrannyGetControlEffectiveWeight( pControl ) );
	GrannySetModelClock( pInstance, 1.0f );
	EXPECT_NEAR( 0.8f, GrannyGetControlEffectiveWeight( pControl ), 0.005f );
	GrannySetModelClock( pInstance, 5.0f );
	EXPECT_NEAR( 0.2f, GrannyGetControlEffectiveWeight( pControl ), 0.005f );

	GrannyFreeModelInstance( pInstance );
}

TEST( Control, EaseControlInAndOutRampFromTheCurrentClock )
{
	CClip clip( 3, 2.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_control *pControl = Bind( clip, pInstance );

	GrannySetModelClock( pInstance, 1.0f );
	EXPECT_FLOAT_EQ( 2.0f, GrannyEaseControlIn( pControl, 2.0f, false ) );
	EXPECT_FLOAT_EQ( 0.0f, GrannyGetControlEffectiveWeight( pControl ) );
	GrannySetModelClock( pInstance, 2.0f );
	EXPECT_NEAR( 0.5f, GrannyGetControlEffectiveWeight( pControl ), 1e-6f );
	GrannySetModelClock( pInstance, 3.0f );
	EXPECT_FLOAT_EQ( 1.0f, GrannyGetControlEffectiveWeight( pControl ) );

	GrannySetControlEaseIn( pControl, false );
	GrannySetModelClock( pInstance, 1.0f );
	EXPECT_FLOAT_EQ( 2.0f, GrannyEaseControlOut( pControl, 2.0f ) );
	GrannySetModelClock( pInstance, 2.0f );
	EXPECT_NEAR( 0.5f, GrannyGetControlEffectiveWeight( pControl ), 1e-6f );
	GrannySetModelClock( pInstance, 3.0f );
	EXPECT_FLOAT_EQ( 0.0f, GrannyGetControlEffectiveWeight( pControl ) );

	GrannyFreeModelInstance( pInstance );
}

TEST( Control, DeactivatingOneDropsItsWeightToZero )
{
	CClip clip( 3, 2.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_control *pControl = Bind( clip, pInstance );
	GrannySetModelClock( pInstance, 0.5f );
	EXPECT_FLOAT_EQ( 1.0f, GrannyGetControlEffectiveWeight( pControl ) );
	GrannySetControlActive( pControl, false );
	EXPECT_FLOAT_EQ( 0.0f, GrannyGetControlEffectiveWeight( pControl ) );
	GrannySetControlActive( pControl, true );
	EXPECT_FLOAT_EQ( 1.0f, GrannyGetControlEffectiveWeight( pControl ) );
	GrannyFreeModelInstance( pInstance );
}

TEST( Control, SetControlRawLocalClockMovesTheStartAndKeepsRunning )
{
	CClip clip( 3, 2.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_control *pControl = Bind( clip, pInstance );
	GrannySetControlLoopCount( pControl, 0 );

	GrannySetModelClock( pInstance, 1.0f );
	EXPECT_FLOAT_EQ( 1.0f, GrannyGetControlClampedLocalClock( pControl ) );
	GrannySetControlRawLocalClock( pControl, 0.25f );
	EXPECT_FLOAT_EQ( 0.25f, GrannyGetControlClampedLocalClock( pControl ) );
	// And it carries on from there rather than being pinned.
	GrannySetModelClock( pInstance, 1.5f );
	EXPECT_FLOAT_EQ( 0.75f, GrannyGetControlClampedLocalClock( pControl ) );

	GrannyFreeModelInstance( pInstance );
}

TEST( Control, SamplingBlendsByWeightAndNormalisesByTheTotal )
{
	// Measured over pairs of controls at a spread of weights: the result is the
	// weighted average divided by the total weight, and the rest pose takes no
	// part. So one clip at a weight of a quarter still shows at full strength,
	// and an ease is visible only as a cross-fade.
	//
	// The loop count is one on purpose. A looping clip identifies its last
	// control with its first, which is right for real data, where every
	// multi-knot curve in the corpus ends exactly at the animation duration. It
	// makes a two-key curve that goes somewhere constant, and a first version of
	// this fixture looped and then could not understand why it read zero.
	CClip clip( 1, 2.0f );
	clip.SetLinearPosition( 0, 0.0f, 100.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_local_pose *pPose = GrannyNewLocalPose( 1 );

	granny_control *pA = Bind( clip, pInstance );
	granny_control *pB = Bind( clip, pInstance );
	ASSERT_NE( nullptr, pA );
	ASSERT_NE( nullptr, pB );
	ASSERT_NE( pA, pB );
	GrannySetControlLoopCount( pA, 1 );
	GrannySetControlLoopCount( pB, 1 );

	// A holds local clock 0 and B holds local clock 1, so they disagree about
	// where the bone is: 0 against 50.
	GrannySetModelClock( pInstance, 0.0f );
	GrannySetControlRawLocalClock( pA, 0.0f );
	GrannySetControlRawLocalClock( pB, 1.0f );
	ASSERT_FLOAT_EQ( 0.0f, GrannyGetControlClampedLocalClock( pA ) );
	ASSERT_FLOAT_EQ( 1.0f, GrannyGetControlClampedLocalClock( pB ) );

	GrannySampleModelAnimations( pInstance, 0, 1, pPose );
	// granny_transform is opaque in the public header, so the test reads it as
	// the layout Structures.h asserts, the same way the engine reads granny211.h's.
	const NGr2::STransform *pBone = reinterpret_cast<const NGr2::STransform *>(
		GrannyGetLocalPoseTransform( pPose, 0 ) );
	ASSERT_NE( nullptr, pBone );
	EXPECT_NEAR( 25.0f, pBone->Position[0], 1e-4f ) << "equal weights, so the mean";

	// A third of the weight to B, so the mean leans towards A.
	GrannySetControlEaseIn( pB, true );
	GrannySetControlEaseInCurve( pB, -1.0f, 1.0e6f, 1.0f / 3.0f, 1.0f / 3.0f,
	                             1.0f / 3.0f, 1.0f / 3.0f );
	GrannySampleModelAnimations( pInstance, 0, 1, pPose );
	const float fWeightB = GrannyGetControlEffectiveWeight( pB );
	ASSERT_LT( fWeightB, 0.5f );
	EXPECT_NEAR( ( 0.0f * 1.0f + 50.0f * fWeightB ) / ( 1.0f + fWeightB ),
	             pBone->Position[0], 1e-3f );

	// One control at a low weight is still its animation at full strength, since
	// the total is what normalises it. This is the reading that a single blend
	// cannot distinguish and two can.
	GrannySetControlActive( pA, false );
	GrannySampleModelAnimations( pInstance, 0, 1, pPose );
	EXPECT_NEAR( 50.0f, pBone->Position[0], 1e-3f );

	// With everything deactivated the rest pose stands.
	GrannySetControlActive( pB, false );
	GrannySampleModelAnimations( pInstance, 0, 1, pPose );
	EXPECT_FLOAT_EQ( 100.0f, pBone->Position[0] ) << "bone 0's own bind position";

	GrannyFreeLocalPose( pPose );
	GrannyFreeModelInstance( pInstance );
}

TEST( Control, BelowAWeightOfATenthTheBindPoseMakesUpTheDifference )
{
	// Measured as a floor at exactly 0.2, over 222 samples across 60 files: below
	// a total weight of 0.2 the shortfall goes to the bone's rest pose, so the
	// result is the bind pose a fraction total/0.2 of the way toward the
	// animation. Above the floor the total normalises and the animation stands
	// alone, which is why every measurement made with two controls missed this:
	// their weights summed past it.
	CClip clip( 1, 2.0f );
	clip.SetLinearPosition( 0, 0.0f, 100.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_local_pose *pPose = GrannyNewLocalPose( 1 );
	granny_control *pControl = Bind( clip, pInstance );
	ASSERT_NE( nullptr, pControl );
	GrannySetControlLoopCount( pControl, 1 );
	GrannySetControlEaseIn( pControl, true );

	GrannySetModelClock( pInstance, 1.0f );
	const NGr2::STransform *pBone = reinterpret_cast<const NGr2::STransform *>(
		GrannyGetLocalPoseTransform( pPose, 0 ) );
	const float fRest = 100.0f;
	const float fAnimated = 50.0f;

	for ( float fAsked : { 0.05f, 0.1f, 0.15f } )
	{
		GrannySetControlEaseInCurve( pControl, -1.0f, 1.0e6f, fAsked, fAsked, fAsked,
		                             fAsked );
		GrannySampleModelAnimations( pInstance, 0, 1, pPose );
		const float fWeight = GrannyGetControlEffectiveWeight( pControl );
		const float fFraction = fWeight / 0.2f;
		EXPECT_NEAR( fRest + ( fAnimated - fRest ) * fFraction, pBone->Position[0],
		             1e-3f )
			<< "at an effective weight of " << fWeight;
	}

	// At and above the floor the animation stands alone.
	for ( float fAsked : { 0.2f, 0.5f, 1.0f } )
	{
		GrannySetControlEaseInCurve( pControl, -1.0f, 1.0e6f, fAsked, fAsked, fAsked,
		                             fAsked );
		GrannySampleModelAnimations( pInstance, 0, 1, pPose );
		EXPECT_NEAR( fAnimated, pBone->Position[0], 1e-3f )
			<< "at an asked weight of " << fAsked;
	}
}

TEST( Control, TheBindPoseBringsItsFlagsInBelowTheFloor )
{
	// A bone whose track speaks only for one part of its transform reads back
	// only that part's flag, until the floor brings the bind pose in and its
	// flags with it. Measured: the DLL reports position and orientation where an
	// implementation that kept the track's flags alone reports orientation only.
	CClip clip( 1, 2.0f );
	clip.SetLinearPosition( 0, 0.0f, 100.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_local_pose *pPose = GrannyNewLocalPose( 1 );
	granny_control *pControl = Bind( clip, pInstance );
	GrannySetControlLoopCount( pControl, 1 );
	GrannySetControlEaseIn( pControl, true );
	GrannySetModelClock( pInstance, 1.0f );

	const NGr2::STransform *pBone = reinterpret_cast<const NGr2::STransform *>(
		GrannyGetLocalPoseTransform( pPose, 0 ) );

	// The fixture's bone has a position curve only, and its bind transform is
	// flagged for position, so both are the same bit here; what the test pins is
	// that the flags are the union rather than either one alone.
	GrannySetControlEaseInCurve( pControl, -1.0f, 1.0e6f, 0.05f, 0.05f, 0.05f, 0.05f );
	GrannySampleModelAnimations( pInstance, 0, 1, pPose );
	EXPECT_EQ( NGr2::TRANSFORM_HAS_POSITION, pBone->nFlags );

	GrannyFreeLocalPose( pPose );
	GrannyFreeModelInstance( pInstance );
}

TEST( Control, ALoopingClipIdentifiesItsLastControlWithItsFirst )
{
	// Which is right for this game's data: every multi-knot curve in the corpus
	// ends exactly at its animation's duration, so the last key and the first are
	// the same keyframe. A two-key curve under looping is therefore constant.
	CClip clip( 1, 2.0f );
	clip.SetLinearPosition( 0, 0.0f, 100.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_local_pose *pPose = GrannyNewLocalPose( 1 );
	granny_control *pControl = Bind( clip, pInstance );
	ASSERT_NE( nullptr, pControl );
	const NGr2::STransform *pBone = reinterpret_cast<const NGr2::STransform *>(
		GrannyGetLocalPoseTransform( pPose, 0 ) );

	GrannySetControlLoopCount( pControl, 1 );
	GrannySetModelClock( pInstance, 1.0f );
	GrannySampleModelAnimations( pInstance, 0, 1, pPose );
	EXPECT_NEAR( 50.0f, pBone->Position[0], 1e-4f ) << "halfway, not looping";

	GrannySetControlLoopCount( pControl, 0 );
	GrannySampleModelAnimations( pInstance, 0, 1, pPose );
	EXPECT_NEAR( 0.0f, pBone->Position[0], 1e-4f )
		<< "looping, so the second key is the first and the curve goes nowhere";

	GrannyFreeLocalPose( pPose );
	GrannyFreeModelInstance( pInstance );
}

TEST( Control, ATrackWithAnEmptyCurveNeutralisesThatPartRatherThanKeepingTheBindPose )
{
	// Measured: a bone whose track carries an orientation curve and no position
	// curve comes back with a position of exactly zero, not with its bind
	// translation. A track that mentions a bone at all replaces every part of its
	// transform, and the parts it says nothing about become neutral.
	CClip clip( 2, 2.0f );
	clip.SetLinearPosition( 0, 10.0f, 20.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_local_pose *pPose = GrannyNewLocalPose( 2 );
	granny_control *pControl = Bind( clip, pInstance );
	ASSERT_NE( nullptr, pControl );
	GrannySetControlLoopCount( pControl, 0 );

	GrannySetModelClock( pInstance, 0.0f );
	GrannySampleModelAnimations( pInstance, 0, 2, pPose );

	const NGr2::STransform *pFirst = reinterpret_cast<const NGr2::STransform *>(
		GrannyGetLocalPoseTransform( pPose, 0 ) );
	ASSERT_NE( nullptr, pFirst );
	EXPECT_FLOAT_EQ( 10.0f, pFirst->Position[0] );
	EXPECT_EQ( NGr2::TRANSFORM_HAS_POSITION, pFirst->nFlags )
		<< "only the position curve has keys";

	// Bone 1's track has no curve at all, so every part is neutral and its flags
	// are zero.
	const NGr2::STransform *pSecond = reinterpret_cast<const NGr2::STransform *>(
		GrannyGetLocalPoseTransform( pPose, 1 ) );
	ASSERT_NE( nullptr, pSecond );
	EXPECT_FLOAT_EQ( 0.0f, pSecond->Position[0] ) << "not the bind position of 101";
	EXPECT_EQ( 0u, pSecond->nFlags );

	GrannyFreeLocalPose( pPose );
	GrannyFreeModelInstance( pInstance );
}

TEST( Control, NullsAreReturnsRatherThanCrashes )
{
	EXPECT_FALSE( GrannyControlIsComplete( nullptr ) );
	EXPECT_FLOAT_EQ( 0.0f, GrannyGetControlClampedLocalClock( nullptr ) );
	EXPECT_FLOAT_EQ( 0.0f, GrannyGetControlDuration( nullptr ) );
	EXPECT_FLOAT_EQ( 0.0f, GrannyGetControlDurationLeft( nullptr ) );
	EXPECT_FLOAT_EQ( 0.0f, GrannyGetControlEffectiveWeight( nullptr ) );
	EXPECT_FLOAT_EQ( 0.0f, GrannyGetControlSpeed( nullptr ) );
	EXPECT_FLOAT_EQ( 0.0f, GrannyEaseControlIn( nullptr, 1.0f, false ) );
	EXPECT_FLOAT_EQ( 0.0f, GrannyEaseControlOut( nullptr, 1.0f ) );
	GrannySetControlSpeed( nullptr, 2.0f );
	GrannySetControlLoopCount( nullptr, 3 );
	GrannySetControlActive( nullptr, false );
	GrannyCompleteControlAt( nullptr, 1.0f );
	GrannySetControlRawLocalClock( nullptr, 1.0f );
	GrannyFreeControl( nullptr );
	GrannyFreeControlOnceUnused( nullptr );
	GrannySetTrackGroupTarget( nullptr, 0, nullptr );
	GrannySetTrackGroupModelMask( nullptr, 0, nullptr );
	GrannySetSkeletonTrackMaskFromTrackGroup( nullptr, nullptr, nullptr, 1, 1, 1 );
}

TEST( Control, FreeingAControlUnbindsItFirst )
{
	// Or the next sample walks a dangling pointer and freeing the instance frees
	// it twice.
	CClip clip( 1, 2.0f );
	clip.SetLinearPosition( 0, 10.0f, 20.0f );
	granny_model_instance *pInstance = GrannyInstantiateModel( clip.Model() );
	granny_local_pose *pPose = GrannyNewLocalPose( 1 );
	granny_control *pA = Bind( clip, pInstance );
	granny_control *pB = Bind( clip, pInstance );
	ASSERT_NE( nullptr, pA );
	ASSERT_NE( nullptr, pB );

	GrannyFreeControl( pA );
	GrannySetModelClock( pInstance, 0.0f );
	GrannySampleModelAnimations( pInstance, 0, 1, pPose );
	const NGr2::STransform *pBone = reinterpret_cast<const NGr2::STransform *>(
		GrannyGetLocalPoseTransform( pPose, 0 ) );
	ASSERT_NE( nullptr, pBone );
	EXPECT_FLOAT_EQ( 10.0f, pBone->Position[0] ) << "B alone still plays";

	GrannyFreeLocalPose( pPose );
	GrannyFreeModelInstance( pInstance );
}

TEST( Control, ATrackMaskWeightsBonesIndividually )
{
	// The engine passes one for all three values, so every bone gets weight one
	// and none of this shows in the game. It is here because a mask with the
	// wrong shape would be invisible until some other caller used one.
	CClip clip( 3, 2.0f );
	clip.SetLinearPosition( 1, 0.0f, 10.0f );
	granny_track_mask *pMask = GrannyNewTrackMask( 0.25f, 3 );
	ASSERT_NE( nullptr, pMask );
	EXPECT_FLOAT_EQ( 0.25f, pMask->WeightFor( 0 ) );
	EXPECT_FLOAT_EQ( 0.25f, pMask->WeightFor( 99 ) ) << "out of range is the default";

	GrannySetSkeletonTrackMaskFromTrackGroup( pMask, clip.Skeleton(), clip.Group(),
	                                          0.0f, 0.5f, 1.0f );
	// Bone 1 has a two-knot curve, so it is animated; the others have no curves
	// at all, so their tracks are the identity.
	EXPECT_FLOAT_EQ( 0.0f, pMask->WeightFor( 0 ) );
	EXPECT_FLOAT_EQ( 1.0f, pMask->WeightFor( 1 ) );
	EXPECT_FLOAT_EQ( 0.0f, pMask->WeightFor( 2 ) );
}
