#include "stdafx.h"

#include "GltfAnimation.h"

#include "DBScene.h"
#include "GAnimUtils.h"

#include <fastgltf/tools.hpp>

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace NAnimation
{
namespace
{
bool GetAnimationTimeRange( const NGltf::CGltfFile &file,
	const fastgltf::Animation &animation, float *pStart, float *pEnd )
{
	bool haveTimes = false;
	for ( const fastgltf::AnimationSampler &sampler : animation.samplers )
	{
		const std::vector<float> &times = file.ScalarAccessor( sampler.inputAccessor );
		if ( times.empty() )
			continue;
		if ( !haveTimes )
		{
			*pStart = times.front();
			*pEnd = times.back();
			haveTimes = true;
		}
		else
		{
			*pStart = (std::min)( *pStart, times.front() );
			*pEnd = (std::max)( *pEnd, times.back() );
		}
	}
	return haveTimes && *pEnd - *pStart > FP_EPSILON;
}

bool InferBakedFrameTimeline( const NGltf::CGltfFile &file,
	const fastgltf::Animation &animation, float animationStart, float animationEnd,
	float *pTimelineStart, float *pSecondsPerFrame, std::size_t *pFrameCount )
{
	std::unordered_set<std::size_t> visitedAccessors;
	std::size_t bestFrameCount = 0;
	for ( const fastgltf::AnimationSampler &sampler : animation.samplers )
	{
		if ( !visitedAccessors.insert(sampler.inputAccessor).second )
			continue;
		const std::vector<float> &times = file.ScalarAccessor( sampler.inputAccessor );
		if ( times.size() < 3 || times.back() - times.front() <= FP_EPSILON )
			continue;

		const float secondsPerFrame = (times.back() - times.front()) /
			static_cast<float>(times.size() - 1);
		const float tolerance = (std::max)( 0.00001f, secondsPerFrame * 0.001f );
		if ( fabsf(times.front() - animationStart) > tolerance ||
			fabsf(times.back() - animationEnd) > tolerance )
			continue;

		bool uniformlySampled = true;
		for ( std::size_t i = 1; i < times.size(); ++i )
		{
			const float delta = times[i] - times[i - 1];
			if ( delta <= 0.0f || fabsf(delta - secondsPerFrame) > tolerance )
			{
				uniformlySampled = false;
				break;
			}
		}
		if ( uniformlySampled && times.size() > bestFrameCount )
		{
			// Blender's sampled export produces one timestamp per source frame. The
			// densest full-duration sampler therefore defines the shared frame clock.
			bestFrameCount = times.size();
			*pTimelineStart = times.front();
			*pSecondsPerFrame = secondsPerFrame;
		}
	}
	*pFrameCount = bestFrameCount;
	return bestFrameCount != 0;
}

struct SKeyInterval
{
	std::size_t first;
	std::size_t second;
	float factor;
	float delta;
};

SKeyInterval FindInterval( const std::vector<float> &times, float time )
{
	if ( times.size() < 2 || time <= times.front() )
		return SKeyInterval{ 0, 0, 0.0f, 0.0f };
	if ( time >= times.back() )
		return SKeyInterval{ times.size() - 1, times.size() - 1, 0.0f, 0.0f };
	const auto upper = std::upper_bound( times.begin(), times.end(), time );
	const std::size_t second = static_cast<std::size_t>(upper - times.begin());
	const std::size_t first = second - 1;
	const float delta = times[second] - times[first];
	return SKeyInterval{ first, second, delta > FP_EPSILON ? (time - times[first]) / delta : 0.0f, delta };
}

float Hermite( float p0, float out0, float p1, float in1, float t, float delta )
{
	const float t2 = t * t;
	const float t3 = t2 * t;
	return (2.0f * t3 - 3.0f * t2 + 1.0f) * p0
		+ (t3 - 2.0f * t2 + t) * delta * out0
		+ (-2.0f * t3 + 3.0f * t2) * p1
		+ (t3 - t2) * delta * in1;
}

fastgltf::math::fvec3 SampleVector( const std::vector<fastgltf::math::fvec3> &values,
	const SKeyInterval &key, fastgltf::AnimationInterpolation interpolation )
{
	if ( values.empty() )
		return fastgltf::math::fvec3( 0.0f );
	if ( interpolation == fastgltf::AnimationInterpolation::CubicSpline )
	{
		const std::size_t first = key.first * 3;
		const std::size_t second = key.second * 3;
		if ( second + 1 >= values.size() )
			return values[(std::min)(first + 1, values.size() - 1)];
		fastgltf::math::fvec3 result;
		for ( int i = 0; i < 3; ++i )
			result[i] = Hermite( values[first + 1][i], values[first + 2][i],
				values[second + 1][i], values[second][i], key.factor, key.delta );
		return result;
	}
	const fastgltf::math::fvec3 &first = values[(std::min)(key.first, values.size() - 1)];
	const fastgltf::math::fvec3 &second = values[(std::min)(key.second, values.size() - 1)];
	if ( interpolation == fastgltf::AnimationInterpolation::Step || key.first == key.second )
		return first;
	return first * (1.0f - key.factor) + second * key.factor;
}

CQuat RawQuaternion( const fastgltf::math::fvec4 &value, bool normalize )
{
	CQuat result;
	result.FromComponents( -value[0], -value[2], -value[1], value[3] );
	if ( normalize )
		result.Normalize();
	return result;
}

CQuat SampleQuaternion( const std::vector<fastgltf::math::fvec4> &values,
	const SKeyInterval &key, fastgltf::AnimationInterpolation interpolation )
{
	if ( values.empty() )
		return QNULL;
	if ( interpolation == fastgltf::AnimationInterpolation::CubicSpline )
	{
		const std::size_t first = key.first * 3;
		const std::size_t second = key.second * 3;
		if ( second + 1 >= values.size() )
			return RawQuaternion( values[(std::min)(first + 1, values.size() - 1)], true );
		fastgltf::math::fvec4 result;
		for ( int i = 0; i < 4; ++i )
			result[i] = Hermite( values[first + 1][i], values[first + 2][i],
				values[second + 1][i], values[second][i], key.factor, key.delta );
		return RawQuaternion( result, true );
	}
	const CQuat first = RawQuaternion( values[(std::min)(key.first, values.size() - 1)], true );
	if ( interpolation == fastgltf::AnimationInterpolation::Step || key.first == key.second )
		return first;
	const CQuat second = RawQuaternion( values[(std::min)(key.second, values.size() - 1)], true );
	CQuat result;
	result.Interpolate( first, second, key.factor );
	result.Normalize();
	return result;
}

SHMatrix GlobalMatrix( const SSkeletonPose &pose )
{
	SHMatrix result;
	result.Set(
		pose.poseGlobal[0], pose.poseGlobal[4], pose.poseGlobal[8], pose.poseGlobal[12],
		pose.poseGlobal[1], pose.poseGlobal[5], pose.poseGlobal[9], pose.poseGlobal[13],
		pose.poseGlobal[2], pose.poseGlobal[6], pose.poseGlobal[10], pose.poseGlobal[14],
		pose.poseGlobal[3], pose.poseGlobal[7], pose.poseGlobal[11], pose.poseGlobal[15] );
	return result;
}

void BuildBoneWorld( std::size_t boneIndex, const NGltf::SSkeletonDefinition &skeleton,
	const SSkeletonPose &pose, const SHMatrix &global,
	std::vector<unsigned char> *pState, NGScene::SSkeletonMatrices *pWorld )
{
	if ( boneIndex >= pose.localPose.size() || (*pState)[boneIndex] == 2 )
		return;
	const SHMatrix local = NGltf::MakeLocalMatrix( pose.localPose[boneIndex] );
	if ( (*pState)[boneIndex] == 1 )
	{
		// Invalid cyclic skin hierarchies are detached instead of reading an
		// uninitialized parent matrix.
		(*pWorld)[boneIndex] = global * local;
		(*pState)[boneIndex] = 2;
		return;
	}
	(*pState)[boneIndex] = 1;
	const int parent = boneIndex < skeleton.parents.size() ? skeleton.parents[boneIndex] : -1;
	if ( parent >= 0 && parent < static_cast<int>(pose.localPose.size()) )
	{
		BuildBoneWorld( static_cast<std::size_t>(parent), skeleton, pose, global, pState, pWorld );
		(*pWorld)[boneIndex] = (*pWorld)[parent] * local;
	}
	else
		(*pWorld)[boneIndex] = global * local;
	(*pState)[boneIndex] = 2;
}
}

CGltfSkeletonAnimator::SAnimationHolder::SAnimationHolder() :
	tStartTime(0), tEndTime(-1), tFadeStart(0), tFadeDuration(0),
	fSpeed(1.0f), fWeight(1.0f), fDuration(0.0f), fSourceStart(0.0f),
	bFadeIn(false), bFadeOut(false), nLoopCount(1)
{
}

int CGltfSkeletonAnimator::SAnimationHolder::operator&( IBinSaver &f )
{
	f.Add( 2, &tStartTime );
	f.Add( 3, &tEndTime );
	f.Add( 4, &tFadeStart );
	f.Add( 5, &tFadeDuration );
	f.Add( 6, &fSpeed );
	f.Add( 7, &fWeight );
	f.Add( 8, &fDuration );
	f.Add( 9, &bFadeIn );
	f.Add( 10, &bFadeOut );
	f.Add( 11, &nLoopCount );
	f.Add( 12, &hAnimation );
	return 0;
}

CGltfSkeletonAnimator::CGltfSkeletonAnimator() :
	bBoneMutatorsEnabled(false), bSmthChanged(true), bJustLoaded(false),
	nAnimWithMovement(-1), fGlobalMovementSpeed(0.0f), tTransitDuration(0)
{
}

CGltfSkeletonAnimator::CGltfSkeletonAnimator( const SSkeletonHandle &_skeletonH,
	CFuncBase<STime> *_pTime ) :
	bBoneMutatorsEnabled(false), bSmthChanged(true), bJustLoaded(false),
	nAnimWithMovement(-1), fGlobalMovementSpeed(0.0f), tTransitDuration(0)
{
	Create( _skeletonH, _pTime );
	SHMatrix identity;
	Identity( &identity );
	SetGlobalPositionInternal( identity );
}

void CGltfSkeletonAnimator::Create( const SSkeletonHandle &_skeletonH, CFuncBase<STime> *_pTime )
{
	skeletonH = _skeletonH;
	pTime = _pTime;
	skeleton = NGltf::SSkeletonDefinition();
	pSkeletonFile.reset();
	if ( !_skeletonH.pSkeleton || _skeletonH.pSkeleton->szModelFileRef.empty() )
		return;
	pSkeletonFile = NGltf::LoadFile( _skeletonH.pSkeleton,
		_skeletonH.pSkeleton->szModelFileRef );
	if ( !NGltf::BuildSkeleton(pSkeletonFile, _skeletonH.pSkeleton->szRootJoint,
		_skeletonH.nModelInFile, &skeleton) )
	{
		if ( _skeletonH.pSkeleton->szRootJoint.empty() )
			DebugTrace( "glTF: could not create skin %d from %s", _skeletonH.nModelInFile,
				_skeletonH.pSkeleton->szModelFileRef.c_str() );
		else
			DebugTrace( "glTF: could not create skeleton for RootJoint %s from %s",
				_skeletonH.pSkeleton->szRootJoint.c_str(),
				_skeletonH.pSkeleton->szModelFileRef.c_str() );
		pSkeletonFile.reset();
		return;
	}
	value.localPose = skeleton.restPose;
	value.worldPose.resize( skeleton.restPose.size() );
	value.compositePose.resize( skeleton.restPose.size() );
	boneMutators.resize( skeleton.restPose.size() );
	for ( SSimpleBoneMutator &mutator : boneMutators )
		mutator.Enable( false );
	for ( SAnimationHolder &holder : animations )
		BindAnimation( &holder );
	bJustLoaded = false;
	bSmthChanged = true;
}

bool CGltfSkeletonAnimator::BindAnimation( SAnimationHolder *pHolder )
{
	pHolder->boundChannels.clear();
	if ( !SelectAnimationRange(pHolder) )
	{
		return false;
	}
	ResolveBoundChannels( pHolder );
	return true;
}

// Reduce the selected animations to the channels this skeleton can actually drive, with the
// target bone resolved and the keyframe data decoded. Everything here depends only on the
// document and the skeleton, so none of it has to be repeated while the animation plays.
void CGltfSkeletonAnimator::ResolveBoundChannels( SAnimationHolder *pHolder )
{
	const NGltf::CGltfFile &file = *pHolder->pFile;
	for ( int animationIndex : pHolder->animationIndices )
	{
		if ( animationIndex < 0 ||
			animationIndex >= static_cast<int>(file.asset.animations.size()) )
		{
			continue;
		}
		const fastgltf::Animation &animation = file.asset.animations[animationIndex];
		for ( const fastgltf::AnimationChannel &channel : animation.channels )
		{
			if ( !channel.nodeIndex.has_value() || *channel.nodeIndex >= file.asset.nodes.size() ||
				channel.samplerIndex >= animation.samplers.size() )
			{
				continue;
			}
			const fastgltf::Node &node = file.asset.nodes[*channel.nodeIndex];
			const std::string nodeName = node.name.empty()
				? "Node_" + std::to_string(*channel.nodeIndex) : std::string(node.name);
			const int nBone = skeleton.FindBone( nodeName );
			if ( nBone < 0 )
			{
				continue;
			}

			const fastgltf::AnimationSampler &sampler = animation.samplers[channel.samplerIndex];
			const std::vector<float> &times = file.ScalarAccessor( sampler.inputAccessor );
			if ( times.empty() )
			{
				continue;
			}

			SBoundChannel bound;
			bound.nBone = nBone;
			bound.path = channel.path;
			bound.interpolation = sampler.interpolation;
			bound.pTimes = &times;
			if ( channel.path == fastgltf::AnimationPath::Translation ||
				channel.path == fastgltf::AnimationPath::Scale )
			{
				bound.pVectors = &file.Vec3Accessor( sampler.outputAccessor );
			}
			else if ( channel.path == fastgltf::AnimationPath::Rotation )
			{
				bound.pRotations = &file.Vec4Accessor( sampler.outputAccessor );
			}
			else
			{
				continue;               // morph weights have no engine representation
			}
			pHolder->boundChannels.push_back( bound );
		}
	}
}

bool CGltfSkeletonAnimator::SelectAnimationRange( SAnimationHolder *pHolder )
{
	pHolder->pFile.reset();
	pHolder->animationIndices.clear();
	pHolder->fSourceStart = 0.0f;
	pHolder->fDuration = 0.0f;
	if ( !pHolder->hAnimation.pAnimFile ||
		pHolder->hAnimation.pAnimFile->GetModelFileRef().empty() )
		return false;
	pHolder->pFile = NGltf::LoadFile( pHolder->hAnimation.pAnimFile,
		pHolder->hAnimation.pAnimFile->GetModelFileRef() );
	if ( !pHolder->pFile || pHolder->pFile->asset.animations.empty() )
		return false;

	const std::string &clipName = pHolder->hAnimation.pAnimFile->GetClipName();
	const int firstFrame = pHolder->hAnimation.pAnimFile->GetFirstFrame();
	const int lastFrame = pHolder->hAnimation.pAnimFile->GetLastFrame();
	const bool rangeWasSpecified = firstFrame != 0 || lastFrame != 0;
	const bool bNodeSkeleton = pSkeletonFile && pSkeletonFile->asset.skins.empty();
	if ( !clipName.empty() )
	{
		for ( std::size_t i = 0; i < pHolder->pFile->asset.animations.size(); ++i )
		{
			if ( std::string(pHolder->pFile->asset.animations[i].name) != clipName )
				continue;
			pHolder->animationIndices.push_back( static_cast<int>(i) );
		}
		if ( pHolder->animationIndices.empty() )
		{
			DebugTrace( "glTF: animation %s was not found in %s", clipName.c_str(),
				pHolder->pFile->sourcePath.c_str() );
			return false;
		}
		if ( pHolder->animationIndices.size() > 1 && !bNodeSkeleton )
		{
			DebugTrace( "glTF: animation name %s is ambiguous in %s", clipName.c_str(),
				pHolder->pFile->sourcePath.c_str() );
			return false;
		}
	}
	else
	{
		pHolder->animationIndices.push_back( 0 );
		if ( bNodeSkeleton && rangeWasSpecified )
		{
			// Blender may store one action per animated object. Frame slicing
			// denotes a shared timeline, so compose all of those records.
			for ( std::size_t i = 1; i < pHolder->pFile->asset.animations.size(); ++i )
				pHolder->animationIndices.push_back( static_cast<int>(i) );
		}
	}

	bool haveTimes = false;
	float animationStart = 0.0f;
	float animationEnd = 0.0f;
	for ( int animationIndex : pHolder->animationIndices )
	{
		const fastgltf::Animation &animation =
			pHolder->pFile->asset.animations[animationIndex];
		float localStart = 0.0f;
		float localEnd = 0.0f;
		if ( GetAnimationTimeRange(*pHolder->pFile, animation, &localStart, &localEnd) )
		{
			if ( !haveTimes )
			{
				animationStart = localStart;
				animationEnd = localEnd;
				haveTimes = true;
			}
			else
			{
				animationStart = (std::min)( animationStart, localStart );
				animationEnd = (std::max)( animationEnd, localEnd );
			}
		}
	}
	if ( !haveTimes )
	{
		DebugTrace( "glTF: selected animation in %s has no usable time range",
			pHolder->pFile->sourcePath.c_str() );
		return false;
	}

	pHolder->fSourceStart = animationStart;
	pHolder->fDuration = animationEnd - animationStart;
	if ( !clipName.empty() )
		return true;

	float timelineStart = 0.0f;
	float secondsPerFrame = 0.0f;
	std::size_t frameCount = 0;
	for ( int animationIndex : pHolder->animationIndices )
	{
		const fastgltf::Animation &animation =
			pHolder->pFile->asset.animations[animationIndex];
		float localStart = 0.0f;
		float localEnd = 0.0f;
		if ( !GetAnimationTimeRange(*pHolder->pFile, animation, &localStart, &localEnd) )
		{
			continue;
		}
		float candidateStart = 0.0f;
		float candidateSecondsPerFrame = 0.0f;
		std::size_t candidateFrameCount = 0;
		if ( InferBakedFrameTimeline(*pHolder->pFile, animation, localStart, localEnd,
			&candidateStart, &candidateSecondsPerFrame, &candidateFrameCount) &&
			candidateFrameCount > frameCount )
		{
			timelineStart = candidateStart;
			secondsPerFrame = candidateSecondsPerFrame;
			frameCount = candidateFrameCount;
		}
	}
	const bool haveBakedTimeline = frameCount != 0;
	int timelineFirstFrame = 0;
	if ( haveBakedTimeline && secondsPerFrame > FP_EPSILON )
	{
		// Blender writes timestamps on its absolute frame clock. For example, an
		// export starting at frame 1 begins at 1 / FPS, not at logical frame zero.
		const float firstFrameEstimate = timelineStart / secondsPerFrame;
		const int roundedFirstFrame = static_cast<int>(floorf(firstFrameEstimate + 0.5f));
		const float tolerance = (std::max)( 0.00001f, secondsPerFrame * 0.001f );
		if ( fabsf(timelineStart - roundedFirstFrame * secondsPerFrame) <= tolerance )
			timelineFirstFrame = roundedFirstFrame;
	}
	const int timelineLastFrame = timelineFirstFrame + static_cast<int>(frameCount) - 1;
	if ( lastFrame > firstFrame && firstFrame >= 0 && haveBakedTimeline &&
		firstFrame >= timelineFirstFrame && lastFrame <= timelineLastFrame )
	{
		// FirstFrame/LastFrame address the Blender frame clock directly. Length is
		// gameplay metadata and must never change which GLB poses are sampled.
		pHolder->fSourceStart = timelineStart +
			(firstFrame - timelineFirstFrame) * secondsPerFrame;
		const float sourceEnd = timelineStart +
			(lastFrame - timelineFirstFrame) * secondsPerFrame;
		pHolder->fDuration = sourceEnd - pHolder->fSourceStart;
		return pHolder->fDuration > FP_EPSILON;
	}
	if ( rangeWasSpecified )
	{
		if ( !haveBakedTimeline )
			DebugTrace( "glTF: cannot infer a baked frame timeline in %s; export with animation sampling enabled or use ClipName",
				pHolder->pFile->sourcePath.c_str() );
		else
			DebugTrace( "glTF: invalid frame range %d..%d for the inferred %d..%d Blender timeline in %s; using the full selected animation",
				firstFrame, lastFrame, timelineFirstFrame, timelineLastFrame,
				pHolder->pFile->sourcePath.c_str() );
	}
	return true;
}

float CGltfSkeletonAnimator::GetEffectiveWeight( const SAnimationHolder &holder, STime time ) const
{
	float weight = holder.fWeight;
	if ( holder.bFadeIn && holder.tFadeDuration > 0 )
		weight *= (std::clamp)( (time - holder.tFadeStart) / static_cast<float>(holder.tFadeDuration), 0.0f, 1.0f );
	if ( holder.bFadeOut )
	{
		if ( holder.tFadeDuration <= 0 )
			return 0.0f;
		weight *= 1.0f - (std::clamp)( (time - holder.tFadeStart) / static_cast<float>(holder.tFadeDuration), 0.0f, 1.0f );
	}
	if ( tTransitDuration > 0 )
	{
		weight *= (std::clamp)( (time - holder.tStartTime) / static_cast<float>(tTransitDuration), 0.0f, 1.0f );
		if ( holder.tEndTime != -1 )
			weight *= (std::clamp)( (holder.tEndTime - time) / static_cast<float>(tTransitDuration), 0.0f, 1.0f );
	}
	return (std::clamp)( weight, 0.0f, 1.0f );
}

float CGltfSkeletonAnimator::GetLocalTime( const SAnimationHolder &holder, STime time, bool *pActive ) const
{
	*pActive = holder.pFile && time >= holder.tStartTime &&
		(holder.tEndTime == -1 || time <= holder.tEndTime) && holder.fDuration > 0.0f;
	if ( !*pActive )
		return 0.0f;
	float local = (time - holder.tStartTime) * 0.001f * holder.fSpeed;
	if ( holder.nLoopCount == 0 )
		return fmodf( (std::max)(local, 0.0f), holder.fDuration );
	const float total = holder.fDuration * holder.nLoopCount;
	if ( local >= total )
	{
		// Completed non-looping Granny controls continue contributing their
		// clamped final sample until explicitly cleared or faded out.
		return holder.fDuration;
	}
	if ( holder.nLoopCount > 1 )
		local = fmodf( (std::max)(local, 0.0f), holder.fDuration );
	return (std::clamp)( local, 0.0f, holder.fDuration );
}

void CGltfSkeletonAnimator::SetGlobalPositionInternal( const SHMatrix &m )
{
	value.poseGlobal[0] = m._11; value.poseGlobal[4] = m._12; value.poseGlobal[8] = m._13; value.poseGlobal[12] = m._14;
	value.poseGlobal[1] = m._21; value.poseGlobal[5] = m._22; value.poseGlobal[9] = m._23; value.poseGlobal[13] = m._24;
	value.poseGlobal[2] = m._31; value.poseGlobal[6] = m._32; value.poseGlobal[10] = m._33; value.poseGlobal[14] = m._34;
	value.poseGlobal[3] = m._41; value.poseGlobal[7] = m._42; value.poseGlobal[11] = m._43; value.poseGlobal[15] = m._44;
}

void CGltfSkeletonAnimator::ApplyAnimation( const SAnimationHolder &holder, float localTime, float weight )
{
	const float sourceTime = holder.fSourceStart + localTime;
	for ( const SBoundChannel &channel : holder.boundChannels )
	{
		SBoneTransform *pBone = value.GetBone( channel.nBone );
		if ( !pBone )
		{
			continue;
		}

		const SKeyInterval key = FindInterval( *channel.pTimes, sourceTime );
		if ( channel.path == fastgltf::AnimationPath::Translation ||
			channel.path == fastgltf::AnimationPath::Scale )
		{
			const fastgltf::math::fvec3 sampled =
				SampleVector( *channel.pVectors, key, channel.interpolation );
			if ( channel.path == fastgltf::AnimationPath::Translation )
			{
				const CVec3 converted = NGltf::ConvertPosition( sampled );
				pBone->Position[0] += (converted.x - pBone->Position[0]) * weight;
				pBone->Position[1] += (converted.y - pBone->Position[1]) * weight;
				pBone->Position[2] += (converted.z - pBone->Position[2]) * weight;
			}
			else
			{
				const CVec3 converted = NGltf::ConvertScale( sampled );
				pBone->ScaleShear[0][0] += (converted.x - pBone->ScaleShear[0][0]) * weight;
				pBone->ScaleShear[1][1] += (converted.y - pBone->ScaleShear[1][1]) * weight;
				pBone->ScaleShear[2][2] += (converted.z - pBone->ScaleShear[2][2]) * weight;
			}
		}
		else if ( channel.path == fastgltf::AnimationPath::Rotation )
		{
			const CQuat sampled =
				SampleQuaternion( *channel.pRotations, key, channel.interpolation );
			CQuat current;
			current.FromComponents( pBone->Orientation[0], pBone->Orientation[1],
				pBone->Orientation[2], pBone->Orientation[3] );
			CQuat blended;
			blended.Interpolate( current, sampled, weight );
			blended.Normalize();
			memcpy( pBone->Orientation, &blended, sizeof(pBone->Orientation) );
		}
	}
}

void CGltfSkeletonAnimator::ApplyBoneMutators( STime time )
{
	if ( !bBoneMutatorsEnabled )
		return;
	for ( int i = 0; i < static_cast<int>(boneMutators.size()); ++i )
	{
		if ( !boneMutators[i].IsEnabled() )
			continue;
		CQuat deltaRotation;
		CVec3 deltaPosition;
		boneMutators[i].GetAtTime( time, &deltaRotation, &deltaPosition );
		SBoneTransform *pBone = value.GetBone( i );
		CQuat current;
		current.FromComponents( pBone->Orientation[0], pBone->Orientation[1],
			pBone->Orientation[2], pBone->Orientation[3] );

		// Match GrannyPostMultiplyBy exactly: mutation translation is expressed
		// in the bone's current local frame, followed by current * delta rotation.
		const CVec3 scaledDelta(
			pBone->ScaleShear[0][0] * deltaPosition.x +
				pBone->ScaleShear[0][1] * deltaPosition.y +
				pBone->ScaleShear[0][2] * deltaPosition.z,
			pBone->ScaleShear[1][0] * deltaPosition.x +
				pBone->ScaleShear[1][1] * deltaPosition.y +
				pBone->ScaleShear[1][2] * deltaPosition.z,
			pBone->ScaleShear[2][0] * deltaPosition.x +
				pBone->ScaleShear[2][1] * deltaPosition.y +
				pBone->ScaleShear[2][2] * deltaPosition.z );
		const CVec3 rotatedDelta = current.Rotate( scaledDelta );
		pBone->Position[0] += rotatedDelta.x;
		pBone->Position[1] += rotatedDelta.y;
		pBone->Position[2] += rotatedDelta.z;

		// CQuat::operator*= pre-multiplies, so use the binary operator here for
		// Granny's current * delta order.
		CQuat composed = current * deltaRotation;
		composed.Normalize();
		memcpy( pBone->Orientation, &composed, sizeof(pBone->Orientation) );
	}
}

void CGltfSkeletonAnimator::BuildMatrices()
{
	const SHMatrix global = GlobalMatrix( value );
	value.worldPose.resize( value.localPose.size() );
	value.compositePose.resize( value.localPose.size() );
	std::vector<unsigned char> state( value.localPose.size(), 0 );
	for ( std::size_t i = 0; i < value.localPose.size(); ++i )
		BuildBoneWorld( i, skeleton, value, global, &state, &value.worldPose );
	for ( std::size_t i = 0; i < value.localPose.size(); ++i )
		value.compositePose[i] = i < skeleton.inverseBindMatrices.size()
			? value.worldPose[i] * skeleton.inverseBindMatrices[i] : value.worldPose[i];
}

bool CGltfSkeletonAnimator::NeedUpdate()
{
	CheckJustLoaded();
	const bool timeChanged = pTime.Refresh();
	const bool transformChanged = pGlobalTransform && pGlobalTransform.Refresh();
	if ( bSmthChanged )
	{
		bSmthChanged = false;
		return true;
	}
	if ( transformChanged )
		return true;
	if ( IsValid(pSpecMutator) && pSpecMutator->NeedUpdate() )
		return true;
	return timeChanged;
}

void CGltfSkeletonAnimator::Recalc()
{
	CheckJustLoaded();
	if ( skeleton.restPose.empty() )
		return;
	const STime time = pTime->GetValue();
	if ( pGlobalTransform )
	{
		pGlobalTransform.Refresh();
		SetGlobalPositionInternal( pGlobalTransform->GetValue().forward );
	}
	value.localPose = skeleton.restPose;
	for ( const SAnimationHolder &holder : animations )
	{
		bool active = false;
		const float local = GetLocalTime( holder, time, &active );
		const float weight = GetEffectiveWeight( holder, time );
		if ( active && weight > FP_EPSILON )
			ApplyAnimation( holder, local, weight );
	}
	ApplyBoneMutators( time );
	if ( IsValid(pSpecMutator) )
		pSpecMutator->MutateSkeletonPose( &value );
	else
		pSpecMutator = 0;

	if ( nAnimWithMovement >= 0 && nAnimWithMovement < static_cast<int>(animations.size()) )
	{
		bool active = false;
		const SAnimationHolder &holder = animations[nAnimWithMovement];
		const float local = GetLocalTime( holder, time, &active );
		if ( active && fabsf(holder.fSpeed) > FP_EPSILON )
		{
			const float y = fGlobalMovementSpeed * local *
				GetEffectiveWeight(holder, time) / holder.fSpeed;
			value.poseGlobal[12] += value.poseGlobal[4] * y;
			value.poseGlobal[13] += value.poseGlobal[5] * y;
			value.poseGlobal[14] += value.poseGlobal[6] * y;
		}
	}
	BuildMatrices();
}

CGltfSkeletonAnimator::SAnimID CGltfSkeletonAnimator::AddAnimation( STime tStartTime,
	const SAnimHandle &h, bool bLoop, float fSpeed, float fWeight, STime tEndTime )
{
	Touch();
	SAnimationHolder holder;
	holder.tStartTime = tStartTime;
	holder.tEndTime = tEndTime;
	holder.fSpeed = fSpeed;
	holder.fWeight = fWeight;
	holder.nLoopCount = bLoop ? 0 : 1;
	holder.hAnimation = h;
	if ( !BindAnimation(&holder) )
	{
		if ( h.pAnimFile )
			DebugTrace( "glTF: failed to bind animation resource %s", h.pAnimFile->GetDBID().ToString().c_str() );
		return -1;
	}
	animations.push_back( holder );
	return static_cast<int>(animations.size()) - 1;
}

void CGltfSkeletonAnimator::ClearAllAnimations()
{
	Touch();
	animations.clear();
	nAnimWithMovement = -1;
	if ( !bBoneMutatorsEnabled || !IsValid(pTime) )
		return;

	// Granny freezes active mutators when controls are cleared. Turret aiming
	// therefore survives routine idle/movement animation changes.
	const STime freezeTime = pTime->GetValue();
	for ( SSimpleBoneMutator &mutator : boneMutators )
	{
		if ( !mutator.IsEnabled() )
			continue;
		CQuat rotation;
		CVec3 position;
		mutator.GetAtTime( freezeTime, &rotation, &position );
		mutator.Clear();
		mutator.AddBoneTimePose( freezeTime, rotation, position );
	}
}

void CGltfSkeletonAnimator::FadeIn( const STime &duration, SAnimID id )
{
	if ( id < 0 || id >= static_cast<int>(animations.size()) )
		return;
	Touch();
	animations[id].bFadeIn = true;
	animations[id].bFadeOut = false;
	animations[id].tFadeStart = pTime->GetValue();
	animations[id].tFadeDuration = duration;
}

void CGltfSkeletonAnimator::FadeOut( const STime &duration, SAnimID id )
{
	if ( id < 0 || id >= static_cast<int>(animations.size()) )
		return;
	Touch();
	animations[id].bFadeOut = true;
	animations[id].bFadeIn = false;
	animations[id].tFadeStart = pTime->GetValue();
	animations[id].tFadeDuration = duration;
}

void CGltfSkeletonAnimator::FadeOutAllAnimations( const STime &duration )
{
	for ( int i = 0; i < static_cast<int>(animations.size()); ++i )
		FadeOut( duration, i );
}

void CGltfSkeletonAnimator::SetSpeedFactorForAllAnimations( const STime &currentTime, float speed )
{
	for ( int i = 0; i < static_cast<int>(animations.size()); ++i )
	{
		SAnimationHolder &holder = animations[i];
		const float localMilliseconds = (currentTime - holder.tStartTime) * holder.fSpeed;
		holder.fSpeed = speed;
		if ( fabsf(speed) > FP_EPSILON )
			holder.tStartTime = currentTime - static_cast<STime>(localMilliseconds / speed);
	}
	Touch();
}

float CGltfSkeletonAnimator::GetDuration( const SAnimID id )
{
	return id >= 0 && id < static_cast<int>(animations.size()) && fabsf(animations[id].fSpeed) > FP_EPSILON
		? animations[id].fDuration / fabsf(animations[id].fSpeed) : 0.0f;
}

unsigned int CGltfSkeletonAnimator::GetMarkTimes( std::vector<float> *pResult,
	const SAnimID, const std::string & )
{
	pResult->clear();
	return 0;
}

unsigned int CGltfSkeletonAnimator::EnumMarks( std::vector<std::string> *pResult, const SAnimID )
{
	pResult->clear();
	return 0;
}

void CGltfSkeletonAnimator::SetSpeedFactor( const SAnimID id, float speed )
{
	if ( id >= 0 && id < static_cast<int>(animations.size()) )
	{
		Touch();
		SAnimationHolder &holder = animations[id];
		const STime currentTime = pTime->GetValue();
		const float localMilliseconds = (currentTime - holder.tStartTime) * holder.fSpeed;
		holder.fSpeed = speed;
		if ( fabsf(speed) > FP_EPSILON )
			holder.tStartTime = currentTime - static_cast<STime>(localMilliseconds / speed);
	}
}

void CGltfSkeletonAnimator::SetLocalTime( const SAnimID id, const STime time )
{
	if ( id >= 0 && id < static_cast<int>(animations.size()) && fabsf(animations[id].fSpeed) > FP_EPSILON )
	{
		Touch();
		// Match Granny: tTime is an unscaled elapsed clock in milliseconds.
		animations[id].tStartTime = pTime->GetValue() - time;
	}
}

void CGltfSkeletonAnimator::SetEndTime( const SAnimID id, const STime endTime )
{
	if ( id >= 0 && id < static_cast<int>(animations.size()) )
	{
		Touch();
		animations[id].tEndTime = endTime;
	}
}

void CGltfSkeletonAnimator::SetLoopCount( const SAnimID id, const int count )
{
	if ( id >= 0 && id < static_cast<int>(animations.size()) )
	{
		Touch();
		animations[id].nLoopCount = count;
	}
}

void CGltfSkeletonAnimator::SetGlobalAnimTransit( const STime duration )
{
	Touch();
	tTransitDuration = duration;
}

void CGltfSkeletonAnimator::SetGlobalPosition( const SHMatrix &position )
{
	Touch();
	SetGlobalPositionInternal( position );
}

void CGltfSkeletonAnimator::SetGlobalTransform( CFuncBase<SFBTransform> *pTransform )
{
	Touch();
	pGlobalTransform = pTransform;
}

void CGltfSkeletonAnimator::SetGlobMoveAnimation( const SAnimID id, const float movementSpeed )
{
	if ( id >= 0 && id < static_cast<int>(animations.size()) )
	{
		Touch();
		nAnimWithMovement = id;
		fGlobalMovementSpeed = movementSpeed * 1000.0f;
	}
}

int CGltfSkeletonAnimator::GetBoneIndex( const char *pszName )
{
	CheckJustLoaded();
	return pszName ? skeleton.FindBone( pszName ) : -1;
}

void CGltfSkeletonAnimator::GetBoneNames( std::vector<std::string> *pNames )
{
	CheckJustLoaded();
	pNames->insert( pNames->end(), skeleton.boneNames.begin(), skeleton.boneNames.end() );
}

bool CGltfSkeletonAnimator::GetBonePosition( int index, SHMatrix *pResult )
{
	CheckJustLoaded();
	BuildMatrices();
	if ( index < 0 || index >= static_cast<int>(value.worldPose.size()) )
		return false;
	*pResult = value.worldPose[index];
	return true;
}

bool CGltfSkeletonAnimator::GetBonePosition( int index, CVec3 *pResult )
{
	SHMatrix matrix;
	if ( !GetBonePosition(index, &matrix) )
		return false;
	*pResult = matrix.GetTranslation();
	return true;
}

bool CGltfSkeletonAnimator::GetBonePosition( const char *name, SHMatrix *pResult )
{
	return GetBonePosition( GetBoneIndex(name), pResult );
}

bool CGltfSkeletonAnimator::GetBonePosition( const char *name, CVec3 *pResult )
{
	return GetBonePosition( GetBoneIndex(name), pResult );
}

bool CGltfSkeletonAnimator::GetLocalBonePosition( const char *name, SHMatrix *pResult )
{
	CheckJustLoaded();
	BuildMatrices();
	const int index = GetBoneIndex( name );
	if ( index < 0 || index >= static_cast<int>(value.localPose.size()) )
		return false;
	*pResult = NGltf::MakeLocalMatrix( value.localPose[index] );
	return true;
}

void CGltfSkeletonAnimator::SetBoneMutator( const char *name, const STime &start,
	const std::vector<SDesiredBoneMove> &mutation )
{
	SetBoneMutator( GetBoneIndex(name), start, mutation );
}

void CGltfSkeletonAnimator::SetBoneMutator( const int index, const STime &start,
	const std::vector<SDesiredBoneMove> &mutation )
{
	if ( index < 0 || index >= static_cast<int>(boneMutators.size()) )
		return;
	Touch();
	bBoneMutatorsEnabled = true;
	CQuat currentRotation = QNULL;
	CVec3 currentPosition = VNULL3;
	if ( boneMutators[index].IsEnabled() )
		boneMutators[index].GetAtTime( start, &currentRotation, &currentPosition );
	boneMutators[index].Clear();
	boneMutators[index].AddBoneTimePose( start, currentRotation, currentPosition );
	STime end = start;
	for ( const SDesiredBoneMove &move : mutation )
	{
		end += move.tDuration;
		boneMutators[index].AddBoneTimePose( end, move.finalRot, move.finalPos );
	}
}

void CGltfSkeletonAnimator::SetSpecialMutator( IAnimMutator *pMutator )
{
	Touch();
	pSpecMutator = pMutator;
}

CFuncBase<SFBTransform> *CGltfSkeletonAnimator::CreateTransform( const std::string &name )
{
	return CreateTransform( GetBoneIndex(name.c_str()) );
}

CFuncBase<SFBTransform> *CGltfSkeletonAnimator::CreateTransform( int index )
{
	if ( index < 0 || index >= static_cast<int>(skeleton.boneNames.size()) )
		return 0;
	return new CAddBoneFilter( this, skeletonH, index );
}

int CGltfSkeletonAnimator::operator&( CStructureSaver &f )
{
	f.Add( 2, &pTime );
	f.Add( 3, &skeletonH );
	f.Add( 4, &pSpecMutator );
	f.Add( 5, &animations );
	for ( int i = 0; i < 16; ++i )
		f.Add( 6, &value.poseGlobal[i], i + 1 );
	f.Add( 7, &pGlobalTransform );
	f.Add( 8, &nAnimWithMovement );
	f.Add( 9, &fGlobalMovementSpeed );
	f.Add( 10, &tTransitDuration );
	f.Add( 11, &boneMutators );
	f.Add( 12, &bBoneMutatorsEnabled );
	if ( f.IsReading() )
		bJustLoaded = true;
	return 0;
}

void CGltfSkeletonAnimator::CheckJustLoaded()
{
	if ( !bJustLoaded )
		return;
	const std::vector<SSimpleBoneMutator> savedMutators = boneMutators;
	Create( skeletonH, pTime );
	if ( savedMutators.size() == boneMutators.size() )
		boneMutators = savedMutators;
	bJustLoaded = false;
}

}

using namespace NAnimation;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x71321150, CGltfSkeletonAnimator )
