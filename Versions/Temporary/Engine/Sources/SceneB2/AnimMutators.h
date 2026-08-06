#pragma once

#include "3Dmotor/GAnimation.hpp"

using namespace NAnimation;

struct ITreeFallingMutator : public IAnimMutator
{
	enum { typeID = 0x12094B80 };
	virtual void Setup( ISkeletonAnimator *pAnimator, const CVec2 &vDir, float _fEndAngle, const CQuat &qRot,
											const std::vector<std::string> &leafNames, int nEffectID, const CVec3 &vPos,
											float fEffectHeight, float fFallCycles, int nFallDuration, NTimer::STime timeStart ) = 0;
};

struct ITreeWindMutator : public IAnimMutator
{
	enum { typeID = 0x19132B40 };
	virtual void Setup( ISkeletonAnimator *pAnimator, const CVec3 &_vPos3, const std::vector<std::string> &leafNames ) = 0;
};

struct IMechUnitJoggingMutator : public IAnimMutator
{
	enum { typeID = 0x15095B00 };
	//
	struct SJoggingParams
	{
		float fPeriod1, fPeriod2;
		float fAmp1, fAmp2;
		float fPhaze1, fPhaze2;
	};
	//
	virtual void Setup( const int nBasisBoneIndex, const SJoggingParams &_joggingX, const SJoggingParams &_joggingY ) = 0;
	// Propeller entries are paired by index; missing entries and unknown model-part names are ignored.
	virtual void SetupPropellers( ISkeletonAnimator *pAnimator, const std::vector<std::string> &propellerObjects,
									const std::vector<CVec3> &propellerSpeedsRad ) = 0;
	virtual void Play() = 0;
	virtual void Stop() = 0;
};

struct IWingScaleMutator : public IAnimMutator
{
	enum { typeID = 0x3119AB00 };
	virtual bool Setup( ISkeletonAnimator *pAnimator, const std::string &szScaledWingPrefix, const std::string &szStaticWingName ) = 0;
	virtual void SetScale( const float fScale ) = 0;
	virtual void ShowStatic( const bool bShow ) = 0;
};


