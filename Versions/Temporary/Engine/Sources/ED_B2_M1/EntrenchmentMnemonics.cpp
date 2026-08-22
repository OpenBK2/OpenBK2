#include "stdafx.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "EntrenchmentMnemonics.h"

#include <zconf.h>

CEntrenchmentSegmentTypeMnemonics::CEntrenchmentSegmentTypeMnemonics() : 
CMnemonicsCollector<int>( NDb::EST_LINE, "EST_LINE" )
{
	Insert( NDb::EST_LINE,				"EST_LINE" );
	Insert( NDb::EST_FIREPLACE,		"EST_FIREPLACE" );
	Insert( NDb::EST_TERMINATOR,	"EST_TERMINATOR" );
	Insert( NDb::EST_ARC,					"EST_ARC" );
}

CEntrenchmentSegmentTypeMnemonics typeEntrenchmentSegment;


