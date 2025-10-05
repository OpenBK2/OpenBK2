#include "stdafx.h"

#include "ClientAckManager.h"
#include "Stats_B2_M1/DBAnimB2.h"
#include "MOUnit.h"
#include "Stats_B2_M1/DBAttachedModelVisObj.h"
#include "3Dmotor/GAnimation.hpp"
#include "Input/Bind.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "Stats_B2_M1/AIAckTypes.h"
#include "Main/GameTimer.h"
#include "Stats_B2_M1/AbilityActions.h"
#include "SceneB2/AttachedObj.h"

#include "System/Commands.h"
#include "MOUnitInfantry.h"

static float s_fPointerOffset = 3.0f;

bool CMOUnit::CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor )
{
	const float fNewHP = pUpdate->info.fHitPoints / GetStats()->fMaxHP;

	const NDb::SVisObj *pVisObj = ChooseVisObjForHP( fNewHP );

	const NDb::SModel *pModel = GetModel( pVisObj, eSeason );
	if ( pModel == 0 ) 
		return false;

	const NDb::SModel *pLowLevelModel = GetLowLevelModel( pVisObj, eSeason );

	const CVec3 vResize = CVec3( pUpdate->info.fResize, pUpdate->info.fResize, pUpdate->info.fResize );
	CVec3 vPos;
	CQuat qRot;
	GetPlacementFromUpdate( &vPos, &qRot, pUpdate );
	if ( pUpdate != 0 )
	{
		MakeOrientation( &qRot, DWORDToVec3( pUpdate->info.dwNormal ) );
		SetPlacement( vPos, qRot );
		SetScale( vResize );
	}
	Scene()->AddObject( nUniqueID, pModel, vPos, qRot, vResize, OBJ_ANIM_MODE_FORCE_ANIMATED, 0, pLowLevelModel );
	SetModel( pModel );
	nOldAreasTime = 0;
	return true;
}

void CMOUnit::GetStatus( SObjectStatus *pStatus ) const
{
	IMOUnit::GetStatus( pStatus );
	
	pStatus->nSupply = nSupply;
	pStatus->fFuel = fFuel;

	if ( GetStatsLocal() )
		pStatus->pArmorPattern = GetStatsLocal()->pArmorPattern;
}

bool CMOUnit::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	const SAINewUnitUpdate *pUpdate = checked_cast<const SAINewUnitUpdate *>( _pUpdate );
	SetStats( checked_cast_ptr<const NDb::SUnitBaseRPGStats*>( pUpdate->info.pStats ) );
	if ( GetStatsLocal()->GetActions() )
	{
		for ( std::vector< CDBPtr< NDb::SUnitSpecialAblityDesc > >::const_iterator it = GetStatsLocal()->GetActions()->specialAbilities.begin(); it != GetStatsLocal()->GetActions()->specialAbilities.end(); ++it )
		{
			if ( (*it) != 0 )
				abilityMap[(*it)->eName] = SAbilityInfo( EASS_READY_TO_ON, 0.0f );
		}

		// Add Entrench button info
		if ( GetStatsLocal()->GetActions()->availUserActions.HasAction( NDb::USER_ACTION_ENTRENCH_SELF ) )
			abilityMap[NDb::ABILITY_ENTRENCH_SELF] = SAbilityInfo( EASS_READY_TO_ON, 0.0f );
	}

	bOpen = true;
	nLevel = 0;
	fFuel = -1.0f;
	bool bCreated = CMapObj::Create( nUniqueID, _pUpdate, eSeason, eDayTime, bInEditor );
	if ( bCreated )
	{
		SetVisible( false, eSeason, ( eDayTime == NDb::DAY_NIGHT ) ); // чтобы изначально не было видно под warfog'ом.
//		Scene()->ShowObject( nUniqueID, IsVisible() );
	}

	nMaxAmmo = 0;
	if ( NDb::IsInfantry( checked_cast<const NDb::SUnitBaseRPGStats*>(GetStats())->etype ) )
	{
		const NDb::SInfantryRPGStats *pS = checked_cast<const NDb::SInfantryRPGStats*>( GetStats() );
		if ( pS )
		{
			for ( int j = 0; j < pS->GetGunsSize( GetID(), 0 ); ++j )
				nMaxAmmo += pS->GetGun( GetID(), 0, j ).nAmmo;
		}
	}
	else
	{
		const NDb::SMechUnitRPGStats *pS = checked_cast<const NDb::SMechUnitRPGStats*>( GetStats() );
		if ( pS )
		{
			for ( int i = 0; i < pS->GetPlatformsSize( GetID() ); ++i )
				for ( int j = 0; j < pS->GetGunsSize( GetID(), i ); ++j )
					nMaxAmmo += pS->GetGun( GetID(), i, j ).nAmmo;
		}
	}

	nCurAmmo = nMaxAmmo;
	bNewAbility = false;
	bPointer = false;
	bShowUnitRank = false;
	return bCreated;
}

