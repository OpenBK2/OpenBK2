#include "stdafx.h"

#include "DeadHouseAnimation.h"
#include "DeadHouseAnimations.h"
#include "MOBuilding.h"
#include "Stats_B2_M1/IClientGameConsts.h"
#include "Input/Bind.h"
#include "Misc/Win32Random.h"
#include "SceneB2/AttachedObj.h"
#include "SceneB2/WindController.h"
#include "Sound/SoundScene.h"
#include "AILogic/ScenarioTracker.h"
#include "System/Text.h"

namespace
{

#define DAMAGE_EFFECT_PROBABILITY 30
#define WINDOW_BREAK_PROBABILITY 50

typedef std::unordered_map< NDb::EDesignBuildingType, SIconsSetInfo, SEnumHash > CIconsSet;
static bool bIsInitializedByDB = false;
CIconsSet iconsSets;
SIconsSetInfo iconsSetDefault;

const SIconsSetInfo& GetDBIconsSet( NDb::EDesignBuildingType eType )
{
	if ( !bIsInitializedByDB )
	{		
		const NDb::SClientGameConsts *pClient = Singleton<IClientGameConsts>()->GetClientGameConsts();
		
		for ( int i = 0; i < pClient->buildingIconsSets.size(); ++i )
		{
			const NDb::SClientGameConsts::SBuildingIconsSet &iconsSet = pClient->buildingIconsSets[i];
			iconsSets[iconsSet.eType] = SIconsSetInfo( iconsSet.fRaising, iconsSet.fHPBarLen );
			if ( iconsSet.eType == NDb::BUILDING_TYPE_UNKNOWN )
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

// CMOBuilding

CMOBuilding::CMOBuilding() :
	wAmbientSound(0), 
	wCycledSound(0), 
	wCycledSoundTimed(0),
	fCapturingProgress( 0.0f ),
	bCanEnter( true )
{
	SetCanSelect( false );
}

bool CMOBuilding::IsInside( const int nID )
{
	for ( std::vector< CPtr<CMOSelectable> >::iterator it = vPassangers.begin(); it != vPassangers.end(); ++it )
	{
		if ( (*it)->GetID() == nID )
			return true;
	}
	return false;
}

bool CMOBuilding::CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor )
{
	const float fNewHP = pUpdate->info.fHitPoints / GetStats()->fMaxHP;
	const NDb::SVisObj *pVisObj = ChooseVisObjForHP( fNewHP );
	if ( pVisObj == 0 )
		return false;

	nOldModelStage = 0;
	for ( int i = 0; i < GetStats()->damageLevels.size(); ++i )
	{
		const NDb::SHPObjectRPGStats::SDamageLevel &level = GetStats()->damageLevels[i];
		if ( level.pVisObj == pVisObj )
		{
			nOldModelStage = i;
			break;
		}
	}
	const NDb::SModel *pModel = GetModel( pVisObj, eSeason );
	if ( pModel == 0 )
		return false;
	//
	CVec3 vPos;
	CQuat qRot;
	GetPlacementFromUpdate( &vPos, &qRot, pUpdate );
	SetPlacement( vPos, qRot );

	if ( fNewHP <= 0 )
		AddDynamicDebris( pModel, vPos, qRot, eSeason );

	const NDb::SBuildingRPGStats *pStats = checked_cast<const NDb::SBuildingRPGStats*>( GetStats() );
	bool bAnimated = pVisObj->bForceAnimated; 

	// Check if there are any non-death animations
	for ( int i = 0; i < pModel->animations.size(); ++i )
	{
		const NDb::SAnimB2 *pAnim = checked_cast_ptr< const NDb::SAnimB2 * >( pModel->animations[ i ] );
		if ( pAnim && pAnim->eType != NDb::ANIMATION_DEATH )
		{
			bAnimated = true;
			break;
		}
	}
	// Init attached objects
	attachedObjects.clear();
	attachedObjects.resize( __ESSOT_COUNTER ); 

	attachedWindows.resize( GetStats()->slots.size(), EWS_NONE );

	attachedObjectsHP.resize( GetStats()->slots.size(), 0.0f );
	for ( int i = 0; i < GetStats()->slots.size(); ++i )
		attachedObjectsHP[i] = GetStats()->slots[i].window.fMaxHP;

	fMaxDistance = 0;

	for ( int i = 0; i < GetStats()->slots.size(); ++i )
		for ( int j = 0; j < GetStats()->slots.size(); ++j )
		{
			if ( i != j )
				fMaxDistance = (std::max)( fabs2( GetStats()->slots[i].vDamageCenter - GetStats()->slots[j].vDamageCenter ), fMaxDistance );
		}

	fBuildingHP = pUpdate->info.fHitPoints;

	Scene()->AddObject( nUniqueID, pModel, vPos, qRot, CVec3(1, 1, 1), 
		                  bAnimated ? OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC : OBJ_ANIM_MODE_FORCE_NON_ANIMATED, 0 );
	SetModel( pModel );

	return true;
}

bool CMOBuilding::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	fMaxDistance = 0;
	const SAINewUnitUpdate *pUpdate = checked_cast<const SAINewUnitUpdate *>( _pUpdate );
	const NDb::SBuildingRPGStats *pStats = checked_cast_ptr<const NDb::SBuildingRPGStats*>( pUpdate->info.pStats );
	SetStats( pStats );
	if ( pStats )
		bStorage = pStats->etype == NDb::TYPE_MAIN_RU_STORAGE || pStats->etype == NDb::TYPE_MAIN_RU_STORAGE;

	if ( !bInEditor ) 
	{
		if ( pStats->iconsSetParams.bCustom )
			SetIconsSetInfo( SIconsSetInfo( pStats->iconsSetParams.fRaising, pStats->iconsSetParams.fHPBarLen ) );
		else
		{
			const SIconsSetInfo &info = GetDBIconsSet( pStats->eBuildingType );
			SetIconsSetInfo( info );
		}
	}

	nCurrentAmmo = 0;
	for ( int i = 0; i < GetStats()->slots.size(); ++i )
	{
		const NDb::SBaseGunRPGStats &gun = GetStats()->slots[i].gun;
		if ( gun.pWeapon )
		{
			SObjectStatus::SWeapon weapon;

			nCurrentAmmo += gun.nAmmo;
		}
	}

	if ( CMapObj::Create(nUniqueID, _pUpdate, eSeason, eDayTime, bInEditor) )
	{
		RunDefaultObjectAnimation( GetModelDesc()->pSkeleton, Scene()->GetAnimator(GetID()) );
		if ( !bInEditor )
		{
			ISoundScene *pSoundScene = Singleton<ISoundScene>();
			const NDb::SComplexSoundDesc* pAmbientSoundDesc = pStats->GetAmbientSoundDesc( eSeason, eDayTime );
			if ( pAmbientSoundDesc )
				wAmbientSound = pSoundScene->AddSoundToMap( pAmbientSoundDesc, GetCenter() );
			if ( pStats->pCycledSound )
				wCycledSound = pSoundScene->AddSoundToMap( pStats->pCycledSound, GetCenter() );
			if ( pStats->cycledSoundTimed.size() > eDayTime && pStats->cycledSoundTimed[eDayTime] )
				wCycledSoundTimed = pSoundScene->AddSoundToMap( pStats->cycledSoundTimed[eDayTime], GetCenter() );
		}
		SetCanSelect( false );
		return true;
	}

	return false;
}

void CMOBuilding::SwitchLightFX( const bool bNewState, const NDb::ESeason eSeason, const bool bIsNight, const bool bInEditor )
{
	IMOContainer::SwitchLightFX( bNewState, eSeason, bIsNight, bInEditor );

	if ( bNewState && GetHP() != 0 )
		ToggleNightWindows( bIsNight, eSeason );
}

void CMOBuilding::GetActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( eActions == ACTIONS_BY || eActions == ACTIONS_ALL )
	{
		pActions->SetAction( NDb::USER_ACTION_LEAVE );
	}
	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		pActions->SetAction( NDb::USER_ACTION_BOARD );
		pActions->SetAction( NDb::USER_ACTION_ATTACK );
		pActions->SetAction( NDb::USER_ACTION_ENGINEER_REPAIR );
		
		const NDb::SBuildingRPGStats *pStats = checked_cast<const NDb::SBuildingRPGStats*>( GetStats() );
		if ( GetHP() < 0.1f ) // don't allow filling resources when damaged
			pActions->RemoveAction( NDb::USER_ACTION_FILL_RU );
		else if ( IsStorage() )
			pActions->SetAction( NDb::USER_ACTION_FILL_RU );

		if ( IsKeyObject() && Singleton<IAIScenarioTracker>()->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
			pActions->RemoveAction( NDb::USER_ACTION_BOARD );
	}
}

void CMOBuilding::GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( eActions == ACTIONS_BY || eActions == ACTIONS_ALL )
	{
		if ( GetPassangersCount() == 0 )
			pActions->SetAction( NDb::USER_ACTION_LEAVE );
	}
	if ( eActions == ACTIONS_WITH || eActions == ACTIONS_ALL )
	{
		int nEntranceCount = GetStats() ? GetStats()->entrances.size() : 0;
		if ( GetHP() == 0.0f || GetFreePlaces() == 0 || nEntranceCount == 0 || !bCanEnter )
			pActions->SetAction( NDb::USER_ACTION_BOARD );
		if ( GetHP() == 0.0f )
			pActions->SetAction( NDb::USER_ACTION_ATTACK );
		if ( GetHP() == 1.0f )
			pActions->SetAction( NDb::USER_ACTION_ENGINEER_REPAIR );
	}
}

