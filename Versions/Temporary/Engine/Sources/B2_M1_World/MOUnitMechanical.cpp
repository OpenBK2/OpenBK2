#include "stdafx.h"

#include "Stats_B2_M1/DBAnimB2.h"
#include "Stats_B2_M1/DBAttachedModelVisObj.h"
#include "IdleMechProcess.h"
#include "MOProjectile.h"
#include "MOUnitMechanical.h"
#include "B2_M1_Terrain/fmtVSO.h"

#include "Input/Bind.h"
#include "Main/GameTimer.h"
#include "Misc/Win32Random.h"
#include "SceneB2/AttachedObj.h"
#include "SceneB2/Camera.h"
#include "Sound/SoundScene.h"
#include "Stats_B2_M1/AdditionalActions.h"
#include "Stats_B2_M1/IClientGameConsts.h"
#include "System/Text.h"
#include "Common_RTS_AI/AIClasses.h"

namespace
{

typedef hash_map< NDb::EDesignUnitType, SIconsSetInfo, SEnumHash > CIconsSet;
static bool bIsInitializedByDB = false;
CIconsSet iconsSets;
SIconsSetInfo iconsSetDefault;

static const int GetAttachedGunID( const int nPlatform, const int nGun )
{
	return (nPlatform << 6) | nGun;
}

const SIconsSetInfo& GetDBIconsSet( NDb::EDesignUnitType eType )
{
	if ( !bIsInitializedByDB )
	{		
		const NDb::SClientGameConsts *pClient = Singleton<IClientGameConsts>()->GetClientGameConsts();
		
		for ( int i = 0; i < pClient->mechUnitIconsSets.size(); ++i )
		{
			const NDb::SClientGameConsts::SMechUnitIconsSet &iconsSet = pClient->mechUnitIconsSets[i];
			iconsSets[iconsSet.eType] = SIconsSetInfo( iconsSet.fRaising, iconsSet.fHPBarLen );
			if ( iconsSet.eType == NDb::UNIT_TYPE_UNKNOWN )
			{
				iconsSetDefault.fRaising = iconsSet.fRaising;
				iconsSetDefault.fHPBarLen = iconsSet.fHPBarLen;
			}
		}

		bIsInitializedByDB = true;
	}

	CIconsSet::const_iterator it = iconsSets.find( eType );
	if ( it == iconsSets.end() )
		return iconsSetDefault;
	return it->second;
}

} //namespace

bool CMOUnitMechanical::IsInside( const int nID )
{
	for ( vector< CPtr<CMOSelectable> >::iterator it = vPassangers.begin(); it != vPassangers.end(); ++it )
	{
		if ( (*it)->GetID() == nID )
			return true;
	}
	return false;
}

void CMOUnitMechanical::SetDiveSound( bool bDive )
{
	if ( GetStatsLocal()->pSoundDive )
	{
		if ( bDive )
			AttachSound( EAST_PLANE_DIVE, GetStatsLocal()->pSoundDive, false );
		else
			DetachSound( EAST_PLANE_DIVE );
	}
}

void CMOUnitMechanical::InitAttached( const NDb::ESeason eSeason, IChooseAttached *pChooseFunc )
{
	const NDb::SMechUnitRPGStats* pStats = GetStatsLocal();
	const int nID = GetID();
	IScene *pScene = Scene();

	for ( int i = 0; i < pStats->GetPlatformsSize( nID ); ++i )
	{
		const NDb::SMechUnitRPGStats::SPlatform &platform = pStats->GetPlatform( nID, i );
		const NDb::SAttachedModelVisObj *pAttachedObj = platform.pAttachedPlatformVisObj;
		TryToAttach( pAttachedObj, pChooseFunc, eSeason, platform.szAttachedPlatformLocator, ESSOT_PLATFORMS, i );

		for ( int j = 0; j < pStats->GetGunsSize( nID, i ); ++j )
		{
			const NDb::SBaseGunRPGStats &gun = pStats->GetGun( nID, i, j );
			const NDb::SAttachedModelVisObj *pAttachedObj = gun.pAttachedGunVisObj;
			TryToAttach( pAttachedObj, pChooseFunc, eSeason, gun.szAttachedGunLocator, ESSOT_GUNS, GetAttachedGunID( i, j ) );
		}
	}

	for ( int i = 0; i < pStats->slots.size(); ++i )
	{
		const int nObject = Singleton<CConstructorInfo>()->GetSlotObject( nID, i );
		if ( nObject >= 0 && nObject < pStats->slots[i].attachedVisObjects.size() )
		{
			const NDb::SAttachedModelVisObj *pAttachedObj = pStats->slots[i].attachedVisObjects[nObject];
			TryToAttach( pAttachedObj, pChooseFunc, eSeason, pStats->slots[i].szAttachedLocator, ESSOT_SLOTS, i );
		}
	}
}

bool CMOUnitMechanical::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	bMoved = false;
	bArtilleryHooked = false;
	nLastTrackTime = -1;
	wLastTrackDir = 0;
	bForwardMoving = false;
	bTrackBroken = false;
	const bool bResult = CMOUnit::Create( nUniqueID, _pUpdate, eSeason, eDayTime, bInEditor );
	const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats *>( GetStats() );

	if ( pStats->eSelectionType == NDb::SELECTION_TYPE_CANNOT_SELECT )
		SetCanSelect( false );

	if ( pStats->bLeavesTracks || pStats->pEffectWheelDust )
	{
		lastTrackPoints.resize( 5 );
		for ( int i = 0; i < 5; ++i )
			lastTrackPoints[i] = VNULL3;
		trackPoints.resize( 8 );
		for ( int i = 0; i < 8; ++i )
			trackPoints[i] = VNULL3;
		fTrackWidth = pStats->vAABBHalfSize.x * 2.0f * pStats->fTrackWidth;
	}
	else
	{
		lastTrackPoints.resize( 0 );
		trackPoints.resize( 0 );
		fTrackWidth = 0;
	}

	if ( bResult ) 
	{
		if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID()) )
		{
			// setup jogging
			CDynamicCast<NAnimation::IGetBone> pGetBone = pAnimator;
			const int nBoneIndex = pGetBone->GetBoneIndex( "Basis" );
			if ( nBoneIndex >= 0 )
			{
				pJoggingMutator = MakeObject<IMechUnitJoggingMutator>( IMechUnitJoggingMutator::typeID );
				const IMechUnitJoggingMutator::SJoggingParams jx = { pStats->jx.fPeriod1, pStats->jx.fPeriod2, pStats->jx.fAmp1, 
					pStats->jx.fAmp2, pStats->jx.fPhaze1, pStats->jx.fPhaze2 };
				const IMechUnitJoggingMutator::SJoggingParams jy = { pStats->jy.fPeriod1, pStats->jy.fPeriod2, pStats->jy.fAmp1, 
					pStats->jy.fAmp2, pStats->jy.fPhaze1, pStats->jy.fPhaze2 };
				if ( pJoggingMutator )
					pJoggingMutator->Setup( nBoneIndex, jx, jy );
				pAnimator->SetSpecialMutator( pJoggingMutator );
			}
			// run default animation (idle)
			RunDefaultObjectAnimation( GetModelDesc()->pSkeleton, pAnimator );
		}
	}
	pIdleProcess = 0;

	if ( !bInEditor ) 
	{
		if ( pStats->iconsSetParams.bCustom )
			SetIconsSetInfo( SIconsSetInfo( pStats->iconsSetParams.fRaising, pStats->iconsSetParams.fHPBarLen ) );
		else
		{
			const SIconsSetInfo &info = GetDBIconsSet( GetStatsLocal()->eUnitType );
			SetIconsSetInfo( info );
		}
	}

	if ( bResult )
	{
		CPtr<IChooseAttached> pChooseFunc = new SChooseAttachedByDefault();
		InitAttached( eSeason, pChooseFunc );
		// hide attached
		if ( !IsVisible() )
			Scene()->ShowObject( GetID(), IsVisible() );
	}

	return bResult;
}

