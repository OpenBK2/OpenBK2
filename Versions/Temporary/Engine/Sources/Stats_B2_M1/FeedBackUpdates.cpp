#include "stdafx.h"

#include "FeedBackUpdates.h"

#include "Stats_B2_M1_export.h"

int CRAP_LinkerTooSmart_FeedBackUpdates()
{
	return 0;
}

REGISTER_SAVELOAD_CLASS( STATS_B2_M1, 0x120B7302, SAIFeedbackUpdate )
REGISTER_SAVELOAD_CLASS( STATS_B2_M1, 0x111BEC80, SFeedBackObjectiveState )
REGISTER_SAVELOAD_CLASS( STATS_B2_M1, 0x111BEC81, SFeedBackUnitsArray )

