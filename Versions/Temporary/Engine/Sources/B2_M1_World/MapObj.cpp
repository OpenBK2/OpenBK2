#include "StdAfx.h"

#include "Stats_B2_M1/DBAnimB2.h"
#include "MapObj.h"
#include "3Dmotor/GAnimation.hpp"
#include "Main/GameTimer.h"
#include "Misc/Win32Random.h"
#include "Sound/SoundScene.h"
#include "Stats_B2_M1/IClientGameConsts.h"
#include "System/Commands.h"
#include "System/Text.h"
#include "Stats_B2_M1/StatusUpdates.h"

const float SOLID_ICON_ALPHA = 1.0f;
float FADED_ICON_ALPHA = 0.25f;
const float MOUSE_PICKED_ALPHA_BONUS = 0.6f;
const CVec3 HIDE_RADIUS_COLOR = CVec3( 0.0f, 1.0f, 0.0f );
const float HIDE_RADIUS_WIDTH = 1.0f;

bool AddAnimation( const NDb::SAnimB2 *pAnim, const NTimer::STime timeStart, NAnimation::ISkeletonAnimator *pAnimator, bool bLooped, float fSpeed )
{
	return pAnimator->AddAnimation( timeStart, NAnimation::SAnimHandle(pAnim, 0), bLooped, fSpeed ) != -1;
}
bool AddAnimation( const NDb::SAnimB2 *pAnim, const NTimer::STime timeStart, NAnimation::ISkeletonAnimator *pAnimator )
{
	return AddAnimation( pAnim, timeStart, pAnimator, pAnim->bLooped, 1.0f );
}

// SObjectStatus

SObjectStatus::SObjectStatus()
{
	armors.resize( EOS_ARMOR_COUNT );
	for ( int i = 0; i < armors.size(); ++i )
	{
		armors[i] = 0;
	}
}

void SObjectStatus::Clear()
{
	nHP = 0;
	nMaxHP = 0;

	nSupply = 0;

	for ( int i = 0; i < armors.size(); ++i )
	{
		armors[i] = 0;
	}
	pArmorPattern = 0;
	
	bIsTransport = false;
	fFuel = -1.0f;
}

void SObjectStatus::AddWeapon( const SWeapon &weapon )
{
	vector<SWeapon>::iterator it = find( weapons.begin(), weapons.end(), weapon );
	if ( it != weapons.end() )
	{
		SWeapon &current = *it;
		++current.nCount;
		current.nAmmo += weapon.nAmmo;
		current.nMaxAmmo += weapon.nMaxAmmo;
	}
	else
	{
		if ( weapon.bPrimary )
			weapons.insert( weapons.begin(), weapon );
		else
			weapons.push_back( weapon );
	}
}

CMapObj::CMapObj() : vScale( 1, 1, 1 ), fHP( 1 ), eDiplomacy( EDI_NEUTRAL ), bLoopedAnimation( false ), 
	bVisible( false ), bIsSilentlyDead( false ), bHasMoveAnimation( false ), fAnimationSpeed( 1.0f ), 
	attachedSounds( __EAST_COUNTER__ ), nKeyObjectPlayer( -1 ), nParentID( -1 ), nPlayer( -1 ),
	nColorIndex( 0 )
{
	for ( int i = 0; i < __EAST_COUNTER__; ++i )
		attachedSounds[i] = 0;
}

CMapObj::~CMapObj()
{
	for ( int i = 0; i < attachedSounds.size(); ++i )
		DetachSound( (EAttachedSoundType)i );

	Scene()->RemoveAllAttached( GetID(), ESSOT_LIGHT );
}

void CMapObj::AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, struct ISoundScene *pSoundScene, IClientAckManager *pAckManager )
{
	if ( pUpdate->bShowEffects )
	{
		CVec3 vPos, vScale;
		CQuat qRot;
		GetPlacement( &vPos, &qRot, &vScale );
		if ( const NDb::SStaticObjectRPGStats* pStaticObjectStats = dynamic_cast<const NDb::SStaticObjectRPGStats*>( GetStats() ) )
		{
			if ( GetStats()->eGameType == NDb::SGVOGT_FENCE )
			{
				// Fences play death effect when disappearing
				CDynamicCast<const NDb::SObjectBaseRPGStats> pObjectStats = pStaticObjectStats;
				if ( pObjectStats && pObjectStats->pSeasonedEffectExplosion )
					PlayComplexSeasonedEffect( OBJECT_ID_FORGET, pObjectStats->pSeasonedEffectExplosion, pUpdate->nUpdateTime, vPos, eSeason );
				else if ( pStaticObjectStats->pEffectDeath != 0 )
					PlayComplexEffect( OBJECT_ID_FORGET, pStaticObjectStats->pEffectDeath, pUpdate->nUpdateTime, vPos );
				else if ( pStaticObjectStats->pEffectDisappear != 0 )
					PlayComplexEffect( OBJECT_ID_FORGET, pStaticObjectStats->pEffectDisappear, pUpdate->nUpdateTime, vPos );
			}
			else
				if ( pStaticObjectStats->pEffectDisappear != 0 )
					PlayComplexEffect( OBJECT_ID_FORGET, pStaticObjectStats->pEffectDisappear, pUpdate->nUpdateTime, vPos );
		}
	}
}

void CMapObj::AttachSound( EAttachedSoundType eType, const NDb::SComplexSoundDesc *pSound, bool bLooped )
{
	if ( attachedSounds[eType] != 0 )
		DetachSound( eType );
	attachedSounds[eType] = SoundScene()->AddSound( pSound, GetCenter(), SFX_MIX_ALWAYS, bLooped ? SAM_LOOPED_NEED_ID : SAM_NEED_ID, 0, 2 );
}