void CMOUnitMechanical::GetStatus( SObjectStatus *pStatus ) const
{
	CMOUnit::GetStatus( pStatus );

	if ( !GetStatsLocal() )
		return;

	// Armor
	if ( GetStatsLocal()->armors.size() >= ARMOR_COUNT )
	{
		pStatus->armors[EOS_ARMOR_FRONT] = (GetStatsLocal()->armors[ARMOR_FRONT].fMin + GetStatsLocal()->armors[ARMOR_FRONT].fMax) / 2;
		pStatus->armors[EOS_ARMOR_SIDE] = (GetStatsLocal()->armors[ARMOR_SIDE_1].fMin + GetStatsLocal()->armors[ARMOR_SIDE_1].fMax +
			GetStatsLocal()->armors[ARMOR_SIDE_2].fMin + GetStatsLocal()->armors[ARMOR_SIDE_2].fMax) / 4;
		pStatus->armors[EOS_ARMOR_BACK] = (GetStatsLocal()->armors[ARMOR_BACK].fMin + GetStatsLocal()->armors[ARMOR_BACK].fMax) / 2;
		pStatus->armors[EOS_ARMOR_TOP] = (GetStatsLocal()->armors[ARMOR_TOP].fMin + GetStatsLocal()->armors[ARMOR_TOP].fMax) / 2;
	}

	// Weapons
	const int nID = GetID();
	for ( int i = 0; i < GetStatsLocal()->GetPlatformsSize( nID ); ++i )
	{
		const NDb::SMechUnitRPGStats::SPlatform &platform = GetStatsLocal()->GetPlatform( nID, i );

		for ( int j = 0; j < GetStatsLocal()->GetGunsSize( nID, i ); ++j )
		{
			const NDb::SBaseGunRPGStats &gun = GetStatsLocal()->GetGun( nID, i, j );
			if ( !gun.pWeapon )
				continue;

			SObjectStatus::SWeapon weapon;

			const NDb::SWeaponRPGStats::SShell *pShell = gun.pWeapon->shells.empty() ? 0 : &gun.pWeapon->shells.front();
			weapon.pWeaponID = gun.pWeapon;
			weapon.nCount = 1;
			weapon.bPrimary = gun.bIsPrimary;

			if ( CHECK_TEXT_NOT_EMPTY_PRE(gun.pWeapon->,LocalizedName) )
				weapon.szLocalizedName = GET_TEXT_PRE(gun.pWeapon->,LocalizedName);

			weapon.nDamage = pShell->fDamagePower;
			weapon.nPenetration = pShell->nPiercing;
			weapon.nMaxAmmo = gun.nAmmo;

			if ( weapon.pWeaponID && weapon.pWeaponID->eWeaponType != NDb::SWeaponRPGStats::WEAPON_HIDED )
				pStatus->AddWeapon( weapon );
		}
	}

	// Ammo total
	for ( int i = 0; i < pStatus->weapons.size(); ++i )
	{
		SObjectStatus::SWeapon &weapon = pStatus->weapons[i];

		weapon.nAmmo = GetWeaponAmmo( weapon.pWeaponID );
	}

	pStatus->bIsTransport = GetStatsLocal()->IsTransport();
}

void CMOUnitMechanical::GetActions( CUserActions *pActions, EActionsType eActions ) const
{
	CMOUnit::GetActions( pActions, eActions );
	const NDb::SMechUnitRPGStats * pStats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );

	if ( eActions == ACTIONS_BY || eActions == ACTIONS_ALL )
	{		
		if ( bArtilleryHooked )
			pActions->RemoveAction( NDb::USER_ACTION_HOOK_ARTILLERY );
		else
			pActions->RemoveAction( NDb::USER_ACTION_DEPLOY_ARTILLERY );
		if ( pStats && pStats->IsTransport() && GetSupply() < 1000.0f )
			pActions->SetAction( NDb::USER_ACTION_FILL_RU );
		else
			pActions->RemoveAction( NDb::USER_ACTION_FILL_RU );

		if ( pStats && pActions->HasAction( NDb::USER_ACTION_MOVE ) )
		{
			if ( (pStats->nAIPassabilityClass & EAC_TRACK) != 0 )
				pActions->SetAction( NDb::USER_ACTION_MOVE_TRACK );
			if ( (pStats->nAIPassabilityClass & EAC_WHELL) != 0 )
				pActions->SetAction( NDb::USER_ACTION_MOVE_WHELL );
		}
	}
	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		if ( !IsNeutral() )
			pActions->RemoveAction( NDb::USER_ACTION_CAPTURE_ARTILLERY );
		// break track for tanks (CRAP)
		if ( (pStats->IsArmor() || pStats->IsSPG() ) && pStats->eSelectionType != NDb::SELECTION_TYPE_WATER )
			pActions->SetAction( NDb::USER_ACTION_TRACK_TARGETING );		
		// human
		if ( GetFreePlaces() <= 0 || IsNeutral() )
			pActions->RemoveAction( NDb::USER_ACTION_BOARD );
		// mech
		if ( GetFreeMechPlaces() <= 0 || IsNeutral() )
			pActions->RemoveAction( NDb::USER_ACTION_MECH_BOARD );
		if ( GetHP() < 1.0f )
			pActions->SetAction( NDb::USER_ACTION_ENGINEER_REPAIR );
		else
			pActions->RemoveAction( NDb::USER_ACTION_ENGINEER_REPAIR );
		// count ammo for guns
		bool bResupply = false;
		if ( pStats && !pStats->IsAviation() )
		{
			int nAmmo = 0;
			const int nID = GetID();
			if ( pStats )
			{
				for ( int i = 0; i < pStats->GetPlatformsSize( nID ); ++i )
				{
					for ( int j = 0; j < pStats->GetGunsSize( nID, i ); ++j )
					{
						const NDb::SBaseGunRPGStats &gun = pStats->GetGun( nID, i, j );
						nAmmo += gun.nAmmo;
					}
				}
				if ( GetWeaponAmmoTotal() < nAmmo )
					bResupply = true;
			}
		}
		if ( bResupply )
			pActions->SetAction( NDb::USER_ACTION_SUPPORT_RESUPPLY );
		else
			pActions->RemoveAction( NDb::USER_ACTION_SUPPORT_RESUPPLY );
	}
}

void CMOUnitMechanical::GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const
{
	CMOUnit::GetDisabledActions( pActions, eActions );

	if ( eActions == ACTIONS_BY || eActions == ACTIONS_ALL )
	{
		if ( vPassangers.empty() /*|| bArtilleryHooked*/ )
			pActions->SetAction( NDb::USER_ACTION_LEAVE );
		if ( !IsOpen() || (pTransport && !pTransport->IsOpen()) )
		{
			pActions->SetAction( NDb::USER_ACTION_LEAVE );
			pActions->SetAction( NDb::USER_ACTION_BOARD );
			pActions->SetAction( NDb::USER_ACTION_MECH_BOARD );
		}
	}
}

void CMOUnitMechanical::AIUpdateState( const int nParam )
{
	switch( nParam ) 
	{
	case ECS_HOOK_CANNON:
		bArtilleryHooked = true;
		break;
	case ECS_UNHOOK_CANNON:
		bArtilleryHooked = false;
		break;
	}
	if ( IsSelected() )
	{
		NInput::PostEvent( "update_selected_unit", 0, 0 );
	}
}

const CVec3 CMOUnitMechanical::GetFirePoint( const int nPlatform, const int nGun ) const
{
	const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );
	const NDb::SMechUnitRPGStats::SMechUnitGun &gun = pStats->GetGun( GetID(), nPlatform, nGun );
	NI_VERIFY( !gun.szShootPoint.empty(), StrFmt( "Shoot point not defined for gun %d at platform %d", nGun, nPlatform ), return VNULL3 );
	NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID(), gun.szShootPoint );
	if ( !pAnimator )
		return VNULL3;
	SHMatrix mShootPoint;
	NI_VERIFY( pAnimator->GetBonePosition( gun.szShootPoint.c_str(), &mShootPoint ), StrFmt( "Shoot point not found for gun %d at platform %d (bone's name \"%s\")", nGun, nPlatform, gun.szShootPoint.c_str() ), return VNULL3 );
	return CVec3( mShootPoint._14, mShootPoint._24, mShootPoint._34 );
}

