#include "stdafx.h"
#include "UnitTypes.h"
#include "RPGStats.h"

namespace NDb
{
	EUnitRPGType GetMainType( EUnitRPGType type ) { return EUnitRPGType( type & 0xffff0000 ); }
	bool IsInfantry( EUnitRPGType type ) { return (type & RPG_TYPE_INFANTRY) != 0; }
	bool IsTransport( EUnitRPGType type ) { return (type & RPG_TYPE_TRANSPORT) != 0; }
	bool IsArtillery( EUnitRPGType type ) { return (type & RPG_TYPE_ARTILLERY) != 0; }
	bool IsSPG( EUnitRPGType type ) { return (type & RPG_TYPE_SPG) != 0; }
	bool IsArmor( EUnitRPGType type ) { return (type & RPG_TYPE_ARMOR) != 0; }
	bool IsAviation( EUnitRPGType type ) { return (type & RPG_TYPE_AVIATION) != 0; }
	bool IsTrain( EUnitRPGType type ) { return (type & RPG_TYPE_TRAIN) != 0; }

	
	enum EUnitRPGType ReMapRPGType( enum EDBUnitRPGType eType )
	{
		switch( eType )
		{
		case DB_RPG_TYPE_SOLDIER						:return RPG_TYPE_SOLDIER						;
		case DB_RPG_TYPE_ENGINEER						:return RPG_TYPE_ENGINEER						;
		case DB_RPG_TYPE_SNIPER							:return RPG_TYPE_SNIPER							;
		case DB_RPG_TYPE_OFFICER						:return RPG_TYPE_OFFICER						;
		case DB_RPG_TYPE_TRN_CARRIER					:return RPG_TYPE_TRN_CARRIER				;
		case DB_RPG_TYPE_TRN_SUPPORT					:return RPG_TYPE_TRN_SUPPORT				;
		case DB_RPG_TYPE_TRN_MEDICINE					:return RPG_TYPE_TRN_MEDICINE				;
		case DB_RPG_TYPE_TRN_TRACTOR					:return RPG_TYPE_TRN_TRACTOR				;
		case DB_RPG_TYPE_TRN_MILITARY_AUTO				:return RPG_TYPE_TRN_MILITARY_AUTO	;
		case DB_RPG_TYPE_TRN_CIVILIAN_AUTO				:return RPG_TYPE_TRN_CIVILIAN_AUTO	;
		case DB_RPG_TYPE_ART_GUN						:return RPG_TYPE_ART_GUN						;
		case DB_RPG_TYPE_ART_HOWITZER					:return RPG_TYPE_ART_HOWITZER				;
		case DB_RPG_TYPE_ART_HEAVY_GUN					:return RPG_TYPE_ART_HEAVY_GUN			;
		case DB_RPG_TYPE_ART_AAGUN						:return RPG_TYPE_ART_AAGUN					;
		case DB_RPG_TYPE_ART_ROCKET						:return RPG_TYPE_ART_ROCKET					;
		case DB_RPG_TYPE_ART_SUPER						:return RPG_TYPE_ART_SUPER					;
		case DB_RPG_TYPE_ART_MORTAR						:return RPG_TYPE_ART_MORTAR					;
		case DB_RPG_TYPE_ART_HEAVY_MG					:return RPG_TYPE_ART_HEAVY_MG				;
		case DB_RPG_TYPE_SPG_ASSAULT					:return RPG_TYPE_SPG_ASSAULT				;
		case DB_RPG_TYPE_SPG_ANTITANK					:return RPG_TYPE_SPG_ANTITANK				;
		case DB_RPG_TYPE_SPG_SUPER						:return RPG_TYPE_SPG_SUPER					;
		case DB_RPG_TYPE_SPG_AAGUN						:return RPG_TYPE_SPG_AAGUN					;
		case DB_RPG_TYPE_ARM_LIGHT						:return RPG_TYPE_ARM_LIGHT					;
		case DB_RPG_TYPE_ARM_MEDIUM						:return RPG_TYPE_ARM_MEDIUM					;
		case DB_RPG_TYPE_ARM_HEAVY						:return RPG_TYPE_ARM_HEAVY					;
		case DB_RPG_TYPE_ARM_SUPER						:return RPG_TYPE_ARM_SUPER					;
		case DB_RPG_TYPE_AVIA_SCOUT						:return RPG_TYPE_AVIA_SCOUT					;
		case DB_RPG_TYPE_AVIA_BOMBER					:return RPG_TYPE_AVIA_BOMBER				;
		case DB_RPG_TYPE_AVIA_ATTACK					:return RPG_TYPE_AVIA_ATTACK				;
		case DB_RPG_TYPE_AVIA_FIGHTER					:return RPG_TYPE_AVIA_FIGHTER				;
		case DB_RPG_TYPE_AVIA_SUPER						:return RPG_TYPE_AVIA_SUPER					;
		case DB_RPG_TYPE_AVIA_LANDER					:return RPG_TYPE_AVIA_LANDER				;
		case DB_RPG_TYPE_TRAIN_LOCOMOTIVE				:return RPG_TYPE_TRAIN_LOCOMOTIVE		;
		case DB_RPG_TYPE_TRAIN_CARGO					:return RPG_TYPE_TRAIN_CARGO				;
		case DB_RPG_TYPE_TRAIN_CARRIER					:return RPG_TYPE_TRAIN_CARRIER			;
		case DB_RPG_TYPE_TRAIN_SUPER					:return RPG_TYPE_TRAIN_SUPER				;
		case DB_RPG_TYPE_TRAIN_ARMOR					:return RPG_TYPE_TRAIN_ARMOR				;
		case DB_RPG_TYPE_AVIA_STRAT_BOMBER				:return RPG_TYPE_AVIA_STRAT_BOMBER				;
		}
		NI_ASSERT( false, StrFmt( "wrong type %i", eType ) );
		return RPG_TYPE_INFANTRY;
	};
	const EUnitRPGClass GetRPGClass( const EUnitRPGType eType )
	{
		switch ( eType ) 
		{
		case RPG_TYPE_ART_GUN:
		case RPG_TYPE_ART_AAGUN:
		case RPG_TYPE_ART_ROCKET:
		case RPG_TYPE_ART_HOWITZER:
		case RPG_TYPE_ART_HEAVY_GUN:
		case RPG_TYPE_ART_SUPER:
			return RPG_CLASS_ARTILLERY;
		case RPG_TYPE_SPG_SUPER:
		case RPG_TYPE_SPG_AAGUN:
		case RPG_TYPE_SPG_ASSAULT:
		case RPG_TYPE_SPG_ANTITANK:
		case RPG_TYPE_ARM_LIGHT:
		case RPG_TYPE_ARM_MEDIUM:
		case RPG_TYPE_ARM_SUPER:
		case RPG_TYPE_ARM_HEAVY:
			return RPG_CLASS_TANK;
		case RPG_TYPE_SNIPER:
			return RPG_CLASS_SNIPER;
		}
		return RPG_CLASS_UNKNOWN;
	}
}


