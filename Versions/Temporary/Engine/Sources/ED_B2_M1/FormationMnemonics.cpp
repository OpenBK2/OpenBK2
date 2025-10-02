#include "stdafx.h"
#include "../misc/2darray.h"
#include "../stats_b2_m1/rpgstats.h"
#include "FormationMnemonics.h"

#include <zconf.h>

CFormationMnemonics::CFormationMnemonics() : CMnemonicsCollector<int>( NDb::SWeaponRPGStats::WEAPON_PISTOL, "WEAPON_PISTOL" )
{
	Insert( NDb::SSquadRPGStats::SFormation::DEFAULT, "DEFAULT" );
	Insert( NDb::SSquadRPGStats::SFormation::MOVEMENT, "MOVEMENT" );
	Insert( NDb::SSquadRPGStats::SFormation::DEFENSIVE, "DEFENSIVE" );
	Insert( NDb::SSquadRPGStats::SFormation::OFFENSIVE, "OFFENSIVE" );
	Insert( NDb::SSquadRPGStats::SFormation::SNEAK, "SNEAK" );
}

CFormationMnemonics typeFormationMnemonics;


