#pragma once

#include "Stats_B2_M1/RPGStats.h"

#include "AILogic_export.h"

namespace NHitProtection
{
// These bypass only the named protection; geometry, armor and damage keep their own checks.
AILOGIC_EXPORT bool CracksAABB( const NDb::SWeaponRPGStats::SShell &shell, const NDb::SUnitBaseRPGStats &target );
AILOGIC_EXPORT bool CracksCover( const NDb::SWeaponRPGStats::SShell &shell, const NDb::SUnitBaseRPGStats &target );
// Resistance-based splash replaces the legacy armor threshold with this independent gate.
AILOGIC_EXPORT bool CracksAreaResistance( const NDb::SWeaponRPGStats::SShell &shell,
                                          const NDb::SUnitBaseRPGStats &target );
} // namespace NHitProtection