NDb::EUserAction CMOBuilding::GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, 
	bool bAltMode ) const
{
	if ( bAltMode && pActionsWith->HasAction( NDb::USER_ACTION_BOARD ) )
		return NDb::USER_ACTION_BOARD;

	if ( GetPassangersCount() == 0 )
		pActionsWith->RemoveAction( NDb::USER_ACTION_ATTACK );
	return CMOSelectable::GetBestAutoAction( actionsBy, pActionsWith, bAltMode );
}

void CMOBuilding::GetStatus( SObjectStatus *pStatus ) const
{
	IMOContainer::GetStatus( pStatus );

	// Armor
	if ( GetStats()->defences.size() >= ARMOR_COUNT )
	{
		pStatus->armors[EOS_ARMOR_FRONT] = (GetStats()->defences[ARMOR_FRONT].nArmorMin + GetStats()->defences[ARMOR_FRONT].nArmorMax) / 2;
		pStatus->armors[EOS_ARMOR_SIDE] = (GetStats()->defences[ARMOR_SIDE_1].nArmorMin + GetStats()->defences[ARMOR_SIDE_1].nArmorMax +
			GetStats()->defences[ARMOR_SIDE_2].nArmorMin + GetStats()->defences[ARMOR_SIDE_2].nArmorMax) / 4;
		pStatus->armors[EOS_ARMOR_BACK] = (GetStats()->defences[ARMOR_BACK].nArmorMin + GetStats()->defences[ARMOR_BACK].nArmorMax) / 2;
		pStatus->armors[EOS_ARMOR_TOP] = (GetStats()->defences[ARMOR_TOP].nArmorMin + GetStats()->defences[ARMOR_TOP].nArmorMax) / 2;
	}

	if ( GetStats()->pArmorPattern )
		pStatus->pArmorPattern = GetStats()->pArmorPattern;

	for ( int i = 0; i < GetStats()->slots.size(); ++i )
	{
		const NDb::SBaseGunRPGStats &gun = GetStats()->slots[i].gun;
		if ( gun.pWeapon )
		{
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
			weapon.nAmmo = nCurrentAmmo;

			pStatus->AddWeapon( weapon );
			
			break;
		}
	}
}

