#include "StdAfx.h"
#include "GParticles.h"
#include "GParticleFormat.h"
#include "GParticleInfo.h"
#include "DBScene.h"

namespace NGScene
{

// CParticleAnimator

float CParticleAnimator::GetRealTime( const STime &time )
{
	return (time - stBeginTime) * pInstance->fSpeed / 1000.f - pInstance->fOffset * pInstance->fSpeed;
}

void CParticleAnimator::CheckFirstTime()
{
	if ( IsValid( pValue ) )
		return;
	CStandardParticleEffect *pRealValue = new CStandardParticleEffect( pInstance->nPriority );
	pValue = pRealValue;
	CStandardParticleEffect &value = *pRealValue;
	value.nGrassSize = 0;
	value.textures = textureIDs;
	value.pInfo = pInfo;
	value.fScale = pInstance->fScale;
	value.fEndCycle = pInstance->fEndCycle;
	value.pivot = pInstance->vPivot;
	value.pFilter = pFilter;
	value.fTStopGeneration = -1;
	value.nStopCycle = -1;
}

void CParticleAnimator::Recalc()
{
	CheckFirstTime();
	CDynamicCast<CStandardParticleEffect> pRealValue(pValue);
	CStandardParticleEffect &value = *pRealValue;

	value.bEnd = true;
	value.bLeaveParticlesWhereStarted = bLeaveParticlesWhereStarted;
	value.bSampleCenterWarfog = !bPerParticleFog;

//	value.bAlphaAdd = (pInstance->alpha == NDb::SParticleInstance::A_ADDITIVE);
	value.transform = pPlacement->GetValue().forward;
	value.frames.clear();
	value.vWrap = CVec2(0,0);
	if ( pInstance && pInstance->pParticle )
		value.vWrap = pInstance->pParticle->vWrapSize;
	
	STime time = pTime->GetValue();
	if ( time < stBeginTime )
	{
		value.bEnd = false;
		return;
	}
	float fTObject = GetRealTime( time );
	CParticlesInfo *pEffect = pInfo->GetValue();
	if ( !pEffect )
	{
		value.bEnd = false;
		return;
	}
	if ( fTObject < 0 )
	{
		value.bEnd = false;
		return;
	}
	vector<float> fTimes;
	if ( pInstance->fEndCycle == 0 )
	{
		if ( pInstance->nCycleCount )
		{
			if ( fTObject < pEffect->fTEnd )
			{
				fTimes.push_back( fTObject );
				value.bEnd = false;
			}
		}
		else
		{
			fTimes.push_back(0);
			value.bEnd = false;
		}
	}
	else
	{
		int nCurCycle = int( fTObject / pInstance->fEndCycle ) + 1;
		int nCycleCount = pInstance->nCycleCount;
		if ( value.nStopCycle >= 0 )
			nCycleCount = value.nStopCycle;
		if ( nCycleCount && nCurCycle > nCycleCount )
			nCurCycle = nCycleCount;
		float fT = fTObject - (nCurCycle - 1) * pInstance->fEndCycle;
		for ( int i = 0; i < nCurCycle && fT < pEffect->fTEnd; ++i )
		{
			fTimes.push_back( fT );
			fT += pInstance->fEndCycle;
		}
		if ( nCycleCount == 0	|| fTObject < pEffect->fTEnd + (nCycleCount - 1) * pInstance->fEndCycle )
			value.bEnd = false;
	}

	for ( int i = 0; i < fTimes.size(); ++i )
	{
		SParticleFrame frame;
		frame.fT = fTimes[i] * pEffect->fFrameRate;
		frame.bLastCycle = (i == 0);
		frame.nNumFrame = (fTObject - fTimes[i]) / pInstance->fEndCycle + 1;;
		value.frames.push_back( frame );
	}
}

void CParticleAnimator::StopParticlesGeneration( const STime &tStop )
{
	CheckFirstTime();
	ASSERT( IsValid( pValue ) );
	if ( !IsValid( pValue ) )
		return;
	pInfo.Refresh();
	CParticlesInfo *pEffect = pInfo->GetValue();
	if ( !pEffect )
		return;
	CDynamicCast<CStandardParticleEffect> pRealValue(pValue);
	CStandardParticleEffect &value = *pRealValue;
	float fT = GetRealTime( tStop );
	if ( pInstance->fEndCycle == 0 )
		value.fTStopGeneration = fT * pEffect->fFrameRate;
	else
	{
		int nCycle = int( fT / pInstance->fEndCycle ) + 1;
		if ( pInstance->nCycleCount && nCycle > pInstance->nCycleCount )
				nCycle = pInstance->nCycleCount;
		float fTTruncated = fT - (nCycle - 1) * pInstance->fEndCycle;
		value.fTStopGeneration = fTTruncated * pEffect->fFrameRate;
		value.nStopCycle = nCycle;
	}
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x27041142, CParticleAnimator )