void CMapObj::DetachSound( EAttachedSoundType eType )
{
	if ( attachedSounds[eType] != 0 ) 
	{
		SoundScene()->RemoveSound( attachedSounds[eType] );
		attachedSounds[eType] = 0;
	}
}

bool CMapObj::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	const SAINewUnitUpdate *pUpdate = checked_cast<const SAINewUnitUpdate *>( _pUpdate );
	pStats = pUpdate->info.pStats;
	nID = nUniqueID;
	nPlayer = pUpdate->info.nPlayer;

	const bool bCreated = CreateSceneObject( nUniqueID, pUpdate, eSeason, bInEditor );
	
	fHP = pUpdate->info.fHitPoints / pStats->fMaxHP;
	NI_ASSERT( fHP <= 1.0f, StrFmt( "From AILogic: (Create) current unit's hp exceed maximum hp (%s)", 
		pStats ? pStats->GetDBID().ToString().c_str() : "" ) );
	eDiplomacy = pUpdate->info.eDipl;

	return bCreated;
}

void CMapObj::AfterLoad()
{
	UpdateIcons();
}

void CMapObj::SwitchLightFX( const bool bNewState, const NDb::ESeason eSeason, const bool bIsNight, const bool bInEditor )
{
	if ( GetStats() )
	{
		if ( bNewState && fHP > 0.25f )			// Switch on if HP > 1/4
		{
			const NTimer::STime currTime = GameTimer()->GetGameTime();
			for ( int i = 0; i < GetStats()->lightEffects.size(); ++i )
			{		
				if ( ( GetStats()->lightEffects[ i ].bOnAtNight && bIsNight ) ||
					( GetStats()->lightEffects[ i ].bOnAtDay && !bIsNight ) )
					Scene()->AttachLightEffect( GetID(), &(GetStats()->lightEffects[ i ]), currTime, ESAT_NO_REPLACE, bInEditor );
			}
		}
		else									// Switch off
		{
			Scene()->RemoveAllAttached( GetID(), ESSOT_LIGHT );
		}
	}
}

void CMapObj::ForceSwitchLightFX( const bool bNewState, const bool bInEditor )
{
	Scene()->RemoveAllAttached( GetID(), ESSOT_LIGHT );

	if ( bNewState )
	{
		const NTimer::STime currTime = GameTimer()->GetGameTime();
		for ( int i = 0; i < GetStats()->lightEffects.size(); ++i )
			Scene()->AttachLightEffect( GetID(), &(GetStats()->lightEffects[ i ]), currTime, ESAT_NO_REPLACE, bInEditor );
	}
}

void CMapObj::ForceSwitchLightFX( int nEffect, const bool bNewState, const bool bInEditor )
{
	if ( bNewState )
	{
		const NTimer::STime currTime = GameTimer()->GetGameTime();
		Scene()->RemoveAttached( GetID(), ESSOT_LIGHT, nEffect );
		Scene()->AttachLightEffect( GetID(), &(GetStats()->lightEffects[ nEffect ]), currTime, ESAT_NO_REPLACE, bInEditor, nEffect );
	}
	else
		Scene()->RemoveAttached( GetID(), ESSOT_LIGHT, nEffect );
}

void CMapObj::SetDeathState()
{
	if ( fHP > 0.0f )
		fHP = 0.0f;
}

const NDb::SVisObj* CMapObj::ChooseVisObjForHP( float fHP )
{
	const NDb::SVisObj *pResult = pStats->pvisualObject;
	float fResultHP = 1.0f;
	for ( int i = 0; i < pStats->damageLevels.size(); ++i )
	{
		const NDb::SHPObjectRPGStats::SDamageLevel &level = pStats->damageLevels[i];
		if ( level.fDamageHP >= fHP && fResultHP > level.fDamageHP )
		{
			fResultHP = level.fDamageHP;
			pResult = level.pVisObj;
		}
	}
	return pResult;
}

void CMapObj::ChangeModelToAnimable( const NDb::SModel *pNewModel, const NDb::ESeason eSeason )
{
	pModel = pNewModel; 
	Scene()->ChangeModel( nID, pModel );
}

void CMapObj::ChangeModelToTransportable( const NDb::SModel *pNewModel, const NDb::ESeason eSeason )
{
	pModel = pNewModel; 
	Scene()->ChangeModel( nID, pModel );
}

void CMapObj::ChangeModelToUsual( const NDb::SModel *pNewModel, const NDb::ESeason eSeason )
{
	pModel = pNewModel; 
	Scene()->ChangeModel( nID, pModel );
}

void CMapObj::ChangeModelToDamaged( const int nDamaged, const NDb::SModel *pNewModel, const NDb::ESeason eSeason )
{
	pModel = pNewModel; 
	Scene()->ChangeModel( nID, pModel );
}