void CMOUnitMechanical::AIUpdateShot( const SAINotifyBaseShot &_shot, const NTimer::STime &currTime, IScene *pScene, NDb::ESeason eSeason )
{																																								
	const SAINotifyMechShot &shot = *( static_cast<const SAINotifyMechShot*>(&_shot) );

	const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );

	// откат и отдача орудия
	SHMatrix mShootPoint;
	const int nID = GetID();
	const NDb::SMechUnitRPGStats::SMechUnitGun &gun = pStats->GetGun( nID, shot.cPlatform, shot.cGun );
	if ( !gun.szShootPoint.empty() ) 
	{
		NAnimation::ISkeletonAnimator *pAnimator = pScene->GetAnimator( nID, gun.szShootPoint );
		if ( !pAnimator )
			return;

		const NDb::SComplexEffect *pComplexEffect = gun.pWeapon->shells[shot.cShell].pEffectGunFire;
		if ( !pAnimator->GetBonePosition( gun.szShootPoint.c_str(), &mShootPoint ) )
		{
			NI_ASSERT( 0, StrFmt( "Shoot point not found for gun %d at platform %d (bone's name \"%s\")", shot.cGun, shot.cPlatform, gun.szShootPoint.c_str() ) );
			return;
		}
		if ( pStats->animdescs.size() > NDb::ANIMATION_SHOOT && !pStats->animdescs[NDb::ANIMATION_SHOOT].anims.empty() )
		{
			if ( pComplexEffect != 0 && IsVisible() )
				PlayComplexEffect( OBJECT_ID_FORGET, pComplexEffect, _shot.time, mShootPoint );
		}
		else if ( gun.bRecoil )
		{
			vector<NAnimation::ISkeletonAnimator::SDesiredBoneMove> mutator;
			mutator.push_back( NAnimation::ISkeletonAnimator::SDesiredBoneMove( gun.nrecoilTime, QNULL, CVec3( 0, -gun.fRecoilLength, 0 ) ) );
			mutator.push_back( NAnimation::ISkeletonAnimator::SDesiredBoneMove( gun.nRecoilShakeTime - gun.nrecoilTime, QNULL, VNULL3 ) );

			pAnimator->SetBoneMutator( gun.szRecoilPoint.c_str(), currTime, mutator );

			CVec3 vRot;
			SHMatrix mBasisInv, mBasis, mLocalShoot;
			pAnimator->GetBonePosition( 0, &mBasis );
			Invert( &mBasisInv, mBasis );
			Multiply( &mLocalShoot, mBasisInv, mShootPoint );
			mLocalShoot.RotateVector( &vRot, V3_AXIS_X );

			CQuat qRecoil( gun.fRecoilShakeAngle, vRot );
			vector<NAnimation::ISkeletonAnimator::SDesiredBoneMove> mutatorBasis;
			mutatorBasis.push_back( NAnimation::ISkeletonAnimator::SDesiredBoneMove( (float)(gun.nRecoilShakeTime)*0.1, qRecoil ) );
			mutatorBasis.push_back( NAnimation::ISkeletonAnimator::SDesiredBoneMove( (float)(gun.nRecoilShakeTime)*0.9, QNULL ) );
			pAnimator->SetBoneMutator( 0, currTime, mutatorBasis ); // 0 as const char *

			if ( pComplexEffect != 0 && IsVisible() )
				PlayComplexEffect( OBJECT_ID_FORGET, pComplexEffect, _shot.time, mShootPoint );
		}
		else if ( pComplexEffect != 0 && IsVisible() )
			PlayComplexEffect( nID, gun.szShootPoint, ESSOT_GUN_FIRE, pComplexEffect, _shot.time, ESAT_REPLACE_ON_BONE );
		// play effect shoot dust (for primary guns)
		if ( IsVisible() && gun.bIsPrimary && pStats->pEffectShootDust != 0 )
			PlayComplexSeasonedEffect( OBJECT_ID_FORGET, pStats->pEffectShootDust, _shot.time, GetCenter(), GetOrientation(), eSeason );
		//
		AddShotTrace( gun.pWeapon, shot, CVec3(mShootPoint._14, mShootPoint._24, mShootPoint._34), currTime, pScene );
		// earthquake
		if ( gun.pWeapon->shells[shot.cShell].fDetonationPower > 0 )
		{
			Camera()->AddEarthquake( CVec3( mShootPoint._14, mShootPoint._24, mShootPoint._34 ), 
				                      gun.pWeapon->shells[shot.cShell].fDetonationPower );
		}
	}
}

void CMOUnitMechanical::AddShotTrace( const CDBPtr<NDb::SWeaponRPGStats> pWeapon, const struct SAINotifyBaseShot &shot,
																			const CVec3 &vStart, const NTimer::STime &currTime, IScene *pScene )
{
	if ( (pWeapon->shells[shot.cShell].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE) &&
			 (NWin32Random::Random( 100 ) + 1 <= pWeapon->shells[shot.cShell].fTraceProbability * 100.0f) )
	{
		//CVec3 vStart( mShootPoint._14, mShootPoint._24, mShootPoint._34 );
		CVec3 vEnd( shot.vDestPos );
		AI2Vis( &vEnd );
		if ( vEnd.z == 0 )
			vEnd.z = AI2Vis( Scene()->GetZ(shot.vDestPos.x, shot.vDestPos.y) );
		//
		pScene->AddShotTrace( vStart, vEnd, currTime, &(pWeapon->shells[shot.cShell]) );
		//pScene->AddLaserMark( vStart, vEnd );
	}
}

void CMOUnitMechanical::AIUpdateTurretTurn( const struct SAINotifyTurretTurn &turn, const NTimer::STime &currTime, IScene *pScene, const bool bHorTurn )
{
	CQuat qRotTurret;
	vector<NAnimation::ISkeletonAnimator::SDesiredBoneMove> mutator;
	const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );
	const int nID = GetID();
	const NDb::SMechUnitRPGStats::SPlatform &platform = pStats->GetPlatform( nID, turn.nPlantform );
	if ( bHorTurn )
	{
		const string szBoneName = platform.pAttachedPlatformVisObj ? platform.szAttachedPlatformLocator : platform.szRotatePoint;
		if ( !szBoneName.empty() ) 
		{
			NAnimation::ISkeletonAnimator *pAnimator = pScene->GetAnimator( nID, szBoneName );
			if ( pAnimator )
			{
				qRotTurret.FromAngleAxis( AI2VisRad(turn.wAngle), V3_AXIS_Z );
				mutator.push_back( NAnimation::ISkeletonAnimator::SDesiredBoneMove( turn.endTime - currTime, qRotTurret ) );
				pAnimator->SetBoneMutator( szBoneName.c_str(), currTime, mutator );
			}
		}
	}
	else
	{
		qRotTurret.FromAngleAxis( AI2VisRad(turn.wAngle), V3_AXIS_X );
		mutator.push_back( NAnimation::ISkeletonAnimator::SDesiredBoneMove( turn.endTime - currTime, qRotTurret ) );
		for ( int i = 0; i < pStats->GetGunsSize( nID, turn.nPlantform ); ++i )
		{
			const NDb::SMechUnitRPGStats::SMechUnitGun &gun = pStats->GetGun( nID, turn.nPlantform, i );
			const string szBoneName = gun.pAttachedGunVisObj ? gun.szAttachedGunLocator : gun.szRotatePoint;

			if ( !szBoneName.empty() )
			{
				NAnimation::ISkeletonAnimator *pAnimator = pScene->GetAnimator( nID, szBoneName );
				if ( pAnimator )
					pAnimator->SetBoneMutator( szBoneName.c_str(), currTime, mutator );
			}
		}
	}
}

