#include "stdafx.h"

#include "Stats_B2_M1/DBAnimB2.h"
#include "MOProjectile.h"
#include "MOUnitHelicopter.h"
#include "MOUnitMechanical.h"

#include "Input/Bind.h"
#include "Main/GameTimer.h"
#include "Misc/Win32Random.h"
#include "SceneB2/AttachedObj.h"
#include "Stats_B2_M1/M1UnitSpecific.h"
#include "Stats_B2_M1/IClientGameConsts.h"
typedef std::unordered_map< NDb::EDesignUnitType, SIconsSetInfo, SEnumHash > CIconsSet;
static bool bIsInitializedByDB = false;
CIconsSet iconsSets;
SIconsSetInfo iconsSetDefault;

REGISTER_SAVELOAD_CLASS( B2_M1_WORLD, 0x31197AC0, CMOUnitHelicopter );

static const int GetAttachedGunID( const int nPlatform, const int nGun )
{
	return (nPlatform << 6) | nGun;
}

static const SIconsSetInfo& GetDBIconsSet( NDb::EDesignUnitType eType )
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

bool CMOUnitHelicopter::Create( const int nUniqueID, const SAIBasicUpdate *pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	if ( !CMOUnit::Create( nUniqueID, pUpdate, eSeason, eDayTime, bInEditor ) )
		return false;

	pStats = dynamic_cast<const NDb::SMechUnitRPGStats *>( GetStats() );
	NI_VERIFY( pStats, "SMechUnitRPGStats not defined for helicopter", return false );
	pHelicopterStats = dynamic_cast<const NDb::SM1UnitHelicopter *>( pStats->pM1UnitSpecific.GetPtr() );
	NI_VERIFY( pHelicopterStats, "SM1UnitHelicopter not defined for helicopter", return false );

	if ( pStats->eSelectionType == NDb::SELECTION_TYPE_CANNOT_SELECT )
		SetCanSelect( false );

	vPropellers.resize( pHelicopterStats->axes.size(), SPropellerInfo() );
	fPropSpeed = -1.0f;
	SetPropellersSpeed( 0.0f, eSeason );

	if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID()) )
	{
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
			{
				pJoggingMutator->Setup( nBoneIndex, jx, jy );
				pAnimator->SetSpecialMutator( pJoggingMutator );
				pJoggingMutator->Play();
			}
		}

	}

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

	return true;
}