void CMOUnit::GetActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( GetStatsLocal() == 0 || GetStatsLocal()->GetActions() == 0 )
		return;
	if ( eActions == ACTIONS_BY )
	{
		*pActions |= GetStatsLocal()->GetActions()->availUserActions;
		for ( int i = 0; i < GetStatsLocal()->GetActions()->specialAbilities.size(); ++i )
		{
			const NDb::SUnitSpecialAblityDesc *pDesc = GetStatsLocal()->GetActions()->specialAbilities[ i ];
			if ( !pDesc )
				continue;
			if ( i <= nLevel )
				pActions->SetAction( GetActionByAbility( pDesc->eName ) );
			else
				pActions->RemoveAction( GetActionByAbility( pDesc->eName ) );
		}
		if ( pActions->HasAction( NDb::USER_ACTION_MOVE ) )
			pActions->SetAction( NDb::USER_ACTION_MOVE_LIKE_TERRAIN );
	}
	else if ( eActions == ACTIONS_WITH ) 
	{
		*pActions |= GetStatsLocal()->GetActions()->availUserExposures;
		pActions->SetAction( NDb::USER_ACTION_EXACT_SHOT );		
		pActions->SetAction( NDb::USER_ACTION_SUPPORT_FIRE );
		if ( !checked_cast<const NDb::SUnitBaseRPGStats*>(GetStats())->IsAviation() )
			pActions->SetAction( NDb::USER_ACTION_DROP_BOMB );
	}
}

void CMOUnit::GetPossibleActions( CUserActions *pActions ) const
{
	if ( GetStatsLocal() == 0 || GetStatsLocal()->GetActions() == 0 )
		return;

	*pActions |= GetStatsLocal()->GetActions()->availUserActions;
	for ( int i = 0; i < GetStatsLocal()->GetActions()->specialAbilities.size(); ++i )
	{
		const NDb::SUnitSpecialAblityDesc *pDesc = GetStatsLocal()->GetActions()->specialAbilities[ i ];
		if ( !pDesc )
			continue;
		pActions->SetAction( GetActionByAbility( pDesc->eName ) );
	}
	if ( pActions->HasAction( NDb::USER_ACTION_MOVE ) )
		pActions->SetAction( NDb::USER_ACTION_MOVE_LIKE_TERRAIN );
}

void CMOUnit::GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const
{
}

IClientUpdatableProcess* CMOUnit::AIUpdateRPGStats( const SAINotifyRPGStats &stats, struct IClientAckManager *pAckManager, NDb::ESeason eSeason )
{
	// Ammo
	nSupply = stats.nSupply;

	ammos.clear();
	int nNewCurAmmo = 0;
	for ( std::vector<SAINotifyRPGStats::SWeaponAmmo>::const_iterator it = stats.ammo.begin(); it != stats.ammo.end(); ++it )
	{
		const SAINotifyRPGStats::SWeaponAmmo &updateAmmo = *it;
		nNewCurAmmo += updateAmmo.nAmmo;
		std::vector<SAmmo>::iterator iAmmo = find_if( ammos.begin(), ammos.end(), SAmmoCompare( updateAmmo.pStats ) );
		if ( iAmmo == ammos.end() )
		{
			SAmmo ammo;
			ammo.pWeapon = updateAmmo.pStats;
			ammo.nWeaponCount = 1;
			ammo.nAmmo = updateAmmo.nAmmo;
			ammos.push_back( ammo );
		}
		else
		{
			SAmmo &ammo = *iAmmo;
			++ammo.nWeaponCount;
			ammo.nAmmo += updateAmmo.nAmmo;
		}
	}
	
	if ( pAckManager && nNewCurAmmo < nMaxAmmo / 3 && nCurAmmo >= nMaxAmmo / 3 )
	{
		if ( NGlobal::GetVar( "m1", 0 ).GetFloat() == 0 )
			pAckManager->AddAcknowledgement( this, NDb::ACK_LOW_AMMO, checked_cast<const NDb::SUnitBaseRPGStats*>(GetStats())->ChooseAcknowledgement( NDb::ACK_LOW_AMMO, 0 ), 0, 0 );
	}

	nCurAmmo = nNewCurAmmo;
	const float fNewHP = stats.fHitPoints / GetStatsLocal()->fMaxHP;
	CommonUpdateHP( fNewHP, stats, Scene(), eSeason );
	fFuel = stats.fFuel;
	return 0;
}

