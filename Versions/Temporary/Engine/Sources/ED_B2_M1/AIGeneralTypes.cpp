#include "stdafx.h"
#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/dbmapinfo.h"
#include "AIGeneralTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CAIGeneralParcelTypeMnemonics::CAIGeneralParcelTypeMnemonics() : 
CMnemonicsCollector<int>(NDb::EPATCH_UNKNOWN, "EPATCH_UNKNOWN" )
{
	Insert( NDb::EPATCH_UNKNOWN, "EPATCH_UNKNOWN" );
	Insert( NDb::EPATCH_DEFENCE, "EPATCH_DEFENCE" );
	Insert( NDb::EPATCH_REINFORCE, "EPATCH_REINFORCE" );
}

CAIGeneralParcelTypeMnemonics typeAIGeneralParcel;