bool CMOBuilding::LoadSquad( struct IMOSquad *pSquad, bool bEnter )
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

void CMOBuilding::GetPassangers( std::vector<CMOSelectable*> *pBuffer ) const
{
	NI_ASSERT( pBuffer, "Wrong pointer" );
	for ( std::vector< CPtr<CMOSelectable> >::const_iterator it = vPassangers.begin(); it != vPassangers.end(); ++it )
		pBuffer->push_back( (*it) );
}

int CMOBuilding::GetFreePlaces() const
{
	const NDb::SBuildingRPGStats *stats = checked_cast<const NDb::SBuildingRPGStats*>( GetStats() );
	return stats->nRestSlots + stats->nMedicalSlots - vPassangers.size();
}

bool CMOBuilding::NeedShowInterrior() const
{
	return true;
}

IClientUpdatableProcess* CMOBuilding::AIUpdateRPGStats( const SAINotifyRPGStats &stats, struct IClientAckManager *pAckManager, NDb::ESeason eSeason )
{ 
	nCurrentAmmo = 0;
	for ( std::vector<SAINotifyRPGStats::SWeaponAmmo>::const_iterator it = stats.ammo.begin(); it != stats.ammo.end(); ++it )
	{
		const SAINotifyRPGStats::SWeaponAmmo &updateAmmo = *it;
		nCurrentAmmo += updateAmmo.nAmmo;
		break;
	}
	
	fBuildingHP = stats.fHitPoints;
	const float fNewHP = stats.fHitPoints / GetStats()->fMaxHP;
	bool bAlive = fNewHP > 0;
	int nStage = CommonUpdateHP( fNewHP, stats, Scene(), eSeason );
	if ( nStage >= 0 )
	{
		RemoveSubObjectsFromSlot( -1, ESSOT_LIGHT );
		RemoveSubObjectsFromSlot( -1, ESSOT_WINDOW );
		RemoveSubObjectsFromSlot( -1, ESSOT_EXHAUST );

		CDBPtr<NDb::SBuildingRPGStats> pStats = GetStats();
		const NDb::SHPObjectRPGStats::SDamageLevel &level = pStats->damageLevels[nStage];
		CVec3 vPos, vScale;
		CQuat qRot;
		GetPlacement( &vPos, &qRot, &vScale );
		if ( level.pDamageEffectSmoke != 0 && nStage > nOldModelStage )
		{
			for ( int i = 0; i < pStats->smokePoints.size(); ++i )
			{
				const NDb::SBuildingRPGStats::SFirePoint &point = pStats->smokePoints[i];
				CVec3 vEffectPos;
				const CVec3 vPointPos( point.vPos.y - pStats->vOrigin.x, point.vPos.x - pStats->vOrigin.y, point.vPos.z );
				qRot.Rotate( &vEffectPos, vPointPos ); 
				vEffectPos += vPos;
				const float fAngle = AI2VisRad(point.fDirection) + FP_PI;
				CQuat qEffectRot( fAngle, CVec3( 0, 0, 1.0f ) );
				qEffectRot = qRot * qEffectRot * CQuat( FP_PI * 0.5f, CVec3( 1.0f, 0, 0 ) );
				SHMatrix mEffectPos;
				AI2Vis( &vEffectPos );
				MakeMatrix( &mEffectPos, vEffectPos, qEffectRot );
				PlayComplexEffect( OBJECT_ID_FORGET, level.pDamageEffectSmoke, stats.time, mEffectPos );
			}
		}
		
		if ( bAlive /* && nStage > nOldModelStage */ )
		{
			//for rotating effect up
			const NDb::SEffect *pEffect = level.pDamageEffectWindow != 0 ? level.pDamageEffectWindow->GetSceneEffect() : 0;
			for ( int i = 0; i < pStats->slots.size(); ++i )		
			{
				int nDamageRandom = NWin32Random::Random(100);
				/*if ( nDamageRandom < WINDOW_BREAK_PROBABILITY )				// Break random windows
					BreakWindow( i, eSeason );*/
				if ( pEffect && nDamageRandom < DAMAGE_EFFECT_PROBABILITY )
					AttachEffectToSlot( i, ESSOT_EXHAUST, pEffect, stats.time, true, true );
			}
			RunDefaultObjectAnimation( GetModelDesc()->pSkeleton, Scene()->GetAnimator(GetID()) );
		}
		if ( !bAlive )
		{
			// Remove all attached objects
			RemoveSubObjectsFromSlot( -1, ESSOT_EXHAUST );
			RemoveSubObjectsFromSlot( -1, ESSOT_WEAPON );
			RemoveSubObjectsFromSlot( -1, ESSOT_LIGHT );
			// remove attached sounds
			Singleton<ISoundScene>()->RemoveSoundFromMap( wAmbientSound );
			Singleton<ISoundScene>()->RemoveSoundFromMap( wCycledSound );
			//Singleton<ISoundScene>()->RemoveSoundFromMap( wAmbientSoundTimed );
			Singleton<ISoundScene>()->RemoveSoundFromMap( wCycledSoundTimed );
			wAmbientSound = 0;
			wCycledSound = 0;
			//wAmbientSoundTimed = 0;
			wCycledSoundTimed = 0;

			DisableIcons( true );

			const NDb::SModel *pDeadModel = GetModel( level.pVisObj, eSeason );
			AddDynamicDebris( pDeadModel, vPos, qRot, eSeason );
			const NDb::SVisObj *pVOPreDeath = pStats->pvisualObject;
			float fPreDeathHP = 1.0f;
			for ( int i = 0; i < pStats->damageLevels.size(); ++i )
			{
				if ( pStats->damageLevels[i].fDamageHP > 0 && pStats->damageLevels[i].fDamageHP < fPreDeathHP )
				{
					fPreDeathHP = pStats->damageLevels[i].fDamageHP;
					pVOPreDeath = pStats->damageLevels[i].pVisObj;
				}
			}
			const NDb::SModel *pModel = GetModel( pVOPreDeath, eSeason );
			CDBPtr<NDb::SSkeleton> pSkeleton = pModel->pSkeleton;
			for ( int i = 0; i < pSkeleton->animations.size(); ++i )
			{
				CDBPtr<NDb::SAnimB2> pAnim = checked_cast<const NDb::SAnimB2*>( pSkeleton->animations[i].GetPtr() );
				if ( pAnim->eType == NDb::ANIMATION_DEATH )
				{
					int nRuinsID = Scene()->AddObject( OBJECT_ID_GENERATE, pModel, vPos, qRot, vScale, OBJ_ANIM_MODE_FORCE_ANIMATED, 0 );
					NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nRuinsID );
					AddAnimation( pAnim, stats.time, pAnimator );
					CDeadHouseAnimation *pHolder = new CDeadHouseAnimation;
					pHolder->Init( nRuinsID, stats.time, pAnim );
					return pHolder;
				}
			}
		}
		nOldModelStage = nStage;
	}
	
	return 0;
}

