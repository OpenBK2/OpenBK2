#include "stdafx.h"
#include "../misc/2darray.h"
#include "../stats_b2_m1/rpgstats.h"
#include "UnitDesignTypes.h"

#include <zconf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MNEMO( e ) Insert( NDb::DB_RPG_TYPE_##e, #e );

CUnitDesignTypeMnemonics::CUnitDesignTypeMnemonics() : 
CMnemonicsCollector<int>(-1, "UNIT_TYPE_UNKNOWN" )
{
	MNEMO( SOLDIER );
	MNEMO( ENGINEER );
	MNEMO( SNIPER );
	MNEMO( OFFICER );
	MNEMO( TRN_CARRIER );
	MNEMO( TRN_SUPPORT );
	MNEMO( TRN_MEDICINE );
	MNEMO( TRN_TRACTOR );
	MNEMO( TRN_MILITARY_AUTO );
	MNEMO( TRN_CIVILIAN_AUTO );
	MNEMO( ART_GUN );
	MNEMO( ART_HOWITZER );
	MNEMO( ART_HEAVY_GUN );
	MNEMO( ART_AAGUN );
	MNEMO( ART_ROCKET );
	MNEMO( ART_SUPER );
	MNEMO( ART_MORTAR );
	MNEMO( ART_HEAVY_MG );
	MNEMO( SPG_ASSAULT );
	MNEMO( SPG_ANTITANK );
	MNEMO( SPG_SUPER );
	MNEMO( SPG_AAGUN );
	MNEMO( ARM_LIGHT );
	MNEMO( ARM_MEDIUM );
	MNEMO( ARM_HEAVY );
	MNEMO( ARM_SUPER );
	MNEMO( AVIA_SCOUT );
	MNEMO( AVIA_BOMBER );
	MNEMO( AVIA_ATTACK );
	MNEMO( AVIA_FIGHTER );
	MNEMO( AVIA_SUPER );
	MNEMO( AVIA_LANDER );
	MNEMO( TRAIN_LOCOMOTIVE );
	MNEMO( TRAIN_CARGO );
	MNEMO( TRAIN_CARRIER );
	MNEMO( TRAIN_SUPER );
	MNEMO( TRAIN_ARMOR );
}

CUnitDesignTypeMnemonics typeUnitDesignTypeMnemonics;


