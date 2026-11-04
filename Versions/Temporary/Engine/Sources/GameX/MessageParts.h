#pragma once

#include "GameX_export.h"

#include <string>

namespace NDb
{
struct SUIConstsB2;
}

namespace NGameX
{
// Explicit message values override general UIConsts Tags. Empty/unreadable files
// fall through, so partial translations can still use older data or English.
GAMEX_EXPORT const std::wstring &FindMessagePart( const NDb::SUIConstsB2 *pConsts, const std::wstring &wszName );

// The regular <val> handler uses this lookup, including built-in English backups.
GAMEX_EXPORT const std::wstring &GetMessagePart( const NDb::SUIConstsB2 *pConsts, const std::wstring &wszName );
} // namespace NGameX
