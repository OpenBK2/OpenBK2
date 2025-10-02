#pragma once

#include "AnimMutators.h"
#include "../vendor/Granny/include/granny.h"

using namespace NAnimation;

class CTreeWindMutator: public ITreeWindMutator
{
	OBJECT_BASIC_METHODS( CTreeWindMutator );

	// Bone indices
	vector<int> leafBones;
	CVec2 vPos;
	NTimer::STime lastUpdateTime;
	float fMagnitude;							//Cached

	void TransformRootBone( granny_transform *pTransform, const CQuat &qRot );
	void TransformLeafBone( granny_transform *pTransform, const CQuat &qRot );

public:
	CTreeWindMutator() {}

	void Setup( ISkeletonAnimator *pAnimator, const CVec3 &_vPos3, const vector<string> &leafNames );

	bool NeedUpdate();
	void MutateSkeletonPose( granny_local_pose *pPose );
	int operator&( IBinSaver &saver );
};