void CMOUnitMechanical::AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, ISoundScene *pSoundScene, NDb::ESeason eSeason )
{
	if ( NGlobal::GetVar( "m1", 0 ) == 0 && GameTimer()->GetPauseType() != -1 )
		return;
	CMOUnit::AIUpdatePlacement( placement, pScene, pSoundScene, eSeason );
	const NDb::SMechUnitRPGStats *pStats = GetStatsLocal();
	// in-transport units don't leave tracks, no tracks on zero speed
	if ( ( pStats->bLeavesTracks || pStats->pEffectWheelDust != 0 ) && !pTransport && placement.fSpeed > 0.0f ) 
	{
		if ( nLastTrackTime < 0 )
		{
			//initialize data
			bForwardMoving = true;
			CVec2 vBack = pStats->vAABBCenter;
			vBack.y -= pStats->vAABBHalfSize.y * ( 1.0f - pStats->fTrackEnd * 0.5f );
			trackPoints[0].Set( vBack.x - pStats->vAABBHalfSize.x * ( 1.0f - pStats->fTrackOffset * 0.5f ), vBack.y, 0 );
			trackPoints[1].Set( trackPoints[0].x + fTrackWidth, trackPoints[0].y, 0 );
			trackPoints[3].Set( vBack.x + pStats->vAABBHalfSize.x * ( 1.0f - pStats->fTrackOffset * 0.5f ), vBack.y, 0 );
			trackPoints[2].Set( trackPoints[3].x - fTrackWidth, trackPoints[3].y, 0 );			
			CVec2 vFront = pStats->vAABBCenter;
			vFront.y += pStats->vAABBHalfSize.y * ( 1.0f - pStats->fTrackStart * 0.5f );
			trackPoints[4].Set( vFront.x - pStats->vAABBHalfSize.x * ( 1.0f - pStats->fTrackOffset * 0.5f ), vFront.y, 0 );
			trackPoints[5].Set( trackPoints[4].x + fTrackWidth, trackPoints[4].y, 0 );
			trackPoints[7].Set( vFront.x + pStats->vAABBHalfSize.x * ( 1.0f - pStats->fTrackOffset * 0.5f ), vFront.y, 0 );
			trackPoints[6].Set( trackPoints[7].x - fTrackWidth, trackPoints[7].y, 0 );			
		}
		bool bPlaceSegment = false;
		bool bNewDir = bForwardMoving;
		WORD wDirRange = abs( wLastTrackDir - placement.dir ); 
		if ( nLastTrackTime < 0 || ( Singleton<IGameTimer>()->GetGameTime() - nLastTrackTime > pStats->nTrackLifetime ) || ( wDirRange > 32768 ? wDirRange < ( 65536 - 8192 ) : wDirRange > 8192 ) )
		{
			//reset old placed track points, center and dir, put current segment	
			bPlaceSegment = true;
			if ( nLastTrackTime >= 0 )
			{
				const CVec2 vDir = GetVectorByDirection( placement.dir );
				float fDir = vDir.x * ( placement.center.x - lastTrackPoints[4].x ) + vDir.y * ( placement.center.y - lastTrackPoints[4].y );
				bNewDir = ( fDir == 0 ? bForwardMoving : ( fDir > 0 ) );
			}
			lastTrackPoints[4].Set( placement.center.x, placement.center.y, placement.z );
			wLastTrackDir = placement.dir;
			CVec3 vPos, vScale;
			CQuat qRot;
			GetPlacement( &vPos, &qRot, &vScale );
			int nDirectionOffset = bForwardMoving ? 0 : 4;
			for ( int i = 0; i < 4; ++i )
			{
				qRot.Rotate( &( lastTrackPoints[i] ), trackPoints[i + nDirectionOffset] );
				lastTrackPoints[i] += vPos;		
			}
		}
		else
		{
			//range updates
			float fRange = fabs2( lastTrackPoints[4] - CVec3( placement.center.x, placement.center.y, placement.z ) );
			bPlaceSegment = ( fRange > 0 && ( wDirRange > 32768 ? wDirRange < ( 65536 - 2048 ) : wDirRange > 2048 ) ) || fRange > sqr( pStats->vAABBHalfSize.y * 2.0f * pStats->fTrackFrequency );
			const CVec2 vDir = GetVectorByDirection( placement.dir );
			float fDir = vDir.x * ( placement.center.x - lastTrackPoints[4].x ) + vDir.y * ( placement.center.y - lastTrackPoints[4].y );
			bNewDir = ( fDir == 0 ? bForwardMoving : ( fDir > 0 ) );
		}
		if ( bNewDir != bForwardMoving )
			bPlaceSegment = true;
		if ( bPlaceSegment )
		{
			//get point coordinates
			CVec3 vPos, vScale;
			CQuat qRot;
			GetPlacement( &vPos, &qRot, &vScale );
			CVec3 newTrackPoints[4];
			int nDirectionOffset = bForwardMoving ? 4 : 0;
			for ( int i = 0; i < 4; ++i )
			{
				qRot.Rotate( &( newTrackPoints[i] ), trackPoints[i + nDirectionOffset] );
				newTrackPoints[i] += vPos;
			}	

			//calc normale
			CVec3 vNorm3;
			qRot.Rotate( &vNorm3, CVec3( 1.0f, 0 , 0 ) );
			CVec2 vNorm2( vNorm3.x, vNorm3.y );

			//put tracks
			CVec2 curPts[8];
			CVec3 dustPts[2];
			SHMatrix mDustPlace1, mDustPlace2;
			AI2Vis( &vPos );
			SHMatrix mOwnPlace( vPos, qRot );
			SHMatrix mMirror( -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );
			if ( bForwardMoving )
			{
				curPts[0].Set( lastTrackPoints[1].x, lastTrackPoints[1].y );
				curPts[1].Set( lastTrackPoints[0].x, lastTrackPoints[0].y );
				curPts[2].Set( newTrackPoints[0].x, newTrackPoints[0].y );
				curPts[3].Set( newTrackPoints[1].x, newTrackPoints[1].y );
				curPts[4].Set( lastTrackPoints[3].x, lastTrackPoints[3].y );
				curPts[5].Set( lastTrackPoints[2].x, lastTrackPoints[2].y );
				curPts[6].Set( newTrackPoints[2].x, newTrackPoints[2].y );
				curPts[7].Set( newTrackPoints[3].x, newTrackPoints[3].y );
				dustPts[0] = trackPoints[0];
				AI2Vis( &(dustPts[0]) );
				mDustPlace1.Set( dustPts[0], QNULL );
				dustPts[1] = trackPoints[3];
				AI2Vis( &(dustPts[1]) );
				mDustPlace2.Set( dustPts[1], QNULL );
			}
			else
			{
				curPts[0].Set( newTrackPoints[1].x, newTrackPoints[1].y );
				curPts[1].Set( newTrackPoints[0].x, newTrackPoints[0].y );
				curPts[2].Set( lastTrackPoints[0].x, lastTrackPoints[0].y );
				curPts[3].Set( lastTrackPoints[1].x, lastTrackPoints[1].y );
				curPts[4].Set( newTrackPoints[3].x, newTrackPoints[3].y );
				curPts[5].Set( newTrackPoints[2].x, newTrackPoints[2].y );				
				curPts[6].Set( lastTrackPoints[2].x, lastTrackPoints[2].y );
				curPts[7].Set( lastTrackPoints[3].x, lastTrackPoints[3].y );
				dustPts[0] = trackPoints[4];
				AI2Vis( &(dustPts[0]) );
				mDustPlace1.Set( dustPts[0], QNULL );
				dustPts[1] = trackPoints[7];
				AI2Vis( &(dustPts[1]) );
				mDustPlace2.Set( dustPts[1], QNULL );
			}
			mDustPlace2 = mDustPlace2 * mMirror;
			mDustPlace1 = mOwnPlace * mDustPlace1;
			mDustPlace2 = mOwnPlace * mDustPlace2;

			/*//Debug
			dustPts[0] = mDustPlace1.GetTrans3();
			dustPts[1] = mDustPlace2.GetTrans3();
			CSegment segm;
			segm.p1 = CVec2( dustPts[0].x, dustPts[0].y );
			Vis2AI( &segm.p1 );
			segm.p2 = CVec2( vPos.x, vPos.y );
			Vis2AI( &segm.p2 );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 1, NDebugInfo::RED );
			segm.p1 = CVec2( dustPts[1].x, dustPts[1].y );
			Vis2AI( &segm.p1 );
			segm.p2 = CVec2( vPos.x, vPos.y );
			Vis2AI( &segm.p2 );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 1, NDebugInfo::GREEN );*/

			if ( IsVisible() )
			{
				// CRAP{ fake test for the ship
				const bool bIsShip = pStats->shipEffects.pBoardSideEffect != 0 || pStats->shipEffects.pRastrumEffect != 0;
				// CRAP}
				//				const string szText = StrFmt( "%d, %g, {%g, %g}, {%g, %g}, {%g, %g}, {%g, %g}, {%g, %g}, %g, %g",
				//					nID, fFadingSpeed, _v1.x, _v1.y, _v2.x, _v2.y, _v3.x, _v3.y, _v4.x, _v4.y, vNorm.x, vNorm.y, _fWidth, fAplha );
				//				const string szText = StrFmt( "%d => %x", GetID(), int(placement.cSoil) );
				//				Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW, szText.c_str(), 0xff00ff00 );
				//
				if ( pStats->bLeavesTracks && ( placement.cSoil & SVectorStripeObjectDesc::ESP_TRACE ) )
				{
					pScene->AddTrack( GetID(), 256.0f / pStats->nTrackLifetime, curPts[0], curPts[1], curPts[2], curPts[3], vNorm2, fTrackWidth * 0.5f, 1.0f - pStats->fTrackIntensity );
					pScene->AddTrack( GetID(), 256.0f / pStats->nTrackLifetime, curPts[4], curPts[5], curPts[6], curPts[7], vNorm2, fTrackWidth * 0.5f, 1.0f - pStats->fTrackIntensity );
				}
				if ( bForwardMoving && (pStats->pEffectWheelDust != 0) && 
					   ( (placement.cSoil & SVectorStripeObjectDesc::ESP_DUST) || bIsShip) )
				{
					PlayComplexSeasonedEffect( OBJECT_ID_FORGET, pStats->pEffectWheelDust, Singleton<IGameTimer>()->GetGameTime(), mDustPlace1, eSeason );
					PlayComplexSeasonedEffect( OBJECT_ID_FORGET, pStats->pEffectWheelDust, Singleton<IGameTimer>()->GetGameTime(), mDustPlace2, eSeason );
				}
				if ( bForwardMoving && (pStats->pEffectWheelSplash != 0) && 
					( (placement.cSoil & SVectorStripeObjectDesc::ESP_SPLASH) || bIsShip) )
				{
					PlayComplexSeasonedEffect( OBJECT_ID_FORGET, pStats->pEffectWheelSplash, Singleton<IGameTimer>()->GetGameTime(), mDustPlace1, eSeason );
					PlayComplexSeasonedEffect( OBJECT_ID_FORGET, pStats->pEffectWheelSplash, Singleton<IGameTimer>()->GetGameTime(), mDustPlace2, eSeason );
				}
				if ( pStats->bLeavesTracks && bForwardMoving && pStats->shipEffects.pRastrumEffect != 0 )
				{
					for ( int i = 0; i < pStats->shipEffects.rastrumLocators.size(); ++i )
						pScene->AttachEffect( GetID(), ESSOT_WATER_DROPS, pStats->shipEffects.rastrumLocators[i], pStats->shipEffects.pRastrumEffect->GetSceneEffect(), Singleton<IGameTimer>()->GetGameTime(), ESAT_NO_REPLACE );
				}
			}

			//remember for future use
			for ( int i = 0; i < 4; ++i )
				lastTrackPoints[i] = newTrackPoints[i];
			nLastTrackTime = Singleton<IGameTimer>()->GetGameTime();
			lastTrackPoints[4].Set( placement.center.x, placement.center.y, placement.z );
			wLastTrackDir = placement.dir;
		}
		bForwardMoving = bNewDir;
	}
	// trail effect
	if ( !smokeTrails.empty() )
	{
		CVec3 vPos, vScale;
		CQuat qRot;
		GetPlacement( &vPos, &qRot, &vScale );
		AI2Vis( &vPos );
		const NTimer::STime currTime = GameTimer()->GetGameTime();
		for ( vector< CObj<CSmokeTrailEffect> >::iterator it = smokeTrails.begin(); it != smokeTrails.end(); ++it )
			(*it)->UpdatePlacement( vPos, qRot, currTime, IsVisible() );
	}
}

