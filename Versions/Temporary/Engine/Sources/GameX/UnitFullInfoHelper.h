#pragma once

namespace NDb
{
	struct SWeaponRPGStats;
	struct SMechUnitRPGStats;
	struct SSquadRPGStats;
	struct SInfantryRPGStats;
	struct SHPObjectRPGStats;
	struct SAnimB2;
	struct SModel;
	enum ESeason;
	struct SVisObj;
}
class CUserActions;

namespace NUnitFullInfo
{

// DB indexes
enum EArmor
{
	ARMOR_FRONT				= 0,
	ARMOR_SIDE_1			= 1,
	ARMOR_BACK				= 2,
	ARMOR_SIDE_2			= 3,
	ARMOR_TOP					= 4,
	ARMOR_BOTTOM			= 5,
	
	ARMOR_COUNT				= 6,
};

// visual indexes
enum EStatsArmor
{
	ES_ARMOR_FRONT,
	ES_ARMOR_SIDE,
	ES_ARMOR_BACK,
	ES_ARMOR_TOP,

	ES_ARMOR_COUNT,
};

struct SWeapon
{
	ZDATA
	CDBPtr<NDb::SWeaponRPGStats> pWeapon;
	int nCount;
	bool bPrimary;
	std::wstring wszLocalizedName;
	int nDamage;
	int nPenetration;
	int nAmmo;
	int nMaxAmmo;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWeapon); f.Add(3,&nCount); f.Add(4,&bPrimary); f.Add(5,&wszLocalizedName); f.Add(6,&nDamage); f.Add(7,&nPenetration); f.Add(8,&nAmmo); f.Add(9,&nMaxAmmo); return 0; }
	
	bool operator==( const SWeapon &weapon ) const
	{
		return pWeapon == weapon.pWeapon;
	}
};

struct SWeaponsSort
{
	bool operator()( const SWeapon &weapon1, const SWeapon &weapon2 ) const;
	
	int GetWeaponPriority( const SWeapon &weapon ) const;
};

struct SHP
{
	ZDATA
	float fFraction;
	int nHP;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&fFraction); f.Add(3,&nHP); return 0; }
};

void MakeWeapons( std::vector<SWeapon> &weapons, const NDb::SHPObjectRPGStats *pStats );
void MakeArmors( std::vector<int> &armors, const NDb::SHPObjectRPGStats *pStats );
int MakeHP( const NDb::SHPObjectRPGStats *pStats );

const NDb::SAnimB2* FindAnimation( const NDb::SInfantryRPGStats *pStats );
const NDb::SModel* GetModel( const NDb::SVisObj *pVisObj, NDb::ESeason eSeason );

bool IsResourcesCarrier( const CUserActions &userActions );

} //namespace NUnitFullInfo


