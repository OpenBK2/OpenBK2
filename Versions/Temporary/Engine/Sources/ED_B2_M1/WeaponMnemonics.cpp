#include "stdafx.h"

#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/rpgstats.h"
#include "WeaponMnemonics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CMayaWeaponMnemonics::CMayaWeaponMnemonics() : CMnemonicsCollector<int>( NDb::SWeaponRPGStats::WEAPON_PISTOL, "PISTOL" )
{
	Insert( NDb::SWeaponRPGStats::WEAPON_PISTOL, "PISTOL" );
	Insert( NDb::SWeaponRPGStats::WEAPON_MACHINEGUN, "MACHINEGUN" );
	Insert( NDb::SWeaponRPGStats::WEAPON_SUBMACHINEGUN, "SUBMACHINEGUN" );
	Insert( NDb::SWeaponRPGStats::WEAPON_RIFLE, "RIFLE" );
	Insert( NDb::SWeaponRPGStats::WEAPON_SNIPER_RIFLE, "SNIPER_RIFLE" );
	Insert( NDb::SWeaponRPGStats::WEAPON_ANTITANK_RIFLE, "ANTITANK_RIFLE" );
	Insert( NDb::SWeaponRPGStats::WEAPON_BAZOOKA, "BAZOOKA" );
	Insert( NDb::SWeaponRPGStats::WEAPON_HEAVY_CANNON, "HEAVY_CANNON" );
	Insert( NDb::SWeaponRPGStats::WEAPON_PIAT, "PIAT" );
	Insert( NDb::SWeaponRPGStats::WEAPON_RIFLE_AMERICAN, "AMERICAN_RIFLE" );
	Insert( NDb::SWeaponRPGStats::WEAPON_FLAME_THROWER, "FLAME_THROWER" );
	Insert( NDb::SWeaponRPGStats::WEAPON_STEN, "STEN" );
	Insert( NDb::SWeaponRPGStats::WEAPON_PANZERFAUST, "PANZERFAUST" );
	Insert( NDb::SWeaponRPGStats::WEAPON_LUFTFAUST, "LUFTFAUST" );
}

CWeaponMnemonics::CWeaponMnemonics() : CMnemonicsCollector<int>( NDb::SWeaponRPGStats::WEAPON_PISTOL, "WEAPON_PISTOL" )
{
	Insert( NDb::SWeaponRPGStats::WEAPON_PISTOL, "WEAPON_PISTOL" );
	Insert( NDb::SWeaponRPGStats::WEAPON_MACHINEGUN, "WEAPON_MACHINEGUN" );
	Insert( NDb::SWeaponRPGStats::WEAPON_SUBMACHINEGUN, "WEAPON_SUBMACHINEGUN" );
	Insert( NDb::SWeaponRPGStats::WEAPON_RIFLE, "WEAPON_RIFLE" );
	Insert( NDb::SWeaponRPGStats::WEAPON_SNIPER_RIFLE, "WEAPON_SNIPER_RIFLE" );
	Insert( NDb::SWeaponRPGStats::WEAPON_ANTITANK_RIFLE, "WEAPON_ANTITANK_RIFLE" );
	Insert( NDb::SWeaponRPGStats::WEAPON_BAZOOKA, "WEAPON_BAZOOKA" );
	Insert( NDb::SWeaponRPGStats::WEAPON_HEAVY_CANNON, "WEAPON_HEAVY_CANNON" );
	Insert( NDb::SWeaponRPGStats::WEAPON_PIAT, "WEAPON_PIAT" );
	Insert( NDb::SWeaponRPGStats::WEAPON_RIFLE_AMERICAN, "WEAPON_RIFLE_AMERICAN" );
	Insert( NDb::SWeaponRPGStats::WEAPON_FLAME_THROWER, "WEAPON_FLAME_THROWER" );
	Insert( NDb::SWeaponRPGStats::WEAPON_STEN, "WEAPON_STEN" );
	Insert( NDb::SWeaponRPGStats::WEAPON_PANZERFAUST, "WEAPON_PANZERFAUST" );
	Insert( NDb::SWeaponRPGStats::WEAPON_LUFTFAUST, "WEAPON_LUFTFAUST" );
};

CMayaWeaponMnemonics typeMayaWeaponMnemonics;
CWeaponMnemonics typeWeaponMnemonics;

// basement storage  