void CMOUnitHelicopter::SetPropellersSpeed( const float _fPropSpeed, NDb::ESeason eSeason )
{
	if ( fPropSpeed == _fPropSpeed )
		return;

	if ( fPropSpeed != 0.0f && _fPropSpeed == 0.0f )
		DetachSound( EAST_MOVEMENT );
	else if ( fPropSpeed != 1.0f && _fPropSpeed == 1.0f )
		AttachSound( EAST_MOVEMENT, pStats->pSoundMoveCycle, true );

	if ( _fPropSpeed == 1.0f )
	{
		for ( int i = 0; i < vPropellers.size(); ++i )
		{
			Scene()->RemoveAttached( GetID(), ESSOT_PROPELLERS, i );
			const NDb::SVisObj *pVisObj = pHelicopterStats->axes[i].pDynamic;
			const NDb::SModel *pModel = GetModel( pVisObj, eSeason );
			if ( pModel )
			{
				Scene()->AttachSubModel( GetID(), ESSOT_PROPELLERS, pHelicopterStats->axes[i].szLocatorName, pModel, ESAT_REPLACE_ON_TYPE, i, true, true );
				IAttachedObject* pAttached = Scene()->GetAttached( GetID(), ESSOT_PROPELLERS, i );
				if ( pAttached )
				{
					if ( IsVisible() )
						pAttached->ReCreate( Scene()->GetGView(), Scene()->GetGameTimer() );
					else
						pAttached->Clear( Singleton<IGameTimer>()->GetGameTime() );
					vPropellers[i].pAnimator = pAttached->GetAnimator();
					if ( vPropellers[i].pAnimator )
					{
						vPropellers[i].pAnimator->ClearAllAnimations();
						vPropellers[i].pScaleMutator = 0;
						for ( int n = 0; n < pModel->animations.size(); ++n )  
						{
							const NDb::SAnimB2* pAnimB2 = dynamic_cast_ptr<const NDb::SAnimB2*>( pModel->animations[n] );
							if ( pAnimB2 && pAnimB2->eType == NDb::ANIMATION_MOVE )
							{
								AddAnimation( pAnimB2, GameTimer()->GetGameTime(), vPropellers[i].pAnimator, true );
								break;
							}
						}
						if ( vPropellers[i].pAnimator )
							vPropellers[i].pAnimator->SetSpeedFactorForAllAnimations( Singleton<IGameTimer>()->GetGameTime(), 1.0f );
					}
				}
			}
		}
	}
	else if ( fPropSpeed == 1.0f || fPropSpeed == -1.0f )
	{
		for ( int i = 0; i < vPropellers.size(); ++i )
		{
			Scene()->RemoveAttached( GetID(), ESSOT_PROPELLERS, i );
			const NDb::SVisObj *pVisObj = pHelicopterStats->axes[i].pScaled;
			const NDb::SModel *pModel = GetModel( pVisObj, eSeason );
			if ( pModel )
			{
				Scene()->AttachSubModel( GetID(), ESSOT_PROPELLERS, pHelicopterStats->axes[i].szLocatorName, pModel, ESAT_REPLACE_ON_TYPE, i, true, true );
				IAttachedObject* pAttached = Scene()->GetAttached( GetID(), ESSOT_PROPELLERS, i );
				if ( pAttached )
				{
					if ( IsVisible() )
						pAttached->ReCreate( Scene()->GetGView(), Scene()->GetGameTimer() );
					else
						pAttached->Clear( Singleton<IGameTimer>()->GetGameTime() );
					vPropellers[i].pAnimator = pAttached->GetAnimator();
					if ( vPropellers[i].pAnimator )
					{
						vPropellers[i].pAnimator->ClearAllAnimations();
						vPropellers[i].pScaleMutator = MakeObject<IWingScaleMutator>( IWingScaleMutator::typeID );
						if ( vPropellers[i].pScaleMutator->Setup( vPropellers[i].pAnimator, StrFmt( "wingScaled%02d_", i+1 ), StrFmt( "wingsStatic%02d", i+1 ) ) )
						{
							vPropellers[i].pScaleMutator->ShowStatic( _fPropSpeed < pHelicopterStats->axes[i].fHideStaticSpeed );
							vPropellers[i].pScaleMutator->SetScale( _fPropSpeed < pHelicopterStats->axes[i].fStartScaleSpeed ? 0.0f : ( _fPropSpeed - pHelicopterStats->axes[i].fStartScaleSpeed )/( 1.0f - pHelicopterStats->axes[i].fStartScaleSpeed ) );
						}
						else
							vPropellers[i].pScaleMutator = 0;
						for ( int n = 0; n < pModel->animations.size(); ++n )  
						{
							const NDb::SAnimB2* pAnimB2 = dynamic_cast_ptr<const NDb::SAnimB2*>( pModel->animations[n] );
							if ( pAnimB2 && pAnimB2->eType == NDb::ANIMATION_MOVE )
							{
								AddAnimation( pAnimB2, GameTimer()->GetGameTime(), vPropellers[i].pAnimator, true );
								break;
							}
						}
						if ( vPropellers[i].pAnimator )
							vPropellers[i].pAnimator->SetSpeedFactorForAllAnimations( Singleton<IGameTimer>()->GetGameTime(), _fPropSpeed*30.0f );
					}
				}
			}
		}
	}
	else
	{
		for ( int i = 0; i < vPropellers.size(); ++i )
		{
			if ( vPropellers[i].pScaleMutator )
			{
				vPropellers[i].pScaleMutator->ShowStatic( _fPropSpeed < pHelicopterStats->axes[i].fHideStaticSpeed );
				vPropellers[i].pScaleMutator->SetScale( _fPropSpeed < pHelicopterStats->axes[i].fStartScaleSpeed ? 0.0f : ( _fPropSpeed - pHelicopterStats->axes[i].fStartScaleSpeed )/( 1.0f - pHelicopterStats->axes[i].fStartScaleSpeed ) );
			}
			if ( vPropellers[i].pAnimator )
				vPropellers[i].pAnimator->SetSpeedFactorForAllAnimations( Singleton<IGameTimer>()->GetGameTime(), _fPropSpeed*30.0f );
		}
	}
	fPropSpeed = _fPropSpeed;
}

