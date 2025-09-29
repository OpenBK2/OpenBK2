#include "StdAfx.h"

#include "MOUnitInfantry.h"
#include "../Sound/SoundScene.h"
#include "../Stats_B2_M1/DBAnimB2.h"
#include "../3Dmotor/GAnimation.hpp"
#include "ParatrooperAnimation.h"
#include "../Main/GameTimer.h"
#include "../Misc/Win32Random.h"
#include "MOProjectile.h"

#include "../System/Commands.h"
#include "../System/Text.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::SetVisible( const bool bVisible, const NDb::ESeason eSeason, const bool bIsNight )
{
//	if ( GetTransport() != 0 && bVisible )
//		return;
	//
	CMOUnit::SetVisible( bVisible, eSeason, bIsNight );
	if ( nParachuteID != -1 )
		Scene()->ShowObject( nParachuteID, bVisible );
	if ( pSquad )
		pSquad->UpdateSquadIcons();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::InitAttached( const NDb::ESeason eSeason, IChooseAttached *pChooseFunc )
{
	const NDb::SInfantryRPGStats* pStats = GetStatsLocal();
	const int nUniqueID = GetID();
	IScene *pScene = Scene();

	for ( int j = 0; j < pStats->GetGunsSize( nUniqueID, 0 ); ++j )
	{
		const NDb::SBaseGunRPGStats &gun = pStats->GetGun( nUniqueID, 0, j );
		const NDb::SAttachedModelVisObj *pAttachedObj = gun.pAttachedGunVisObj;
		TryToAttach( pAttachedObj, pChooseFunc, eSeason, gun.szAttachedGunLocator, ESSOT_GUNS, j );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMOUnitInfantry::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	const bool bResult = CMOUnit::Create( nUniqueID, _pUpdate, eSeason, eDayTime, bInEditor );
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool s_bTestDrawWeapon = true;
bool CMOUnitInfantry::CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor )
{
	if ( CMOUnit::CreateSceneObject( nUniqueID, pUpdate, eSeason, bInEditor ) == false )
		return false;

	if ( s_bTestDrawWeapon )
		SetupWeapon( eSeason );

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::SetupWeapon( NDb::ESeason eSeason )
{
	const NDb::SInfantryRPGStats *pStats = checked_cast<const NDb::SInfantryRPGStats*>( GetStats() );
	if ( pStats && pStats->GetGunsSize( GetID(), 0 ) > 0 )
	{
		if ( CDBPtr<NDb::SWeaponRPGStats> pWeaponStats = pStats->GetGun( GetID(), 0, 0 ).pWeapon )
		{
			if ( CDBPtr<NDb::SVisObj> pVisObj = pWeaponStats->pVisObj )
			{
				CDBPtr<NDb::SModel> pModel = GetModel( pVisObj, eSeason );
				Scene()->AttachSubModel( GetID(), ESSOT_WEAPON, pStats->szGunBoneName, pModel, ESAT_REPLACE_ON_TYPE, 0, false, false );
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::ChangeRPGStats( const struct SAIChangeDBIDUpdate &update, const NDb::ESeason eSeason )
{
	const NDb::SVisObj *pCurVO = GetStats()->pvisualObject;

	const NDb::SInfantryRPGStats *pNewStats = checked_cast_ptr<const NDb::SInfantryRPGStats *>( update.info.pStats );
	const NDb::SVisObj *pNewVO = pNewStats->pvisualObject;
	const NDb::SInfantryRPGStats *pOldStats = checked_cast<const NDb::SInfantryRPGStats*>( GetStats() );

	if ( !pNewVO )
		return;

	const NDb::SModel *pNewModel = GetModel( pNewVO, eSeason );

	if ( pNewModel != GetModelDesc() )
	{
		ChangeModelToDamaged( 0, GetModel( pNewVO, eSeason ), eSeason );
		SetupWeapon( eSeason );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::GetStatus( SObjectStatus *pStatus ) const
{
	CMOUnit::GetStatus( pStatus );

	if ( !GetStatsLocal() )
		return;

	// Armor
	pStatus->armors[EOS_ARMOR_FRONT] = GetStatsLocal()->fArmor;
	pStatus->armors[EOS_ARMOR_SIDE] = GetStatsLocal()->fArmor;
	pStatus->armors[EOS_ARMOR_BACK] = GetStatsLocal()->fArmor;
	pStatus->armors[EOS_ARMOR_TOP] = GetStatsLocal()->fArmor;

	// Weapons
	for ( int i = 0; i < GetStatsLocal()->GetGunsSize( GetID(), 0 ); ++i )
	{
		const NDb::SBaseGunRPGStats &gun = GetStatsLocal()->GetGun( GetID(), 0, i );
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

		if ( weapon.pWeaponID && weapon.pWeaponID->eWeaponType != NDb::SWeaponRPGStats::WEAPON_HEAVY_CANNON &&
			weapon.pWeaponID->eWeaponType != NDb::SWeaponRPGStats::WEAPON_HIDED ) // exclude grenades
			pStatus->AddWeapon( weapon );
	}

	// Ammo total
	for ( int i = 0; i < pStatus->weapons.size(); ++i )
	{
		SObjectStatus::SWeapon &weapon = pStatus->weapons[i];

		weapon.nAmmo = GetWeaponAmmo( weapon.pWeaponID );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::SetSquad( interface IMOSquad *_pSquad )
{
	if ( pSquad )
		pSquad->Load( this, false );
	pSquad = _pSquad;
	if ( pSquad )
		pSquad->Load( this, true );
		
	if ( pSquad )
	{
		bool bNeedUpdate = (GetIconsSetInfo().fHPBarLen != pSquad->GetIconsSetInfo().fHPBarLen );
		SetIconsSetInfo( pSquad->GetIconsSetInfo() );
		if ( bNeedUpdate )
			UpdateIcons();
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::AIUpdateShot( const struct SAINotifyBaseShot &shot, const NTimer::STime &currTime, IScene *pScene, NDb::ESeason eSeason )
{
	const SAINotifyInfantryShot &infShot = *( static_cast<const SAINotifyInfantryShot*>(&shot) );
	const SAINotifyInfantryShot *pInfShot = &infShot;
	NI_ASSERT( pInfShot, "Wrong infantry shot notification" );
	if ( !pInfShot ) 
		return;

	const NDb::SComplexEffect *pEffectTemp = 0;
  
	// Идущий дальше фокус нужен нам в M1, потому что мы иначе не умеем передавать pWeapon
	// Фокус{

	const CDBPtr<NDb::SWeaponRPGStats> *ppWeapon = &pInfShot->pWeapon;

	if ( ! (*ppWeapon) )
	{
		ppWeapon = & ( GetStatsLocal()->GetGun( pInfShot->nObjUniqueID, pInfShot->nPlatform, pInfShot->nGun ).pWeapon );
	}

	const CDBPtr<NDb::SWeaponRPGStats> &pWeapon = *ppWeapon;
	
	// Фокус}

	pEffectTemp = pWeapon->shells[ pInfShot->cShell ].pEffectGunFire.GetPtr();

	const NDb::SComplexEffect *pEffect = pEffectTemp;
	if ( !pEffect ) 
		return;

	int nTime = currTime - shot.time;
	nTime = ( nTime > 0 ) ? nTime : 0;

	const NDb::SComplexSoundDesc *pSound = pEffect->pSoundEffect.GetPtr();
	if ( pSound ) 
	{
		SoundScene()->AddSound( pSound, GetCenter(), SFX_MIX_IF_TIME_EQUALS, SAM_ADD_N_FORGET, nTime, 2 );
	}

	const NDb::SEffect *pVisEffect = pEffect->GetSceneEffect();
	if ( pVisEffect && IsVisible() )
	{
		const NDb::SInfantryRPGStats *pInfStats = checked_cast<const NDb::SInfantryRPGStats*>( GetStats() );
		NAnimation::ISkeletonAnimator *pAnimator = pScene->GetAnimator( GetID(), pInfStats->szGunBoneName );
		if ( !pAnimator )
			return;

		const NDb::SInfantryRPGStats::SInfantryGun &gun = pInfStats->GetGun( GetID(), 0, 0 );
		CQuat qFireRot;
		if ( gun.bShootEffectInvert )
			qFireRot.FromAngleAxis( -FP_PI2, 1, 0, 0, true );
		else
			qFireRot.FromAngleAxis( FP_PI2, 1, 0, 0, true );
		SHMatrix mShootOffset( gun.vShootPointOffset, qFireRot );
		//
		//
		Scene()->AttachEffect( GetID(), ESSOT_GUN_FIRE, pInfStats->szGunBoneName, mShootOffset, pVisEffect, Min( shot.time, currTime ), ESAT_REPLACE_ON_BONE );
		//
	}
	AddShotTrace( pWeapon, shot, currTime, pScene );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::AddShotTrace( const CDBPtr<NDb::SWeaponRPGStats> pWeapon, const struct SAINotifyBaseShot &shot,
																	  const NTimer::STime &currTime, IScene *pScene )
{
	const SAINotifyInfantryShot &infShot = *( static_cast<const SAINotifyInfantryShot*>(&shot) );
	if ( (pWeapon->shells[infShot.cShell].etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE) /*&&
			 (NWin32Random::Random(100) + 1 <= pWeapon->shells[infShot.cShell].fTraceProbability * 100.0f)*/ )
	{
		const NDb::SInfantryRPGStats *pInfStats = checked_cast<const NDb::SInfantryRPGStats*>( GetStats() );
		NAnimation::ISkeletonAnimator *pAnimator = pScene->GetAnimator( GetID(), pInfStats->szGunBoneName );
		if ( !pAnimator )
			return;

		CVec3 vStart;
		pAnimator->GetBonePosition( pInfStats->szGunBoneName.c_str(), &vStart );
		CVec3 vEnd( shot.vDestPos );
		AI2Vis( &vEnd );
		if ( vEnd.z == 0 )
			vEnd.z = AI2Vis( Scene()->GetZ(shot.vDestPos.x, shot.vDestPos.y) );
		//
		pScene->AddShotTrace( vStart, vEnd, currTime, &(pWeapon->shells[infShot.cShell]) );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::GetActionsBy( CUserActions *pActions ) const
{
	if ( pActions->HasAction( NDb::USER_ACTION_RADIO_CONTROLLED_MODE ) )
	{
		pActions->SetAction( NDb::USER_ACTION_CONTROLLED_CHARGE );
		pActions->SetAction( NDb::USER_ACTION_DETONATE );
	}
	else
	{
		pActions->RemoveAction( NDb::USER_ACTION_CONTROLLED_CHARGE );
		pActions->RemoveAction( NDb::USER_ACTION_DETONATE );
	}

	if ( pSquad )
	{
		if ( const NDb::SSquadRPGStats *pStats = checked_cast<const NDb::SSquadRPGStats*>( pSquad->GetStats() ) )
		{
			if ( pStats->formations.size() > 1 )
			{
				pActions->SetAction( NDb::USER_ACTION_FORMATION );
				for ( int i = 0; i < pStats->formations.size(); ++i )
					pActions->SetAction( NDb::USER_ACTION_FORMATION_0 + pStats->formations[i].etype );
			}
			else
			{
				pActions->RemoveAction( NDb::USER_ACTION_FORMATION );
				pActions->RemoveAction( NDb::USER_ACTION_FORMATION_0 );
				pActions->RemoveAction( NDb::USER_ACTION_FORMATION_1 );
				pActions->RemoveAction( NDb::USER_ACTION_FORMATION_2 );
				pActions->RemoveAction( NDb::USER_ACTION_FORMATION_3 );
				pActions->RemoveAction( NDb::USER_ACTION_FORMATION_4 );
			}
		}
	}

	if ( pActions->HasAction( NDb::USER_ACTION_MOVE ) )
		pActions->SetAction( NDb::USER_ACTION_MOVE_HUMAN );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::GetActions( CUserActions *pActions, EActionsType eActions ) const
{
	CMOUnit::GetActions( pActions, eActions );
	if ( eActions == ACTIONS_BY || eActions == ACTIONS_ALL )
		GetActionsBy( pActions );

	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		pActions->SetAction( NDb::USER_ACTION_SPY_MODE );
		pActions->SetAction( NDb::USER_ACTION_FIRST_AID );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::GetPossibleActions( CUserActions *pActions ) const
{
	CMOUnit::GetPossibleActions( pActions );
	//
	GetActionsBy( pActions );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::AIUpdatePlacement( const struct SAINotifyPlacement &placement, interface IScene *pScene, interface ISoundScene *pSoundScene, NDb::ESeason eSeason )
{
	CMOUnit::AIUpdatePlacement( placement, pScene, pSoundScene, eSeason );
	if ( HasMoveAnimation() )
	{
		const float fSpeedCoeff = placement.fSpeed == 0 ? 1.0f : placement.fSpeed / GetAnimSpeed();
		NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID(), false );
		pAnimator->SetSpeedFactorForAllAnimations( Singleton<IGameTimer>()->GetGameTime(), fSpeedCoeff );
	}
	if ( nParachuteID != -1 )
	{
		CVec3 vParachutePos, vParachuteScale;
		CQuat qParachuteRot;
		GetPlacement( &vParachutePos, &qParachuteRot, &vParachuteScale );
		pScene->MoveObject( nParachuteID, vParachutePos, qParachuteRot, vParachuteScale );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::AIUpdateDeadUnit( const SAIDeadUnitUpdate *pUpdate, const NDb::ESeason eSeason, const bool bIsNight, interface ISoundScene *pSoundScene, IClientAckManager *pAckManager  )
{
	const bool bIsM1 = NGlobal::GetVar( "m1", 0 ) != 0;
	if ( !IsVisible() && !bIsM1 )
	{
//		Scene()->ShowObject( GetID(), true );
		SetVisible( true, eSeason, bIsNight );
	}
	
	CMOUnit::AIUpdateDeadUnit( pUpdate, eSeason, bIsNight, pSoundScene, pAckManager );

	SetSquad( 0 );
	SetCanSelect( false );

  NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID() );

	if ( pAnimator != 0 )
	{
		if ( !IsVisible() && bIsM1 )
		{
			pAnimator->ClearAllAnimations();
			return;
		}
		SetHasMoveAnimation( false );
		const int nStartTime = pUpdate->dieAnimation.time;
		pAnimator->SetSpeedFactorForAllAnimations( nStartTime, 1.0f );
		pAnimator->ClearAllAnimations();
		if ( pUpdate->dieAnimation.nParam != -1 )
		{
			const int nAnimID = pUpdate->dieAnimation.nParam & 0x0000ffff;
			const int nAnimType = ( pUpdate->dieAnimation.nParam & 0x0fff0000 ) >> 16;
			const NDb::SInfantryRPGStats *pStats = checked_cast<const NDb::SInfantryRPGStats*>( GetStats() );
			if ( (nAnimID >= 0) && (nAnimType < pStats->animdescs.size()) && (nAnimID < pStats->animdescs[nAnimType].anims.size()) )
			{
				if ( const NDb::SAnimB2 *pAnimation = checked_cast<const NDb::SAnimB2*>( pStats->animdescs[nAnimType].anims[nAnimID].pAnimation.GetPtr() ) ) 
				{
					SetLoopedAnimation( pAnimation->bLooped );
					AddAnimation( pAnimation, nStartTime, pAnimator );
				}
			}
		}
		//
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, interface ISoundScene *pSoundScene, IClientAckManager *pAckManager )
{
	CMOUnit::AIUpdateDissapear( pUpdate, pSoundScene, pAckManager );

	SetSquad( 0 );
	SetCanSelect( false );
	SetTransport( 0 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::AIUpdateAction( const SAIActionUpdate *pUpdate, const NDb::ESeason eSeason )
{
	/*
	switch ( pUpdate->eUpdateType ) 
	{
	case ACTION_NOTIFY_DELAYED_SHOOT:
		if ( const NDb::SUnitBaseRPGStats *pStats = checked_cast<const NDb::SUnitBaseRPGStats*>( GetStats() ) )
		{
			if ( pStats->animdescs.size() > NDb::ANIMATION_SHOOT && !pStats->animdescs[NDb::ANIMATION_SHOOT].anims.empty() )
				PlayAnimation( NDb::ANIMATION_SHOOT );
		}
		break;
	}
	*/
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CMOProjectile* CMOUnitInfantry::LaunchProjectile( const SAINewProjectileUpdate *pUpdate )
{
	const NDb::SWeaponRPGStats *pWeapon = GetStatsLocal()->GetGun( GetID(), pUpdate->info.nPlatform, pUpdate->info.nGun ).pWeapon;
	NI_ASSERT( pWeapon != 0, StrFmt( "Can't find weapon for mechunit \"%s\", nCommonGun %d", GetStatsLocal()->GetDBID().ToString().c_str(), pUpdate->info.nGun ) );
	if ( pWeapon == 0 )
		return 0;

	NI_ASSERT( pUpdate->info.nShell < pWeapon->shells.size(), StrFmt( "Wrong number of shell (%d), total number of shells (%d)", pUpdate->info.nShell, pWeapon->shells.size() ) );
	if ( pUpdate->info.nShell >= pWeapon->shells.size() )
		return 0;
	//
	CVec3 vShootPointTransition;
	AI2Vis( &vShootPointTransition, GetCenter() );
	vShootPointTransition += CVec3( 0, 0, 1 );
	//
	CPtr<CMOProjectile> pProjectile = new CMOProjectile();
	if ( pProjectile->Create( pUpdate, pWeapon->shells[pUpdate->info.nShell].pvisProjectile, vShootPointTransition, QNULL, pWeapon->shells[pUpdate->info.nShell].pEffectTrajectory ) )
		return pProjectile.Extract();
	else
		return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::FillIconsInfo( SSceneObjIconInfo &iconInfo )
{
	bool bServed = (pSquad != 0) && (pSquad->GetServedGun() != 0);
	bool bShowIcons = IsVisible() && !bServed && !pTransport && IsFirstUnit();
	if ( NGlobal::GetVar( "m1", 0 ) == 1 )
	{		
		SetIconsHitbar( IsVisible() && ( IsSelected() || IsMousePicked() ) && !pTransport, IsSelected() );
	}
	else
	{
//		SetIconsHitbar( IsVisible() && IsSelected(), IsSelected() );
//		bool bServed = (pSquad != 0) && (pSquad->GetServedGun() != 0);
//		SetIconsHitbar( IsVisible() && !bServed && !pTransport, IsSelected() );
		SetIconsHitbar( bShowIcons, CMOSelectable::IsSelected() );
	}	
//	SetIconsGroup( IsVisible() ? (IsSelected() ? GetSelectionGroup() : -1) : -1, IsSelected() );
	SetIconsGroup( bShowIcons ? GetSelectionGroup() : -1, CMOSelectable::IsSelected() );

	CMOUnit::FillIconsInfo( iconInfo );

	if ( pSquad && IsFirstUnit() )
		pSquad->FillIconsInfoForFirstUnit( iconInfo );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::Select( bool bSelect )
{
	// show unit selections	
  if ( bSelect )
    Scene()->SelectObject( GetID(), GetCenter(), GetSelectionScale(), GetSelectionType() );
  else
    Scene()->UnselectObject( GetID() );

  CMOSelectable::Select( bSelect );

  UpdateIcons();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::SetTransport( IMOContainer *_pTransport )
{
	bool bChanged = (pTransport != _pTransport);

	pTransport = _pTransport; 

	if ( bChanged )
		UpdateVisibility( true );
	if ( pSquad )
		pSquad->UpdateSquadIcons();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IClientUpdatableProcess *CMOUnitInfantry::AIUpdateStartFinishParadrop( const SParadropStartFinishUpdate *pUpdate, NDb::ESeason eSeason )
{
	return pUpdate->bStart ? StartParadrop( pUpdate, eSeason ) : FinishParadrop( pUpdate, eSeason );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IClientUpdatableProcess *CMOUnitInfantry::StartParadrop( const SParadropStartFinishUpdate *pUpdate, NDb::ESeason eSeason )
{
	const NDb::SVisObj *pUnitVisObj = pUpdate->pNewSoldierVisObj;
	const NDb::SVisObj *pParachuteVisObj = pUpdate->pParachuteVisObj;
	if ( pUnitVisObj == 0 || pParachuteVisObj == 0 )
		return 0;
	//
	const NDb::SModel *pUnitModel = GetModel( pUnitVisObj, eSeason );
	const NDb::SModel *pParachuteModel = GetModel( pParachuteVisObj, eSeason );
	if ( pUnitModel == 0 || pParachuteModel == 0 )
		return 0;
	// change model and create parachute model
	SetModel( pUnitModel );
	Scene()->ChangeModel( GetID(), pUnitModel );

	CVec3 vParachutePos, vParachuteScale;
	CQuat qParachuteRot;
	GetPlacement( &vParachutePos, &qParachuteRot, &vParachuteScale );
	nParachuteID = Scene()->AddObject( OBJECT_ID_GENERATE, pParachuteModel, 
		vParachutePos, qParachuteRot, vParachuteScale, 
		OBJ_ANIM_MODE_FORCE_ANIMATED, 0, false );
	//
	const NTimer::STime timeBegin = Min( pUpdate->nUpdateTime, GameTimer()->GetGameTime() );
	// change animation for main model
	const NDb::SAnimB2 *pUnitIdleAnim = 0;
	int nBeginAnimLength = -1;
	for ( int i = 0; i < pUnitModel->pSkeleton->animations.size(); ++i )
	{
		const NDb::SAnimB2 *pAnim = checked_cast_ptr<const NDb::SAnimB2 *>( pUnitModel->pSkeleton->animations[i] );
		switch ( pAnim->eType )
		{
		case NDb::ANIMATION_USE:
			AIUpdateAnimationChanged( pAnim, timeBegin );
			nBeginAnimLength = pAnim->nLength;
			break;
		case NDb::ANIMATION_IDLE:
			pUnitIdleAnim = pAnim;
			break;
		}
	}
	// change animation for parachute
	const NDb::SAnimB2 *pParachuteIdleAnim = 0;
	for ( int i = 0; i < pParachuteModel->pSkeleton->animations.size(); ++i )
	{
		const NDb::SAnimB2 *pAnim = checked_cast_ptr<const NDb::SAnimB2 *>( pParachuteModel->pSkeleton->animations[i] );
		switch ( pAnim->eType )
		{
		case NDb::ANIMATION_USE:
			if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nParachuteID ) )
			{
				pAnimator->ClearAllAnimations();
				AddAnimation( pAnim, timeBegin, pAnimator );
			}
			break;

		case NDb::ANIMATION_IDLE:
			pParachuteIdleAnim = pAnim;
			break;
		}
	}
	//
	Scene()->ShowObject( GetID(), IsVisible() );
	//
	if ( pUnitIdleAnim != 0 && nBeginAnimLength != -1 )
		return new CParatrooperAnimationProcess( this, timeBegin + nBeginAnimLength, pUnitIdleAnim, nParachuteID, pParachuteIdleAnim );
	//
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IClientUpdatableProcess *CMOUnitInfantry::FinishParadrop( const SParadropStartFinishUpdate *pUpdate, NDb::ESeason eSeason )
{
	const NDb::SVisObj *pUnitVisObj = pUpdate->pNewSoldierVisObj;
	const NDb::SVisObj *pParachuteVisObj = pUpdate->pParachuteVisObj;
	if ( pUnitVisObj == 0 || pParachuteVisObj == 0 )
		return 0;
	//
	const NDb::SModel *pUnitModel = GetModel( pUnitVisObj, eSeason );
	const NDb::SModel *pParachuteModel = GetModel( pParachuteVisObj, eSeason );
	if ( pUnitModel == 0 || pParachuteModel == 0 )
		return 0;
	//
	const NDb::SModel *pParaModel = GetModelDesc();
	SetModel( pUnitModel );
	const NTimer::STime timeBegin = Min( pUpdate->nUpdateTime, GameTimer()->GetGameTime() );
	// change animation for main model
	int nUnitIdleAnimID = -1;
	int nFinishAnimLength = -1;
	for ( int i = 0; i < pParaModel->pSkeleton->animations.size(); ++i )
	{
		const NDb::SAnimB2 *pAnim = checked_cast_ptr<const NDb::SAnimB2 *>( pParaModel->pSkeleton->animations[i] );
		if ( pAnim->eType == NDb::ANIMATION_USE_DOWN )
		{
			AIUpdateAnimationChanged( pAnim, timeBegin );
			nFinishAnimLength = pAnim->nLength;
			break;
		}
	}
	// change animation for parachute
	for ( int i = 0; i < pParachuteModel->pSkeleton->animations.size(); ++i )
	{
		const NDb::SAnimB2 *pAnim = checked_cast_ptr<const NDb::SAnimB2 *>( pParachuteModel->pSkeleton->animations[i] );
		if ( pAnim->eType == NDb::ANIMATION_USE_DOWN )
		{
			if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nParachuteID ) )
			{
				pAnimator->ClearAllAnimations();
				AddAnimation( pAnim, timeBegin, pAnimator );
			}
		}
	}
	//
	const NDb::SAnimB2 *pIdleAnim = 0;
	if ( const NDb::SUnitBaseRPGStats *pStats = dynamic_cast<const NDb::SUnitBaseRPGStats*>(GetStats()) )
	{
		if ( NDb::ANIMATION_IDLE < pStats->animdescs.size() && !pStats->animdescs[NDb::ANIMATION_IDLE].anims.empty() )
			pIdleAnim = checked_cast_ptr<const NDb::SAnimB2 *>( pStats->animdescs[NDb::ANIMATION_IDLE].anims[0].pAnimation );
	}
	//
	Scene()->ShowObject( GetID(), IsVisible() );
	//
	if ( nFinishAnimLength != -1 )
		return new CParachuteFinishProcess( this, timeBegin + nFinishAnimLength, pIdleAnim, eSeason );
	//
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::HideParachute()
{
	if ( nParachuteID )
	{
		Scene()->RemoveObject( nParachuteID );
		nParachuteID = -1;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CVec3 CMOUnitInfantry::GetFirePoint( const int nPlatform, const int nGun ) const
{
	const NDb::SInfantryRPGStats *pStats = checked_cast<const NDb::SInfantryRPGStats*>( GetStats() );
	NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID(), pStats->szGunBoneName );
	if ( !pAnimator )
		return VNULL3;
	SHMatrix mShootPoint;
	NI_VERIFY( pAnimator->GetBonePosition( pStats->szGunBoneName.c_str(), &mShootPoint ), StrFmt( "Shoot point not found for infantry gun (bone's name \"%s\")", pStats->szGunBoneName.c_str() ), return VNULL3 );
	return CVec3( mShootPoint._14, mShootPoint._24, mShootPoint._34 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMOUnitInfantry::IsFirstUnit() const
{
	if ( !pSquad )
		return true;
	return pSquad->IsFirstUnit( this );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMOUnitInfantry::IsVisible() const
{
	return CMOUnit::IsVisible() && pTransport == 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMOUnitInfantry::SetEntrench( bool bEntrench )
{
	bool bChanged = (bEntrenched != bEntrench);

	bEntrenched = bEntrench;

	UpdateVisibility( bChanged );
	if ( pSquad )
		pSquad->UpdateSquadIcons();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
START_REGISTER( MOUnitInfantry )
REGISTER_VAR_EX( "test_draw_weapon", NGlobal::VarBoolHandler, &s_bTestDrawWeapon, true, STORAGE_NONE );
FINISH_REGISTER
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x100A7486, CMOUnitInfantry );