int CMapObj::CommonUpdateHP( const float fNewHP, const SAINotifyRPGStats &stats, IScene * pScene, NDb::ESeason eSeason )
{
	NI_ASSERT( fNewHP <= 1.0f, StrFmt( "From AILogic: (CommonUpdateHP) current unit's hp exceed maximum hp (%s)", 
		pStats ? pStats->GetDBID().ToString().c_str() : "" ) );
	int nResult = -1;
	if ( fNewHP != fHP )
	{
		if ( !pStats->damageLevels.empty() )
		{
			float fCurHPVO = 1.0f;
			float fNewHPVO = 1.0f;
			const NDb::SVisObj *pCurVO = pStats->pvisualObject;
			const NDb::SVisObj *pNewVO = pStats->pvisualObject;
			for ( int i = 0; i < pStats->damageLevels.size(); ++i )
			{
				const float fDamageHP = pStats->damageLevels[i].fDamageHP;
				const NDb::SVisObj *pDamageVO = pStats->damageLevels[i].pVisObj;
				if ( fHP <= fDamageHP && fCurHPVO > fDamageHP )
				{
					fCurHPVO = fDamageHP;
					pCurVO = pDamageVO;
				}	
				if ( fNewHP <= fDamageHP && fNewHPVO > fDamageHP )
				{
					fNewHPVO = fDamageHP;
					pNewVO = pDamageVO;
					nResult = i;
				}	
			}
			if ( pCurVO != pNewVO )
			{
				Scene()->RemoveAllAttached( nID, ESSOT_WINDOW );
				Scene()->RemoveAllAttached( nID, ESSOT_LIGHT );

				if ( pNewVO != 0 )
				{
					if ( pNewVO == pStats->pvisualObject )
					{
						// object is repeared to original state, so we need to restore all effects that it had
						ChangeModelToUsual( GetModel( pNewVO, eSeason ), eSeason );
						UpdateVisibility( true );
						RunDefaultObjectAnimation( GetModelDesc()->pSkeleton, Scene()->GetAnimator(GetID()) );
					}
					else
						ChangeModelToDamaged( nResult, GetModel( pNewVO, eSeason ), eSeason );
				}
			}
			if ( fCurHPVO == fNewHPVO )
				nResult = -1;
				
		}
		fHP = fNewHP;
		if ( fHP <= 0.0f )			// Dead, remove attached objects
		{
			Scene()->RemoveAllAttached( nID, ESSOT_WINDOW );
			Scene()->RemoveAllAttached( nID, ESSOT_LIGHT );
		}
		UpdateIcons();
	}
	
	return nResult;
}

void CMapObj::AIUpdateHit( const NDb::SComplexEffect *pEffect, WORD wDir, NTimer::STime time )
{
	if ( pEffect == 0 ) 
		return;
	const NTimer::STime timeEffect = Min( time, GameTimer()->GetGameTime() );
	if ( !pStats->surfacePoints.empty() ) 
	{
		const CVec3 vDir = CVec3( GetVectorByDirection( wDir ), 0 );
		CQuat qInvRot;
		qInvRot.UnitInverse( qRot );
		CVec3 vInvDir;
		qInvRot.Rotate( &vInvDir, vDir );
		int nIndex = -1;
		float fCos = 0;
		for ( int i = 0; i < pStats->surfacePoints.size(); ++i )
		{
			const NDb::SHPObjectRPGStats::SModelSurfacePoint &point = pStats->surfacePoints[i];
			const CQuat qNorm( point.vOrient );
			CVec3 vNorm;
			qNorm.Rotate( &vNorm, V3_AXIS_Z );
			if ( vNorm * vInvDir >= fCos )
			{
				nIndex = i;
				fCos = vNorm * vInvDir;
			}
		}
		if ( nIndex != -1 )
		{
			const NDb::SHPObjectRPGStats::SModelSurfacePoint &point = pStats->surfacePoints[nIndex];
			CVec3 vRelPos = point.vPos;
			qRot.Rotate( &vRelPos, vRelPos );
			vRelPos += vPos;
			AI2Vis( &vRelPos );
			CQuat qNorm( point.vOrient );
			SHMatrix mPlace( vRelPos, qRot * qNorm );
			PlayComplexEffect( OBJECT_ID_FORGET, pEffect, timeEffect, mPlace );
		}
		else
			PlayComplexEffect( OBJECT_ID_FORGET, pEffect, timeEffect, vPos );
	}
	else
	{
		PlayComplexEffect( OBJECT_ID_FORGET, pEffect, timeEffect, vPos );
	}
}

void CMapObj::AIUpdateKeyObject( const struct SAINotifyKeyBuilding &update )
{
	nKeyObjectPlayer = update.nPlayer;
	
	UpdateIcons();
}

void CMapObj::AINewUnitInfo( const struct SNewUnitInfo &info, struct IScene *pScene, struct ISoundScene *pSoundScene, NDb::ESeason eSeason )
{
	nPlayer = info.nPlayer;

	AIUpdatePlacement( info, pScene, pSoundScene, eSeason );

	if ( info.fHitPoints == 0.0f )
		fHP = 0.0f;
}

void CMapObj::AIUpdatePlacement( const SAINotifyPlacement &placement, IScene *pScene, struct ISoundScene *pSoundScene, NDb::ESeason eSeason )
{
	if ( placement.bNewFormat )
	{
		SetPlacement( placement.vPlacement, placement.rotation );
		pScene->MoveObject( GetID(), placement.vPlacement, placement.rotation );
	}
	else
	{
		CQuat quat;
		quat.FromAngleAxis( ToRadian( float( placement.dir ) / 65536.0f * 360.0f ), 0, 0, 1 );
		// move main object
		MakeOrientation( &quat, DWORDToVec3(placement.dwNormal) );
		SetPlacement( CVec3( placement.center, placement.z ), quat );
		pScene->MoveObject( GetID(), CVec3( placement.center, placement.z ), quat, vScale );
	}
	for ( int i = 0; i < __EAST_COUNTER__; ++i )
	{
		if ( attachedSounds[i] != 0 )
			pSoundScene->SetSoundPos( attachedSounds[i], GetCenter() );
	}
}