IClientUpdatableProcess* CMOBuilding::AIUpdateDamage( int nProjectileID, float fDamage, const std::list<int> &probableHitAttached, struct IScene *pScene, NDb::ESeason eSeason, bool bFromAIUpdate )
{
	if ( projectilesAlreadyHit.find( nProjectileID ) != projectilesAlreadyHit.end() )
		return 0;
	if ( !bFromAIUpdate && !IsVisible() )
		return 0;

	projectilesAlreadyHit.insert( nProjectileID );
	
	int nSlot = -1;
	for ( std::list<int>::const_iterator iter = probableHitAttached.begin(); iter != probableHitAttached.end() && nSlot == -1; ++iter )
	{
		const int nHitObj = *iter;
		for ( int i = 0; i < attachedObjects.size() && nSlot == -1; ++i )
		{
			CAttachedObjIDs &attaches = attachedObjects[i];
			for ( CAttachedObjIDs::iterator attachIter = attaches.begin(); attachIter != attaches.end() && nSlot == -1; ++attachIter )
			{
				std::pair< int, int > &attach = *attachIter;
				if ( attach.second == nHitObj )
					nSlot = attach.first;
			}
		}
	}

	float fTime = Singleton<IGameTimer>()->GetGameTime();
	
	fBuildingHP = (std::max)( fBuildingHP - fDamage, 0.0f );
	const bool bAlive = fBuildingHP > 0;
	CPtr<CDeadHouseAnimations> pHolder = new CDeadHouseAnimations();
	// наносим повреждения для секций
	if ( fMaxDistance > 0.1f && !GetStats()->slots.empty() )
	{
		nSlot = NWin32Random::Random( GetStats()->slots.size() );
		
		const CVec3 vPoint = GetStats()->slots[nSlot].vDamageCenter;
		for ( int i = 0; i < GetStats()->slots.size(); ++i )
		{
			if ( attachedObjectsHP[i] > 0.0f && ( attachedWindows[i] == EWS_DAY || attachedWindows[i] == EWS_NIGHT ) )
			{
				const float fDist2 = fabs2( vPoint - GetStats()->slots[i].vDamageCenter );
				const float fSlotDamage = fDamage * ( 1 - fDist2/fMaxDistance*( 1 - GetStats()->fDamageCoeff ) );
				const int nOldIndex = GetSlotHPIndex( i, attachedWindows[i] );
				attachedObjectsHP[i] = ( bAlive ) ? (std::max)( 0.0f, attachedObjectsHP[i] - fSlotDamage ) : 0.0f;
				const int nNewIndex = GetSlotHPIndex( i, attachedWindows[i] );
				if ( !bAlive )
				{
					RemoveSubObjectsFromSlot( i, ESSOT_LIGHT );

					const int nWindowID = AttachWindow( i, eSeason, attachedWindows[i] );
//					const int nWindowID = BreakWindow( i, eSeason );

					const CDBPtr<NDb::SModel> pModel = GetModel( GetWindowObj( i, attachedWindows[i] ), eSeason );
					CDBPtr<NDb::SSkeleton> pSkeleton = pModel->pSkeleton;
					const int nDelta = pSkeleton->animations.size() - nOldIndex - 2;
					if ( nDelta < pSkeleton->animations.size() && nDelta >= 0 )
					{
						CDBPtr<NDb::SAnimB2> pAnim = checked_cast<const NDb::SAnimB2*>( pSkeleton->animations[nDelta].GetPtr() );
						if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nWindowID ) )
							AddAnimation( pAnim, fTime, pAnimator );
						if ( !bAlive )
							pHolder->Add( nWindowID, fTime, pAnim );
					}
					else if ( !bAlive )
						RemoveSubObjectsFromSlot( i, ESSOT_WINDOW );
				}
				else if ( nNewIndex != nOldIndex )
				{
					RemoveSubObjectsFromSlot( i, ESSOT_LIGHT );
					const std::vector<NDb::SSlotDamageLevel> &levels = ( attachedWindows[i] == EWS_DAY ) ? GetStats()->slots[i].window.dayDamageLevels : GetStats()->slots[i].window.nightDamageLevels;
					const NDb::SComplexEffect *pEffect = levels[nNewIndex].pDamageEffect;

					DetachWindow( i, eSeason );
					const int nWindowID = AttachWindow( i, eSeason, attachedWindows[i] );

					const CDBPtr<NDb::SModel> pModel = GetModel( GetWindowObj( i, attachedWindows[i] ), eSeason );
					CDBPtr<NDb::SSkeleton> pSkeleton = pModel->pSkeleton;
					const int nDelta = nNewIndex - nOldIndex - 1;
					if ( nDelta < pSkeleton->animations.size() )
					{
						CDBPtr<NDb::SAnimB2> pAnim = checked_cast<const NDb::SAnimB2*>( pSkeleton->animations[nDelta].GetPtr() );
						if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nWindowID ) )
							AddAnimation( pAnim, GameTimer()->GetGameTime(), pAnimator, false );
					}

					if ( pEffect )
						AttachComplexEffectToSlot( i, ESSOT_WINDOW, pEffect, Singleton<IGameTimer>()->GetGameTime(), false );
				}
			}
			else if ( !bAlive && ( attachedWindows[i] == EWS_DAY || attachedWindows[i] == EWS_NIGHT ) )
				RemoveSubObjectsFromSlot( i, ESSOT_WINDOW );
		}
	}

	// наносим повреждения для всего здания
	SAINotifyRPGStats stats;
	stats.fHitPoints = fBuildingHP;
	stats.fFuel = 0.0f;
	stats.nSupply = 0;
	stats.time = fTime;
	
	const int nStage = CommonUpdateHP( fBuildingHP / GetStats()->fMaxHP, stats, pScene, eSeason );
	if ( nStage >= 0 )
	{
		CDBPtr<NDb::SBuildingRPGStats> pStats = GetStats();
		const NDb::SHPObjectRPGStats::SDamageLevel &level = pStats->damageLevels[nStage];
		CVec3 vPos, vScale;
		CQuat qRot;
		GetPlacement( &vPos, &qRot, &vScale );
		if ( level.pDamageEffectSmoke != 0 )
		{
			for ( int i = 0; i < pStats->smokePoints.size(); ++i )
			{
				const NDb::SBuildingRPGStats::SFirePoint &point = pStats->smokePoints[i];
				CVec3 vEffectPos;
				const CVec3 vPointPos( point.vPos.y - pStats->vOrigin.x, point.vPos.x - pStats->vOrigin.y, point.vPos.z );
				qRot.Rotate( &vEffectPos, vPointPos ); 
				vEffectPos += vPos;
				const float fAngle = AI2VisRad(point.fDirection) + FP_PI;
				CQuat qEffectRot( fAngle, CVec3( 0, 0, 1.0f ) );
				qEffectRot = qRot * qEffectRot * CQuat( FP_PI * 0.5f, CVec3( 1.0f, 0, 0 ) );
				SHMatrix mEffectPos;
				AI2Vis( &vEffectPos );
				MakeMatrix( &mEffectPos, vEffectPos, qEffectRot );
				PlayComplexEffect( OBJECT_ID_FORGET, level.pDamageEffectSmoke, stats.time, mEffectPos );
			}
		}
		if ( bAlive )
		{
			//for rotating effect up
			const NDb::SEffect *pEffect = level.pDamageEffectWindow != 0 ? level.pDamageEffectWindow->GetSceneEffect() : 0;
			for ( int i = 0; i < pStats->slots.size(); ++i )		
			{
				int nDamageRandom = NWin32Random::Random(100);
				if ( pEffect && nDamageRandom < DAMAGE_EFFECT_PROBABILITY )
					AttachEffectToSlot( i, ESSOT_EXHAUST, pEffect, stats.time, true, true );
			}
		}
		if ( !bAlive )
		{
			// Remove all attached objects
			RemoveSubObjectsFromSlot( -1, ESSOT_EXHAUST );
			RemoveSubObjectsFromSlot( -1, ESSOT_WEAPON );
			RemoveSubObjectsFromSlot( -1, ESSOT_LIGHT );
			// remove attached sounds
			Singleton<ISoundScene>()->RemoveSoundFromMap( wAmbientSound );
			Singleton<ISoundScene>()->RemoveSoundFromMap( wCycledSound );
			//Singleton<ISoundScene>()->RemoveSoundFromMap( wAmbientSoundTimed );
			Singleton<ISoundScene>()->RemoveSoundFromMap( wCycledSoundTimed );
			wAmbientSound = 0;
			wCycledSound = 0;
			//wAmbientSoundTimed = 0;
			wCycledSoundTimed = 0;

			DisableIcons( true );

			const NDb::SModel *pDeadModel = GetModel( level.pVisObj, eSeason );
			AddDynamicDebris( pDeadModel, vPos, qRot, eSeason );
			const NDb::SVisObj *pVOPreDeath = pStats->pvisualObject;
			float fPreDeathHP = 1.0f;
			for ( int i = 0; i < pStats->damageLevels.size(); ++i )
			{
				if ( pStats->damageLevels[i].fDamageHP > 0 && pStats->damageLevels[i].fDamageHP < fPreDeathHP )
				{
					fPreDeathHP = pStats->damageLevels[i].fDamageHP;
					pVOPreDeath = pStats->damageLevels[i].pVisObj;
				}
			}
			const NDb::SModel *pModel = GetModel( pVOPreDeath, eSeason );
			CDBPtr<NDb::SSkeleton> pSkeleton = pModel->pSkeleton;
			for ( int i = 0; i < pSkeleton->animations.size(); ++i )
			{
				CDBPtr<NDb::SAnimB2> pAnim = checked_cast<const NDb::SAnimB2*>( pSkeleton->animations[i].GetPtr() );
				if ( pAnim->eType == NDb::ANIMATION_DEATH )
				{
					int nRuinsID = Scene()->AddObject( OBJECT_ID_GENERATE, pModel, vPos, qRot, vScale, OBJ_ANIM_MODE_FORCE_ANIMATED, 0 );
					if ( NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( nRuinsID ) )
						AddAnimation( pAnim, stats.time, pAnimator );
					pHolder->Add( nRuinsID, stats.time, pAnim );
					break;
				}
			}
		}
	}
	return ( pHolder->IsEmpty() ) ? 0 : pHolder.Extract();
}