#define MIN_DIFFERENCE 0.1f
bool CMOUnit::AIUpdateSpecialAbility( const struct SAISpecialAbilityUpdate &update )
{
	NDb::EUnitSpecialAbility eAbility = static_cast<NDb::EUnitSpecialAbility>(update.info.nAbilityType);
  CAbilityInfo::iterator pos = abilityMap.find( eAbility );
	if ( pos == abilityMap.end() )
		return false;
	SAbilityInfo &info = pos->second;

	bool bResult = true;
	if ( fabs( info.fParam = update.info.fCurValue ) < MIN_DIFFERENCE && info.abilityState.dwStateValue == update.info.state.dwStateValue )
		bResult = false;
	else
		info.fParam = update.info.fCurValue;

	info.abilityState.dwStateValue = update.info.state.dwStateValue;

	return bResult;
}

int CMOUnit::GetAbilityTier( NDb::EUserAction eAction ) const
{
	if ( GetStatsLocal() && GetStatsLocal()->GetActions() )
	{
		for ( int i = 0; i < GetStatsLocal()->GetActions()->specialAbilities.size(); ++i )
		{
			const NDb::SUnitSpecialAblityDesc *pDesc = GetStatsLocal()->GetActions()->specialAbilities[i];
			if ( pDesc && GetActionByAbility( pDesc->eName ) == eAction )
				return i;
		}
	}
	return -1;
}

void CMOUnit::GetAbilityInfo( CAbilityInfo &abilityList ) const
{
	abilityList.clear();
	for ( CAbilityInfo::const_iterator it = abilityMap.begin(); it != abilityMap.end(); ++it )
	{
		NDb::EUnitSpecialAbility eAbility = it->first;
		const SAbilityInfo &ability = it->second;
		abilityList[eAbility] = ability;
	}
}

void CMOUnit::SetAnimation( const NDb::SAnimB2 *pAnimation, const NTimer::STime startTime )
{
	if ( pAnimation == 0 ) 
		return;
	NAnimation::ISkeletonAnimator *pAnimator = Scene()->GetAnimator( GetID() );
	if ( pAnimator != 0 )
	{
		if ( HasLoopedAnimation() )
		{
			pAnimator->ClearAllAnimations();
			SetLoopedAnimation( false );
		}
		//
		SetLoopedAnimation( pAnimation->bLooped );
		AddAnimation( pAnimation, startTime, pAnimator );
	}
}

int CMOUnit::GetWeaponAmmo( const NDb::SWeaponRPGStats *pWeapon ) const
{
	std::vector<SAmmo>::const_iterator iAmmo = find_if( ammos.begin(), ammos.end(), SAmmoCompare( pWeapon ) );
	NI_VERIFY( iAmmo != ammos.end(), "Wrong weapon", return 0 );
	return iAmmo->nAmmo;
}

int CMOUnit::GetWeaponAmmoTotal() const
{
	int nAmmo = 0;
	for ( std::vector<SAmmo>::const_iterator it = ammos.begin(); it != ammos.end(); ++it )
	{
		const SAmmo &ammo = *it;
		nAmmo += ammo.nAmmo;
	}
	return nAmmo;
}

void CMOUnit::FillIconsInfo( SSceneObjIconInfo &iconInfo )
{
	IMOUnit::FillIconsInfo( iconInfo );

	// Display "new ability"
	if ( IsFriend() )
	{
		if ( !bShowUnitRank )
		{
			if ( bNewAbility )
				iconInfo.eIconLevelup = NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NEW_ABILITY;
		}
		else
		{
			switch ( nLevel )
			{
				case 1:
				{
					iconInfo.eIconLevelup = NDb::SVisObjIconsSet::SVisObjIcon::VOIT_ABILITY_RANK_2;
					break;
				}

				case 2:
				{
					iconInfo.eIconLevelup = NDb::SVisObjIconsSet::SVisObjIcon::VOIT_ABILITY_RANK_3;
					break;
				}

				case 3:
				{
					iconInfo.eIconLevelup = NDb::SVisObjIconsSet::SVisObjIcon::VOIT_ABILITY_RANK_4;
					break;
				}
			}
		}
	}
}