const NDb::SAnimB2* CMapObj::GetAnimation( const int _nAnimation )
{
	if ( _nAnimation < 0 )
		return 0;

	if ( GetTypeID() == NDb::SInfantryRPGStats::typeID )
	{
		CDynamicCast<NDb::SInfantryRPGStats> pStats = GetStats();
		if ( !pStats )
			return 0;			

		int nAnimation = _nAnimation;
		if ( pStats->animdescs.size() <= nAnimation )
			nAnimation = NDb::ANIMATION_IDLE;

		if ( pStats->animdescs.size() <= nAnimation )
			return 0;

		const vector<NDb::SAnimDesc> &anims = pStats->animdescs[nAnimation].anims;
		if ( anims.empty() )
			return 0;

		return checked_cast_ptr<const NDb::SAnimB2*>( anims[0].pAnimation );
	}
	else
	{
		if ( pModel && pModel->pSkeleton && _nAnimation < pModel->pSkeleton->animations.size() ) 
		{
			return checked_cast_ptr<const NDb::SAnimB2*>( pModel->pSkeleton->animations[_nAnimation] );
		}
		else
			return 0;
	}

	return 0;
}

bool CMapObj::AIUpdateDiplomacy( const struct SAINotifyDiplomacy &diplomacy )
{
	nPlayer = diplomacy.nPlayer;
	SetDiplomacy( diplomacy.eDiplomacy );
	return IsFriend();
}

void CMapObj::AIUpdateAnimationChanged( const NDb::SAnimB2 *pAnim, const NTimer::STime startTime )
{
	NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nID );
	if ( pAnimator != 0 )
	{
		SetHasMoveAnimation( false );
		pAnimator->SetSpeedFactorForAllAnimations( startTime, 1.0f );
		if ( HasLoopedAnimation() || pAnim != 0 )
		{
			pAnimator->ClearAllAnimations();
			SetLoopedAnimation( false );
		}
		//
		if ( pAnim != 0 ) 
		{
			const NDb::SAnimB2 *pAnimation = pAnim;
			//
			if ( pAnimation != 0 ) 
			{
				SetLoopedAnimation( pAnimation->bLooped );
				SetHasMoveAnimation( pAnimation->eType == NDb::ANIMATION_CRAWL || pAnimation->eType == NDb::ANIMATION_MARCH || 
					                   pAnimation->eType == NDb::ANIMATION_MOVE || pAnimation->eType == NDb::ANIMATION_WALK );
				SetAnimSpeed( HasMoveAnimation() ? ( pAnimation->fMoveSpeed * AI_TILE_SIZE ) / 1000.0f : 1.0f );
				AddAnimation( pAnimation, startTime, pAnimator );
			}
		}
	}
}

pair<int,bool> CMapObj::PlayAnimation( const int nAnimation )
{
	NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nID );

	if ( pAnimator )
	{
		const int nTotalAnimations = 
			GetTypeID() == NDb::SInfantryRPGStats::typeID ? 
			NDb::__ANIMATION_TYPE_COUNTER : pModel->pSkeleton->animations.size();

		if ( nAnimation >= 0 )
		{
			if ( const NDb::SAnimB2 *pAnimation = GetAnimation( nAnimation ) ) 
			{
				const NTimer::STime startTime = Singleton<IGameTimer>()->GetGameTime();

				pAnimator->SetSpeedFactorForAllAnimations( startTime, 1.0f );
				pAnimator->ClearAllAnimations();
				SetLoopedAnimation( false );

				if ( AddAnimation( pAnimation, startTime, pAnimator ) )
					return pair<int,bool>( pAnimation->nLength, pAnimation->bLooped );
			}
			
			return pair<int,bool>( 0, false );
		}
	}

	return pair<int,bool>( -1, false );
}

void CMapObj::GetStatus( SObjectStatus *pStatus ) const
{
	pStatus->Clear();

	if ( !GetStats() )
		return;

	pStatus->nHP = fHP;
	pStatus->nMaxHP = GetStats()->fMaxHP;

	if ( CHECK_TEXT_NOT_EMPTY_PRE(GetStats()->,LocalizedName) )
		pStatus->szLocalizedName = GET_TEXT_PRE(GetStats()->,LocalizedName);
}

void CMapObj::SetVisible( bool _bVisible, const NDb::ESeason _eSeason, const bool _bIsNight )
{ 
	bool bOldVisible = bVisible;

	bVisible = _bVisible;
	eSeason = _eSeason;
	bIsNight = _bIsNight;

	UpdateVisibility( bVisible != bOldVisible );
}

void CMapObj::UpdateVisibility( bool bForced )
{
	Scene()->ShowObject( GetID(), IsVisible() );
	if ( bForced )
	{
		SwitchLightFX( bVisible, eSeason, bIsNight );
		UpdateIcons();
	}
}

void CMapObj::ChangeLight( const int nLight, NDb::ESeason eSeason, bool bLight )
{
	/*NI_ASSERT( nLight < GetStats()->lights.size(), "Wrong number of light (%d), total number of lights (%d)" );
	if ( nLight < GetStats()->lights.size() )
	{
		const bool bRightLight = bool(attachedLights[nLight]) != bLight;
		NI_ASSERT( bRightLight, "Non-correspondence of lights changing" );
		if ( bRightLight )
		{
			attachedLights[nLight] = bLight;
			
			const NDb::SModel *pLightModel = GetModel( GetStats()->lights[nLight].pvisualObject, eSeason );
			if ( pLightModel )
			{
				const string szBoneName = GetStats()->lights[nLight].szLocator;
				if ( bLight )
					Scene()->AttachSubModel( GetID(), ESSOT_LIGHT, szBoneName, pLightModel, ESAT_NO_REPLACE, nLight, false );
				else
					Scene()->RemoveAttached( GetID(), ESSOT_LIGHT, nLight );
			}
		}
	}*/
}

void CMapObj::GetEnabledActions( CUserActions *pActions, EActionsType eActions ) const
{
	CUserActions actions;
	GetActions( &actions, eActions );
	CUserActions actionsDisabled;
	GetDisabledActions( &actionsDisabled, eActions );
	actions &= ~actionsDisabled;
	*pActions |= actions;
}

