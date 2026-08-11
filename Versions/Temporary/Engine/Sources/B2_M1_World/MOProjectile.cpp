#include "stdafx.h"

#include "Misc/2Darray.h"
#include "3Dmotor/DBScene.h"
#include "MOProjectile.h"
#include "Main/GameTimer.h"
#include "Sound/DBSound.h"
#include "System/Commands.h"

#include <zconf.h>

int g_nProjectileFallSoundMaxTime = 2000;
START_REGISTER(ProjectileConsts)
REGISTER_VAR_EX( "Sound.ProjectileFallSoundMaxTime", NGlobal::VarIntHandler, &g_nProjectileFallSoundMaxTime, 2000, STORAGE_NONE );
FINISH_REGISTER


CMOProjectile::~CMOProjectile()
{
	DetachSound( EAST_MOVEMENT );
}

void CMOProjectile::CreateAttachedEffect()
{
	if ( const NDb::SComplexEffect *pComplexEffect = pProjectile->pAttachedEffect )
	{
		if ( const NDb::SEffect *pEffect = pComplexEffect->GetSceneEffect() )
		{
			Scene()->AttachEffect(
				GetID(), ESSOT_PROJECTILE, pProjectile->szAttachedEffectLocator, 
				pEffect, Singleton<IGameTimer>()->GetGameTime(), ESAT_REPLACE_ON_BONE );
		}
		//
		if ( pComplexEffect->pSoundEffect )
			AttachSound( EAST_MOVEMENT, pComplexEffect->pSoundEffect, pComplexEffect->pSoundEffect->bLooped );
	}
	else if ( pTrajectoryEffect )
	{
		AttachSound( EAST_MOVEMENT, pTrajectoryEffect->pSoundEffect, pTrajectoryEffect->pSoundEffect->bLooped );
	}
}

void CMOProjectile::InitSmokyExhaustInfo( const CVec3 &vVisPos, const CQuat &qRot, NTimer::STime currTime )
{
	if ( pProjectile && pProjectile->pSmokyExhaustEffect )
	{
		NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID() );

		SHMatrix mLocalSmokyPos;
		if ( pAnimator && !pProjectile->szSmokyEffectLocator.empty() )
		{
			CalcRelativePos( &mLocalSmokyPos, SHMatrix(vVisPos, qRot), pProjectile->szSmokyEffectLocator, pAnimator );
			pTrailEffect = new CSmokeTrailEffect( mLocalSmokyPos, pProjectile->fSmokyExhaustEffectInterval, pProjectile->pSmokyExhaustEffect, vVisPos, qRot, currTime, IsVisible() );
		}
		else
		{
			Identity( &mLocalSmokyPos );
			pTrailEffect = new CSmokeTrailEffect( mLocalSmokyPos, pProjectile->fSmokyExhaustEffectInterval, pProjectile->pSmokyExhaustEffect, vVisPos, qRot, currTime, IsVisible() );
		}
	}
	else
		pTrailEffect = 0;
}

bool CMOProjectile::Create( const SAINewProjectileUpdate *pUpdate, const NDb::SProjectile *_pProjectile, const CVec3 &vVisPos, const CQuat &qRot, const NDb::SComplexEffect *_pTrajectoryEffect, const CVec3 &_vRotationSpeedRad )
{
	pProjectile = _pProjectile;
	pTrajectoryEffect = _pTrajectoryEffect;
	vRotationSpeedRad = _vRotationSpeedRad;
	SetID( pUpdate->info.nObjUniqueID );

	bModelExists = pProjectile != 0 && pProjectile->pModel != 0;
	if ( NGlobal::GetVar( "m1", 0 ).GetFloat() == 0.0f && !bModelExists )
		return false;

	CVec3 vPos( vVisPos );
	Vis2AI( &vPos );

	vPosDiff = vPos - pUpdate->info.vAIStartPos;
	startTime = Singleton<IGameTimer>()->GetGameTime();
	timeToEqualizePos = pUpdate->info.timeToEqualizePos;
	qStartRot = qRot;
	SetPlacement( vPos, qRot );

	if ( bModelExists )
	{
		Scene()->AddObject( GetID(), pProjectile->pModel, vPos, qRot, CVec3(1, 1, 1), OBJ_ANIM_MODE_FORCE_ANIMATED, 0 );

		if ( pProjectile->pModel->pSkeleton )
		{
			if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator(GetID()) )
				RunDefaultObjectAnimation( pProjectile->pModel->pSkeleton, pAnimator );
		}

		CreateAttachedEffect();

		InitSmokyExhaustInfo( vVisPos, qRot, startTime );
	}
	
	SetVisible( false, NDb::SEASON_SUMMER, false );
	return true;
}

