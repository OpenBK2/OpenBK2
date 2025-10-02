// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "acktypes.h"

namespace NDb
{


string EnumToString( NDb::EUnitAckType eValue )
{
	switch ( eValue )
	{
	case NDb::ACK_NONE:
		return "ACK_NONE";
	case NDb::ACK_BORED_ATTACK:
		return "ACK_BORED_ATTACK";
	case NDb::ACK_BORED_IDLE:
		return "ACK_BORED_IDLE";
	case NDb::ACK_BUILDING_FINISHED:
		return "ACK_BUILDING_FINISHED";
	case NDb::ACK_CANNOT_FINISH_BUILD:
		return "ACK_CANNOT_FINISH_BUILD";
	case NDb::ACK_CANNOT_MOVE_TRACK_DAMAGED:
		return "ACK_CANNOT_MOVE_TRACK_DAMAGED";
	case NDb::ACK_CANNOT_PIERCE:
		return "ACK_CANNOT_PIERCE";
	case NDb::ACK_DONT_SEE_THE_ENEMY:
		return "ACK_DONT_SEE_THE_ENEMY";
	case NDb::ACK_GOING_TO_STORAGE:
		return "ACK_GOING_TO_STORAGE";
	case NDb::ACK_INVALID_TARGET:
		return "ACK_INVALID_TARGET";
	case NDb::ACK_KILLED_ENEMY:
		return "ACK_KILLED_ENEMY";
	case NDb::ACK_NEGATIVE:
		return "ACK_NEGATIVE";
	case NDb::ACK_NEGATIVE_NOTIFICATION:
		return "ACK_NEGATIVE_NOTIFICATION";
	case NDb::ACK_NO_AMMO:
		return "ACK_NO_AMMO";
	case NDb::ACK_NO_RESOURCES_CANT_FIND_DEPOT:
		return "ACK_NO_RESOURCES_CANT_FIND_DEPOT";
	case NDb::ACK_NOT_IN_ATTACK_ANGLE:
		return "ACK_NOT_IN_ATTACK_ANGLE";
	case NDb::ACK_NOT_IN_FIRE_RANGE:
		return "ACK_NOT_IN_FIRE_RANGE";
	case NDb::ACK_PLANE_LEAVING:
		return "ACK_PLANE_LEAVING";
	case NDb::ACK_PLANE_REACH_POINT_START_ATTACK:
		return "ACK_PLANE_REACH_POINT_START_ATTACK";
	case NDb::ACK_POSITIVE:
		return "ACK_POSITIVE";
	case NDb::ACK_SELECTED:
		return "ACK_SELECTED";
	case NDb::ACK_SELECTION_TO_MUCH:
		return "ACK_SELECTION_TO_MUCH";
	case NDb::ACK_START_SERVICE_REPAIR:
		return "ACK_START_SERVICE_REPAIR";
	case NDb::ACK_START_SERVICE_RESUPPLY:
		return "ACK_START_SERVICE_RESUPPLY";
	case NDb::ACK_UNIT_DIED:
		return "ACK_UNIT_DIED";
	case NDb::ACK_BEING_ATTACKED_BY_AVIATION:
		return "ACK_BEING_ATTACKED_BY_AVIATION";
	case NDb::____ACK_B2_SPECIFIC:
		return "____ACK_B2_SPECIFIC";
	case NDb::ACK_MOVE_END:
		return "ACK_MOVE_END";
	case NDb::ACK_UNDER_ATTACK:
		return "ACK_UNDER_ATTACK";
	case NDb::ACK_ENEMY_FOUND:
		return "ACK_ENEMY_FOUND";
	case NDb::ACK_ORDER_FINISHED:
		return "ACK_ORDER_FINISHED";
	case NDb::ACK_REINFORCEMENT_ARRIVED:
		return "ACK_REINFORCEMENT_ARRIVED";
	case NDb::ACK_MINE_FOUND:
		return "ACK_MINE_FOUND";
	case NDb::ACK_START_SERVICE_BUILDING:
		return "ACK_START_SERVICE_BUILDING";
	case NDb::ACK_LOW_AMMO:
		return "ACK_LOW_AMMO";
	case NDb::ACK_LOW_HIT_POINTS:
		return "ACK_LOW_HIT_POINTS";
	case NDb::____ACK_M1_SPECIFIC:
		return "____ACK_M1_SPECIFIC";
	case NDb::ACK_BORED_LOW_AMMO:
		return "ACK_BORED_LOW_AMMO";
	case NDb::ACK_BORED_LOW_HIT_POINTS:
		return "ACK_BORED_LOW_HIT_POINTS";
	case NDb::ACK_ENEMY_IS_TO_CLOSE:
		return "ACK_ENEMY_IS_TO_CLOSE";
	case NDb::ACK_ATTACKING_AVIATION:
		return "ACK_ATTACKING_AVIATION";
	case NDb::ACK_BORED_INFANTRY_TRAVEL:
		return "ACK_BORED_INFANTRY_TRAVEL";
	case NDb::ACK_BORED_MINIMUM_MORALE:
		return "ACK_BORED_MINIMUM_MORALE";
	case NDb::ACK_BORED_NO_AMMO:
		return "ACK_BORED_NO_AMMO";
	case NDb::ACK_BORED_RUSH:
		return "ACK_BORED_RUSH";
	case NDb::ACK_BORED_SNIPER_SNEAK:
		return "ACK_BORED_SNIPER_SNEAK";
	case NDb::ACK_CANNOT_FIND_PATH_TO_TARGET:
		return "ACK_CANNOT_FIND_PATH_TO_TARGET";
	case NDb::ACK_CANNOT_MOVE_NEED_TO_BE_TOWED_TO_MOVE:
		return "ACK_CANNOT_MOVE_NEED_TO_BE_TOWED_TO_MOVE";
	case NDb::ACK_CANNOT_MOVE_WAITING_FOR_LOADERS:
		return "ACK_CANNOT_MOVE_WAITING_FOR_LOADERS";
	case NDb::ACK_CANNOT_START_BUILD:
		return "ACK_CANNOT_START_BUILD";
	case NDb::ACK_CANNOT_SUPPLY_NOT_PATH:
		return "ACK_CANNOT_SUPPLY_NOT_PATH";
	case NDb::ACK_DIVEBOMBER_CANNOT_DIVE:
		return "ACK_DIVEBOMBER_CANNOT_DIVE";
	case NDb::ACK_ENEMY_ISNT_IN_FIRE_SECTOR:
		return "ACK_ENEMY_ISNT_IN_FIRE_SECTOR";
	case NDb::ACK_GETTING_AMMO:
		return "ACK_GETTING_AMMO";
	case NDb::ACK_KILLED_ENEMY_AVIATION:
		return "ACK_KILLED_ENEMY_AVIATION";
	case NDb::ACK_KILLED_ENEMY_INFANTRY:
		return "ACK_KILLED_ENEMY_INFANTRY";
	case NDb::ACK_KILLED_ENEMY_TANK:
		return "ACK_KILLED_ENEMY_TANK";
	case NDb::ACK_NEED_INSTALL:
		return "ACK_NEED_INSTALL";
	case NDb::ACK_NO_CANNOT_HOOK_ARTILLERY_NO_PATH:
		return "ACK_NO_CANNOT_HOOK_ARTILLERY_NO_PATH";
	case NDb::ACK_NO_CANNOT_UNHOOK_ARTILLERY_HERE:
		return "ACK_NO_CANNOT_UNHOOK_ARTILLERY_HERE";
	case NDb::ACK_NO_ENGINEERS_CANNOT_REACH_BUILDPOINT:
		return "ACK_NO_ENGINEERS_CANNOT_REACH_BUILDPOINT";
	case NDb::ACK_NO_RESOURCES_CANT_FIND_PATH_TO_DEPOT:
		return "ACK_NO_RESOURCES_CANT_FIND_PATH_TO_DEPOT";
	case NDb::ACK_NO_TOO_HEAVY_ARTILLERY_FOR_TRANSPORT:
		return "ACK_NO_TOO_HEAVY_ARTILLERY_FOR_TRANSPORT";
	case NDb::ACK_NOT_TRACEABLE:
		return "ACK_NOT_TRACEABLE";
	case NDb::ACK_PLANE_TAKING_OFF:
		return "ACK_PLANE_TAKING_OFF";
	case NDb::ACK_CANNOT_ENTER:
		return "ACK_CANNOT_ENTER";
	case NDb::ACK_CANNOT_STORM:
		return "ACK_CANNOT_STORM";
	case NDb::ACK_UNIT_BUSY:
		return "ACK_UNIT_BUSY";
	case NDb::ACK_CANNOT_LOAD:
		return "ACK_CANNOT_LOAD";
	case NDb::ACK_CANNOT_UNLOAD:
		return "ACK_CANNOT_UNLOAD";
	default:
		return "ACK_NONE";
	}
}

NDb::EUnitAckType NDb::StringToEnum_NDb_EUnitAckType( const string &szValue )
{
	if ( szValue == "ACK_NONE" )
		return NDb::ACK_NONE;
	if ( szValue == "ACK_BORED_ATTACK" )
		return NDb::ACK_BORED_ATTACK;
	if ( szValue == "ACK_BORED_IDLE" )
		return NDb::ACK_BORED_IDLE;
	if ( szValue == "ACK_BUILDING_FINISHED" )
		return NDb::ACK_BUILDING_FINISHED;
	if ( szValue == "ACK_CANNOT_FINISH_BUILD" )
		return NDb::ACK_CANNOT_FINISH_BUILD;
	if ( szValue == "ACK_CANNOT_MOVE_TRACK_DAMAGED" )
		return NDb::ACK_CANNOT_MOVE_TRACK_DAMAGED;
	if ( szValue == "ACK_CANNOT_PIERCE" )
		return NDb::ACK_CANNOT_PIERCE;
	if ( szValue == "ACK_DONT_SEE_THE_ENEMY" )
		return NDb::ACK_DONT_SEE_THE_ENEMY;
	if ( szValue == "ACK_GOING_TO_STORAGE" )
		return NDb::ACK_GOING_TO_STORAGE;
	if ( szValue == "ACK_INVALID_TARGET" )
		return NDb::ACK_INVALID_TARGET;
	if ( szValue == "ACK_KILLED_ENEMY" )
		return NDb::ACK_KILLED_ENEMY;
	if ( szValue == "ACK_NEGATIVE" )
		return NDb::ACK_NEGATIVE;
	if ( szValue == "ACK_NEGATIVE_NOTIFICATION" )
		return NDb::ACK_NEGATIVE_NOTIFICATION;
	if ( szValue == "ACK_NO_AMMO" )
		return NDb::ACK_NO_AMMO;
	if ( szValue == "ACK_NO_RESOURCES_CANT_FIND_DEPOT" )
		return NDb::ACK_NO_RESOURCES_CANT_FIND_DEPOT;
	if ( szValue == "ACK_NOT_IN_ATTACK_ANGLE" )
		return NDb::ACK_NOT_IN_ATTACK_ANGLE;
	if ( szValue == "ACK_NOT_IN_FIRE_RANGE" )
		return NDb::ACK_NOT_IN_FIRE_RANGE;
	if ( szValue == "ACK_PLANE_LEAVING" )
		return NDb::ACK_PLANE_LEAVING;
	if ( szValue == "ACK_PLANE_REACH_POINT_START_ATTACK" )
		return NDb::ACK_PLANE_REACH_POINT_START_ATTACK;
	if ( szValue == "ACK_POSITIVE" )
		return NDb::ACK_POSITIVE;
	if ( szValue == "ACK_SELECTED" )
		return NDb::ACK_SELECTED;
	if ( szValue == "ACK_SELECTION_TO_MUCH" )
		return NDb::ACK_SELECTION_TO_MUCH;
	if ( szValue == "ACK_START_SERVICE_REPAIR" )
		return NDb::ACK_START_SERVICE_REPAIR;
	if ( szValue == "ACK_START_SERVICE_RESUPPLY" )
		return NDb::ACK_START_SERVICE_RESUPPLY;
	if ( szValue == "ACK_UNIT_DIED" )
		return NDb::ACK_UNIT_DIED;
	if ( szValue == "ACK_BEING_ATTACKED_BY_AVIATION" )
		return NDb::ACK_BEING_ATTACKED_BY_AVIATION;
	if ( szValue == "____ACK_B2_SPECIFIC" )
		return NDb::____ACK_B2_SPECIFIC;
	if ( szValue == "ACK_MOVE_END" )
		return NDb::ACK_MOVE_END;
	if ( szValue == "ACK_UNDER_ATTACK" )
		return NDb::ACK_UNDER_ATTACK;
	if ( szValue == "ACK_ENEMY_FOUND" )
		return NDb::ACK_ENEMY_FOUND;
	if ( szValue == "ACK_ORDER_FINISHED" )
		return NDb::ACK_ORDER_FINISHED;
	if ( szValue == "ACK_REINFORCEMENT_ARRIVED" )
		return NDb::ACK_REINFORCEMENT_ARRIVED;
	if ( szValue == "ACK_MINE_FOUND" )
		return NDb::ACK_MINE_FOUND;
	if ( szValue == "ACK_START_SERVICE_BUILDING" )
		return NDb::ACK_START_SERVICE_BUILDING;
	if ( szValue == "ACK_LOW_AMMO" )
		return NDb::ACK_LOW_AMMO;
	if ( szValue == "ACK_LOW_HIT_POINTS" )
		return NDb::ACK_LOW_HIT_POINTS;
	if ( szValue == "____ACK_M1_SPECIFIC" )
		return NDb::____ACK_M1_SPECIFIC;
	if ( szValue == "ACK_BORED_LOW_AMMO" )
		return NDb::ACK_BORED_LOW_AMMO;
	if ( szValue == "ACK_BORED_LOW_HIT_POINTS" )
		return NDb::ACK_BORED_LOW_HIT_POINTS;
	if ( szValue == "ACK_ENEMY_IS_TO_CLOSE" )
		return NDb::ACK_ENEMY_IS_TO_CLOSE;
	if ( szValue == "ACK_ATTACKING_AVIATION" )
		return NDb::ACK_ATTACKING_AVIATION;
	if ( szValue == "ACK_BORED_INFANTRY_TRAVEL" )
		return NDb::ACK_BORED_INFANTRY_TRAVEL;
	if ( szValue == "ACK_BORED_MINIMUM_MORALE" )
		return NDb::ACK_BORED_MINIMUM_MORALE;
	if ( szValue == "ACK_BORED_NO_AMMO" )
		return NDb::ACK_BORED_NO_AMMO;
	if ( szValue == "ACK_BORED_RUSH" )
		return NDb::ACK_BORED_RUSH;
	if ( szValue == "ACK_BORED_SNIPER_SNEAK" )
		return NDb::ACK_BORED_SNIPER_SNEAK;
	if ( szValue == "ACK_CANNOT_FIND_PATH_TO_TARGET" )
		return NDb::ACK_CANNOT_FIND_PATH_TO_TARGET;
	if ( szValue == "ACK_CANNOT_MOVE_NEED_TO_BE_TOWED_TO_MOVE" )
		return NDb::ACK_CANNOT_MOVE_NEED_TO_BE_TOWED_TO_MOVE;
	if ( szValue == "ACK_CANNOT_MOVE_WAITING_FOR_LOADERS" )
		return NDb::ACK_CANNOT_MOVE_WAITING_FOR_LOADERS;
	if ( szValue == "ACK_CANNOT_START_BUILD" )
		return NDb::ACK_CANNOT_START_BUILD;
	if ( szValue == "ACK_CANNOT_SUPPLY_NOT_PATH" )
		return NDb::ACK_CANNOT_SUPPLY_NOT_PATH;
	if ( szValue == "ACK_DIVEBOMBER_CANNOT_DIVE" )
		return NDb::ACK_DIVEBOMBER_CANNOT_DIVE;
	if ( szValue == "ACK_ENEMY_ISNT_IN_FIRE_SECTOR" )
		return NDb::ACK_ENEMY_ISNT_IN_FIRE_SECTOR;
	if ( szValue == "ACK_GETTING_AMMO" )
		return NDb::ACK_GETTING_AMMO;
	if ( szValue == "ACK_KILLED_ENEMY_AVIATION" )
		return NDb::ACK_KILLED_ENEMY_AVIATION;
	if ( szValue == "ACK_KILLED_ENEMY_INFANTRY" )
		return NDb::ACK_KILLED_ENEMY_INFANTRY;
	if ( szValue == "ACK_KILLED_ENEMY_TANK" )
		return NDb::ACK_KILLED_ENEMY_TANK;
	if ( szValue == "ACK_NEED_INSTALL" )
		return NDb::ACK_NEED_INSTALL;
	if ( szValue == "ACK_NO_CANNOT_HOOK_ARTILLERY_NO_PATH" )
		return NDb::ACK_NO_CANNOT_HOOK_ARTILLERY_NO_PATH;
	if ( szValue == "ACK_NO_CANNOT_UNHOOK_ARTILLERY_HERE" )
		return NDb::ACK_NO_CANNOT_UNHOOK_ARTILLERY_HERE;
	if ( szValue == "ACK_NO_ENGINEERS_CANNOT_REACH_BUILDPOINT" )
		return NDb::ACK_NO_ENGINEERS_CANNOT_REACH_BUILDPOINT;
	if ( szValue == "ACK_NO_RESOURCES_CANT_FIND_PATH_TO_DEPOT" )
		return NDb::ACK_NO_RESOURCES_CANT_FIND_PATH_TO_DEPOT;
	if ( szValue == "ACK_NO_TOO_HEAVY_ARTILLERY_FOR_TRANSPORT" )
		return NDb::ACK_NO_TOO_HEAVY_ARTILLERY_FOR_TRANSPORT;
	if ( szValue == "ACK_NOT_TRACEABLE" )
		return NDb::ACK_NOT_TRACEABLE;
	if ( szValue == "ACK_PLANE_TAKING_OFF" )
		return NDb::ACK_PLANE_TAKING_OFF;
	if ( szValue == "ACK_CANNOT_ENTER" )
		return NDb::ACK_CANNOT_ENTER;
	if ( szValue == "ACK_CANNOT_STORM" )
		return NDb::ACK_CANNOT_STORM;
	if ( szValue == "ACK_UNIT_BUSY" )
		return NDb::ACK_UNIT_BUSY;
	if ( szValue == "ACK_CANNOT_LOAD" )
		return NDb::ACK_CANNOT_LOAD;
	if ( szValue == "ACK_CANNOT_UNLOAD" )
		return NDb::ACK_CANNOT_UNLOAD;
	return NDb::ACK_NONE;
}

string EnumToString( NDb::EAckClass eValue )
{
	switch ( eValue )
	{
	case NDb::ACKT_POSITIVE:
		return "ACKT_POSITIVE";
	case NDb::ACKT_NEGATIVE:
		return "ACKT_NEGATIVE";
	case NDb::ACKT_SELECTION:
		return "ACKT_SELECTION";
	case NDb::ACKT_NOTIFY:
		return "ACKT_NOTIFY";
	case NDb::ACKT_BORED:
		return "ACKT_BORED";
	default:
		return "ACKT_POSITIVE";
	}
}

NDb::EAckClass NDb::StringToEnum_NDb_EAckClass( const string &szValue )
{
	if ( szValue == "ACKT_POSITIVE" )
		return NDb::ACKT_POSITIVE;
	if ( szValue == "ACKT_NEGATIVE" )
		return NDb::ACKT_NEGATIVE;
	if ( szValue == "ACKT_SELECTION" )
		return NDb::ACKT_SELECTION;
	if ( szValue == "ACKT_NOTIFY" )
		return NDb::ACKT_NOTIFY;
	if ( szValue == "ACKT_BORED" )
		return NDb::ACKT_BORED;
	return NDb::ACKT_POSITIVE;
}

string EnumToString( NDb::EAckPosition eValue )
{
	switch ( eValue )
	{
	case NDb::ACK_POS_UNIT:
		return "ACK_POS_UNIT";
	case NDb::ACK_POS_INTERFACE:
		return "ACK_POS_INTERFACE";
	default:
		return "ACK_POS_UNIT";
	}
}

NDb::EAckPosition NDb::StringToEnum_NDb_EAckPosition( const string &szValue )
{
	if ( szValue == "ACK_POS_UNIT" )
		return NDb::ACK_POS_UNIT;
	if ( szValue == "ACK_POS_INTERFACE" )
		return NDb::ACK_POS_INTERFACE;
	return NDb::ACK_POS_UNIT;
}


void SAckType::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "AckType", (BYTE*)&eAckType - pThis, sizeof(eAckType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Ack", (BYTE*)&pAck - pThis, sizeof(pAck), NTypeDef::TYPE_TYPE_REF );
}

int SAckType::operator&( IXmlSaver &saver )
{
	saver.Add( "AckType", &eAckType );
	saver.Add( "Ack", &pAck );

	return 0;
}

int SAckType::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eAckType );
	saver.Add( 3, &pAck );

	return 0;
}

DWORD SAckType::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eAckType;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