bool CMOUnit::NeedShowInterrior() const
{
	return false;
}

void CMOUnit::AIUpdateAcknowledgement( const NDb::EUnitAckType eAck, struct IClientAckManager *pAckManager, const int nSet )
{
	const NDb::SUnitBaseRPGStats *pStats = checked_cast<const NDb::SUnitBaseRPGStats*>( GetStats() );
	pAckManager->AddAcknowledgement( this, eAck, pStats->ChooseAcknowledgement( eAck, nSet ), nSet, 0 );
}

void CMOUnit::AIUpdateBoredAcknowledgement( const struct SAIBoredAcknowledgement &ack, struct IClientAckManager *pAckManager )
{
	if ( ack.bPresent )
		pAckManager->RegisterAsBored( NDb::EUnitAckType( ack.nAck ), this );
	else
		pAckManager->UnRegisterAsBored( NDb::EUnitAckType( ack.nAck ), this );
}

void CMOUnit::SendAcknowledgement( struct IClientAckManager *pAckManager, const NDb::EUnitAckType eAck )
{
	const NDb::SUnitBaseRPGStats *pStats = checked_cast<const NDb::SUnitBaseRPGStats*>( GetStats() );
	pAckManager->AddAcknowledgement( this, eAck, pStats->ChooseAcknowledgement( eAck, 0 ), 0, 0 );
}

void CMOUnit::AIUpdateDeadUnit( const SAIDeadUnitUpdate *pUpdate, const NDb::ESeason eSeason, const bool bIsNight, ISoundScene *pSoundScene, IClientAckManager *pAckManager )
{
	const NDb::SUnitBaseRPGStats *pStats = checked_cast<const NDb::SUnitBaseRPGStats*>( GetStats() );
	if ( pUpdate->bVisibleWhenDie )
	{
		pAckManager->UnitDead( this, pSoundScene );
		pAckManager->AddDeathAcknowledgement( GetCenter(),  pStats->ChooseAcknowledgement( NDb::ACK_UNIT_DIED, 0 ), 0 );
	}
	Scene()->ClearMarkers( ESMT_SHOOT_AREA, GetID() );
	DisableIcons( true );
	SetDeathState();
}

void CMOUnit::AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, struct ISoundScene *pSoundScene, IClientAckManager *pAckManager )
{
	if ( pUpdate->bShowEffects )
		pAckManager->UnitDead( this, pSoundScene );
	DisableIcons( true );
	SetDeathState();
}

void CMOUnit::AIUpdateShootAreas( const SAIShootAreaUpdate *pUpdate )
{
	if ( pUpdate->nUpdateTime - nOldAreasTime < 1000 && !AreasChanged( pUpdate->info ) && !Singleton<IGameTimer>()->HasPause( 0 ) )
		return;
	nOldAreasTime = pUpdate->nUpdateTime;
	CopyAreas( pUpdate->info );
	Scene()->ClearMarkers( ESMT_SHOOT_AREA, GetID() );
	for ( int i = 0; i < pUpdate->info.size(); ++i )
	{
		const SShootAreas &areas = pUpdate->info[i];
		for ( std::list<SShootArea>::const_iterator it = areas.areas.begin(); it != areas.areas.end(); ++it )
		{
			const SShootArea &area = *it;
			DWORD dwColor = area.GetColor();
			CVec3 vColor( ( dwColor & 0x00ff0000 ) >> 16, ( dwColor & 0x0000ff00 ) >> 8, dwColor & 0x000000ff );
			vColor = vColor / 256.0f;
			CVec2 vCenter( area.vCenter3D.x, area.vCenter3D.y );
			const float fStartAngle = AI2VisRad(area.wStartAngle) + FP_PI * 0.5;
			const float fEndAngle = AI2VisRad(area.wFinishAngle) + FP_PI * 0.5;
			Scene()->AddShootArea( GetID(), fStartAngle, fEndAngle, area.fMinR, area.fMaxR, vColor, vCenter );
		}
	}
}

