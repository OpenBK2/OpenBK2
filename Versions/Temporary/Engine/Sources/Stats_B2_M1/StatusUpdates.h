#pragma once

#include "Stats_B2_M1_export.h"


#include "AIUpdates.h"

enum EUnitStatus : unsigned int
{
	EUS_UNDEFINED         = 0x00000000,

	EUS_PROCESS_GROUP     = 0x00010000,
	EUS_BLASTING_CHARGE   = 0x00010001, //пехота
	EUS_BUILD_OBSTACLES   = 0x00010002, //грузовичок
	EUS_DEMINE            = 0x00010003, //грузовичок
	EUS_DIG_TRENCHES      = 0x00010004, //грузовичок
	EUS_SET_MINE_FIELD    = 0x00010005, //грузовичок
	EUS_HOOK_ARTILLERY    = 0x00010006, //грузовичок
	EUS_DEPLOY_ARTILLERY  = 0x00010007, //грузовичок
	EUS_ENTRENCH          = 0x00010008, //юнит
	EUS_FILL_RESOURCE     = 0x00010009, //грузовичок
	EUS_LEAVE             = 0x0001000A, //пехота
	EUS_LOAD              = 0x0001000B, //пехота
	EUS_REPAIR            = 0x0001000C, //грузовичок
	EUS_REPAIR_OBJECT     = 0x0001000D, //грузовичок
	EUS_RESUPPLY          = 0x0001000E, //грузовичок
	EUS_TAKE_ARTILLERY    = 0x0001000F, //пехота
	EUS_UNLOAD            = 0x00010010, //грузовичок
	EUS_ROTATE            = 0x00010011, //юнит

	EUS_GRUAD_GROUP       = 0x00020000,
	EUS_HOLD_SECTOR       = 0x00020012,
	EUS_USE_SPY_GLASS     = 0x00020013,
	EUS_STAND_GROUND      = 0x00020014,
	EUS_IN_TANK_PIT				= 0x00020026,

	EUS_REPAIRS_GROUP     = 0x00030000,
	EUS_TRACK_DAMAGED     = 0x00030015,

	EUS_RESUPPLY_GROUP    = 0x00040000,
	EUS_NO_AMMO           = 0x00040017,
	EUS_LOW_AMMO          = 0x00040018,

	EUS_AGGRESSIVE_GROUP  = 0x00050000,
	EUS_COUNTER_FIRE      = 0x00050019,
	EUS_SUPPORT_FIRE      = 0x0005001A,
	EUS_SUPPRESSIVE_FIRE  = 0x0005001B,
	EUS_ZEROING_IN        = 0x0005001C,
	EUS_SWARM_TO          = 0x0005001D,

	EUS_BEST_SHOT_GROUP   = 0x00060000,
	EUS_CRITICAL_TARGET   = 0x0006001E,
	EUS_EXACT_SHOT        = 0x0006001F,
	EUS_TRACK_TARGET      = 0x00060020,

	EUS_PACIFIC_GROUP     = 0x00070000,
	EUS_SMOKE_SHOT        = 0x00070021,

	EUS_INVISIBLE_GROUP   = 0x00080000,
	EUS_SPY_MODE          = 0x00080022,
	EUS_CAMOUFLAGE        = 0x00080023,
	EUS_AMBUSH            = 0x00080024,

	EUS_KEY_POINT         = 0x00090000,
	EUS_KEY_POINT_DAMAGED = 0x00090025,

	EUS_GROUP_MASK        = 0xFFFF0000,
	EUS_STATUS_MASK       = 0x0000FFFF,
};

struct SUnitStatusUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SUnitStatusUpdate )
public:
	ZDATA_( SAIBasicUpdate )
		EUnitStatus eStatus;
		int nUnitID; // sets in CUpdateStatusTransformer
		ZSKIP //float fTimeToWait; // sets in constructor
		float fRadius;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&eStatus); f.Add(3,&nUnitID); f.Add(5,&fRadius); return 0; }
	SUnitStatusUpdate() : eStatus( EUS_UNDEFINED ), nUnitID( -1 ), fRadius( 0.0f ) {}
	SUnitStatusUpdate( const EUnitStatus eStatus, const bool bEnabled, const float fRadius );
};

STATS_B2_M1_EXPORT SUnitStatusUpdate *CreateStatusUpdate( const EUnitStatus eStatus, const bool bEnabled );
STATS_B2_M1_EXPORT SUnitStatusUpdate *CreateStatusUpdate( const EUnitStatus eStatus, const bool bEnabled, const float fRadius );
const char *GetStatusName( const EUnitStatus eStatus );


