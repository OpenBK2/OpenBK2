// automatically generated file, don't change manually!

#include "stdafx.h"

#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "RPGStats.h"
#include "AnimationType.h"
#include "DBAnimB2.h"
#include "DBVisObj.h"
#include "include_infantryrpgstats_cpp.h"
#include "include_mechunitrpgstats_cpp.h"
#include "include_unitbaserpgstats_cpp.h"

namespace NDb
{


std::string EnumToString( NDb::EUnitSpecialAbility eValue )
{
	switch ( eValue )
	{
	case NDb::ABILITY_NOT_ABILITY:
		return "ABILITY_NOT_ABILITY";
	case NDb::ABILITY_PLACE_CHARGE:
		return "ABILITY_PLACE_CHARGE";
	case NDb::ABILITY_PLACE_CONTROLLED_CHARGE:
		return "ABILITY_PLACE_CONTROLLED_CHARGE";
	case NDb::ABILITY_DETONATE:
		return "ABILITY_DETONATE";
	case NDb::ABILITY_EXACT_SHOT:
		return "ABILITY_EXACT_SHOT";
	case NDb::ABILITY_CRITICAL_TARGETING:
		return "ABILITY_CRITICAL_TARGETING";
	case NDb::ABILITY_RAPID_SHOT_MODE:
		return "ABILITY_RAPID_SHOT_MODE";
	case NDb::ABILITY_REPAIR_LESSER_DAMAGE:
		return "ABILITY_REPAIR_LESSER_DAMAGE";
	case NDb::ABILITY_HOLD_SECTOR:
		return "ABILITY_HOLD_SECTOR";
	case NDb::ABILITY_TRACK_TARGETING:
		return "ABILITY_TRACK_TARGETING";
	case NDb::ABILITY_HIGH_PASSABLENESS_MODE:
		return "ABILITY_HIGH_PASSABLENESS_MODE";
	case NDb::ABILITY_MANUVERABLE_FIGHT_MODE:
		return "ABILITY_MANUVERABLE_FIGHT_MODE";
	case NDb::ABILITY_USE_BINOCULAR:
		return "ABILITY_USE_BINOCULAR";
	case NDb::ABILITY_REMOVE_MINE_FIELD:
		return "ABILITY_REMOVE_MINE_FIELD";
	case NDb::ABILITY_RAPID_FIRE_MODE:
		return "ABILITY_RAPID_FIRE_MODE";
	case NDb::ABILITY_SET_MINE_FIELD:
		return "ABILITY_SET_MINE_FIELD";
	case NDb::ABILITY_DIG_TRENCHES:
		return "ABILITY_DIG_TRENCHES";
	case NDb::ABILITY_ADRENALINE_RUSH:
		return "ABILITY_ADRENALINE_RUSH";
	case NDb::ABILITY_DIE_HARD:
		return "ABILITY_DIE_HARD";
	case NDb::ABILITY_BUILD_OBSTACLES:
		return "ABILITY_BUILD_OBSTACLES";
	case NDb::ABILITY_CAMOFLAGE_MODE:
		return "ABILITY_CAMOFLAGE_MODE";
	case NDb::ABILITY_ADAVNCED_CAMOFLAGE_MODE:
		return "ABILITY_ADAVNCED_CAMOFLAGE_MODE";
	case NDb::ABILITY_THROW_GRENADE:
		return "ABILITY_THROW_GRENADE";
	case NDb::ABILITY_THROW_ANTITANK_GRENADE:
		return "ABILITY_THROW_ANTITANK_GRENADE";
	case NDb::ABILITY_SPY_MODE:
		return "ABILITY_SPY_MODE";
	case NDb::ABILITY_LOW_HEIGHT_MODE:
		return "ABILITY_LOW_HEIGHT_MODE";
	case NDb::ABILITY_OVERLOAD_MODE:
		return "ABILITY_OVERLOAD_MODE";
	case NDb::ABILITY_DROP_BOMBS:
		return "ABILITY_DROP_BOMBS";
	case NDb::ABILITY_FIRE_AA_MISSILES:
		return "ABILITY_FIRE_AA_MISSILES";
	case NDb::ABILITY_FIRE_AA_GUIDED_MISSILES:
		return "ABILITY_FIRE_AA_GUIDED_MISSILES";
	case NDb::ABILITY_FIRE_AS_MISSILES:
		return "ABILITY_FIRE_AS_MISSILES";
	case NDb::ABILITY_REPEAT_LAST_SHOT:
		return "ABILITY_REPEAT_LAST_SHOT";
	case NDb::ABILITY_AREA_FIRE:
		return "ABILITY_AREA_FIRE";
	case NDb::ABILITY_SMOKE_SHOTS:
		return "ABILITY_SMOKE_SHOTS";
	case NDb::ABILITY_ANTIATRILLERY_FIRE:
		return "ABILITY_ANTIATRILLERY_FIRE";
	case NDb::ABILITY_TAKE_HIDED_REINFORCEMENT:
		return "ABILITY_TAKE_HIDED_REINFORCEMENT";
	case NDb::ABILITY_COMMAND_VOICE:
		return "ABILITY_COMMAND_VOICE";
	case NDb::ABILITY_AUTHORITY:
		return "ABILITY_AUTHORITY";
	case NDb::ABILITY_ARMOR_TUTOR:
		return "ABILITY_ARMOR_TUTOR";
	case NDb::ABILITY_TAKE_PLACE:
		return "ABILITY_TAKE_PLACE";
	case NDb::ABILITY_LAND_MINE:
		return "ABILITY_LAND_MINE";
	case NDb::ABILITY_ENTRENCH_SELF:
		return "ABILITY_ENTRENCH_SELF";
	case NDb::ABILITY_SUPRESS:
		return "ABILITY_SUPRESS";
	case NDb::ABILITY_AMBUSH:
		return "ABILITY_AMBUSH";
	case NDb::ABILITY_MOBILE_FORTRESS:
		return "ABILITY_MOBILE_FORTRESS";
	case NDb::ABILITY_FLAMETHROWER:
		return "ABILITY_FLAMETHROWER";
	case NDb::ABILITY_CAUTION:
		return "ABILITY_CAUTION";
	case NDb::ABILITY_FIRST_AID:
		return "ABILITY_FIRST_AID";
	case NDb::ABILITY_LINKED_GRENADES:
		return "ABILITY_LINKED_GRENADES";
	case NDb::ABILITY_MASTER_OF_STREETS:
		return "ABILITY_MASTER_OF_STREETS";
	case NDb::ABILITY_ZEROING_IN:
		return "ABILITY_ZEROING_IN";
	case NDb::ABILITY_SUPPORT_FIRE:
		return "ABILITY_SUPPORT_FIRE";
	case NDb::ABILITY_COVER_FIRE:
		return "ABILITY_COVER_FIRE";
	case NDb::ABILITY_PATROL:
		return "ABILITY_PATROL";
	case NDb::ABILITY_EXACT_BOMBING:
		return "ABILITY_EXACT_BOMBING";
	case NDb::ABILITY_DROP_PARATROOPERS:
		return "ABILITY_DROP_PARATROOPERS";
	case NDb::ABILITY_MASTER_PILOT:
		return "ABILITY_MASTER_PILOT";
	case NDb::ABILITY_SKY_GUARD:
		return "ABILITY_SKY_GUARD";
	case NDb::ABILITY_SURVIVAL:
		return "ABILITY_SURVIVAL";
	case NDb::ABILITY_TANK_HUNTER:
		return "ABILITY_TANK_HUNTER";
	case NDb::ABILITY_RADIO_CONTROLLED_MODE:
		return "ABILITY_RADIO_CONTROLLED_MODE";
	case NDb::_ABILITY_COUNT:
		return "_ABILITY_COUNT";
	default:
		return "ABILITY_NOT_ABILITY";
	}
}

NDb::EUnitSpecialAbility NDb::StringToEnum_NDb_EUnitSpecialAbility( const std::string &szValue )
{
	if ( szValue == "ABILITY_NOT_ABILITY" )
		return NDb::ABILITY_NOT_ABILITY;
	if ( szValue == "ABILITY_PLACE_CHARGE" )
		return NDb::ABILITY_PLACE_CHARGE;
	if ( szValue == "ABILITY_PLACE_CONTROLLED_CHARGE" )
		return NDb::ABILITY_PLACE_CONTROLLED_CHARGE;
	if ( szValue == "ABILITY_DETONATE" )
		return NDb::ABILITY_DETONATE;
	if ( szValue == "ABILITY_EXACT_SHOT" )
		return NDb::ABILITY_EXACT_SHOT;
	if ( szValue == "ABILITY_CRITICAL_TARGETING" )
		return NDb::ABILITY_CRITICAL_TARGETING;
	if ( szValue == "ABILITY_RAPID_SHOT_MODE" )
		return NDb::ABILITY_RAPID_SHOT_MODE;
	if ( szValue == "ABILITY_REPAIR_LESSER_DAMAGE" )
		return NDb::ABILITY_REPAIR_LESSER_DAMAGE;
	if ( szValue == "ABILITY_HOLD_SECTOR" )
		return NDb::ABILITY_HOLD_SECTOR;
	if ( szValue == "ABILITY_TRACK_TARGETING" )
		return NDb::ABILITY_TRACK_TARGETING;
	if ( szValue == "ABILITY_HIGH_PASSABLENESS_MODE" )
		return NDb::ABILITY_HIGH_PASSABLENESS_MODE;
	if ( szValue == "ABILITY_MANUVERABLE_FIGHT_MODE" )
		return NDb::ABILITY_MANUVERABLE_FIGHT_MODE;
	if ( szValue == "ABILITY_USE_BINOCULAR" )
		return NDb::ABILITY_USE_BINOCULAR;
	if ( szValue == "ABILITY_REMOVE_MINE_FIELD" )
		return NDb::ABILITY_REMOVE_MINE_FIELD;
	if ( szValue == "ABILITY_RAPID_FIRE_MODE" )
		return NDb::ABILITY_RAPID_FIRE_MODE;
	if ( szValue == "ABILITY_SET_MINE_FIELD" )
		return NDb::ABILITY_SET_MINE_FIELD;
	if ( szValue == "ABILITY_DIG_TRENCHES" )
		return NDb::ABILITY_DIG_TRENCHES;
	if ( szValue == "ABILITY_ADRENALINE_RUSH" )
		return NDb::ABILITY_ADRENALINE_RUSH;
	if ( szValue == "ABILITY_DIE_HARD" )
		return NDb::ABILITY_DIE_HARD;
	if ( szValue == "ABILITY_BUILD_OBSTACLES" )
		return NDb::ABILITY_BUILD_OBSTACLES;
	if ( szValue == "ABILITY_CAMOFLAGE_MODE" )
		return NDb::ABILITY_CAMOFLAGE_MODE;
	if ( szValue == "ABILITY_ADAVNCED_CAMOFLAGE_MODE" )
		return NDb::ABILITY_ADAVNCED_CAMOFLAGE_MODE;
	if ( szValue == "ABILITY_THROW_GRENADE" )
		return NDb::ABILITY_THROW_GRENADE;
	if ( szValue == "ABILITY_THROW_ANTITANK_GRENADE" )
		return NDb::ABILITY_THROW_ANTITANK_GRENADE;
	if ( szValue == "ABILITY_SPY_MODE" )
		return NDb::ABILITY_SPY_MODE;
	if ( szValue == "ABILITY_LOW_HEIGHT_MODE" )
		return NDb::ABILITY_LOW_HEIGHT_MODE;
	if ( szValue == "ABILITY_OVERLOAD_MODE" )
		return NDb::ABILITY_OVERLOAD_MODE;
	if ( szValue == "ABILITY_DROP_BOMBS" )
		return NDb::ABILITY_DROP_BOMBS;
	if ( szValue == "ABILITY_FIRE_AA_MISSILES" )
		return NDb::ABILITY_FIRE_AA_MISSILES;
	if ( szValue == "ABILITY_FIRE_AA_GUIDED_MISSILES" )
		return NDb::ABILITY_FIRE_AA_GUIDED_MISSILES;
	if ( szValue == "ABILITY_FIRE_AS_MISSILES" )
		return NDb::ABILITY_FIRE_AS_MISSILES;
	if ( szValue == "ABILITY_REPEAT_LAST_SHOT" )
		return NDb::ABILITY_REPEAT_LAST_SHOT;
	if ( szValue == "ABILITY_AREA_FIRE" )
		return NDb::ABILITY_AREA_FIRE;
	if ( szValue == "ABILITY_SMOKE_SHOTS" )
		return NDb::ABILITY_SMOKE_SHOTS;
	if ( szValue == "ABILITY_ANTIATRILLERY_FIRE" )
		return NDb::ABILITY_ANTIATRILLERY_FIRE;
	if ( szValue == "ABILITY_TAKE_HIDED_REINFORCEMENT" )
		return NDb::ABILITY_TAKE_HIDED_REINFORCEMENT;
	if ( szValue == "ABILITY_COMMAND_VOICE" )
		return NDb::ABILITY_COMMAND_VOICE;
	if ( szValue == "ABILITY_AUTHORITY" )
		return NDb::ABILITY_AUTHORITY;
	if ( szValue == "ABILITY_ARMOR_TUTOR" )
		return NDb::ABILITY_ARMOR_TUTOR;
	if ( szValue == "ABILITY_TAKE_PLACE" )
		return NDb::ABILITY_TAKE_PLACE;
	if ( szValue == "ABILITY_LAND_MINE" )
		return NDb::ABILITY_LAND_MINE;
	if ( szValue == "ABILITY_ENTRENCH_SELF" )
		return NDb::ABILITY_ENTRENCH_SELF;
	if ( szValue == "ABILITY_SUPRESS" )
		return NDb::ABILITY_SUPRESS;
	if ( szValue == "ABILITY_AMBUSH" )
		return NDb::ABILITY_AMBUSH;
	if ( szValue == "ABILITY_MOBILE_FORTRESS" )
		return NDb::ABILITY_MOBILE_FORTRESS;
	if ( szValue == "ABILITY_FLAMETHROWER" )
		return NDb::ABILITY_FLAMETHROWER;
	if ( szValue == "ABILITY_CAUTION" )
		return NDb::ABILITY_CAUTION;
	if ( szValue == "ABILITY_FIRST_AID" )
		return NDb::ABILITY_FIRST_AID;
	if ( szValue == "ABILITY_LINKED_GRENADES" )
		return NDb::ABILITY_LINKED_GRENADES;
	if ( szValue == "ABILITY_MASTER_OF_STREETS" )
		return NDb::ABILITY_MASTER_OF_STREETS;
	if ( szValue == "ABILITY_ZEROING_IN" )
		return NDb::ABILITY_ZEROING_IN;
	if ( szValue == "ABILITY_SUPPORT_FIRE" )
		return NDb::ABILITY_SUPPORT_FIRE;
	if ( szValue == "ABILITY_COVER_FIRE" )
		return NDb::ABILITY_COVER_FIRE;
	if ( szValue == "ABILITY_PATROL" )
		return NDb::ABILITY_PATROL;
	if ( szValue == "ABILITY_EXACT_BOMBING" )
		return NDb::ABILITY_EXACT_BOMBING;
	if ( szValue == "ABILITY_DROP_PARATROOPERS" )
		return NDb::ABILITY_DROP_PARATROOPERS;
	if ( szValue == "ABILITY_MASTER_PILOT" )
		return NDb::ABILITY_MASTER_PILOT;
	if ( szValue == "ABILITY_SKY_GUARD" )
		return NDb::ABILITY_SKY_GUARD;
	if ( szValue == "ABILITY_SURVIVAL" )
		return NDb::ABILITY_SURVIVAL;
	if ( szValue == "ABILITY_TANK_HUNTER" )
		return NDb::ABILITY_TANK_HUNTER;
	if ( szValue == "ABILITY_RADIO_CONTROLLED_MODE" )
		return NDb::ABILITY_RADIO_CONTROLLED_MODE;
	if ( szValue == "_ABILITY_COUNT" )
		return NDb::_ABILITY_COUNT;
	return NDb::ABILITY_NOT_ABILITY;
}

std::string EnumToString( NDb::EUnitSpecialAbilityGroup eValue )
{
	switch ( eValue )
	{
	case NDb::ABILITY_GROUP_NOGROUP:
		return "ABILITY_GROUP_NOGROUP";
	case NDb::ABILITY_GROUP_CAMOFLAGE:
		return "ABILITY_GROUP_CAMOFLAGE";
	default:
		return "ABILITY_GROUP_NOGROUP";
	}
}

NDb::EUnitSpecialAbilityGroup NDb::StringToEnum_NDb_EUnitSpecialAbilityGroup( const std::string &szValue )
{
	if ( szValue == "ABILITY_GROUP_NOGROUP" )
		return NDb::ABILITY_GROUP_NOGROUP;
	if ( szValue == "ABILITY_GROUP_CAMOFLAGE" )
		return NDb::ABILITY_GROUP_CAMOFLAGE;
	return NDb::ABILITY_GROUP_NOGROUP;
}


void SUnitSpecialAblityDesc::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UnitSpecialAblityDesc", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Name", (BYTE*)&eName - pThis, sizeof(eName), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "RefreshTime", (BYTE*)&nRefreshTime - pThis, sizeof(nRefreshTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "WorkTime", (BYTE*)&nWorkTime - pThis, sizeof(nWorkTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "SwitchOnTime", (BYTE*)&nSwitchOnTime - pThis, sizeof(nSwitchOnTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "SwitchOffTime", (BYTE*)&nSwitchOffTime - pThis, sizeof(nSwitchOffTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "GroupID", (BYTE*)&eGroupID - pThis, sizeof(eGroupID), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "DisableGroupTime", (BYTE*)&nDisableGroupTime - pThis, sizeof(nDisableGroupTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "StatsBonus", (BYTE*)&pStatsBonus - pThis, sizeof(pStatsBonus), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Parameter", (BYTE*)&fParameter - pThis, sizeof(fParameter), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "StopCurrentAction", (BYTE*)&bStopCurrentAction - pThis, sizeof(bStopCurrentAction), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "LocalizedNameFileRef", (BYTE*)&szLocalizedNameFileRef - pThis, sizeof(szLocalizedNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "LocalizedDescFileRef", (BYTE*)&szLocalizedDescFileRef - pThis, sizeof(szLocalizedDescFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SUnitSpecialAblityDesc::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Name", &eName );
	saver.Add( "RefreshTime", &nRefreshTime );
	saver.Add( "WorkTime", &nWorkTime );
	saver.Add( "SwitchOnTime", &nSwitchOnTime );
	saver.Add( "SwitchOffTime", &nSwitchOffTime );
	saver.Add( "GroupID", &eGroupID );
	saver.Add( "DisableGroupTime", &nDisableGroupTime );
	saver.Add( "StatsBonus", &pStatsBonus );
	saver.Add( "Parameter", &fParameter );
	saver.Add( "StopCurrentAction", &bStopCurrentAction );
	saver.Add( "LocalizedNameFileRef", &szLocalizedNameFileRef );
	saver.Add( "LocalizedDescFileRef", &szLocalizedDescFileRef );

	return 0;
}

int SUnitSpecialAblityDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eName );
	saver.Add( 3, &nRefreshTime );
	saver.Add( 4, &nWorkTime );
	saver.Add( 5, &nSwitchOnTime );
	saver.Add( 6, &nSwitchOffTime );
	saver.Add( 7, &eGroupID );
	saver.Add( 8, &nDisableGroupTime );
	saver.Add( 9, &pStatsBonus );
	saver.Add( 10, &fParameter );
	saver.Add( 11, &bStopCurrentAction );
	saver.Add( 12, &szLocalizedNameFileRef );
	saver.Add( 13, &szLocalizedDescFileRef );

	return 0;
}

DWORD SUnitSpecialAblityDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eName << nRefreshTime << nWorkTime << nSwitchOnTime << nSwitchOffTime << eGroupID << nDisableGroupTime << pStatsBonus << fParameter << bStopCurrentAction;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EArmorDirection eValue )
{
	switch ( eValue )
	{
	case NDb::RPG_FRONT:
		return "RPG_FRONT";
	case NDb::RPG_LEFT:
		return "RPG_LEFT";
	case NDb::RPG_BACK:
		return "RPG_BACK";
	case NDb::RPG_RIGHT:
		return "RPG_RIGHT";
	case NDb::RPG_TOP:
		return "RPG_TOP";
	case NDb::RPG_BOTTOM:
		return "RPG_BOTTOM";
	default:
		return "RPG_FRONT";
	}
}

NDb::EArmorDirection NDb::StringToEnum_NDb_EArmorDirection( const std::string &szValue )
{
	if ( szValue == "RPG_FRONT" )
		return NDb::RPG_FRONT;
	if ( szValue == "RPG_LEFT" )
		return NDb::RPG_LEFT;
	if ( szValue == "RPG_BACK" )
		return NDb::RPG_BACK;
	if ( szValue == "RPG_RIGHT" )
		return NDb::RPG_RIGHT;
	if ( szValue == "RPG_TOP" )
		return NDb::RPG_TOP;
	if ( szValue == "RPG_BOTTOM" )
		return NDb::RPG_BOTTOM;
	return NDb::RPG_FRONT;
}

std::string EnumToString( NDb::EObjGameType eValue )
{
	switch ( eValue )
	{
	case NDb::SGVOGT_UNKNOWN:
		return "SGVOGT_UNKNOWN";
	case NDb::SGVOGT_UNIT:
		return "SGVOGT_UNIT";
	case NDb::SGVOGT_BUILDING:
		return "SGVOGT_BUILDING";
	case NDb::SGVOGT_FORTIFICATION:
		return "SGVOGT_FORTIFICATION";
	case NDb::SGVOGT_ENTRENCHMENT:
		return "SGVOGT_ENTRENCHMENT";
	case NDb::SGVOGT_TANK_PIT:
		return "SGVOGT_TANK_PIT";
	case NDb::SGVOGT_BRIDGE:
		return "SGVOGT_BRIDGE";
	case NDb::SGVOGT_MINE:
		return "SGVOGT_MINE";
	case NDb::SGVOGT_OBJECT:
		return "SGVOGT_OBJECT";
	case NDb::SGVOGT_FENCE:
		return "SGVOGT_FENCE";
	case NDb::SGVOGT_TERRAOBJ:
		return "SGVOGT_TERRAOBJ";
	case NDb::SGVOGT_EFFECT:
		return "SGVOGT_EFFECT";
	case NDb::SGVOGT_PROJECTILE:
		return "SGVOGT_PROJECTILE";
	case NDb::SGVOGT_SHADOW:
		return "SGVOGT_SHADOW";
	case NDb::SGVOGT_ICON:
		return "SGVOGT_ICON";
	case NDb::SGVOGT_SQUAD:
		return "SGVOGT_SQUAD";
	case NDb::SGVOGT_FLASH:
		return "SGVOGT_FLASH";
	case NDb::SGVOGT_FLAG:
		return "SGVOGT_FLAG";
	case NDb::SGVOGT_SOUND:
		return "SGVOGT_SOUND";
	case NDb::SGVOGT_FLORA:
		return "SGVOGT_FLORA";
	default:
		return "SGVOGT_UNKNOWN";
	}
}

NDb::EObjGameType NDb::StringToEnum_NDb_EObjGameType( const std::string &szValue )
{
	if ( szValue == "SGVOGT_UNKNOWN" )
		return NDb::SGVOGT_UNKNOWN;
	if ( szValue == "SGVOGT_UNIT" )
		return NDb::SGVOGT_UNIT;
	if ( szValue == "SGVOGT_BUILDING" )
		return NDb::SGVOGT_BUILDING;
	if ( szValue == "SGVOGT_FORTIFICATION" )
		return NDb::SGVOGT_FORTIFICATION;
	if ( szValue == "SGVOGT_ENTRENCHMENT" )
		return NDb::SGVOGT_ENTRENCHMENT;
	if ( szValue == "SGVOGT_TANK_PIT" )
		return NDb::SGVOGT_TANK_PIT;
	if ( szValue == "SGVOGT_BRIDGE" )
		return NDb::SGVOGT_BRIDGE;
	if ( szValue == "SGVOGT_MINE" )
		return NDb::SGVOGT_MINE;
	if ( szValue == "SGVOGT_OBJECT" )
		return NDb::SGVOGT_OBJECT;
	if ( szValue == "SGVOGT_FENCE" )
		return NDb::SGVOGT_FENCE;
	if ( szValue == "SGVOGT_TERRAOBJ" )
		return NDb::SGVOGT_TERRAOBJ;
	if ( szValue == "SGVOGT_EFFECT" )
		return NDb::SGVOGT_EFFECT;
	if ( szValue == "SGVOGT_PROJECTILE" )
		return NDb::SGVOGT_PROJECTILE;
	if ( szValue == "SGVOGT_SHADOW" )
		return NDb::SGVOGT_SHADOW;
	if ( szValue == "SGVOGT_ICON" )
		return NDb::SGVOGT_ICON;
	if ( szValue == "SGVOGT_SQUAD" )
		return NDb::SGVOGT_SQUAD;
	if ( szValue == "SGVOGT_FLASH" )
		return NDb::SGVOGT_FLASH;
	if ( szValue == "SGVOGT_FLAG" )
		return NDb::SGVOGT_FLAG;
	if ( szValue == "SGVOGT_SOUND" )
		return NDb::SGVOGT_SOUND;
	if ( szValue == "SGVOGT_FLORA" )
		return NDb::SGVOGT_FLORA;
	return NDb::SGVOGT_UNKNOWN;
}

std::string EnumToString( NDb::EObjectVisType eValue )
{
	switch ( eValue )
	{
	case NDb::SGVOT_MESH:
		return "SGVOT_MESH";
	case NDb::SGVOT_SPRITE:
		return "SGVOT_SPRITE";
	default:
		return "SGVOT_MESH";
	}
}

NDb::EObjectVisType NDb::StringToEnum_NDb_EObjectVisType( const std::string &szValue )
{
	if ( szValue == "SGVOT_MESH" )
		return NDb::SGVOT_MESH;
	if ( szValue == "SGVOT_SPRITE" )
		return NDb::SGVOT_SPRITE;
	return NDb::SGVOT_MESH;
}


void SCommonRPGStats::ReportMetaInfo() const
{
	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "KeyName", (BYTE*)&szKeyName - pThis, sizeof(szKeyName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "ParentName", (BYTE*)&szParentName - pThis, sizeof(szParentName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "StatsType", (BYTE*)&szStatsType - pThis, sizeof(szStatsType), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "VisType", (BYTE*)&eVisType - pThis, sizeof(eVisType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "GameType", (BYTE*)&eGameType - pThis, sizeof(eGameType), NTypeDef::TYPE_TYPE_ENUM );
}

int SCommonRPGStats::operator&( IXmlSaver &saver )
{
	saver.Add( "KeyName", &szKeyName );
	saver.Add( "ParentName", &szParentName );
	saver.Add( "StatsType", &szStatsType );
	saver.Add( "VisType", &eVisType );
	saver.Add( "GameType", &eGameType );

	return 0;
}

int SCommonRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szKeyName );
	saver.Add( 3, &szParentName );
	saver.Add( 4, &szStatsType );
	saver.Add( 5, &eVisType );
	saver.Add( 6, &eGameType );

	return 0;
}

DWORD SCommonRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szKeyName << szParentName << szStatsType << eVisType << eGameType;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SComplexEffect::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ComplexEffect", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "SceneEffect", (BYTE*)&pSceneEffect - pThis, sizeof(pSceneEffect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( "SceneEffects", &sceneEffects, pThis );
	NMetaInfo::ReportMetaInfo( "SoundEffect", (BYTE*)&pSoundEffect - pThis, sizeof(pSoundEffect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SComplexEffect::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "SceneEffect", &pSceneEffect );
	saver.Add( "SceneEffects", &sceneEffects );
	saver.Add( "SoundEffect", &pSoundEffect );

	return 0;
}

int SComplexEffect::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pSceneEffect );
	saver.Add( 3, &sceneEffects );
	saver.Add( 4, &pSoundEffect );

	return 0;
}



void SSeasonEffect::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "SeasonToUse", (BYTE*)&eSeasonToUse - pThis, sizeof(eSeasonToUse), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "SceneEffect", (BYTE*)&pSceneEffect - pThis, sizeof(pSceneEffect), NTypeDef::TYPE_TYPE_REF );
}