NDb::EUserAction CMapObj::GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, 
	bool bAltMode ) const
{
	const NDb::SClientGameConsts *pClient = Singleton<IClientGameConsts>()->GetClientGameConsts();

	CUserActions actions = actionsBy;
	actions &= *pActionsWith;
	
	const vector<NDb::EUserAction> &priority = IsFriend() ? pClient->actionsPriority.friendActions : 
	( IsEnemy() ? pClient->actionsPriority.enemyActions : pClient->actionsPriority.neutralActions );
	for ( int i = 0; i < priority.size(); ++i )
	{
		// если в списке встретилось NDb::USER_ACTION_UNKNOWN, выберем нейтральное действие по умолчанию
		if ( priority[i] == NDb::USER_ACTION_UNKNOWN )
			return NDb::USER_ACTION_UNKNOWN;
			
		if ( actions.HasAction( priority[i] ) )
			return priority[i];
	}
	return NDb::USER_ACTION_UNKNOWN;
}

NDb::EUserAction CMapObj::GetBestAutoAction( const CUserActions &actionsBy, bool bAltMode ) const
{
	CUserActions actionsWith;
	GetEnabledActions( &actionsWith, ACTIONS_WITH );

	return GetBestAutoAction( actionsBy, &actionsWith, bAltMode );
}

NDb::EUserAction CMapObj::GetBestSelfAction( const CUserActions &actionsBy, bool bAltMode ) const
{
	const NDb::SClientGameConsts *pClient = Singleton<IClientGameConsts>()->GetClientGameConsts();

	for ( int i = 0; i < pClient->actionsPriority.selfActions.size(); ++i )
		if ( actionsBy.HasAction( pClient->actionsPriority.selfActions[i] ) )
			return static_cast<NDb::EUserAction>( pClient->actionsPriority.selfActions[i] );
	return NDb::USER_ACTION_UNKNOWN;
}

void CMapObj::SetColorIndex( int _nColorIndex, bool bForceUpdate )
{
	nColorIndex = _nColorIndex;
	AIUpdateKeyObjectCaptureProgress( 0.0f, 0 );
	if ( bForceUpdate )
		UpdateIcons();
}

void CMapObj::UpdateIcons()
{
	if ( CanShowIcons() )
	{
		SSceneObjIconInfo iconInfo( GetID() );
		FillIconsInfo( iconInfo );
		Scene()->SetIcon( iconInfo );
  }
	else
		Scene()->RemoveIcon( GetID() );
}

// CMOSelectable

void CMOSelectable::FillIconsInfo( SSceneObjIconInfo &iconInfo )
{
	const float fAlpha = ( bHighlighted ? SOLID_ICON_ALPHA : FADED_ICON_ALPHA ) + 
		( IsMousePicked() ? MOUSE_PICKED_ALPHA_BONUS : 0.0f );
	iconInfo.fAlpha = Clamp( fAlpha, 0.0f, 1.0f );
	iconInfo.nHPBarBaseLength = GetIconsHPBarLen();
	iconInfo.fAddHeight = GetIconsRaising();

	iconInfo.bIsMainHitbar = bIconHitbar;
	if ( iconInfo.bIsMainHitbar )
	{
		iconInfo.fHPBarValue = GetHP();
		iconInfo.nHPBarColorIndex = GetColorIndex();
		iconInfo.fHPBarAdditionalValue = 0.0f;
		iconInfo.nHPBarAdditionalColorIndex = GetColorIndex();
	}

	if ( HasVisualGroup( EUS_KEY_POINT ) )
		iconInfo.eIconDamagedBuilding = NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NEED_REPAIR_KEY_OBJECT;

	if ( IsFriend() )
	{
		iconInfo.eIconGroup = eIconGroup;

		if ( HasVisualGroup( EUS_PROCESS_GROUP ) )	
			iconInfo.icons.push_back( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_PROCESS );
		if ( HasVisualGroup( EUS_GRUAD_GROUP ) )	
			iconInfo.icons.push_back( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GUARD );
		if ( HasVisualGroup( EUS_REPAIRS_GROUP ) )	
			iconInfo.icons.push_back( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_BROKENTRUCK );
		if ( HasVisualGroup( EUS_RESUPPLY_GROUP ) )	
			iconInfo.icons.push_back( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NEED_RESUPPLY );
		if ( HasVisualGroup( EUS_AGGRESSIVE_GROUP ) )	
			iconInfo.icons.push_back( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_AGRESSIVE );
		if ( HasVisualGroup( EUS_BEST_SHOT_GROUP ) )	
			iconInfo.icons.push_back( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_BEST_SHOT );
		if ( HasVisualGroup( EUS_PACIFIC_GROUP ) )	
			iconInfo.icons.push_back( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_PACIFIST );
		if ( HasVisualGroup( EUS_INVISIBLE_GROUP ) )	
			iconInfo.icons.push_back( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_INVISIBLE );
	}
}

void CMOSelectable::SetIconsHitbar( bool bHitbar, bool _bHighlighted )
{
	bIconHitbar = bHitbar;
	bHighlighted = _bHighlighted;
}

void CMOSelectable::SetIconsGroup( int nGroup, bool _bHighlighted )
{
  if ( nGroup >= 0 )
  {
		eIconGroup = static_cast<NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType>( 
			NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP00 + nGroup );
	}
	else
		eIconGroup = NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE;
	bHighlighted = _bHighlighted;
}

void CMOSelectable::DisableIcons( bool bDisable )
{
	bDisableIcons = bDisable;
	if ( !CanShowIcons() )
	  Scene()->RemoveIcon( GetID() );
}

bool CMOSelectable::CanShowIcons() const
{
	return IsVisible() && !bDisableIcons && NGlobal::GetVar( "MissionIconsMovieMode", 0.0f ) == 0.0f;
}

