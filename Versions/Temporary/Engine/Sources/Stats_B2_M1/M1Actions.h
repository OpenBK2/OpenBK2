#pragma once

#include "Stats_B2_M1_export.h"


// automatically generated file, don't change manually!


struct IXmlSaver;

namespace NDb
{

	enum EM1Action : int
	{
		M1_ACTION_UNKNOWN = 0,
		M1_ACTION_MOVE = 1,
		M1_ACTION_ATTACK = 2,
		M1_ACTION_STOP = 3,
		M1_ACTION_ROTATE = 4,
		M1_ACTION_LEAVE = 5,
		M1_ACTION_LAND_MINE = 6,
		M1_ACTION_BUILD = 7,
		M1_ACTION_CAPTURE = 8,
		M1_ACTION_STORM = 9,
		M1_ACTION_LOAD = 10,
		M1_ACTION_UNLOAD = 11,
		M1_ACTION_LASER_MARK = 12,
		M1_ACTION_BOARD = 13,
		M1_ACTION_ENTER_SIEGE_MODE = 14,
		M1_ACTION_LEAVE_SIEGE_MODE = 15,
		M1_ACTION_GLOBE_BOMB_MISSION = 16,
	};
}

namespace NDb
{
	STATS_B2_M1_EXPORT std::string EnumToString( NDb::EM1Action eValue );
	STATS_B2_M1_EXPORT EM1Action StringToEnum_NDb_EM1Action( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EM1Action>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EM1Action eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EM1Action ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EM1Action( szValue ); }
};

