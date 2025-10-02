#include "StdAfx.h"
#include "SmokeTrailEffect.h"
#include "3Dmotor/GAnimation.hpp"
#include "MapObj.h"

CSmokeTrailEffect::CSmokeTrailEffect( const SHMatrix &_mLocalPos, float _fInterval, const NDb::SComplexEffect *_pEffect,
									                    const CVec3 &_vPos, const CQuat &_qRot, NTimer::STime currTime, bool bVisible )
{
	vLastVisPos = _vPos;
	fTimeLastUpdate = currTime;
	pEffect = _pEffect;
	fInterval = _fInterval == 0 ? 2 : _fInterval;
	mLocalPos = _mLocalPos;
	//
	CreateEffect( _vPos, _qRot, currTime, bVisible );
}

void CSmokeTrailEffect::CreateEffect( const CVec3 &vPos, const CQuat &qRot, NTimer::STime time, bool bVisible )
{
	if ( !bVisible )
		return;
	SHMatrix mResultPos;
	Multiply( &mResultPos, SHMatrix(vPos, qRot), mLocalPos );
	PlayComplexEffect( OBJECT_ID_FORGET, pEffect, time, mResultPos );
}

void CSmokeTrailEffect::UpdatePlacement( const CVec3 &vPos, const CQuat &qRot, NTimer::STime currTime, bool bVisible )
{
	static const float fInterval2 = fabs2( fInterval );
	const float fCurrTime = currTime;

	CVec3 vDir( vPos - vLastVisPos );
	const float fDist2 = fabs2( vDir );
	if ( fDist2 < fInterval2 )
		return;
	//
	Normalize( &vDir );
	vDir *= fInterval;

	float fDist = sqrt( fDist2 );
	//
	float fTimeInterval = 0;
	CVec3 vDirInterval = VNULL3;
	if ( fCurrTime >= fTimeLastUpdate )
	{
		const float fNumIntervals = fDist / fInterval;
		fTimeInterval = ( fCurrTime - fTimeLastUpdate ) / fNumIntervals;
		vDirInterval = vDir / fNumIntervals;
	}
	float fTime = fTimeLastUpdate;
	//
	do
	{
		vLastVisPos += vDirInterval;
		fTime += fTimeInterval;
		CreateEffect( vLastVisPos + vDir, qRot, NTimer::STime(fTime), bVisible );
		fDist -= fInterval;
	} 
	while ( fDist >= fInterval );
	//
	fTimeLastUpdate = fTime;
}

void CalcRelativePos( SHMatrix *pmRelativePos, const SHMatrix &mPos, const string &szBoneName, NAnimation::ISkeletonAnimator *pAnimator )
{
	SHMatrix mBoneLocalPos;
	if ( szBoneName.empty() || pAnimator == 0 || pAnimator->GetLocalBonePosition( szBoneName.c_str(), &mBoneLocalPos ) == false )
	{
		Identity( pmRelativePos );
		return;
	}

	SHMatrix mCalculatedEffectPos;
	Multiply( &mCalculatedEffectPos, mPos, mBoneLocalPos );

	SHMatrix mRealEffectPos;
	if ( !pAnimator->GetBonePosition( szBoneName.c_str(), &mRealEffectPos ) )
		return;

	SHMatrix mInvCalculated;
	Invert( &mInvCalculated, mCalculatedEffectPos );

	SHMatrix mMulti;
	Multiply( &mMulti, mInvCalculated, mRealEffectPos );

	Multiply( pmRelativePos, mBoneLocalPos, mMulti );
}

REGISTER_SAVELOAD_CLASS( 0x101BCCC0, CSmokeTrailEffect )