void CMOSelectable::Select( bool bSelect )
{
	bSelected = bSelect;
	if ( !bSelected )
	{
		SetSelectionGroup( -1 );
	}
}

void CMOSelectable::SetSelectionGroup( int _nSelectionGroup )
{
	if ( nSelectionGroup != _nSelectionGroup )
	{
		nSelectionGroup = _nSelectionGroup;
		UpdateIcons();
	}
}

NDb::EUserAction CMOSelectable::GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, bool bAltMode ) const
{
	if ( bAltMode && pActionsWith->HasAction( NDb::USER_ACTION_FOLLOW ) )
		return NDb::USER_ACTION_FOLLOW;

	return CMapObj::GetBestAutoAction( actionsBy, pActionsWith, bAltMode );
}	

bool CMOSelectable::CanSelect() const
{ 
	return bCanSelect && GetStats()->eSelectionType != NDb::SELECTION_TYPE_NONE;
}

void CMOSelectable::UpdateVisualStatus( const struct SUnitStatusUpdate &update )
{
	if ( (update.eStatus & EUS_STATUS_MASK) == 0 )
	{
		ClearVisualGroup( update.eStatus );
	}
	else
	{
		SetVisualGroup( update.eStatus );
	}
/*	if ( fVisualRadius != update.fRadius )
	{
		fVisualRadius = update.fRadius;
		Scene()->SetCircle( GetID(), fVisualRadius, HIDE_RADIUS_COLOR, HIDE_RADIUS_WIDTH );
	}*/
//	Scene()->SetCircle( GetID(), 5.0f, HIDE_RADIUS_COLOR, HIDE_RADIUS_WIDTH ); // CRAP
	
	UpdateIcons();
}

bool CMOSelectable::HasVisualGroup( enum EUnitStatus eGroup ) const
{
	return (dwVisualStatus & (1 << (eGroup >> 16))) != 0;
}

void CMOSelectable::SetVisualGroup( enum EUnitStatus eGroup )
{
	dwVisualStatus |= 1 << (eGroup >> 16);
}

void CMOSelectable::ClearVisualGroup( enum EUnitStatus eGroup )
{
	dwVisualStatus &= ~(1 << (eGroup >> 16));
}

IMOContainer* CMOSelectable::GetTopContainer() const
{
	IMOContainer *pTopContainer = GetContainer();
	while ( pTopContainer )
	{
		if ( IMOContainer *pContainer = pTopContainer->GetContainer() )
			pTopContainer = pContainer;
		else
			break;
	}
	return pTopContainer;
}

// IMOContainer

void IMOContainer::GetPassangers( vector<IB2MapObj*> *pPassangers ) const
{
	vector<CMOSelectable*> passangers;
	GetPassangers( &passangers );
	pPassangers->resize( passangers.size() );
	for ( int i = 0; i < passangers.size(); ++i )
		(*pPassangers)[i] = passangers[i];
}

void IMOContainer::FillIconsInfo( SSceneObjIconInfo &iconInfo )
{
	CMOSelectable::FillIconsInfo( iconInfo );

	if ( iconInfo.bIsMainHitbar )
	{
		vector<CMOSelectable*> passengers;
		GetPassangers( &passengers );
		iconInfo.smallHitbars.reserve( passengers.size() );
		const NDb::SClientGameConsts *pClient = Singleton<IClientGameConsts>()->GetClientGameConsts();
		iconInfo.nHPBarPassengerLength = pClient->passengerIconsSet.fHPBarLen;
		for ( int i = 0; i < passengers.size(); ++i )
		{
			CMapObj *pMO = passengers[i];
			if ( pMO )
			{
				SSceneObjIconInfo::SHitbar hitBar;
				hitBar.fValue = pMO->GetVisualHPFraction();
				hitBar.nColorIndex = pMO->GetColorIndex();
				iconInfo.smallHitbars.push_back( hitBar );
			}
		}
		iconInfo.fHPBarValue = GetHP();
		iconInfo.nHPBarColorIndex = GetColorIndex();
		iconInfo.fHPBarAdditionalValue = 0.0f;
		iconInfo.nHPBarAdditionalColorIndex = GetColorIndex();
	}
}


void PlaceCrater( const NDb::SCraterSet *pCrater, NDb::ESeason eSeason, const CVec2 &vPos )
{
	int nSeasonIndex = -1;
	for ( int i = 0; i < pCrater->craters.size(); ++i )
	{
		if ( pCrater->craters[i].eSeason == eSeason )
		{
			nSeasonIndex = i;
			break;
		}
	}

	if ( nSeasonIndex < 0 )
	{
		// Lets find default description
		for ( int i = 0; i < pCrater->craters.size(); ++i )
		{
			if ( pCrater->craters[i].eSeason == NDb::SEASON_SUMMER )
			{
				nSeasonIndex = i;
				break;
			}
		}
	}
	if ( nSeasonIndex >= 0 )
	{
		const NDb::SCraterSet::SSingleSeasonCraters &craters = pCrater->craters[nSeasonIndex];
		if ( !craters.craters.empty() )
		{
			const NDb::SCraterSet::SSingleSeasonCraters::SCraterDesc &crater = craters.craters[NWin32Random::Random( 0, craters.craters.size() - 1 )];
			NI_ASSERT( crater.pMaterial != 0, "Crater material not set" );
			if ( crater.pMaterial != 0 )
			{
				const NDb::STexture *pTexture = crater.pMaterial->pTexture;
				CVec2 vSize( pTexture->nWidth, pTexture->nHeight );
				vSize *= crater.fScale * 0.5f;
				Vis2AI( &vSize );
				CVec2 v1 = vPos - vSize;
				CVec2 v2 = vPos + vSize;
				Scene()->AddExplosion( v1, v2, crater.pMaterial );
			}
		}
	}
}

