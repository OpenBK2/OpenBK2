#pragma once

#include "AnimMutators.h"
#include "System/FastMath.h"
#include "vendor/granny/include/granny.h"

class CMechUnitJoggingMutator : public IMechUnitJoggingMutator
{
	OBJECT_BASIC_METHODS( CMechUnitJoggingMutator )

	struct SJoggingParams2
	{
		float fPeriod1, fPeriod2;
		float fAmp1, fAmp2;
		float fPhaze1, fPhaze2;
		//
#if !defined( _FINALRELEASE ) && !defined( _BETARELEASE )
		// for uninitialized hunt
		SJoggingParams2()
			: fPeriod1( 0 )
			, fPeriod2( 0 )
			, fAmp1( 0 )
			, fAmp2( 0 )
			, fPhaze1( 0 )
			, fPhaze2( 0 )
		{
		}
#endif
		float GetValue( const NTimer::STime nDeltaTime ) const;
	};

	SJoggingParams2 joggingX;
	SJoggingParams2 joggingY;

	NTimer::STime nStartTime;
	NTimer::STime nStopTime;
	bool bStopped;

	int nBasisBoneIndex;
	std::vector<int> propellerBoneIndices;
	std::vector<CVec3> propellerSpeedsRad;
	NTimer::STime nPropellerStartTime;

	bool NeedUpdate() override;

public:
	CMechUnitJoggingMutator()
		: nStartTime( 0 )
		, nStopTime( 0 )
		, bStopped( true )
		, nBasisBoneIndex( -1 )
		, nPropellerStartTime( 0 )
	{
	}

	void Setup( const int nBasisBoneIndex, const SJoggingParams &_joggingX, const SJoggingParams &_joggingY ) override;
	void SetupPropellers( NAnimation::ISkeletonAnimator *pAnimator,
	                      const std::vector<std::string> &propellerObjects,
	                      const std::vector<CVec3> &propellerSpeedsRad ) override;
	void Play() override;
	void Stop() override;

	void MutateSkeletonPose( NAnimation::SSkeletonPose *pPose ) override;

	int operator&( IBinSaver &saver ) override;
};