int SSeasonEffect::operator&( IXmlSaver &saver )
{
	saver.Add( "SeasonToUse", &eSeasonToUse );
	saver.Add( "SceneEffect", &pSceneEffect );

	return 0;
}

int SSeasonEffect::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eSeasonToUse );
	saver.Add( 3, &pSceneEffect );

	return 0;
}

DWORD SSeasonEffect::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eSeasonToUse;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SComplexSeasonedEffect::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ComplexSeasonedEffect", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "SoundEffect", (BYTE*)&pSoundEffect - pThis, sizeof(pSoundEffect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "Seasons", &seasons, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SComplexSeasonedEffect::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "SoundEffect", &pSoundEffect );
	saver.Add( "Seasons", &seasons );

	return 0;
}

int SComplexSeasonedEffect::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pSoundEffect );
	saver.Add( 3, &seasons );

	return 0;
}



void SAckSetRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "AckSetRPGStats", typeID, sizeof(*this) );
	SCommonRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "types", &types, pThis );
	NMetaInfo::ReportMetaInfo( "VoiceNumber", (BYTE*)&nVoiceNumber - pThis, sizeof(nVoiceNumber), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SAckSetRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SCommonRPGStats*)(this) );
	saver.Add( "types", &types );
	saver.Add( "VoiceNumber", &nVoiceNumber );

	return 0;
}

int SAckSetRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SCommonRPGStats*)this );
	saver.Add( 2, &types );
	saver.Add( 3, &nVoiceNumber );

	return 0;
}


std::string EnumToString( NDb::ESelectionType eValue )
{
	switch ( eValue )
	{
	case NDb::SELECTION_TYPE_GROUND:
		return "SELECTION_TYPE_GROUND";
	case NDb::SELECTION_TYPE_WATER:
		return "SELECTION_TYPE_WATER";
	case NDb::SELECTION_TYPE_AIR:
		return "SELECTION_TYPE_AIR";
	case NDb::SELECTION_TYPE_NONE:
		return "SELECTION_TYPE_NONE";
	case NDb::SELECTION_TYPE_CANNOT_SELECT:
		return "SELECTION_TYPE_CANNOT_SELECT";
	default:
		return "SELECTION_TYPE_GROUND";
	}
}

NDb::ESelectionType NDb::StringToEnum_NDb_ESelectionType( const std::string &szValue )
{
	if ( szValue == "SELECTION_TYPE_GROUND" )
		return NDb::SELECTION_TYPE_GROUND;
	if ( szValue == "SELECTION_TYPE_WATER" )
		return NDb::SELECTION_TYPE_WATER;
	if ( szValue == "SELECTION_TYPE_AIR" )
		return NDb::SELECTION_TYPE_AIR;
	if ( szValue == "SELECTION_TYPE_NONE" )
		return NDb::SELECTION_TYPE_NONE;
	if ( szValue == "SELECTION_TYPE_CANNOT_SELECT" )
		return NDb::SELECTION_TYPE_CANNOT_SELECT;
	return NDb::SELECTION_TYPE_GROUND;
}


void SAttachedLightEffect::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "LocatorName", (BYTE*)&szLocatorName - pThis, sizeof(szLocatorName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Rot", &qRot, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "FlarePos", &vFlarePos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "FlareSize", (BYTE*)&fFlareSize - pThis, sizeof(fFlareSize), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "PointLightPos", &vPointLightPos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "PointLightSize", (BYTE*)&fPointLightSize - pThis, sizeof(fPointLightSize), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "ConeLength", (BYTE*)&fConeLength - pThis, sizeof(fConeLength), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "ConeSize", (BYTE*)&fConeSize - pThis, sizeof(fConeSize), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Colour", &vColour, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "AdditionalEffect", (BYTE*)&pAdditionalEffect - pThis, sizeof(pAdditionalEffect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "OnAtDay", (BYTE*)&bOnAtDay - pThis, sizeof(bOnAtDay), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "OnAtNight", (BYTE*)&bOnAtNight - pThis, sizeof(bOnAtNight), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "ConeTexture", (BYTE*)&pConeTexture - pThis, sizeof(pConeTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "FlareTexture", (BYTE*)&pFlareTexture - pThis, sizeof(pFlareTexture), NTypeDef::TYPE_TYPE_REF );
}

int SAttachedLightEffect::operator&( IXmlSaver &saver )
{
	saver.Add( "LocatorName", &szLocatorName );
	saver.Add( "Pos", &vPos );
	saver.Add( "Rot", &qRot );
	saver.Add( "FlarePos", &vFlarePos );
	saver.Add( "FlareSize", &fFlareSize );
	saver.Add( "PointLightPos", &vPointLightPos );
	saver.Add( "PointLightSize", &fPointLightSize );
	saver.Add( "ConeLength", &fConeLength );
	saver.Add( "ConeSize", &fConeSize );
	saver.Add( "Colour", &vColour );
	saver.Add( "AdditionalEffect", &pAdditionalEffect );
	saver.Add( "OnAtDay", &bOnAtDay );
	saver.Add( "OnAtNight", &bOnAtNight );
	saver.Add( "ConeTexture", &pConeTexture );
	saver.Add( "FlareTexture", &pFlareTexture );

	return 0;
}

int SAttachedLightEffect::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szLocatorName );
	saver.Add( 3, &vPos );
	saver.Add( 4, &qRot );
	saver.Add( 5, &vFlarePos );
	saver.Add( 6, &fFlareSize );
	saver.Add( 7, &vPointLightPos );
	saver.Add( 8, &fPointLightSize );
	saver.Add( 9, &fConeLength );
	saver.Add( 10, &fConeSize );
	saver.Add( 11, &vColour );
	saver.Add( 12, &pAdditionalEffect );
	saver.Add( 13, &bOnAtDay );
	saver.Add( 14, &bOnAtNight );
	saver.Add( 15, &pConeTexture );
	saver.Add( 16, &pFlareTexture );

	return 0;
}

DWORD SAttachedLightEffect::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szLocatorName << vPos << qRot << vFlarePos << fFlareSize << vPointLightPos << fPointLightSize << fConeLength << fConeSize << vColour << bOnAtDay << bOnAtNight;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SIconsSetParams::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Custom", (BYTE*)&bCustom - pThis, sizeof(bCustom), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Raising", (BYTE*)&fRaising - pThis, sizeof(fRaising), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "HPBarLen", (BYTE*)&fHPBarLen - pThis, sizeof(fHPBarLen), NTypeDef::TYPE_TYPE_FLOAT );
}

int SIconsSetParams::operator&( IXmlSaver &saver )
{
	saver.Add( "Custom", &bCustom );
	saver.Add( "Raising", &fRaising );
	saver.Add( "HPBarLen", &fHPBarLen );

	return 0;
}

int SIconsSetParams::operator&( IBinSaver &saver )
{
	saver.Add( 2, &bCustom );
	saver.Add( 3, &fRaising );
	saver.Add( 4, &fHPBarLen );

	return 0;
}

DWORD SIconsSetParams::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << bCustom << fRaising << fHPBarLen;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SHPObjectRPGStats::SByteArray2::SByteArray1::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "data", &data, pThis );
}

int SHPObjectRPGStats::SByteArray2::SByteArray1::operator&( IXmlSaver &saver )
{
	saver.Add( "data", &data );

	return 0;
}

int SHPObjectRPGStats::SByteArray2::SByteArray1::operator&( IBinSaver &saver )
{
	saver.Add( 2, &data );

	return 0;
}

DWORD SHPObjectRPGStats::SByteArray2::SByteArray1::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << data;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SHPObjectRPGStats::SByteArray2::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "data", &data, pThis );
}

int SHPObjectRPGStats::SByteArray2::operator&( IXmlSaver &saver )
{
	saver.Add( "data", &data );

	return 0;
}

int SHPObjectRPGStats::SByteArray2::operator&( IBinSaver &saver )
{
	saver.Add( 2, &data );

	return 0;
}

DWORD SHPObjectRPGStats::SByteArray2::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << data;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SHPObjectRPGStats::SDefenseRPGStats::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "ArmorMin", (BYTE*)&nArmorMin - pThis, sizeof(nArmorMin), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "ArmorMax", (BYTE*)&nArmorMax - pThis, sizeof(nArmorMax), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Silhouette", (BYTE*)&fSilhouette - pThis, sizeof(fSilhouette), NTypeDef::TYPE_TYPE_FLOAT );
}

int SHPObjectRPGStats::SDefenseRPGStats::operator&( IXmlSaver &saver )
{
	saver.Add( "ArmorMin", &nArmorMin );
	saver.Add( "ArmorMax", &nArmorMax );
	saver.Add( "Silhouette", &fSilhouette );

	return 0;
}

int SHPObjectRPGStats::SDefenseRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nArmorMin );
	saver.Add( 3, &nArmorMax );
	saver.Add( 4, &fSilhouette );

	return 0;
}

DWORD SHPObjectRPGStats::SDefenseRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nArmorMin << nArmorMax << fSilhouette;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SHPObjectRPGStats::SDamageLevel::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "DamageHP", (BYTE*)&fDamageHP - pThis, sizeof(fDamageHP), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "VisObj", (BYTE*)&pVisObj - pThis, sizeof(pVisObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "DamageEffectWindow", (BYTE*)&pDamageEffectWindow - pThis, sizeof(pDamageEffectWindow), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "DamageEffectSmoke", (BYTE*)&pDamageEffectSmoke - pThis, sizeof(pDamageEffectSmoke), NTypeDef::TYPE_TYPE_REF );
}

int SHPObjectRPGStats::SDamageLevel::operator&( IXmlSaver &saver )
{
	saver.Add( "DamageHP", &fDamageHP );
	saver.Add( "VisObj", &pVisObj );
	saver.Add( "DamageEffectWindow", &pDamageEffectWindow );
	saver.Add( "DamageEffectSmoke", &pDamageEffectSmoke );

	return 0;
}

int SHPObjectRPGStats::SDamageLevel::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fDamageHP );
	saver.Add( 3, &pVisObj );
	saver.Add( 4, &pDamageEffectWindow );
	saver.Add( 5, &pDamageEffectSmoke );

	return 0;
}

DWORD SHPObjectRPGStats::SDamageLevel::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fDamageHP;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SHPObjectRPGStats::SModelSurfacePoint::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Orient", &vOrient, pThis );
}

int SHPObjectRPGStats::SModelSurfacePoint::operator&( IXmlSaver &saver )
{
	saver.Add( "Pos", &vPos );
	saver.Add( "Orient", &vOrient );

	return 0;
}

int SHPObjectRPGStats::SModelSurfacePoint::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPos );
	saver.Add( 3, &vOrient );

	return 0;
}

DWORD SHPObjectRPGStats::SModelSurfacePoint::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos << vOrient;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SHPObjectRPGStats::ReportMetaInfo() const
{
	SCommonRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "MaxHP", (BYTE*)&fMaxHP - pThis, sizeof(fMaxHP), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructArrayMetaInfo( "DamageLevels", &damageLevels, pThis );
	NMetaInfo::ReportMetaInfo( "RepairCost", (BYTE*)&fRepairCost - pThis, sizeof(fRepairCost), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructArrayMetaInfo( "defences", &defences, pThis );
	NMetaInfo::ReportMetaInfo( "visualObject", (BYTE*)&pvisualObject - pThis, sizeof(pvisualObject), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "infoVisualObject", (BYTE*)&pinfoVisualObject - pThis, sizeof(pinfoVisualObject), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "IconTexture", (BYTE*)&pIconTexture - pThis, sizeof(pIconTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "IconFlagBackground", (BYTE*)&pIconFlagBackground - pThis, sizeof(pIconFlagBackground), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "LocalizedNameFileRef", (BYTE*)&szLocalizedNameFileRef - pThis, sizeof(szLocalizedNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "SelectionScale", (BYTE*)&fSelectionScale - pThis, sizeof(fSelectionScale), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "SelectionType", (BYTE*)&eSelectionType - pThis, sizeof(eSelectionType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructArrayMetaInfo( "LightEffects", &lightEffects, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "SurfacePoints", &surfacePoints, pThis );
	NMetaInfo::ReportMetaInfo( "IconsSet", (BYTE*)&pIconsSet - pThis, sizeof(pIconsSet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "IconsSetParams", &iconsSetParams, pThis );
}

int SHPObjectRPGStats::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SCommonRPGStats*)(this) );
	saver.Add( "MaxHP", &fMaxHP );
	saver.Add( "DamageLevels", &damageLevels );
	saver.Add( "RepairCost", &fRepairCost );
	saver.Add( "defences", &defences );
	saver.Add( "visualObject", &pvisualObject );
	saver.Add( "infoVisualObject", &pinfoVisualObject );
	saver.Add( "IconTexture", &pIconTexture );
	saver.Add( "IconFlagBackground", &pIconFlagBackground );
	saver.Add( "LocalizedNameFileRef", &szLocalizedNameFileRef );
	saver.Add( "SelectionScale", &fSelectionScale );
	saver.Add( "SelectionType", &eSelectionType );
	saver.Add( "LightEffects", &lightEffects );
	saver.Add( "SurfacePoints", &surfacePoints );
	saver.Add( "IconsSet", &pIconsSet );
	saver.Add( "IconsSetParams", &iconsSetParams );

	return 0;
}

int SHPObjectRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SCommonRPGStats*)this );
	saver.Add( 2, &fMaxHP );
	saver.Add( 3, &damageLevels );
	saver.Add( 4, &fRepairCost );
	saver.Add( 5, &defences );
	saver.Add( 6, &pvisualObject );
	saver.Add( 7, &pinfoVisualObject );
	saver.Add( 8, &pIconTexture );
	saver.Add( 9, &pIconFlagBackground );
	saver.Add( 10, &szLocalizedNameFileRef );
	saver.Add( 11, &fSelectionScale );
	saver.Add( 12, &eSelectionType );
	saver.Add( 13, &lightEffects );
	saver.Add( 14, &surfacePoints );
	saver.Add( 15, &pIconsSet );
	saver.Add( 16, &iconsSetParams );

	return 0;
}

DWORD SHPObjectRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SCommonRPGStats::CalcCheckSum() << fMaxHP << fRepairCost << defences;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBurningFuel::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "BurningFuel", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "BurnTime", (BYTE*)&nBurnTime - pThis, sizeof(nBurnTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "WeaponFireEachSecond", (BYTE*)&pWeaponFireEachSecond - pThis, sizeof(pWeaponFireEachSecond), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "WeaponFireOnDesctruction", (BYTE*)&pWeaponFireOnDesctruction - pThis, sizeof(pWeaponFireOnDesctruction), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SBurningFuel::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "BurnTime", &nBurnTime );
	saver.Add( "WeaponFireEachSecond", &pWeaponFireEachSecond );
	saver.Add( "WeaponFireOnDesctruction", &pWeaponFireOnDesctruction );

	return 0;
}

int SBurningFuel::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nBurnTime );
	saver.Add( 3, &pWeaponFireEachSecond );
	saver.Add( 4, &pWeaponFireOnDesctruction );

	return 0;
}

DWORD SBurningFuel::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nBurnTime << pWeaponFireEachSecond << pWeaponFireOnDesctruction;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SStaticObjectRPGStats::ReportMetaInfo() const
{
	SHPObjectRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "AIPassabilityClass", (BYTE*)&nAIPassabilityClass - pThis, sizeof(nAIPassabilityClass), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Burn", (BYTE*)&bBurn - pThis, sizeof(bBurn), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "EffectExplosion", (BYTE*)&pEffectExplosion - pThis, sizeof(pEffectExplosion), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EffectDeath", (BYTE*)&pEffectDeath - pThis, sizeof(pEffectDeath), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EffectDisappear", (BYTE*)&pEffectDisappear - pThis, sizeof(pEffectDisappear), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "LeaveCorpse", (BYTE*)&bLeaveCorpse - pThis, sizeof(bLeaveCorpse), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "DestructableCorpse", (BYTE*)&bDestructableCorpse - pThis, sizeof(bDestructableCorpse), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "ShootOnDestruction", (BYTE*)&pShootOnDestruction - pThis, sizeof(pShootOnDestruction), NTypeDef::TYPE_TYPE_REF );
}

int SStaticObjectRPGStats::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SHPObjectRPGStats*)(this) );
	saver.Add( "AIPassabilityClass", &nAIPassabilityClass );
	saver.Add( "Burn", &bBurn );
	saver.Add( "EffectExplosion", &pEffectExplosion );
	saver.Add( "EffectDeath", &pEffectDeath );
	saver.Add( "EffectDisappear", &pEffectDisappear );
	saver.Add( "LeaveCorpse", &bLeaveCorpse );
	saver.Add( "DestructableCorpse", &bDestructableCorpse );
	saver.Add( "ShootOnDestruction", &pShootOnDestruction );

	return 0;
}

int SStaticObjectRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SHPObjectRPGStats*)this );
	saver.Add( 2, &nAIPassabilityClass );
	saver.Add( 3, &bBurn );
	saver.Add( 4, &pEffectExplosion );
	saver.Add( 5, &pEffectDeath );
	saver.Add( 6, &pEffectDisappear );
	saver.Add( 7, &bLeaveCorpse );
	saver.Add( 8, &bDestructableCorpse );
	saver.Add( 9, &pShootOnDestruction );

	return 0;
}

DWORD SStaticObjectRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SHPObjectRPGStats::CalcCheckSum() << nAIPassabilityClass << bBurn << bLeaveCorpse << bDestructableCorpse << pShootOnDestruction;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SCraterSet::SSingleSeasonCraters::SCraterDesc::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Material", (BYTE*)&pMaterial - pThis, sizeof(pMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Scale", (BYTE*)&fScale - pThis, sizeof(fScale), NTypeDef::TYPE_TYPE_FLOAT );
}

int SCraterSet::SSingleSeasonCraters::SCraterDesc::operator&( IXmlSaver &saver )
{
	saver.Add( "Material", &pMaterial );
	saver.Add( "Scale", &fScale );

	return 0;
}

int SCraterSet::SSingleSeasonCraters::SCraterDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMaterial );
	saver.Add( 3, &fScale );

	return 0;
}

DWORD SCraterSet::SSingleSeasonCraters::SCraterDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fScale;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SCraterSet::SSingleSeasonCraters::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Season", (BYTE*)&eSeason - pThis, sizeof(eSeason), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Craters", &craters, pThis );
}

int SCraterSet::SSingleSeasonCraters::operator&( IXmlSaver &saver )
{
	saver.Add( "Season", &eSeason );
	saver.Add( "Craters", &craters );

	return 0;
}

int SCraterSet::SSingleSeasonCraters::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eSeason );
	saver.Add( 3, &craters );

	return 0;
}

DWORD SCraterSet::SSingleSeasonCraters::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eSeason << craters;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SCraterSet::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "CraterSet", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Craters", &craters, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SCraterSet::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Craters", &craters );

	return 0;
}

int SCraterSet::operator&( IBinSaver &saver )
{
	saver.Add( 2, &craters );

	return 0;
}



void SProjectile::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Projectile", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Model", (BYTE*)&pModel - pThis, sizeof(pModel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SmokyExhaustEffect", (BYTE*)&pSmokyExhaustEffect - pThis, sizeof(pSmokyExhaustEffect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SmokyEffectLocator", (BYTE*)&szSmokyEffectLocator - pThis, sizeof(szSmokyEffectLocator), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "SmokyExhaustEffectInterval", (BYTE*)&fSmokyExhaustEffectInterval - pThis, sizeof(fSmokyExhaustEffectInterval), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AttachedEffect", (BYTE*)&pAttachedEffect - pThis, sizeof(pAttachedEffect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EffectBeforeHit", (BYTE*)&pEffectBeforeHit - pThis, sizeof(pEffectBeforeHit), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "AttachedEffectLocator", (BYTE*)&szAttachedEffectLocator - pThis, sizeof(szAttachedEffectLocator), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SProjectile::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Model", &pModel );
	saver.Add( "SmokyExhaustEffect", &pSmokyExhaustEffect );
	saver.Add( "SmokyEffectLocator", &szSmokyEffectLocator );
	saver.Add( "SmokyExhaustEffectInterval", &fSmokyExhaustEffectInterval );
	saver.Add( "AttachedEffect", &pAttachedEffect );
	saver.Add( "EffectBeforeHit", &pEffectBeforeHit );
	saver.Add( "AttachedEffectLocator", &szAttachedEffectLocator );

	return 0;
}

int SProjectile::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pModel );
	saver.Add( 3, &pSmokyExhaustEffect );
	saver.Add( 4, &szSmokyEffectLocator );
	saver.Add( 5, &fSmokyExhaustEffectInterval );
	saver.Add( 6, &pAttachedEffect );
	saver.Add( 7, &pEffectBeforeHit );
	saver.Add( 8, &szAttachedEffectLocator );

	return 0;
}


std::string EnumToString( NDb::SWeaponRPGStats::SShell::ETrajectoryType eValue )
{
	switch ( eValue )
	{
	case NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE:
		return "TRAJECTORY_LINE";
	case NDb::SWeaponRPGStats::SShell::TRAJECTORY_HOWITZER:
		return "TRAJECTORY_HOWITZER";
	case NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB:
		return "TRAJECTORY_BOMB";
	case NDb::SWeaponRPGStats::SShell::TRAJECTORY_CANNON:
		return "TRAJECTORY_CANNON";
	case NDb::SWeaponRPGStats::SShell::TRAJECTORY_ROCKET:
		return "TRAJECTORY_ROCKET";
	case NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE:
		return "TRAJECTORY_GRENADE";
	case NDb::SWeaponRPGStats::SShell::TRAJECTORY_TORPEDO:
		return "TRAJECTORY_TORPEDO";
	case NDb::SWeaponRPGStats::SShell::TRAJECTORY_AA_ROCKET:
		return "TRAJECTORY_AA_ROCKET";
	case NDb::SWeaponRPGStats::SShell::TRAJECTORY_FLAME_THROWER:
		return "TRAJECTORY_FLAME_THROWER";
	default:
		return "TRAJECTORY_LINE";
	}
}

NDb::SWeaponRPGStats::SShell::ETrajectoryType NDb::StringToEnum_NDb_SWeaponRPGStats_SShell_ETrajectoryType( const std::string &szValue )
{
	if ( szValue == "TRAJECTORY_LINE" )
		return NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE;
	if ( szValue == "TRAJECTORY_HOWITZER" )
		return NDb::SWeaponRPGStats::SShell::TRAJECTORY_HOWITZER;
	if ( szValue == "TRAJECTORY_BOMB" )
		return NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB;
	if ( szValue == "TRAJECTORY_CANNON" )
		return NDb::SWeaponRPGStats::SShell::TRAJECTORY_CANNON;
	if ( szValue == "TRAJECTORY_ROCKET" )
		return NDb::SWeaponRPGStats::SShell::TRAJECTORY_ROCKET;
	if ( szValue == "TRAJECTORY_GRENADE" )
		return NDb::SWeaponRPGStats::SShell::TRAJECTORY_GRENADE;
	if ( szValue == "TRAJECTORY_TORPEDO" )
		return NDb::SWeaponRPGStats::SShell::TRAJECTORY_TORPEDO;
	if ( szValue == "TRAJECTORY_AA_ROCKET" )
		return NDb::SWeaponRPGStats::SShell::TRAJECTORY_AA_ROCKET;
	if ( szValue == "TRAJECTORY_FLAME_THROWER" )
		return NDb::SWeaponRPGStats::SShell::TRAJECTORY_FLAME_THROWER;
	return NDb::SWeaponRPGStats::SShell::TRAJECTORY_LINE;
}

std::string EnumToString( NDb::SWeaponRPGStats::SShell::EShellDamageType eValue )
{
	switch ( eValue )
	{
	case NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH:
		return "DAMAGE_HEALTH";
	case NDb::SWeaponRPGStats::SShell::DAMAGE_MORALE:
		return "DAMAGE_MORALE";
	case NDb::SWeaponRPGStats::SShell::DAMAGE_FOG:
		return "DAMAGE_FOG";
	default:
		return "DAMAGE_HEALTH";
	}
}

NDb::SWeaponRPGStats::SShell::EShellDamageType NDb::StringToEnum_NDb_SWeaponRPGStats_SShell_EShellDamageType( const std::string &szValue )
{
	if ( szValue == "DAMAGE_HEALTH" )
		return NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH;
	if ( szValue == "DAMAGE_MORALE" )
		return NDb::SWeaponRPGStats::SShell::DAMAGE_MORALE;
	if ( szValue == "DAMAGE_FOG" )
		return NDb::SWeaponRPGStats::SShell::DAMAGE_FOG;
	return NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH;
}


