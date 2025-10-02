#include "stdafx.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\rpgstats.h"
#include "ReinforcementTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CReinforcementTypeMnemonics::CReinforcementTypeMnemonics() : 
CMnemonicsCollector<int>( NDb::_RT_NONE, "_RT_NONE" )
{
	Insert( NDb::RT_MAIN_INFANTRY, "RT_MAIN_INFANTRY" );
	Insert( NDb::RT_ASSAULT_INFANTRY, "RT_ASSAULT_INFANTRY" );
	Insert( NDb::RT_ARTILLERY_ANTITANK, "RT_ARTILLERY_ANTITANK" );
	Insert( NDb::RT_ARTILLERY, "RT_ARTILLERY" );
	Insert( NDb::RT_ASSAULT_GUNS, "RT_ASSAULT_GUNS" );
	Insert( NDb::RT_TANK_DESTROYERS, "RT_TANK_DESTROYERS" );
	Insert( NDb::RT_ARTILLERY_ROCKET, "RT_ARTILLERY_ROCKET" );
	Insert( NDb::RT_LIGHT_TANKS, "RT_LIGHT_TANKS" );
	Insert( NDb::RT_TANKS, "RT_TANKS" );
	Insert( NDb::RT_HEAVY_TANKS, "RT_HEAVY_TANKS" );
	Insert( NDb::RT_LIGHT_AAA, "RT_LIGHT_AAA" );
	Insert( NDb::RT_HEAVY_AAA, "RT_HEAVY_AAA" );
	Insert( NDb::RT_FIGHTERS, "RT_FIGHTERS" );
	Insert( NDb::RT_BOMBERS, "RT_BOMBERS" );
	Insert( NDb::RT_GROUND_ATTACK_PLANES, "RT_GROUND_ATTACK_PLANES" );
	Insert( NDb::RT_RECON, "RT_RECON" );
	Insert( NDb::RT_PARATROOPS, "RT_PARATROOPS" );
	Insert( NDb::RT_ENGINEERING, "RT_ENGINEERING" );
	Insert( NDb::RT_HEAVY_ARTILLERY, "RT_HEAVY_ARTILLERY" );
	Insert( NDb::_RT_NONE, "_RT_NONE" );
}

CReinforcementTypeMnemonics typeReinforcementMnemonics;

