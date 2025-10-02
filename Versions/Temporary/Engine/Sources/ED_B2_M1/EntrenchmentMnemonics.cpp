#include "stdafx.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\dbmapinfo.h"
#include "EntrenchmentMnemonics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CEntrenchmentSegmentTypeMnemonics::CEntrenchmentSegmentTypeMnemonics() : 
CMnemonicsCollector<int>( NDb::EST_LINE, "EST_LINE" )
{
	Insert( NDb::EST_LINE,				"EST_LINE" );
	Insert( NDb::EST_FIREPLACE,		"EST_FIREPLACE" );
	Insert( NDb::EST_TERMINATOR,	"EST_TERMINATOR" );
	Insert( NDb::EST_ARC,					"EST_ARC" );
}

CEntrenchmentSegmentTypeMnemonics typeEntrenchmentSegment;

