#include "stdafx.h"

#include "3Dmotor/GAnimation.hpp"
#include "MapObj.h"
#include "SmokeTrailEffect.h"

#include <algorithm>
#include <cmath>

CSmokeTrailEffect::CSmokeTrailEffect( const SHMatrix &_mLocalPos,
                                      float _fInterval,
                                      const NDb::SComplexEffect *_pEffect,
                                      const CVec3 &_vPos,
                                      const CQuat &_qRot,
                                      NTimer::STime currTime,
                                      bool bVisible )
{
	vLastVisPos = _vPos;
	fTimeLastUpdate = currTime;
	pEffect = _pEffect;
	fInterval = std::isfinite( _fInterval ) && _fInterval > 0.0f ? _fInterval : 2.0f;
	mLocalPos = _mLocalPos;
	//
	CreateEffect( _vPos, _qRot, currTime, bVisible );
}

void CSmokeTrailEffect::CreateEffect( const CVec3 &vPos, const CQuat &qRot, NTimer::STime time, bool bVisible )
{
	if ( !bVisible )
	{
		return;
	}
	SHMatrix mResultPos;
	Multiply( &mResultPos, SHMatrix( vPos, qRot ), mLocalPos );
	PlayComplexEffect( OBJECT_ID_FORGET, pEffect, time, mResultPos );
}

void CSmokeTrailEffect::UpdatePlacement( const CVec3 &vPos, const CQuat &qRot, NTimer::STime currTime, bool bVisible )
{
	// Validate loaded effects too, and compute this per instance rather than from the first trail ever updated.
	if ( !std::isfinite( fInterval ) || fInterval <= 0.0f )
	{
		fInterval = 2.0f;
	}
	if ( !std::isfinite( vPos.x ) || !std::isfinite( vPos.y ) || !std::isfinite( vPos.z ) )
	{
		return;
	}
	const float fCurrTime = currTime;

	CVec3 vDir( vPos - vLastVisPos );
	const float fDist2 = fabs2( vDir );
	const float fDistance = std::sqrt( fDist2 );
	if ( !std::isfinite( fDistance ) || !std::isfinite( fTimeLastUpdate ) || fTimeLastUpdate > fCurrTime ||
	     fTimeLastUpdate < 0.0f || double( fDistance ) / double( fInterval ) > 4096.0 )
	{
		// Teleports and corrupt/tiny spacing must not emit an unbounded backlog in one render update.
		vLastVisPos = vPos;
		fTimeLastUpdate = fCurrTime;
		CreateEffect( vPos, qRot, currTime, bVisible );
		return;
	}
	if ( fDistance == 0.0f || fDistance < fInterval )
	{
		return;
	}
	//
	Normalize( &vDir );
	vDir *= fInterval;

	float fDist = fDistance;
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
	int nSamples = 0;
	do
	{
		vLastVisPos += vDirInterval;
		fTime += fTimeInterval;
		const NTimer::STime time = NTimer::STime( (std::min)( double( fTime ), double( currTime ) ) );
		CreateEffect( vLastVisPos + vDir, qRot, time, bVisible );
		const float fRemaining = fDist - fInterval;
		if ( !( fRemaining < fDist ) )
		{
			break;
		}
		fDist = fRemaining;
	} while ( ++nSamples < 4096 && fDist >= fInterval );
	//
	fTimeLastUpdate = fTime;
}

void CalcRelativePos( SHMatrix *pmRelativePos,
                      const SHMatrix &mPos,
                      const std::string &szBoneName,
                      NAnimation::ISkeletonAnimator *pAnimator )
{
	SHMatrix mBoneLocalPos;
	if ( szBoneName.empty() || pAnimator == 0 ||
	     pAnimator->GetLocalBonePosition( szBoneName.c_str(), &mBoneLocalPos ) == false )
	{
		Identity( pmRelativePos );
		return;
	}

	SHMatrix mCalculatedEffectPos;
	Multiply( &mCalculatedEffectPos, mPos, mBoneLocalPos );

	SHMatrix mRealEffectPos;
	if ( !pAnimator->GetBonePosition( szBoneName.c_str(), &mRealEffectPos ) )
	{
		return;
	}

	SHMatrix mInvCalculated;
	Invert( &mInvCalculated, mCalculatedEffectPos );

	SHMatrix mMulti;
	Multiply( &mMulti, mInvCalculated, mRealEffectPos );

	Multiply( pmRelativePos, mBoneLocalPos, mMulti );
}

REGISTER_SAVELOAD_CLASS( B2_M1_WORLD, 0x101BCCC0, CSmokeTrailEffect )