void CMOBuilding::AIUpdateKeyObject( const struct SAINotifyKeyBuilding &update )
{
	IMOContainer::AIUpdateKeyObject( update );
	
	bStorage = update.bStorage;
}

void CMOBuilding::AIUpdateKeyObjectCaptureProgress( float fProgress, int nColorIndex )
{
	nCapturingColorIndex = nColorIndex;
	fCapturingProgress = fProgress;

	UpdateIcons();
}

void CMOBuilding::AddDynamicDebris( const NDb::SModel *pDeadModel, const CVec3 &vPos, const CQuat &qRot, NDb::ESeason eSeason )
{
	CDBPtr<NDb::SBuildingRPGStats> pStats = GetStats();
	const CVec2 vFlatPos( vPos.x, vPos.y );
	const CVec2 vFlatSize( pDeadModel->pGeometry->vSize.x * VIS_TO_AI, pDeadModel->pGeometry->vSize.y * VIS_TO_AI );
	float fAngle;
	CVec3 vZAxis;
	qRot.DecompAngleAxis( &fAngle, &vZAxis );
	for ( int i = 0; i < pStats->dynamicDebris.masks.size(); ++i )
	{
		const NDb::SObjectBaseRPGStats::SDynamicDebris::SDynamicMaskDesc &desc = pStats->dynamicDebris.masks[i];
		if ( desc.eSeason == eSeason )
		{
			Scene()->AddDebris( vFlatSize, vFlatPos, fAngle, desc.fWidth, desc.pMaterial );
			break;
		}
	}
}

