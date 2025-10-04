#include "stdafx.h"
#include "Misc/2Darray.h"
#include "stats_b2_m1/dbmapinfo.h"
#include "AIGeneralTypes.h"

#include <zconf.h>

CAIGeneralParcelTypeMnemonics::CAIGeneralParcelTypeMnemonics() : 
CMnemonicsCollector<int>(NDb::EPATCH_UNKNOWN, "EPATCH_UNKNOWN" )
{
	Insert( NDb::EPATCH_UNKNOWN, "EPATCH_UNKNOWN" );
	Insert( NDb::EPATCH_DEFENCE, "EPATCH_DEFENCE" );
	Insert( NDb::EPATCH_REINFORCE, "EPATCH_REINFORCE" );
}

CAIGeneralParcelTypeMnemonics typeAIGeneralParcel;