IClientUpdatableProcess* CMOUnitHelicopter::AIUpdateMovement( const NTimer::STime &time, const bool _bMove, IScene *pScene, ISoundScene *pSoundScene )
{
/*
	if ( bMove != _bMove )
	{
		if ( bMove )
		{
			if ( pJoggingMutator )
				pJoggingMutator->Play();
		}
		else
		{
			if ( pJoggingMutator )
				pJoggingMutator->Stop();
		}
		bMove = _bMove;

	}
*/
	return CMOUnit::AIUpdateMovement( time, _bMove, pScene, pSoundScene );
}

void CMOUnitHelicopter::AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, ISoundScene *pSoundScene, NDb::ESeason eSeason )
{
	CMOUnit::AIUpdatePlacement( placement, pScene, pSoundScene, eSeason );
	SetPropellersSpeed( placement.fSpeed, eSeason );
	if ( !smokeTrails.empty() )
	{
		CVec3 vPos, vScale;
		CQuat qRot;
		GetPlacement( &vPos, &qRot, &vScale );
		AI2Vis( &vPos );
		const NTimer::STime currTime = GameTimer()->GetGameTime();
		for ( std::vector< CObj<CSmokeTrailEffect> >::iterator it = smokeTrails.begin(); it != smokeTrails.end(); ++it )
			(*it)->UpdatePlacement( vPos, qRot, currTime, IsVisible() );
	}
}

IClientUpdatableProcess* CMOUnitHelicopter::AIUpdateRPGStats( const SAINotifyRPGStats &stats, struct IClientAckManager *pAckManager, NDb::ESeason eSeason )
{
	const float fNewHP = stats.fHitPoints / GetStatsLocal()->fMaxHP;
	if ( fNewHP > 0.0f )
		return CMOUnit::AIUpdateRPGStats( stats, pAckManager, eSeason );
	else
	{
		SAINotifyRPGStats notify( stats );
		notify.fHitPoints = 0.01f*GetStatsLocal()->fMaxHP;
		return CMOUnit::AIUpdateRPGStats( notify, pAckManager, eSeason );
	}
}

void CMOUnitHelicopter::AIUpdateDeadPlane( const SAIActionUpdate *pUpdate, NDb::ESeason eSeason )
{
	DetachSound( EAST_MOVEMENT );
	const NTimer::STime timeEffect = (std::min)( GameTimer()->GetGameTime(), pUpdate->nUpdateTime );
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
		for ( std::vector<NDb::SMechUnitRPGStats::SSmokeTrailEffect>::const_iterator it = pStats->smokeTrails.begin(); it != pStats->smokeTrails.end(); ++it )
		{
			SHMatrix mLocalPos;
			CalcRelativePos( &mLocalPos, mPlacement, it->szLocatorName, pAnimator );
			smokeTrails.push_back( new CSmokeTrailEffect( mLocalPos, it->fInterval, it->pEffect, vPos, qRot, timeEffect, IsVisible() ) );
		}
	}
	else if ( pUpdate->nParam == 0 ) // explode
	{
		smokeTrails.clear();
		SAINotifyRPGStats notify;
		notify.fHitPoints = 0.0f;
		CommonUpdateHP( 0.0f, notify, Scene(), eSeason );
		if (  pStats->pEffectFatality ) 
			PlayComplexEffect( OBJECT_ID_FORGET, pStats->pEffectFatality, timeEffect, GetCenter() );
	
		const NDb::SAnimB2* pAnimB2 = 0; 
		const NDb::SModel *pModel = GetModelDesc();
		if ( pModel && pModel->pSkeleton )
		{
			for ( int i = 0; i < pModel->pSkeleton->animations.size(); ++i )
			{
				pAnimB2 = dynamic_cast_ptr<const NDb::SAnimB2*>( pModel->pSkeleton->animations[i] );
				if ( pAnimB2->eType == NDb::ANIMATION_DEATH )
					break;
			}
		}
		NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID() );
		if ( pAnimB2 && pAnimator && pAnimB2->eType == NDb::ANIMATION_DEATH )
		{
			pAnimator->ClearAllAnimations();
			SetLoopedAnimation( pAnimB2->bLooped );
			AddAnimation( pAnimB2, timeEffect, pAnimator );
		}
	}
}

