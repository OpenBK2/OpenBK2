#pragma once

#include "AILogic_export.h"

#include <cstdint>

namespace NDb
{
	struct SPassProfile;
}

namespace NAIVisInfo
{
	AILOGIC_EXPORT void AddProfile( const int nID, const CVec3 &vCenter, const uint16_t wDir, const NDb::SPassProfile &profile );
	AILOGIC_EXPORT void RemoveProfile( const int nID );
	AILOGIC_EXPORT void ToggleLockProfiles();
}