void SWeaponRPGStats::SShell::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "DamageType", (BYTE*)&eDamageType - pThis, sizeof(eDamageType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Piercing", (BYTE*)&nPiercing - pThis, sizeof(nPiercing), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "DamageRandom", (BYTE*)&nDamageRandom - pThis, sizeof(nDamageRandom), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "DamagePower", (BYTE*)&fDamagePower - pThis, sizeof(fDamagePower), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "PiercingRandom", (BYTE*)&nPiercingRandom - pThis, sizeof(nPiercingRandom), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Area", (BYTE*)&fArea - pThis, sizeof(fArea), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Area2", (BYTE*)&fArea2 - pThis, sizeof(fArea2), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Speed", (BYTE*)&fSpeed - pThis, sizeof(fSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "TraceSpeedCoeff", (BYTE*)&fTraceSpeedCoeff - pThis, sizeof(fTraceSpeedCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "TraceProbability", (BYTE*)&fTraceProbability - pThis, sizeof(fTraceProbability), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "TraceLength", (BYTE*)&fTraceLength - pThis, sizeof(fTraceLength), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "TraceWidth", (BYTE*)&fTraceWidth - pThis, sizeof(fTraceWidth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "TraceMaterial", (BYTE*)&pTraceMaterial - pThis, sizeof(pTraceMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "DetonationPower", (BYTE*)&fDetonationPower - pThis, sizeof(fDetonationPower), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "trajectory", (BYTE*)&etrajectory - pThis, sizeof(etrajectory), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "BrokeTrackProbability", (BYTE*)&fBrokeTrackProbability - pThis, sizeof(fBrokeTrackProbability), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "FireSound", (BYTE*)&szFireSound - pThis, sizeof(szFireSound), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "EffectGunFire", (BYTE*)&pEffectGunFire - pThis, sizeof(pEffectGunFire), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "EffectTrajectory", (BYTE*)&pEffectTrajectory - pThis, sizeof(pEffectTrajectory), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "EffectHitDirect", (BYTE*)&pEffectHitDirect - pThis, sizeof(pEffectHitDirect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "EffectHitMiss", (BYTE*)&pEffectHitMiss - pThis, sizeof(pEffectHitMiss), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "EffectHitReflect", (BYTE*)&pEffectHitReflect - pThis, sizeof(pEffectHitReflect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "EffectHitGround", (BYTE*)&pEffectHitGround - pThis, sizeof(pEffectHitGround), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "EffectHitWater", (BYTE*)&pEffectHitWater - pThis, sizeof(pEffectHitWater), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "EffectHitAir", (BYTE*)&pEffectHitAir - pThis, sizeof(pEffectHitAir), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Craters", (BYTE*)&pCraters - pThis, sizeof(pCraters), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "FireRate", (BYTE*)&fFireRate - pThis, sizeof(fFireRate), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "RelaxTime", (BYTE*)&fRelaxTime - pThis, sizeof(fRelaxTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "visProjectile", (BYTE*)&pvisProjectile - pThis, sizeof(pvisProjectile), NTypeDef::TYPE_TYPE_REF );
}

int SWeaponRPGStats::SShell::operator&( IXmlSaver &saver )
{
	saver.Add( "DamageType", &eDamageType );
	saver.Add( "Piercing", &nPiercing );
	saver.Add( "DamageRandom", &nDamageRandom );
	saver.Add( "DamagePower", &fDamagePower );
	saver.Add( "PiercingRandom", &nPiercingRandom );
	saver.Add( "Area", &fArea );
	saver.Add( "Area2", &fArea2 );
	saver.Add( "Speed", &fSpeed );
	saver.Add( "TraceSpeedCoeff", &fTraceSpeedCoeff );
	saver.Add( "TraceProbability", &fTraceProbability );
	saver.Add( "TraceLength", &fTraceLength );
	saver.Add( "TraceWidth", &fTraceWidth );
	saver.Add( "TraceMaterial", &pTraceMaterial );
	saver.Add( "DetonationPower", &fDetonationPower );
	saver.Add( "trajectory", &etrajectory );
	saver.Add( "BrokeTrackProbability", &fBrokeTrackProbability );
	saver.Add( "FireSound", &szFireSound );
	saver.Add( "EffectGunFire", &pEffectGunFire );
	saver.Add( "EffectTrajectory", &pEffectTrajectory );
	saver.Add( "EffectHitDirect", &pEffectHitDirect );
	saver.Add( "EffectHitMiss", &pEffectHitMiss );
	saver.Add( "EffectHitReflect", &pEffectHitReflect );
	saver.Add( "EffectHitGround", &pEffectHitGround );
	saver.Add( "EffectHitWater", &pEffectHitWater );
	saver.Add( "EffectHitAir", &pEffectHitAir );
	saver.Add( "Craters", &pCraters );
	saver.Add( "FireRate", &fFireRate );
	saver.Add( "RelaxTime", &fRelaxTime );
	saver.Add( "visProjectile", &pvisProjectile );

	return 0;
}

int SWeaponRPGStats::SShell::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eDamageType );
	saver.Add( 3, &nPiercing );
	saver.Add( 4, &nDamageRandom );
	saver.Add( 5, &fDamagePower );
	saver.Add( 6, &nPiercingRandom );
	saver.Add( 7, &fArea );
	saver.Add( 8, &fArea2 );
	saver.Add( 9, &fSpeed );
	saver.Add( 10, &fTraceSpeedCoeff );
	saver.Add( 11, &fTraceProbability );
	saver.Add( 12, &fTraceLength );
	saver.Add( 13, &fTraceWidth );
	saver.Add( 14, &pTraceMaterial );
	saver.Add( 15, &fDetonationPower );
	saver.Add( 16, &etrajectory );
	saver.Add( 17, &fBrokeTrackProbability );
	saver.Add( 18, &szFireSound );
	saver.Add( 19, &pEffectGunFire );
	saver.Add( 20, &pEffectTrajectory );
	saver.Add( 21, &pEffectHitDirect );
	saver.Add( 22, &pEffectHitMiss );
	saver.Add( 23, &pEffectHitReflect );
	saver.Add( 24, &pEffectHitGround );
	saver.Add( 25, &pEffectHitWater );
	saver.Add( 26, &pEffectHitAir );
	saver.Add( 27, &pCraters );
	saver.Add( 28, &fFireRate );
	saver.Add( 29, &fRelaxTime );
	saver.Add( 30, &pvisProjectile );

	return 0;
}

DWORD SWeaponRPGStats::SShell::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eDamageType << nPiercing << nDamageRandom << fDamagePower << nPiercingRandom << fArea << fArea2 << fSpeed << fTraceSpeedCoeff << fTraceProbability << fTraceLength << fTraceWidth << etrajectory << fBrokeTrackProbability << szFireSound << fFireRate << fRelaxTime;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::SWeaponRPGStats::EWeaponType eValue )
{
	switch ( eValue )
	{
	case NDb::SWeaponRPGStats::WEAPON_PISTOL:
		return "WEAPON_PISTOL";
	case NDb::SWeaponRPGStats::WEAPON_MACHINEGUN:
		return "WEAPON_MACHINEGUN";
	case NDb::SWeaponRPGStats::WEAPON_SUBMACHINEGUN:
		return "WEAPON_SUBMACHINEGUN";
	case NDb::SWeaponRPGStats::WEAPON_RIFLE:
		return "WEAPON_RIFLE";
	case NDb::SWeaponRPGStats::WEAPON_SNIPER_RIFLE:
		return "WEAPON_SNIPER_RIFLE";
	case NDb::SWeaponRPGStats::WEAPON_ANTITANK_RIFLE:
		return "WEAPON_ANTITANK_RIFLE";
	case NDb::SWeaponRPGStats::WEAPON_BAZOOKA:
		return "WEAPON_BAZOOKA";
	case NDb::SWeaponRPGStats::WEAPON_PIAT:
		return "WEAPON_PIAT";
	case NDb::SWeaponRPGStats::WEAPON_RIFLE_AMERICAN:
		return "WEAPON_RIFLE_AMERICAN";
	case NDb::SWeaponRPGStats::WEAPON_FLAME_THROWER:
		return "WEAPON_FLAME_THROWER";
	case NDb::SWeaponRPGStats::WEAPON_STEN:
		return "WEAPON_STEN";
	case NDb::SWeaponRPGStats::WEAPON_PANZERFAUST:
		return "WEAPON_PANZERFAUST";
	case NDb::SWeaponRPGStats::WEAPON_LUFTFAUST:
		return "WEAPON_LUFTFAUST";
	case NDb::SWeaponRPGStats::WEAPON_HEAVY_CANNON:
		return "WEAPON_HEAVY_CANNON";
	case NDb::SWeaponRPGStats::WEAPON_HIDED:
		return "WEAPON_HIDED";
	case NDb::SWeaponRPGStats::_WEAPON_COUNTER:
		return "_WEAPON_COUNTER";
	default:
		return "WEAPON_PISTOL";
	}
}

NDb::SWeaponRPGStats::EWeaponType NDb::StringToEnum_NDb_SWeaponRPGStats_EWeaponType( const std::string &szValue )
{
	if ( szValue == "WEAPON_PISTOL" )
		return NDb::SWeaponRPGStats::WEAPON_PISTOL;
	if ( szValue == "WEAPON_MACHINEGUN" )
		return NDb::SWeaponRPGStats::WEAPON_MACHINEGUN;
	if ( szValue == "WEAPON_SUBMACHINEGUN" )
		return NDb::SWeaponRPGStats::WEAPON_SUBMACHINEGUN;
	if ( szValue == "WEAPON_RIFLE" )
		return NDb::SWeaponRPGStats::WEAPON_RIFLE;
	if ( szValue == "WEAPON_SNIPER_RIFLE" )
		return NDb::SWeaponRPGStats::WEAPON_SNIPER_RIFLE;
	if ( szValue == "WEAPON_ANTITANK_RIFLE" )
		return NDb::SWeaponRPGStats::WEAPON_ANTITANK_RIFLE;
	if ( szValue == "WEAPON_BAZOOKA" )
		return NDb::SWeaponRPGStats::WEAPON_BAZOOKA;
	if ( szValue == "WEAPON_PIAT" )
		return NDb::SWeaponRPGStats::WEAPON_PIAT;
	if ( szValue == "WEAPON_RIFLE_AMERICAN" )
		return NDb::SWeaponRPGStats::WEAPON_RIFLE_AMERICAN;
	if ( szValue == "WEAPON_FLAME_THROWER" )
		return NDb::SWeaponRPGStats::WEAPON_FLAME_THROWER;
	if ( szValue == "WEAPON_STEN" )
		return NDb::SWeaponRPGStats::WEAPON_STEN;
	if ( szValue == "WEAPON_PANZERFAUST" )
		return NDb::SWeaponRPGStats::WEAPON_PANZERFAUST;
	if ( szValue == "WEAPON_LUFTFAUST" )
		return NDb::SWeaponRPGStats::WEAPON_LUFTFAUST;
	if ( szValue == "WEAPON_HEAVY_CANNON" )
		return NDb::SWeaponRPGStats::WEAPON_HEAVY_CANNON;
	if ( szValue == "WEAPON_HIDED" )
		return NDb::SWeaponRPGStats::WEAPON_HIDED;
	if ( szValue == "_WEAPON_COUNTER" )
		return NDb::SWeaponRPGStats::_WEAPON_COUNTER;
	return NDb::SWeaponRPGStats::WEAPON_PISTOL;
}


void SWeaponRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WeaponRPGStats", typeID, sizeof(*this) );
	SCommonRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "WeaponType", (BYTE*)&eWeaponType - pThis, sizeof(eWeaponType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "WeaponTypeTexture", (BYTE*)&pWeaponTypeTexture - pThis, sizeof(pWeaponTypeTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Dispersion", (BYTE*)&fDispersion - pThis, sizeof(fDispersion), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AimingTime", (BYTE*)&fAimingTime - pThis, sizeof(fAimingTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "AmmoPerBurst", (BYTE*)&nAmmoPerBurst - pThis, sizeof(nAmmoPerBurst), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "RangeMax", (BYTE*)&fRangeMax - pThis, sizeof(fRangeMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RangeMin", (BYTE*)&fRangeMin - pThis, sizeof(fRangeMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Ceiling", (BYTE*)&nCeiling - pThis, sizeof(nCeiling), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "RevealRadius", (BYTE*)&fRevealRadius - pThis, sizeof(fRevealRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DeltaAngle", (BYTE*)&fDeltaAngle - pThis, sizeof(fDeltaAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructArrayMetaInfo( "shells", &shells, pThis );
	NMetaInfo::ReportMetaInfo( "VisObj", (BYTE*)&pVisObj - pThis, sizeof(pVisObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "LocalizedNameFileRef", (BYTE*)&szLocalizedNameFileRef - pThis, sizeof(szLocalizedNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SWeaponRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SCommonRPGStats*)(this) );
	saver.Add( "WeaponType", &eWeaponType );
	saver.Add( "WeaponTypeTexture", &pWeaponTypeTexture );
	saver.Add( "Dispersion", &fDispersion );
	saver.Add( "AimingTime", &fAimingTime );
	saver.Add( "AmmoPerBurst", &nAmmoPerBurst );
	saver.Add( "RangeMax", &fRangeMax );
	saver.Add( "RangeMin", &fRangeMin );
	saver.Add( "Ceiling", &nCeiling );
	saver.Add( "RevealRadius", &fRevealRadius );
	saver.Add( "DeltaAngle", &fDeltaAngle );
	saver.Add( "shells", &shells );
	saver.Add( "VisObj", &pVisObj );
	saver.Add( "LocalizedNameFileRef", &szLocalizedNameFileRef );

	return 0;
}

int SWeaponRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SCommonRPGStats*)this );
	saver.Add( 2, &eWeaponType );
	saver.Add( 3, &pWeaponTypeTexture );
	saver.Add( 4, &fDispersion );
	saver.Add( 5, &fAimingTime );
	saver.Add( 6, &nAmmoPerBurst );
	saver.Add( 7, &fRangeMax );
	saver.Add( 8, &fRangeMin );
	saver.Add( 9, &nCeiling );
	saver.Add( 10, &fRevealRadius );
	saver.Add( 11, &fDeltaAngle );
	saver.Add( 12, &shells );
	saver.Add( 13, &pVisObj );
	saver.Add( 14, &szLocalizedNameFileRef );

	return 0;
}

DWORD SWeaponRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SCommonRPGStats::CalcCheckSum() << eWeaponType << fDispersion << fAimingTime << nAmmoPerBurst << fRangeMax << fRangeMin << nCeiling << fRevealRadius << fDeltaAngle << shells;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBaseGunRPGStats::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Weapon", (BYTE*)&pWeapon - pThis, sizeof(pWeapon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "AttachedGunVisObj", (BYTE*)&pAttachedGunVisObj - pThis, sizeof(pAttachedGunVisObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "AttachedGunLocator", (BYTE*)&szAttachedGunLocator - pThis, sizeof(szAttachedGunLocator), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "ShootPointOffset", &vShootPointOffset, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "ShootEffectInvert", (BYTE*)&bShootEffectInvert - pThis, sizeof(bShootEffectInvert), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Priority", (BYTE*)&nPriority - pThis, sizeof(nPriority), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "IsPrimary", (BYTE*)&bIsPrimary - pThis, sizeof(bIsPrimary), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Ammo", (BYTE*)&nAmmo - pThis, sizeof(nAmmo), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&fDirection - pThis, sizeof(fDirection), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "ReloadCost", (BYTE*)&fReloadCost - pThis, sizeof(fReloadCost), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "TargetAAOnly", (BYTE*)&bTargetAAOnly - pThis, sizeof(bTargetAAOnly), NTypeDef::TYPE_TYPE_BOOL );
}

int SBaseGunRPGStats::operator&( IXmlSaver &saver )
{
	saver.Add( "Weapon", &pWeapon );
	saver.Add( "AttachedGunVisObj", &pAttachedGunVisObj );
	saver.Add( "AttachedGunLocator", &szAttachedGunLocator );
	saver.Add( "ShootPointOffset", &vShootPointOffset );
	saver.Add( "ShootEffectInvert", &bShootEffectInvert );
	saver.Add( "Priority", &nPriority );
	saver.Add( "IsPrimary", &bIsPrimary );
	saver.Add( "Ammo", &nAmmo );
	saver.Add( "Direction", &fDirection );
	saver.Add( "ReloadCost", &fReloadCost );
	saver.Add( "TargetAAOnly", &bTargetAAOnly );

	return 0;
}

int SBaseGunRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pWeapon );
	saver.Add( 3, &pAttachedGunVisObj );
	saver.Add( 4, &szAttachedGunLocator );
	saver.Add( 5, &vShootPointOffset );
	saver.Add( 6, &bShootEffectInvert );
	saver.Add( 7, &nPriority );
	saver.Add( 8, &bIsPrimary );
	saver.Add( 9, &nAmmo );
	saver.Add( 10, &fDirection );
	saver.Add( 11, &fReloadCost );
	saver.Add( 12, &bTargetAAOnly );

	return 0;
}

DWORD SBaseGunRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pWeapon << szAttachedGunLocator << vShootPointOffset << bShootEffectInvert << nPriority << bIsPrimary << nAmmo << fDirection << fReloadCost << bTargetAAOnly;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SDynamicDebrisSet::SDynamicDebrisDesc::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Texture", (BYTE*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Season", (BYTE*)&eSeason - pThis, sizeof(eSeason), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Width", (BYTE*)&fWidth - pThis, sizeof(fWidth), NTypeDef::TYPE_TYPE_FLOAT );
}

int SDynamicDebrisSet::SDynamicDebrisDesc::operator&( IXmlSaver &saver )
{
	saver.Add( "Texture", &pTexture );
	saver.Add( "Season", &eSeason );
	saver.Add( "Width", &fWidth );

	return 0;
}

int SDynamicDebrisSet::SDynamicDebrisDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pTexture );
	saver.Add( 3, &eSeason );
	saver.Add( 4, &fWidth );

	return 0;
}

DWORD SDynamicDebrisSet::SDynamicDebrisDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eSeason << fWidth;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SDynamicDebrisSet::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "DynamicDebrisSet", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Debris", &debris, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SDynamicDebrisSet::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Debris", &debris );

	return 0;
}

int SDynamicDebrisSet::operator&( IBinSaver &saver )
{
	saver.Add( 2, &debris );

	return 0;
}



void SObjectBaseRPGStats::SDynamicDebris::SDynamicMaskDesc::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Material", (BYTE*)&pMaterial - pThis, sizeof(pMaterial), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Origin", &vOrigin, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Size", &vSize, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Season", (BYTE*)&eSeason - pThis, sizeof(eSeason), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Width", (BYTE*)&fWidth - pThis, sizeof(fWidth), NTypeDef::TYPE_TYPE_FLOAT );
}

int SObjectBaseRPGStats::SDynamicDebris::SDynamicMaskDesc::operator&( IXmlSaver &saver )
{
	saver.Add( "Material", &pMaterial );
	saver.Add( "Origin", &vOrigin );
	saver.Add( "Size", &vSize );
	saver.Add( "Season", &eSeason );
	saver.Add( "Width", &fWidth );

	return 0;
}

int SObjectBaseRPGStats::SDynamicDebris::SDynamicMaskDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMaterial );
	saver.Add( 3, &vOrigin );
	saver.Add( 4, &vSize );
	saver.Add( 5, &eSeason );
	saver.Add( 6, &fWidth );

	return 0;
}

DWORD SObjectBaseRPGStats::SDynamicDebris::SDynamicMaskDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vOrigin << vSize << eSeason << fWidth;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SObjectBaseRPGStats::SDynamicDebris::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Debris", (BYTE*)&pDebris - pThis, sizeof(pDebris), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Masks", &masks, pThis );
}

int SObjectBaseRPGStats::SDynamicDebris::operator&( IXmlSaver &saver )
{
	saver.Add( "Debris", &pDebris );
	saver.Add( "Masks", &masks );

	return 0;
}

int SObjectBaseRPGStats::SDynamicDebris::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pDebris );
	saver.Add( 3, &masks );

	return 0;
}

DWORD SObjectBaseRPGStats::SDynamicDebris::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << masks;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SObjectBaseRPGStats::SAmbientSound::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "SoundDesc", (BYTE*)&pSoundDesc - pThis, sizeof(pSoundDesc), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Season", (BYTE*)&eSeason - pThis, sizeof(eSeason), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "DayTime", (BYTE*)&nDayTime - pThis, sizeof(nDayTime), NTypeDef::TYPE_TYPE_INT );
}

int SObjectBaseRPGStats::SAmbientSound::operator&( IXmlSaver &saver )
{
	saver.Add( "SoundDesc", &pSoundDesc );
	saver.Add( "Season", &eSeason );
	saver.Add( "DayTime", &nDayTime );

	return 0;
}

int SObjectBaseRPGStats::SAmbientSound::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pSoundDesc );
	saver.Add( 3, &eSeason );
	saver.Add( 4, &nDayTime );

	return 0;
}

DWORD SObjectBaseRPGStats::SAmbientSound::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eSeason << nDayTime;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SObjectBaseRPGStats::ReportMetaInfo() const
{
	SStaticObjectRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "DynamicDebris", &dynamicDebris, pThis );
	NMetaInfo::ReportStructMetaInfo( "passability", &passability, pThis );
	NMetaInfo::ReportStructMetaInfo( "PassProfile", &passProfile, pThis );
	NMetaInfo::ReportStructMetaInfo( "Origin", &vOrigin, pThis );
	NMetaInfo::ReportMetaInfo( "UsePassabilityForVisibility", (BYTE*)&bUsePassabilityForVisibility - pThis, sizeof(bUsePassabilityForVisibility), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructMetaInfo( "visibility", &visibility, pThis );
	NMetaInfo::ReportStructMetaInfo( "VisOrigin", &vVisOrigin, pThis );
	NMetaInfo::ReportMetaInfo( "AmbientSound", (BYTE*)&pAmbientSound - pThis, sizeof(pAmbientSound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "CycledSound", (BYTE*)&pCycledSound - pThis, sizeof(pCycledSound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "AmbientSounds", &ambientSounds, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "CycledSoundTimed", &cycledSoundTimed, pThis );
	NMetaInfo::ReportMetaInfo( "CanFall", (BYTE*)&bCanFall - pThis, sizeof(bCanFall), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "ObjectHeight", (BYTE*)&nObjectHeight - pThis, sizeof(nObjectHeight), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "FallEffect", (BYTE*)&pFallEffect - pThis, sizeof(pFallEffect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "FallDuration", (BYTE*)&nFallDuration - pThis, sizeof(nFallDuration), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "FallCycles", (BYTE*)&fFallCycles - pThis, sizeof(fFallCycles), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "SeasonedEffectExplosion", (BYTE*)&pSeasonedEffectExplosion - pThis, sizeof(pSeasonedEffectExplosion), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SeasonedEffectDeath", (BYTE*)&pSeasonedEffectDeath - pThis, sizeof(pSeasonedEffectDeath), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SeasonedFallEffect", (BYTE*)&pSeasonedFallEffect - pThis, sizeof(pSeasonedFallEffect), NTypeDef::TYPE_TYPE_REF );
}

int SObjectBaseRPGStats::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SStaticObjectRPGStats*)(this) );
	saver.Add( "DynamicDebris", &dynamicDebris );
	saver.Add( "passability", &passability );
	saver.Add( "PassProfile", &passProfile );
	saver.Add( "Origin", &vOrigin );
	saver.Add( "UsePassabilityForVisibility", &bUsePassabilityForVisibility );
	saver.Add( "visibility", &visibility );
	saver.Add( "VisOrigin", &vVisOrigin );
	saver.Add( "AmbientSound", &pAmbientSound );
	saver.Add( "CycledSound", &pCycledSound );
	saver.Add( "AmbientSounds", &ambientSounds );
	saver.Add( "CycledSoundTimed", &cycledSoundTimed );
	saver.Add( "CanFall", &bCanFall );
	saver.Add( "ObjectHeight", &nObjectHeight );
	saver.Add( "FallEffect", &pFallEffect );
	saver.Add( "FallDuration", &nFallDuration );
	saver.Add( "FallCycles", &fFallCycles );
	saver.Add( "SeasonedEffectExplosion", &pSeasonedEffectExplosion );
	saver.Add( "SeasonedEffectDeath", &pSeasonedEffectDeath );
	saver.Add( "SeasonedFallEffect", &pSeasonedFallEffect );

	return 0;
}

int SObjectBaseRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SStaticObjectRPGStats*)this );
	saver.Add( 3, &dynamicDebris );
	saver.Add( 4, &passability );
	saver.Add( 5, &passProfile );
	saver.Add( 6, &vOrigin );
	saver.Add( 7, &bUsePassabilityForVisibility );
	saver.Add( 8, &visibility );
	saver.Add( 9, &vVisOrigin );
	saver.Add( 10, &pAmbientSound );
	saver.Add( 11, &pCycledSound );
	saver.Add( 12, &ambientSounds );
	saver.Add( 13, &cycledSoundTimed );
	saver.Add( 14, &bCanFall );
	saver.Add( 15, &nObjectHeight );
	saver.Add( 16, &pFallEffect );
	saver.Add( 17, &nFallDuration );
	saver.Add( 18, &fFallCycles );
	saver.Add( 19, &pSeasonedEffectExplosion );
	saver.Add( 20, &pSeasonedEffectDeath );
	saver.Add( 21, &pSeasonedFallEffect );

	return 0;
}

DWORD SObjectBaseRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SStaticObjectRPGStats::CalcCheckSum() << dynamicDebris << passability << passProfile << vOrigin << bUsePassabilityForVisibility << visibility << vVisOrigin << ambientSounds << cycledSoundTimed << nObjectHeight;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void STerraObjSetRPGStats::SSegment::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "passability", &passability, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Origin", &vOrigin, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "visibility", &visibility, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "VisOrigin", &vVisOrigin, pThis );
}

int STerraObjSetRPGStats::SSegment::operator&( IXmlSaver &saver )
{
	saver.Add( "passability", &passability );
	saver.Add( "Origin", &vOrigin );
	saver.Add( "visibility", &visibility );
	saver.Add( "VisOrigin", &vVisOrigin );

	return 0;
}

int STerraObjSetRPGStats::SSegment::operator&( IBinSaver &saver )
{
	saver.Add( 2, &passability );
	saver.Add( 3, &vOrigin );
	saver.Add( 4, &visibility );
	saver.Add( 5, &vVisOrigin );

	return 0;
}

DWORD STerraObjSetRPGStats::SSegment::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << passability << vOrigin << visibility << vVisOrigin;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void STerraObjSetRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "TerraObjSetRPGStats", typeID, sizeof(*this) );
	SStaticObjectRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "segments", &segments, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "fronts", &fronts, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "backs", &backs, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int STerraObjSetRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SStaticObjectRPGStats*)(this) );
	saver.Add( "segments", &segments );
	saver.Add( "fronts", &fronts );
	saver.Add( "backs", &backs );

	return 0;
}

int STerraObjSetRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SStaticObjectRPGStats*)this );
	saver.Add( 2, &segments );
	saver.Add( 3, &fronts );
	saver.Add( 4, &backs );

	return 0;
}

DWORD STerraObjSetRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SStaticObjectRPGStats::CalcCheckSum() << segments << fronts << backs;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SObjectRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ObjectRPGStats", typeID, sizeof(*this) );
	SObjectBaseRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "SpecificJoints", &specificJoints, pThis );
	NMetaInfo::ReportMetaInfo( "HideForPerformance", (BYTE*)&bHideForPerformance - pThis, sizeof(bHideForPerformance), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::FinishMetaInfoReport();
}

int SObjectRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SObjectBaseRPGStats*)(this) );
	saver.Add( "SpecificJoints", &specificJoints );
	saver.Add( "HideForPerformance", &bHideForPerformance );

	return 0;
}

int SObjectRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SObjectBaseRPGStats*)this );
	saver.Add( 2, &specificJoints );
	saver.Add( 3, &bHideForPerformance );

	return 0;
}

DWORD SObjectRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SObjectBaseRPGStats::CalcCheckSum() << bHideForPerformance;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EBuildingType eValue )
{
	switch ( eValue )
	{
	case NDb::TYPE_BULDING:
		return "TYPE_BULDING";
	case NDb::TYPE_MAIN_RU_STORAGE:
		return "TYPE_MAIN_RU_STORAGE";
	case NDb::TYPE_TEMP_RU_STORAGE:
		return "TYPE_TEMP_RU_STORAGE";
	case NDb::TYPE_DOT:
		return "TYPE_DOT";
	default:
		return "TYPE_BULDING";
	}
}

NDb::EBuildingType NDb::StringToEnum_NDb_EBuildingType( const std::string &szValue )
{
	if ( szValue == "TYPE_BULDING" )
		return NDb::TYPE_BULDING;
	if ( szValue == "TYPE_MAIN_RU_STORAGE" )
		return NDb::TYPE_MAIN_RU_STORAGE;
	if ( szValue == "TYPE_TEMP_RU_STORAGE" )
		return NDb::TYPE_TEMP_RU_STORAGE;
	if ( szValue == "TYPE_DOT" )
		return NDb::TYPE_DOT;
	return NDb::TYPE_BULDING;
}

std::string EnumToString( NDb::EDesignBuildingType eValue )
{
	switch ( eValue )
	{
	case NDb::BUILDING_TYPE_UNKNOWN:
		return "BUILDING_TYPE_UNKNOWN";
	case NDb::Small:
		return "Small";
	case NDb::Medium:
		return "Medium";
	case NDb::Large:
		return "Large";
	default:
		return "BUILDING_TYPE_UNKNOWN";
	}
}

NDb::EDesignBuildingType NDb::StringToEnum_NDb_EDesignBuildingType( const std::string &szValue )
{
	if ( szValue == "BUILDING_TYPE_UNKNOWN" )
		return NDb::BUILDING_TYPE_UNKNOWN;
	if ( szValue == "Small" )
		return NDb::Small;
	if ( szValue == "Medium" )
		return NDb::Medium;
	if ( szValue == "Large" )
		return NDb::Large;
	return NDb::BUILDING_TYPE_UNKNOWN;
}


void SSlotDamageLevel::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "DamageHP", (BYTE*)&fDamageHP - pThis, sizeof(fDamageHP), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "VisObj", (BYTE*)&pVisObj - pThis, sizeof(pVisObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "DamageEffect", (BYTE*)&pDamageEffect - pThis, sizeof(pDamageEffect), NTypeDef::TYPE_TYPE_REF );
}

int SSlotDamageLevel::operator&( IXmlSaver &saver )
{
	saver.Add( "DamageHP", &fDamageHP );
	saver.Add( "VisObj", &pVisObj );
	saver.Add( "DamageEffect", &pDamageEffect );

	return 0;
}

int SSlotDamageLevel::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fDamageHP );
	saver.Add( 3, &pVisObj );
	saver.Add( 4, &pDamageEffect );

	return 0;
}

DWORD SSlotDamageLevel::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fDamageHP;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindowInfo::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "DayObj", (BYTE*)&pDayObj - pThis, sizeof(pDayObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "NightObj", (BYTE*)&pNightObj - pThis, sizeof(pNightObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "DestroyedObj", (BYTE*)&pDestroyedObj - pThis, sizeof(pDestroyedObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "DestroyEffect", (BYTE*)&pDestroyEffect - pThis, sizeof(pDestroyEffect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "DayDamageLevels", &dayDamageLevels, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "NightDamageLevels", &nightDamageLevels, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "MaxHP", (BYTE*)&fMaxHP - pThis, sizeof(fMaxHP), NTypeDef::TYPE_TYPE_FLOAT );
}

int SWindowInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "DayObj", &pDayObj );
	saver.Add( "NightObj", &pNightObj );
	saver.Add( "DestroyedObj", &pDestroyedObj );
	saver.Add( "DestroyEffect", &pDestroyEffect );
	saver.Add( "DayDamageLevels", &dayDamageLevels );
	saver.Add( "NightDamageLevels", &nightDamageLevels );
	saver.Add( "MaxHP", &fMaxHP );

	return 0;
}

int SWindowInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pDayObj );
	saver.Add( 3, &pNightObj );
	saver.Add( 4, &pDestroyedObj );
	saver.Add( 5, &pDestroyEffect );
	saver.Add( 6, &dayDamageLevels );
	saver.Add( 7, &nightDamageLevels );
	saver.Add( 8, &fMaxHP );

	return 0;
}

DWORD SWindowInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << dayDamageLevels << nightDamageLevels << fMaxHP;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBuildingRPGStats::SEntrance::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Stormable", (BYTE*)&bStormable - pThis, sizeof(bStormable), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Dir", (BYTE*)&nDir - pThis, sizeof(nDir), NTypeDef::TYPE_TYPE_INT );
}

int SBuildingRPGStats::SEntrance::operator&( IXmlSaver &saver )
{
	saver.Add( "Pos", &vPos );
	saver.Add( "Stormable", &bStormable );
	saver.Add( "Dir", &nDir );

	return 0;
}

int SBuildingRPGStats::SEntrance::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPos );
	saver.Add( 3, &bStormable );
	saver.Add( 4, &nDir );

	return 0;
}

DWORD SBuildingRPGStats::SEntrance::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos << bStormable << nDir;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBuildingRPGStats::SSlot::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "LocatorName", (BYTE*)&szLocatorName - pThis, sizeof(szLocatorName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "NumFirePlaces", (BYTE*)&nNumFirePlaces - pThis, sizeof(nNumFirePlaces), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "DamageCenter", &vDamageCenter, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Rot", &qRot, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&fDirection - pThis, sizeof(fDirection), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Angle", (BYTE*)&fAngle - pThis, sizeof(fAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Coverage", (BYTE*)&fCoverage - pThis, sizeof(fCoverage), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Gun", &gun, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "RotationSpeed", (BYTE*)&fRotationSpeed - pThis, sizeof(fRotationSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Window", &window, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "WindowScale", &vWindowScale, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "SoldierStatsModifier", (BYTE*)&pSoldierStatsModifier - pThis, sizeof(pSoldierStatsModifier), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "SightMultiplier", (BYTE*)&fSightMultiplier - pThis, sizeof(fSightMultiplier), NTypeDef::TYPE_TYPE_FLOAT );
}

int SBuildingRPGStats::SSlot::operator&( IXmlSaver &saver )
{
	saver.Add( "LocatorName", &szLocatorName );
	saver.Add( "NumFirePlaces", &nNumFirePlaces );
	saver.Add( "Pos", &vPos );
	saver.Add( "DamageCenter", &vDamageCenter );
	saver.Add( "Rot", &qRot );
	saver.Add( "Direction", &fDirection );
	saver.Add( "Angle", &fAngle );
	saver.Add( "Coverage", &fCoverage );
	saver.Add( "Gun", &gun );
	saver.Add( "RotationSpeed", &fRotationSpeed );
	saver.Add( "Window", &window );
	saver.Add( "WindowScale", &vWindowScale );
	saver.Add( "SoldierStatsModifier", &pSoldierStatsModifier );
	saver.Add( "SightMultiplier", &fSightMultiplier );

	return 0;
}

int SBuildingRPGStats::SSlot::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szLocatorName );
	saver.Add( 3, &nNumFirePlaces );
	saver.Add( 4, &vPos );
	saver.Add( 5, &vDamageCenter );
	saver.Add( 6, &qRot );
	saver.Add( 7, &fDirection );
	saver.Add( 8, &fAngle );
	saver.Add( 9, &fCoverage );
	saver.Add( 10, &gun );
	saver.Add( 11, &fRotationSpeed );
	saver.Add( 12, &window );
	saver.Add( 13, &vWindowScale );
	saver.Add( 14, &pSoldierStatsModifier );
	saver.Add( 15, &fSightMultiplier );

	return 0;
}

DWORD SBuildingRPGStats::SSlot::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nNumFirePlaces << vPos << vDamageCenter << qRot << fDirection << fAngle << fCoverage << gun << fRotationSpeed << window << pSoldierStatsModifier << fSightMultiplier;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBuildingRPGStats::SFirePoint::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&fDirection - pThis, sizeof(fDirection), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "VerticalAngle", (BYTE*)&fVerticalAngle - pThis, sizeof(fVerticalAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "FireEffect", (BYTE*)&szFireEffect - pThis, sizeof(szFireEffect), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Coverage", (BYTE*)&fCoverage - pThis, sizeof(fCoverage), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "PicturePosition", &vPicturePosition, pThis );
}

int SBuildingRPGStats::SFirePoint::operator&( IXmlSaver &saver )
{
	saver.Add( "Pos", &vPos );
	saver.Add( "Direction", &fDirection );
	saver.Add( "VerticalAngle", &fVerticalAngle );
	saver.Add( "FireEffect", &szFireEffect );
	saver.Add( "Coverage", &fCoverage );
	saver.Add( "PicturePosition", &vPicturePosition );

	return 0;
}

int SBuildingRPGStats::SFirePoint::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPos );
	saver.Add( 3, &fDirection );
	saver.Add( 4, &fVerticalAngle );
	saver.Add( 5, &szFireEffect );
	saver.Add( 6, &fCoverage );
	saver.Add( 7, &vPicturePosition );

	return 0;
}

DWORD SBuildingRPGStats::SFirePoint::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos << fDirection << fVerticalAngle << szFireEffect << fCoverage << vPicturePosition;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBuildingRPGStats::SDirectionExplosion::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&fDirection - pThis, sizeof(fDirection), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "VerticalAngle", (BYTE*)&fVerticalAngle - pThis, sizeof(fVerticalAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "PicturePosition", &vPicturePosition, pThis );
}

int SBuildingRPGStats::SDirectionExplosion::operator&( IXmlSaver &saver )
{
	saver.Add( "Pos", &vPos );
	saver.Add( "Direction", &fDirection );
	saver.Add( "VerticalAngle", &fVerticalAngle );
	saver.Add( "PicturePosition", &vPicturePosition );

	return 0;
}

int SBuildingRPGStats::SDirectionExplosion::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPos );
	saver.Add( 3, &fDirection );
	saver.Add( 4, &fVerticalAngle );
	saver.Add( 5, &vPicturePosition );

	return 0;
}

DWORD SBuildingRPGStats::SDirectionExplosion::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos << fDirection << fVerticalAngle << vPicturePosition;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBuildingRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "BuildingRPGStats", typeID, sizeof(*this) );
	SObjectBaseRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "type", (BYTE*)&etype - pThis, sizeof(etype), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "RestSlots", (BYTE*)&nRestSlots - pThis, sizeof(nRestSlots), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "MedicalSlots", (BYTE*)&nMedicalSlots - pThis, sizeof(nMedicalSlots), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "PrimaryGun", (BYTE*)&pPrimaryGun - pThis, sizeof(pPrimaryGun), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "slots", &slots, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "entrances", &entrances, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "firePoints", &firePoints, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "smokePoints", &smokePoints, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "dirExplosions", &dirExplosions, pThis );
	NMetaInfo::ReportMetaInfo( "SightMultiplier", (BYTE*)&fSightMultiplier - pThis, sizeof(fSightMultiplier), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "SoldierStatsModifier", (BYTE*)&pSoldierStatsModifier - pThis, sizeof(pSoldierStatsModifier), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "BuildingType", (BYTE*)&eBuildingType - pThis, sizeof(eBuildingType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "VisibilitySrink", (BYTE*)&bVisibilitySrink - pThis, sizeof(bVisibilitySrink), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "ArmorPattern", (BYTE*)&pArmorPattern - pThis, sizeof(pArmorPattern), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "DamageCoeff", (BYTE*)&fDamageCoeff - pThis, sizeof(fDamageCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SBuildingRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SObjectBaseRPGStats*)(this) );
	saver.Add( "type", &etype );
	saver.Add( "RestSlots", &nRestSlots );
	saver.Add( "MedicalSlots", &nMedicalSlots );
	saver.Add( "PrimaryGun", &pPrimaryGun );
	saver.Add( "slots", &slots );
	saver.Add( "entrances", &entrances );
	saver.Add( "firePoints", &firePoints );
	saver.Add( "smokePoints", &smokePoints );
	saver.Add( "dirExplosions", &dirExplosions );
	saver.Add( "SightMultiplier", &fSightMultiplier );
	saver.Add( "SoldierStatsModifier", &pSoldierStatsModifier );
	saver.Add( "BuildingType", &eBuildingType );
	saver.Add( "VisibilitySrink", &bVisibilitySrink );
	saver.Add( "ArmorPattern", &pArmorPattern );
	saver.Add( "DamageCoeff", &fDamageCoeff );

	return 0;
}

int SBuildingRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SObjectBaseRPGStats*)this );
	saver.Add( 2, &etype );
	saver.Add( 3, &nRestSlots );
	saver.Add( 4, &nMedicalSlots );
	saver.Add( 5, &pPrimaryGun );
	saver.Add( 6, &slots );
	saver.Add( 7, &entrances );
	saver.Add( 8, &firePoints );
	saver.Add( 9, &smokePoints );
	saver.Add( 10, &dirExplosions );
	saver.Add( 11, &fSightMultiplier );
	saver.Add( 12, &pSoldierStatsModifier );
	saver.Add( 13, &eBuildingType );
	saver.Add( 14, &bVisibilitySrink );
	saver.Add( 15, &pArmorPattern );
	saver.Add( 16, &fDamageCoeff );

	return 0;
}

DWORD SBuildingRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SObjectBaseRPGStats::CalcCheckSum() << etype << nRestSlots << nMedicalSlots << pPrimaryGun << slots << entrances << firePoints << smokePoints << dirExplosions << fSightMultiplier << pSoldierStatsModifier << eBuildingType << bVisibilitySrink << fDamageCoeff;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::SBridgeRPGStats::EDirection eValue )
{
	switch ( eValue )
	{
	case NDb::SBridgeRPGStats::VERTICAL:
		return "VERTICAL";
	case NDb::SBridgeRPGStats::HORIZONTAL:
		return "HORIZONTAL";
	default:
		return "VERTICAL";
	}
}

NDb::SBridgeRPGStats::EDirection NDb::StringToEnum_NDb_SBridgeRPGStats_EDirection( const std::string &szValue )
{
	if ( szValue == "VERTICAL" )
		return NDb::SBridgeRPGStats::VERTICAL;
	if ( szValue == "HORIZONTAL" )
		return NDb::SBridgeRPGStats::HORIZONTAL;
	return NDb::SBridgeRPGStats::VERTICAL;
}

std::string EnumToString( NDb::SBridgeRPGStats::SSegmentRPGStats::ESegmentType eValue )
{
	switch ( eValue )
	{
	case NDb::SBridgeRPGStats::SSegmentRPGStats::SLAB:
		return "SLAB";
	case NDb::SBridgeRPGStats::SSegmentRPGStats::GIRDER:
		return "GIRDER";
	default:
		return "SLAB";
	}
}

NDb::SBridgeRPGStats::SSegmentRPGStats::ESegmentType NDb::StringToEnum_NDb_SBridgeRPGStats_SSegmentRPGStats_ESegmentType( const std::string &szValue )
{
	if ( szValue == "SLAB" )
		return NDb::SBridgeRPGStats::SSegmentRPGStats::SLAB;
	if ( szValue == "GIRDER" )
		return NDb::SBridgeRPGStats::SSegmentRPGStats::GIRDER;
	return NDb::SBridgeRPGStats::SSegmentRPGStats::SLAB;
}


void SBridgeRPGStats::SSegmentRPGStats::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructMetaInfo( szAddName + "passability", &passability, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Origin", &vOrigin, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "UsePassabilityForVisibility", (BYTE*)&bUsePassabilityForVisibility - pThis, sizeof(bUsePassabilityForVisibility), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructMetaInfo( szAddName + "visibility", &visibility, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "VisOrigin", &vVisOrigin, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "RelPos", &vRelPos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "VisObj", (BYTE*)&pVisObj - pThis, sizeof(pVisObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "FrameIndex", (BYTE*)&nFrameIndex - pThis, sizeof(nFrameIndex), NTypeDef::TYPE_TYPE_INT );
}

int SBridgeRPGStats::SSegmentRPGStats::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "passability", &passability );
	saver.Add( "Origin", &vOrigin );
	saver.Add( "UsePassabilityForVisibility", &bUsePassabilityForVisibility );
	saver.Add( "visibility", &visibility );
	saver.Add( "VisOrigin", &vVisOrigin );
	saver.Add( "RelPos", &vRelPos );
	saver.Add( "VisObj", &pVisObj );
	saver.Add( "FrameIndex", &nFrameIndex );

	return 0;
}

int SBridgeRPGStats::SSegmentRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &passability );
	saver.Add( 4, &vOrigin );
	saver.Add( 5, &bUsePassabilityForVisibility );
	saver.Add( 6, &visibility );
	saver.Add( 7, &vVisOrigin );
	saver.Add( 8, &vRelPos );
	saver.Add( 9, &pVisObj );
	saver.Add( 10, &nFrameIndex );

	return 0;
}

DWORD SBridgeRPGStats::SSegmentRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << passability << vOrigin << bUsePassabilityForVisibility << visibility << vVisOrigin << vRelPos << nFrameIndex;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBridgeRPGStats::SSpan::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Slab", (BYTE*)&nSlab - pThis, sizeof(nSlab), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "BackGirder", (BYTE*)&nBackGirder - pThis, sizeof(nBackGirder), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "FrontGirder", (BYTE*)&nFrontGirder - pThis, sizeof(nFrontGirder), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Width", (BYTE*)&fWidth - pThis, sizeof(fWidth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Length", (BYTE*)&fLength - pThis, sizeof(fLength), NTypeDef::TYPE_TYPE_FLOAT );
}

int SBridgeRPGStats::SSpan::operator&( IXmlSaver &saver )
{
	saver.Add( "Slab", &nSlab );
	saver.Add( "BackGirder", &nBackGirder );
	saver.Add( "FrontGirder", &nFrontGirder );
	saver.Add( "Width", &fWidth );
	saver.Add( "Length", &fLength );

	return 0;
}

int SBridgeRPGStats::SSpan::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nSlab );
	saver.Add( 3, &nBackGirder );
	saver.Add( 4, &nFrontGirder );
	saver.Add( 5, &fWidth );
	saver.Add( 6, &fLength );

	return 0;
}

DWORD SBridgeRPGStats::SSpan::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nSlab << nBackGirder << nFrontGirder << fWidth << fLength;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBridgeRPGStats::SDamageState::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "spans", &spans, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "begins", &begins, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "lines", &lines, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "ends", &ends, pThis );
}

int SBridgeRPGStats::SDamageState::operator&( IXmlSaver &saver )
{
	saver.Add( "spans", &spans );
	saver.Add( "begins", &begins );
	saver.Add( "lines", &lines );
	saver.Add( "ends", &ends );

	return 0;
}

int SBridgeRPGStats::SDamageState::operator&( IBinSaver &saver )
{
	saver.Add( 2, &spans );
	saver.Add( 3, &begins );
	saver.Add( 4, &lines );
	saver.Add( 5, &ends );

	return 0;
}

DWORD SBridgeRPGStats::SDamageState::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << spans << begins << lines << ends;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBridgeRPGStats::SBridgeFirePoint::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&fDirection - pThis, sizeof(fDirection), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "VerticalAngle", (BYTE*)&fVerticalAngle - pThis, sizeof(fVerticalAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "FireEffect", (BYTE*)&szFireEffect - pThis, sizeof(szFireEffect), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "PicturePosition", &vPicturePosition, pThis );
}

int SBridgeRPGStats::SBridgeFirePoint::operator&( IXmlSaver &saver )
{
	saver.Add( "Pos", &vPos );
	saver.Add( "Direction", &fDirection );
	saver.Add( "VerticalAngle", &fVerticalAngle );
	saver.Add( "FireEffect", &szFireEffect );
	saver.Add( "PicturePosition", &vPicturePosition );

	return 0;
}

int SBridgeRPGStats::SBridgeFirePoint::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPos );
	saver.Add( 3, &fDirection );
	saver.Add( 4, &fVerticalAngle );
	saver.Add( 5, &szFireEffect );
	saver.Add( 6, &vPicturePosition );

	return 0;
}

DWORD SBridgeRPGStats::SBridgeFirePoint::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos << fDirection << fVerticalAngle << szFireEffect << vPicturePosition;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBridgeRPGStats::SBridgeDirectionExplosion::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&fDirection - pThis, sizeof(fDirection), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "VerticalAngle", (BYTE*)&fVerticalAngle - pThis, sizeof(fVerticalAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "FireEffect", (BYTE*)&szFireEffect - pThis, sizeof(szFireEffect), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "PicturePosition", &vPicturePosition, pThis );
}

int SBridgeRPGStats::SBridgeDirectionExplosion::operator&( IXmlSaver &saver )
{
	saver.Add( "Pos", &vPos );
	saver.Add( "Direction", &fDirection );
	saver.Add( "VerticalAngle", &fVerticalAngle );
	saver.Add( "FireEffect", &szFireEffect );
	saver.Add( "PicturePosition", &vPicturePosition );

	return 0;
}

int SBridgeRPGStats::SBridgeDirectionExplosion::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPos );
	saver.Add( 3, &fDirection );
	saver.Add( 4, &fVerticalAngle );
	saver.Add( 5, &szFireEffect );
	saver.Add( 6, &vPicturePosition );

	return 0;
}

DWORD SBridgeRPGStats::SBridgeDirectionExplosion::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos << fDirection << fVerticalAngle << szFireEffect << vPicturePosition;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBridgeRPGStats::SElementRPGStats::SBridgeDamageState::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "DamageHP", (BYTE*)&fDamageHP - pThis, sizeof(fDamageHP), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "VisObjects", &visObjects, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "SmokeEffect", (BYTE*)&pSmokeEffect - pThis, sizeof(pSmokeEffect), NTypeDef::TYPE_TYPE_REF );
}

int SBridgeRPGStats::SElementRPGStats::SBridgeDamageState::operator&( IXmlSaver &saver )
{
	saver.Add( "DamageHP", &fDamageHP );
	saver.Add( "VisObjects", &visObjects );
	saver.Add( "SmokeEffect", &pSmokeEffect );

	return 0;
}

int SBridgeRPGStats::SElementRPGStats::SBridgeDamageState::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fDamageHP );
	saver.Add( 3, &visObjects );
	saver.Add( 4, &pSmokeEffect );

	return 0;
}

DWORD SBridgeRPGStats::SElementRPGStats::SBridgeDamageState::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fDamageHP << visObjects;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBridgeRPGStats::SElementRPGStats::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Passability", &passability, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Origin", &vOrigin, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Size", &vSize, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "DamageStates", &damageStates, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "VisualObjects", &visualObjects, pThis );
}

int SBridgeRPGStats::SElementRPGStats::operator&( IXmlSaver &saver )
{
	saver.Add( "Passability", &passability );
	saver.Add( "Origin", &vOrigin );
	saver.Add( "Size", &vSize );
	saver.Add( "DamageStates", &damageStates );
	saver.Add( "VisualObjects", &visualObjects );

	return 0;
}

int SBridgeRPGStats::SElementRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 2, &passability );
	saver.Add( 3, &vOrigin );
	saver.Add( 4, &vSize );
	saver.Add( 5, &damageStates );
	saver.Add( 6, &visualObjects );

	return 0;
}

DWORD SBridgeRPGStats::SElementRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << passability << vOrigin << vSize << damageStates << visualObjects;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBridgeRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "BridgeRPGStats", typeID, sizeof(*this) );
	SStaticObjectRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "Center", &center, pThis );
	NMetaInfo::ReportStructMetaInfo( "End", &end, pThis );
	NMetaInfo::ReportMetaInfo( "Height", (BYTE*)&fHeight - pThis, sizeof(fHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "direction", (BYTE*)&edirection - pThis, sizeof(edirection), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructArrayMetaInfo( "segments", &segments, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "states", &states, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "firePoints", &firePoints, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "smokePoints", &smokePoints, pThis );
	NMetaInfo::ReportMetaInfo( "SmokeEffect", (BYTE*)&szSmokeEffect - pThis, sizeof(szSmokeEffect), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructArrayMetaInfo( "dirExplosions", &dirExplosions, pThis );
	NMetaInfo::ReportMetaInfo( "DirExplosionEffect", (BYTE*)&szDirExplosionEffect - pThis, sizeof(szDirExplosionEffect), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "SightMultiplier", (BYTE*)&fSightMultiplier - pThis, sizeof(fSightMultiplier), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SBridgeRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SStaticObjectRPGStats*)(this) );
	saver.Add( "Center", &center );
	saver.Add( "End", &end );
	saver.Add( "Height", &fHeight );
	saver.Add( "direction", &edirection );
	saver.Add( "segments", &segments );
	saver.Add( "states", &states );
	saver.Add( "firePoints", &firePoints );
	saver.Add( "smokePoints", &smokePoints );
	saver.Add( "SmokeEffect", &szSmokeEffect );
	saver.Add( "dirExplosions", &dirExplosions );
	saver.Add( "DirExplosionEffect", &szDirExplosionEffect );
	saver.Add( "SightMultiplier", &fSightMultiplier );

	return 0;
}

int SBridgeRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SStaticObjectRPGStats*)this );
	saver.Add( 2, &center );
	saver.Add( 3, &end );
	saver.Add( 4, &fHeight );
	saver.Add( 5, &edirection );
	saver.Add( 6, &segments );
	saver.Add( 7, &states );
	saver.Add( 8, &firePoints );
	saver.Add( 9, &smokePoints );
	saver.Add( 10, &szSmokeEffect );
	saver.Add( 11, &dirExplosions );
	saver.Add( 12, &szDirExplosionEffect );
	saver.Add( 13, &fSightMultiplier );

	return 0;
}

DWORD SBridgeRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SStaticObjectRPGStats::CalcCheckSum() << center << end << fHeight << edirection << segments << states << firePoints << smokePoints << szSmokeEffect << dirExplosions << szDirExplosionEffect << fSightMultiplier;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EEntrenchSegmType eValue )
{
	switch ( eValue )
	{
	case NDb::EST_LINE:
		return "EST_LINE";
	case NDb::EST_FIREPLACE:
		return "EST_FIREPLACE";
	case NDb::EST_TERMINATOR:
		return "EST_TERMINATOR";
	case NDb::EST_ARC:
		return "EST_ARC";
	default:
		return "EST_LINE";
	}
}

NDb::EEntrenchSegmType NDb::StringToEnum_NDb_EEntrenchSegmType( const std::string &szValue )
{
	if ( szValue == "EST_LINE" )
		return NDb::EST_LINE;
	if ( szValue == "EST_FIREPLACE" )
		return NDb::EST_FIREPLACE;
	if ( szValue == "EST_TERMINATOR" )
		return NDb::EST_TERMINATOR;
	if ( szValue == "EST_ARC" )
		return NDb::EST_ARC;
	return NDb::EST_LINE;
}


void SEntrenchmentRPGStats::SEntrenchSegmentRPGStats::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "VisObj", (BYTE*)&pVisObj - pThis, sizeof(pVisObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( szAddName + "AABBCenter", &vAABBCenter, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "AABBHalfSize", &vAABBHalfSize, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Coverage", (BYTE*)&fCoverage - pThis, sizeof(fCoverage), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "fireplaces", &fireplaces, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
}

int SEntrenchmentRPGStats::SEntrenchSegmentRPGStats::operator&( IXmlSaver &saver )
{
	saver.Add( "VisObj", &pVisObj );
	saver.Add( "AABBCenter", &vAABBCenter );
	saver.Add( "AABBHalfSize", &vAABBHalfSize );
	saver.Add( "Coverage", &fCoverage );
	saver.Add( "fireplaces", &fireplaces );
	saver.Add( "Type", &eType );

	return 0;
}

int SEntrenchmentRPGStats::SEntrenchSegmentRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pVisObj );
	saver.Add( 3, &vAABBCenter );
	saver.Add( 4, &vAABBHalfSize );
	saver.Add( 5, &fCoverage );
	saver.Add( 6, &fireplaces );
	saver.Add( 7, &eType );

	return 0;
}

DWORD SEntrenchmentRPGStats::SEntrenchSegmentRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vAABBCenter << vAABBHalfSize << fCoverage << fireplaces << eType;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SEntrenchmentRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "EntrenchmentRPGStats", typeID, sizeof(*this) );
	SHPObjectRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "segments", &segments, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "lines", &lines, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "fireplaces", &fireplaces, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "terminators", &terminators, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "arcs", &arcs, pThis );
	NMetaInfo::ReportMetaInfo( "InnerUnitBonus", (BYTE*)&pInnerUnitBonus - pThis, sizeof(pInnerUnitBonus), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SEntrenchmentRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SHPObjectRPGStats*)(this) );
	saver.Add( "segments", &segments );
	saver.Add( "lines", &lines );
	saver.Add( "fireplaces", &fireplaces );
	saver.Add( "terminators", &terminators );
	saver.Add( "arcs", &arcs );
	saver.Add( "InnerUnitBonus", &pInnerUnitBonus );

	return 0;
}

int SEntrenchmentRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SHPObjectRPGStats*)this );
	saver.Add( 2, &segments );
	saver.Add( 3, &lines );
	saver.Add( 4, &fireplaces );
	saver.Add( 5, &terminators );
	saver.Add( 6, &arcs );
	saver.Add( 7, &pInnerUnitBonus );

	return 0;
}

DWORD SEntrenchmentRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SHPObjectRPGStats::CalcCheckSum() << segments << lines << fireplaces << terminators << arcs << pInnerUnitBonus;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EFenceDirection eValue )
{
	switch ( eValue )
	{
	case NDb::FENCE_DIRECTION_0:
		return "FENCE_DIRECTION_0";
	case NDb::FENCE_DIRECTION_1:
		return "FENCE_DIRECTION_1";
	case NDb::FENCE_DIRECTION_2:
		return "FENCE_DIRECTION_2";
	case NDb::FENCE_DIRECTION_3:
		return "FENCE_DIRECTION_3";
	default:
		return "FENCE_DIRECTION_0";
	}
}

NDb::EFenceDirection NDb::StringToEnum_NDb_EFenceDirection( const std::string &szValue )
{
	if ( szValue == "FENCE_DIRECTION_0" )
		return NDb::FENCE_DIRECTION_0;
	if ( szValue == "FENCE_DIRECTION_1" )
		return NDb::FENCE_DIRECTION_1;
	if ( szValue == "FENCE_DIRECTION_2" )
		return NDb::FENCE_DIRECTION_2;
	if ( szValue == "FENCE_DIRECTION_3" )
		return NDb::FENCE_DIRECTION_3;
	return NDb::FENCE_DIRECTION_0;
}

std::string EnumToString( NDb::EFenceDamageType eValue )
{
	switch ( eValue )
	{
	case NDb::FENCE_TYPE_NORMAL:
		return "FENCE_TYPE_NORMAL";
	case NDb::FENCE_TYPE_LDAMAGE:
		return "FENCE_TYPE_LDAMAGE";
	case NDb::FENCE_TYPE_RDAMAGE:
		return "FENCE_TYPE_RDAMAGE";
	case NDb::FENCE_TYPE_CDAMAGE:
		return "FENCE_TYPE_CDAMAGE";
	default:
		return "FENCE_TYPE_NORMAL";
	}
}

NDb::EFenceDamageType NDb::StringToEnum_NDb_EFenceDamageType( const std::string &szValue )
{
	if ( szValue == "FENCE_TYPE_NORMAL" )
		return NDb::FENCE_TYPE_NORMAL;
	if ( szValue == "FENCE_TYPE_LDAMAGE" )
		return NDb::FENCE_TYPE_LDAMAGE;
	if ( szValue == "FENCE_TYPE_RDAMAGE" )
		return NDb::FENCE_TYPE_RDAMAGE;
	if ( szValue == "FENCE_TYPE_CDAMAGE" )
		return NDb::FENCE_TYPE_CDAMAGE;
	return NDb::FENCE_TYPE_NORMAL;
}