void CMOUnitMechanical::SetTransport( IMOContainer *_pTransport ) 
{ 
	bool bChanged = (pTransport != _pTransport);

	pTransport = _pTransport; 

	if ( pJoggingMutator )
		pJoggingMutator->Stop();
	DetachSound( EAST_MOVEMENT );
	DetachSound( EAST_IDLE );
	bMoved = FALSE;

	if ( bChanged )
		UpdateVisibility( true );
}

IClientUpdatableProcess* CMOUnitMechanical::AIUpdateMovement( const NTimer::STime &time, const bool _bMove, IScene *pScene, ISoundScene *pSoundScene )
{
	pIdleProcess = 0;
	if ( pTransport )
		return pIdleProcess;

	if ( _bMove )
	{
		if ( pJoggingMutator )
			pJoggingMutator->Play();
		if ( !bMoved )
		{
			bMoved = true;
			const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );
			pSoundScene->AddSound( pStats->pSoundMoveStart, GetCenter(), SFX_MIX_IF_TIME_EQUALS, SAM_ADD_N_FORGET, 0, 2 );
			AttachSound( EAST_MOVEMENT, pStats->pSoundMoveCycle, true );
			DetachSound( EAST_IDLE );
			// Play exhaust only for visible units, sound is played always
			if ( IsVisible() && !pStats->exhaustPoints.empty() && pStats->pEffectDiesel && pStats->pEffectDiesel->GetSceneEffect() )
			{			
				IScene *pScene = Scene();
				for ( int i = 0; i < pStats->exhaustPoints.size(); ++i )
					pScene->AttachEffect( GetID(), ESSOT_EXHAUST, pStats->exhaustPoints[i], pStats->pEffectDiesel->GetSceneEffect(), time, i == 0 ? ESAT_REPLACE_ON_TYPE : ESAT_NO_REPLACE );
			}
		}
	}
	else
	{
		DetachSound( EAST_MOVEMENT );
		const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );
		if ( pStats->pSoundMoveStop ) 
			pSoundScene->AddSound( pStats->pSoundMoveStop, GetCenter(), SFX_MIX_IF_TIME_EQUALS, SAM_ADD_N_FORGET, 0, 2 );
		if ( pJoggingMutator )
			pJoggingMutator->Stop();
		bMoved = false;
		if ( pStats->shipEffects.pBoardSideEffect != 0 && !pStats->shipEffects.boardSideLocators.empty() )
			pIdleProcess = new CIdleMechProcess( GetID(), pStats->shipEffects.boardSideLocators, pStats->shipEffects.pBoardSideEffect );
		//
		if ( pStats->pSoundIdle )
			AttachSound( EAST_IDLE, pStats->pSoundIdle, true );
	}
	return pIdleProcess;
}

bool CMOUnitMechanical::Load( struct IMOUnit *pMO, bool bEnter )
{
	const int nID = pMO->GetID();
	if ( bEnter )
	{
		if ( !IsInside( nID ) ) 
			vPassangers.push_back( pMO );
	}
	else
	{
		if ( vPassangers.empty() )
			return false;
		bool bShift = false;
		for ( int i = 0; i < vPassangers.size()-1; ++i )
		{
			if ( vPassangers[i]->GetID() == nID )
				bShift = true;
			if ( bShift )
				vPassangers[i] = vPassangers[i+1];
		}
		if ( bShift || vPassangers[vPassangers.size()-1]->GetID() == nID )
			vPassangers.resize( vPassangers.size()-1 );
	}
	if ( IsSelected() )
	{
		NInput::PostEvent( "update_selected_unit", 0, 0 );
	}
	UpdateIcons();
	return true;
}