void CMOBuilding::Select( bool bSelect )
{
#define SHOW_UNIT_SELECTION
#ifdef SHOW_UNIT_SELECTION
	if ( bSelect )
		Scene()->SelectObject( GetID(), GetCenter(), GetSelectionScale(), GetSelectionType() );
	else
		Scene()->UnselectObject( GetID() );
#endif
	//
	CMOSelectable::Select( bSelect );

  UpdateIcons();
}

void CMOBuilding::SetCanSelect( bool bCanSelect )
{
	CMOSelectable::SetCanSelect( bCanSelect );
	
//	DisableIcons( !bCanSelect );
}

const NDb::SBuildingRPGStats* CMOBuilding::GetStats() const
{
	return checked_cast<const NDb::SBuildingRPGStats*>( IMOContainer::GetStats() );
}

void CMOBuilding::FillIconsInfo( SSceneObjIconInfo &iconInfo )
{
	DisableIcons( false );//GetPassangersCount() == 0 );
	
	SetIconsHitbar( IsVisible() && IsHitbarVisible(), IsSelected() );
	SetIconsGroup( IsVisible() ? GetSelectionGroup() : -1, IsSelected() );
	
	IMOContainer::FillIconsInfo( iconInfo );

	iconInfo.fHPBarAdditionalValue = fCapturingProgress;
	iconInfo.nHPBarAdditionalColorIndex = nCapturingColorIndex;
}

const int CMOBuilding::GetSlotHPIndex( const int nWindow, const EWindowState eState ) const
{
	if ( eState != EWS_DESTROYED )  
	{
		const float fHPRatio = attachedObjectsHP[nWindow] / GetStats()->slots[nWindow].window.fMaxHP;
		const std::vector<NDb::SSlotDamageLevel> &levels = ( eState == EWS_DAY ) ? GetStats()->slots[nWindow].window.dayDamageLevels : GetStats()->slots[nWindow].window.nightDamageLevels;
		if ( levels.empty() || levels.front().fDamageHP < fHPRatio )
			return -1;
		if ( fHPRatio <= levels.back().fDamageHP )
			return levels.size() - 1;

		for ( int i = levels.size()-2; i >= 0; --i )
			if ( fHPRatio <= levels[i].fDamageHP )
				return i;
	}

  return -1;
}

const NDb::SVisObj* CMOBuilding::GetWindowObj( const int nWindow, const EWindowState eState )
{
	switch( eState ) {
	case EWS_DESTROYED:
		return GetStats()->slots[nWindow].window.pDestroyedObj;
		break;
	case EWS_DAY:
		{
			const int nHPIndex = GetSlotHPIndex( nWindow, eState );
			if ( nHPIndex == -1 )
				return GetStats()->slots[nWindow].window.pDayObj;
			else
				return GetStats()->slots[nWindow].window.dayDamageLevels[nHPIndex].pVisObj;
		}
		break;
	case EWS_NIGHT:
		{
			const int nHPIndex = GetSlotHPIndex( nWindow, eState );
			if ( nHPIndex == -1 )
				return GetStats()->slots[nWindow].window.pNightObj;
			else
				return GetStats()->slots[nWindow].window.nightDamageLevels[nHPIndex].pVisObj;
		}
		break;
	default:
		return 0;
	}
}

void CMOBuilding::DetachWindow( const int nWindow, const NDb::ESeason eSeason )
{
	if ( attachedWindows[nWindow] != EWS_NONE )
	{
		RemoveSubObjectsFromSlot( nWindow, ESSOT_WINDOW );
	}
}