std::string EnumToString( NDb::SFenceRPGStats::EFencePlacementMode eValue )
{
	switch ( eValue )
	{
	case NDb::SFenceRPGStats::FENCE_PLACE_ON_TERRAIN:
		return "FENCE_PLACE_ON_TERRAIN";
	case NDb::SFenceRPGStats::FENCE_PLACE_STAGGERED:
		return "FENCE_PLACE_STAGGERED";
	default:
		return "FENCE_PLACE_ON_TERRAIN";
	}
}

NDb::SFenceRPGStats::EFencePlacementMode NDb::StringToEnum_NDb_SFenceRPGStats_EFencePlacementMode( const std::string &szValue )
{
	if ( szValue == "FENCE_PLACE_ON_TERRAIN" )
		return NDb::SFenceRPGStats::FENCE_PLACE_ON_TERRAIN;
	if ( szValue == "FENCE_PLACE_STAGGERED" )
		return NDb::SFenceRPGStats::FENCE_PLACE_STAGGERED;
	return NDb::SFenceRPGStats::FENCE_PLACE_ON_TERRAIN;
}


void SFenceRPGStats::SFenceSegmentRPGStats::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "passability", &passability, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Origin", &vOrigin, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "visibility", &visibility, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "VisOrigin", &vVisOrigin, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "SpriteIndex", (BYTE*)&nSpriteIndex - pThis, sizeof(nSpriteIndex), NTypeDef::TYPE_TYPE_INT );
}

int SFenceRPGStats::SFenceSegmentRPGStats::operator&( IXmlSaver &saver )
{
	saver.Add( "passability", &passability );
	saver.Add( "Origin", &vOrigin );
	saver.Add( "visibility", &visibility );
	saver.Add( "VisOrigin", &vVisOrigin );
	saver.Add( "SpriteIndex", &nSpriteIndex );

	return 0;
}

int SFenceRPGStats::SFenceSegmentRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 2, &passability );
	saver.Add( 3, &vOrigin );
	saver.Add( 4, &visibility );
	saver.Add( 5, &vVisOrigin );
	saver.Add( 6, &nSpriteIndex );

	return 0;
}

DWORD SFenceRPGStats::SFenceSegmentRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << passability << vOrigin << visibility << vVisOrigin << nSpriteIndex;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFenceRPGStats::SDir::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "centers", &centers, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "ldamages", &ldamages, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "rdamages", &rdamages, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "cdamages", &cdamages, pThis );
}

int SFenceRPGStats::SDir::operator&( IXmlSaver &saver )
{
	saver.Add( "centers", &centers );
	saver.Add( "ldamages", &ldamages );
	saver.Add( "rdamages", &rdamages );
	saver.Add( "cdamages", &cdamages );

	return 0;
}

int SFenceRPGStats::SDir::operator&( IBinSaver &saver )
{
	saver.Add( 2, &centers );
	saver.Add( 3, &ldamages );
	saver.Add( 4, &rdamages );
	saver.Add( 5, &cdamages );

	return 0;
}

DWORD SFenceRPGStats::SDir::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << centers << ldamages << rdamages << cdamages;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFenceRPGStats::SSegments::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "VisObjes", &visObjes, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "passability", &passability, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "PassProfile", &passProfile, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "UsePassabilityForVisibility", (BYTE*)&bUsePassabilityForVisibility - pThis, sizeof(bUsePassabilityForVisibility), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Origin", &vOrigin, pThis );
}

int SFenceRPGStats::SSegments::operator&( IXmlSaver &saver )
{
	saver.Add( "VisObjes", &visObjes );
	saver.Add( "passability", &passability );
	saver.Add( "PassProfile", &passProfile );
	saver.Add( "UsePassabilityForVisibility", &bUsePassabilityForVisibility );
	saver.Add( "Origin", &vOrigin );

	return 0;
}

int SFenceRPGStats::SSegments::operator&( IBinSaver &saver )
{
	saver.Add( 2, &visObjes );
	saver.Add( 3, &passability );
	saver.Add( 4, &passProfile );
	saver.Add( 5, &bUsePassabilityForVisibility );
	saver.Add( 6, &vOrigin );

	return 0;
}

DWORD SFenceRPGStats::SSegments::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << visObjes << passability << passProfile << bUsePassabilityForVisibility << vOrigin;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFenceRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "FenceRPGStats", typeID, sizeof(*this) );
	SStaticObjectRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "stats", &stats, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "dirs", &dirs, pThis );
	NMetaInfo::ReportMetaInfo( "PlacementType", (BYTE*)&ePlacementType - pThis, sizeof(ePlacementType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructMetaInfo( "CenterSegments", &centerSegments, pThis );
	NMetaInfo::ReportStructMetaInfo( "DamagedSegments", &damagedSegments, pThis );
	NMetaInfo::ReportStructMetaInfo( "DamagedSegmentsOtherSide", &damagedSegmentsOtherSide, pThis );
	NMetaInfo::ReportStructMetaInfo( "DestroyedSegments", &destroyedSegments, pThis );
	NMetaInfo::ReportMetaInfo( "FenceHeight", (BYTE*)&fFenceHeight - pThis, sizeof(fFenceHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SFenceRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SStaticObjectRPGStats*)(this) );
	saver.Add( "stats", &stats );
	saver.Add( "dirs", &dirs );
	saver.Add( "PlacementType", &ePlacementType );
	saver.Add( "CenterSegments", &centerSegments );
	saver.Add( "DamagedSegments", &damagedSegments );
	saver.Add( "DamagedSegmentsOtherSide", &damagedSegmentsOtherSide );
	saver.Add( "DestroyedSegments", &destroyedSegments );
	saver.Add( "FenceHeight", &fFenceHeight );

	return 0;
}

int SFenceRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SStaticObjectRPGStats*)this );
	saver.Add( 2, &stats );
	saver.Add( 3, &dirs );
	saver.Add( 4, &ePlacementType );
	saver.Add( 5, &centerSegments );
	saver.Add( 6, &damagedSegments );
	saver.Add( 7, &damagedSegmentsOtherSide );
	saver.Add( 8, &destroyedSegments );
	saver.Add( 9, &fFenceHeight );

	return 0;
}

DWORD SFenceRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SStaticObjectRPGStats::CalcCheckSum() << stats << dirs << ePlacementType << centerSegments << damagedSegments << damagedSegmentsOtherSide << destroyedSegments << fFenceHeight;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EMineType eValue )
{
	switch ( eValue )
	{
	case NDb::MT_INFANTRY:
		return "MT_INFANTRY";
	case NDb::MT_TECHNICS:
		return "MT_TECHNICS";
	case NDb::MT_CHARGE:
		return "MT_CHARGE";
	case NDb::MT_LANDMINE:
		return "MT_LANDMINE";
	case NDb::MT_INFANTRY_AND_TECHNICS:
		return "MT_INFANTRY_AND_TECHNICS";
	default:
		return "MT_INFANTRY";
	}
}

NDb::EMineType NDb::StringToEnum_NDb_EMineType( const std::string &szValue )
{
	if ( szValue == "MT_INFANTRY" )
		return NDb::MT_INFANTRY;
	if ( szValue == "MT_TECHNICS" )
		return NDb::MT_TECHNICS;
	if ( szValue == "MT_CHARGE" )
		return NDb::MT_CHARGE;
	if ( szValue == "MT_LANDMINE" )
		return NDb::MT_LANDMINE;
	if ( szValue == "MT_INFANTRY_AND_TECHNICS" )
		return NDb::MT_INFANTRY_AND_TECHNICS;
	return NDb::MT_INFANTRY;
}


void SMineRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "MineRPGStats", typeID, sizeof(*this) );
	SObjectBaseRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Weapon", (BYTE*)&pWeapon - pThis, sizeof(pWeapon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "type", (BYTE*)&etype - pThis, sizeof(etype), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Weight", (BYTE*)&fWeight - pThis, sizeof(fWeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FlagModel", (BYTE*)&szFlagModel - pThis, sizeof(szFlagModel), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "TriggerRange", (BYTE*)&nTriggerRange - pThis, sizeof(nTriggerRange), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "RangeDetonator", (BYTE*)&bRangeDetonator - pThis, sizeof(bRangeDetonator), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::FinishMetaInfoReport();
}

int SMineRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SObjectBaseRPGStats*)(this) );
	saver.Add( "Weapon", &pWeapon );
	saver.Add( "type", &etype );
	saver.Add( "Weight", &fWeight );
	saver.Add( "FlagModel", &szFlagModel );
	saver.Add( "TriggerRange", &nTriggerRange );
	saver.Add( "RangeDetonator", &bRangeDetonator );

	return 0;
}

int SMineRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SObjectBaseRPGStats*)this );
	saver.Add( 2, &pWeapon );
	saver.Add( 3, &etype );
	saver.Add( 4, &fWeight );
	saver.Add( 5, &szFlagModel );
	saver.Add( 6, &nTriggerRange );
	saver.Add( 7, &bRangeDetonator );

	return 0;
}

DWORD SMineRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SObjectBaseRPGStats::CalcCheckSum() << pWeapon << etype << fWeight << szFlagModel << nTriggerRange << bRangeDetonator;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAnimDesc::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Length", (BYTE*)&nLength - pThis, sizeof(nLength), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Action", (BYTE*)&nAction - pThis, sizeof(nAction), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "FrameIndex", (BYTE*)&nFrameIndex - pThis, sizeof(nFrameIndex), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "AABB_A", (BYTE*)&nAABB_A - pThis, sizeof(nAABB_A), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "AABB_D", (BYTE*)&nAABB_D - pThis, sizeof(nAABB_D), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Animation", (BYTE*)&pAnimation - pThis, sizeof(pAnimation), NTypeDef::TYPE_TYPE_REF );
}

int SAnimDesc::operator&( IXmlSaver &saver )
{
	saver.Add( "Length", &nLength );
	saver.Add( "Action", &nAction );
	saver.Add( "FrameIndex", &nFrameIndex );
	saver.Add( "AABB_A", &nAABB_A );
	saver.Add( "AABB_D", &nAABB_D );
	saver.Add( "Animation", &pAnimation );

	return 0;
}

int SAnimDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nLength );
	saver.Add( 3, &nAction );
	saver.Add( 4, &nFrameIndex );
	saver.Add( 5, &nAABB_A );
	saver.Add( 6, &nAABB_D );
	saver.Add( 7, &pAnimation );

	return 0;
}

DWORD SAnimDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nLength << nAction << nFrameIndex << nAABB_A << nAABB_D << pAnimation;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void Svector_AnimDescs::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "anims", &anims, pThis );
}

int Svector_AnimDescs::operator&( IXmlSaver &saver )
{
	saver.Add( "anims", &anims );

	return 0;
}

int Svector_AnimDescs::operator&( IBinSaver &saver )
{
	saver.Add( 2, &anims );

	return 0;
}

DWORD Svector_AnimDescs::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << anims;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUnitActions::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UnitActions", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "availCommands", (BYTE*)&availCommands - pThis, sizeof(availCommands), NTypeDef::TYPE_TYPE_BINARY );
	NMetaInfo::ReportMetaInfo( "availExposures", (BYTE*)&availExposures - pThis, sizeof(availExposures), NTypeDef::TYPE_TYPE_BINARY );
	NMetaInfo::ReportStructMetaInfo( "availUserActions", &availUserActions, pThis );
	NMetaInfo::ReportStructMetaInfo( "availUserExposures", &availUserExposures, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "SpecialAbilities", &specialAbilities, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SUnitActions::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "availCommands", &availCommands );
	saver.Add( "availExposures", &availExposures );
	saver.Add( "availUserActions", &availUserActions );
	saver.Add( "availUserExposures", &availUserExposures );
	saver.Add( "SpecialAbilities", &specialAbilities );

	return 0;
}

int SUnitActions::operator&( IBinSaver &saver )
{
	saver.Add( 2, &availCommands );
	saver.Add( 3, &availExposures );
	saver.Add( 4, &availUserActions );
	saver.Add( 5, &availUserExposures );
	saver.Add( 6, &specialAbilities );

	return 0;
}

DWORD SUnitActions::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << availCommands << availExposures << availUserActions << availUserExposures << specialAbilities;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EDBUnitRPGType eValue )
{
	switch ( eValue )
	{
	case NDb::DB_RPG_TYPE_SOLDIER:
		return "DB_RPG_TYPE_SOLDIER";
	case NDb::DB_RPG_TYPE_ENGINEER:
		return "DB_RPG_TYPE_ENGINEER";
	case NDb::DB_RPG_TYPE_SNIPER:
		return "DB_RPG_TYPE_SNIPER";
	case NDb::DB_RPG_TYPE_OFFICER:
		return "DB_RPG_TYPE_OFFICER";
	case NDb::DB_RPG_TYPE_TRN_CARRIER:
		return "DB_RPG_TYPE_TRN_CARRIER";
	case NDb::DB_RPG_TYPE_TRN_SUPPORT:
		return "DB_RPG_TYPE_TRN_SUPPORT";
	case NDb::DB_RPG_TYPE_TRN_MEDICINE:
		return "DB_RPG_TYPE_TRN_MEDICINE";
	case NDb::DB_RPG_TYPE_TRN_TRACTOR:
		return "DB_RPG_TYPE_TRN_TRACTOR";
	case NDb::DB_RPG_TYPE_TRN_MILITARY_AUTO:
		return "DB_RPG_TYPE_TRN_MILITARY_AUTO";
	case NDb::DB_RPG_TYPE_TRN_CIVILIAN_AUTO:
		return "DB_RPG_TYPE_TRN_CIVILIAN_AUTO";
	case NDb::DB_RPG_TYPE_ART_GUN:
		return "DB_RPG_TYPE_ART_GUN";
	case NDb::DB_RPG_TYPE_ART_HOWITZER:
		return "DB_RPG_TYPE_ART_HOWITZER";
	case NDb::DB_RPG_TYPE_ART_HEAVY_GUN:
		return "DB_RPG_TYPE_ART_HEAVY_GUN";
	case NDb::DB_RPG_TYPE_ART_AAGUN:
		return "DB_RPG_TYPE_ART_AAGUN";
	case NDb::DB_RPG_TYPE_ART_ROCKET:
		return "DB_RPG_TYPE_ART_ROCKET";
	case NDb::DB_RPG_TYPE_ART_SUPER:
		return "DB_RPG_TYPE_ART_SUPER";
	case NDb::DB_RPG_TYPE_ART_MORTAR:
		return "DB_RPG_TYPE_ART_MORTAR";
	case NDb::DB_RPG_TYPE_ART_HEAVY_MG:
		return "DB_RPG_TYPE_ART_HEAVY_MG";
	case NDb::DB_RPG_TYPE_SPG_ASSAULT:
		return "DB_RPG_TYPE_SPG_ASSAULT";
	case NDb::DB_RPG_TYPE_SPG_ANTITANK:
		return "DB_RPG_TYPE_SPG_ANTITANK";
	case NDb::DB_RPG_TYPE_SPG_SUPER:
		return "DB_RPG_TYPE_SPG_SUPER";
	case NDb::DB_RPG_TYPE_SPG_AAGUN:
		return "DB_RPG_TYPE_SPG_AAGUN";
	case NDb::DB_RPG_TYPE_ARM_LIGHT:
		return "DB_RPG_TYPE_ARM_LIGHT";
	case NDb::DB_RPG_TYPE_ARM_MEDIUM:
		return "DB_RPG_TYPE_ARM_MEDIUM";
	case NDb::DB_RPG_TYPE_ARM_HEAVY:
		return "DB_RPG_TYPE_ARM_HEAVY";
	case NDb::DB_RPG_TYPE_ARM_SUPER:
		return "DB_RPG_TYPE_ARM_SUPER";
	case NDb::DB_RPG_TYPE_AVIA_SCOUT:
		return "DB_RPG_TYPE_AVIA_SCOUT";
	case NDb::DB_RPG_TYPE_AVIA_BOMBER:
		return "DB_RPG_TYPE_AVIA_BOMBER";
	case NDb::DB_RPG_TYPE_AVIA_ATTACK:
		return "DB_RPG_TYPE_AVIA_ATTACK";
	case NDb::DB_RPG_TYPE_AVIA_FIGHTER:
		return "DB_RPG_TYPE_AVIA_FIGHTER";
	case NDb::DB_RPG_TYPE_AVIA_SUPER:
		return "DB_RPG_TYPE_AVIA_SUPER";
	case NDb::DB_RPG_TYPE_AVIA_LANDER:
		return "DB_RPG_TYPE_AVIA_LANDER";
	case NDb::DB_RPG_TYPE_TRAIN_LOCOMOTIVE:
		return "DB_RPG_TYPE_TRAIN_LOCOMOTIVE";
	case NDb::DB_RPG_TYPE_TRAIN_CARGO:
		return "DB_RPG_TYPE_TRAIN_CARGO";
	case NDb::DB_RPG_TYPE_TRAIN_CARRIER:
		return "DB_RPG_TYPE_TRAIN_CARRIER";
	case NDb::DB_RPG_TYPE_TRAIN_SUPER:
		return "DB_RPG_TYPE_TRAIN_SUPER";
	case NDb::DB_RPG_TYPE_TRAIN_ARMOR:
		return "DB_RPG_TYPE_TRAIN_ARMOR";
	case NDb::DB_RPG_TYPE_COUNT:
		return "DB_RPG_TYPE_COUNT";
	default:
		return "DB_RPG_TYPE_SOLDIER";
	}
}

NDb::EDBUnitRPGType NDb::StringToEnum_NDb_EDBUnitRPGType( const std::string &szValue )
{
	if ( szValue == "DB_RPG_TYPE_SOLDIER" )
		return NDb::DB_RPG_TYPE_SOLDIER;
	if ( szValue == "DB_RPG_TYPE_ENGINEER" )
		return NDb::DB_RPG_TYPE_ENGINEER;
	if ( szValue == "DB_RPG_TYPE_SNIPER" )
		return NDb::DB_RPG_TYPE_SNIPER;
	if ( szValue == "DB_RPG_TYPE_OFFICER" )
		return NDb::DB_RPG_TYPE_OFFICER;
	if ( szValue == "DB_RPG_TYPE_TRN_CARRIER" )
		return NDb::DB_RPG_TYPE_TRN_CARRIER;
	if ( szValue == "DB_RPG_TYPE_TRN_SUPPORT" )
		return NDb::DB_RPG_TYPE_TRN_SUPPORT;
	if ( szValue == "DB_RPG_TYPE_TRN_MEDICINE" )
		return NDb::DB_RPG_TYPE_TRN_MEDICINE;
	if ( szValue == "DB_RPG_TYPE_TRN_TRACTOR" )
		return NDb::DB_RPG_TYPE_TRN_TRACTOR;
	if ( szValue == "DB_RPG_TYPE_TRN_MILITARY_AUTO" )
		return NDb::DB_RPG_TYPE_TRN_MILITARY_AUTO;
	if ( szValue == "DB_RPG_TYPE_TRN_CIVILIAN_AUTO" )
		return NDb::DB_RPG_TYPE_TRN_CIVILIAN_AUTO;
	if ( szValue == "DB_RPG_TYPE_ART_GUN" )
		return NDb::DB_RPG_TYPE_ART_GUN;
	if ( szValue == "DB_RPG_TYPE_ART_HOWITZER" )
		return NDb::DB_RPG_TYPE_ART_HOWITZER;
	if ( szValue == "DB_RPG_TYPE_ART_HEAVY_GUN" )
		return NDb::DB_RPG_TYPE_ART_HEAVY_GUN;
	if ( szValue == "DB_RPG_TYPE_ART_AAGUN" )
		return NDb::DB_RPG_TYPE_ART_AAGUN;
	if ( szValue == "DB_RPG_TYPE_ART_ROCKET" )
		return NDb::DB_RPG_TYPE_ART_ROCKET;
	if ( szValue == "DB_RPG_TYPE_ART_SUPER" )
		return NDb::DB_RPG_TYPE_ART_SUPER;
	if ( szValue == "DB_RPG_TYPE_ART_MORTAR" )
		return NDb::DB_RPG_TYPE_ART_MORTAR;
	if ( szValue == "DB_RPG_TYPE_ART_HEAVY_MG" )
		return NDb::DB_RPG_TYPE_ART_HEAVY_MG;
	if ( szValue == "DB_RPG_TYPE_SPG_ASSAULT" )
		return NDb::DB_RPG_TYPE_SPG_ASSAULT;
	if ( szValue == "DB_RPG_TYPE_SPG_ANTITANK" )
		return NDb::DB_RPG_TYPE_SPG_ANTITANK;
	if ( szValue == "DB_RPG_TYPE_SPG_SUPER" )
		return NDb::DB_RPG_TYPE_SPG_SUPER;
	if ( szValue == "DB_RPG_TYPE_SPG_AAGUN" )
		return NDb::DB_RPG_TYPE_SPG_AAGUN;
	if ( szValue == "DB_RPG_TYPE_ARM_LIGHT" )
		return NDb::DB_RPG_TYPE_ARM_LIGHT;
	if ( szValue == "DB_RPG_TYPE_ARM_MEDIUM" )
		return NDb::DB_RPG_TYPE_ARM_MEDIUM;
	if ( szValue == "DB_RPG_TYPE_ARM_HEAVY" )
		return NDb::DB_RPG_TYPE_ARM_HEAVY;
	if ( szValue == "DB_RPG_TYPE_ARM_SUPER" )
		return NDb::DB_RPG_TYPE_ARM_SUPER;
	if ( szValue == "DB_RPG_TYPE_AVIA_SCOUT" )
		return NDb::DB_RPG_TYPE_AVIA_SCOUT;
	if ( szValue == "DB_RPG_TYPE_AVIA_BOMBER" )
		return NDb::DB_RPG_TYPE_AVIA_BOMBER;
	if ( szValue == "DB_RPG_TYPE_AVIA_ATTACK" )
		return NDb::DB_RPG_TYPE_AVIA_ATTACK;
	if ( szValue == "DB_RPG_TYPE_AVIA_FIGHTER" )
		return NDb::DB_RPG_TYPE_AVIA_FIGHTER;
	if ( szValue == "DB_RPG_TYPE_AVIA_SUPER" )
		return NDb::DB_RPG_TYPE_AVIA_SUPER;
	if ( szValue == "DB_RPG_TYPE_AVIA_LANDER" )
		return NDb::DB_RPG_TYPE_AVIA_LANDER;
	if ( szValue == "DB_RPG_TYPE_TRAIN_LOCOMOTIVE" )
		return NDb::DB_RPG_TYPE_TRAIN_LOCOMOTIVE;
	if ( szValue == "DB_RPG_TYPE_TRAIN_CARGO" )
		return NDb::DB_RPG_TYPE_TRAIN_CARGO;
	if ( szValue == "DB_RPG_TYPE_TRAIN_CARRIER" )
		return NDb::DB_RPG_TYPE_TRAIN_CARRIER;
	if ( szValue == "DB_RPG_TYPE_TRAIN_SUPER" )
		return NDb::DB_RPG_TYPE_TRAIN_SUPER;
	if ( szValue == "DB_RPG_TYPE_TRAIN_ARMOR" )
		return NDb::DB_RPG_TYPE_TRAIN_ARMOR;
	if ( szValue == "DB_RPG_TYPE_COUNT" )
		return NDb::DB_RPG_TYPE_COUNT;
	return NDb::DB_RPG_TYPE_SOLDIER;
}

std::string EnumToString( NDb::EUnitPoliticalSide eValue )
{
	switch ( eValue )
	{
	case NDb::POLITICAL_SIDE_UNKNOWN:
		return "POLITICAL_SIDE_UNKNOWN";
	case NDb::POLITICAL_SIDE_ALLIES:
		return "POLITICAL_SIDE_ALLIES";
	case NDb::POLITICAL_SIDE_GERMAN:
		return "POLITICAL_SIDE_GERMAN";
	case NDb::POLITICAL_SIDE_JAPAN:
		return "POLITICAL_SIDE_JAPAN";
	case NDb::POLITICAL_SIDE_USSR:
		return "POLITICAL_SIDE_USSR";
	case NDb::POLITICAL_SIDE_USA:
		return "POLITICAL_SIDE_USA";
	default:
		return "POLITICAL_SIDE_UNKNOWN";
	}
}

NDb::EUnitPoliticalSide NDb::StringToEnum_NDb_EUnitPoliticalSide( const std::string &szValue )
{
	if ( szValue == "POLITICAL_SIDE_UNKNOWN" )
		return NDb::POLITICAL_SIDE_UNKNOWN;
	if ( szValue == "POLITICAL_SIDE_ALLIES" )
		return NDb::POLITICAL_SIDE_ALLIES;
	if ( szValue == "POLITICAL_SIDE_GERMAN" )
		return NDb::POLITICAL_SIDE_GERMAN;
	if ( szValue == "POLITICAL_SIDE_JAPAN" )
		return NDb::POLITICAL_SIDE_JAPAN;
	if ( szValue == "POLITICAL_SIDE_USSR" )
		return NDb::POLITICAL_SIDE_USSR;
	if ( szValue == "POLITICAL_SIDE_USA" )
		return NDb::POLITICAL_SIDE_USA;
	return NDb::POLITICAL_SIDE_UNKNOWN;
}

std::string EnumToString( NDb::EEncyclopediaFilterUnitType eValue )
{
	switch ( eValue )
	{
	case NDb::EFUT_UNKNOWN:
		return "EFUT_UNKNOWN";
	case NDb::EFUT_ARTILLERY:
		return "EFUT_ARTILLERY";
	case NDb::EFUT_ARMOR:
		return "EFUT_ARMOR";
	case NDb::EFUT_AIR:
		return "EFUT_AIR";
	case NDb::EFUT_SEA:
		return "EFUT_SEA";
	case NDb::EFUT_TRANSPORT:
		return "EFUT_TRANSPORT";
	case NDb::EFUT_MISC:
		return "EFUT_MISC";
	case NDb::EFUT_COUNT:
		return "EFUT_COUNT";
	default:
		return "EFUT_UNKNOWN";
	}
}

NDb::EEncyclopediaFilterUnitType NDb::StringToEnum_NDb_EEncyclopediaFilterUnitType( const std::string &szValue )
{
	if ( szValue == "EFUT_UNKNOWN" )
		return NDb::EFUT_UNKNOWN;
	if ( szValue == "EFUT_ARTILLERY" )
		return NDb::EFUT_ARTILLERY;
	if ( szValue == "EFUT_ARMOR" )
		return NDb::EFUT_ARMOR;
	if ( szValue == "EFUT_AIR" )
		return NDb::EFUT_AIR;
	if ( szValue == "EFUT_SEA" )
		return NDb::EFUT_SEA;
	if ( szValue == "EFUT_TRANSPORT" )
		return NDb::EFUT_TRANSPORT;
	if ( szValue == "EFUT_MISC" )
		return NDb::EFUT_MISC;
	if ( szValue == "EFUT_COUNT" )
		return NDb::EFUT_COUNT;
	return NDb::EFUT_UNKNOWN;
}


void SBoundCircle::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Radius", (BYTE*)&fRadius - pThis, sizeof(fRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "IsRound", (BYTE*)&bIsRound - pThis, sizeof(bIsRound), NTypeDef::TYPE_TYPE_BOOL );
}

int SBoundCircle::operator&( IXmlSaver &saver )
{
	saver.Add( "Radius", &fRadius );
	saver.Add( "IsRound", &bIsRound );

	return 0;
}

int SBoundCircle::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fRadius );
	saver.Add( 3, &bIsRound );

	return 0;
}

DWORD SBoundCircle::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fRadius << bIsRound;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUnitBaseRPGStats::SAABBDesc::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Center", &vCenter, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "HalfSize", &vHalfSize, pThis );
}

int SUnitBaseRPGStats::SAABBDesc::operator&( IXmlSaver &saver )
{
	saver.Add( "Center", &vCenter );
	saver.Add( "HalfSize", &vHalfSize );

	return 0;
}

int SUnitBaseRPGStats::SAABBDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vCenter );
	saver.Add( 3, &vHalfSize );

	return 0;
}