bool CMOUnitMechanical::LoadSquad( struct IMOSquad *pSquad, bool bEnter )
{
	if ( pSquad == 0 )
		return true;
	pSquad->SetContainer( bEnter ? this : 0 );
	const int nID = pSquad->GetID();
	if ( bEnter )
	{
		if ( !IsInside( nID ) ) 
			vPassangers.push_back( pSquad );
	}
	else
	{
		if ( vPassangers.empty() )
			return false;
		bool bShift = false;
		for ( int i = 0; i < vPassangers.size()-1; ++i )
		{
			if ( vPassangers[i]->GetID() == nID )
				bShift = true;
			if ( bShift )
				vPassangers[i] = vPassangers[i+1];
		}
		if ( bShift || vPassangers[vPassangers.size()-1]->GetID() == nID )
			vPassangers.resize( vPassangers.size()-1 );
	}
	if ( IsSelected() )
	{
		NInput::PostEvent( "update_selected_unit", 0, 0 );
	}
	UpdateIcons();
	return true;
}

void CMOUnitMechanical::GetPassangers( vector<CMOSelectable*> *pBuffer ) const
{
	NI_ASSERT( pBuffer, "Wrong pointer" );
	pBuffer->resize( vPassangers.size() );
	for ( int i = 0; i < vPassangers.size(); ++i )
		(*pBuffer)[i] = vPassangers[i];
}

int CMOUnitMechanical::GetFreePlaces() const
{
	const NDb::SMechUnitRPGStats *stats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );
	return stats->nPassangers - (vPassangers.size() - GetMechPassangersCount());
}

int CMOUnitMechanical::GetFreeMechPlaces() const
{
	const NDb::SMechUnitRPGStats *stats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );
	return stats->boardedMechUnitPosition.size() - GetMechPassangersCount();
}

int CMOUnitMechanical::GetMechPassangersCount() const
{
	int nCount = 0;
	for ( int i = 0; i < vPassangers.size(); ++i )
	{
		if ( dynamic_cast<CMOUnitMechanical*>( vPassangers[i].GetPtr() ) )
			nCount++;
	}
	return nCount;
}

void CMOUnitMechanical::SetTrackBroken( const bool bNewValue )
{ 
	if ( bTrackBroken != bNewValue ) 
	{ 
		bTrackBroken = bNewValue; 
		UpdateIcons();
	} 
}

void CMOUnitMechanical::FillIconsInfo( SSceneObjIconInfo &iconInfo )
{
	SetIconsHitbar( IsVisible(), IsSelected() );
	SetIconsGroup( IsVisible() ? GetSelectionGroup() : -1, IsSelected() );

	CMOUnit::FillIconsInfo( iconInfo );

	// Display "Track broken"
//	if ( bTrackBroken )
//		iconInfo.icons.push_back( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_BROKENTRUCK );
}

const NDb::SAnimB2* CMOUnitMechanical::GetAnimB2(
	const NDb::SModel *pModel, const vector<NDb::Svector_AnimDescs> &animdescs, 
	const NDb::EAnimationType eAnimType, const int nAnimID )
{
	if ( pModel && pModel->pSkeleton )
	{
		if ( eAnimType < animdescs.size() && nAnimID >= 0 && nAnimID < animdescs[eAnimType].anims.size() )
		{
			const int nAnimSkeletonIndex = animdescs[eAnimType].anims[nAnimID].nFrameIndex;
			if ( nAnimSkeletonIndex >= 0 && nAnimSkeletonIndex < pModel->pSkeleton->animations.size() )			
				return dynamic_cast_ptr<const NDb::SAnimB2*>(pModel->pSkeleton->animations[nAnimSkeletonIndex]);
		}
	}

	return 0;
}

void CMOUnitMechanical::PlayAnimDescForAttached( IAttachedObject *pObject, const vector<NDb::Svector_AnimDescs> &animdescs,
																								const int nStartTime, const NDb::EAnimationType eAnimType, const int nAnimID )
{
	if ( pObject )
	{
		const NDb::SAnimB2* pAnimB2 = GetAnimB2( pObject->GetModel(), animdescs, eAnimType, nAnimID );
		if ( pAnimB2 && pObject->GetAnimator() )
		{
			SFBTransform transform;
			if ( Scene()->GetVisObjPlacement(GetID(), &transform) )
			{
				CPtr< CFuncBase<SFBTransform> > pTransform = new CCSFBTransform( transform );

				pObject->SetTransform( pTransform );
				IScene *pScene = Scene();
				pObject->ReCreate( pScene->GetGView(), pScene->GetGameTimer() );

				NAnimation::ISkeletonAnimator *pAnimator = pObject->GetAnimator();
				if ( pAnimator )
				{
					pAnimator->ClearAllAnimations();
					SetLoopedAnimation( pAnimB2->bLooped );
					AddAnimation( pAnimB2, nStartTime, pAnimator );
				}
			}
		}
	}
}

void CMOUnitMechanical::PlayDieAnimation( const SAIDeadUnitUpdate *pUpdate )
{
	const int nID = GetID();
	NDb::EAnimationType eAnimType = NDb::ANIMATION_DEATH;
	const int nStartTime = Min( pUpdate->dieAnimation.time, GameTimer()->GetGameTime() );
	if ( pUpdate->dieAnimation.nParam != -1 ) 
	{
		eAnimType = NDb::EAnimationType( ( pUpdate->dieAnimation.nParam >> 16 ) & 0x00000fff );
		const int nAnimID = pUpdate->dieAnimation.nParam & 0x0000ffff;
		const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );

		NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nID );
		const NDb::SAnimB2* pAnimB2 = GetAnimB2( GetModelDesc(), pStats->animdescs, eAnimType, nAnimID );
		if ( pAnimB2 && pAnimator )
		{
			pAnimator->ClearAllAnimations();
			SetLoopedAnimation( pAnimB2->bLooped );
			AddAnimation( pAnimB2, nStartTime, pAnimator );
		}

		IScene *pScene = Scene();
		for ( int i = 0; i < pStats->GetPlatformsSize( nID ); ++i )
		{
			const NDb::SMechUnitRPGStats::SPlatform &platform = pStats->GetPlatform( nID, i );
			if ( platform.pAttachedPlatformVisObj )
			{
				PlayAnimDescForAttached( 
					pScene->GetAttached( nID, ESSOT_PLATFORMS, i ), platform.pAttachedPlatformVisObj->animdescs,
					nStartTime, eAnimType, nAnimID );
			}

			for ( int j = 0; j < pStats->GetGunsSize( nID, i ); ++j )
			{
				const NDb::SBaseGunRPGStats &gun = pStats->GetGun( nID, i, j );
				if ( gun.pAttachedGunVisObj )
				{
					PlayAnimDescForAttached(
						pScene->GetAttached( nID, ESSOT_GUNS, GetAttachedGunID( i, j ) ), platform.pAttachedPlatformVisObj->animdescs, 
						nStartTime, eAnimType, nAnimID );
				}
			}
		}
	}
}