void CMOProjectile::GetStatus( SObjectStatus *pStatus ) const
{
	CMapObj::GetStatus( pStatus );
}

void CMOProjectile::AIUpdatePlacement( const SAINotifyPlacement &placement, IScene *pScene, ISoundScene *pSoundScene, NDb::ESeason eSeason )
{
	if ( bHitTarget )
		return;

	NTimer::STime curTime = Singleton<IGameTimer>()->GetGameTime();

	if ( curTime > startTime + timeToEqualizePos - g_nProjectileFallSoundMaxTime && !IsSoundAttached( EAST_FALLING ) )
	{
		if ( const NDb::SComplexEffect *pComplexEffect = pProjectile->pEffectBeforeHit )
		{
			if ( const NDb::SEffect *pEffect = pComplexEffect->GetSceneEffect() )
			{
				Scene()->AttachEffect(
					GetID(), ESSOT_PROJECTILE, pProjectile->szAttachedEffectLocator, 
					pEffect, Singleton<IGameTimer>()->GetGameTime(), ESAT_REPLACE_ON_BONE );
			}
			if ( pComplexEffect->pSoundEffect )
					AttachSound( EAST_FALLING, pComplexEffect->pSoundEffect, pComplexEffect->pSoundEffect->bLooped );
		}
	}

	SAINotifyPlacement projPlacement;
	projPlacement.bNewFormat = true;
	if ( placement.bNewFormat )
	{
		projPlacement.vPlacement = placement.vPlacement;
		projPlacement.rotation = placement.rotation;
	}
	else
	{
		projPlacement.vPlacement = CVec3( placement.center, placement.z );
		projPlacement.rotation.FromAngleAxis( ToRadian( float( placement.dir ) / 65536.0f * 360.0f ), 0, 0, 1 );
		// move main object
		MakeOrientation( &projPlacement.rotation, DWORDToVec3(placement.dwNormal) );
	}

	// Apply an absolute local-space Euler spin on top of the flight-path orientation.
	const float fElapsedSeconds = (std::max)( 0.0f, ( curTime - startTime ) / 1000.0f );
	const CQuat qLocalSpin = CQuat( vRotationSpeedRad.x * fElapsedSeconds, V3_AXIS_X ) *
		CQuat( vRotationSpeedRad.y * fElapsedSeconds, V3_AXIS_Y ) *
		CQuat( vRotationSpeedRad.z * fElapsedSeconds, V3_AXIS_Z );
	projPlacement.rotation = projPlacement.rotation * qLocalSpin;

	
	
	if ( bTraceTargetIntersection || bModelExists )
	{	
		if ( curTime < startTime + timeToEqualizePos )
		{
			const float fTimeCoeff = (curTime - startTime) / (float)timeToEqualizePos;
			projPlacement.vPlacement += ( 1.0f - fTimeCoeff ) * vPosDiff;

//			const CQuat rot( projPlacement.rotation );
//			projPlacement.rotation.Slerp( fTimeCoeff, qStartRot, rot );
		}
	}

	if ( pTarget && pTarget->IsRefValid() && bTraceTargetIntersection )
	{
		std::list<IScene::SPickObjInfo> objects;
		std::list<int> attached;
		Scene()->PickAllObjects( GetCenter(), projPlacement.vPlacement, &objects, &attached );

		std::list<IScene::SPickObjInfo>::iterator iter = objects.begin();
		while ( iter != objects.end() && iter->nObjID != pTarget->GetID() )
			++iter;
		if ( iter != objects.end() )
		{
			CVec3 vAIPickPoint;
			Vis2AI( &vAIPickPoint, iter->vPickPoint );
			Explode( SAINotifyHitInfo::EHT_HIT, eSeason, vAIPickPoint, iter->vNormal );

			Scene()->RemoveObject( GetID() );
		}
	}

	if ( !bHitTarget )
	{
		/*
		vector<CVec3> pos( 5 );
		pos[0] = projPlacement.vPlacement + CVec3( 10, 10, 0 );
		pos[1] = projPlacement.vPlacement + CVec3( -10, 10, 0 );
		pos[2] = projPlacement.vPlacement + CVec3( -10, -10, 0 );
		pos[3] = projPlacement.vPlacement + CVec3( 10, -10, 0 );
		pos[4] = projPlacement.vPlacement + CVec3( 10, 10, 0 );
		Scene()->AddPolyline( -1, pos, CVec4( 1, 0, 0, 0 ), false );
		*/
		if ( bModelExists )
		{
			CMapObj::AIUpdatePlacement( projPlacement, pScene, pSoundScene, eSeason );

			if ( pTrailEffect )
			{
				CVec3 vVisPlacement( projPlacement.vPlacement );
				AI2Vis( &vVisPlacement );
				pTrailEffect->UpdatePlacement( vVisPlacement, projPlacement.rotation, curTime, IsVisible() );
			}
		}
	}
}

