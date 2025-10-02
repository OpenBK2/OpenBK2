#include "stdafx.h"
#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/rpgstats.h"
#include "FormationMnemonics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFormationMnemonics::CFormationMnemonics() : CMnemonicsCollector<int>( NDb::SWeaponRPGStats::WEAPON_PISTOL, "WEAPON_PISTOL" )
{
	Insert( NDb::SSquadRPGStats::SFormation::DEFAULT, "DEFAULT" );
	Insert( NDb::SSquadRPGStats::SFormation::MOVEMENT, "MOVEMENT" );
	Insert( NDb::SSquadRPGStats::SFormation::DEFENSIVE, "DEFENSIVE" );
	Insert( NDb::SSquadRPGStats::SFormation::OFFENSIVE, "OFFENSIVE" );
	Insert( NDb::SSquadRPGStats::SFormation::SNEAK, "SNEAK" );
}

CFormationMnemonics typeFormationMnemonics;