void PlaySoundEffect( const int nID, const NDb::SComplexSoundDesc *pEffect, NTimer::STime timeStart, const CVec3 &vPos )
{
	if ( pEffect )
	{
		const NTimer::STime currTime = GameTimer()->GetGameTime();
		SoundScene()->AddSound( pEffect, vPos, SFX_MIX_IF_TIME_EQUALS, SAM_ADD_N_FORGET, currTime > timeStart ? currTime - timeStart : 0, 2 );
	}
}

int PlayComplexEffect( const int nID, const NDb::SComplexEffect *pEffect, NTimer::STime timeStart, const CVec3 &vPos )
{
	PlaySoundEffect( nID, pEffect->pSoundEffect, timeStart, vPos );
	
	if ( const NDb::SEffect *pSceneEffect = pEffect->GetSceneEffect() )
	{
		const NTimer::STime currTime = GameTimer()->GetGameTime();
		// randomize effect direction
		const float fAngle = NWin32Random::Random( 0.0f, FP_2PI );
		return Scene()->AddEffect( nID, pSceneEffect, Min(timeStart, currTime), vPos, CQuat(fAngle, V3_AXIS_Z, false) ); 
	}

	return -1;
}

int PlayComplexSeasonedEffect( const int nID, const NDb::SComplexSeasonedEffect *pEffect, NTimer::STime timeStart, const CVec3 &vPos, NDb::ESeason eSeason )
{
	const NTimer::STime currTime = GameTimer()->GetGameTime();
	if ( pEffect->pSoundEffect )
		SoundScene()->AddSound( pEffect->pSoundEffect, vPos, SFX_MIX_IF_TIME_EQUALS, SAM_ADD_N_FORGET, currTime > timeStart ? currTime - timeStart : 0, 2 );
	for ( int i = 0; i < pEffect->seasons.size(); ++i )
	{
		const NDb::SSeasonEffect &sceneEffect = pEffect->seasons[i];
		if ( sceneEffect.eSeasonToUse == eSeason && sceneEffect.pSceneEffect != 0 )
		{
			// randomize effect direction
			const float fAngle = NWin32Random::Random( 0.0f, FP_2PI );
			return Scene()->AddEffect( nID, sceneEffect.pSceneEffect, Min(timeStart, currTime), vPos, CQuat(fAngle, V3_AXIS_Z, false) ); 
		}
	}
	return -1;
}

int PlayComplexSeasonedEffect( const int nID, const NDb::SComplexSeasonedEffect *pEffect, NTimer::STime timeStart, const CVec3 &vPos, const CQuat &qRot, NDb::ESeason eSeason )
{
	const NTimer::STime currTime = GameTimer()->GetGameTime();
	if ( pEffect->pSoundEffect )
		SoundScene()->AddSound( pEffect->pSoundEffect, vPos, SFX_MIX_IF_TIME_EQUALS, SAM_ADD_N_FORGET, currTime > timeStart ? currTime - timeStart : 0, 2 );
	for ( int i = 0; i < pEffect->seasons.size(); ++i )
	{
		const NDb::SSeasonEffect &sceneEffect = pEffect->seasons[i];
		if ( sceneEffect.eSeasonToUse == eSeason && sceneEffect.pSceneEffect != 0 )
			return Scene()->AddEffect( nID, sceneEffect.pSceneEffect, Min(timeStart, currTime), vPos, qRot );
	}
	return -1;
}

int PlayComplexSeasonedEffect( const int nID, const NDb::SComplexSeasonedEffect *pEffect, NTimer::STime timeStart, const SHMatrix &mPlace, NDb::ESeason eSeason )
{
	const NTimer::STime currTime = GameTimer()->GetGameTime();
	if ( pEffect->pSoundEffect )
	{
		CVec3 vPos = mPlace.GetTrans3();
		SoundScene()->AddSound( pEffect->pSoundEffect, vPos, SFX_MIX_IF_TIME_EQUALS, SAM_ADD_N_FORGET, currTime > timeStart ? currTime - timeStart : 0, 2 );
	}
	for ( int i = 0; i < pEffect->seasons.size(); ++i )
	{
		const NDb::SSeasonEffect &sceneEffect = pEffect->seasons[i];
		if ( sceneEffect.eSeasonToUse == eSeason && sceneEffect.pSceneEffect != 0 )
			return Scene()->AddEffect( nID, sceneEffect.pSceneEffect, Min(timeStart, currTime), mPlace );
	}
	return -1;
}

int PlayComplexEffect( const int nID, const NDb::SComplexEffect *pEffect, NTimer::STime timeStart, const SHMatrix &mPlace )
{
	const NTimer::STime currTime = GameTimer()->GetGameTime();
	if ( pEffect->pSoundEffect )
	{
		CVec3 vPos( mPlace._14, mPlace._24, mPlace._34 );
		Vis2AI( &vPos );
		SoundScene()->AddSound( pEffect->pSoundEffect, vPos, SFX_MIX_IF_TIME_EQUALS, SAM_ADD_N_FORGET, currTime > timeStart ? currTime - timeStart : 0, 2 );
	}
	if ( const NDb::SEffect *pSceneEffect = pEffect->GetSceneEffect() )
		return Scene()->AddEffect( nID, pSceneEffect, Min(timeStart, currTime), mPlace ); 
	return -1;
}

