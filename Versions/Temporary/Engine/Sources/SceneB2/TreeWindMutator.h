#pragma once

#include "AnimMutators.h"
#include "vendor/granny/include/granny.h"

class CTreeWindMutator : public ITreeWindMutator
{
	OBJECT_BASIC_METHODS( CTreeWindMutator );

	// Bone indices
	std::vector<int> leafBones;
	CVec2 vPos;
	NTimer::STime lastUpdateTime;
	float fMagnitude; //Cached

	void TransformRootBone( NAnimation::SBoneTransform *pTransform, const CQuat &qRot );
	void TransformLeafBone( NAnimation::SBoneTransform *pTransform, const CQuat &qRot );

public:
	CTreeWindMutator() {}

	void Setup( NAnimation::ISkeletonAnimator *pAnimator,
	            const CVec3 &_vPos3,
	            const std::vector<std::string> &leafNames ) override;

	bool NeedUpdate() override;
	void MutateSkeletonPose( NAnimation::SSkeletonPose *pPose ) override;
	int operator&( IBinSaver &saver ) override;
};