void CMOUnitMechanical::AIUpdateDeadUnit( const SAIDeadUnitUpdate *pUpdate, const NDb::ESeason eSeason, const bool bIsNight, 
																				 ISoundScene *pSoundScene, struct IClientAckManager *pAckManager )
{
	if ( pJoggingMutator )
		pJoggingMutator->Stop();
	//
	if ( !IsVisible() )
	{
//		Scene()->ShowObject( GetID(), true );
		SetVisible( true, eSeason, bIsNight );
	}

	// Remove "broken track" icon, if any
	bTrackBroken = false;
	UpdateIcons();

	CMOUnit::AIUpdateDeadUnit( pUpdate, eSeason, bIsNight, pSoundScene, pAckManager );

	DetachSound( EAST_MOVEMENT );
	DetachSound( EAST_IDLE );
	SetCanSelect( false );
	// change model to 'destroyed'
	const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );
	if ( !pStats->damageLevels.empty() )
	{
		for ( int i = pStats->damageLevels.size() - 1; i >= 0; --i )
		{
			if ( pStats->damageLevels[i].fDamageHP <= 0 && pStats->damageLevels[i].pVisObj != 0 ) 
			{
				if ( const NDb::SModel *pNewModel = GetModel(pStats->damageLevels[i].pVisObj, eSeason) )
				{
					ChangeModelToDamaged( i, pNewModel, eSeason );
					break;
				}
			}
		}
	}
	if ( pUpdate->nUpdateTime != 0 )
	{
		PlayDieAnimation( pUpdate );

		// check for fatality or simply death play effect
		if ( IsVisible() && pUpdate->dieAnimation.nParam != -1 )
		{
			const NDb::EAnimationType eAnimType = NDb::EAnimationType( ( pUpdate->dieAnimation.nParam >> 16 ) & 0x00000fff );
			const int nStartTime = Min( pUpdate->dieAnimation.time, GameTimer()->GetGameTime() );

			if ( eAnimType == NDb::ANIMATION_DEATH_FATALITY && pStats->pEffectFatality ) 
			{
				if ( const NDb::SEffect *pEffect = pStats->pEffectFatality->GetSceneEffect() )
				{
					NAnimation::ISkeletonAnimator *pFatalityAnimator = Scene()->GetAnimator( GetID(), pStats->szFatalitySmokePoint );
					if ( !pFatalityAnimator )
					{
						// no locator, attach to center
						SFBTransform transform;
						if ( Scene()->GetVisObjPlacement(GetID(), &transform) )
							Scene()->AttachEffect( GetID(), ESSOT_DEATH_EFFECTS, new CCSFBTransform( transform ), pEffect, nStartTime, ESAT_NO_REPLACE, -1, true );
					}
					else
						Scene()->AttachEffect( GetID(), ESSOT_DEATH_EFFECTS, pStats->szFatalitySmokePoint, pEffect, nStartTime, ESAT_NO_REPLACE, true );
				}

				PlaySoundEffect( OBJECT_ID_FORGET, pStats->pEffectFatality->pSoundEffect, nStartTime, GetCenter() );
			}
			else if ( eAnimType == NDb::ANIMATION_DEATH && pStats->pEffectSmoke )
			{
				if ( const NDb::SEffect *pEffect = pStats->pEffectSmoke->GetSceneEffect() )
				{
					NAnimation::ISkeletonAnimator *pDeathAnimator = 0;

					string szLocator;
					if ( !pStats->damagePoints.empty() )
					{
						szLocator = pStats->damagePoints[NWin32Random::Random(pStats->damagePoints.size())];
						pDeathAnimator = Scene()->GetAnimator( GetID(), szLocator );
					}
					
					if ( !pDeathAnimator )
					{
						// no locator, attach to center
						SFBTransform transform;
						if ( Scene()->GetVisObjPlacement(GetID(), &transform) )
							Scene()->AttachEffect( GetID(), ESSOT_DEATH_EFFECTS, new CCSFBTransform( transform ), pEffect, nStartTime, ESAT_NO_REPLACE, -1, true );
					}
					else
						Scene()->AttachEffect( GetID(), ESSOT_DEATH_EFFECTS, szLocator, pEffect, nStartTime, ESAT_NO_REPLACE, true );
				}

				PlaySoundEffect( OBJECT_ID_FORGET, pStats->pEffectSmoke->pSoundEffect, nStartTime, GetCenter() );
			}
		}
	}

	if ( pStats->pdeathCraters != 0 && ( pUpdate->dieAnimation.nParam & 0x80000000 ) )
	{
		const CVec3 vCenter = GetCenter();
		PlaceCrater( pStats->pdeathCraters, eSeason, CVec2( vCenter.x, vCenter.y ) );
	}
}

void CMOUnitMechanical::AIUpdateDeadPlane( const SAIActionUpdate *pUpdate )
{
	const NTimer::STime timeEffect = Min( GameTimer()->GetGameTime(), pUpdate->nUpdateTime );
	const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );
	if ( pUpdate->nParam == -1 )		// started to dive, make burn effect
	{
		smokeTrails.clear();
		CVec3 vPos, vScale;
		CQuat qRot;
		GetPlacement( &vPos, &qRot, &vScale );
		AI2Vis( &vPos );
		NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID() );
		const SHMatrix mPlacement( vPos, qRot );
		for ( vector<NDb::SMechUnitRPGStats::SSmokeTrailEffect>::const_iterator it = pStats->smokeTrails.begin(); it != pStats->smokeTrails.end(); ++it )
		{
			SHMatrix mLocalPos;
			CalcRelativePos( &mLocalPos, mPlacement, it->szLocatorName, pAnimator );
			smokeTrails.push_back( new CSmokeTrailEffect( mLocalPos, it->fInterval, it->pEffect, vPos, qRot, timeEffect, IsVisible() ) );
		}
	}
	else if ( pUpdate->nParam == 0 ) // explode
	{
		smokeTrails.clear();
		if (  pStats->pEffectFatality ) 
			PlayComplexEffect( OBJECT_ID_FORGET, pStats->pEffectFatality, timeEffect, GetCenter() );
	}
}

void CMOUnitMechanical::AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, struct ISoundScene *pSoundScene, IClientAckManager *pAckManager )
{
	CMOUnit::AIUpdateDissapear( pUpdate, pSoundScene, pAckManager );
	
	DetachSound( EAST_MOVEMENT );
	DetachSound( EAST_IDLE );
	Scene()->RemoveAllAttached( GetID(), ESSOT_DEATH_EFFECTS );
	if ( pUpdate->bShowEffects && !GetStatsLocal()->IsAviation() )
	{
		CVec3 vPos, vScale;
		CQuat qRot;
		GetPlacement( &vPos, &qRot, &vScale );
		if ( GetStatsLocal()->pEffectDisappear != 0 )
			PlayComplexEffect( OBJECT_ID_FORGET, GetStatsLocal()->pEffectDisappear, pUpdate->nUpdateTime, vPos );		
	}
	if ( pTransport )
		pTransport->Load( this, false );
	SetTransport( 0 );
}