const int CMOBuilding::AttachWindow( const int nWindow, const NDb::ESeason eSeason, const EWindowState eState )
{
	int nResult = -1;
	const NDb::SModel *pWindowModel = GetModel( GetWindowObj( nWindow, eState ), eSeason );
	if ( pWindowModel )
	{
		nResult = AttachSubObjectToSlot( nWindow, ESSOT_WINDOW, pWindowModel );

		if ( eState == EWS_DESTROYED && attachedWindows[nWindow] != EWS_DESTROYED )
		{
			const NDb::SComplexEffect *pEffect = GetStats()->slots[nWindow].window.pDestroyEffect;
			if ( pEffect )
			{
				AttachComplexEffectToSlot( nWindow, ESSOT_WINDOW, pEffect, Singleton<IGameTimer>()->GetGameTime(), true );
			}
		}
	}

	attachedWindows[nWindow] = eState;
	return nResult;
}

const int CMOBuilding::BreakWindow( const int nWindow, const NDb::ESeason eSeason )
{
	const std::vector<NDb::SBuildingRPGStats::SSlot> &slots = GetStats()->slots;
	NI_ASSERT( nWindow < slots.size(), StrFmt( "Wrong number of window (%d), building %s, total slots %d", nWindow, GetStats()->GetDBID().ToString().c_str(), slots.size() ) );

	if ( nWindow < slots.size() && attachedWindows[nWindow] != EWS_DESTROYED )
	{
		DetachWindow( nWindow, eSeason );
		return AttachWindow( nWindow, eSeason, EWS_DESTROYED );
	}
	return -1;
}

void CMOBuilding::BreakAllWindows( const NDb::ESeason eSeason )
{
	RemoveSubObjectsFromSlot( -1, ESSOT_WINDOW );

	for ( int i = 0; i < GetStats()->slots.size(); ++i )
		AttachWindow( i, eSeason, EWS_DESTROYED );
}

void CMOBuilding::ToggleNightWindows( const bool bNightOn, const NDb::ESeason eSeason )
{
	Scene()->RemoveAllAttached( GetID(), ESSOT_WINDOW );
	float fProb = NGlobal::GetVar( "night_window_probability", 0.3f ).GetFloat();
	const int nWindowProb = 100 * fProb;
	for ( int i = 0; i < GetStats()->slots.size(); ++i )
	{
		if ( attachedWindows[i] == EWS_DESTROYED )
			AttachWindow( i, eSeason, EWS_DESTROYED );
		else
		{
			int nWindowRandom = NWin32Random::Random(100);
			AttachWindow( i, eSeason, ( bNightOn && nWindowRandom < nWindowProb ) ? EWS_NIGHT : EWS_DAY );
		}
	}
}

void CMOBuilding::AIUpdateShot( const struct SAINotifyBaseShot &shot, const NTimer::STime &currTime, IScene *pScene, NDb::ESeason eSeason )
{
	CDBPtr<NDb::SBuildingRPGStats> pStats = GetStats();
	const SAINotifyInfantryShot *pShotInfo = static_cast<const SAINotifyInfantryShot*>( &shot );

	const int nSlot = pStats->aiSlotToSlot[pShotInfo->nSlot];

	const NDb::SBuildingRPGStats::SSlot &slot = pStats->slots[nSlot];

	const NDb::SComplexEffect *pEffect = pShotInfo->pWeapon->shells[pShotInfo->cShell].pEffectGunFire;
	if ( pEffect != 0 )
	{
		AttachComplexEffectToSlot( nSlot, ESSOT_GUN_FIRE, pEffect, pShotInfo->time, true );
	}
}

int CMOBuilding::operator&( IBinSaver &saver )
{
	saver.Add( 1, checked_cast<IMOContainer*>(this) );
	saver.Add( 3, &vPassangers );
	saver.Add( 5, &attachedWindows );
	saver.Add( 6, &attachedObjects );
	saver.Add( 7, &attachedObjectsHP );
	saver.Add( 8, &fMaxDistance );
	saver.Add( 9, &wAmbientSound );
	saver.Add( 10, &wCycledSound );
	saver.Add( 11, &fBuildingHP );
	saver.Add( 12, &bStorage );
	saver.Add( 13, &projectilesAlreadyHit );
	//saver.Add( 14, &wAmbientSoundTimed );
	saver.Add( 15, &wCycledSoundTimed );
	saver.Add( 16, &nCurrentAmmo );
	saver.Add( 17, &fCapturingProgress );
	saver.Add( 18, &nCapturingColorIndex );
	saver.Add( 19, &bCanEnter );

	return 0;
}

void CMOBuilding::AttachEffectToSlot( const int nSlot, const ESceneSubObjType eType, const NDb::SEffect *pEffect, const NTimer::STime _time, const bool bTurnWithWind, const bool bRemoveObject )
{
	CVec3 vEffectPos = GetStats()->slots[ nSlot ].vPos;
	AI2Vis( &vEffectPos );
	CQuat qEffectRot = bTurnWithWind ? QNULL : GetStats()->slots[ nSlot ].qRot;
	SHMatrix mEffectPos, mBuildingPos;
	MakeMatrix( &mEffectPos, vEffectPos, qEffectRot );
	vEffectPos = GetCenter();
	AI2Vis( &vEffectPos );
	qEffectRot = GetOrientation();				// Get the building's orientation (need it later)
	MakeMatrix( &mBuildingPos, vEffectPos, qEffectRot );

	SHMatrix mPos;
	if ( bTurnWithWind )				// Use wind data to turn the effect there
	{
		const float fWindDir = Scene()->GetWindController()->GetWindDirection() * PI / 180 + PI;
		const float fWindForceFactor = FP_PI2; // was ( 10 - Scene()->GetWindController()->GetWindIntensity() ) * PI / 20;		// 0-10 force is 0-90 degrees
		CQuat qWindRot1( fWindDir, CVec3( 0, 0, 1 ) );
		CQuat qWindRot2( fWindForceFactor, CVec3( 1, 0, 0 ) );			// Around 0X?
		qEffectRot.Inverse();
		SHMatrix mWindTurn( VNULL3, qEffectRot * qWindRot1 * qWindRot2 );				// Compensate for building orientation

		mPos = mBuildingPos * mEffectPos * mWindTurn;
	}
	else
		mPos = mBuildingPos * mEffectPos;

	if ( bRemoveObject )
	{
		RemoveSubObjectsFromSlot( nSlot, eType );

		CAttachedObjIDs &attaches = attachedObjects[eType];
		attaches.emplace_front();
		CAttachedObjIDs::iterator itAttach = attaches.begin();
		itAttach->first = nSlot;
		itAttach->second = Scene()->AddEffect( OBJECT_ID_GENERATE, pEffect, _time, mPos );
	}
	else
		Scene()->AddEffect( OBJECT_ID_GENERATE, pEffect, _time, mPos );
}