void PlayComplexEffect( const int nID, const string &szBoneName, ESceneSubObjType eType, const NDb::SComplexEffect *pEffect, NTimer::STime timeStart, ESceneAttachMode eMode )
{
	const NTimer::STime currTime = GameTimer()->GetGameTime();
	if ( pEffect->pSoundEffect )
	{
		if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nID, szBoneName ) )
		{
			SHMatrix mPoint;
			pAnimator->GetBonePosition( szBoneName.c_str(), &mPoint );
			CVec3 vPos( mPoint._14, mPoint._24, mPoint._34 );
			Vis2AI( &vPos );
			SoundScene()->AddSound( pEffect->pSoundEffect, vPos, SFX_MIX_IF_TIME_EQUALS, SAM_ADD_N_FORGET, currTime > timeStart ? currTime - timeStart : 0, 2 );
		}
	}
	if ( const NDb::SEffect *pSceneEffect = pEffect->GetSceneEffect() )
		Scene()->AttachEffect( nID, eType, szBoneName, pSceneEffect, Min(timeStart, currTime), eMode ); 
}

bool RunDefaultObjectAnimation( const NDb::SSkeleton *pSkeleton, NAnimation::ISkeletonAnimator *pAnimator )
{
	if ( pAnimator == 0 ) 
		return false;
	//
	for ( int i = 0; i < pSkeleton->animations.size(); ++i )
	{
		const NDb::SAnimB2 *pAnim = checked_cast_ptr<const NDb::SAnimB2*>( pSkeleton->animations[i] );
		if ( pAnim && pAnim->eType == NDb::ANIMATION_IDLE )
		{
			AddAnimation( pAnim, GameTimer()->GetGameTime(), pAnimator );
			return true;
		}
	}
	return false;
}

void GetPlacementFromUpdate( CVec3 *pvPos, CQuat *pqRot, const SAINewUnitUpdate *pUpdate )
{
	if ( pUpdate )
	{
		if ( pUpdate->info.bNewFormat )
		{
			*pvPos = pUpdate->info.vPlacement;
			*pqRot = pUpdate->info.rotation;
		}
		else
		{
			*pvPos = CVec3( pUpdate->info.center.x, pUpdate->info.center.y, pUpdate->info.z );
			*pqRot = CQuat( float(pUpdate->info.dir) / 65536 * FP_2PI, V3_AXIS_Z );
		}
	}
	else
	{
		*pvPos = VNULL3;
		*pqRot = QNULL;
	}
}

int CMapObj::operator&( IBinSaver &saver )
{
	saver.Add( 3, &vPos );
	saver.Add( 4, &qRot );
	saver.Add( 5, &vScale );
	saver.Add( 6, &fHP );
	saver.Add( 8, &nID );
	saver.Add( 9, &eDiplomacy );
	saver.Add( 10, &pStats );
	saver.Add( 11, &bLoopedAnimation );
	//saver.Add( 11, &nObjectID );
	saver.Add( 12, &bVisible );
	saver.Add( 13, &bIsSilentlyDead );
	saver.Add( 14, &bHasMoveAnimation );
	saver.Add( 15, &fAnimationSpeed );
	saver.Add( 17, &attachedSounds );
	saver.Add( 18, &nKeyObjectPlayer );
	saver.Add( 19, &pModel );
	saver.Add( 20, &nParentID );
	saver.Add( 21, &nPlayer );
	saver.Add( 22, &nColorIndex );
	saver.Add( 23, &eSeason );
	saver.Add( 24, &bIsNight );

	//
	if ( saver.IsReading() )
	{
		if ( attachedSounds.size() != __EAST_COUNTER__ )
			attachedSounds.resize( __EAST_COUNTER__, 0 );
	}

	return 0;
}


void CombineAbilities( CAbilityInfo *pAbilities, NDb::EUnitSpecialAbility eAbility, const SAbilityInfo &abilityInfo )
{
	CAbilityInfo::iterator it = pAbilities->find( eAbility );
	if ( it == pAbilities->end() )
	{
		// Not found, add
		(*pAbilities)[eAbility] = abilityInfo;
	}
	else
	{
		// Ability is enabled if it is enabled for any unit
		// Priority: in progress, enabled, on, disabled
		SAbilityInfo &oldAbilityInfo = it->second;

		if ( oldAbilityInfo.abilityState.eState == EASS_SWITCHING_ON || 
			oldAbilityInfo.abilityState.eState == EASS_SWITCHING_OFF ||
			oldAbilityInfo.abilityState.eState == EASS_OFF )
		{
			// in progress, do nothing
		}
		else if ( abilityInfo.abilityState.eState == EASS_SWITCHING_ON || 
			abilityInfo.abilityState.eState == EASS_SWITCHING_OFF ||
			abilityInfo.abilityState.eState == EASS_OFF )
		{
			// in progress, new
			oldAbilityInfo = abilityInfo;
		}
		else if ( oldAbilityInfo.abilityState.eState == EASS_READY_TO_ON )
		{
			// enabled, do nothing
		}
		else if ( abilityInfo.abilityState.eState == EASS_READY_TO_ON )
		{
			// enabled, new
			oldAbilityInfo = abilityInfo;
		}
		else if ( oldAbilityInfo.abilityState.eState == EASS_ACTIVE )
		{
			// enabled, do nothing
		}
		else if ( abilityInfo.abilityState.eState == EASS_ACTIVE )
		{
			// enabled, new
			oldAbilityInfo = abilityInfo;
		}
	}
}

void CombineAbilities( CAbilityInfo *pAbilities, const CAbilityInfo &abilities )
{
	for ( CAbilityInfo::const_iterator it = abilities.begin(); it != abilities.end(); ++it )
		CombineAbilities( pAbilities, it->first, it->second );
}

START_REGISTER(MapObjectCommands)
REGISTER_VAR_EX( "object_icons_faded_alpha", NGlobal::VarFloatHandler, &FADED_ICON_ALPHA, 0.5f, STORAGE_NONE );
FINISH_REGISTER


