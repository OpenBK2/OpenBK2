#include "stdafx.h"
#include "./predictedantiaviationfire.h"
#include "SerializeOwner.h"

#include "Aviation.h"
#include "Guns.h"
#include "Diplomacy.h"
#include "AAFeedBacks.h"
#include "Manuver.h"


extern CDiplomacy theDipl;
extern NTimer::STime curTime;
extern CAAFeedBacks theAAFeedBacks;

CPredictedAntiAviationFire::SPredict::SPredict( const CVec3 &pt, const float _fRange, const NTimer::STime _timeToFire, CAIUnit *pOwner )
: vPt( pt ), fRange( _fRange ), timeToFire ( _timeToFire )
{
	const CVec2 vCenter( pOwner->GetCenterPlain() );
	const CVec2 vDir( pt.x - vCenter.x, pt.y - vCenter.y );

	wHor = GetDirectionByVector( vDir );
	wVer = GetDirectionByVector( fabs(vDir), pt.z );
}

void CPredictedAntiAviationFire::OnSerialize( IBinSaver &saver )
{
	SerializeOwner( 2, &pUnit, &saver );
}

void CPredictedAntiAviationFire::StopFire()
{
	for ( Guns::iterator it = nGuns.begin(); it != nGuns.end(); ++it )
		pUnit->GetGun( *it )->StopFire();
}

bool CPredictedAntiAviationFire::IsFinishedFire()
{
	bool bFiring = false;
	for ( Guns::iterator it = nGuns.begin(); it != nGuns.end(); ++it )
		bFiring |= pUnit->GetGun( *it )->IsFiring();
	return !bFiring;
}

bool CPredictedAntiAviationFire::CanFireNow() const
{
	return pUnit->GetGun( *nGuns.begin() )->CanShootToUnitWOMove( pPlane );
}

void CPredictedAntiAviationFire::FireNow()
{
	for ( Guns::iterator it = nGuns.begin(); it != nGuns.end(); ++it )
	{
		CBasicGun *pGun = pUnit->GetGun( *it );
		pGun->CanShoot();
	}
}

void CPredictedAntiAviationFire::SetTarget( CAviation *_pPlane )
{
	pPlane = _pPlane;

	eState = SAAS_ESITMATING;
}

void CPredictedAntiAviationFire::Segment()
{
	if (	eState != SAAS_WAIT_FOR_END_OF_BURST && 
				eState != SAAS_FINISH && 
				( !IsValidObj( pPlane ) || EDI_ENEMY != theDipl.GetDiplStatus( pPlane->GetPlayer(), pUnit->GetPlayer() ) ) )
	{
		eState = SAAS_FINISH;
		StopFire();
	}
	
	if ( nGuns.empty() )
		return;

	switch ( eState )
	{
	case SAAS_ESITMATING:
		{
			if ( CalcAimPoint() )
			{
				eState = SAAS_START_AIMING_TO_PREDICTED_POINT;
				timeLastAimUpdate = curTime;
			}
			else
				eState = SAAS_FINISHED_TASK;
		}

		break;
	case SAAS_FINISHED_TASK:
		break;
	case SAAS_FINISH:
		if ( IsFinishedFire() )
			eState = SAAS_FINISHED_TASK;
		else
		{
			StopFire();
			eState = SAAS_WAIT_FOR_END_OF_BURST;
		}

		break;
	case SAAS_WAIT_FOR_END_OF_BURST:
		if ( IsFinishedFire() )
		{
			StopFire();
			eState = SAAS_FINISHED_TASK;
		}

		break;
	case SAAS_START_AIMING_TO_PREDICTED_POINT:
		{
			if ( curTime - timeLastAimUpdate > SConsts::AA_BEH_UPDATE_DURATION )
				CalcAimPoint();

			if ( fabs( CVec2( aimPoint.GetPt().x, aimPoint.GetPt().y) - pUnit->GetCenterPlain() ) <= aimPoint.GetRange() )
			{
				// pUnit->SendAcknowledgement( ACK_ATTACKING_AVIATION, true );
				bAttacking = true;

				eState = SAAS_AIM_TO_PREDICTED_POINT;
				timeOfStartBurst = curTime;
			}
			else
				eState = SAAS_FINISH;
		}

		break;
	case SAAS_AIM_TO_PREDICTED_POINT:

		if ( aimPoint.GetPt().z <= 0 )			// don't want to shoot to the ground.
			eState = SAAS_ESITMATING;
		else
		{
			Aim();
			if ( curTime >= aimPoint.GetFireTime() )
			{
				FireNow();
				theAAFeedBacks.Fired( pUnit, pPlane );
				eState = SAAS_FIRING_TO_PREDICTED_POINT;
			}
		}
		break;
	case SAAS_FIRING_TO_PREDICTED_POINT:
		if ( IsFinishedFire() )
			eState = SAAS_ESITMATING;

		break;
	}
}

void CPredictedAntiAviationFire::Aim()
{
	CBasicGun *pGun = pUnit->GetGun( *nGuns.begin() );
	if ( CTurret *pTurret = pGun->GetTurret() )
	{
		pGun->DontShoot();
		pGun->StartPointBurst( aimPoint.GetPt(), false );
	}	
}

void CPredictedAntiAviationFire::Stop()
{
	if ( nGuns.empty() )
		return;
	CBasicGun *pGun = pUnit->GetGun( *nGuns.begin() );
	if ( CTurret *pTurret = pGun->GetTurret() )
	{
		pGun->CanShoot();
	}
}

bool CPredictedAntiAviationFire::CalcAimPoint()
{
	if ( !IsValidObj( pPlane ) ) 
		return false;
	timeLastAimUpdate = curTime;

	float fRange = 0;
	CTurret * pTurret = 0;
	CBasicGun * pGunAbleToShoot = 0;
	// choose the first gun that able to shoot to plane
	CBasicGun *pGun = pUnit->GetGun( *nGuns.begin() );

	if ( pGun->CanShootByHeight( pPlane ) && pGun->IsOnTurret() )
	{
		pTurret = pGun->GetTurret();
		fRange = pGun->GetFireRangeMax();
		pGunAbleToShoot = pGun;
	}

	if ( !pGunAbleToShoot ) 
	{
		StopFire();
		return false;
	}

	const CVec3 vPlaneCenter( pPlane->GetPosB2() );

	CVec3 vPredict = vPlaneCenter;
	NTimer::STime nTime = 0;
	for ( int i = 0; i < 3; ++i )
	{
		nTime = pGunAbleToShoot->GetTimeToShootToPoint( vPredict );
		if ( pPlane->IsHelicopter() )
		{
			// Helicopters do not own plane maneuvers; AA uses deterministic linear prediction for them.
			vPredict = pPlane->GetPosB2() + pPlane->GetSpeedB2() * float( nTime );
		}
		else
			vPredict = pPlane->GetManuver()->GetProspectivePoint( nTime );
	}

	const int nTimeToShoot = nTime / SConsts::AI_SEGMENT_DURATION * SConsts::AI_SEGMENT_DURATION;

	DRAW_WHITE_CROSS(vPredict);

	aimPoint = SPredict( vPredict, fRange, curTime + nTimeToShoot, pUnit );
	return true;
}