void CMOBuilding::AttachComplexEffectToSlot( const int nSlot, const ESceneSubObjType eType, const NDb::SComplexEffect *pEffect, const NTimer::STime _time, const bool bRemoveObject )
{
	CVec3 vEffectPos = GetStats()->slots[ nSlot ].vPos;
	AI2Vis( &vEffectPos );
	const NTimer::STime currTime = GameTimer()->GetGameTime();
	if ( pEffect->pSoundEffect )
	{
		Vis2AI( &vEffectPos );
		SoundScene()->AddSound( pEffect->pSoundEffect, vEffectPos, SFX_MIX_IF_TIME_EQUALS, SAM_ADD_N_FORGET, currTime > _time ? currTime - _time : 0, 2 );
	}
	if ( const NDb::SEffect *pSceneEffect = pEffect->GetSceneEffect() )
		AttachEffectToSlot( nSlot, eType, pSceneEffect, (std::min)(_time, currTime), false, bRemoveObject );
}

static void MakeSlotPos( const CVec3 &_vBuildingPos, const CQuat &qBuildingOrient, const CVec3 &_vSlotPos, const CQuat &qSlotOrient, const CVec2 &vSlotScale, SHMatrix *pmResult )
{
	CVec3 vSlotPos;
	AI2Vis( &vSlotPos, _vSlotPos );
	SHMatrix mSlotPos;
	MakeMatrix( &mSlotPos, vSlotPos, qSlotOrient );

	CVec3 vBuildingPos;
	AI2Vis( &vBuildingPos, _vBuildingPos);
	SHMatrix mBuildingPos;
	MakeMatrix( &mBuildingPos, vBuildingPos, qBuildingOrient );

	*pmResult = mBuildingPos * mSlotPos;

	SHMatrix mScale;
	Identity( &mScale );
	mScale.xx = vSlotScale.x;
	mScale.yy = vSlotScale.y;

	*pmResult = *pmResult * mScale;
}

const int CMOBuilding::AttachSubObjectToSlot( const int nSlot, const ESceneSubObjType eType, const NDb::SModel *pSubModel )
{
	RemoveSubObjectsFromSlot( nSlot, eType );	
	
	SHMatrix mPos;
	MakeSlotPos( GetCenter(), GetOrientation(), GetStats()->slots[nSlot].vPos, GetStats()->slots[nSlot].qRot, GetStats()->slots[nSlot].vWindowScale, &mPos );
	CAttachedObjIDs &attaches = attachedObjects[eType];

	attaches.emplace_front();
	CAttachedObjIDs::iterator itAttach = attaches.begin();
	itAttach->first = nSlot;
	const int nResult = Scene()->AddObject( OBJECT_ID_GENERATE, pSubModel, mPos, OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC, 0, false );
	Scene()->AddAttachedMapping( nResult, GetID() );

	itAttach->second = nResult;
	return nResult;
}

void CMOBuilding::AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, struct ISoundScene *pSoundScene, NDb::ESeason eSeason )
{
	CAttachedObjIDs &attaches = attachedObjects[ESSOT_WINDOW];
	IMOContainer::AIUpdatePlacement( placement, pScene, pSoundScene, eSeason );
	for ( CAttachedObjIDs::iterator iter = attaches.begin(); iter != attaches.end(); ++iter )
	{
		const int nSlot = iter->first;
		const int nSlotSceneID = iter->second;
		
		SHMatrix mPos;
		MakeSlotPos( GetCenter(), GetOrientation(), GetStats()->slots[nSlot].vPos, GetStats()->slots[nSlot].qRot, GetStats()->slots[nSlot].vWindowScale, &mPos );
		pScene->MoveObject( nSlotSceneID, mPos );
	}
}

void CMOBuilding::RemoveSubObjectsFromSlot( const int nSlot, const ESceneSubObjType eType )
{
	CAttachedObjIDs &attaches = attachedObjects[eType];
	for ( CAttachedObjIDs::iterator it = attaches.begin(); it != attaches.end(); )
	{
		if ( nSlot == -1 || it->first == nSlot )			//Pass -1 as slot number to delete all
		{
			Scene()->ShowObject( it->second, false );
			Scene()->StopEffectGeneration( it->second, GameTimer()->GetGameTime() );

			Scene()->RemoveAttachedMapping( it->second );

			it = attaches.erase( it );
		}
		else
			++it;
	}
}

bool CMOBuilding::IsHitbarVisible() const
{
	return GetPassangersCount() != 0 || IsSelected() || IsMousePicked() || IsKeyObject();
}

REGISTER_SAVELOAD_CLASS( 0x100A7481, CMOBuilding );

