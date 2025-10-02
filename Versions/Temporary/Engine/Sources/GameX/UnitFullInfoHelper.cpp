#include "StdAfx.h"
#include "UnitFullInfoHelper.h"
#include "../Stats_B2_M1/RPGStats.h"
#include "../Stats_B2_M1/AnimationType.h"
#include "../Stats_B2_M1/DBAnimB2.h"
#include "../Stats_B2_M1/DBVisObj.h"
#include "../System/Text.h"

namespace NUnitFullInfo
{

void MakeMechWeapons( vector<SWeapon> &weapons, const NDb::SMechUnitRPGStats *pStats );
void MakeSquadWeapons( vector<SWeapon> &weapons, const NDb::SSquadRPGStats *pStats );
void AddInfantryWeapons( vector<SWeapon> &weapons, const NDb::SInfantryRPGStats *pStats );
void AddWeapon( vector<SWeapon> &weapons, const SWeapon &weapon );

void MakeMechArmors( vector<int> &armors, const NDb::SMechUnitRPGStats *pStats );
void MakeSquadArmors( vector<int> &armors, const NDb::SSquadRPGStats *pStats );

const NDb::SModel* GetExactModel( const NDb::SVisObj *pVisObj, NDb::ESeason eSeason );

// SWeaponsSort

bool SWeaponsSort::operator()( const SWeapon &weapon1, const SWeapon &weapon2 ) const
{
	if ( weapon1.bPrimary != weapon2.bPrimary )
		return weapon1.bPrimary;
	int nPriority1 = GetWeaponPriority( weapon1 );
	int nPriority2 = GetWeaponPriority( weapon2 );
	if ( nPriority1 != nPriority2 )
		return nPriority1 > nPriority2;
	if ( weapon1.nPenetration != weapon2.nPenetration )
		return weapon1.nPenetration > weapon2.nPenetration;
	return weapon1.nDamage > weapon2.nDamage;
}

int SWeaponsSort::GetWeaponPriority( const SWeapon &weapon ) const
{
	switch ( weapon.pWeapon->eWeaponType )
	{
		case NDb::SWeaponRPGStats::WEAPON_PISTOL:
			return 20;
			
		case NDb::SWeaponRPGStats::WEAPON_MACHINEGUN:
			return 30;
			
		case NDb::SWeaponRPGStats::WEAPON_SUBMACHINEGUN:
			return 100;
			
		case NDb::SWeaponRPGStats::WEAPON_RIFLE:
			return 50;
			
		case NDb::SWeaponRPGStats::WEAPON_SNIPER_RIFLE:
			return 70;
			
		case NDb::SWeaponRPGStats::WEAPON_ANTITANK_RIFLE:
			return 500;
			
		case NDb::SWeaponRPGStats::WEAPON_BAZOOKA:
			return 700;
			
		case NDb::SWeaponRPGStats::WEAPON_PIAT:
			return 600;
			
		case NDb::SWeaponRPGStats::WEAPON_RIFLE_AMERICAN:
			return 60;
			
		case NDb::SWeaponRPGStats::WEAPON_FLAME_THROWER:
			return 1000; // high
			
		case NDb::SWeaponRPGStats::WEAPON_STEN:
			return 90;
			
		case NDb::SWeaponRPGStats::WEAPON_PANZERFAUST:
			return 800;
			
		case NDb::SWeaponRPGStats::WEAPON_LUFTFAUST:
			return 900;
		
		case NDb::SWeaponRPGStats::WEAPON_HEAVY_CANNON:
			return 10; // low
		
		default:
			return 0;
	}
}

void MakeMechWeapons( vector<SWeapon> &weapons, const NDb::SMechUnitRPGStats *pStats )
{
	weapons.clear();
	
	const int nID = -1;
	for ( int i = 0; i < pStats->GetPlatformsSize( nID ); ++i )
	{
		const NDb::SMechUnitRPGStats::SPlatform &platform = pStats->GetPlatform( nID, i );

		for ( int j = 0; j < pStats->GetGunsSize( nID, i ); ++j )
		{
			const NDb::SBaseGunRPGStats &gun = pStats->GetGun( nID, i, j );
			if ( !gun.pWeapon )
				continue;

			SWeapon weapon;

			const NDb::SWeaponRPGStats::SShell *pShell = gun.pWeapon->shells.empty() ? 0 : &gun.pWeapon->shells.front();
			weapon.pWeapon = gun.pWeapon;
			weapon.nCount = 1;
			weapon.bPrimary = gun.bIsPrimary;

			if ( CHECK_TEXT_NOT_EMPTY_PRE(gun.pWeapon->,LocalizedName) )
				weapon.wszLocalizedName = GET_TEXT_PRE(gun.pWeapon->,LocalizedName);

			weapon.nDamage = pShell->fDamagePower;
			weapon.nPenetration = pShell->nPiercing;
			weapon.nMaxAmmo = gun.nAmmo;
			weapon.nAmmo = weapon.nMaxAmmo;

			AddWeapon( weapons, weapon );
		}
	}
}

void MakeSquadWeapons( vector<SWeapon> &weapons, const NDb::SSquadRPGStats *pStats )
{
	weapons.clear();

	for ( vector< CDBPtr< NDb::SInfantryRPGStats > >::const_iterator it = pStats->members.begin();
		it != pStats->members.end(); ++it )
	{
		const NDb::SInfantryRPGStats *pInfantryStats = *it;
		
		AddInfantryWeapons( weapons, pInfantryStats );
	}
	sort( weapons.begin(), weapons.end(), SWeaponsSort() );
}

void AddInfantryWeapons( vector<SWeapon> &weapons, const NDb::SInfantryRPGStats *pStats )
{
	// Weapons
	const int nID = -1;
	for ( int i = 0; i < pStats->GetGunsSize( nID, 0 ); ++i )
	{
		const NDb::SBaseGunRPGStats &gun = pStats->GetGun( nID, 0, i );
		if ( !gun.pWeapon )
			continue;

		SWeapon weapon;

		const NDb::SWeaponRPGStats::SShell *pShell = gun.pWeapon->shells.empty() ? 0 : &gun.pWeapon->shells.front();
		weapon.pWeapon = gun.pWeapon;
		weapon.nCount = 1;
		weapon.bPrimary = gun.bIsPrimary;
		
		if ( CHECK_TEXT_NOT_EMPTY_PRE(gun.pWeapon->,LocalizedName) )
			weapon.wszLocalizedName = GET_TEXT_PRE(gun.pWeapon->,LocalizedName);

		weapon.nDamage = pShell->fDamagePower;
		weapon.nPenetration = pShell->nPiercing;
		weapon.nMaxAmmo = gun.nAmmo;
		weapon.nAmmo = weapon.nMaxAmmo;

		if ( weapon.pWeapon && weapon.pWeapon->eWeaponType != NDb::SWeaponRPGStats::WEAPON_HEAVY_CANNON ) // exclude grenades
			AddWeapon( weapons, weapon );
	}
}

void AddWeapon( vector<SWeapon> &weapons, const SWeapon &weapon )
{
	vector<SWeapon>::iterator it = find( weapons.begin(), weapons.end(), weapon );
	if ( it != weapons.end() )
	{
		SWeapon &current = *it;
		++current.nCount;
		current.nAmmo += weapon.nAmmo;
		current.nMaxAmmo = current.nAmmo;
	}
	else
	{
		if ( weapon.bPrimary )
			weapons.insert( weapons.begin(), weapon );
		else
			weapons.push_back( weapon );
	}
}

void MakeWeapons( vector<SWeapon> &weapons, const NDb::SHPObjectRPGStats *pStats )
{
	if ( const NDb::SMechUnitRPGStats *pMechStats = dynamic_cast<const NDb::SMechUnitRPGStats*>( pStats ) )
	{
		MakeMechWeapons( weapons, pMechStats );
	}
	else if ( const NDb::SSquadRPGStats *pSquadStats = dynamic_cast<const NDb::SSquadRPGStats*>( pStats ) )
	{
		MakeSquadWeapons( weapons, pSquadStats );
	}
}

void MakeArmors( vector<int> &armors, const NDb::SHPObjectRPGStats *pStats )
{
	if ( const NDb::SMechUnitRPGStats *pMechStats = dynamic_cast<const NDb::SMechUnitRPGStats*>( pStats ) )
	{
		MakeMechArmors( armors, pMechStats );
	}
	else if ( const NDb::SSquadRPGStats *pSquadStats = dynamic_cast<const NDb::SSquadRPGStats*>( pStats ) )
	{
		MakeSquadArmors( armors, pSquadStats );
	}
}

void MakeMechArmors( vector<int> &armors, const NDb::SMechUnitRPGStats *pStats )
{
	if ( pStats->armors.size() >= ARMOR_COUNT )
	{
		int nArmorFront = (pStats->armors[ARMOR_FRONT].fMin + pStats->armors[ARMOR_FRONT].fMax) / 2;
		int nArmorSize = (pStats->armors[ARMOR_SIDE_1].fMin + pStats->armors[ARMOR_SIDE_1].fMax +
			pStats->armors[ARMOR_SIDE_2].fMin + pStats->armors[ARMOR_SIDE_2].fMax) / 4;
		int nArmorBack = (pStats->armors[ARMOR_BACK].fMin + pStats->armors[ARMOR_BACK].fMax) / 2;
		int nArmorTop = (pStats->armors[ARMOR_TOP].fMin + pStats->armors[ARMOR_TOP].fMax) / 2;
		
		if ( nArmorFront + nArmorSize + nArmorBack + nArmorTop > 0 )
		{
			armors.resize( ARMOR_COUNT );
			
			armors[ES_ARMOR_FRONT] = nArmorFront;
			armors[ES_ARMOR_SIDE] = nArmorSize;
			armors[ES_ARMOR_BACK] = nArmorBack;
			armors[ES_ARMOR_TOP] = nArmorTop;
		}
	}
}

void MakeSquadArmors( vector<int> &armors, const NDb::SSquadRPGStats *pStats )
{
	armors.resize( ARMOR_COUNT );

	armors[ES_ARMOR_FRONT] = 0;
	armors[ES_ARMOR_SIDE] = 0;
	armors[ES_ARMOR_BACK] = 0;
	armors[ES_ARMOR_TOP] = 0;
	
	for ( vector< CDBPtr< NDb::SInfantryRPGStats > >::const_iterator it = pStats->members.begin();
		it != pStats->members.end(); ++it )
	{
		const NDb::SInfantryRPGStats *pStats = *it;
		armors[ES_ARMOR_FRONT] += pStats->fArmor;
		armors[ES_ARMOR_SIDE] += pStats->fArmor;
		armors[ES_ARMOR_BACK] += pStats->fArmor;
		armors[ES_ARMOR_TOP] += pStats->fArmor;
	}
	if ( int nCount = pStats->members.size() > 0 )
	{
		armors[ES_ARMOR_FRONT] /= nCount;
		armors[ES_ARMOR_SIDE] /= nCount;
		armors[ES_ARMOR_BACK] /= nCount;
		armors[ES_ARMOR_TOP] /= nCount;
	}
	if ( armors[ES_ARMOR_FRONT] + armors[ES_ARMOR_SIDE] + armors[ES_ARMOR_BACK] + armors[ES_ARMOR_TOP] <= 0 )
		armors.resize( 0 );
}

int MakeHP( const NDb::SHPObjectRPGStats *pStats )
{
	if ( const NDb::SSquadRPGStats *pSquadStats = dynamic_cast<const NDb::SSquadRPGStats*>( pStats ) )
	{
		return pSquadStats->members.size();
	}
	else
	{
		return pStats->fMaxHP;
	}
}

const NDb::SAnimB2* FindAnimation( const NDb::SInfantryRPGStats *pStats )
{
	if ( !pStats )
		return 0;

	const NDb::SAnimB2 *pAnim = 0;
	if ( NDb::ANIMATION_MARCH < pStats->animdescs.size() )
	{
		const NDb::Svector_AnimDescs &animDescs = pStats->animdescs[NDb::ANIMATION_MARCH];
		if ( !animDescs.anims.empty() )
		{
			const NDb::SAnimDesc &animDesc = animDescs.anims[0];
			if ( animDesc.pAnimation )
				pAnim = checked_cast_ptr<const NDb::SAnimB2 *>( animDesc.pAnimation );
		}
	}
	if ( !pAnim && NDb::ANIMATION_IDLE < pStats->animdescs.size() )
	{
		const NDb::Svector_AnimDescs &animDescs = pStats->animdescs[NDb::ANIMATION_IDLE];
		if ( !animDescs.anims.empty() )
		{
			const NDb::SAnimDesc &animDesc = animDescs.anims[0];
			if ( animDesc.pAnimation )
				pAnim = checked_cast_ptr<const NDb::SAnimB2 *>( animDesc.pAnimation );
		}
	}
	return pAnim;
}

const NDb::SModel* GetExactModel( const NDb::SVisObj *pVisObj, NDb::ESeason eSeason )
{
	if ( pVisObj )
	{
		for ( vector<NDb::SVisObj::SSingleObj>::const_iterator it = pVisObj->models.begin(); it != pVisObj->models.end(); ++it )
		{
			if ( it->eSeason == eSeason ) 
				return it->pModel;
		}
	}

	return 0;
}

const NDb::SModel* GetModel( const NDb::SVisObj *pVisObj, NDb::ESeason eSeason )
{
	const NDb::SModel *pExactModel = GetExactModel( pVisObj, eSeason );
	return pExactModel != 0 ? pExactModel : GetExactModel( pVisObj, NDb::SEASON_SUMMER );
}

bool IsResourcesCarrier( const CUserActions &userActions )
{
	return userActions.HasAction( NDb::USER_ACTION_ENGINEER_REPAIR ) ||
		userActions.HasAction( NDb::USER_ACTION_SUPPORT_RESUPPLY ) ||
		userActions.HasAction( NDb::USER_ACTION_FILL_RU ) ||
		userActions.HasAction( NDb::USER_ACTION_ENGINEER_BUILD_DEFENSE ) ||
		userActions.HasAction( NDb::USER_ACTION_ENGINEER_BUILD_ENTRENCHMENT ) ||
		userActions.HasAction( NDb::USER_ACTION_ENGINEER_BUILD_FENCE ) ||
		userActions.HasAction( NDb::USER_ACTION_ENGINEER_PLACE_MINES ) ||
		userActions.HasAction( NDb::USER_ACTION_ENGINEER_CLEAR_MINES );
}

} //namespace NUnitFullInfo


