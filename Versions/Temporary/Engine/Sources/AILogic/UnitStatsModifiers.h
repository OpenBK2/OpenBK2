#pragma once

#include "Stats_B2_M1/RPGStats.h"
#include "System/Basic.h"
#include "System/DB.h"
#include "System/det_map.h"

#include "AILogic_export.h"

#include <string>

namespace NStatsModifierSource
{
inline constexpr char Formation[] = "formation";
inline constexpr char Building[] = "shelter.building";
inline constexpr char Trench[] = "shelter.trench";
inline constexpr char TankPit[] = "shelter.tank_pit";
inline constexpr char Transport[] = "transport";
inline constexpr char GunCrew[] = "gun_crew";
inline constexpr char Water[] = "environment.water";
inline constexpr char Night[] = "environment.night";
inline constexpr char Weather[] = "environment.bad_weather";
inline constexpr char Experience[] = "experience";
inline constexpr char Leader[] = "leader";
inline constexpr char Difficulty[] = "difficulty";
inline constexpr char Chapter[] = "chapter";
inline constexpr char BalanceTest[] = "balance_test";
inline constexpr char AirAttack[] = "air_attack";
inline constexpr char AntiAviation[] = "shot.anti_aviation";

AILOGIC_EXPORT std::string Ability( NDb::EUnitSpecialAbility eAbility );
} // namespace NStatsModifierSource

// Sources own the state; the combined modifier is only a cache for frequent stat queries.
class AILOGIC_EXPORT CUnitStatsModifiers
{
	det_map<std::string, CDBPtr<NDb::SUnitStatsModifier>> sources;
	mutable CObj<NDb::SUnitStatsModifier> pCombined;

	void Rebuild() const;

public:
	void Set( const std::string &szSource, const NDb::SUnitStatsModifier *pModifier );
	void Remove( const std::string &szSource );
	void Clear();
	const NDb::SUnitStatsModifier *Find( const std::string &szSource ) const;
	const NDb::SUnitStatsModifier *GetCombined() const;
	int operator&( IBinSaver &saver );
};

class CAIUnit;

// Restore the previous source on scope exit, including nested shots using the same source.
class AILOGIC_EXPORT CScopedUnitStatsModifier
{
	CAIUnit *pUnit;
	std::string szSource;
	CDBPtr<NDb::SUnitStatsModifier> pPrevious;

public:
	CScopedUnitStatsModifier( CAIUnit *pUnit, const std::string &szSource, const NDb::SUnitStatsModifier *pModifier );
	~CScopedUnitStatsModifier();
	CScopedUnitStatsModifier( const CScopedUnitStatsModifier & ) = delete;
	CScopedUnitStatsModifier &operator=( const CScopedUnitStatsModifier & ) = delete;
};
