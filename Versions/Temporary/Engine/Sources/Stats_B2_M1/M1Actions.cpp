// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "m1actions.h"

namespace NDb
{


string EnumToString( NDb::EM1Action eValue )
{
	switch ( eValue )
	{
	case NDb::M1_ACTION_UNKNOWN:
		return "M1_ACTION_UNKNOWN";
	case NDb::M1_ACTION_MOVE:
		return "M1_ACTION_MOVE";
	case NDb::M1_ACTION_ATTACK:
		return "M1_ACTION_ATTACK";
	case NDb::M1_ACTION_STOP:
		return "M1_ACTION_STOP";
	case NDb::M1_ACTION_ROTATE:
		return "M1_ACTION_ROTATE";
	case NDb::M1_ACTION_LEAVE:
		return "M1_ACTION_LEAVE";
	case NDb::M1_ACTION_LAND_MINE:
		return "M1_ACTION_LAND_MINE";
	case NDb::M1_ACTION_BUILD:
		return "M1_ACTION_BUILD";
	case NDb::M1_ACTION_CAPTURE:
		return "M1_ACTION_CAPTURE";
	case NDb::M1_ACTION_STORM:
		return "M1_ACTION_STORM";
	case NDb::M1_ACTION_LOAD:
		return "M1_ACTION_LOAD";
	case NDb::M1_ACTION_UNLOAD:
		return "M1_ACTION_UNLOAD";
	case NDb::M1_ACTION_LASER_MARK:
		return "M1_ACTION_LASER_MARK";
	case NDb::M1_ACTION_BOARD:
		return "M1_ACTION_BOARD";
	case NDb::M1_ACTION_ENTER_SIEGE_MODE:
		return "M1_ACTION_ENTER_SIEGE_MODE";
	case NDb::M1_ACTION_LEAVE_SIEGE_MODE:
		return "M1_ACTION_LEAVE_SIEGE_MODE";
	case NDb::M1_ACTION_GLOBE_BOMB_MISSION:
		return "M1_ACTION_GLOBE_BOMB_MISSION";
	default:
		return "M1_ACTION_UNKNOWN";
	}
}

NDb::EM1Action NDb::StringToEnum_NDb_EM1Action( const string &szValue )
{
	if ( szValue == "M1_ACTION_UNKNOWN" )
		return NDb::M1_ACTION_UNKNOWN;
	if ( szValue == "M1_ACTION_MOVE" )
		return NDb::M1_ACTION_MOVE;
	if ( szValue == "M1_ACTION_ATTACK" )
		return NDb::M1_ACTION_ATTACK;
	if ( szValue == "M1_ACTION_STOP" )
		return NDb::M1_ACTION_STOP;
	if ( szValue == "M1_ACTION_ROTATE" )
		return NDb::M1_ACTION_ROTATE;
	if ( szValue == "M1_ACTION_LEAVE" )
		return NDb::M1_ACTION_LEAVE;
	if ( szValue == "M1_ACTION_LAND_MINE" )
		return NDb::M1_ACTION_LAND_MINE;
	if ( szValue == "M1_ACTION_BUILD" )
		return NDb::M1_ACTION_BUILD;
	if ( szValue == "M1_ACTION_CAPTURE" )
		return NDb::M1_ACTION_CAPTURE;
	if ( szValue == "M1_ACTION_STORM" )
		return NDb::M1_ACTION_STORM;
	if ( szValue == "M1_ACTION_LOAD" )
		return NDb::M1_ACTION_LOAD;
	if ( szValue == "M1_ACTION_UNLOAD" )
		return NDb::M1_ACTION_UNLOAD;
	if ( szValue == "M1_ACTION_LASER_MARK" )
		return NDb::M1_ACTION_LASER_MARK;
	if ( szValue == "M1_ACTION_BOARD" )
		return NDb::M1_ACTION_BOARD;
	if ( szValue == "M1_ACTION_ENTER_SIEGE_MODE" )
		return NDb::M1_ACTION_ENTER_SIEGE_MODE;
	if ( szValue == "M1_ACTION_LEAVE_SIEGE_MODE" )
		return NDb::M1_ACTION_LEAVE_SIEGE_MODE;
	if ( szValue == "M1_ACTION_GLOBE_BOMB_MISSION" )
		return NDb::M1_ACTION_GLOBE_BOMB_MISSION;
	return NDb::M1_ACTION_UNKNOWN;
}
}
using namespace NDb;