void CMOUnitMechanical::AIUpdateAction( const SAIActionUpdate *pUpdate, const NDb::ESeason eSeason )
{
	switch ( pUpdate->eUpdateType ) 
	{
	case ACTION_NOTIFY_INSTALL_TRANSPORT:
	case ACTION_NOTIFY_INSTALL_MOVE:
	case ACTION_NOTIFY_INSTALL_ROTATE:
		{
			if ( pUpdate->eUpdateType == ACTION_NOTIFY_INSTALL_TRANSPORT )
			{
				const NDb::SVisObj *pVisObj = GetStatsLocal()->pvisualObject; 
				if ( GetStatsLocal()->pAnimableModel != 0 )
					pVisObj = GetStatsLocal()->pAnimableModel;
				ChangeModelToAnimable( GetModel( pVisObj, eSeason ), eSeason );
			}
			const NDb::EAnimationType eType = pUpdate->eUpdateType == ACTION_NOTIFY_INSTALL_ROTATE ? 
				NDb::ANIMATION_INSTALL_ROT : 
				( pUpdate->eUpdateType == ACTION_NOTIFY_INSTALL_MOVE ? NDb::ANIMATION_INSTALL_PUSH : NDb::ANIMATION_INSTALL );
			const NDb::SModel *pModel = GetModelDesc();
			if ( eType < GetStatsLocal()->animdescs.size() && GetStatsLocal()->animdescs[eType].anims.size() > 0 ) 
			{
				const int nIndex = GetStatsLocal()->animdescs[eType].anims[0].nFrameIndex;
				if ( nIndex >= 0 && nIndex < pModel->pSkeleton->animations.size() ) 
				{
					if ( const NDb::SAnimB2 *pAnim = checked_cast<const NDb::SAnimB2*>( pModel->pSkeleton->animations[nIndex].GetPtr() ) ) 
					{
						if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID() ) )
						{
							if ( HasLoopedAnimation() )
							{
								pAnimator->ClearAllAnimations();
								SetLoopedAnimation( false );
							}
							//
							SetLoopedAnimation( pAnim->bLooped );
							const float fActionTime = eType == NDb::ANIMATION_INSTALL_ROT ? GetStatsLocal()->fUninstallRotate : GetStatsLocal()->fUninstallTransport;
							const float fAnimSpeed = GetStatsLocal()->animdescs[eType].anims[0].nLength * 0.001f / fActionTime;
							AddAnimation( pAnim, pUpdate->nUpdateTime, pAnimator, pAnim->bLooped, fAnimSpeed );
						}
					}
				}
			}
		}
		break;
	case ACTION_NOTIFY_FINISH_INSTALL_TRANSPORT:
	case ACTION_NOTIFY_FINISH_INSTALL_MOVE:
	case ACTION_NOTIFY_FINISH_INSTALL_ROTATE:
		ChangeModelToUsual( GetModel( GetStatsLocal()->pvisualObject, eSeason ), eSeason );
		break;
	case ACTION_NOTIFY_UNINSTALL_ROTATE:
	case ACTION_NOTIFY_UNINSTALL_MOVE:
	case ACTION_NOTIFY_UNINSTALL_TRANSPORT:
		if ( GetStatsLocal()->pAnimableModel != 0 )
		{
			ChangeModelToAnimable( GetModel( GetStatsLocal()->pAnimableModel, eSeason ), eSeason );
			const NDb::EAnimationType eType = pUpdate->eUpdateType == ACTION_NOTIFY_UNINSTALL_ROTATE ?
				NDb::ANIMATION_UNINSTALL_ROT : 
				( pUpdate->eUpdateType == ACTION_NOTIFY_UNINSTALL_MOVE ? NDb::ANIMATION_UNINSTALL_PUSH : NDb::ANIMATION_UNINSTALL );
			if ( eType < GetStatsLocal()->animdescs.size() && GetStatsLocal()->animdescs[eType].anims.size() > 0 ) 
			{
				const int nIndex = GetStatsLocal()->animdescs[eType].anims[0].nFrameIndex;
				if ( nIndex >= 0 && nIndex < GetModelDesc()->pSkeleton->animations.size() ) 
				{
					if ( const NDb::SAnimB2 *pAnim = checked_cast<const NDb::SAnimB2*>( GetModelDesc()->pSkeleton->animations[nIndex].GetPtr() ) ) 
					{
						if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID() ) )
						{
							if ( HasLoopedAnimation() )
							{
								pAnimator->ClearAllAnimations();
								SetLoopedAnimation( false );
							}
							//
							SetLoopedAnimation( pAnim->bLooped );
							const float fActionTime = eType == NDb::ANIMATION_UNINSTALL_ROT ? GetStatsLocal()->fUninstallRotate : GetStatsLocal()->fUninstallTransport;
							const float fAnimSpeed = GetStatsLocal()->animdescs[eType].anims[0].nLength * 0.001f / fActionTime;
							AddAnimation( pAnim, pUpdate->nUpdateTime, pAnimator, pAnim->bLooped, fAnimSpeed  );
						}
					}
				}
			}
		}
		break;
	case ACTION_NOTIFY_FINISH_UNINSTALL_ROTATE:
	case ACTION_NOTIFY_FINISH_UNINSTALL_MOVE:
		break;

	case ACTION_NOTIFY_FINISH_UNINSTALL_TRANSPORT:
		if ( GetStatsLocal()->pTransportableModel ) 
			ChangeModelToTransportable( GetModel( GetStatsLocal()->pTransportableModel, eSeason ), eSeason )	;

		break;

	case ACTION_NOTIFY_DELAYED_SHOOT:
		if ( const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() ) )
		{
			if ( pStats->animdescs.size() > NDb::ANIMATION_SHOOT && !pStats->animdescs[NDb::ANIMATION_SHOOT].anims.empty() )
				PlayAnimation( pStats->animdescs[NDb::ANIMATION_SHOOT].anims[0].nFrameIndex );
		}
		break;
	}
}

void CMOUnitMechanical::Select( bool bSelect )
{
	// show unit selections	
  if ( bSelect )
    Scene()->SelectObject( GetID(), GetCenter(), GetSelectionScale(), GetSelectionType() );
  else
    Scene()->UnselectObject( GetID() );

  CMOSelectable::Select( bSelect );
  UpdateIcons();
}

bool CMOUnitMechanical::NeedShowInterrior() const
{
	return !bArtilleryHooked;
}

CMOProjectile* CMOUnitMechanical::LaunchProjectile( const SAINewProjectileUpdate *pUpdate )
{
	const int nID = GetID();
	const NDb::SWeaponRPGStats *pWeapon = GetStatsLocal()->GetGun( nID, pUpdate->info.nPlatform, pUpdate->info.nGun ).pWeapon;
	NI_ASSERT( pWeapon != 0, StrFmt( "Can't find weapon for mechunit \"%s\", nCommonGun %d", GetStatsLocal()->GetDBID().ToString().c_str(), pUpdate->info.nGun ) );
	if ( pWeapon == 0 )
		return 0;

	NI_ASSERT( pUpdate->info.nShell < pWeapon->shells.size(), StrFmt( "Wrong number of shell (%d), total number of shells (%d)", pUpdate->info.nShell, pWeapon->shells.size() ) );
	if ( pUpdate->info.nShell >= pWeapon->shells.size() )
		return 0;

	// calculate shoot point transition and direction
	CQuat qShootPointRotation = QNULL;
	CVec3 vShootPointTransition;
	AI2Vis( &vShootPointTransition, GetCenter() );
	const NDb::SMechUnitRPGStats::SMechUnitGun &gun = GetStatsLocal()->GetGun( nID, pUpdate->info.nPlatform, pUpdate->info.nGun );
	if ( NAnimation::ISkeletonAnimator *pShootPointAnimator = Scene()->GetAnimator(nID, gun.szShootPoint) )
	{
		SHMatrix mShootPointPose;
		if ( !pShootPointAnimator->GetBonePosition( gun.szShootPoint.c_str(), &mShootPointPose ) )
			return 0;
		//
		vShootPointTransition = mShootPointPose.GetTrans3();
		qShootPointRotation.FromEulerMatrix( mShootPointPose );
		qShootPointRotation = qShootPointRotation * CQuat( FP_PI2, V3_AXIS_X );
	}

//	// calculate gun direction
//	const NDb::SMechUnitRPGStats::SPlatform &platform = GetStatsLocal()->GetPlatform( nID, pUpdate->info.nPlatform );
//	const string szPlatformBoneName = 
//		platform.pAttachedPlatformVisObj ? platform.szAttachedPlatformLocator : platform.szRotatePoint;
//	NAnimation::ISkeletonAnimator *pPlatformAnimator = Scene()->GetAnimator( nID, szPlatformBoneName );
//	if ( !pPlatformAnimator )
//		return 0;
//
//	SHMatrix mLocalPlatformPose;
//	if ( !pPlatformAnimator->GetBonePosition( szPlatformBoneName.c_str(), &mLocalPlatformPose ) )
//	{
//		NI_ASSERT( 0, StrFmt( "Platform rotate point not found for platform %d (bone's name \"%s\")", pUpdate->info.nPlatform, gun.szShootPoint.c_str() ) );
//		return 0;
//	}
//	CQuat qRot;
//	qRot.FromEulerMatrix( mLocalPlatformPose );

	CPtr<CMOProjectile> pProjectile = new CMOProjectile();
	if ( pProjectile->Create( pUpdate, pWeapon->shells[pUpdate->info.nShell].pvisProjectile, vShootPointTransition, qShootPointRotation, pWeapon->shells[pUpdate->info.nShell].pEffectTrajectory ) )
		return pProjectile.Extract();
	else
		return 0;
}

void CMOUnitMechanical::OnSerialize( IBinSaver &saver )
{
}

void CMOUnitMechanical::SendAcknowledgement( struct IClientAckManager *pAckManager, const NDb::EUnitAckType eAck )
{
	if ( IsValid( pOneFromCrew ) )
		return pOneFromCrew->SendAcknowledgement( pAckManager, eAck );
	else
		return CMOUnit::SendAcknowledgement( pAckManager, eAck );
}

REGISTER_SAVELOAD_CLASS( 0x100A7487, CMOUnitMechanical );

