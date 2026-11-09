#include "stdafx.h"

#include "AIUnit.h"
#include "UnitStatsModifiers.h"

#include <string>

std::string NStatsModifierSource::Ability( NDb::EUnitSpecialAbility eAbility )
{
	// Keep ability keys independent of localization and of the modifier resource they reference.
	switch ( eAbility )
	{
		case NDb::ABILITY_EXACT_SHOT:
			return "ability.exact_shot";
		case NDb::ABILITY_CRITICAL_TARGETING:
			return "ability.critical_targeting";
		case NDb::ABILITY_RAPID_SHOT_MODE:
			return "ability.rapid_shot";
		case NDb::ABILITY_HOLD_SECTOR:
			return "ability.hold_sector";
		case NDb::ABILITY_TRACK_TARGETING:
			return "ability.track_targeting";
		case NDb::ABILITY_HIGH_PASSABLENESS_MODE:
			return "ability.high_passableness";
		case NDb::ABILITY_MANUVERABLE_FIGHT_MODE:
			return "ability.moving_fire";
		case NDb::ABILITY_RAPID_FIRE_MODE:
			return "ability.rapid_fire";
		case NDb::ABILITY_ADRENALINE_RUSH:
			return "ability.adrenaline_rush";
		case NDb::ABILITY_CAMOFLAGE_MODE:
			return "ability.camouflage";
		case NDb::ABILITY_ADAVNCED_CAMOFLAGE_MODE:
			return "ability.advanced_camouflage";
		case NDb::ABILITY_SPY_MODE:
			return "ability.spy_mode";
		case NDb::ABILITY_OVERLOAD_MODE:
			return "ability.overload";
		case NDb::ABILITY_SMOKE_SHOTS:
			return "ability.smoke_shots";
		case NDb::ABILITY_ANTIATRILLERY_FIRE:
			return "ability.counter_fire";
		case NDb::ABILITY_ZEROING_IN:
			return "ability.zeroing_in";
		case NDb::ABILITY_SUPPORT_FIRE:
			return "ability.support_fire";
		case NDb::ABILITY_MASTER_OF_STREETS:
			return "ability.master_of_streets";
		case NDb::ABILITY_EXACT_BOMBING:
			return "ability.exact_bombing";
		case NDb::ABILITY_SKY_GUARD:
			return "ability.sky_guard";
		case NDb::ABILITY_SURVIVAL:
			return "ability.survival";
		case NDb::ABILITY_AMBUSH:
			return "ability.ambush";
		case NDb::ABILITY_CAUTION:
			return "ability.caution";
		case NDb::ABILITY_LINKED_GRENADES:
			return "ability.linked_grenades";
		default:
			return "ability.id." + std::to_string( int( eAbility ) );
	}
}

void CUnitStatsModifiers::Rebuild() const
{
	pCombined = new NDb::SUnitStatsModifier;
	// det_map preserves deterministic insertion order. Removal never uses inverse arithmetic,
	// so zero multipliers and overlapping modifiers can be removed without losing other sources.
	for ( const auto &source : sources )
	{
		if ( const NDb::SUnitStatsModifier *pModifier = source.second )
		{
			pCombined->Combine( *pModifier );
		}
	}
}

void CUnitStatsModifiers::Set( const std::string &szSource, const NDb::SUnitStatsModifier *pModifier )
{
	if ( !pModifier )
	{
		Remove( szSource );
		return;
	}
	// Replacing an existing source preserves its position and never stacks another copy.
	sources.insert_or_assign( szSource, pModifier );
	Rebuild();
}

void CUnitStatsModifiers::Remove( const std::string &szSource )
{
	if ( sources.erase( szSource ) )
	{
		Rebuild();
	}
}

void CUnitStatsModifiers::Clear()
{
	sources.clear();
	pCombined = nullptr;
}

const NDb::SUnitStatsModifier *CUnitStatsModifiers::Find( const std::string &szSource ) const
{
	const auto found = sources.find( szSource );
	return found != sources.end() ? found->second.GetPtr() : nullptr;
}

const NDb::SUnitStatsModifier *CUnitStatsModifiers::GetCombined() const
{
	if ( !pCombined )
	{
		Rebuild();
	}
	return pCombined;
}

int CUnitStatsModifiers::operator&( IBinSaver &saver )
{
	// Save sources, including their insertion order, and derive the cache after loading.
	if ( saver.IsReading() )
	{
		Clear();
	}
	saver.Add( 1, &sources );
	return 0;
}

CScopedUnitStatsModifier::CScopedUnitStatsModifier( CAIUnit *_pUnit,
                                                    const std::string &_szSource,
                                                    const NDb::SUnitStatsModifier *pModifier )
	: pUnit( pModifier ? _pUnit : nullptr )
	, szSource( _szSource )
{
	if ( pUnit )
	{
		pPrevious = pUnit->FindStatsModifier( szSource );
		pUnit->SetStatsModifier( szSource, pModifier );
	}
}

CScopedUnitStatsModifier::~CScopedUnitStatsModifier()
{
	if ( pUnit )
	{
		pUnit->SetStatsModifier( szSource, pPrevious );
	}
}
