#pragma once

#include "AILogic_export.h"

class CCommonUnit;

namespace NDb
{
struct SAIGameConsts;
}

// Presentation-only gain. The observer is explicit; no unit or simulation state is changed.
AILOGIC_EXPORT float GetUnitFOWSoundVolume( const CCommonUnit &unit, int nParty, const NDb::SAIGameConsts *pConsts );