DWORD SUnitBaseRPGStats::SAABBDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vCenter << vHalfSize;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUnitBaseRPGStats::ReportMetaInfo() const
{
	SHPObjectRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "DBtype", (BYTE*)&eDBtype - pThis, sizeof(eDBtype), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "M1UnitTargetType", (BYTE*)&pM1UnitTargetType - pThis, sizeof(pM1UnitTargetType), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "M1UnitSpecific", (BYTE*)&pM1UnitSpecific - pThis, sizeof(pM1UnitSpecific), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "M1UnitActions", (BYTE*)&pM1UnitActions - pThis, sizeof(pM1UnitActions), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PoliticalSide", (BYTE*)&ePoliticalSide - pThis, sizeof(ePoliticalSide), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "FullDescriptionFileRef", (BYTE*)&szFullDescriptionFileRef - pThis, sizeof(szFullDescriptionFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "EncyclopediaFilterUnitType", (BYTE*)&eEncyclopediaFilterUnitType - pThis, sizeof(eEncyclopediaFilterUnitType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "AIPassabilityClass", (BYTE*)&nAIPassabilityClass - pThis, sizeof(nAIPassabilityClass), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportSimpleArrayMetaInfo( "AcksNames", &acksNames, pThis );
	NMetaInfo::ReportMetaInfo( "Sight", (BYTE*)&fSight - pThis, sizeof(fSight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "SightPower", (BYTE*)&fSightPower - pThis, sizeof(fSightPower), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Speed", (BYTE*)&fSpeed - pThis, sizeof(fSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RotateSpeed", (BYTE*)&fRotateSpeed - pThis, sizeof(fRotateSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Passability", (BYTE*)&fPassability - pThis, sizeof(fPassability), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Priority", (BYTE*)&nPriority - pThis, sizeof(nPriority), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Camouflage", (BYTE*)&fCamouflage - pThis, sizeof(fCamouflage), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxArmor", (BYTE*)&nMaxArmor - pThis, sizeof(nMaxArmor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "MinArmor", (BYTE*)&nMinArmor - pThis, sizeof(nMinArmor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "BoundTileRadius", (BYTE*)&nBoundTileRadius - pThis, sizeof(nBoundTileRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Weight", (BYTE*)&fWeight - pThis, sizeof(fWeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Price", (BYTE*)&fPrice - pThis, sizeof(fPrice), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "AABBCenter", &vAABBCenter, pThis );
	NMetaInfo::ReportStructMetaInfo( "AABBHalfSize", &vAABBHalfSize, pThis );
	NMetaInfo::ReportStructMetaInfo( "BoundCircle", &boundCircle, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "aabb_as", &aabb_as, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "aabb_ds", &aabb_ds, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "animdescs", &animdescs, pThis );
	NMetaInfo::ReportMetaInfo( "SmallAABBCoeff", (BYTE*)&fSmallAABBCoeff - pThis, sizeof(fSmallAABBCoeff), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Actions", (BYTE*)&pActions - pThis, sizeof(pActions), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "UninstallTransport", (BYTE*)&fUninstallTransport - pThis, sizeof(fUninstallTransport), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "UninstallRotate", (BYTE*)&fUninstallRotate - pThis, sizeof(fUninstallRotate), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ArmorPattern", (BYTE*)&pArmorPattern - pThis, sizeof(pArmorPattern), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ExpPrice", (BYTE*)&fExpPrice - pThis, sizeof(fExpPrice), NTypeDef::TYPE_TYPE_FLOAT );
}

int SUnitBaseRPGStats::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SHPObjectRPGStats*)(this) );
	saver.Add( "DBtype", &eDBtype );
	saver.Add( "M1UnitTargetType", &pM1UnitTargetType );
	saver.Add( "M1UnitSpecific", &pM1UnitSpecific );
	saver.Add( "M1UnitActions", &pM1UnitActions );
	saver.Add( "PoliticalSide", &ePoliticalSide );
	saver.Add( "FullDescriptionFileRef", &szFullDescriptionFileRef );
	saver.Add( "EncyclopediaFilterUnitType", &eEncyclopediaFilterUnitType );
	saver.Add( "AIPassabilityClass", &nAIPassabilityClass );
	saver.Add( "AcksNames", &acksNames );
	saver.Add( "Sight", &fSight );
	saver.Add( "SightPower", &fSightPower );
	saver.Add( "Speed", &fSpeed );
	saver.Add( "ReverseSpeedModifier", &fReverseSpeedModifier );
	saver.Add( "RotateSpeed", &fRotateSpeed );
	saver.Add( "Passability", &fPassability );
	saver.Add( "Priority", &nPriority );
	saver.Add( "Camouflage", &fCamouflage );
	saver.Add( "MaxArmor", &nMaxArmor );
	saver.Add( "MinArmor", &nMinArmor );
	saver.Add( "BoundTileRadius", &nBoundTileRadius );
	saver.Add( "Weight", &fWeight );
	saver.Add( "Price", &fPrice );
	saver.Add( "AABBCenter", &vAABBCenter );
	saver.Add( "AABBHalfSize", &vAABBHalfSize );
	saver.Add( "BoundCircle", &boundCircle );
	saver.Add( "aabb_as", &aabb_as );
	saver.Add( "aabb_ds", &aabb_ds );
	saver.Add( "animdescs", &animdescs );
	saver.Add( "SmallAABBCoeff", &fSmallAABBCoeff );
	saver.Add( "Actions", &pActions );
	saver.Add( "UninstallTransport", &fUninstallTransport );
	saver.Add( "UninstallRotate", &fUninstallRotate );
	saver.Add( "ArmorPattern", &pArmorPattern );
	saver.Add( "ExpPrice", &fExpPrice );

	return 0;
}

int SUnitBaseRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SHPObjectRPGStats*)this );
	saver.Add( 2, &eDBtype );
	saver.Add( 3, &pM1UnitTargetType );
	saver.Add( 4, &pM1UnitSpecific );
	saver.Add( 5, &pM1UnitActions );
	saver.Add( 6, &ePoliticalSide );
	saver.Add( 7, &szFullDescriptionFileRef );
	saver.Add( 8, &eEncyclopediaFilterUnitType );
	saver.Add( 9, &nAIPassabilityClass );
	saver.Add( 10, &acksNames );
	saver.Add( 11, &fSight );
	saver.Add( 12, &fSightPower );
	saver.Add( 13, &fSpeed );
	saver.Add( 14, &fRotateSpeed );
	saver.Add( 15, &fPassability );
	saver.Add( 16, &nPriority );
	saver.Add( 17, &fCamouflage );
	saver.Add( 18, &nMaxArmor );
	saver.Add( 19, &nMinArmor );
	saver.Add( 20, &nBoundTileRadius );
	saver.Add( 21, &fWeight );
	saver.Add( 22, &fPrice );
	saver.Add( 23, &vAABBCenter );
	saver.Add( 24, &vAABBHalfSize );
	saver.Add( 25, &boundCircle );
	saver.Add( 26, &aabb_as );
	saver.Add( 27, &aabb_ds );
	saver.Add( 28, &animdescs );
	saver.Add( 29, &fSmallAABBCoeff );
	saver.Add( 30, &pActions );
	saver.Add( 31, &fUninstallTransport );
	saver.Add( 32, &fUninstallRotate );
	saver.Add( 33, &pArmorPattern );
	saver.Add( 34, &fExpPrice );
	saver.Add( 35, &fReverseSpeedModifier );

	return 0;
}

DWORD SUnitBaseRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SHPObjectRPGStats::CalcCheckSum() << eDBtype << pM1UnitTargetType << pM1UnitSpecific << pM1UnitActions << ePoliticalSide << eEncyclopediaFilterUnitType << nAIPassabilityClass << acksNames << fSight << fSightPower << fSpeed << fRotateSpeed << fPassability << nPriority << fCamouflage << nMaxArmor << nMinArmor << nBoundTileRadius << fWeight << fPrice << vAABBCenter << vAABBHalfSize << boundCircle << aabb_as << aabb_ds << animdescs << fSmallAABBCoeff << pActions << fUninstallTransport << fUninstallRotate << fExpPrice;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SArmorPatternPlacement::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ArmorPatternPlacement", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "FrontPos", &ptFrontPos, pThis );
	NMetaInfo::ReportStructMetaInfo( "SidePos", &ptSidePos, pThis );
	NMetaInfo::ReportStructMetaInfo( "BackPos", &ptBackPos, pThis );
	NMetaInfo::ReportStructMetaInfo( "TopPos", &ptTopPos, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SArmorPatternPlacement::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "FrontPos", &ptFrontPos );
	saver.Add( "SidePos", &ptSidePos );
	saver.Add( "BackPos", &ptBackPos );
	saver.Add( "TopPos", &ptTopPos );

	return 0;
}

int SArmorPatternPlacement::operator&( IBinSaver &saver )
{
	saver.Add( 2, &ptFrontPos );
	saver.Add( 3, &ptSidePos );
	saver.Add( 4, &ptBackPos );
	saver.Add( 5, &ptTopPos );

	return 0;
}



void SArmorPattern::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ArmorPattern", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Picture", (BYTE*)&pPicture - pThis, sizeof(pPicture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Placement", (BYTE*)&pPlacement - pThis, sizeof(pPlacement), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SArmorPattern::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Picture", &pPicture );
	saver.Add( "Placement", &pPlacement );

	return 0;
}

int SArmorPattern::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pPicture );
	saver.Add( 3, &pPlacement );

	return 0;
}



void SInfantryRPGStats::SInfantryGun::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	SBaseGunRPGStats::ReportMetaInfo( szAddName, pThis );

	NMetaInfo::ReportMetaInfo( szAddName + "Fake", (BYTE*)&nFake - pThis, sizeof(nFake), NTypeDef::TYPE_TYPE_INT );
}

int SInfantryRPGStats::SInfantryGun::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SBaseGunRPGStats*)(this) );
	saver.Add( "Fake", &nFake );

	return 0;
}

int SInfantryRPGStats::SInfantryGun::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SBaseGunRPGStats*)this );
	saver.Add( 2, &nFake );

	return 0;
}

DWORD SInfantryRPGStats::SInfantryGun::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SBaseGunRPGStats::CalcCheckSum() << nFake;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SInfantryRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "InfantryRPGStats", typeID, sizeof(*this) );
	SUnitBaseRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "guns", &guns, pThis );
	NMetaInfo::ReportMetaInfo( "CanAttackUp", (BYTE*)&bCanAttackUp - pThis, sizeof(bCanAttackUp), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "CanAttackDown", (BYTE*)&bCanAttackDown - pThis, sizeof(bCanAttackDown), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "RunSpeed", (BYTE*)&fRunSpeed - pThis, sizeof(fRunSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "CrawlSpeed", (BYTE*)&fCrawlSpeed - pThis, sizeof(fCrawlSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportSimpleArrayMetaInfo( "animtimes", &animtimes, pThis );
	NMetaInfo::ReportMetaInfo( "Armor", (BYTE*)&fArmor - pThis, sizeof(fArmor), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "GunBoneName", (BYTE*)&szGunBoneName - pThis, sizeof(szGunBoneName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SInfantryRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUnitBaseRPGStats*)(this) );
	saver.Add( "guns", &guns );
	saver.Add( "CanAttackUp", &bCanAttackUp );
	saver.Add( "CanAttackDown", &bCanAttackDown );
	saver.Add( "RunSpeed", &fRunSpeed );
	saver.Add( "CrawlSpeed", &fCrawlSpeed );
	saver.Add( "animtimes", &animtimes );
	saver.Add( "Armor", &fArmor );
	saver.Add( "GunBoneName", &szGunBoneName );

	return 0;
}

int SInfantryRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUnitBaseRPGStats*)this );
	saver.Add( 2, &guns );
	saver.Add( 3, &bCanAttackUp );
	saver.Add( 4, &bCanAttackDown );
	saver.Add( 5, &fRunSpeed );
	saver.Add( 6, &fCrawlSpeed );
	saver.Add( 7, &animtimes );
	saver.Add( 8, &fArmor );
	saver.Add( 9, &szGunBoneName );

	return 0;
}

DWORD SInfantryRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SUnitBaseRPGStats::CalcCheckSum() << guns << bCanAttackUp << bCanAttackDown << fRunSpeed << fCrawlSpeed << animtimes << fArmor << szGunBoneName;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SBoardedMechUnitPosition::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&nDirection - pThis, sizeof(nDirection), NTypeDef::TYPE_TYPE_INT );
}

int SBoardedMechUnitPosition::operator&( IXmlSaver &saver )
{
	saver.Add( "Pos", &vPos );
	saver.Add( "Direction", &nDirection );

	return 0;
}

int SBoardedMechUnitPosition::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPos );
	saver.Add( 3, &nDirection );

	return 0;
}

DWORD SBoardedMechUnitPosition::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos << nDirection;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SGunnersVector::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "gunners", &gunners, pThis );
}

int SGunnersVector::operator&( IXmlSaver &saver )
{
	saver.Add( "gunners", &gunners );

	return 0;
}

int SGunnersVector::operator&( IBinSaver &saver )
{
	saver.Add( 2, &gunners );

	return 0;
}

DWORD SGunnersVector::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << gunners;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EDesignUnitType eValue )
{
	switch ( eValue )
	{
	case NDb::UNIT_TYPE_UNKNOWN:
		return "UNIT_TYPE_UNKNOWN";
	case NDb::Heavy_Anti_Aircraft_Gun:
		return "Heavy_Anti_Aircraft_Gun";
	case NDb::Light_Anti_Aircraft_Gun:
		return "Light_Anti_Aircraft_Gun";
	case NDb::Anti_Aircraft_SPG:
		return "Anti_Aircraft_SPG";
	case NDb::Anti_Tank_Gun:
		return "Anti_Tank_Gun";
	case NDb::Assault_Gun:
		return "Assault_Gun";
	case NDb::Field_Artillery:
		return "Field_Artillery";
	case NDb::Heavy_Machine_Gun:
		return "Heavy_Machine_Gun";
	case NDb::MLRS:
		return "MLRS";
	case NDb::Engineering_Track:
		return "Engineering_Track";
	case NDb::Resupply_Track:
		return "Resupply_Track";
	case NDb::Tractor:
		return "Tractor";
	case NDb::APC:
		return "APC";
	case NDb::Armored_Fighting_Vehicle:
		return "Armored_Fighting_Vehicle";
	case NDb::Fighter:
		return "Fighter";
	case NDb::Bomber:
		return "Bomber";
	case NDb::Ground_Attack_Plane:
		return "Ground_Attack_Plane";
	case NDb::Cargo_Plane:
		return "Cargo_Plane";
	case NDb::Recon_Plane:
		return "Recon_Plane";
	case NDb::Coast_Gun:
		return "Coast_Gun";
	case NDb::Landing_Boat:
		return "Landing_Boat";
	case NDb::Mortar:
		return "Mortar";
	case NDb::Light_Tank:
		return "Light_Tank";
	case NDb::Medium_Tank:
		return "Medium_Tank";
	case NDb::Heavy_Tank:
		return "Heavy_Tank";
	case NDb::Tank_Destroyer:
		return "Tank_Destroyer";
	case NDb::Torpedo_Boat:
		return "Torpedo_Boat";
	case NDb::Train:
		return "Train";
	case NDb::Super:
		return "Super";
	default:
		return "UNIT_TYPE_UNKNOWN";
	}
}

NDb::EDesignUnitType NDb::StringToEnum_NDb_EDesignUnitType( const std::string &szValue )
{
	if ( szValue == "UNIT_TYPE_UNKNOWN" )
		return NDb::UNIT_TYPE_UNKNOWN;
	if ( szValue == "Heavy_Anti_Aircraft_Gun" )
		return NDb::Heavy_Anti_Aircraft_Gun;
	if ( szValue == "Light_Anti_Aircraft_Gun" )
		return NDb::Light_Anti_Aircraft_Gun;
	if ( szValue == "Anti_Aircraft_SPG" )
		return NDb::Anti_Aircraft_SPG;
	if ( szValue == "Anti_Tank_Gun" )
		return NDb::Anti_Tank_Gun;
	if ( szValue == "Assault_Gun" )
		return NDb::Assault_Gun;
	if ( szValue == "Field_Artillery" )
		return NDb::Field_Artillery;
	if ( szValue == "Heavy_Machine_Gun" )
		return NDb::Heavy_Machine_Gun;
	if ( szValue == "MLRS" )
		return NDb::MLRS;
	if ( szValue == "Engineering_Track" )
		return NDb::Engineering_Track;
	if ( szValue == "Resupply_Track" )
		return NDb::Resupply_Track;
	if ( szValue == "Tractor" )
		return NDb::Tractor;
	if ( szValue == "APC" )
		return NDb::APC;
	if ( szValue == "Armored_Fighting_Vehicle" )
		return NDb::Armored_Fighting_Vehicle;
	if ( szValue == "Fighter" )
		return NDb::Fighter;
	if ( szValue == "Bomber" )
		return NDb::Bomber;
	if ( szValue == "Ground_Attack_Plane" )
		return NDb::Ground_Attack_Plane;
	if ( szValue == "Cargo_Plane" )
		return NDb::Cargo_Plane;
	if ( szValue == "Recon_Plane" )
		return NDb::Recon_Plane;
	if ( szValue == "Coast_Gun" )
		return NDb::Coast_Gun;
	if ( szValue == "Landing_Boat" )
		return NDb::Landing_Boat;
	if ( szValue == "Mortar" )
		return NDb::Mortar;
	if ( szValue == "Light_Tank" )
		return NDb::Light_Tank;
	if ( szValue == "Medium_Tank" )
		return NDb::Medium_Tank;
	if ( szValue == "Heavy_Tank" )
		return NDb::Heavy_Tank;
	if ( szValue == "Tank_Destroyer" )
		return NDb::Tank_Destroyer;
	if ( szValue == "Torpedo_Boat" )
		return NDb::Torpedo_Boat;
	if ( szValue == "Train" )
		return NDb::Train;
	if ( szValue == "Super" )
		return NDb::Super;
	return NDb::UNIT_TYPE_UNKNOWN;
}


void SMechUnitRPGStats::SMechUnitGun::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	SBaseGunRPGStats::ReportMetaInfo( szAddName, pThis );

	NMetaInfo::ReportMetaInfo( szAddName + "ShootPoint", (BYTE*)&szShootPoint - pThis, sizeof(szShootPoint), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "AIShootPointPos", &vAIShootPointPos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Recoil", (BYTE*)&bRecoil - pThis, sizeof(bRecoil), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "RecoilLength", (BYTE*)&fRecoilLength - pThis, sizeof(fRecoilLength), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "recoilTime", (BYTE*)&nrecoilTime - pThis, sizeof(nrecoilTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "RecoilShakeTime", (BYTE*)&nRecoilShakeTime - pThis, sizeof(nRecoilShakeTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "RecoilShakeAngle", (BYTE*)&fRecoilShakeAngle - pThis, sizeof(fRecoilShakeAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "RecoilPoint", (BYTE*)&szRecoilPoint - pThis, sizeof(szRecoilPoint), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "RotatePoint", (BYTE*)&szRotatePoint - pThis, sizeof(szRotatePoint), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "ModelPart", (BYTE*)&nModelPart - pThis, sizeof(nModelPart), NTypeDef::TYPE_TYPE_INT );
}

int SMechUnitRPGStats::SMechUnitGun::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SBaseGunRPGStats*)(this) );
	saver.Add( "ShootPoint", &szShootPoint );
	saver.Add( "AIShootPointPos", &vAIShootPointPos );
	saver.Add( "Recoil", &bRecoil );
	saver.Add( "RecoilLength", &fRecoilLength );
	saver.Add( "recoilTime", &nrecoilTime );
	saver.Add( "RecoilShakeTime", &nRecoilShakeTime );
	saver.Add( "RecoilShakeAngle", &fRecoilShakeAngle );
	saver.Add( "RecoilPoint", &szRecoilPoint );
	saver.Add( "RotatePoint", &szRotatePoint );
	saver.Add( "ModelPart", &nModelPart );

	return 0;
}

int SMechUnitRPGStats::SMechUnitGun::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SBaseGunRPGStats*)this );
	saver.Add( 2, &szShootPoint );
	saver.Add( 3, &vAIShootPointPos );
	saver.Add( 4, &bRecoil );
	saver.Add( 5, &fRecoilLength );
	saver.Add( 6, &nrecoilTime );
	saver.Add( 7, &nRecoilShakeTime );
	saver.Add( 8, &fRecoilShakeAngle );
	saver.Add( 9, &szRecoilPoint );
	saver.Add( 10, &szRotatePoint );
	saver.Add( 11, &nModelPart );

	return 0;
}

DWORD SMechUnitRPGStats::SMechUnitGun::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SBaseGunRPGStats::CalcCheckSum() << szShootPoint << vAIShootPointPos << bRecoil << fRecoilLength << nrecoilTime << nRecoilShakeTime << fRecoilShakeAngle << szRecoilPoint << szRotatePoint << nModelPart;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMechUnitRPGStats::SConstraint::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Min", (BYTE*)&fMin - pThis, sizeof(fMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Max", (BYTE*)&fMax - pThis, sizeof(fMax), NTypeDef::TYPE_TYPE_FLOAT );
}

int SMechUnitRPGStats::SConstraint::operator&( IXmlSaver &saver )
{
	saver.Add( "Min", &fMin );
	saver.Add( "Max", &fMax );

	return 0;
}

int SMechUnitRPGStats::SConstraint::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fMin );
	saver.Add( 3, &fMax );

	return 0;
}

DWORD SMechUnitRPGStats::SConstraint::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fMin << fMax;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMechUnitRPGStats::SPlatform::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "RecoilShakeAngle", (BYTE*)&fRecoilShakeAngle - pThis, sizeof(fRecoilShakeAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "HorizontalRotationSpeed", (BYTE*)&fHorizontalRotationSpeed - pThis, sizeof(fHorizontalRotationSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "VerticalRotationSpeed", (BYTE*)&fVerticalRotationSpeed - pThis, sizeof(fVerticalRotationSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "ModelPart", (BYTE*)&nModelPart - pThis, sizeof(nModelPart), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "constraint", &constraint, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "constraintVertical", &constraintVertical, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "GunCarriageParts", (BYTE*)&nGunCarriageParts - pThis, sizeof(nGunCarriageParts), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "RotatePoint", (BYTE*)&szRotatePoint - pThis, sizeof(szRotatePoint), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( szAddName + "AIRotatePointPos", &vAIRotatePointPos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "AttachedPlatformVisObj", (BYTE*)&pAttachedPlatformVisObj - pThis, sizeof(pAttachedPlatformVisObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "AttachedPlatformLocator", (BYTE*)&szAttachedPlatformLocator - pThis, sizeof(szAttachedPlatformLocator), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "ParentPlatform", (BYTE*)&nParentPlatform - pThis, sizeof(nParentPlatform), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "guns", &guns, pThis );
}

int SMechUnitRPGStats::SPlatform::operator&( IXmlSaver &saver )
{
	saver.Add( "RecoilShakeAngle", &fRecoilShakeAngle );
	saver.Add( "HorizontalRotationSpeed", &fHorizontalRotationSpeed );
	saver.Add( "VerticalRotationSpeed", &fVerticalRotationSpeed );
	saver.Add( "ModelPart", &nModelPart );
	saver.Add( "constraint", &constraint );
	saver.Add( "constraintVertical", &constraintVertical );
	saver.Add( "GunCarriageParts", &nGunCarriageParts );
	saver.Add( "RotatePoint", &szRotatePoint );
	saver.Add( "AIRotatePointPos", &vAIRotatePointPos );
	saver.Add( "AttachedPlatformVisObj", &pAttachedPlatformVisObj );
	saver.Add( "AttachedPlatformLocator", &szAttachedPlatformLocator );
	saver.Add( "ParentPlatform", &nParentPlatform );
	saver.Add( "guns", &guns );

	return 0;
}

int SMechUnitRPGStats::SPlatform::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fRecoilShakeAngle );
	saver.Add( 3, &fHorizontalRotationSpeed );
	saver.Add( 4, &fVerticalRotationSpeed );
	saver.Add( 5, &nModelPart );
	saver.Add( 6, &constraint );
	saver.Add( 7, &constraintVertical );
	saver.Add( 8, &nGunCarriageParts );
	saver.Add( 9, &szRotatePoint );
	saver.Add( 10, &vAIRotatePointPos );
	saver.Add( 11, &pAttachedPlatformVisObj );
	saver.Add( 12, &szAttachedPlatformLocator );
	saver.Add( 13, &nParentPlatform );
	saver.Add( 14, &guns );

	return 0;
}

DWORD SMechUnitRPGStats::SPlatform::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fRecoilShakeAngle << fHorizontalRotationSpeed << fVerticalRotationSpeed << nModelPart << constraint << constraintVertical << nGunCarriageParts << szRotatePoint << vAIRotatePointPos << szAttachedPlatformLocator << nParentPlatform << guns;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMechUnitRPGStats::SArmor::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Min", (BYTE*)&fMin - pThis, sizeof(fMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Max", (BYTE*)&fMax - pThis, sizeof(fMax), NTypeDef::TYPE_TYPE_FLOAT );
}

int SMechUnitRPGStats::SArmor::operator&( IXmlSaver &saver )
{
	saver.Add( "Min", &fMin );
	saver.Add( "Max", &fMax );

	return 0;
}

int SMechUnitRPGStats::SArmor::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fMin );
	saver.Add( 3, &fMax );

	return 0;
}

DWORD SMechUnitRPGStats::SArmor::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fMin << fMax;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMechUnitRPGStats::SJoggingParams::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Period1", (BYTE*)&fPeriod1 - pThis, sizeof(fPeriod1), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Period2", (BYTE*)&fPeriod2 - pThis, sizeof(fPeriod2), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Amp1", (BYTE*)&fAmp1 - pThis, sizeof(fAmp1), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Amp2", (BYTE*)&fAmp2 - pThis, sizeof(fAmp2), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Phaze1", (BYTE*)&fPhaze1 - pThis, sizeof(fPhaze1), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Phaze2", (BYTE*)&fPhaze2 - pThis, sizeof(fPhaze2), NTypeDef::TYPE_TYPE_FLOAT );
}

int SMechUnitRPGStats::SJoggingParams::operator&( IXmlSaver &saver )
{
	saver.Add( "Period1", &fPeriod1 );
	saver.Add( "Period2", &fPeriod2 );
	saver.Add( "Amp1", &fAmp1 );
	saver.Add( "Amp2", &fAmp2 );
	saver.Add( "Phaze1", &fPhaze1 );
	saver.Add( "Phaze2", &fPhaze2 );

	return 0;
}

int SMechUnitRPGStats::SJoggingParams::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fPeriod1 );
	saver.Add( 3, &fPeriod2 );
	saver.Add( 4, &fAmp1 );
	saver.Add( 5, &fAmp2 );
	saver.Add( 6, &fPhaze1 );
	saver.Add( 7, &fPhaze2 );

	return 0;
}

DWORD SMechUnitRPGStats::SJoggingParams::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fPeriod1 << fPeriod2 << fAmp1 << fAmp2 << fPhaze1 << fPhaze2;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMechUnitRPGStats::SSlotInfo::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "AttachedLocator", (BYTE*)&szAttachedLocator - pThis, sizeof(szAttachedLocator), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "AttachedVisObjects", &attachedVisObjects, pThis );
}

int SMechUnitRPGStats::SSlotInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "AttachedLocator", &szAttachedLocator );
	saver.Add( "AttachedVisObjects", &attachedVisObjects );

	return 0;
}

int SMechUnitRPGStats::SSlotInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szAttachedLocator );
	saver.Add( 3, &attachedVisObjects );

	return 0;
}

DWORD SMechUnitRPGStats::SSlotInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szAttachedLocator << attachedVisObjects;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMechUnitRPGStats::SShipEffects::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "BoardSideEffect", (BYTE*)&pBoardSideEffect - pThis, sizeof(pBoardSideEffect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "BoardSideLocators", &boardSideLocators, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "RastrumEffect", (BYTE*)&pRastrumEffect - pThis, sizeof(pRastrumEffect), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "RastrumLocators", &rastrumLocators, pThis );
}

int SMechUnitRPGStats::SShipEffects::operator&( IXmlSaver &saver )
{
	saver.Add( "BoardSideEffect", &pBoardSideEffect );
	saver.Add( "BoardSideLocators", &boardSideLocators );
	saver.Add( "RastrumEffect", &pRastrumEffect );
	saver.Add( "RastrumLocators", &rastrumLocators );

	return 0;
}

