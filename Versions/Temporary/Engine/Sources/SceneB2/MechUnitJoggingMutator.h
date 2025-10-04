#pragma once

#include "AnimMutators.h"
#include "System/FastMath.h"
#include "vendor/granny/include/granny.h"

using namespace NAnimation;

class CMechUnitJoggingMutator : public IMechUnitJoggingMutator
{
	OBJECT_BASIC_METHODS( CMechUnitJoggingMutator )

	struct SJoggingParams2
	{
		float fPeriod1, fPeriod2;
		float fAmp1, fAmp2;
		float fPhaze1, fPhaze2;
		//
#if !defined(_FINALRELEASE) && !defined(_BETARELEASE)
		// for uninitialized hunt
		SJoggingParams2() : fPeriod1( 0 ), fPeriod2 ( 0 ), fAmp1( 0 ), fAmp2( 0 ), fPhaze1 ( 0 ), fPhaze2( 0 ) {}
#endif
		float GetValue( const NTimer::STime nDeltaTime ) const;
	};

	SJoggingParams2 joggingX;
	SJoggingParams2 joggingY;

	NTimer::STime nStartTime;
	NTimer::STime nStopTime;
	bool bStopped;

	int	nBasisBoneIndex;

	bool NeedUpdate();

public:
	CMechUnitJoggingMutator() :	bStopped( true ), nBasisBoneIndex( -1 ), nStartTime( 0 ), nStopTime( 0 ) {}

	void Setup( const int nBasisBoneIndex, const SJoggingParams &_joggingX, const SJoggingParams &_joggingY );
	void Play();
	void Stop();

	void MutateSkeletonPose( granny_local_pose *pPose );

	int operator&( IBinSaver &saver );
};


