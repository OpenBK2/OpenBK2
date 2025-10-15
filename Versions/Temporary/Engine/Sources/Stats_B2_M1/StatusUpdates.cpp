#include "stdafx.h"

#include "StatusUpdates.h"

#include "System/Commands.h"

REGISTER_SAVELOAD_CLASS( STATS_B2_M1, 0x31247300, SUnitStatusUpdate );

SUnitStatusUpdate::SUnitStatusUpdate( const EUnitStatus _eStatus, const bool bEnabled, const float _fRadius )
: SAIBasicUpdate( ACTION_NOTIFY_UPDATE_STATUS, 0 ), eStatus( _eStatus ), nUnitID( -1 ), fRadius( _fRadius )
{
	if ( !bEnabled )
		eStatus = (EUnitStatus)( (int)eStatus & EUS_GROUP_MASK);
}

SUnitStatusUpdate *CreateStatusUpdate( const EUnitStatus eStatus, const bool bEnabled )
{
	return new SUnitStatusUpdate( eStatus, bEnabled, 0.0f );
}

SUnitStatusUpdate *CreateStatusUpdate( const EUnitStatus eStatus, const bool bEnabled, const float fRadius )
{
	return new SUnitStatusUpdate( eStatus, bEnabled, fRadius );
}

const char *GetStatusName( const EUnitStatus eStatus )
{
	switch( eStatus )
	{
		case EUS_UNDEFINED : return "EUS_UNDEFINED";
		case EUS_PROCESS_GROUP : return "EUS_PROCESS_GROUP";
		case EUS_BLASTING_CHARGE : return "EUS_BLASTING_CHARGE";
		case EUS_BUILD_OBSTACLES : return "EUS_BUILD_OBSTACLES";
		case EUS_DEMINE : return "EUS_DEMINE";
		case EUS_DIG_TRENCHES : return "EUS_DIG_TRENCHES";
		case EUS_SET_MINE_FIELD : return "EUS_SET_MINE_FIELD";
		case EUS_HOOK_ARTILLERY : return "EUS_HOOK_ARTILLERY";
		case EUS_DEPLOY_ARTILLERY : return "EUS_DEPLOY_ARTILLERY";
		case EUS_ENTRENCH : return "EUS_ENTRENCH";
		case EUS_FILL_RESOURCE : return "EUS_FILL_RESOURCE";
		case EUS_LEAVE : return "EUS_LEAVE";
		case EUS_LOAD : return "EUS_LOAD";
		case EUS_REPAIR : return "EUS_REPAIR";
		case EUS_REPAIR_OBJECT : return "EUS_REPAIR_OBJECT";
		case EUS_RESUPPLY : return "EUS_RESUPPLY";
		case EUS_TAKE_ARTILLERY : return "EUS_TAKE_ARTILLERY";
		case EUS_UNLOAD : return "EUS_UNLOAD";
		case EUS_ROTATE : return "EUS_ROTATE";

		case EUS_GRUAD_GROUP : return "EUS_GRUAD_GROUP";
		case EUS_HOLD_SECTOR : return "EUS_HOLD_SECTOR";
		case EUS_USE_SPY_GLASS : return "EUS_USE_SPY_GLASS";
		case EUS_STAND_GROUND : return "EUS_STAND_GROUND";

		case EUS_REPAIRS_GROUP : return "EUS_REPAIRS_GROUP";
		case EUS_TRACK_DAMAGED : return "EUS_TRACK_DAMAGED";

		case EUS_RESUPPLY_GROUP : return "EUS_RESUPPLY_GROUP";
		case EUS_NO_AMMO : return "EUS_NO_AMMO";
		case EUS_LOW_AMMO : return "EUS_LOW_AMMO";

		case EUS_AGGRESSIVE_GROUP : return "EUS_AGGRESSIVE_GROUP";
		case EUS_COUNTER_FIRE : return "EUS_COUNTER_FIRE";
		case EUS_SUPPORT_FIRE : return "EUS_SUPPORT_FIRE";
		case EUS_SUPPRESSIVE_FIRE : return "EUS_SUPPRESSIVE_FIRE";
		case EUS_ZEROING_IN : return "EUS_ZEROING_IN";
		case EUS_SWARM_TO : return "EUS_SWARM_TO";

		case EUS_BEST_SHOT_GROUP : return "EUS_BEST_SHOT_GROUP";
		case EUS_CRITICAL_TARGET : return "EUS_CRITICAL_TARGET";
		case EUS_EXACT_SHOT : return "EUS_EXACT_SHOT";
		case EUS_TRACK_TARGET : return "EUS_TRACK_TARGET";

		case EUS_PACIFIC_GROUP : return "EUS_PACIFIC_GROUP";
		case EUS_SMOKE_SHOT : return "EUS_SMOKE_SHOT";

		case EUS_INVISIBLE_GROUP : return "EUS_INVISIBLE_GROUP";
		case EUS_SPY_MODE : return "EUS_SPY_MODE";
		case EUS_CAMOUFLAGE : return "EUS_CAMOUFLAGE";
		case EUS_AMBUSH : return "EUS_AMBUSH";

		case EUS_KEY_POINT : return "EUS_KEY_POINT";
		case EUS_KEY_POINT_DAMAGED : return "EUS_KEY_POINT_DAMAGED";
		case EUS_GROUP_MASK : return "EUS_GROUP_MASK";
		case EUS_STATUS_MASK : return "EUS_STATUS_MASK";
		default: return "Unknown status";
	}
}


