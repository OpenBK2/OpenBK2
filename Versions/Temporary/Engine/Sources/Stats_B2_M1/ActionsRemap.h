#pragma once

#include "RPGStats.h"

#include "Stats_B2_M1_export.h"

STATS_B2_M1_EXPORT NDb::EUnitSpecialAbility GetAbilityByCommand( EActionCommand actionCommand );
STATS_B2_M1_EXPORT EActionCommand GetCommandByAbility( NDb::EUnitSpecialAbility specialAbility );
NDb::EUserAction GetActionByCommand( EActionCommand actionCommand );
STATS_B2_M1_EXPORT EActionCommand GetCommandByAction( NDb::EUserAction action );
STATS_B2_M1_EXPORT NDb::EUserAction GetActionByAbility( NDb::EUnitSpecialAbility specialAbility );