void CMOUnit::CopyAreas( const std::vector<SShootAreas> &newAreas )
{
	oldAreas.resize( newAreas.size() );
	for ( int i = 0; i < newAreas.size(); ++i )
	{
		oldAreas[i].areas.clear();
		for ( std::list<SShootArea>::const_iterator it = newAreas[i].areas.begin(); it != newAreas[i].areas.end(); ++it )
			oldAreas[i].areas.push_back( *it );
	}
}

bool CMOUnit::AreasChanged( const std::vector<SShootAreas> &newAreas ) const
{
	if ( oldAreas.size() != newAreas.size() )
		return true;
	for ( int i = 0; i < newAreas.size(); ++i )
	{
		std::list<SShootArea>::const_iterator itOld = oldAreas[i].areas.begin();
		std::list<SShootArea>::const_iterator itNew = newAreas[i].areas.begin();
		while ( itOld != oldAreas[i].areas.end() && itNew != newAreas[i].areas.end() )
		{
			if ( *itOld != *itNew )
				break;
			++itOld;
			++itNew;
		}
		if ( itOld != oldAreas[i].areas.end() || itNew != newAreas[i].areas.end() )
			return true;
	}
	return false;
}

void CMOUnit::AIUpdateModifyEntranceState( bool _bOpen )
{
	bOpen = _bOpen;
}

void CMOUnit::SetUnitLevel( int _nLevel, bool _bShowUnitRank )
{
	nLevel = _nLevel;
	bShowUnitRank = true;//_bShowUnitRank;
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "debug_unit_levels", 0 ) != 0 )
		nLevel = 3;
#endif _FINALRELEASE
}

void CMOUnit::SetNewAbility( bool _bNewAbility )
{
	bNewAbility = _bNewAbility;
}

struct SChooseAttachedToDamaged : public CMOUnit::IChooseAttached
{
	OBJECT_NOCOPY_METHODS( SChooseAttachedToDamaged );
public:
	const int nDamageLevel;

	SChooseAttachedToDamaged() : nDamageLevel( 0 ) { }
	SChooseAttachedToDamaged( const int _nDamageLevel ) : nDamageLevel( _nDamageLevel ) { }

	virtual const NDb::SVisObj* Choose( const NDb::SAttachedModelVisObj *pAttachedVisObj )
	{
		const std::vector<NDb::SAttachedModelVisObj::SSDamageLevel> &damageLevels = pAttachedVisObj->damageLevels;
		return damageLevels.size() > nDamageLevel && damageLevels[nDamageLevel].pVisObj ?
					 damageLevels[nDamageLevel].pVisObj : 0;
	}
};

struct SChooseAttachedToAnimable : public CMOUnit::IChooseAttached
{
	OBJECT_NOCOPY_METHODS( SChooseAttachedToAnimable );
public:
	virtual const NDb::SVisObj* Choose( const NDb::SAttachedModelVisObj *pAttachedVisObj )
	{
		return pAttachedVisObj->pAnimableModel;
	}
};

struct SChooseAttachedToTransportable : public CMOUnit::IChooseAttached
{
	OBJECT_NOCOPY_METHODS( SChooseAttachedToTransportable );
public:
	virtual const NDb::SVisObj* Choose( const NDb::SAttachedModelVisObj *pAttachedVisObj )
	{
		return pAttachedVisObj->pTransportableModel;
	}
};

const NDb::SVisObj* CMOUnit::SChooseAttachedByDefault::Choose( const NDb::SAttachedModelVisObj *pAttachedVisObj )
{
	return pAttachedVisObj->pvisualObject;
}

void CMOUnit::ChangeModelToDamaged( const int nDamaged, const NDb::SModel *pNewModel, const NDb::ESeason eSeason )
{
	IMOUnit::ChangeModelToDamaged( nDamaged, pNewModel, eSeason );
	CPtr<IChooseAttached> pChooseFunc = new SChooseAttachedToDamaged( nDamaged );
	InitAttached( eSeason, pChooseFunc );
}

void CMOUnit::ChangeModelToUsual( const NDb::SModel *pNewModel, const NDb::ESeason eSeason )
{
	IMOUnit::ChangeModelToUsual( pNewModel, eSeason );
	CPtr<IChooseAttached> pChooseFunc = new SChooseAttachedByDefault();
	InitAttached( eSeason, pChooseFunc );
}

