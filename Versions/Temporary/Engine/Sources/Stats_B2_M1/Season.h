#pragma once

#include "Stats_B2_M1_export.h"


// automatically generated file, don't change manually!


interface IXmlSaver;

namespace NDb
{

	enum ESeason
	{
		SEASON_WINTER = 0,
		SEASON_SPRING = 1,
		SEASON_SUMMER = 2,
		SEASON_AUTUMN = 3,
		SEASON_AFRICA = 4,
		SEASON_ASIA = 5,
	};

	enum EDayNight
	{
		DAY_MORNING = 0,
		DAY_DAY = 1,
		DAY_EVENING = 2,
		DAY_NIGHT = 3,
	};
}

namespace NDb
{
	STATS_B2_M1_EXPORT string EnumToString( NDb::ESeason eValue );
	STATS_B2_M1_EXPORT ESeason StringToEnum_NDb_ESeason( const string &szValue );
}

template <>
struct SKnownEnum<NDb::ESeason>
{
	enum { isKnown = 1 };
	static string ToString( NDb::ESeason eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::ESeason ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_ESeason( szValue ); }
};

namespace NDb
{
	string EnumToString( NDb::EDayNight eValue );
	EDayNight StringToEnum_NDb_EDayNight( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EDayNight>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EDayNight eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EDayNight ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EDayNight( szValue ); }
};