bool CMOUnitHelicopter::IsInside( const int nID )
{
	for ( std::vector< CPtr<CMOSelectable> >::const_iterator it = vPassangers.begin(); it != vPassangers.end(); ++it )
	{
		if ( (*it)->GetID() == nID )
			return true;
	}
	return false;
}

/*void CMOUnitHelicopter::UpdateIcons()
{
	SetIconsHitbar( IsVisible(), IsSelected() );
	SetIconsGroup( IsVisible() ? GetSelectionGroup() : -1, IsSelected() );

	CMOUnit::UpdateIcons();
}*/

void CMOUnitHelicopter::Select( bool bSelect )
{
	//#define SHOW_UNIT_SELECTION
#ifdef SHOW_UNIT_SELECTION
	if ( bSelect )
		Scene()->SelectObject( GetID(), GetCenter() + CVec3( 50.0f, 50.0f, 10.0f ), GetSelectionScale(), GetSelectionType() );
	else
		Scene()->UnselectObject( GetID() );
#endif
	//
	CMOSelectable::Select( bSelect );
	UpdateIcons();
}

bool CMOUnitHelicopter::Load( struct IMOUnit *pMO, bool bEnter )
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
	return true;
}

bool CMOUnitHelicopter::LoadSquad( struct IMOSquad *pSquad, bool bEnter )
{
	if ( pSquad == 0 )
		return true;
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
	return true;
}

void CMOUnitHelicopter::GetPassangers( std::vector<CMOSelectable*> *pBuffer ) const
{
	NI_VERIFY( pBuffer, "Wrong pointer", return );
	pBuffer->resize( vPassangers.size() );
	for ( int i = 0; i < vPassangers.size(); ++i )
		(*pBuffer)[i] = vPassangers[i];
}

int CMOUnitHelicopter::GetFreePlaces() const
{
	return pStats->nPassangers-(vPassangers.size() - GetMechPassangersCount());
}

int CMOUnitHelicopter::GetFreeMechPlaces() const
{
	return pStats->boardedMechUnitPosition.size() - GetMechPassangersCount();
}

const int CMOUnitHelicopter::GetMechPassangersCount() const
{
	int nCount = 0;
	for ( int i = 0; i < vPassangers.size(); ++i )
	{
		if ( dynamic_cast<CMOUnitMechanical*>( vPassangers[i].GetPtr() ) )
			++nCount;
	}
	return nCount;
}

void CMOUnitHelicopter::InitAttached( const NDb::ESeason eSeason, IChooseAttached *pChooseFunc )
{
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

void CMOUnitHelicopter::AIUpdateShot( const SAINotifyBaseShot &_shot, const NTimer::STime &currTime, IScene *pScene, NDb::ESeason eSeason )
{																																								
	const SAINotifyMechShot &shot = *( static_cast<const SAINotifyMechShot*>(&_shot) );

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
			std::vector<NAnimation::ISkeletonAnimator::SDesiredBoneMove> mutator;
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
			std::vector<NAnimation::ISkeletonAnimator::SDesiredBoneMove> mutatorBasis;
			mutatorBasis.push_back( NAnimation::ISkeletonAnimator::SDesiredBoneMove( (float)(gun.nRecoilShakeTime)*0.1, qRecoil ) );
			mutatorBasis.push_back( NAnimation::ISkeletonAnimator::SDesiredBoneMove( (float)(gun.nRecoilShakeTime)*0.9, QNULL ) );
			pAnimator->SetBoneMutator( 0, currTime, mutatorBasis ); // 0 as const char *

			if ( pComplexEffect != 0 && IsVisible() )
				PlayComplexEffect( OBJECT_ID_FORGET, pComplexEffect, _shot.time, mShootPoint );
		}
		else if ( pComplexEffect != 0 && IsVisible() )
			PlayComplexEffect( nID, gun.szShootPoint, ESSOT_GUN_FIRE, pComplexEffect, _shot.time, ESAT_REPLACE_ON_BONE );
		// play effect shoot dust
		if ( IsVisible() && pStats->pEffectShootDust != 0 )
			PlayComplexSeasonedEffect( OBJECT_ID_FORGET, pStats->pEffectShootDust, _shot.time, GetCenter(), GetOrientation(), eSeason );
		// Shot Trace
		if ( gun.pWeapon->shells[shot.cShell].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE &&  NWin32Random::Random( 100 ) + 1 <= gun.pWeapon->shells[shot.cShell].fTraceProbability * 100.0f )
		{
			CVec3 vStart( mShootPoint._14, mShootPoint._24, mShootPoint._34 );
			CVec3 vEnd( _shot.vDestPos );
			AI2Vis( &vEnd );
			if ( vEnd.z == 0 )
				vEnd.z = AI2Vis( Scene()->GetZ(shot.vDestPos.x, shot.vDestPos.y) );
			//
			pScene->AddShotTrace( vStart, vEnd, currTime, &(gun.pWeapon->shells[shot.cShell]) );
		}
	}
}

