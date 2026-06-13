#include "stdafx.h"

#include "rpgstatsautomagic.h"
#include "UnitTypes.h"

REGISTER_SAVELOAD_CLASS( 0x11078380, CRPGStatsAutomagic )

int CRAP_LinkerTooSmart_FeedBackUpdates();


CRPGStatsAutomagic::CRPGStatsAutomagic()
{
	// infantry
	i2s[NDb::RPG_TYPE_SOLDIER] = "soldier";
	i2s[NDb::RPG_TYPE_ENGINEER] = "engineer";
	i2s[NDb::RPG_TYPE_SNIPER] = "sniper";
	i2s[NDb::RPG_TYPE_OFFICER] = "officer";
	// transport
	i2s[NDb::RPG_TYPE_TRN_CARRIER] = "trn_carrier";
	i2s[NDb::RPG_TYPE_TRN_SUPPORT] = "trn_support";
	i2s[NDb::RPG_TYPE_TRN_MEDICINE] = "trn_medicine";
	i2s[NDb::RPG_TYPE_TRN_TRACTOR] = "trn_tractor";
	i2s[NDb::RPG_TYPE_TRN_MILITARY_AUTO] = "trn_military_auto";
	i2s[NDb::RPG_TYPE_TRN_CIVILIAN_AUTO] = "trn_civilian_auto";
	// artillery
	i2s[NDb::RPG_TYPE_ART_GUN] = "art_gun";
	i2s[NDb::RPG_TYPE_ART_HOWITZER] = "art_howitzer";
	i2s[NDb::RPG_TYPE_ART_HEAVY_GUN] = "art_heavy_gun";
	i2s[NDb::RPG_TYPE_ART_AAGUN] = "art_aagun";
	i2s[NDb::RPG_TYPE_ART_ROCKET] = "art_rocket";
	i2s[NDb::RPG_TYPE_ART_SUPER] = "art_super";
	i2s[NDb::RPG_TYPE_ART_MORTAR] = "art_mortar";
	i2s[NDb::RPG_TYPE_ART_HEAVY_MG] = "art_heavy_mg";
	// SPG
	i2s[NDb::RPG_TYPE_SPG_ASSAULT] = "spg_assault";
	i2s[NDb::RPG_TYPE_SPG_ANTITANK] = "spg_antitank";
	i2s[NDb::RPG_TYPE_SPG_SUPER] = "spg_super";
	i2s[NDb::RPG_TYPE_SPG_AAGUN] = "spg_aagun";
	// armor
	i2s[NDb::RPG_TYPE_ARM_LIGHT] = "arm_light";
	i2s[NDb::RPG_TYPE_ARM_MEDIUM] = "arm_medium";
	i2s[NDb::RPG_TYPE_ARM_SUPER] = "arm_super";
	i2s[NDb::RPG_TYPE_ARM_HEAVY] = "arm_heavy";
	// aviation
	i2s[NDb::RPG_TYPE_AVIA_SCOUT] = "avia_scout";
	i2s[NDb::RPG_TYPE_AVIA_BOMBER] = "avia_bomber";
	i2s[NDb::RPG_TYPE_AVIA_ATTACK] = "avia_attack";
	i2s[NDb::RPG_TYPE_AVIA_FIGHTER] = "avia_fighter";
	i2s[NDb::RPG_TYPE_AVIA_SUPER] = "avia_super";
	i2s[NDb::RPG_TYPE_AVIA_LANDER] = "avia_lander";
	i2s[NDb::RPG_TYPE_AVIA_STRAT_BOMBER] = "avia_strat_bomber";
	i2s[NDb::RPG_TYPE_AVIA_HELICOPTER] = "avia_helicopter";
	// train
	i2s[NDb::RPG_TYPE_TRAIN_LOCOMOTIVE] = "train_locomotive";
	i2s[NDb::RPG_TYPE_TRAIN_CARGO] = "train_cargo";
	i2s[NDb::RPG_TYPE_TRAIN_CARRIER] = "train_carrier";
	i2s[NDb::RPG_TYPE_TRAIN_SUPER] = "train_super";
	i2s[NDb::RPG_TYPE_TRAIN_ARMOR] = "train_armor";
	//
	// AI classes
	i2s[0x01] = "wheel";
	i2s[0x08 | 0x10] = "water";
	i2s[0x02] = "track";
	i2s[0x04] = "human";
	//
	//
	for ( CI2SMap::const_iterator it = i2s.begin(); it != i2s.end(); ++it ) 
		s2i[it->second] = it->first;
	//
	int a = CRAP_LinkerTooSmart_FeedBackUpdates();
}

const char* CRPGStatsAutomagic::GetFirstStr() const
{
	return s2i.begin()->first.c_str();
}

const int CRPGStatsAutomagic::GetFirstInt() const
{
	return i2s.begin()->first;
}

bool CRPGStatsAutomagic::IsLastStr( const char* pszVal ) const
{
	CS2IMap::const_iterator iter = s2i.find( pszVal );
	return iter == s2i.end() || ++iter == s2i.end();
}

bool CRPGStatsAutomagic::IsLastInt( const int nVal ) const
{
	CI2SMap::const_iterator iter = i2s.find( nVal );
	return iter == i2s.end() || ++iter == i2s.end();
}

const char* CRPGStatsAutomagic::GetNextStr( const char* pszVal )
{
	if ( IsLastStr( pszVal ) )
		return szUnknown.c_str();
	else
	{
		CS2IMap::const_iterator iter = s2i.find( pszVal );
		return (++iter)->first.c_str();
	}
}

const int CRPGStatsAutomagic::GetNextInt( const int nVal )
{
	if ( IsLastInt( nVal ) )
		return -1;
	else
	{
		CI2SMap::const_iterator iter = i2s.find( nVal );
		return (++iter)->first;
	}
}

static CPtr<IRPGStatsAutomagic> pAutomagic = new CRPGStatsAutomagic();

int StatsB2M1LinkCheatFunction()
{
	return 0;
}

