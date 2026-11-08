#pragma once

#include "GameX_export.h"

#include <string>

namespace NDb
{
struct SMultiplayerConsts;
}

namespace NGameX
{
struct SMPConnectionSettings
{
	std::string szServerAddress;
	int nServerPort;
	int nGameVersion;
};

// Resolve at connection creation so switching mods also switches their endpoint.
// Unset fields independently retain the profile setting and its original default.
GAMEX_EXPORT SMPConnectionSettings GetMPConnectionSettings( const NDb::SMultiplayerConsts *pConsts );
GAMEX_EXPORT int GetMPGameVersion( const NDb::SMultiplayerConsts *pConsts );
} // namespace NGameX