CMOProjectile* CMOUnitHelicopter::LaunchProjectile( const SAINewProjectileUpdate *pUpdate )
{
	const int nID = GetID();
	const NDb::SWeaponRPGStats *pWeapon = pStats->GetGun( nID, pUpdate->info.nPlatform, pUpdate->info.nGun ).pWeapon;
	NI_ASSERT( pWeapon != 0, StrFmt( "Can't find weapon for mechunit \"%s\", nCommonGun %d", pStats->GetDBID().ToString().c_str(), pUpdate->info.nGun ) );
	if ( pWeapon == 0 )
		return 0;

	NI_ASSERT( pUpdate->info.nShell < pWeapon->shells.size(), StrFmt( "Wrong number of shell (%d), total number of shells (%d)", pUpdate->info.nShell, pWeapon->shells.size() ) );
	if ( pUpdate->info.nShell >= pWeapon->shells.size() )
		return 0;

	// calculate shoot point transition
	CVec3 vShootPointTransition;
	AI2Vis( &vShootPointTransition, GetCenter() );
	const NDb::SMechUnitRPGStats::SMechUnitGun &gun = pStats->GetGun( nID, pUpdate->info.nPlatform, pUpdate->info.nGun );
	if ( NAnimation::ISkeletonAnimator *pShootPointAnimator = Scene()->GetAnimator(nID, gun.szShootPoint) )
	{
		if ( !pShootPointAnimator->GetBonePosition( gun.szShootPoint.c_str(), &vShootPointTransition ) )
			return 0;
	}

	// calculate gun direction
	const NDb::SMechUnitRPGStats::SPlatform &platform = pStats->GetPlatform( nID, pUpdate->info.nPlatform );
	const std::string szPlatformBoneName =
		platform.pAttachedPlatformVisObj ? platform.szAttachedPlatformLocator : platform.szRotatePoint;
	NAnimation::ISkeletonAnimator *pPlatformAnimator = Scene()->GetAnimator( nID, szPlatformBoneName );
	if ( !pPlatformAnimator )
		return 0;

	SHMatrix mLocalPlatformPose;
	if ( !pPlatformAnimator->GetBonePosition( szPlatformBoneName.c_str(), &mLocalPlatformPose ) )
	{
		NI_ASSERT( 0, StrFmt( "Platform rotate point not found for platform %d (bone's name \"%s\")", pUpdate->info.nPlatform, gun.szShootPoint.c_str() ) );
		return 0;
	}
	CQuat qRot;
	qRot.FromEulerMatrix( mLocalPlatformPose );

	CPtr<CMOProjectile> pProjectile = new CMOProjectile();
	if ( pProjectile->Create( pUpdate, pWeapon->shells[pUpdate->info.nShell].pvisProjectile, vShootPointTransition, qRot, pWeapon->shells[pUpdate->info.nShell].pEffectTrajectory ) )
		return pProjectile.Extract();
	else
		return 0;
}

void CMOUnitHelicopter::AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, struct ISoundScene *pSoundScene, IClientAckManager *pAckManager )
{
	DetachSound( EAST_MOVEMENT );
	CMOUnit::AIUpdateDissapear( pUpdate, pSoundScene, pAckManager );
}


