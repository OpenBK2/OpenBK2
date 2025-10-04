// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "Season.h"

namespace NDb
{


string EnumToString( NDb::ESeason eValue )
{
	switch ( eValue )
	{
	case NDb::SEASON_WINTER:
		return "SEASON_WINTER";
	case NDb::SEASON_SPRING:
		return "SEASON_SPRING";
	case NDb::SEASON_SUMMER:
		return "SEASON_SUMMER";
	case NDb::SEASON_AUTUMN:
		return "SEASON_AUTUMN";
	case NDb::SEASON_AFRICA:
		return "SEASON_AFRICA";
	case NDb::SEASON_ASIA:
		return "SEASON_ASIA";
	default:
		return "SEASON_WINTER";
	}
}

NDb::ESeason NDb::StringToEnum_NDb_ESeason( const string &szValue )
{
	if ( szValue == "SEASON_WINTER" )
		return NDb::SEASON_WINTER;
	if ( szValue == "SEASON_SPRING" )
		return NDb::SEASON_SPRING;
	if ( szValue == "SEASON_SUMMER" )
		return NDb::SEASON_SUMMER;
	if ( szValue == "SEASON_AUTUMN" )
		return NDb::SEASON_AUTUMN;
	if ( szValue == "SEASON_AFRICA" )
		return NDb::SEASON_AFRICA;
	if ( szValue == "SEASON_ASIA" )
		return NDb::SEASON_ASIA;
	return NDb::SEASON_WINTER;
}

string EnumToString( NDb::EDayNight eValue )
{
	switch ( eValue )
	{
	case NDb::DAY_MORNING:
		return "DAY_MORNING";
	case NDb::DAY_DAY:
		return "DAY_DAY";
	case NDb::DAY_EVENING:
		return "DAY_EVENING";
	case NDb::DAY_NIGHT:
		return "DAY_NIGHT";
	default:
		return "DAY_MORNING";
	}
}

NDb::EDayNight NDb::StringToEnum_NDb_EDayNight( const string &szValue )
{
	if ( szValue == "DAY_MORNING" )
		return NDb::DAY_MORNING;
	if ( szValue == "DAY_DAY" )
		return NDb::DAY_DAY;
	if ( szValue == "DAY_EVENING" )
		return NDb::DAY_EVENING;
	if ( szValue == "DAY_NIGHT" )
		return NDb::DAY_NIGHT;
	return NDb::DAY_MORNING;
}
}
using namespace NDb;