void CMOProjectile::Explode( SAINotifyHitInfo::EHitType eHitType, NDb::ESeason eSeason, const CVec3 &vCenter, const CVec3 &vDir )
{
	if ( bHitTarget )
		return;
	bHitTarget = true;
	
	const NDb::SComplexEffect *pComplexEffect = 0;
	switch ( eHitType )	
	{
		case SAINotifyHitInfo::EHitType::EHT_HIT:
			pComplexEffect = pWeapon->shells[nShell].pEffectHitDirect;
			break;
		case SAINotifyHitInfo::EHitType::EHT_GROUND:
			{
				const NDb::SWeaponRPGStats::SShell &shell = pWeapon->shells[nShell];
				pComplexEffect = shell.pEffectHitGround;
				if ( shell.pCraters != 0 )
					PlaceCrater( shell.pCraters, eSeason, CVec2( vCenter.x, vCenter.y ) );
			}
			break;
		case SAINotifyHitInfo::EHitType::EHT_WATER:
			pComplexEffect = pWeapon->shells[nShell].pEffectHitWater;
			break;
		case SAINotifyHitInfo::EHitType::EHT_AIR_WITH_CRATER:
			{
				// Match the authoritative hit update if this combined outcome reaches projectile prediction.
				const NDb::SWeaponRPGStats::SShell &shell = pWeapon->shells[nShell];
				pComplexEffect = shell.pEffectHitAir;
				if ( shell.pCraters != 0 )
					PlaceCrater( shell.pCraters, eSeason, CVec2( vCenter.x, vCenter.y ) );
			}
			break;
		case SAINotifyHitInfo::EHitType::EHT_AIR:
			pComplexEffect = pWeapon->shells[nShell].pEffectHitAir;
			break;
		default:
			NI_VERIFY( false, StrFmt( "Invalid hit type %d", eHitType), return );
	}

	if ( pComplexEffect != 0 )
	{
		if ( pTarget && pTarget->IsRefValid() )
		{
			if ( pTarget->GetTypeID() == NDb::SBridgeRPGStats::typeID )
				// For bridges, show explosion on top
				PlayComplexEffect( OBJECT_ID_FORGET, pComplexEffect, Singleton<IGameTimer>()->GetGameTime(), vCenter );
			else
			{
				CVec3 vVisCenter;
				AI2Vis( &vVisCenter, vCenter );

				// ( M,N,O )
				CVec3 vO( vDir );
				CVec3 vN( (vDir ^ V3_AXIS_Z) ^ vDir );
				CVec3 vM( vN ^ vO );

				Normalize( &vO );
				Normalize( &vN );
				Normalize( &vM );

				SHMatrix mPlace
					(
						vM.x,	vN.x, vO.x,	vVisCenter.x,
						vM.y,	vN.y,	vO.y,	vVisCenter.y,
						vM.z,	vN.z,	vO.z,	vVisCenter.z,
							 0,		 0,		 0,					   1 );

				PlayComplexEffect( OBJECT_ID_FORGET, pComplexEffect, Singleton<IGameTimer>()->GetGameTime(), mPlace );
			}
		}
		else
			PlayComplexEffect( OBJECT_ID_FORGET, pComplexEffect, Singleton<IGameTimer>()->GetGameTime(), vCenter );
	}
}

void CMOProjectile::SetM1Info( CMapObj *_pTarget, const NDb::SWeaponRPGStats* _pWeapon, int _nShell, bool _bTraceTargetIntersection, float _fDamage )
{
	pTarget = _pTarget;
	pWeapon = _pWeapon;
	nShell = _nShell;
	bTraceTargetIntersection = _bTraceTargetIntersection;
	fDamage = _fDamage;
}

REGISTER_SAVELOAD_CLASS( 0x300C2400, CMOProjectile )

