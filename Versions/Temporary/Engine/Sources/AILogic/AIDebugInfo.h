#pragma once

#include "AILogic_export.h"

namespace NDb
{
	struct SPassProfile;
}

namespace NAIVisInfo
{
	AILOGIC_EXPORT void AddProfile( const int nID, const CVec3 &vCenter, const WORD wDir, const NDb::SPassProfile &profile );
	AILOGIC_EXPORT void RemoveProfile( const int nID );
	AILOGIC_EXPORT void ToggleLockProfiles();
}