int SMechUnitRPGStats::SShipEffects::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pBoardSideEffect );
	saver.Add( 3, &boardSideLocators );
	saver.Add( 4, &pRastrumEffect );
	saver.Add( 5, &rastrumLocators );

	return 0;
}

DWORD SMechUnitRPGStats::SShipEffects::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << boardSideLocators << rastrumLocators;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMechUnitRPGStats::SSmokeTrailEffect::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "LocatorName", (BYTE*)&szLocatorName - pThis, sizeof(szLocatorName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Interval", (BYTE*)&fInterval - pThis, sizeof(fInterval), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Effect", (BYTE*)&pEffect - pThis, sizeof(pEffect), NTypeDef::TYPE_TYPE_REF );
}

int SMechUnitRPGStats::SSmokeTrailEffect::operator&( IXmlSaver &saver )
{
	saver.Add( "LocatorName", &szLocatorName );
	saver.Add( "Interval", &fInterval );
	saver.Add( "Effect", &pEffect );

	return 0;
}

int SMechUnitRPGStats::SSmokeTrailEffect::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szLocatorName );
	saver.Add( 3, &fInterval );
	saver.Add( 4, &pEffect );

	return 0;
}

DWORD SMechUnitRPGStats::SSmokeTrailEffect::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szLocatorName << fInterval;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMechUnitRPGStats::SCameraPlacement::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Anchor", &vAnchor, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Distance", (BYTE*)&fDistance - pThis, sizeof(fDistance), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Pitch", (BYTE*)&fPitch - pThis, sizeof(fPitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Yaw", (BYTE*)&fYaw - pThis, sizeof(fYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Fov", (BYTE*)&fFov - pThis, sizeof(fFov), NTypeDef::TYPE_TYPE_FLOAT );
}

int SMechUnitRPGStats::SCameraPlacement::operator&( IXmlSaver &saver )
{
	saver.Add( "Anchor", &vAnchor );
	saver.Add( "Distance", &fDistance );
	saver.Add( "Pitch", &fPitch );
	saver.Add( "Yaw", &fYaw );
	saver.Add( "Fov", &fFov );

	return 0;
}

int SMechUnitRPGStats::SCameraPlacement::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vAnchor );
	saver.Add( 3, &fDistance );
	saver.Add( 4, &fPitch );
	saver.Add( 5, &fYaw );
	saver.Add( 6, &fFov );

	return 0;
}

DWORD SMechUnitRPGStats::SCameraPlacement::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vAnchor << fDistance << fPitch << fYaw << fFov;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMechUnitRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "MechUnitRPGStats", typeID, sizeof(*this) );
	SUnitBaseRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "UnitType", (BYTE*)&eUnitType - pThis, sizeof(eUnitType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructArrayMetaInfo( "platforms", &platforms, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "slots", &slots, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "armors", &armors, pThis );
	NMetaInfo::ReportMetaInfo( "TowingForce", (BYTE*)&fTowingForce - pThis, sizeof(fTowingForce), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Crew", (BYTE*)&nCrew - pThis, sizeof(nCrew), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Passangers", (BYTE*)&nPassangers - pThis, sizeof(nPassangers), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TurnRadius", (BYTE*)&fTurnRadius - pThis, sizeof(fTurnRadius), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportSimpleArrayMetaInfo( "exhaustPoints", &exhaustPoints, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "damagePoints", &damagePoints, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "peoplePointIndices", &peoplePointIndices, pThis );
	NMetaInfo::ReportMetaInfo( "FatalitySmokePoint", (BYTE*)&szFatalitySmokePoint - pThis, sizeof(szFatalitySmokePoint), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "ShootDustPoint", (BYTE*)&szShootDustPoint - pThis, sizeof(szShootDustPoint), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructMetaInfo( "TowPoint", &vTowPoint, pThis );
	NMetaInfo::ReportStructMetaInfo( "EntrancePoint", &vEntrancePoint, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "PeoplePoints", &peoplePoints, pThis );
	NMetaInfo::ReportStructMetaInfo( "AmmoPoint", &vAmmoPoint, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Gunners", &gunners, pThis );
	NMetaInfo::ReportStructMetaInfo( "HookPoint", &vHookPoint, pThis );
	NMetaInfo::ReportStructMetaInfo( "FrontWheel", &vFrontWheel, pThis );
	NMetaInfo::ReportStructMetaInfo( "BackWheel", &vBackWheel, pThis );
	NMetaInfo::ReportMetaInfo( "EffectDiesel", (BYTE*)&pEffectDiesel - pThis, sizeof(pEffectDiesel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EffectWheelDust", (BYTE*)&pEffectWheelDust - pThis, sizeof(pEffectWheelDust), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EffectWheelSplash", (BYTE*)&pEffectWheelSplash - pThis, sizeof(pEffectWheelSplash), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EffectSmoke", (BYTE*)&pEffectSmoke - pThis, sizeof(pEffectSmoke), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EffectFatality", (BYTE*)&pEffectFatality - pThis, sizeof(pEffectFatality), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "SmokeTrails", &smokeTrails, pThis );
	NMetaInfo::ReportMetaInfo( "EffectShootDust", (BYTE*)&pEffectShootDust - pThis, sizeof(pEffectShootDust), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EffectEntrenching", (BYTE*)&pEffectEntrenching - pThis, sizeof(pEffectEntrenching), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "EffectDisappear", (BYTE*)&pEffectDisappear - pThis, sizeof(pEffectDisappear), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "jx", &jx, pThis );
	NMetaInfo::ReportStructMetaInfo( "jy", &jy, pThis );
	NMetaInfo::ReportStructMetaInfo( "jz", &jz, pThis );
	NMetaInfo::ReportMetaInfo( "LeavesTracks", (BYTE*)&bLeavesTracks - pThis, sizeof(bLeavesTracks), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "TrackWidth", (BYTE*)&fTrackWidth - pThis, sizeof(fTrackWidth), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TrackOffset", (BYTE*)&fTrackOffset - pThis, sizeof(fTrackOffset), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TrackStart", (BYTE*)&fTrackStart - pThis, sizeof(fTrackStart), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TrackEnd", (BYTE*)&fTrackEnd - pThis, sizeof(fTrackEnd), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TrackIntensity", (BYTE*)&fTrackIntensity - pThis, sizeof(fTrackIntensity), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TrackLifetime", (BYTE*)&nTrackLifetime - pThis, sizeof(nTrackLifetime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TrackFrequency", (BYTE*)&fTrackFrequency - pThis, sizeof(fTrackFrequency), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "SoundMoveStart", (BYTE*)&pSoundMoveStart - pThis, sizeof(pSoundMoveStart), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SoundMoveCycle", (BYTE*)&pSoundMoveCycle - pThis, sizeof(pSoundMoveCycle), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SoundMoveStop", (BYTE*)&pSoundMoveStop - pThis, sizeof(pSoundMoveStop), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SoundIdle", (BYTE*)&pSoundIdle - pThis, sizeof(pSoundIdle), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "SoundDive", (BYTE*)&pSoundDive - pThis, sizeof(pSoundDive), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MaxHeight", (BYTE*)&fMaxHeight - pThis, sizeof(fMaxHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DivingAngle", (BYTE*)&fDivingAngle - pThis, sizeof(fDivingAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ClimbAngle", (BYTE*)&fClimbAngle - pThis, sizeof(fClimbAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TiltAngle", (BYTE*)&fTiltAngle - pThis, sizeof(fTiltAngle), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TiltRatio", (BYTE*)&fTiltRatio - pThis, sizeof(fTiltRatio), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TiltAcceleration", (BYTE*)&fTiltAcceleration - pThis, sizeof(fTiltAcceleration), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "TiltSpeed", (BYTE*)&fTiltSpeed - pThis, sizeof(fTiltSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "GAPAirAttackModifier", (BYTE*)&pGAPAirAttackModifier - pThis, sizeof(pGAPAirAttackModifier), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "deathCraters", (BYTE*)&pdeathCraters - pThis, sizeof(pdeathCraters), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ReinforcementPrice", (BYTE*)&fReinforcementPrice - pThis, sizeof(fReinforcementPrice), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Fuel", (BYTE*)&fFuel - pThis, sizeof(fFuel), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportSimpleArrayMetaInfo( "AllowedPlaneManuvers", &allowedPlaneManuvers, pThis );
	NMetaInfo::ReportMetaInfo( "AnimableModel", (BYTE*)&pAnimableModel - pThis, sizeof(pAnimableModel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "TransportableModel", (BYTE*)&pTransportableModel - pThis, sizeof(pTransportableModel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "ShipEffects", &shipEffects, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "BoardedMechUnitPosition", &boardedMechUnitPosition, pThis );
	NMetaInfo::ReportMetaInfo( "DestructableCorpse", (BYTE*)&bDestructableCorpse - pThis, sizeof(bDestructableCorpse), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "InnerUnitBonus", (BYTE*)&pInnerUnitBonus - pThis, sizeof(pInnerUnitBonus), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SMechUnitRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUnitBaseRPGStats*)(this) );
	saver.Add( "UnitType", &eUnitType );
	saver.Add( "platforms", &platforms );
	saver.Add( "slots", &slots );
	saver.Add( "armors", &armors );
	saver.Add( "TowingForce", &fTowingForce );
	saver.Add( "Crew", &nCrew );
	saver.Add( "Passangers", &nPassangers );
	saver.Add( "TurnRadius", &fTurnRadius );
	saver.Add( "exhaustPoints", &exhaustPoints );
	saver.Add( "damagePoints", &damagePoints );
	saver.Add( "peoplePointIndices", &peoplePointIndices );
	saver.Add( "FatalitySmokePoint", &szFatalitySmokePoint );
	saver.Add( "ShootDustPoint", &szShootDustPoint );
	saver.Add( "TowPoint", &vTowPoint );
	saver.Add( "EntrancePoint", &vEntrancePoint );
	saver.Add( "PeoplePoints", &peoplePoints );
	saver.Add( "AmmoPoint", &vAmmoPoint );
	saver.Add( "Gunners", &gunners );
	saver.Add( "HookPoint", &vHookPoint );
	saver.Add( "FrontWheel", &vFrontWheel );
	saver.Add( "BackWheel", &vBackWheel );
	saver.Add( "EffectDiesel", &pEffectDiesel );
	saver.Add( "EffectWheelDust", &pEffectWheelDust );
	saver.Add( "EffectWheelSplash", &pEffectWheelSplash );
	saver.Add( "EffectSmoke", &pEffectSmoke );
	saver.Add( "EffectFatality", &pEffectFatality );
	saver.Add( "SmokeTrails", &smokeTrails );
	saver.Add( "EffectShootDust", &pEffectShootDust );
	saver.Add( "EffectEntrenching", &pEffectEntrenching );
	saver.Add( "EffectDisappear", &pEffectDisappear );
	saver.Add( "jx", &jx );
	saver.Add( "jy", &jy );
	saver.Add( "jz", &jz );
	saver.Add( "LeavesTracks", &bLeavesTracks );
	saver.Add( "TrackWidth", &fTrackWidth );
	saver.Add( "TrackOffset", &fTrackOffset );
	saver.Add( "TrackStart", &fTrackStart );
	saver.Add( "TrackEnd", &fTrackEnd );
	saver.Add( "TrackIntensity", &fTrackIntensity );
	saver.Add( "TrackLifetime", &nTrackLifetime );
	saver.Add( "TrackFrequency", &fTrackFrequency );
	saver.Add( "SoundMoveStart", &pSoundMoveStart );
	saver.Add( "SoundMoveCycle", &pSoundMoveCycle );
	saver.Add( "SoundMoveStop", &pSoundMoveStop );
	saver.Add( "SoundIdle", &pSoundIdle );
	saver.Add( "SoundDive", &pSoundDive );
	saver.Add( "MaxHeight", &fMaxHeight );
	saver.Add( "DivingAngle", &fDivingAngle );
	saver.Add( "ClimbAngle", &fClimbAngle );
	saver.Add( "TiltAngle", &fTiltAngle );
	saver.Add( "TiltRatio", &fTiltRatio );
	saver.Add( "TiltAcceleration", &fTiltAcceleration );
	saver.Add( "TiltSpeed", &fTiltSpeed );
	saver.Add( "GAPAirAttackModifier", &pGAPAirAttackModifier );
	saver.Add( "deathCraters", &pdeathCraters );
	saver.Add( "ReinforcementPrice", &fReinforcementPrice );
	saver.Add( "Fuel", &fFuel );
	saver.Add( "AllowedPlaneManuvers", &allowedPlaneManuvers );
	saver.Add( "AnimableModel", &pAnimableModel );
	saver.Add( "TransportableModel", &pTransportableModel );
	saver.Add( "ShipEffects", &shipEffects );
	saver.Add( "BoardedMechUnitPosition", &boardedMechUnitPosition );
	saver.Add( "DestructableCorpse", &bDestructableCorpse );
	saver.Add( "InnerUnitBonus", &pInnerUnitBonus );

	return 0;
}

int SMechUnitRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUnitBaseRPGStats*)this );
	saver.Add( 2, &eUnitType );
	saver.Add( 3, &platforms );
	saver.Add( 4, &slots );
	saver.Add( 5, &armors );
	saver.Add( 6, &fTowingForce );
	saver.Add( 7, &nCrew );
	saver.Add( 8, &nPassangers );
	saver.Add( 9, &fTurnRadius );
	saver.Add( 10, &exhaustPoints );
	saver.Add( 11, &damagePoints );
	saver.Add( 12, &peoplePointIndices );
	saver.Add( 13, &szFatalitySmokePoint );
	saver.Add( 14, &szShootDustPoint );
	saver.Add( 15, &vTowPoint );
	saver.Add( 16, &vEntrancePoint );
	saver.Add( 17, &peoplePoints );
	saver.Add( 18, &vAmmoPoint );
	saver.Add( 19, &gunners );
	saver.Add( 20, &vHookPoint );
	saver.Add( 21, &vFrontWheel );
	saver.Add( 22, &vBackWheel );
	saver.Add( 23, &pEffectDiesel );
	saver.Add( 24, &pEffectWheelDust );
	saver.Add( 25, &pEffectWheelSplash );
	saver.Add( 26, &pEffectSmoke );
	saver.Add( 27, &pEffectFatality );
	saver.Add( 28, &smokeTrails );
	saver.Add( 29, &pEffectShootDust );
	saver.Add( 30, &pEffectEntrenching );
	saver.Add( 31, &pEffectDisappear );
	saver.Add( 32, &jx );
	saver.Add( 33, &jy );
	saver.Add( 34, &jz );
	saver.Add( 35, &bLeavesTracks );
	saver.Add( 36, &fTrackWidth );
	saver.Add( 37, &fTrackOffset );
	saver.Add( 38, &fTrackStart );
	saver.Add( 39, &fTrackEnd );
	saver.Add( 40, &fTrackIntensity );
	saver.Add( 41, &nTrackLifetime );
	saver.Add( 42, &fTrackFrequency );
	saver.Add( 43, &pSoundMoveStart );
	saver.Add( 44, &pSoundMoveCycle );
	saver.Add( 45, &pSoundMoveStop );
	saver.Add( 46, &pSoundIdle );
	saver.Add( 47, &pSoundDive );
	saver.Add( 48, &fMaxHeight );
	saver.Add( 49, &fDivingAngle );
	saver.Add( 50, &fClimbAngle );
	saver.Add( 51, &fTiltAngle );
	saver.Add( 52, &fTiltRatio );
	saver.Add( 53, &fTiltAcceleration );
	saver.Add( 54, &fTiltSpeed );
	saver.Add( 55, &pGAPAirAttackModifier );
	saver.Add( 56, &pdeathCraters );
	saver.Add( 57, &fReinforcementPrice );
	saver.Add( 58, &fFuel );
	saver.Add( 59, &allowedPlaneManuvers );
	saver.Add( 60, &pAnimableModel );
	saver.Add( 61, &pTransportableModel );
	saver.Add( 62, &shipEffects );
	saver.Add( 63, &boardedMechUnitPosition );
	saver.Add( 64, &bDestructableCorpse );
	saver.Add( 65, &pInnerUnitBonus );

	return 0;
}

DWORD SMechUnitRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SUnitBaseRPGStats::CalcCheckSum() << eUnitType << platforms << slots << armors << fTowingForce << nCrew << nPassangers << fTurnRadius << exhaustPoints << damagePoints << peoplePointIndices << szFatalitySmokePoint << szShootDustPoint << vTowPoint << vEntrancePoint << peoplePoints << vAmmoPoint << gunners << vHookPoint << vFrontWheel << vBackWheel << smokeTrails << jx << jy << jz << bLeavesTracks << fTrackWidth << fTrackOffset << fTrackStart << fTrackEnd << fTrackIntensity << nTrackLifetime << fTrackFrequency << fMaxHeight << fDivingAngle << fClimbAngle << fTiltAngle << fTiltRatio << fTiltAcceleration << fTiltSpeed << pGAPAirAttackModifier << fReinforcementPrice << fFuel << allowedPlaneManuvers << shipEffects << boardedMechUnitPosition << bDestructableCorpse << pInnerUnitBonus;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EEvent eValue )
{
	switch ( eValue )
	{
	case NDb::HIT_NEAR:
		return "HIT_NEAR";
	default:
		return "HIT_NEAR";
	}
}

NDb::EEvent NDb::StringToEnum_NDb_EEvent( const std::string &szValue )
{
	if ( szValue == "HIT_NEAR" )
		return NDb::HIT_NEAR;
	return NDb::HIT_NEAR;
}

std::string EnumToString( NDb::EDesignSquadType eValue )
{
	switch ( eValue )
	{
	case NDb::SQUAD_TYPE_UNKNOWN:
		return "SQUAD_TYPE_UNKNOWN";
	case NDb::Main_Squad:
		return "Main_Squad";
	case NDb::Machine_Gun_Squad:
		return "Machine_Gun_Squad";
	case NDb::Anti_Tank_Squad:
		return "Anti_Tank_Squad";
	case NDb::Assault_Squad:
		return "Assault_Squad";
	case NDb::Special_Squad:
		return "Special_Squad";
	case NDb::Single_Unit:
		return "Single_Unit";
	default:
		return "SQUAD_TYPE_UNKNOWN";
	}
}

NDb::EDesignSquadType NDb::StringToEnum_NDb_EDesignSquadType( const std::string &szValue )
{
	if ( szValue == "SQUAD_TYPE_UNKNOWN" )
		return NDb::SQUAD_TYPE_UNKNOWN;
	if ( szValue == "Main_Squad" )
		return NDb::Main_Squad;
	if ( szValue == "Machine_Gun_Squad" )
		return NDb::Machine_Gun_Squad;
	if ( szValue == "Anti_Tank_Squad" )
		return NDb::Anti_Tank_Squad;
	if ( szValue == "Assault_Squad" )
		return NDb::Assault_Squad;
	if ( szValue == "Special_Squad" )
		return NDb::Special_Squad;
	if ( szValue == "Single_Unit" )
		return NDb::Single_Unit;
	return NDb::SQUAD_TYPE_UNKNOWN;
}

std::string EnumToString( NDb::SSquadRPGStats::ESquadType eValue )
{
	switch ( eValue )
	{
	case NDb::SSquadRPGStats::RIFLEMANS:
		return "RIFLEMANS";
	case NDb::SSquadRPGStats::INFANTRY:
		return "INFANTRY";
	case NDb::SSquadRPGStats::SUBMACHINEGUNNERS:
		return "SUBMACHINEGUNNERS";
	case NDb::SSquadRPGStats::MACHINEGUNNERS:
		return "MACHINEGUNNERS";
	case NDb::SSquadRPGStats::AT_TEAM:
		return "AT_TEAM";
	case NDb::SSquadRPGStats::MORTAR_TEAM:
		return "MORTAR_TEAM";
	case NDb::SSquadRPGStats::SNIPERS:
		return "SNIPERS";
	case NDb::SSquadRPGStats::GUNNERS:
		return "GUNNERS";
	case NDb::SSquadRPGStats::ENGINEERS:
		return "ENGINEERS";
	default:
		return "RIFLEMANS";
	}
}

NDb::SSquadRPGStats::ESquadType NDb::StringToEnum_NDb_SSquadRPGStats_ESquadType( const std::string &szValue )
{
	if ( szValue == "RIFLEMANS" )
		return NDb::SSquadRPGStats::RIFLEMANS;
	if ( szValue == "INFANTRY" )
		return NDb::SSquadRPGStats::INFANTRY;
	if ( szValue == "SUBMACHINEGUNNERS" )
		return NDb::SSquadRPGStats::SUBMACHINEGUNNERS;
	if ( szValue == "MACHINEGUNNERS" )
		return NDb::SSquadRPGStats::MACHINEGUNNERS;
	if ( szValue == "AT_TEAM" )
		return NDb::SSquadRPGStats::AT_TEAM;
	if ( szValue == "MORTAR_TEAM" )
		return NDb::SSquadRPGStats::MORTAR_TEAM;
	if ( szValue == "SNIPERS" )
		return NDb::SSquadRPGStats::SNIPERS;
	if ( szValue == "GUNNERS" )
		return NDb::SSquadRPGStats::GUNNERS;
	if ( szValue == "ENGINEERS" )
		return NDb::SSquadRPGStats::ENGINEERS;
	return NDb::SSquadRPGStats::RIFLEMANS;
}


void SSquadRPGStats::SFormation::SEntry::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Dir", (BYTE*)&fDir - pThis, sizeof(fDir), NTypeDef::TYPE_TYPE_FLOAT );
}

int SSquadRPGStats::SFormation::SEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "Pos", &vPos );
	saver.Add( "Dir", &fDir );

	return 0;
}

int SSquadRPGStats::SFormation::SEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPos );
	saver.Add( 3, &fDir );

	return 0;
}

DWORD SSquadRPGStats::SFormation::SEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos << fDir;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::SSquadRPGStats::SFormation::EFormationMoveType eValue )
{
	switch ( eValue )
	{
	case NDb::SSquadRPGStats::SFormation::DEFAULT:
		return "DEFAULT";
	case NDb::SSquadRPGStats::SFormation::MOVEMENT:
		return "MOVEMENT";
	case NDb::SSquadRPGStats::SFormation::DEFENSIVE:
		return "DEFENSIVE";
	case NDb::SSquadRPGStats::SFormation::OFFENSIVE:
		return "OFFENSIVE";
	case NDb::SSquadRPGStats::SFormation::SNEAK:
		return "SNEAK";
	default:
		return "DEFAULT";
	}
}

NDb::SSquadRPGStats::SFormation::EFormationMoveType NDb::StringToEnum_NDb_SSquadRPGStats_SFormation_EFormationMoveType( const std::string &szValue )
{
	if ( szValue == "DEFAULT" )
		return NDb::SSquadRPGStats::SFormation::DEFAULT;
	if ( szValue == "MOVEMENT" )
		return NDb::SSquadRPGStats::SFormation::MOVEMENT;
	if ( szValue == "DEFENSIVE" )
		return NDb::SSquadRPGStats::SFormation::DEFENSIVE;
	if ( szValue == "OFFENSIVE" )
		return NDb::SSquadRPGStats::SFormation::OFFENSIVE;
	if ( szValue == "SNEAK" )
		return NDb::SSquadRPGStats::SFormation::SNEAK;
	return NDb::SSquadRPGStats::SFormation::DEFAULT;
}


void SSquadRPGStats::SFormation::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "type", (BYTE*)&etype - pThis, sizeof(etype), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "order", &order, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "LieFlag", (BYTE*)&nLieFlag - pThis, sizeof(nLieFlag), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "StatsModifiers", (BYTE*)&pStatsModifiers - pThis, sizeof(pStatsModifiers), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "changesByEvent", &changesByEvent, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "SpeedBonus", (BYTE*)&fSpeedBonus - pThis, sizeof(fSpeedBonus), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "DispersionBonus", (BYTE*)&fDispersionBonus - pThis, sizeof(fDispersionBonus), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "FireRateBonus", (BYTE*)&fFireRateBonus - pThis, sizeof(fFireRateBonus), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "RelaxTimeBonus", (BYTE*)&fRelaxTimeBonus - pThis, sizeof(fRelaxTimeBonus), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "CoverBonus", (BYTE*)&fCoverBonus - pThis, sizeof(fCoverBonus), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "VisibleBonus", (BYTE*)&fVisibleBonus - pThis, sizeof(fVisibleBonus), NTypeDef::TYPE_TYPE_FLOAT );
}

int SSquadRPGStats::SFormation::operator&( IXmlSaver &saver )
{
	saver.Add( "type", &etype );
	saver.Add( "order", &order );
	saver.Add( "LieFlag", &nLieFlag );
	saver.Add( "StatsModifiers", &pStatsModifiers );
	saver.Add( "changesByEvent", &changesByEvent );
	saver.Add( "SpeedBonus", &fSpeedBonus );
	saver.Add( "DispersionBonus", &fDispersionBonus );
	saver.Add( "FireRateBonus", &fFireRateBonus );
	saver.Add( "RelaxTimeBonus", &fRelaxTimeBonus );
	saver.Add( "CoverBonus", &fCoverBonus );
	saver.Add( "VisibleBonus", &fVisibleBonus );

	return 0;
}

int SSquadRPGStats::SFormation::operator&( IBinSaver &saver )
{
	saver.Add( 2, &etype );
	saver.Add( 3, &order );
	saver.Add( 4, &nLieFlag );
	saver.Add( 5, &pStatsModifiers );
	saver.Add( 6, &changesByEvent );
	saver.Add( 7, &fSpeedBonus );
	saver.Add( 8, &fDispersionBonus );
	saver.Add( 9, &fFireRateBonus );
	saver.Add( 10, &fRelaxTimeBonus );
	saver.Add( 11, &fCoverBonus );
	saver.Add( 12, &fVisibleBonus );

	return 0;
}

DWORD SSquadRPGStats::SFormation::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << etype << order << nLieFlag << pStatsModifiers << changesByEvent << fSpeedBonus << fDispersionBonus << fFireRateBonus << fRelaxTimeBonus << fCoverBonus << fVisibleBonus;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SSquadRPGStats::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "SquadRPGStats", typeID, sizeof(*this) );
	SHPObjectRPGStats::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "SquadType", (BYTE*)&eSquadType - pThis, sizeof(eSquadType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Icon", (BYTE*)&szIcon - pThis, sizeof(szIcon), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "type", (BYTE*)&etype - pThis, sizeof(etype), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportSimpleArrayMetaInfo( "members", &members, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "formations", &formations, pThis );
	NMetaInfo::ReportMetaInfo( "availCommands", (BYTE*)&availCommands - pThis, sizeof(availCommands), NTypeDef::TYPE_TYPE_BINARY );
	NMetaInfo::ReportMetaInfo( "availExposures", (BYTE*)&availExposures - pThis, sizeof(availExposures), NTypeDef::TYPE_TYPE_BINARY );
	NMetaInfo::ReportStructMetaInfo( "availUserActions", &availUserActions, pThis );
	NMetaInfo::ReportStructMetaInfo( "availUserExposures", &availUserExposures, pThis );
	NMetaInfo::ReportMetaInfo( "ReinforcementPrice", (BYTE*)&fReinforcementPrice - pThis, sizeof(fReinforcementPrice), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "EntrenchCover", (BYTE*)&fEntrenchCover - pThis, sizeof(fEntrenchCover), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SSquadRPGStats::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SHPObjectRPGStats*)(this) );
	saver.Add( "SquadType", &eSquadType );
	saver.Add( "Icon", &szIcon );
	saver.Add( "type", &etype );
	saver.Add( "members", &members );
	saver.Add( "formations", &formations );
	saver.Add( "availCommands", &availCommands );
	saver.Add( "availExposures", &availExposures );
	saver.Add( "availUserActions", &availUserActions );
	saver.Add( "availUserExposures", &availUserExposures );
	saver.Add( "ReinforcementPrice", &fReinforcementPrice );
	saver.Add( "EntrenchCover", &fEntrenchCover );

	return 0;
}

