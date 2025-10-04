#pragma once

#include "AnimMutators.h"
#include "Main/GameTimer.h"
#include "vendor/granny/include/granny.h"

using namespace NAnimation;

class CTreeFallingMutator : public ITreeFallingMutator
{
	OBJECT_BASIC_METHODS( CTreeFallingMutator );
	//
	struct SLeafMutatorData
	{
		int nBoneIndex;
		float fMaxRotAngle;
		float fNormalRotAngle;
	};
	//
	int nRootBoneIdx;
	float fEndAngle;
	CPtr<IGameTimer> pTimer;
	NTimer::STime nStartTime;
	CVec3 vRotAxis;
	CVec3 vEffectRotAxis;
	list<SLeafMutatorData> leafBones;
	bool bFinished;
	int nEffectID;
	CVec3 vTreePos;
	float fEffectHeight;
	float fCycles;
	float fAnimLength;

	float GetCoeffForTime( int nTime );
	void TransformRootBone( granny_transform *pTransform, const CQuat &qRot );
	void TransformLeafBone( granny_transform *pTransform, const CQuat &qRot );
public:
	CTreeFallingMutator() : bFinished( false ) {}

	void Setup( ISkeletonAnimator *pAnimator, const CVec2 &vDir, float _fEndAngle, const CQuat &qRot, const vector<string> &leafNames,
							int _nEffectID, const CVec3 &vPos, float _fEffectHeight, float fFallCycles, int nFallDuration, NTimer::STime timeStart );
	bool NeedUpdate() { return !bFinished; }
	void MutateSkeletonPose( granny_local_pose *pPose );

	int operator&( IBinSaver &saver );
};