void CMOUnit::ChangeModelToAnimable( const NDb::SModel *pNewModel, const NDb::ESeason eSeason )
{
	IMOUnit::ChangeModelToAnimable( pNewModel, eSeason );
	CPtr<IChooseAttached> pChooseFunc = new SChooseAttachedToAnimable();
	InitAttached( eSeason, pChooseFunc );
}

void CMOUnit::ChangeModelToTransportable( const NDb::SModel *pNewModel, const NDb::ESeason eSeason )
{
	IMOUnit::ChangeModelToTransportable( pNewModel, eSeason );
	CPtr<IChooseAttached> pChooseFunc = new SChooseAttachedToTransportable();
	InitAttached( eSeason, pChooseFunc );
}

void CMOUnit::TryToAttach( const NDb::SAttachedModelVisObj *pAttachedObj, IChooseAttached *pChooseFunc, 
														const NDb::ESeason eSeason, const std::string &szLocator, const ESceneSubObjType eType, const int nNumber )
{
	if ( pAttachedObj )
	{
		const NDb::SVisObj *pNewAttachObj = pChooseFunc ? pChooseFunc->Choose( pAttachedObj ) : 0;
//		if ( pNewAttachObj == 0 )
//			pNewAttachObj = pAttachedObj->pvisualObject;

		if ( pNewAttachObj )
			Scene()->AttachSubModel( GetID(), eType, szLocator, GetModel( pNewAttachObj, eSeason ), ESAT_NO_REPLACE, nNumber, true, true );
	}
}

void CMOUnit::SetPointer( const NDb::SModel *pModel )
{
	if ( bPointer && pModel )
		return;

	ClearPointer();

	if ( pModel )
	{
		CVec3 vPointerOffset( 0.0f, 0.0f, s_fPointerOffset );
		Scene()->AttachSubModel( GetID(), ESSOT_EXTERN, pModel, ESAT_REPLACE_ON_TYPE, 0, true, vPointerOffset );
		IAttachedObject* pAttached = Scene()->GetAttached( GetID(), ESSOT_EXTERN, 0 );
		if ( pAttached )
		{
			pAttached->ReCreate( Scene()->GetGView(), Scene()->GetGameTimer() );

			bPointer = true;

			if ( pModel->pSkeleton != 0 ) 
			{
				if ( NAnimation::ISkeletonAnimator *pAnimator = pAttached->GetAnimator() )
				{
					for ( int i = 0; i < pModel->pSkeleton->animations.size(); ++i )
					{
						const NDb::SAnimB2 *pAnim = checked_cast_ptr<const NDb::SAnimB2*>( pModel->pSkeleton->animations[i] );
						if ( pAnim->eType == NDb::ANIMATION_IDLE )
						{
							AddAnimation( pAnim, Singleton<IGameTimer>()->GetGameTime(), pAnimator, pAnim->bLooped );
							break;
						}
					}
				}
			}
			if ( !IsVisible() )
				pAttached->Clear( Singleton<IGameTimer>()->GetGameTime() );
		}
	}
}

void CMOUnit::ClearPointer()
{
	if ( bPointer )
	{
		Scene()->RemoveAttached( GetID(), ESSOT_EXTERN, 0 );
		bPointer = false;
	}
}

int CMOUnit::operator&( IBinSaver &saver )
{
	saver.Add( 1, checked_cast<IMOUnit*>(this) );
	//saver.Add( 2, &ammos[0] );
	//saver.Add( 3, &ammos[1] );
	saver.Add( 6, &abilityMap);
	saver.Add( 7, &oldAreas );
	saver.Add( 8, &nOldAreasTime );
	saver.Add( 9, &bOpen );
	saver.Add( 10, &nLevel );
	saver.Add( 11, &ammos );
	saver.Add( 12, &nSupply );
	saver.Add( 13, &nMaxAmmo );
	saver.Add( 14, &nCurAmmo );
	saver.Add( 15, &bNewAbility );
	saver.Add( 16, &eReinfType );
	saver.Add( 17, &fFuel );
	saver.Add( 18, &bPointer );
	saver.Add( 19, &bShowUnitRank );
	return 0;
}

START_REGISTER( MOUnit )
REGISTER_VAR_EX( "objective_dynamic_pointer_offset", NGlobal::VarFloatHandler, &s_fPointerOffset, 3.0f, STORAGE_NONE );
FINISH_REGISTER