int SSquadRPGStats::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SHPObjectRPGStats*)this );
	saver.Add( 2, &eSquadType );
	saver.Add( 3, &szIcon );
	saver.Add( 4, &etype );
	saver.Add( 5, &members );
	saver.Add( 6, &formations );
	saver.Add( 7, &availCommands );
	saver.Add( 8, &availExposures );
	saver.Add( 9, &availUserActions );
	saver.Add( 10, &availUserExposures );
	saver.Add( 11, &fReinforcementPrice );
	saver.Add( 12, &fEntrenchCover );

	return 0;
}

DWORD SSquadRPGStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SHPObjectRPGStats::CalcCheckSum() << eSquadType << szIcon << etype << members << formations << availCommands << availExposures << availUserActions << availUserExposures << fReinforcementPrice << fEntrenchCover;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


std::string EnumToString( NDb::EReinforcementType eValue )
{
	switch ( eValue )
	{
	case NDb::RT_MAIN_INFANTRY:
		return "RT_MAIN_INFANTRY";
	case NDb::RT_ASSAULT_INFANTRY:
		return "RT_ASSAULT_INFANTRY";
	case NDb::RT_ELITE_INFANTRY:
		return "RT_ELITE_INFANTRY";
	case NDb::RT_ARTILLERY_ANTITANK:
		return "RT_ARTILLERY_ANTITANK";
	case NDb::RT_ARTILLERY:
		return "RT_ARTILLERY";
	case NDb::RT_ASSAULT_GUNS:
		return "RT_ASSAULT_GUNS";
	case NDb::RT_TANK_DESTROYERS:
		return "RT_TANK_DESTROYERS";
	case NDb::RT_ARTILLERY_ROCKET:
		return "RT_ARTILLERY_ROCKET";
	case NDb::RT_LIGHT_TANKS:
		return "RT_LIGHT_TANKS";
	case NDb::RT_TANKS:
		return "RT_TANKS";
	case NDb::RT_HEAVY_TANKS:
		return "RT_HEAVY_TANKS";
	case NDb::RT_LIGHT_AAA:
		return "RT_LIGHT_AAA";
	case NDb::RT_HEAVY_AAA:
		return "RT_HEAVY_AAA";
	case NDb::RT_FIGHTERS:
		return "RT_FIGHTERS";
	case NDb::RT_BOMBERS:
		return "RT_BOMBERS";
	case NDb::RT_GROUND_ATTACK_PLANES:
		return "RT_GROUND_ATTACK_PLANES";
	case NDb::RT_RECON:
		return "RT_RECON";
	case NDb::RT_PARATROOPS:
		return "RT_PARATROOPS";
	case NDb::RT_ENGINEERING:
		return "RT_ENGINEERING";
	case NDb::RT_HEAVY_ARTILLERY:
		return "RT_HEAVY_ARTILLERY";
	case NDb::RT_SUPER_WEAPON:
		return "RT_SUPER_WEAPON";
	case NDb::RT_EXTRA_GROUND_1:
    	return "RT_EXTRA_GROUND_1";
	case NDb::RT_EXTRA_GROUND_2:
		return "RT_EXTRA_GROUND_2";
	case NDb::RT_EXTRA_GROUND_3:
		return "RT_EXTRA_GROUND_3";
	case NDb::RT_EXTRA_GROUND_4:
		return "RT_EXTRA_GROUND_4";
	case NDb::RT_EXTRA_GROUND_5:
		return "RT_EXTRA_GROUND_5";
	case NDb::RT_EXTRA_GROUND_6:
		return "RT_EXTRA_GROUND_6";
	case NDb::RT_EXTRA_GROUND_7:
		return "RT_EXTRA_GROUND_7";
	case NDb::RT_EXTRA_GROUND_8:
		return "RT_EXTRA_GROUND_8";
	case NDb::RT_EXTRA_GROUND_9:
		return "RT_EXTRA_GROUND_9";
	case NDb::RT_EXTRA_GROUND_10:
		return "RT_EXTRA_GROUND_10";
	case NDb::RT_EXTRA_AIR_1:
		return "RT_EXTRA_AIR_1";
	case NDb::RT_EXTRA_AIR_2:
		return "RT_EXTRA_AIR_2";
	case NDb::RT_EXTRA_AIR_3:
		return "RT_EXTRA_AIR_3";
	case NDb::RT_EXTRA_AIR_4:
		return "RT_EXTRA_AIR_4";
	case NDb::RT_EXTRA_AIR_5:
		return "RT_EXTRA_AIR_5";
	case NDb::RT_EXTRA_MIXED_1:
		return "RT_EXTRA_MIXED_1";
	case NDb::RT_EXTRA_MIXED_2:
		return "RT_EXTRA_MIXED_2";
	case NDb::RT_EXTRA_MIXED_3:
		return "RT_EXTRA_MIXED_3";
	case NDb::RT_EXTRA_MIXED_4:
		return "RT_EXTRA_MIXED_4";
	case NDb::RT_EXTRA_MIXED_5:
		return "RT_EXTRA_MIXED_5";
	case NDb::RT_EXTRA_MAXLVL_GROUND_1:
		return "RT_EXTRA_MAXLVL_GROUND_1";
	case NDb::RT_EXTRA_MAXLVL_GROUND_2:
		return "RT_EXTRA_MAXLVL_GROUND_2";
	case NDb::RT_EXTRA_MAXLVL_GROUND_3:
		return "RT_EXTRA_MAXLVL_GROUND_3";
	case NDb::RT_EXTRA_MAXLVL_GROUND_4:
		return "RT_EXTRA_MAXLVL_GROUND_4";
	case NDb::RT_EXTRA_MAXLVL_GROUND_5:
		return "RT_EXTRA_MAXLVL_GROUND_5";
	case NDb::RT_EXTRA_MAXLVL_AIR_1:
		return "RT_EXTRA_MAXLVL_AIR_1";
	case NDb::RT_EXTRA_MAXLVL_AIR_2:
		return "RT_EXTRA_MAXLVL_AIR_2";
	case NDb::RT_EXTRA_MAXLVL_AIR_3:
		return "RT_EXTRA_MAXLVL_AIR_3";
	case NDb::RT_EXTRA_MAXLVL_AIR_4:
		return "RT_EXTRA_MAXLVL_AIR_4";
	case NDb::RT_EXTRA_MAXLVL_AIR_5:
		return "RT_EXTRA_MAXLVL_AIR_5";
	case NDb::RT_EXTRA_MAXLVL_MIXED_1:
		return "RT_EXTRA_MAXLVL_MIXED_1";
	case NDb::RT_EXTRA_MAXLVL_MIXED_2:
		return "RT_EXTRA_MAXLVL_MIXED_2";
	case NDb::RT_EXTRA_MAXLVL_MIXED_3:
		return "RT_EXTRA_MAXLVL_MIXED_3";
	case NDb::RT_EXTRA_MAXLVL_MIXED_4:
		return "RT_EXTRA_MAXLVL_MIXED_4";
	case NDb::RT_EXTRA_MAXLVL_MIXED_5:
		return "RT_EXTRA_MAXLVL_MIXED_5";
	case NDb::_RT_NONE:
		return "_RT_NONE";
	default:
		return "RT_MAIN_INFANTRY";
	}
}

NDb::EReinforcementType NDb::StringToEnum_NDb_EReinforcementType( const std::string &szValue )
{
	if ( szValue == "RT_MAIN_INFANTRY" )
		return NDb::RT_MAIN_INFANTRY;
	if ( szValue == "RT_ASSAULT_INFANTRY" )
		return NDb::RT_ASSAULT_INFANTRY;
	if ( szValue == "RT_ELITE_INFANTRY" )
		return NDb::RT_ELITE_INFANTRY;
	if ( szValue == "RT_ARTILLERY_ANTITANK" )
		return NDb::RT_ARTILLERY_ANTITANK;
	if ( szValue == "RT_ARTILLERY" )
		return NDb::RT_ARTILLERY;
	if ( szValue == "RT_ASSAULT_GUNS" )
		return NDb::RT_ASSAULT_GUNS;
	if ( szValue == "RT_TANK_DESTROYERS" )
		return NDb::RT_TANK_DESTROYERS;
	if ( szValue == "RT_ARTILLERY_ROCKET" )
		return NDb::RT_ARTILLERY_ROCKET;
	if ( szValue == "RT_LIGHT_TANKS" )
		return NDb::RT_LIGHT_TANKS;
	if ( szValue == "RT_TANKS" )
		return NDb::RT_TANKS;
	if ( szValue == "RT_HEAVY_TANKS" )
		return NDb::RT_HEAVY_TANKS;
	if ( szValue == "RT_LIGHT_AAA" )
		return NDb::RT_LIGHT_AAA;
	if ( szValue == "RT_HEAVY_AAA" )
		return NDb::RT_HEAVY_AAA;
	if ( szValue == "RT_FIGHTERS" )
		return NDb::RT_FIGHTERS;
	if ( szValue == "RT_BOMBERS" )
		return NDb::RT_BOMBERS;
	if ( szValue == "RT_GROUND_ATTACK_PLANES" )
		return NDb::RT_GROUND_ATTACK_PLANES;
	if ( szValue == "RT_RECON" )
		return NDb::RT_RECON;
	if ( szValue == "RT_PARATROOPS" )
		return NDb::RT_PARATROOPS;
	if ( szValue == "RT_ENGINEERING" )
		return NDb::RT_ENGINEERING;
	if ( szValue == "RT_HEAVY_ARTILLERY" )
		return NDb::RT_HEAVY_ARTILLERY;
	if ( szValue == "RT_SUPER_WEAPON" )
		return NDb::RT_SUPER_WEAPON;
	if ( szValue == "RT_EXTRA_GROUND_1" )
    	return NDb::RT_EXTRA_GROUND_1;
	if ( szValue == "RT_EXTRA_GROUND_2" )
		return NDb::RT_EXTRA_GROUND_2;
	if ( szValue == "RT_EXTRA_GROUND_3" )
		return NDb::RT_EXTRA_GROUND_3;
	if ( szValue == "RT_EXTRA_GROUND_4" )
		return NDb::RT_EXTRA_GROUND_4;
	if ( szValue == "RT_EXTRA_GROUND_5" )
		return NDb::RT_EXTRA_GROUND_5;
	if ( szValue == "RT_EXTRA_GROUND_6" )
		return NDb::RT_EXTRA_GROUND_6;
	if ( szValue == "RT_EXTRA_GROUND_7" )
		return NDb::RT_EXTRA_GROUND_7;
	if ( szValue == "RT_EXTRA_GROUND_8" )
		return NDb::RT_EXTRA_GROUND_8;
	if ( szValue == "RT_EXTRA_GROUND_9" )
		return NDb::RT_EXTRA_GROUND_9;
	if ( szValue == "RT_EXTRA_GROUND_10" )
		return NDb::RT_EXTRA_GROUND_10;
	if ( szValue == "RT_EXTRA_AIR_1" )
		return NDb::RT_EXTRA_AIR_1;
	if ( szValue == "RT_EXTRA_AIR_2" )
		return NDb::RT_EXTRA_AIR_2;
	if ( szValue == "RT_EXTRA_AIR_3" )
		return NDb::RT_EXTRA_AIR_3;
	if ( szValue == "RT_EXTRA_AIR_4" )
		return NDb::RT_EXTRA_AIR_4;
	if ( szValue == "RT_EXTRA_AIR_5" )
		return NDb::RT_EXTRA_AIR_5;
	if ( szValue == "RT_EXTRA_MIXED_1" )
		return NDb::RT_EXTRA_MIXED_1;
	if ( szValue == "RT_EXTRA_MIXED_2" )
		return NDb::RT_EXTRA_MIXED_2;
	if ( szValue == "RT_EXTRA_MIXED_3" )
		return NDb::RT_EXTRA_MIXED_3;
	if ( szValue == "RT_EXTRA_MIXED_4" )
		return NDb::RT_EXTRA_MIXED_4;
	if ( szValue == "RT_EXTRA_MIXED_5" )
		return NDb::RT_EXTRA_MIXED_5;
	if ( szValue == "RT_EXTRA_MAXLVL_GROUND_1" )
		return NDb::RT_EXTRA_MAXLVL_GROUND_1;
	if ( szValue == "RT_EXTRA_MAXLVL_GROUND_2" )
		return NDb::RT_EXTRA_MAXLVL_GROUND_2;
	if ( szValue == "RT_EXTRA_MAXLVL_GROUND_3" )
		return NDb::RT_EXTRA_MAXLVL_GROUND_3;
	if ( szValue == "RT_EXTRA_MAXLVL_GROUND_4" )
		return NDb::RT_EXTRA_MAXLVL_GROUND_4;
	if ( szValue == "RT_EXTRA_MAXLVL_GROUND_5" )
		return NDb::RT_EXTRA_MAXLVL_GROUND_5;
	if ( szValue == "RT_EXTRA_MAXLVL_AIR_1" )
		return NDb::RT_EXTRA_MAXLVL_AIR_1;
	if ( szValue == "RT_EXTRA_MAXLVL_AIR_2" )
		return NDb::RT_EXTRA_MAXLVL_AIR_2;
	if ( szValue == "RT_EXTRA_MAXLVL_AIR_3" )
		return NDb::RT_EXTRA_MAXLVL_AIR_3;
	if ( szValue == "RT_EXTRA_MAXLVL_AIR_4" )
		return NDb::RT_EXTRA_MAXLVL_AIR_4;
	if ( szValue == "RT_EXTRA_MAXLVL_AIR_5" )
		return NDb::RT_EXTRA_MAXLVL_AIR_5;
	if ( szValue == "RT_EXTRA_MAXLVL_MIXED_1" )
		return NDb::RT_EXTRA_MAXLVL_MIXED_1;
	if ( szValue == "RT_EXTRA_MAXLVL_MIXED_2" )
		return NDb::RT_EXTRA_MAXLVL_MIXED_2;
	if ( szValue == "RT_EXTRA_MAXLVL_MIXED_3" )
		return NDb::RT_EXTRA_MAXLVL_MIXED_3;
	if ( szValue == "RT_EXTRA_MAXLVL_MIXED_4" )
		return NDb::RT_EXTRA_MAXLVL_MIXED_4;
	if ( szValue == "RT_EXTRA_MAXLVL_MIXED_5" )
		return NDb::RT_EXTRA_MAXLVL_MIXED_5;
	if ( szValue == "_RT_NONE" )
		return NDb::_RT_NONE;
	return NDb::RT_MAIN_INFANTRY;
}


void SReinforcementEntry::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "MechUnit", (BYTE*)&pMechUnit - pThis, sizeof(pMechUnit), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Squad", (BYTE*)&pSquad - pThis, sizeof(pSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "LinkIndex", (BYTE*)&nLinkIndex - pThis, sizeof(nLinkIndex), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "LinkWith", (BYTE*)&nLinkWith - pThis, sizeof(nLinkWith), NTypeDef::TYPE_TYPE_INT );
}

int SReinforcementEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "MechUnit", &pMechUnit );
	saver.Add( "Squad", &pSquad );
	saver.Add( "LinkIndex", &nLinkIndex );
	saver.Add( "LinkWith", &nLinkWith );

	return 0;
}

int SReinforcementEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMechUnit );
	saver.Add( 3, &pSquad );
	saver.Add( 4, &nLinkIndex );
	saver.Add( 5, &nLinkWith );

	return 0;
}

DWORD SReinforcementEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pMechUnit << pSquad << nLinkIndex << nLinkWith;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SDeployTemplate::SDeployTemplateEntry::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Position", &vPosition, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&nDirection - pThis, sizeof(nDirection), NTypeDef::TYPE_TYPE_INT );
}

int SDeployTemplate::SDeployTemplateEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "Position", &vPosition );
	saver.Add( "Direction", &nDirection );

	return 0;
}

int SDeployTemplate::SDeployTemplateEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPosition );
	saver.Add( 3, &nDirection );

	return 0;
}

DWORD SDeployTemplate::SDeployTemplateEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPosition << nDirection;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SDeployTemplate::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "DeployTemplate", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Entries", &entries, pThis );
	NMetaInfo::ReportMetaInfo( "IsDefault", (BYTE*)&bIsDefault - pThis, sizeof(bIsDefault), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "ReinforcementType", (BYTE*)&eReinforcementType - pThis, sizeof(eReinforcementType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::FinishMetaInfoReport();
}

int SDeployTemplate::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Entries", &entries );
	saver.Add( "IsDefault", &bIsDefault );
	saver.Add( "ReinforcementType", &eReinforcementType );

	return 0;
}

int SDeployTemplate::operator&( IBinSaver &saver )
{
	saver.Add( 2, &entries );
	saver.Add( 3, &bIsDefault );
	saver.Add( 4, &eReinforcementType );

	return 0;
}

DWORD SDeployTemplate::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << entries << bIsDefault << eReinforcementType;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcement::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Reinforcement", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "IconTexture", (BYTE*)&pIconTexture - pThis, sizeof(pIconTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "LocalizedNameFileRef", (BYTE*)&szLocalizedNameFileRef - pThis, sizeof(szLocalizedNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructArrayMetaInfo( "Entries", &entries, pThis );
	NMetaInfo::ReportMetaInfo( "TooltipFileRef", (BYTE*)&szTooltipFileRef - pThis, sizeof(szTooltipFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Transports", &transports, pThis );
	NMetaInfo::ReportMetaInfo( "LocalizedDescFileRef", (BYTE*)&szLocalizedDescFileRef - pThis, sizeof(szLocalizedDescFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "TemplateOverride", (BYTE*)&pTemplateOverride - pThis, sizeof(pTemplateOverride), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SReinforcement::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Type", &eType );
	saver.Add( "IconTexture", &pIconTexture );
	saver.Add( "LocalizedNameFileRef", &szLocalizedNameFileRef );
	saver.Add( "Entries", &entries );
	saver.Add( "TooltipFileRef", &szTooltipFileRef );
	saver.Add( "Transports", &transports );
	saver.Add( "LocalizedDescFileRef", &szLocalizedDescFileRef );
	saver.Add( "TemplateOverride", &pTemplateOverride );

	return 0;
}

int SReinforcement::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &pIconTexture );
	saver.Add( 4, &szLocalizedNameFileRef );
	saver.Add( 5, &entries );
	saver.Add( 6, &szTooltipFileRef );
	saver.Add( 7, &transports );
	saver.Add( 8, &szLocalizedDescFileRef );
	saver.Add( 9, &pTemplateOverride );

	return 0;
}

DWORD SReinforcement::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << entries << transports << pTemplateOverride;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SPlayerRank::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "PlayerRank", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "RankNameFileRef", (BYTE*)&szRankNameFileRef - pThis, sizeof(szRankNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Strap", (BYTE*)&pStrap - pThis, sizeof(pStrap), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Party", (BYTE*)&pParty - pThis, sizeof(pParty), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SPlayerRank::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "RankNameFileRef", &szRankNameFileRef );
	saver.Add( "Strap", &pStrap );
	saver.Add( "Party", &pParty );

	return 0;
}

int SPlayerRank::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szRankNameFileRef );
	saver.Add( 3, &pStrap );
	saver.Add( 4, &pParty );

	return 0;
}

DWORD SPlayerRank::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pParty;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementTypeInfo::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "TooltipFileRef", (BYTE*)&szTooltipFileRef - pThis, sizeof(szTooltipFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "UserAction", (BYTE*)&eUserAction - pThis, sizeof(eUserAction), NTypeDef::TYPE_TYPE_ENUM );
}

int SReinforcementTypeInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "TooltipFileRef", &szTooltipFileRef );
	saver.Add( "UserAction", &eUserAction );

	return 0;
}

int SReinforcementTypeInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &szTooltipFileRef );
	saver.Add( 4, &eUserAction );

	return 0;
}

DWORD SReinforcementTypeInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << eUserAction;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementTypes::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ReinforcementTypes", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "TypeInfo", &typeInfo, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SReinforcementTypes::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "TypeInfo", &typeInfo );

	return 0;
}

int SReinforcementTypes::operator&( IBinSaver &saver )
{
	saver.Add( 2, &typeInfo );

	return 0;
}

DWORD SReinforcementTypes::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << typeInfo;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAIExpLevel::SLevel::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Experience", (BYTE*)&fExperience - pThis, sizeof(fExperience), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "StatsBonus", (BYTE*)&pStatsBonus - pThis, sizeof(pStatsBonus), NTypeDef::TYPE_TYPE_REF );
}

int SAIExpLevel::SLevel::operator&( IXmlSaver &saver )
{
	saver.Add( "Experience", &fExperience );
	saver.Add( "StatsBonus", &pStatsBonus );

	return 0;
}

int SAIExpLevel::SLevel::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fExperience );
	saver.Add( 3, &pStatsBonus );

	return 0;
}

DWORD SAIExpLevel::SLevel::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fExperience << pStatsBonus;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAIExpLevel::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "AIExpLevel", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "TypeName", (BYTE*)&szTypeName - pThis, sizeof(szTypeName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "DBType", (BYTE*)&eDBType - pThis, sizeof(eDBType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructArrayMetaInfo( "levels", &levels, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SAIExpLevel::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "TypeName", &szTypeName );
	saver.Add( "DBType", &eDBType );
	saver.Add( "levels", &levels );

	return 0;
}

int SAIExpLevel::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szTypeName );
	saver.Add( 3, &eDBType );
	saver.Add( 4, &levels );

	return 0;
}

DWORD SAIExpLevel::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szTypeName << eDBType << levels;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUnitStatsModifier::SParameterModifier::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "AddBonus", (BYTE*)&fAddBonus - pThis, sizeof(fAddBonus), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "MultBonus", (BYTE*)&fMultBonus - pThis, sizeof(fMultBonus), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "ZeroCount", (BYTE*)&nZeroCount - pThis, sizeof(nZeroCount), NTypeDef::TYPE_TYPE_INT );
}

int SUnitStatsModifier::SParameterModifier::operator&( IXmlSaver &saver )
{
	saver.Add( "AddBonus", &fAddBonus );
	saver.Add( "MultBonus", &fMultBonus );
	saver.Add( "ZeroCount", &nZeroCount );

	return 0;
}

int SUnitStatsModifier::SParameterModifier::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fAddBonus );
	saver.Add( 3, &fMultBonus );
	saver.Add( 4, &nZeroCount );

	return 0;
}

DWORD SUnitStatsModifier::SParameterModifier::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fAddBonus << fMultBonus << nZeroCount;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUnitStatsModifier::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UnitStatsModifier", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "Durability", &durability, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "SmallAABBCoeff", &smallAABBCoeff, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Camouflage", &camouflage, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "SightPower", &sightPower, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "SightRange", &sightRange, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Speed", &speed, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "RotateSpeed", &rotateSpeed, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "WeaponDispersion", &weaponDispersion, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "WeaponDamage", &weaponDamage, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "WeaponPiercing", &weaponPiercing, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "WeaponTrackDamageProb", &weaponTrackDamageProb, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "WeaponRelaxTime", &weaponRelaxTime, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "WeaponAimTime", &weaponAimTime, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "WeaponShellSpeed", &weaponShellSpeed, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "WeaponArea", &weaponArea, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "WeaponArea2", &weaponArea2, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Cover", &cover, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SUnitStatsModifier::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Durability", &durability );
	saver.Add( "SmallAABBCoeff", &smallAABBCoeff );
	saver.Add( "Camouflage", &camouflage );
	saver.Add( "SightPower", &sightPower );
	saver.Add( "SightRange", &sightRange );
	saver.Add( "Speed", &speed );
	saver.Add( "RotateSpeed", &rotateSpeed );
	saver.Add( "WeaponDispersion", &weaponDispersion );
	saver.Add( "WeaponDamage", &weaponDamage );
	saver.Add( "WeaponPiercing", &weaponPiercing );
	saver.Add( "WeaponTrackDamageProb", &weaponTrackDamageProb );
	saver.Add( "WeaponRelaxTime", &weaponRelaxTime );
	saver.Add( "WeaponAimTime", &weaponAimTime );
	saver.Add( "WeaponShellSpeed", &weaponShellSpeed );
	saver.Add( "WeaponArea", &weaponArea );
	saver.Add( "WeaponArea2", &weaponArea2 );
	saver.Add( "Cover", &cover );

	return 0;
}

int SUnitStatsModifier::operator&( IBinSaver &saver )
{
	saver.Add( 2, &durability );
	saver.Add( 3, &smallAABBCoeff );
	saver.Add( 4, &camouflage );
	saver.Add( 5, &sightPower );
	saver.Add( 6, &sightRange );
	saver.Add( 7, &speed );
	saver.Add( 8, &rotateSpeed );
	saver.Add( 9, &weaponDispersion );
	saver.Add( 10, &weaponDamage );
	saver.Add( 11, &weaponPiercing );
	saver.Add( 12, &weaponTrackDamageProb );
	saver.Add( 13, &weaponRelaxTime );
	saver.Add( 14, &weaponAimTime );
	saver.Add( 15, &weaponShellSpeed );
	saver.Add( 16, &weaponArea );
	saver.Add( 17, &weaponArea2 );
	saver.Add( 18, &cover );

	return 0;
}

DWORD SUnitStatsModifier::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << durability << smallAABBCoeff << camouflage << sightPower << sightRange << speed << rotateSpeed << weaponDispersion << weaponDamage << weaponPiercing << weaponTrackDamageProb << weaponRelaxTime << weaponAimTime << weaponShellSpeed << weaponArea << weaponArea2 << cover;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x110832C0, SUnitSpecialAblityDesc ) 
BASIC_REGISTER_DATABASE_CLASS( SCommonRPGStats )
REGISTER_DATABASE_CLASS( 0x15096402, SComplexEffect ) 
REGISTER_DATABASE_CLASS( 0x110CEC40, SComplexSeasonedEffect ) 
REGISTER_DATABASE_CLASS( 0x1106AC40, SAckSetRPGStats ) 
BASIC_REGISTER_DATABASE_CLASS( SHPObjectRPGStats )
REGISTER_DATABASE_CLASS( 0x111C33C0, SBurningFuel ) 
BASIC_REGISTER_DATABASE_CLASS( SStaticObjectRPGStats )
REGISTER_DATABASE_CLASS( 0x120AEBC0, SCraterSet ) 
REGISTER_DATABASE_CLASS( 0x300C3B80, SProjectile ) 
REGISTER_DATABASE_CLASS( 0x11069B82, SWeaponRPGStats ) 
REGISTER_DATABASE_CLASS( 0x140BAB41, SDynamicDebrisSet ) 
BASIC_REGISTER_DATABASE_CLASS( SObjectBaseRPGStats )
REGISTER_DATABASE_CLASS( 0x11069BC1, STerraObjSetRPGStats ) 
REGISTER_DATABASE_CLASS( 0x11069BC4, SObjectRPGStats ) 
REGISTER_DATABASE_CLASS( 0x11069BC9, SBuildingRPGStats ) 
REGISTER_DATABASE_CLASS( 0x11069BCA, SBridgeRPGStats ) 
REGISTER_DATABASE_CLASS( 0x11069BC8, SEntrenchmentRPGStats ) 
REGISTER_DATABASE_CLASS( 0x11069BC7, SFenceRPGStats ) 
REGISTER_DATABASE_CLASS( 0x11069BC5, SMineRPGStats ) 
REGISTER_DATABASE_CLASS( 0x11141380, SUnitActions ) 
BASIC_REGISTER_DATABASE_CLASS( SUnitBaseRPGStats )
REGISTER_DATABASE_CLASS( 0x1711A341, SArmorPatternPlacement ) 
REGISTER_DATABASE_CLASS( 0x1711A340, SArmorPattern ) 
REGISTER_DATABASE_CLASS( 0x11069B81, SInfantryRPGStats ) 
REGISTER_DATABASE_CLASS( 0x11069B80, SMechUnitRPGStats ) 
REGISTER_DATABASE_CLASS( 0x11069BC2, SSquadRPGStats ) 
REGISTER_DATABASE_CLASS( 0x120C4CC0, SDeployTemplate ) 
REGISTER_DATABASE_CLASS( 0x120A6B80, SReinforcement ) 
REGISTER_DATABASE_CLASS( 0x120C5B00, SPlayerRank ) 
REGISTER_DATABASE_CLASS( 0x17146480, SReinforcementTypes ) 
REGISTER_DATABASE_CLASS( 0x11069BCC, SAIExpLevel ) 
REGISTER_DATABASE_CLASS( 0x19126C00, SUnitStatsModifier ) 

