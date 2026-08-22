#include "stdafx.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/DBMapInfo.h"
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


